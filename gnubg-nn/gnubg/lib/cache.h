/*
 * cache.h
 *
 * by Gary Wong, 1997-1999
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

#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdlib.h>

#if defined( GARY_CACHE )

typedef int ( *cachecomparefunc )( void *p0, void *p1 );

typedef struct _cachenode {
    long l;
    void *p;
} cachenode;

typedef struct _cache {
    cachenode **apcn;
    int c, icp, cLookup, cHit;
    cachecomparefunc pccf;
} cache;

extern int CacheCreate( cache *pc, int c, cachecomparefunc pccf );
extern int CacheDestroy( cache *pc );
extern int CacheAdd( cache *pc, unsigned long l, void *p, size_t cb );
extern void *CacheLookup( cache *pc, unsigned long l, void *p );
extern int CacheFlush( cache *pc );
extern int CacheStats( cache *pc, unsigned int *pcLookup, unsigned int *pcHit );

#else

typedef struct _cacheNode {
  unsigned char auchKey[10];
  unsigned int 	nPlies;
  float 	ar[5 /*NUM_OUTPUTS*/];
} cacheNode;

/* name used in eval.c */
typedef cacheNode evalcache;

typedef struct _cache {
  cacheNode*	m;
  
  unsigned int size;
  unsigned int nAdds;
  unsigned int cLookup;
  unsigned int cHit;
} cache;

/* Cache size will be adjusted to a power of 2 */
int
CacheCreate(cache* pc, unsigned int size);

int
CacheResize(cache *pc, unsigned int cNew);

/* l is filled with a value which is passed to CacheAdd */
cacheNode*
CacheLookup(cache* pc, cacheNode* e, unsigned long* l);

void CacheAdd(cache* pc, cacheNode* e, unsigned long l);
void CacheFlush(cache* pc);
void CacheDestroy(cache* pc);
void CacheStats(cache* pc, unsigned int* pcLookup, unsigned int* pcHit);

#endif

#endif
