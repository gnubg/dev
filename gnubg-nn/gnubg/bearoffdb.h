// -*- C++ -*-

/*
 * bearoffdb.h
 * adapted from:
 *
 * bearoff.h
 *
 * by Joern Thyssen <jthyssen@dk.ibm.com>, 2002
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
 * $Id$
 */

#if !defined( BEAROFFDB_H )
#define BEAROFFDB_H

#if defined( OS_BEAROFF_DB )

#if defined(__cplusplus) || defined(__GNUC__)
#define CONST const
#else
#define CONST
#endif


#include <hash.h>

typedef enum _bearofftype {
  BEAROFF_GNUBG,
  BEAROFF_EXACT_BEAROFF,
  BEAROFF_UNKNOWN,
  NUM_BEAROFFS
} bearofftype;

typedef struct _bearoffcontext {

  int h;          /* file handle */
  bearofftype bt; /* type of bearoff database */
  int nPoints;    /* number of points covered by database */
  int nChequers;  /* number of chequers for one-sided database */
  int fInMemory;  /* Is database entirely read into memory? */
  int fTwoSided;  /* one sided or two sided db? */
  int fMalloc;    /* is data malloc'ed? */

  /* one sided dbs */
  int fCompressed; /* is database compressed? */
  int fGammon;     /* gammon probs included */
  int fND;         /* normal distibution instead of exact dist? */
  int fHeuristic;  /* heuristic database? */
  /* two sided dbs */
  int fCubeful;    /* cubeful equities included */

  void *p;        /* pointer to data */

  hash *ph;        /* cache */

  unsigned long int nReads; /* number of reads */

} bearoffcontext;


enum _bearoffoptions {
  BO_NONE = 0,
  BO_IN_MEMORY = 1,
  BO_MUST_BE_ONE_SIDED = 2,
  BO_MUST_BE_TWO_SIDED = 4,
  BO_HEURISTIC = 8
};

extern bearoffcontext *
BearoffInit(CONST char *szFilename, int bo);

extern int
isBearoff ( bearoffcontext *pbc, CONST int anBoard[ 2 ][ 25 ] ); 

extern int
BearoffEval(bearoffcontext* pbc, CONST int anBoard[2][25], float arOutput[]);

extern void
BearoffClose ( bearoffcontext *pbc );

extern void
BearoffDist ( bearoffcontext *pbc, CONST unsigned int nPosID,
              float arProb[ 32 ], float arGammonProb[ 32 ],
              float ar[ 4 ],
              unsigned short int ausProb[ 32 ], 
              unsigned short int ausGammonProb[ 32 ] );

#endif

#endif
