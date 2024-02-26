/*
 * pynets.cc
 *
 * by Joseph Heled <joseph@gnubg.org>, 2000
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "Python.h"

#include "pynets.h"
#include "pygnubg.h"
#include "defs.h"

using namespace BG;

extern "C" {
#include <eval.h>
#include <inputs.h>
};

struct NetObject : PyObject {
  EvalNets_*	net;
};


static void
net_dealloc(NetObject* const self)
{
  if( self->net ) {
    DestroyNets(self->net);
  }
  PyObject_Del(self);
}

PyTypeObject Net_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"bgnet.net", 		/*tp_name*/
	sizeof(NetObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)net_dealloc, /*tp_dealloc*/
	0,			 /*tp_print*/
	0, 			 /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        0,                      /*tp_getattro*/
        0,                      /*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        0,                      /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        0,                      /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/

	0,			/*tp_bases*/
	0,			/*tp_mro*/
	0,			/*tp_cache*/
	0,			/*tp_subclasses*/
	0,			/*tp_weaklist*/
#if PY_MINOR_VERSION >= 3
	0,                      /* tp_del */
#endif
};

static inline bool
isNet(const PyObject* const o)
{
  return o->ob_type == &Net_Type;
}


static PyObject*
net_get(PyObject*, PyObject* args)
{
  const char* name;
  long cacheSize = -1;
  
  if( !PyArg_ParseTuple(args, "s|l", &name, &cacheSize)) {
    return 0;
  }
		
  EvalNets_* n = LoadNet(name, cacheSize);
		
  if( n == 0 ) {
    PyErr_SetString(PyExc_RuntimeError, "failed to load net.") ;
    return 0;
  }

  NetObject* o = PyObject_New(NetObject, &Net_Type);
  
  if( o == 0 ) {
    return PyErr_NoMemory();
  }
 
  o->net = n;

  return o;
}

static PyObject*
net_save(PyObject*, PyObject* args)
{
  const char* name;
  int c = -1;
  
  if( !PyArg_ParseTuple(args, "s|i", &name, &c)) {
    return 0;
  }
  
  int const ok = EvalSave(name, c);

  if( ok != 0 ) {
    PyErr_SetString(PyExc_RuntimeError, "failed to save net.") ;
    return 0;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


static NetObject* current = 0;

static PyObject*
net_current(PyObject*, PyObject* args)
{
  if( PyTuple_Size(args) != 0 ) {
    return 0;
  }
  
  Py_INCREF(current);
  return current;
}

static PyObject*
net_set(PyObject*, PyObject* args)
{
  NetObject* n = 0;

  if( !PyArg_ParseTuple(args, "O", &n)) {
    return 0;
  }

  if( ! isNet(n) ) {
    return 0;
  }

  Py_DECREF(current);

  current = n;
  Py_INCREF(n);

  setNets(n->net);
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
net_inputs(PyObject*, PyObject* args)
{
  Board b;

  if( !PyArg_ParseTuple(args, "O&", &anyBoard, &b) ) {
    return 0;
  }

  positionclass pc;
  unsigned int n;
  
  const float* const i = NetInputs(b, &pc, &n);

  PyObject* list = PyList_New(n);
  
  for(uint k = 0; k < n; ++k) {
    PyList_SET_ITEM(list, k, PyFloat_FromDouble(i[k]));
  }

  PyObject* r = PyList_New(2);
  PyList_SET_ITEM(r, 0, list);
  PyList_SET_ITEM(r, 1, PyInt_FromLong(pc));

  return r;
}

static PyObject*
net_eval(PyObject*, PyObject* args)
{
  int c;
  PyObject* inputsObj;
  Board b;
  
  if( !PyArg_ParseTuple(args, "O&Oi", &anyBoard, &b, &inputsObj, &c) ) {
    return 0;
  }

  if( ! PySequence_Check(inputsObj) ) {
    return 0;
  }
  
  unsigned int const n = PySequence_Length(inputsObj);

  float i[n];

  for(uint k = 0; k < n; ++k) {
    PyObject* const bk = PySequence_Fast_GET_ITEM(inputsObj, k);
    i[k] = PyFloat_AsDouble(bk);
  }

  float p[5];
  
  NetEval(p, b, static_cast<positionclass>(c), i);

  PyObject* tuple = PyTuple_New(5);
  
  for(uint k = 0; k < 5; ++k) {
    PyTuple_SET_ITEM(tuple, k, PyFloat_FromDouble(p[k]));
  }

  return tuple;
}


static PyObject*
net_pinit(PyObject*, PyObject* args)
{
  int c;
  int nHidden = -1;
  
  if( !PyArg_ParseTuple(args, "i|i", &c, &nHidden) ) {
    return 0;
  }

  if( ! neuralNetInitPrune(static_cast<positionclass>(c), nHidden) ) {
    PyErr_SetString(PyExc_RuntimeError, "neuralNetInitPrune failed.") ;
    return 0;
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
net_init(PyObject*, PyObject* args)
{
  int c;
  const char* inputFuncName = 0;
  int nHidden = -1;
  
  if( !PyArg_ParseTuple(args, "i|si", &c, &inputFuncName, &nHidden) ) {
    return 0;
  }

  if( inputFuncName && ! *inputFuncName ) {
    inputFuncName = 0;
  }
  
  if( ! neuralNetInit(static_cast<positionclass>(c),
		      inputFuncName, nHidden) ) {
    PyErr_SetString(PyExc_RuntimeError, "neuralNetInit failed.");
    return 0;
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
net_input_name(PyObject*, PyObject* args)
{
  int k;
  PyObject* arg;
  
  if( !PyArg_ParseTuple(args, "iO", &k, &arg) ) {
    return 0;
  }

  const char* n = 0;
  
  if( PyInt_Check(arg) ) {
    positionclass const pc = static_cast<positionclass>(PyInt_AS_LONG(arg));
  
    n = inputNameByClass(pc,  k);

  } else if( PyString_Check(arg) ) {
    n = inputNameByFunc(PyString_AS_STRING(arg), k);
  }
  
  if( ! n ) {
    PyErr_SetString(PyExc_RuntimeError, "args error.");
    return 0;
  }

  if( *n == 0 ) {
    Py_INCREF(Py_None);
    return Py_None;
  }
    
  return PyString_FromString(n);
}

static PyObject*
net_num_gen_inputs(PyObject*, PyObject*)
{
  uint tot = maxIinputs();
  return PyInt_FromLong(tot);
}

static PyObject*
net_gen_input_name(PyObject*, PyObject* args)
{
  int k;
  
  if( !PyArg_ParseTuple(args, "i", &k) ) {
    PyErr_SetString(PyExc_ValueError, "args error.");
    return 0;
  }

  if( ! (0 <= k && k < int(maxIinputs())) ) {
    PyErr_SetString(PyExc_ValueError, "args error.");
    return 0;
  }
  
  return PyString_FromString(genericInputName(200+k));
}

static PyObject*
net_gen_inputs(PyObject*, PyObject* args)
{
  Board b;
  PyObject* w;

  if( !PyArg_ParseTuple(args, "O&O", &anyBoard, &b, &w) ) {
    return 0;
  }

  if( ! PySequence_Check(w) ) {
    PyErr_SetString(PyExc_ValueError, "second arg not a list");
    return 0;
  }

  uint wLen = PySequence_Length(w);
  int wList[wLen + 1];

  // construct C list from  python sequence
  for(uint k = 0; k < wLen; ++k) {
    PyObject* const wk = PySequence_Fast_GET_ITEM(w, k);
    wList[k] = PyInt_AsLong(wk);
  }
  wList[wLen] = -1;

  // get values
  float values[wLen];
  getInputs(b, wList, values);

  // construct python list
  PyObject* list = PyList_New(wLen);
  
  for(uint k = 0; k < wLen; ++k) {
    PyList_SET_ITEM(list, k, PyFloat_FromDouble(values[k]));
  }

  return list;
}

static PyObject*
net_flush(PyObject*, PyObject*)
{
  EvalCacheFlush();
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
net_cacheResize(PyObject*, PyObject* args)
{
  int cSize;
  
  if( !PyArg_ParseTuple(args, "i", &cSize) ) {
    PyErr_SetString(PyExc_ValueError, "args error.");
    return 0;
  }
  
  EvalCacheResize(cSize);
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
net_sse(PyObject*, PyObject* const args)
{
  int fuse;
  
  if( !PyArg_ParseTuple(args, "i", &fuse) ) {
    return 0;
  }

  if( !( 0 <= fuse && fuse <= 1) ) {
    PyErr_SetString(PyExc_RuntimeError, "not bool");
    return 0;
  }

  if( ! useSSE(fuse) ) {
    PyErr_SetString(PyExc_RuntimeError, "failed to set sse");
  }

  Py_INCREF(Py_None);
  return Py_None;
}

// init - creates a python object for cuttent
// current() - gets current
//  increment count for current var
// set(net)  - sets current to net
//  decrement count for current, create new one for 'net', incrementing it's
//  count

static PyMethodDef net_methods[] = {
  {"current",	net_current, METH_VARARGS,
   "net = current() # Get current net."},
  
  {"get",	net_get, METH_VARARGS,
   "net = get(file,[,cache-size]) - Get net from 'file'"},
  
  {"set",	net_set, METH_VARARGS, "Use 'net'"},
  
  {"save",	net_save, METH_VARARGS, "Save current net to 'file'"},

  {"flush",	net_flush, METH_VARARGS, "flush cache"},

  {"cresize",	net_cacheResize, METH_VARARGS, "resize cache"},
  
  {"inputs",    net_inputs, METH_VARARGS,
   "inputs,classOfPosition = inputs(pos) # raw net inputs"},

  {"eval",    	net_eval, METH_VARARGS,
   "probs = eval(pos) # eval from net inputs"},
  
  {"init",      net_init, METH_VARARGS,
   "Init net of class 'c' to a new random net" },
  
  {"pinit",      net_pinit, METH_VARARGS,
   "Init prune net of class 'c' to a new random net" },

  { "input_name", net_input_name, METH_VARARGS,
    "symbolic name of net input" },

  { "num_gen_inputs", net_num_gen_inputs, METH_VARARGS,
    "total number of generic inputs" },
  
  { "gen_input_name", net_gen_input_name, METH_VARARGS,
    "total number of generic inputs" },

  { "gen_inputs", net_gen_inputs, METH_VARARGS,
    "get generic inputs" },

  {"sse",	net_sse,	METH_VARARGS,
   "Enable/Disable SSE" },
  
  {0,0,0,0}		/* sentinel */
};



//DL_EXPORT(void)
void
initnet(void)
{
  // noddy_NoddyType.ob_type = &PyType_Type;
     
  Py_InitModule("bgnet", net_methods);

  current = PyObject_New(NetObject, &Net_Type);

  current->net = setNets(0);
}

void
closenet(void)
{
  Py_DECREF(current);
}
