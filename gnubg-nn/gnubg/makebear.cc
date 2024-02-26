/*
 * makebear.cc
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1997-1999.
 *    Joseph Heled <joseph@gnubg.org>, 2000
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

/* Generate bearoff database as C code to be compiled, with access methods. */
/* Seperate into two files, which compile faster then one big file          */
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <unistd.h>

extern "C" {
#include "eval.h"
#include "positionid.h"
}

using std::ostream;
using std::ofstream;
using std::endl;

static unsigned short ausRolls[ 54264 ], aaProb[ 54264 ][ 32 ];

#if defined( HAVE_BEAROFF2 )
static double aaEquity[ 924 ][ 924 ];
#endif

static void
BearOff(int const nId)
{
  int i, iBest, iMode, j, anRoll[ 2 ], anBoard[ 2 ][ 25 ],
    anBoardTemp[ 2 ][ 25 ], aProb[ 32 ];
  movelist ml;
  unsigned short us;

  PositionFromBearoff( anBoard[ 1 ], nId );

  for( i = 0; i < 32; i++ )
    aProb[ i ] = 0;
    
  for( i = 6; i < 25; i++ )
    anBoard[ 1 ][ i ] = 0;

  for( i = 0; i < 25; i++ )
    anBoard[ 0 ][ i ] = 0;
    
  for( anRoll[ 0 ] = 1; anRoll[ 0 ] <= 6; anRoll[ 0 ]++ )
    for( anRoll[ 1 ] = 1; anRoll[ 1 ] <= anRoll[ 0 ]; anRoll[ 1 ]++ ) {
      GenerateMoves( &ml, anBoard, anRoll[ 0 ], anRoll[ 1 ] );

      us = 0xFFFF; iBest = -1;
	    
      for( i = 0; i < ml.cMoves; i++ ) {
	PositionFromKey( anBoardTemp, ml.amMoves[ i ].auch );

	j = PositionBearoff( anBoardTemp[ 1 ] );

	if( ausRolls[ j ] < us ) {
	  iBest = j;
	  us = ausRolls[ j ];
	}
      }

      if( anRoll[ 0 ] == anRoll[ 1 ] )
	for( i = 0; i < 31; i++ )
	  aProb[ i + 1 ] += aaProb[ iBest ][ i ];
      else
	for( i = 0; i < 31; i++ )
	  aProb[ i + 1 ] += ( aaProb[ iBest ][ i ] << 1 );
    }
    
  for( i = 0, j = 0, iMode = 0; i < 32; i++ ) {
    j += ( aaProb[ nId ][ i ] = ( aProb[ i ] + 18 ) / 36 );
    if( aaProb[ nId ][ i ] > aaProb[ nId ][ iMode ] )
      iMode = i;
  }

  aaProb[ nId ][ iMode ] -= ( j - 0xFFFF );
    
  for( j = 0, i = 1; i < 32; i++ )
    j += i * aProb[ i ];

  ausRolls[ nId ] = j / 2359;
}

#if defined( HAVE_BEAROFF2 )
static void
BearOff2( int nUs, int nThem )
{

  int i, iBest, j, anRoll[ 2 ], anBoard[ 2 ][ 25 ],
    anBoardTemp[ 2 ][ 25 ];
  movelist ml;
  double r, rTotal;

  PositionFromBearoff( anBoard[ 0 ], nThem );
  PositionFromBearoff( anBoard[ 1 ], nUs );

  for( i = 6; i < 25; i++ )
    anBoard[ 1 ][ i ] = anBoard[ 0 ][ i ] = 0;

  rTotal = 0.0;
    
  for( anRoll[ 0 ] = 1; anRoll[ 0 ] <= 6; anRoll[ 0 ]++ )
    for( anRoll[ 1 ] = 1; anRoll[ 1 ] <= anRoll[ 0 ]; anRoll[ 1 ]++ ) {
      GenerateMoves( &ml, anBoard, anRoll[ 0 ], anRoll[ 1 ] );

      r = -1.0; iBest = -1;
	    
      for( i = 0; i < ml.cMoves; i++ ) {
	PositionFromKey( anBoardTemp, ml.amMoves[ i ].auch );

	j = PositionBearoff( anBoardTemp[ 1 ] );

	if( 1.0 - aaEquity[ nThem ][ j ] > r ) {
	  iBest = j;
	  r = 1.0 - aaEquity[ nThem ][ j ];
	}
      }

      if( anRoll[ 0 ] == anRoll[ 1 ] )
	rTotal += r;
      else
	rTotal += r * 2.0;
    }

  aaEquity[ nUs ][ nThem ] = rTotal / 36.0;
}
#endif

static void
calc( void )
{
  int i;
    
  aaProb[ 0 ][ 0 ] = 0xFFFF;
  for( i = 1; i < 32; i++ )
    aaProb[ 0 ][ i ] = 0;

  ausRolls[ 0 ] = 0;

  //cerr << "bearoff 1" << endl;
  
  for( i = 1; i < 54264; i++ ) {
    BearOff( i );
  }

#if defined( HAVE_BEAROFF2 )
  for( i = 0; i < 924; i++ ) {
    aaEquity[ i ][ 0 ] = 0.0;
    aaEquity[ 0 ][ i ] = 1.0;
  }

  //cerr << "bearoff 2(1)" << endl;
  
  for( i = 1; i < 924; i++ ) {
    int j;
    for( j = 0; j < i; j++ )
      BearOff2( i - j, j + 1 );
  }

  //cerr << "bearoff 2(2)" << endl;
  
  for( i = 0; i < 924; i++ ) {
    int j;
    for( j = i + 2; j < 924; j++ )
      BearOff2( i + 925 - j, j );
  }
#endif
}


static 
const char* const code1[] = {
  "#if defined( __GNUG__ )",
  "#pragma implementation \"br.h\"",
  "#endif",
  "",
  "#include <assert.h>",
  "#include <memory.h>",
  "",
  "#include \"br.h\"",
  "",
  "struct BR {",
  "  // number of probabilities",
  "  unsigned long len : 6;",
  "  // p[0] is probablity of bearing off in 'start' rolls",
  "  unsigned long start : 3;",
  "  // array of probabilities.",
  "  unsigned long off : 23;",
  "};",
  "",
  "static unsigned short const x[] = {",
};

static
const char* const code2[] = {
  "",
  "};",
  "",
  "static BR const z[54264] = {",
};

static
const char* const code3[] = {
  "};",
  "",
  "",
  "extern \"C\" void",
  "getBearoffProbs(unsigned int n, int p[32])",
  "{",
  "  //assert(n < 54264);",
  "",
  "  BR const& b = z[n];",
  "",
  "  memset(p, 0, 32 * sizeof(p[0]));",
  "  const unsigned short* v = &x[b.off];;",
  "  ",
  "  for(unsigned int k = b.start; k < b.start + b.len; ++k) {",
  "    p[k] = *(v++);",
  "  }",
  "}",
  "",
  "extern \"C\" void",
  "getBearoff(unsigned int n, B* b)",
  "{",
  "  //assert(n < 54264);",
  "",
  "  BR const& d = z[n];",
  "",
  "  b->len = d.len;",
  "  b->start = d.start;",
  "",
  "  const unsigned short* v = &x[d.off];;",
  "  ",
  "  for(unsigned int k = 0; k < d.len; ++k) {",
  "    b->p[k] = v[k] / 65535.0;",
  "  }",
  "}",
};

#if defined( HAVE_BEAROFF2 )

static
const char* const code4[] = {
  "",
  "#include \"br.h\"",
  "",
  "static unsigned short const b2[924*924] = {",
};


static
const char* const code5[] = {
  "};",
  "",
  "extern \"C\" unsigned int",
  "getBearoffProbs2(unsigned int n, unsigned int nOpp)",
  "{",
  "  return b2[n * 924 + nOpp];",
  "}",
};
#endif

static void
outit(ostream& o, const char* const lines[], unsigned int const n)
{
  for(unsigned int i = 0; i < n; ++i) {
    o << lines[i] << endl;
  }
}
  
int
main(int ac, char* av[])
{
#if defined( HAVE_BEAROFF2 )
  if( ac != 3 ) {
#else
  if( ac != 2 ) {
#endif
    std::cerr << "Wrong number of args" << endl;
    exit(1);
  }
  
  const char* name1 = av[1];
  
#if defined( HAVE_BEAROFF2 )
  const char* name2 = av[2];
#endif

  calc();
  
  ofstream s1(name1);

  if( ! s1.good() ) {
    std::cerr << "Error opening " << name1 << endl;
    exit(1);
  }
  
  outit(s1, code1, sizeof(code1)/sizeof(code1[0]));
  
  unsigned int k = 0;
  
  
  for(unsigned int n = 0; n < 54264; ++n) {

    for(unsigned int i = 0; i < 32; ++i) {
      unsigned int x = aaProb[n][i];

      if( x ) {
	if( k == 7 ) {
	  s1 << endl;
	  k = 0;
	} else {
	  ++k;
	}
	
	s1 << "0x" << std::hex << x << ", ";
      }
    }
  }
  
  outit(s1, code2, sizeof(code2)/sizeof(code2[0]));

  unsigned int o = 0;

  s1 << std::dec;
  
  for(int n = 0; n < 54264; ++n) {
    
    int i = 0;
    for(; i < 32; ++i) {
      unsigned int x = aaProb[n][i];

      if( x ) {
	break;
      }
    }

    int j = i + i;
    for(; j < 32; ++j) {
      unsigned int x = aaProb[n][j];;

      if( ! x ) {
	break;
      }
    }

    unsigned int l1 = j - i;
    s1 << "{" << l1 << "," << i << "," << o << "}," << endl;

    o += l1;
  }

  outit(s1, code3, sizeof(code3)/sizeof(code3[0]));

  if( ! s1.good() ) {
    exit(1);
  }
  
#if defined( HAVE_BEAROFF2 )
  ofstream s2(name2);

  if( ! s2.good() ) {
    exit(1);
  }
  
  outit(s2, code4, sizeof(code4)/sizeof(code4[0]));

  int j = 0;

  for(int k = 0; k < 924; ++k) {
    for(int i = 0; i < 924; ++i) {
      unsigned short x = (unsigned short)(aaEquity[k][i] * 65535.5);

      s2 << std::hex << "0x" << x << ", ";

      if( j == 7 ) {
	s2 << endl;
	j = 0;
      } else {
	++j;
      }
    }
  }

  outit(s2, code5, sizeof(code5)/sizeof(code5[0]));
  
  if( ! s2.good() ) {
    exit(1);
  }
#endif
  
  return 0;
}
