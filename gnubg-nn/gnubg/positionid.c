/*
 * positionid.c
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1998-1999.
 *
 * An implementation of the position key/IDs at:
 *
 *   http://www.cs.arizona.edu/~gary/backgammon/positionid.html
 *
 * Please see that page for more information.
 *
 *
 * This library also calculates bearoff IDs, which are enumerations of the
 *
 *    c+6
 *       C
 *        6
 *
 * combinations of (up to c) chequers among 6 points.
 *
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
#include <assert.h>
#include <string.h>

#include "positionid.h"

static inline void
addBits(unsigned char auchKey[10], int const  bitPos, int const nBits)
{
  int const k = bitPos / 8;
  int const r = (bitPos & 0x7);
  
  unsigned int const b = (((unsigned int)0x1 << nBits) - 1) << r;
  
  auchKey[k] |= b;
  
  if( k < 8 ) {
    auchKey[k+1] |= b >> 8;
    auchKey[k+2] |= b >> 16;
  } else if( k == 8 ) {
    auchKey[k+1] |= b >> 8;
  }
}

extern void
PositionKey(CONST int anBoard[2][25], unsigned char auchKey[10])
{
  int i, iBit = 0;
  const int* j;

  memset(auchKey, 0, 10 * sizeof(*auchKey));

  for(i = 0; i < 2; ++i) {
    const int* const b = anBoard[i];
    for(j = b; j < b + 25; ++j) {
      int const nc = *j;

      if( nc ) {
	addBits(auchKey, iBit, nc);
	iBit += nc + 1;
      } else {
	++iBit;
      }
    }
  }
}

extern char*
PositionID(CONST int anBoard[2][25])
{
  unsigned char auchKey[ 10 ], *puch = auchKey;
  static char szID[ 15 ];
  char *pch = szID;
  static char aszBase64[ 64 ] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i;
    
  PositionKey( anBoard, auchKey );

  for( i = 0; i < 3; i++ ) {
    *pch++ = aszBase64[ puch[ 0 ] >> 2 ];
    *pch++ = aszBase64[ ( ( puch[ 0 ] & 0x03 ) << 4 ) |
			( puch[ 1 ] >> 4 ) ];
    *pch++ = aszBase64[ ( ( puch[ 1 ] & 0x0F ) << 2 ) |
			( puch[ 2 ] >> 6 ) ];
    *pch++ = aszBase64[ puch[ 2 ] & 0x3F ];

    puch += 3;
  }

  *pch++ = aszBase64[ *puch >> 2 ];
  *pch++ = aszBase64[ ( *puch & 0x03 ) << 4 ];

  *pch = 0;

  return szID;
}

extern void
PositionFromKey(int anBoard[2][25], unsigned char* pauch)
{
  int i = 0, j  = 0, k;
  unsigned char* a;

  memset(anBoard[0], 0, sizeof(anBoard[0]));
  memset(anBoard[1], 0, sizeof(anBoard[1]));
  
  for(a = pauch; a < pauch + 10; ++a) {
    unsigned char cur = *a;
    
    for(k = 0; k < 8; ++k) {
      if( (cur & 0x1) ) {
	++anBoard[i][j];
      } else {
	if( ++j == 25 ) {
	  ++i;
	  j = 0;
	}
      }
      cur >>= 1;
    }
  }
}


static int
Base64(char ch)
{
  if( ch >= 'A' && ch <= 'Z' )
    return ch - 'A';

  if( ch >= 'a' && ch <= 'z' )
    return ch - 'a' + 26;

  if( ch >= '0' && ch <= '9' )
    return ch - '0' + 52;

  if( ch == '+' )
    return 62;

  return 63;
}

extern void
PositionFromID(int anBoard[2][25], CONST char* pchEnc)
{
  unsigned char auchKey[ 10 ], ach[ 15 ], *pch = ach, *puch = auchKey;
  int i;

  memset(ach, 0, 15);

  for( i = 0; i < 14 && pchEnc[ i ]; i++ )
    pch[ i ] = Base64( pchEnc[ i ] );

  pch[ i ] = 0;
    
  for( i = 0; i < 3; i++ ) {
    *puch++ = ( pch[ 0 ] << 2 ) | ( pch[ 1 ] >> 4 );
    *puch++ = ( pch[ 1 ] << 4 ) | ( pch[ 2 ] >> 2 );
    *puch++ = ( pch[ 2 ] << 6 ) | pch[ 3 ];

    pch += 4;
  }

  *puch = ( pch[ 0 ] << 2 ) | ( pch[ 1 ] >> 4 );

  PositionFromKey( anBoard, auchKey );
}


static int anCombination[ 21 ][ 6 ], fCalculated = 0;

static int
InitCombination( void )
{

  int i, j;

  for( i = 0; i < 21; i++ )
    anCombination[ i ][ 0 ] = i + 1;
    
  for( j = 1; j < 6; j++ )
    anCombination[ 0 ][ j ] = 0;

  for( i = 1; i < 21; i++ )
    for( j = 1; j < 6; j++ )
      anCombination[ i ][ j ] = anCombination[ i - 1 ][ j - 1 ] +
	anCombination[ i - 1 ][ j ];

  fCalculated = 1;
    
  return 0;
}

static int
Combination(int n, int r)
{
  assert( n > 0 );
  assert( r > 0 );
  assert( n <= 21 );
  assert( r <= 6 );

  if( !fCalculated )
    InitCombination();
    
  return anCombination[ n - 1 ][ r - 1 ];
}

static int
PositionF(int fBits, int n, int r)
{
  if( n == r )
    return 0;

  return ( fBits & ( 1 << ( n - 1 ) ) ) ? Combination( n - 1, r ) +
    PositionF( fBits, n - 1, r - 1 ) : PositionF( fBits, n - 1, r );
}

extern unsigned short
PositionBearoff(CONST int anBoard[6])
{
  int i, fBits, j;

  for( j = 5, i = 0; i < 6; i++ )
    j += anBoard[ i ];

  fBits = 1 << j;
    
  for( i = 0; i < 6; i++ ) {
    j -= anBoard[ i ] + 1;
    fBits |= ( 1 << j );
  }

  return PositionF( fBits, 21, 6 );
}

static int
PositionInv(int nID, int n, int r)
{
  int nC;

  if( !r )
    return 0;
  else if( n == r )
    return ( 1 << n ) - 1;

  nC = Combination( n - 1, r );

  return ( nID >= nC ) ? ( 1 << ( n - 1 ) ) |
    PositionInv( nID - nC, n - 1, r - 1 ) : PositionInv( nID, n - 1, r );
}

extern void
PositionFromBearoff(int anBoard[6], unsigned short usID)
{
  int fBits = PositionInv(usID, 21, 6);
  int i, j;

  for( i = 0; i < 6; i++ )
    anBoard[i] = 0;
    
  for( j = 5, i = 0; j >= 0 && i < 21; i++ ) {
    if( fBits & ( 1 << i ) )
      j--;
    else
      anBoard[j]++;
  }
}


extern unsigned short
positionIndex(int g, int anBoard[6])
{
  int i, fBits;
  int j = g - 1;

  for(i = 0; i < g; i++ )
    j += anBoard[ i ];

  fBits = 1 << j;
    
  for(i = 0; i < g; i++) {
    j -= anBoard[ i ] + 1;
    fBits |= ( 1 << j );
  }

  return PositionF( fBits, 15, g );
}
