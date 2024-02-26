/*
 * pytrainer.cc
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
#include "Python.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
#include <positionid.h>
#include <eval.h>
}

#include "pytrainer.h"
#include "defs.h"

namespace {
inline unsigned char*
auchFromSring(const char* const s)
{
  static unsigned char auch[10];

#if !defined( NDEBUG )
  for(uint i = 0; i < 20; ++i) {
    assert('A' <= s[i] && s[i] < 'A' + 16);
  }
#endif
  
  for(uint i = 0; i < 10; ++i) {
    auch[i] = ((s[2*i+0] - 'A') << 4) +  (s[2*i+1] - 'A');
  }
  return auch;
}

inline double
eqAbsErr(const float* const p1, const float* const p2)
{
  return (2 * fabs(p1[0] - p2[0]) + fabs(p1[1] - p2[1])
	  + fabs(p1[2] - p2[2]) + fabs(p1[3] - p2[3]) + fabs(p1[4] - p2[4]));
}

inline double
noBGErr(const float* const p1, const float* const p2)
{
  return (2 * fabs(p1[0] - p2[0]) + fabs(p1[1] - p2[1])
	  + fabs(p1[3] - p2[3]));
}

inline double
eqErr(const float* const p1, const float* const p2)
{
  return (2 * (p1[0] - p2[0]) + (p1[1] - p2[1])
	  + (p1[2] - p2[2]) + (p1[3] - p2[3]) + (p1[4] - p2[4]));
}

}

struct DataPosition {
  unsigned char	auch[10];

  float		probs[5];
};

class Trainer {
public:
  Trainer(uint n);
  
  ~Trainer();

  struct Errors {
    Errors(void) :
      equityError(0.0),
      maxEquityError(0.0),
      absEquityError(0.0),
      maxAbsEquityError(0.0),
      noBGerror(0.0),
      maxNoBGerror(0.0)
      {}

    void add_eq(double const e) {
      if( e > maxEquityError ) {
	maxEquityError = e;
      }
      equityError += e * e;
    }
    
    double	equityError;
    double	maxEquityError;

    void add_aeq(double const e) {
      if( e > maxAbsEquityError ) {
	maxAbsEquityError = e;
      }
      absEquityError += e * e;
    }
    
    double	absEquityError;
    double	maxAbsEquityError;
    
    void add_mnbg(double const e) {
      if( e > maxNoBGerror ) {
	maxNoBGerror = e;
      }
      noBGerror += e * e;
    }
    
    double	noBGerror;
    double	maxNoBGerror;

    void adjust(uint const n) {
      equityError = sqrt(equityError / n);
      absEquityError = sqrt(absEquityError / n);
      noBGerror = sqrt(noBGerror / n);
    }
  };
  
  void	errors(Errors& e) const;
  
  void	train(double a, const int* order) const;
  
  uint			nPositions;
  DataPosition*		positions;

  bool			ignoreBGs;
  bool			pruneNet;
  int*			tList;
};

Trainer::Trainer(uint const n) :
  nPositions(n),
  positions(new DataPosition [nPositions]),
  ignoreBGs(false),
  pruneNet(false),
  tList(0)
{}

Trainer::~Trainer()
{
  delete [] positions;
  delete [] tList;
}

typedef int Board[2][25];

void
Trainer::errors(Errors& e) const
{
  Board board;
  float p[5];

  for(uint k = 0; k < nPositions; ++k) {
    DataPosition const& t = positions[k];
    
    PositionFromKey(board, const_cast<unsigned char*>(t.auch));

    if( pruneNet ) {
      evalPrune(board, p);
    } else {
      EvaluatePositionFast(board, p);
    }

    e.add_eq(eqErr(p, t.probs));
    e.add_aeq(eqAbsErr(p, t.probs));
    e.add_mnbg(noBGErr(p, t.probs));
  }

  e.adjust(nPositions);
}

void
Trainer::train(double const a, const int* const order) const
{
  Board board;

  for(uint k = 0; k < nPositions; ++k) {
    DataPosition const& t = positions[order ? order[k] : k];
    
    PositionFromKey(board, const_cast<unsigned char*>(t.auch));

    if( ignoreBGs ) {
      float p[5] = {t.probs[0], t.probs[1], 0.0, t.probs[3], 0.0};
      if( pruneNet ) {
	PruneTrainPosition(board, p, a);
      } else {
	TrainPosition(board, p, a, tList);
      }
    } else {
      if( pruneNet ) {
	PruneTrainPosition(board, const_cast<float*>(t.probs), a);
      } else {
	TrainPosition(board, const_cast<float*>(t.probs), a, tList);
      }
    }
  }
}



struct TrainerObject : PyObject {
  Trainer*	trainer;
};

static void
trainer_dealloc(TrainerObject* const self)
{
  delete self->trainer;
  PyObject_Del(self);
}

static PyObject*
trainer_errors(PyObject* self, PyObject*);

static PyObject*
trainer_train(PyObject* self, PyObject* args);


static PyMethodDef trainer_methods[] = {
  {"errors",	trainer_errors, METH_NOARGS,
   ""},

  {"train",	trainer_train, METH_VARARGS,
   ""},
  
  {0,0,0,0}		/* sentinel */
};

static PyObject *
xx_getattr(PyObject* dp, char *name)
{
  return Py_FindMethod(trainer_methods, (PyObject *)dp, name);
}

PyTypeObject Trainer_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"gnubg.trainer", 	/*tp_name*/
	sizeof(TrainerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)trainer_dealloc, /*tp_dealloc*/
	0,			 /*tp_print*/
	xx_getattr,	        /*tp_getattr*/
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


static PyObject*
trainer_errors(PyObject* self, PyObject*)
{
  {                               assert( self->ob_type == &Trainer_Type ); }

  Trainer& t = *static_cast<TrainerObject*>(self)->trainer;

  Trainer::Errors e;
  t.errors(e);

  return Py_BuildValue("dddddd",
		       e.absEquityError, e.maxAbsEquityError,
		       e.equityError, e.maxEquityError,
		       e.noBGerror, e.maxNoBGerror);
}

static PyObject*
trainer_train(PyObject* self, PyObject* args)
{
  {                                 assert( self->ob_type == &Trainer_Type ); }

  Trainer& t = *static_cast<TrainerObject*>(self)->trainer;
  
  double a;
  PyObject* porder = 0;
  
  if( !PyArg_ParseTuple(args, "d|O", &a, &porder) ) {
    PyErr_SetString(PyExc_ValueError, "wrong args.") ;
    return 0;
  }


  int* order = 0;
  if( porder ) {
    if( !(PySequence_Check(porder) &&
	  PySequence_Size(porder) == int(t.nPositions))) {

      PyErr_SetString(PyExc_ValueError, "invalid order.") ;
      return 0;
    }
    
    order = new int [t.nPositions];

    for(uint k = 0; k < t.nPositions; ++k) {
      PyObject* const i = PySequence_Fast_GET_ITEM(porder, k);
      order[k] = PyInt_AsLong(i);
    }
  }
  
  t.train(a, order);

  delete [] order;
  
  Py_INCREF(Py_None);
  return Py_None;
}


PyObject*
newTrainer(PyObject* const args)
{
  PyObject* data;
  int flag = 0;
  int prune = 0;
  PyObject* tList = 0;
  
  if( !PyArg_ParseTuple(args, "O|iiO", &data, &flag, &prune, &tList)) {
    return 0;
  }

  if( ! PySequence_Check(data) ) {
    return 0;
  }
  
  uint const nTrain = PySequence_Size(data);
  Trainer& t = *(new Trainer(nTrain));

  t.ignoreBGs = flag;
  t.pruneNet = prune;

  if( tList ) {
    if( ! PySequence_Check(tList) ) {
      PyErr_SetString(PyExc_ValueError, "not a list.") ;
      return 0;
    }
    if( uint const s = PySequence_Size(tList) ) {
      t.tList = new int [s+1];

      for(uint k = 0; k < s; ++k) {
	PyObject* const i = PySequence_Fast_GET_ITEM(tList, k);
	t.tList[k] = PyInt_AsLong(i);
      }
      t.tList[s] = -1;
    }
  }
  
  for(uint k = 0; k < nTrain; ++k) {
    DataPosition& p = t.positions[k];

    PyObject* const pl = PySequence_Fast_GET_ITEM(data, k);
    if( ! (pl && PyString_Check(pl)) ) {
      return 0;
    }

    const char* l = PyString_AS_STRING(pl);

    if( strlen(l) < 20 ) {
      PyErr_Format(PyExc_ValueError, "invalid pos id (%s).", l);
      delete &t;
      return 0;
    }
      
    char id[20];

    strncpy(id, l, sizeof(id));

    memcpy(p.auch, auchFromSring(id), sizeof(p.auch));

    l += 20;
    {
      char* endp = 0;
      for(uint j = 0; j < 5; ++j) {
	p.probs[j] = strtod(l, &endp);
	l = endp;
      }
    }
  }

  TrainerObject* o = PyObject_New(TrainerObject, &Trainer_Type);
  o->trainer = &t;
  
  return o;
}
