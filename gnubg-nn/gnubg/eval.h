/*
 * eval.h
 *
 * by Gary Wong <gary@cs.arizona.edu>, 1998-1999.
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

#ifndef _EVAL_H_
#define _EVAL_H_

#if defined(__cplusplus) || defined(__GNUC__)
#define CONST const
#else
#define CONST
#endif


#include <time.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define NUM_OUTPUTS 5

#define BETA_HIDDEN 0.1
#define BETA_OUTPUT 1.0

#define OUTPUT_WIN 0
#define OUTPUT_WINGAMMON 1
#define OUTPUT_WINBACKGAMMON 2
#define OUTPUT_LOSEGAMMON 3
#define OUTPUT_LOSEBACKGAMMON 4

/* Evaluation structure of database positions */
typedef struct _evaluation {
    time_t t;
    short asEq[ 6 ];
} evaluation;

#define GNUBG_WEIGHTS "gnubg.weights"
#define GNUBG_DATABASE "gnubg.bd"

typedef struct _move {
    int anMove[ 8 ];
    unsigned char auch[ 10 ];
    int cMoves, cPips;
    float rScore, *pEval;
} move;

extern move amMoves[];
// extern volatile int fInterrupt;

typedef struct _movelist {
    int cMoves; /* and current move when building list */
    int cMaxMoves, cMaxPips;
    int iMoveBest;
    float rBestScore;
    move *amMoves;
} movelist;

typedef enum _positionclass {
    CLASS_OVER = 0,  /* Game already finished */
    CLASS_BEAROFF2,  /* Two-sided bearoff database */
    CLASS_BEAROFF1,  /* One-sided bearoff database */
    CLASS_RACE,      /* Race neural network */
    CLASS_CRASHED,   /* */
    CLASS_CONTACT,   /* Contact neural network */
#if defined( CONTAINMENT_CODE )
    CLASS_BACKCONTAIN,
#endif
} positionclass;

#if defined( CONTAINMENT_CODE )
#define N_CLASSES (CLASS_BACKCONTAIN + 1)
#else
#define N_CLASSES (CLASS_CONTACT + 1)
#endif

#define CLASS_PERFECT CLASS_BEAROFF2

extern positionclass
ClassifyPosition(CONST int anBoard[2][25]);

extern CONST char* EvalInitialise(CONST char *szWeights
#if defined( LOADED_BO )
				  , CONST char* osDataBase, CONST char* tsDataBase
#endif 
#if defined( OS_BEAROFF_DB )
				  , CONST char* osDB
#endif
				  );

extern void 
EvalShutdown(void);

struct EvalNets_;

extern struct EvalNets_*
LoadNet(CONST char* szWeights, long cSize);

void
DestroyNets(struct EvalNets_* evalNets);

/* if evalNets == 0, return current nets and do nothing */

struct EvalNets_*
setNets(struct EvalNets_* evalNets);

typedef struct Stats_ {
  CONST char*   name;
  unsigned long nEvals;
  unsigned int	lookUps;
  unsigned int	hits;
} Stats;

extern Stats* netStats(void);

extern int EvalSave( CONST char *szWeights, int c );

extern void RollDice( int anDice[ 2 ] );

extern void
EvaluatePositionFast(CONST int anBoard[2][25], float arOutput[]);

extern int EvaluatePosition(CONST int anBoard[2][25], float arOutput[],
			    int nPlies, int wide,
			    int direction, float* p,
			    unsigned int snp,
			    unsigned char* pauch);

extern void
EvalBearoffOneSided(CONST int anBoard[2][25], float arOutput[]);

#if defined( OS_BEAROFF_DB )
void disableOSdb(void);
void enableOSdb(void);

extern int
EvalOSrace(CONST int anBoard[2][25], float arOutput[]);
#endif

void
NetEvalRace(CONST int anBoard[2][25], float arOutput[]);

extern int
EvaluatePositionToBO(CONST int anBoard[2][25], float arOutput[],
		     int direction);

extern void InvertEvaluation( float ar[ NUM_OUTPUTS ] );

extern int Rollout( int anBoard[ 2 ][ 25 ], float arOutput[], float arStdDev[],
		    int nPlies, int nTruncate, int cGames, int pure,
		    int direction, int verbose);

extern int FindBestMove( unsigned int nPlies, int anMove[ 8 ],
			 int nDice0, int nDice1,
			 int anBoard[ 2 ][ 25 ], unsigned int nc,
			 int direction );
extern int
FindPubevalMove( int nDice0, int nDice1, int anBoard[ 2 ][ 25 ],
		 int anMove[8] );

float*
NetInputs(CONST int anBoard[2][25], positionclass* pc, unsigned int* n);

void
NetEval(float* p, CONST int anBoard[2][25], positionclass pc, float* inputs);

extern int
TrainPosition(CONST int anBoard[2][25], float arDesired[], float alpha,
	      CONST int* k);

extern int
PruneTrainPosition(CONST int anBoard[2][25], float arDesired[], float alpha);

/* extern int DumpPosition( int anBoard[ 2 ][ 25 ], char *szOutput, int nPlies );
 */

extern void SwapSides( int anBoard[ 2 ][ 25 ] );
extern int GameStatus( CONST int anBoard[ 2 ][ 25 ] );

extern int EvalCacheStats( unsigned int *pc, unsigned int *pcLookup, unsigned int *pcHit );
extern void EvalCacheFlush(void);
extern void EvalCacheResize(int cSize);

#define GenerateMoves eGenerateMoves

extern int GenerateMoves( movelist *pml, CONST int anBoard[ 2 ][ 25 ],
			  int n0, int n1 );
extern int FindBestMoves( movelist *pml, float ar[][ NUM_OUTPUTS ], int nPlies,
			  int nDice0, int nDice1, CONST int anBoard[ 2 ][ 25 ],
			  int c, float d );

extern int neuralNetInit(positionclass  pc,
			 CONST char*    inputFuncName,
			 int            nHidden);
extern int
neuralNetInitPrune(positionclass pc, int nHidden);

extern float
playsToRace(CONST int anBoard[ 2 ][ 25 ], int cGames);

extern int setShortCuts(int use);
#if defined( PUBEVALFILTER )
extern int setPubevalShortCuts(unsigned int nMoves); 
#endif

extern void
sortPubEval(movelist* pml, int nDice0, int nDice1, CONST int anBoard[2][25]);
extern float
pubEvalVal(int race, int b[2][25]);


/*  extern void setNetShortCuts(unsigned int nMovesContact, */
/*  			   unsigned int nMovesRace); */
extern void setNetShortCuts(unsigned int n);

extern void
FindBestMoveInEval(int nDice0, int nDice1, int anBoard[2][25], int direction);

extern int
evalPrune(CONST int anBoard[2][25], float arOutput[]);

extern CONST char*
inputNameByClass(positionclass pc, unsigned int k);

extern CONST char*
inputNameByFunc(CONST char* inpFunc, unsigned int k);

#include "lib/sse.h"

#endif
