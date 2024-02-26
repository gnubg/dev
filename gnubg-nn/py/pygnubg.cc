/*
 * pygnubg.cc
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

#include <iostream>

#include "stdutil.h"

#include <analyze.h>
#include <bm.h>
#include <player.h>
#include <equities.h>

#include <misc.h>

extern "C" {
#include <positionid.h>
#include <eval.h>
};
#include <br.h>
#include <osr.h>

#include "pygnubg.h"

#include "pynets.h"
#include "pytrainer.h"
#include "raceinfo.h"

#if defined(MOTIF)
#include <motif.h>
#endif

using namespace BG;

typedef short int AnalyzeBoard[26];

namespace {
  Player analyzer;
}

namespace {
unsigned char*
auchFromSring(const char* const s)
{
  static unsigned char auch[10];

  for(uint i = 0; i < 10; ++i) {
    auch[i] = ((s[2*i+0] - 'A') << 4) +  (s[2*i+1] - 'A');
  }
  return auch;
}

const char*
posString(const unsigned char* const auch)
{
  static char p[21];

  for(uint i = 0; i < 10; ++i) {
    p[2*i+0] = 'A' + ((auch[i] >> 4) & 0xf);
    p[2*i+1] = 'A' + (auch[i] & 0xf);
  }
  p[20] = 0;

  return p;
}

const char*
boardString(Board const& board)
{
  unsigned char auch[10];
	
  PositionKey(const_cast<Board&>(board), auch);

  return posString(auch);
}



/// X(-) is 0(zero)
void
setBoard(AnalyzeBoard list, Board const& board)
{
  list[0] = board[1][24];
    
  for(uint k = 0; k < 24; ++k) {
    int val = 0;
    
    if( board[1][23-k] > 0 ) {
      {                                           assert( board[0][k] == 0 ); }
      val = board[1][23-k];
      
    } else if( board[0][k] > 0 ) {
      val = -board[0][k];
    }
    
    list[k+1] = val;
  }

  list[25] = -board[0][24];
}

PyObject*
boardObj(Board const& board)
{
  AnalyzeBoard l;
  setBoard(l, board);

  uint const n = sizeof(l)/sizeof(l[0]);

  PyObject* tuple = PyTuple_New(n);
  
  for(uint k = 0; k < n; ++k) {
    PyTuple_SET_ITEM(tuple, k, PyInt_FromLong(l[k]));
  }

  return tuple;
}


bool
stringToBoard(const char* const s, Board& b)
{
  uint const len = strlen(s);

  switch( len ) {
    case 20:
    {
      PositionFromKey(b, auchFromSring(s));
      break;
    }
    case 14:
    {
      PositionFromID(b, s);
      break;
    }
    default:
    {
      //
      return false;
    }
  }

  return true;
}


int
seqToBoard(PyObject* o, Board& board)
{
  memset(board[0], 0, 25 * sizeof(int));
  memset(board[1], 0, 25 * sizeof(int));

  for(uint k = 0; k < 26; ++k) {
    PyObject* const bk = PySequence_Fast_GET_ITEM(o, k);
    int const n = PyInt_AsLong(bk);    

    if( n != 0 ) {
      switch( k ) {
	case 25:
	{
	  // bar of -
	  board[0][24] = abs(n);
	  break;
	}
	case 0:
	{
	  // bar of +
	  board[1][24] = abs(n);
	  break;
	}
	default:
	{
	  if( n < 0 ) {
	    board[0][k - 1] = -n;
	  } else {
	    board[1][24 - k] = n;
	  }
	  break;
	}
      }
    }
  }

  {
    uint s0 = 0;
    uint s1 = 0;
  
    for(uint k = 0; k < 24; ++k) {
      s0 += board[0][k];
      s1 += board[1][k];
    }

    if( ! (s0 <= 15 && s1 <= 15) ) {
      PyErr_Format(PyExc_ValueError, "Invalid board (%d %d)", s0, s1);
      return 0;
    }
  }

  return 1;
}


enum {
  PLY_OSR = -2,
  PLY_BEAROFF = -3,
  PLY_PRUNE = -4,
  PLY_1SBEAR = -5,
  PLY_RACENET = -6,
  PLY_1ANDHALF = -7,
  PLY_1SRACE = -8,
};

static int
readPly(PyObject* o, void* const b)
{
  int& np = *static_cast<int*>(b);
  
  if( PyInt_Check(o) ) {
    np = PyInt_AS_LONG(o);

    if( np >= 0 || (np <=  PLY_OSR && np >= PLY_1ANDHALF) ) {
      return 1;
    }

    PyErr_SetString(PyExc_ValueError, "invalid ply");
    return 0;
    
  }

  PyErr_SetString(PyExc_TypeError, "invalid ply type");
  return 0;
}

static int
anyAnalyzeBoard(PyObject* o, void* const b)
{
  AnalyzeBoard& board = *static_cast<AnalyzeBoard*>(b);
  
  if( PySequence_Check(o) && PySequence_Size(o) == 26 ) {
    uint s0 = 0;
    uint s1 = 0;
    
    for(uint k = 0; k < 26; ++k) {
      PyObject* const bk = PySequence_Fast_GET_ITEM(o, k);
      board[k] = PyInt_AsLong(bk);
      if( board[k] > 0 ) {
	s0 += board[k];
      } else if( board[k] < 0 ) {
	s1 += -board[k];
      }
    }

    if( ! (s0 <= 15 && s1 <= 15) ) {
      PyErr_Format(PyExc_ValueError, "Invalid board (%d %d)", s0, s1);
      return 0;
    }
    return 1;
  }

  if( PyString_Check(o) ) {
    const char* const s = PyString_AS_STRING(o);

    Board b1;
    if( stringToBoard(s, b1) ) {
      setBoard(board, b1);
      
      return 1;
    }
  }

  PyErr_SetString(PyExc_ValueError, "invalid board");
  return 0;
}

}

namespace BG {
int
anyBoard(PyObject* o, void* const b)
{
  Board& board = *static_cast<Board*>(b);
  
  if( PySequence_Check(o) && PySequence_Size(o) == 26 ) {
    return seqToBoard(o, board);
  }


  if( PyString_Check(o) ) {
    const char* const s = PyString_AS_STRING(o);

    if( stringToBoard(s, board) ) {
      return 1;
    }
  }

  PyErr_SetString(PyExc_ValueError, "invalid board");
  return 0;
}

}

static PyObject*
gnubg_boardfromkey(PyObject*, PyObject* const args)
{
  const char* s;

  if( !PyArg_ParseTuple(args, "s", &s) ) {
    return 0;
  }

  Board b;
  
  if( stringToBoard(s, b) ) {
    return boardObj(b);
  }

  PyErr_SetString(PyExc_ValueError, "invalid board");
  return 0;
}

static PyObject*
gnubg_keyofboard(PyObject*, PyObject* const args)
{
  Board b;

  if( !PyArg_ParseTuple(args, "O&", &anyBoard, &b) ) {
    return 0;
  }
  
  return PyString_FromString(boardString(b));
}

static PyObject*
gnubg_id(PyObject*, PyObject* const args)
{
  Board b;

  if( !PyArg_ParseTuple(args, "O&", &anyBoard, &b) ) {
    return 0;
  }
  
  return PyString_FromString(PositionID(b));
}

static PyObject*
gnubg_classify(PyObject*, PyObject* const args)
{
  Board b;

  if( !PyArg_ParseTuple(args, "O&", &anyBoard, &b) ) {
    return 0;
  }

  positionclass pc = ClassifyPosition(b);

  if( pc == CLASS_BEAROFF2 ) {
    pc = CLASS_BEAROFF1;
  }
  
  return PyInt_FromLong(pc);
}

static int
readBearoffId(PyObject* const o, void* const pi)
{
  int& i = *static_cast<int*>(pi);
  
  if( PyInt_Check(o) ) {
    i = PyInt_AS_LONG(o);

    if( ! (0 < i && i < 54264) ) {
      PyErr_SetString(PyExc_ValueError,
		      "bearoff id outside of range [1,54264)");
      return 0;
    }

    return 1;
  } else if( PySequence_Check(o) && PySequence_Size(o) == 6 ) {
    int p[6];
    for(uint k = 0; k < 6; ++k) {
      PyObject* const ik = PySequence_Fast_GET_ITEM(o, k);
      // (FIXME) error check
      p[k] = PyInt_AsLong(ik);
    }

    i = PositionBearoff(p);
    return 1;
  }

  PyErr_SetString(PyExc_TypeError, "invalid type for bearoff id");
  return 0;
}

static PyObject*
gnubg_bearoffid2pos(PyObject*, PyObject* const args)
{
  int i;
  
  if( !PyArg_ParseTuple(args, "O&", &readBearoffId, &i) ) {
    return 0;
  }
    
  int p[6];
  PositionFromBearoff(p, i);

  return Py_BuildValue("iiiiii", p[0], p[1], p[2], p[3], p[4], p[5]);
}


// static PyObject*
// gnubg_bearoffpos2id(PyObject*, PyObject* const args)
// {
// }

static PyObject*
gnubg_bearoffprobs(PyObject*, PyObject* const args)
{
  int i;
  
  if( !PyArg_ParseTuple(args, "O&", &readBearoffId, &i) ) {
    return 0;
  }

  B b;
  getBearoff(i, &b);

  uint const s = b.start - 1;
  PyObject* tuple = PyTuple_New(s + b.len);
  
  for(uint k = 0; k < s; ++k) {
    PyTuple_SET_ITEM(tuple, k, PyFloat_FromDouble(0.0));
  }

  for(uint k = 0; k < b.len; ++k) {
    PyTuple_SET_ITEM(tuple, k + s, PyFloat_FromDouble(b.p[k]));
  }

  return tuple;
}

static PyObject*
gnubg_moves(PyObject*, PyObject* const args)
{
  int n1;
  int n2;
  int m = 0;
  Board board;

  if( !PyArg_ParseTuple(args, "O&ii|i", &anyBoard, &board, &n1, &n2, &m) ) {
    return 0;
  }

  movelist pml;
	
  GenerateMoves(&pml, board, n1, n2);
  
  PyObject* tuple = PyTuple_New(pml.cMoves);
  
  for(uint k = 0; k < uint(pml.cMoves); ++k) {
    move const& mv = pml.amMoves[k];
    
    const char* const s = posString(mv.auch);
    if( m ) {
      uint n = 0;
      for( /**/; n < 8; n += 2) {
				if( mv.anMove[n] < 0 ) {
					break;
				}
      }
      
      PyObject* moveTuple = PyTuple_New(n/2);
      
      for(uint j = 0; j < n; j += 2) {
				int const from = mv.anMove[j]+1;
				int const to = (mv.anMove[j+1] >= 0) ? mv.anMove[j+1]+1 : 0;

				PyObject* const s = PyTuple_New(2);
				PyTuple_SET_ITEM(s, 0, PyInt_FromLong(from));
				PyTuple_SET_ITEM(s, 1, PyInt_FromLong(to));

				PyTuple_SET_ITEM(moveTuple, j/2, s);
      }
      
      PyObject* const t = PyTuple_New(2);
      PyTuple_SET_ITEM(t, 0, PyString_FromString(s));
      PyTuple_SET_ITEM(t, 1, moveTuple);
      
      PyTuple_SET_ITEM(tuple, k, t);
	  
    } else {
      PyTuple_SET_ITEM(tuple, k, PyString_FromString(s));
    }
  }

  return tuple;
}


static PyObject*
gnubg_probs(PyObject*, PyObject* const args)
{
  Board board;
  int nPlies;
  unsigned int nr = 1296;
  
  if( !PyArg_ParseTuple(args, "O&O&|i", &anyBoard, &board,
			&readPly, &nPlies, &nr) ) {
    return 0;
  }

  float p[5];

  switch( nPlies ) {
    case PLY_OSR:
    {
      if( ! isRace(board) ) {
				PyErr_SetString(PyExc_RuntimeError, "Not a race position");
				return 0;
      }

      raceProbs(board, p, nr);

      break;
    }
    case PLY_BEAROFF:
    {
      EvaluatePositionToBO(board, p, false);
      break;
    }
    case PLY_PRUNE:
    {
      evalPrune(board, p);
      //PyErr_SetString(PyExc_RuntimeError, "No prune net loaded."); 
      //return 0;
      break;
    }
    case PLY_1SBEAR:
    {
      EvalBearoffOneSided(board, p);
      break;
    }
    case PLY_RACENET:
    {
      NetEvalRace(board, p);
      break;
    }
    case PLY_1ANDHALF:
    {
      float p1[5];
      
      EvaluatePosition(board, p, 0, 0, 0, 0, 0, 0);
      EvaluatePosition(board, p1, 1, 0, 0, 0, 0, 0);
      for(uint k = 0; k < 5; ++k) {
				p[k] = (p[k] + p1[k])/2;
      }
      break;
    }
    case PLY_1SRACE:
    {
#if defined( OS_BEAROFF_DB )
      if( ! EvalOSrace(board, p) ) {
				PyErr_SetString(PyExc_RuntimeError, "can't eval with OS race dababase");
	return 0;
      }
#else
      return 0;
#endif
      break;
    }
    default:
    {
      EvaluatePosition(board, p, nPlies, 0, 0, 0, 0, 0);
      break;
    }
  }

  PyObject* tuple = PyTuple_New(5);
  
  for(uint k = 0; k < 5; ++k) {
    PyTuple_SET_ITEM(tuple, k, PyFloat_FromDouble(p[k]));
  }

  return tuple;
}

#if defined( MOTIF )
static PyObject*
motif_probs(PyObject*, PyObject* const args)
{
  Board board;
  int nPly = 0;
  
  if( !PyArg_ParseTuple(args, "O&|i", &anyBoard, &board, &nPly) ) {
    return 0;
  }

  if( ! (0 <= nPly && nPly <= 2) ) {
    PyErr_SetString(PyExc_ValueError, "invalid ply");
    return 0;
  }
  
  float p[5];

  AnalyzeBoard list;
  setBoard(list, board);
  signed char b[26];
  for(uint k = 0; k < 26; ++k) {
    b[k] = list[25-k];
  }
  
  motif_moneyeval(b, p, nPly);

  PyObject* tuple = PyTuple_New(5);
  
  for(uint k = 0; k < 5; ++k) {
    PyTuple_SET_ITEM(tuple, k, PyFloat_FromDouble(p[k]));
  }

  return tuple;
}
#endif

static PyObject*
gnubg_pubevalscore(PyObject*, PyObject* const args)
{
  Board board;
  
  if( !PyArg_ParseTuple(args, "O&", &anyBoard, &board) ) {
		return 0;
  }

  int const fRace = ClassifyPosition( board ) <= CLASS_RACE;
	float const score = pubEvalVal(fRace, board);

  return PyFloat_FromDouble(score);
}

static PyObject*
gnubg_pubbestmove(PyObject*, PyObject* const args)
{
  Board board;
  int dice1, dice2;
	
  if( !PyArg_ParseTuple(args, "O&ii|i",
												&anyBoard, &board, &dice1, &dice2) ) {
    return 0;
  }

  if( dice1 < 0 || dice1 > 6 || dice2 < 0 || dice2 > 6 ) {
    PyErr_SetString(PyExc_ValueError, "invalid dice");
    return 0;
  }

  int move[8];
  int n = FindPubevalMove(dice1, dice2, board, move);

  PyObject* moveTuple = PyTuple_New(n/2);
  
  for(int j = 0; j < n/2; ++j) {
    int const k = 2*j;
    int const from = (move[k] >= 0) ? move[k]+1 : 0;
    int const to = (move[k+1] >= 0) ? move[k+1]+1 : 0;

    PyObject* const s = PyTuple_New(2);
    PyTuple_SET_ITEM(s, 0, PyInt_FromLong(from));
    PyTuple_SET_ITEM(s, 1, PyInt_FromLong(to));

    PyTuple_SET_ITEM(moveTuple, j, s);
  }
  
	SwapSides(board);

	PyObject* s = PyString_FromString(boardString(board));

	PyObject* result = PyTuple_New(2);

  PyTuple_SET_ITEM(result, 0, moveTuple);
  PyTuple_SET_ITEM(result, 1, s);
  
  return result;  
}

extern float centeredLDweight, ownedLDweight;
extern bool useRaceMagic;

static PyObject*
gnubg_doubleroll(PyObject*, PyObject* const args, PyObject* keywds)
{
  if( Equities::match.xAway == 0 && Equities::match.oAway == 0 ) {
    PyErr_SetString(PyExc_RuntimeError, "Not implemented for money") ;
    return 0;
  }
  
  int nPlies = 0;
  int nPliesVerify = -1;
  double ad = 0.5;

  char side = 0;
  int verboseInfo = 0;
  PyObject* p  = 0;

  float const s_centeredLDweight = centeredLDweight;
  float const s_ownedLDweight = ownedLDweight;
  float probs[5];
  probs[0] = -1;
	
  bool APopt = true;
  bool APshortcuts = false;

  AnalyzeBoard board;

  static const char* kwlist[] = {"pos", "n", "v", "s", "i", "p", 0};
  
  if( !PyArg_ParseTupleAndKeywords(args, keywds, "O&|iiciO", (char**)kwlist, 
				   &anyAnalyzeBoard, &board,
				   &nPlies, &nPliesVerify, &side,
				   &verboseInfo, &p)) {
    return 0;
  }
  
  bool xOnPlay = false;

  switch( side ) {
    case 'X': case 'x': xOnPlay = true; break;
    case 'O': case 'o': xOnPlay = false; break;
    case 0: break;
    default:
    {
      PyErr_SetString(PyExc_ValueError, "invalid side");
      return 0;
    }
  }

  if( Equities::match.cube > 1 && Equities::match.xOwns != xOnPlay ) {
    PyErr_Format(PyExc_RuntimeError, "side (%c) does not own cube",
		 side) ;
    return 0;
  }
  
  if( p ) {
    if( ! (PySequence_Check(p) && PySequence_Size(p) == 5) ) {
      PyErr_SetString(PyExc_ValueError, "invalid probablities") ;
      return 0;
    }
    
    for(uint k = 0; k < 5; ++k) {
      PyObject* const pk = PySequence_Fast_GET_ITEM(p, k);
      probs[k] = PyFloat_AsDouble(pk);
      if( ! (0 <= probs[k] && probs[k] <= 1) ) {
	PyErr_SetString(PyExc_ValueError, "invalid probablities") ;
	return 0;
      }
    }
  }
  
  Analyze::nPliesToDouble = nPlies;

  if( nPliesVerify < 0 ) {
    nPliesVerify = (nPlies == 0 ? 0 : 2);
  }
	
  Analyze::nPliesToDoubleVerify = nPliesVerify;

  Player::R1 const& info =
    analyzer.rollOrDouble(board, xOnPlay, ad, APopt, APshortcuts,
			  probs[0] >= 0 ? probs : 0);
  
  centeredLDweight = s_centeredLDweight;
  ownedLDweight = s_ownedLDweight;

  if( verboseInfo ) {
     PyObject* const v =
       Py_BuildValue("iiiddd",
		     info.actionDouble, info.actionTake, info.tooGood,
		     info.matchProbNoDouble,
		     info.matchProbDoubleTake,
		     info.matchProbDoubleDrop);
     return v;
  }
  
  return PyInt_FromLong(info.actionDouble);
}

static PyObject*
gnubg_rollout(PyObject*, PyObject* const args, PyObject* keywds)
{
  int nTruncate = 500;
  int nPlies = 0;
  int cGames = 1296;
  int wantSts = 0;
  
  Analyze::RolloutEndsAt level = Analyze::AUTO;
  
  AnalyzeBoard board;

  static const char* kwlist[] = {"pos", "n", "np",  "level", "nt", "std", 0};
  
  if( !PyArg_ParseTupleAndKeywords(args, keywds, "O&|iiiii", (char**)kwlist, 
				   &anyAnalyzeBoard, &board,
				   &cGames, &nPlies, &level,
				   &nTruncate, &wantSts)) {
    return 0;
  }

  float p[NUM_OUTPUTS];
  float ars[NUM_OUTPUTS];

  if( cGames <= 0 ) {
    PyErr_Format(PyExc_ValueError, "Invalid number of games (%d)", cGames) ;
    return 0;
  }
  
  analyzer.rollout(board, false, p, ars, nPlies,
		   nTruncate, cGames, level);

  if( ! wantSts ) {
    return Py_BuildValue("ddddd", p[0], p[1], p[2], p[3], p[4]);
  }

  return Py_BuildValue("((ddddd)(ddddd))",
		       p[0], p[1], p[2], p[3], p[4],
		       ars[0], ars[1], ars[2], ars[3], ars[4]);
}


static PyObject*
gnubg_cubefullRollout(PyObject*, PyObject* const args, PyObject* keywds)
{
  if( Equities::match.xAway == 0 && Equities::match.oAway == 0 ) {
    PyErr_SetString(PyExc_RuntimeError, "Not implemented for money") ;
    return 0;
  }
  
  AnalyzeBoard board;
  uint nGames = 576;
  uint nPlies = 0;
  char side = 0;
  
  static const char* kwlist[] = {"pos", "ngames", "side", "ply", 0};
  
  if( !PyArg_ParseTupleAndKeywords(args, keywds, "O&|ici", (char**)kwlist, 
				   &anyAnalyzeBoard, &board,
				   &nGames, &side, &nPlies)) {
    return 0;
  }
  
  bool xOnPlay = false;

  switch( side ) {
    case 'X': case 'x': xOnPlay = true; break;
    case 'O': case 'o': xOnPlay = false; break;
    case 0: break;
    default:
    {
      PyErr_SetString(PyExc_ValueError, "invalid side");
      return 0;
    }
  }

  Analyze::GNUbgBoard anBoard;

  Analyze::set(board, anBoard, xOnPlay);
  
  const float* r = analyzer.rolloutCubefull(anBoard, nPlies, nGames, xOnPlay);

  PyObject* tuple = PyTuple_New(13);
  
  for(uint k = 0; k < 13; ++k) {
    PyTuple_SET_ITEM(tuple, k, PyFloat_FromDouble(r[k]));
  }

  return tuple;
}

static PyObject*
gnubg_bestmove(PyObject*, PyObject* const args, PyObject* keywds)
{
  int nPlies = 0;
  char side = 0;
  int moveBoard = 0;
  int resignInfo = 0;
  int list = 0;
  int reduced = 0;
  
  Board board;
  int dice1, dice2;
  
  static const char* kwlist[] = {"pos", "dice1", "dice2", "n", "s", "b", "r",
			   "list", "reduced", 0};
  
  if( !PyArg_ParseTupleAndKeywords(args, keywds, "O&ii|iciiii", (char**)kwlist, 
				   &anyBoard, &board, &dice1, &dice2,
				   &nPlies, &side,
				   &moveBoard, &resignInfo, &list, &reduced)) {
    return 0;
  }

  if( nPlies < 0 ) {
    PyErr_SetString(PyExc_ValueError, "negative ply");
    return 0;
  }

  if( dice1 < 0 || dice1 > 6 || dice2 < 0 || dice2 > 6 ) {
    PyErr_SetString(PyExc_ValueError, "invalid dice");
    return 0;
  }
  
  bool xOnPlay = false;

  switch( side ) {
    case 'X': case 'x': xOnPlay = true; break;
    case 'O': case 'o': xOnPlay = false; break;
    case 0: break;
    default:
    {
      PyErr_SetString(PyExc_ValueError, "invalid side");
      return 0;
    }
  }

  std::vector<MoveRecord>* r = 0;
  if( list ) {
    r = new std::vector<MoveRecord>;
  }
  
  int move[8];
  
  int const n = findBestMove(move, dice1, dice2, board, xOnPlay, nPlies, r, reduced);

  PyObject* moveTuple = PyTuple_New(n/2);
  
  for(int j = 0; j < n/2; ++j) {
    int const k = 2*j;
    int const from = (move[k] >= 0) ? move[k]+1 : 0;
    int const to = (move[k+1] >= 0) ? move[k+1]+1 : 0;

    PyObject* const s = PyTuple_New(2);
    PyTuple_SET_ITEM(s, 0, PyInt_FromLong(from));
    PyTuple_SET_ITEM(s, 1, PyInt_FromLong(to));

    PyTuple_SET_ITEM(moveTuple, j, s);
  }

  if( ! (moveBoard || resignInfo || list ) ) {
    return moveTuple;
  }

  PyObject* resultTupe = PyTuple_New(1 + moveBoard + resignInfo + list);

  PyTuple_SET_ITEM(resultTupe, 0, moveTuple);

  uint tupleIndex = 1;
  
  if( moveBoard ) {
    SwapSides(board);

    PyObject* s = PyString_FromString(boardString(board));

    PyTuple_SET_ITEM(resultTupe, tupleIndex, s);
    ++tupleIndex;
    
    SwapSides(board);
  }

  if( resignInfo ) {
    int r = 0;
	      
    if( isRace(board) ) {
      AnalyzeBoard b;
	      
      setBoard(b, board);
	      
      r = analyzer.offerResign(nPlies, 2, b, true);
    }

    PyTuple_SET_ITEM(resultTupe, tupleIndex, PyInt_FromLong(r));
    ++tupleIndex;
  }

  if( list ) {
    PyObject* listTuple = PyTuple_New(r->size());
    
    for(uint i = 0; i < r->size(); ++i) {
      PyObject* s = PyTuple_New(4);

      MoveRecord& ri = (*r)[i];
      
      const char* const a = posString(ri.pos);
      PyTuple_SET_ITEM(s, 0, PyString_FromString(a));

      {
				int* move = ri.move;
				int n = 0;
				for(/**/; n < 4; ++n) {
					if( move[2*n] < 0 ) {
						break;
					}
				}
				PyObject* m = PyTuple_New(2*n);
				for(int j = 0; j < n; ++j) {
					int const k = 2*j;
					int const from = (move[k] >= 0) ? move[k]+1 : 0;
					int const to = (move[k+1] >= 0) ? move[k+1]+1 : 0;
					PyTuple_SET_ITEM(m, k, PyInt_FromLong(from));
					PyTuple_SET_ITEM(m, k+1, PyInt_FromLong(to));
				}
				PyTuple_SET_ITEM(s, 1, m);
      }
      
      PyObject* p = PyTuple_New(5);
  
      for(uint k = 0; k < 5; ++k) {
				PyTuple_SET_ITEM(p, k, PyFloat_FromDouble(ri.probs[k]));
      }

      PyTuple_SET_ITEM(s, 2, p);

      PyTuple_SET_ITEM(s, 3, PyFloat_FromDouble(ri.matchScore));
      
      PyTuple_SET_ITEM(listTuple, i, s);
    }

    PyTuple_SET_ITEM(resultTupe, tupleIndex, listTuple);
    ++tupleIndex;
  }
  
  return resultTupe;  
}

static PyObject*
roll_dice(PyObject*, PyObject*)
{
  int dice[2];
	
  RollDice(dice);

  return Py_BuildValue("ii", dice[0], dice[1]);
}
  


static PyObject*
gnubg_trainer(PyObject*, PyObject* const args)
{
  return newTrainer(args);
}


static PyObject*
gnubg_ocr(PyObject*, PyObject* const args)
{
  int n;
  
  if( !PyArg_ParseTuple(args, "i", &n) ) {
    return 0;
  }

  float stdv;
  float const v = ocr(n, &stdv);

  if( v == 0.0 ) {
    return 0;
  }
  
  return Py_BuildValue("dd", v,stdv);
}

static PyMethodDef gnubg_methods[] = {
  {"boardfromkey",	gnubg_boardfromkey,	METH_VARARGS,
   "Expanded board representation from key."},
  
  {"keyofboard",	gnubg_keyofboard,	METH_VARARGS,
   "Board key from board."},
  
  {"id",		gnubg_id,		METH_VARARGS,
   "Board gnubg ID from board."},

  {"classify",		gnubg_classify,		METH_VARARGS,
   "Classify position."},

  {"bearoffid2pos",     gnubg_bearoffid2pos,	METH_VARARGS,
   "Bearoff position from index"},

//   {"bearoffpos2id",     gnubg_bearoffpos2id,	METH_VARARGS,
//    "Bearoff id from position"},

  {"bearoffprobs",      gnubg_bearoffprobs,	METH_VARARGS,
   "Bearoff probablities"},
  
  {"moves",		gnubg_moves,		METH_VARARGS,
   "All moves for board and dice."},
  
  {"probs",		gnubg_probs,		METH_VARARGS,
   "Evaluate a position."},

  {"doubleroll",	(PyCFunction)gnubg_doubleroll,
   METH_VARARGS|METH_KEYWORDS,   "Double or roll."},

  {"bestmove",		(PyCFunction)gnubg_bestmove,
   METH_VARARGS|METH_KEYWORDS,   "Find best move."},

  {"rollout",           (PyCFunction)gnubg_rollout,
   METH_VARARGS|METH_KEYWORDS,  "Rollout a position" },

  {"crollout",           (PyCFunction)gnubg_cubefullRollout,
   METH_VARARGS|METH_KEYWORDS,  "Cubefull rollout" },
    
  {"roll",      	roll_dice,	METH_VARARGS,
   "Roll dice" },
  
  {"trainer",		gnubg_trainer, METH_VARARGS,
   "Create trainer"},

  {"onecrace",		gnubg_ocr, METH_VARARGS,
   "One Chequer Race"},

#if defined( MOTIF )
  { "motif_probs", motif_probs,		METH_VARARGS,
   "Evaluate a position using Motif engine."},
#endif

  { "pubevalscore", gnubg_pubevalscore, METH_VARARGS,
    "PUBEVAL score of position."},

  { "pubbestmove", gnubg_pubbestmove, METH_VARARGS,
    "PUBEVAL best move."},
	
  {0,		0, 0, 0}		/* sentinel */
};



static PyObject*
set_seed(PyObject*, PyObject* const args)
{
  unsigned long seed;
  
  if( !PyArg_ParseTuple(args, "l", &seed) ) {
    return 0;
  }
  
  Analyze::srandom(seed);
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
set_shortcuts(PyObject*, PyObject* const args)
{
  int n = -1;
  int use;
  
  if( !PyArg_ParseTuple(args, "i|i", &use, &n) ) {
    return 0;
  }

  setShortCuts(!!use);

  if( n > 0 ) {
    setNetShortCuts(n);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
set_osdb(PyObject*, PyObject* const args)
{
#if defined( OS_BEAROFF_DB )
  int use;
  
  if( !PyArg_ParseTuple(args, "i", &use) ) {
    return 0;
  }

  if( use ) {
    enableOSdb();
  } else {
    disableOSdb();
  }

#endif

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject*
set_ps(PyObject*, PyObject* const args)
{
  int nPlies, nMoves, nAdditional;
  double th;
  
  if( !PyArg_ParseTuple(args, "iiid",
			&nPlies, &nMoves, &nAdditional, &th) ) {
    return 0;
  }

  if( nPlies < 0 || nMoves < 2 || nAdditional < 0 || th < 0.0 ) {
    PyErr_SetString(PyExc_ValueError, "invalid args");
    return 0;
  }
  
  setPlyBounds(nPlies, nMoves, nAdditional, th);
  
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject*
set_equities(PyObject*, PyObject* const args)
{
  const char* which;

  switch( PyTuple_Size(args) ) {
    case 1:
    {
      if( !PyArg_ParseTuple(args, "s", &which) ) {
				return 0;
      }

      if( strcasecmp("gnur", which) == 0 ) {
	Equities::set(Equities::gnur);
      } else if( strcasecmp("jacobs", which) == 0 ) {
	Equities::set(Equities::Jacobs);
      } else if( strcasecmp("woolsey", which) == 0 ) {
	Equities::set(Equities::WoolseyHeinrich);
      } else if( strcasecmp("snowie", which) == 0 ) {
	Equities::set(Equities::Snowie);
      } else if( strcasecmp("mec26", which) == 0 ) {
	Equities::set(Equities::mec26);
      } else if( strcasecmp("mec57", which) == 0 ) {
	Equities::set(Equities::mec57);
      } else {
	PyErr_SetString(PyExc_RuntimeError, "Not a valid equities table name");
	return 0;
      }
      break;
    }
    case 2:
    {
      double w, gr;
      if( !PyArg_ParseTuple(args, "dd", &w, &gr) ) {
	return 0;
      }
      
      if( ! (w > 0.0 && w < 1.0 && gr > 0.0 && gr < 1.0) ) {
	PyErr_SetString(PyExc_ValueError, "Not in [01] range");
	return 0;
      }
      
      Equities::set(w, gr);
      break;
    }
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
set_score(PyObject*, PyObject* const args)
{
  int crawford = 0;
  int usAway, opAway;
  
  if( !PyArg_ParseTuple(args, "ii|i", &usAway, &opAway, &crawford) ) {
    return 0;
  }

  if( usAway < 0 || opAway < 0 || (crawford < 0 || crawford > 1) ) {
    return 0;
  }

  if( crawford == 1 &&
      ! ((opAway == 1 && usAway > 1) || (opAway > 1 && usAway == 1)) ) {
    PyErr_SetString(PyExc_RuntimeError, "Away not compatible with crawford");
    return 0;
  }
  
  // opp is X
  analyzer.setScore(opAway, usAway);
  analyzer.crawford(crawford);
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
set_cube(PyObject*, PyObject* const args)
{
  int cube;
  char x = 0;
  
  if( !PyArg_ParseTuple(args, "i|c", &cube, &x) ) {
    return 0;
  }

  if( cube <= 0 ) {
    return 0;
  }

  int xOwn = false;
  
  if( cube > 1 ) {
    if( x == 0 ) {
      return 0;
    }
  
    if( tolower(x) == 'x' ) {
      xOwn = true;
    } else if( tolower(x) == 'o' ) {
      xOwn = false;
    } else {
      return 0;
    }
  }

  analyzer.setCube(cube, xOwn);
  
  Py_INCREF(Py_None);
  return Py_None;
}
  

static PyMethodDef gnubg_set_methods[] = {
  {"seed",	set_seed,	METH_VARARGS,
   "Set internal random generator seed."},
  
  {"shortcuts",	set_shortcuts,	METH_VARARGS,
   "Set evaluation shortcuts."},

  {"osdb",	set_osdb,	METH_VARARGS,
   "Enable/Disable OS database" },
  
  {"ps",	set_ps,		METH_VARARGS,
   "Set move filters" },
  
  {"equities",	set_equities,	METH_VARARGS,
   "Set equities table to use" },

  {"score",	set_score,	METH_VARARGS,
   "Set match score" },

  {"cube",	set_cube,	METH_VARARGS,
   "Set match cube" },

  {0,		0, 0, 0}		/* sentinel */
};


static PyObject*
equities_value(PyObject*, PyObject* const args)
{
  int xAway, oAway;
  
  if( !PyArg_ParseTuple(args, "ii", &xAway, &oAway) ) {
    return 0;
  }

  if( ! (0 <= xAway && xAway <= 25 &&  0 <= oAway && oAway <= 25) ) {
    PyErr_SetString(PyExc_ValueError, "Score not in 0-25 range");
    return 0;
  }

  float const v = Equities::value(xAway, oAway);

  return PyFloat_FromDouble(v);
}

static PyMethodDef gnubg_equities_methods[] = {
  {"value",	equities_value,	METH_VARARGS,
   ""},
  
  {0,		0, 0, 0}		/* sentinel */
};

static inline void
setInt(PyObject* m, const char* const attr, int v)
{
  PyObject* const o = PyInt_FromLong(v);
  PyObject_SetAttrString(m, const_cast<char*>(attr), o);
  Py_DECREF(o);
}

void
initGnuBG(void)
{
  Py_InitModule("gnubg_setup", gnubg_set_methods);
  Py_InitModule("gnubg_equities", gnubg_equities_methods);
  
  PyObject* const m = Py_InitModule("gnubg", gnubg_methods);

  setInt(m, "c_over", 		CLASS_OVER);
  setInt(m, "c_bearoff",	CLASS_BEAROFF1);
  setInt(m, "c_race", 		CLASS_RACE);
  setInt(m, "c_crashed", 	CLASS_CRASHED);
#if defined( CONTAINMENT_CODE )
  setInt(m, "c_backcontain", 	CLASS_BACKCONTAIN);
#endif
  setInt(m, "c_contact", 	CLASS_CONTACT);

  setInt(m, "p_osr", 		PLY_OSR);
  setInt(m, "p_bearoff",	PLY_BEAROFF);
  setInt(m, "p_prune",		PLY_PRUNE);
  setInt(m, "p_1sbear",		PLY_1SBEAR);
  setInt(m, "p_race",		PLY_RACENET);
  setInt(m, "p_1srace",		PLY_1SRACE);
  setInt(m, "p_0plus1",		PLY_1ANDHALF);

  setInt(m, "ro_race", 		Analyze::RACE);
  setInt(m, "ro_bearoff",	Analyze::BEAROFF);
  setInt(m, "ro_over",		Analyze::OVER);
  setInt(m, "ro_auto",		Analyze::AUTO);
}

extern "C" int Py_Main(int argc, char **argv);

int
main(int ac, char** av)
{
  char* av1[ac];
  uint ac1 = 1;
  
  av1[0] = av[0];

  const char* wFile = 0;
  
  for(int i = 1; i < ac; ++i) {
    const char* const o = av[i];
    
    if( o[0] == '-' && o[1] == 'W' ) {
      if( o[2] ) {
	wFile = o + 2;
      } else {
	++i;
	if( i >= ac ) {
	  std::cerr << "-W requires an argument" << std::endl;
	  exit(1);
	}
	
	wFile = av[i];
      }
    } else {
      av1[ac1] = av[i];
      ++ac1;
    }
  }
  
  if( ! Analyze::init(wFile) ) {
    std::cerr << std::endl << "failed to initalize GNU bg" << std::endl;
    
    exit(1);
  }

#if defined(MOTIF)
  motif_init();
#endif
  
  /* Pass argv[0] to the Python interpreter */
  Py_SetProgramName(av[0]);

  /* Initialize the Python interpreter.  Required. */
  Py_Initialize();

  
  /* Add a static module */
  initGnuBG();

  initnet();
  
  /* Define sys.argv.  It is up to the application if you
	   want this; you can also let it undefined (since the Python 
	   code is generally not a main program it has no business
	   touching sys.argv...) */
  PySys_SetArgv(ac1, av1);

  /* Do some application specific code */

  // load all gnubg modules

  // By default, try using SSE 
  useSSE(1);
  
  PyRun_SimpleString("import gnubg, gnubg_setup, gnubg_equities, bgnet\n"
		     "gnubg.set = gnubg_setup\n"
		     "gnubg.net = bgnet\n"
		     "gnubg.equities = gnubg_equities\n"
		     "gnubg.board = gnubg.boardfromkey\n");

  int const o = Py_Main(ac1, av1);

  closenet();

  EvalShutdown();

  /* Exit, cleaning up the interpreter */
  
  Py_Exit(o);

  /*NOTREACHED*/
}
