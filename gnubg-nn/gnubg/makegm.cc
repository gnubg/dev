/*
 * makegm.cc
 *
 * by Joseph Heled <joseph@gnubg.org>, 2002
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

#include <stdlib.h>

#include <iostream> 
#include <unistd.h>
#include <algorithm>

extern "C" {
#include "eval.h"
#include "positionid.h"
}

typedef unsigned int uint;

using std::cout;
using std::endl;
using std::ostream;

static unsigned long probs[54264][4];

// Total number of checkers in bearoff position 'b'
static inline int
tot(const int* const b)
{
  return b[0] + b[1] + b[2] + b[3] + b[4] + b[5];
}

// Average number or rolls to bearoff 1 checker
// Scaled, good only for comparison.
//
static inline unsigned long
avg(const unsigned long* const p)
{
  return p[0]*(36L*36L*36L) + 2 * p[1]*(36L*36L) + 3 * p[2]*(36L) + 4 * p[3];
}

#if !defined( NDEBUG )
// Sum of probablities
// 
static inline unsigned long
sum(const unsigned long* const p)
{
  return p[0]*(36L*36L*36L) + p[1]*(36L*36L) + p[2]*(36L) + p[3];
}
#endif

static void
setEntry(int const nId)
{
  int board[6];
  
  PositionFromBearoff(board, nId);

  for(uint i = 0; i < 4; ++i) {
    probs[nId][i] = 0;
  }

  
  if( tot(board) != 15 ) {
    return;
  }

  // speedup 
  if( (board[4] && board[3] && board[2]) ||
      (!board[5] && !board[4] && board[3] && board[2]) ||
      (!board[5] && !board[4] && !board[3] && board[2]) ||
      (!board[5] && !board[4] && !board[3] && !board[2]) ) {
    probs[nId][0] = 36;
    return;
  }

  int anBoard[2][25];
  
  for(uint i = 0; i < 6; ++i) {
    anBoard[1][i] = board[i];
  }
    
  for(uint i = 6; i < 25; ++i) {
    anBoard[1][i] = 0;
  }

  for(uint i = 0; i < 25; ++i) {
    anBoard[0][i] = 0;
  }
    
  for(uint r0 = 1; r0 <= 6; ++r0) {
    for(uint r1 = 1; r1 <= r0; ++r1) {
      if( board[r0-1] || board[r1-1] || r0+r1<=6 &&  board[r0+r1-1] ) {
	// one roll
	probs[nId][0] += (r0 != r1) ? 2 : 1;
      } else {
	movelist ml;
	int tmp[2][25];
	
	GenerateMoves(&ml, anBoard, r0, r1);

	unsigned long* best = 0;
	
	for(int i = 0; i < ml.cMoves; ++i) {
	  PositionFromKey(tmp, ml.amMoves[i].auch);

	  uint const j = PositionBearoff(tmp[1]);
	  if( probs[j][0] == 0 ) {
	    best = probs[j];
	    break;
	  }
	    
	  if( ! best ) {
	    best = probs[j];
	  } else if( avg(probs[j]) < avg(best) ) {
	    best = probs[j];
	  }
	}

	if( best[0] == 0 ) {
	  probs[nId][0] += (r0 != r1) ? 2 : 1;
	} else {
	  for(uint i = 0; i < 3; ++i) {
	    long n = best[i];
	    if( r0 != r1 ) {
	      n = n + n;
	    }
	    probs[nId][i + 1] += n;
	  }

	  if( best[3] ) {
	    probs[nId][3] += ((r0 != r1) ? 2 : 1) * (best[3]/36);
	  }
	}
      }
    }
  }
}

static inline void
outp(const unsigned long* const p)
{
  cout << "{" << p[1] << "," << p[2]
       << "," << p[3]  << "," << p[0] << "}";
}

static void
outit(ostream& o, const char* const lines[], unsigned int const n)
{
  for(unsigned int i = 0; i < n; ++i) {
    o << lines[i] << endl;
  }
}

// compares two bearoff probablities
class Sorter {
public:
  bool operator ()(int const i1, int const i2) const {
    //i1 < i2
    return avg(probs[i1]) < avg(probs[i2]);
  }
};


static
const char* const code0[] = {
  "#include \"bearoffgammon.h\"",
  "",
  "struct GroupInfo {",
  "  GammonProbs    gDefault;",
  "  unsigned char* info;",
  "  unsigned int   base;",
  "  unsigned int   size;",
  "};",
  "",
};

static
const char* const code1[] = {
  "#include <cassert>",
  "extern \"C\" {",
  "#include \"positionid.h\"",
  "}",
  "",
  "extern \"C\" const GammonProbs*",
  "getBearoffGammonProbs(const int board[6])",
  "{",
  "  int group = 0;",
  "  for(int i = 5; i >= 0; --i) { ",
  "    group += (0x1 << i) * !!board[i];",
  "  }",
  "",
  "  ",
  "  GroupInfo const& i = info[group-1];",
  "  if( ! i.info ) {",
  "    return &i.gDefault;",
  "  }",
  "",
  "  int grpSize = 0;",
  "  int b1[6];",
  "",
  "  for(unsigned int k = 0; k < 6; ++k) {",
  "    if( (group & (0x1 << k)) ) {",
  "      {                                               assert( board[k] > 0 ); }",
  "      ",
  "      b1[grpSize] = board[k] - 1;",
  "      ++grpSize;",
  "    }",
  "  }",
  "",
  "  unsigned int const index = positionIndex(grpSize, b1);",
  "",
  "  {                     assert( index >= i.base && index - i.base < i.size ); }",
  "  {            assert( i.info[index - i.base] < sizeof(all)/sizeof(all[0]) ); }",
  "  ",
  "  return &all[i.info[index - i.base]];",
  "}",
};

int
main(int /*ac*/, char** /*av*/)
{
  for(uint i = 0; i < 4; ++i) {
    probs[0][i] = 0;
  }

  for(uint i = 1; i < 54264; ++i) {
    setEntry(i);
    
#if !defined( NDEBUG )
    if( ! (probs[i][0] == 0 || sum(probs[i]) == 36L*36L*36L*36L) ) {
      cout << i << endl;
      exit(1);
    }
#endif
  }

  outit(cout, code0, sizeof(code0)/sizeof(code0[0]));

  unsigned short grpAssign[54264];
  uint grpFirst[54];
  bool grpSingle[64];

  for(uint j = 0; j < 64; ++j) {
    grpFirst[j] = 0;
    grpSingle[j] = true;
  }
  
  for(uint nId = 1; nId < 54264; ++nId) {
    if( probs[nId][0] == 0 ) {
      grpAssign[nId] = 0;
    } else {
      int board[6];
      PositionFromBearoff(board, nId);

      int group = 0;
      for(int i = 5; i >= 0; --i) { 
	group += (0x1 << i) * !!board[i];
      }
      grpAssign[nId] = group;

      if( grpFirst[group] == 0 ) {
	grpFirst[group] = nId;
	// speedup, if alredy detected as non single, no checkes needed
      } else if( grpSingle[group] ) {
	if( memcmp(probs[nId],probs[grpFirst[group]],sizeof(probs[0])) != 0 ) {
	  grpSingle[group] = false;
	}
      }
    }
  }


  unsigned int sorted[54264];
  uint nToSort = 0;
  for(uint nId = 0; nId < 54264; ++nId) {
    if( probs[nId][0] > 0 && !grpSingle[grpAssign[nId]] ) {
      sorted[nToSort] = nId;
      ++nToSort;
    }
  }

  {
    Sorter c;
    std::sort(sorted, sorted + nToSort, c);
  }

  cout << "static GammonProbs all[] = {" << endl;
  
  unsigned int vals[54264];
  uint count = 0;
  {
    unsigned long* prev = 0;
    
    for(uint k = 0; k < nToSort; ++k) {
      uint const nId = sorted[k];
    
      if( ! prev || memcmp(probs[nId], prev, sizeof(probs[0])) != 0 ) {
	outp(probs[nId]);
	cout << "," << endl;
	
	prev = probs[nId];
	++count;
      }
	
      vals[nId] = count - 1;
    }
  }

  cout << "};" << endl;


  
  // Establish in this loop because it is convenient to do here
  unsigned int grpBase[64];
  unsigned int size[64];

  for(uint j = 1; j < 64; ++j) {
    if( ! grpSingle[j] ) {
      size[j] = 0;
      
      int to[6] = {-1,-1,-1,-1,-1,-1};

      int grpSize = 0;
      for(uint k = 0; k < 6; ++k) {
      	if( (j & (0x1 << k)) ) {
	  to[grpSize] = k;
	  ++grpSize;
	}
      }

      {
	int b[6];
	PositionFromBearoff(b, grpFirst[j]);

	int b1[6];
	for(uint k = 0; k < 6; ++k) {
	  if( to[k] >= 0 ) {
	    b1[k] = b[to[k]] - 1;
	  } else {
	    b1[k] = 0;
	  }
	}

	grpBase[j] = positionIndex(grpSize, b1);
      }

      const char* const aType =
	(count < 256) ? "unsigned char" : "unsigned word";
      
      cout << endl
		<< "static " << aType << " x" << j << "[] = {" << endl;
      
      for(uint nId = grpFirst[j]; nId < 54264; ++nId) {
	if( grpAssign[nId] == j ) {
	  
#if defined(DEBUG)
	  {
	    int b[6];
	    PositionFromBearoff(b, nId);
	    cout << "// " << size[j] << " "
		      << b[0] << " " << b[1] << " "<< b[2] << " "
		      << b[3] << " " << b[4] << " "<< b[5] << endl;
	  }
#endif	  
	  cout << vals[nId] << "," << endl;
	  ++ size[j];
	}
      }
      
      cout << "};" << endl << endl;
    }
  }

  cout << "GroupInfo const info[63] = {" << endl;

  for(uint j = 1; j < 64; ++j) {
    cout << "// " << j << " ";
    for(uint k = 0; k < 6; ++k) {
      cout << ((j & (0x1 << k)) ? "1" : "0");
    }
    cout << endl << "{ ";

    if( grpSingle[j] ) {
      uint const nId = grpFirst[j];
      
      outp(probs[nId]);
      cout << ", 0, 0, 0";
    } else {
      cout << "{0, 0, 0, 0}, x" << j << ", " << grpBase[j]
		<< ", " << size[j];
    }
    cout << "}, " << endl;
  }

  cout << "};" << endl;

  outit(cout, code1, sizeof(code1)/sizeof(code1[0]));

  return 0;
}
