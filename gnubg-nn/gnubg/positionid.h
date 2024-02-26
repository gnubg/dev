/*
 * positionid.h
 *
 * by Gary Wong, 1998-1999
 *
 */

#ifndef _POSITIONID_H_
#define _POSITIONID_H_


#if defined(__cplusplus) || defined(__GNUC__)
#define CONST const
#else
#define CONST
#endif

extern void PositionKey( CONST int anBoard[][25], unsigned char auchKey[10]);
extern char *PositionID( CONST int anBoard[ 2 ][ 25 ] );
extern unsigned short PositionBearoff( CONST int anBoard[ 6 ] );
extern void PositionFromKey( int anBoard[ 2 ][ 25 ],
			     unsigned char *puch );
extern void PositionFromID( int anBoard[ 2 ][ 25 ], CONST char *szID );
extern void PositionFromBearoff(int anBoard[ 6 ], unsigned short usID);
extern unsigned short positionIndex(int g, int anBoard[6]);

#if defined( __GNUC__ )

#include <string.h>

static inline int
EqualKeys(unsigned char auch0[10], unsigned char auch1[10])
{
  return memcmp(auch0, auch1, 10 * sizeof(auch0[0])) == 0;
}

#else

#define EqualKeys(auch0, auch1) \
  (memcmp(auch0, auch1, 10 * sizeof(auch0[0])) == 0)

#endif

#endif
