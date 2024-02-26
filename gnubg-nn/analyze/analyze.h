// -*- C++ -*-
#if !defined( ANALYZE_H )
#define ANALYZE_H

/*
 * analyze.h
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

#if defined( __GNUG__ )
#pragma interface
#endif

#include "defs.h"

class Advantage;
class GetDice;
struct _movelist;

///
class Analyze {
public:
  typedef short PrintMatchBoard[26];

  /// GNU bg board representation.
  typedef int GNUbgBoard[ 2 ][ 25 ];

  Analyze(void);
  
  ~Analyze(void);
  
  static const char* init(const char* netFile = 0, bool shortCuts = false);
  static const char* initializedWith(void);
  
  static uint	nPliesMove;
  static uint	nPliesVerifyBlunder;

  static uint	nPliesToDouble;
  static uint	nPliesToDoubleVerify;

  static float  jokerDiff;

  static uint   maxPliesJoker;
  
  static float  blunderTH;
  
  static float  cubeLife;

  static bool	calcLuck;

  static uint	netSearchSpace;
  
  static const char*	weightsVersion;
  
  static bool	useOSRinRollouts;
  
  bool	setScore(uint xAway, uint oAway);
  bool	setScore(uint xScore, uint oScore, uint matchLen);
  void	setCube(uint cube, bool xOwns);
  void	crawford(bool on);

  float matchEquity(void) const;

  ///
  class R1 {
  public:
    R1(uint n = 0) :
      nRolloutGames(n),
      rollOutProbs(true)
      {}
    
    // Plies level used to evaluate double
    uint	nPlies;

    uint	nRolloutGames;
    // cubeless probablities at @ref{nPlies}.
    float 	probs[6];

    /// When doing a cubeful rollout (@arg{nRolloutGames} > 0),
    /// select between a rollout or evaluation for position cubeless probs.
    //
    bool	rollOutProbs;

    float	money(void) const;
    float	match(void) const;
    
    //float	takeTH;

    float	matchProbNoDouble;
    float	matchProbDoubleTake;
    float	matchProbDoubleDrop;
    
    bool	actionTake;
    bool	actionDouble;
    bool	tooGood;
    
    void 	analyze(GNUbgBoard const  board,
			bool              xOnPlay,
			uint              nPlies,
			const Advantage*  ad = 0,
			//uint              nPliesEval = 0,
			const float*            prb = 0,
			bool              optimize = true,
			bool              cbfMoves = false);
    
    void	get(float* const p) const;
    
    void	setProbs(const float* p);

    void	setDecision(void);
  private:

    bool	xOnPlay;

    // Number of plies used when evaluating cubeless probablities for a
    // @dfn{leaf} position. This non zero only in develpment/research, where
    // you might want (say) nPlies == 0 and nPliesEval == 2.
    //
    //uint	nPliesEval;

    // When false, use same moves (regardless of cube) for all evaluations.
    //
    bool	fullEval;

    const Advantage* 	advantage;
    bool 		cbfMoves;
    
    float	cubefulEquity(GNUbgBoard const  anBoard,
			      bool              xOnPlay_,
			      uint              nPlies_,
			      uint              onPlayAway,
			      uint              opAway,
			      bool              onPlayHoldsCube,
			      uint              cube);
    
    float	cubefulEquity0(GNUbgBoard const  anBoard,
			       bool              xOnPlay_,
			       uint              onPlayAway,
			       uint              opAway,
			       uint              cube,
			       bool              onPlayHoldsCube);

    void	cubefulEquities(GNUbgBoard const  anBoard);
    
  };

#if defined(DEBUG)
  static long	debugFlags;
  static unsigned int level[32];

  enum DT {
    DOUBLE = 0,
    DOUBLE1 = 1,
    RLUCK = 2,
    PLAYERAC = 3,
  };

  static bool	setDebug(const char* st);
  static void	setDebug(uint t, uint l);
  
  static inline bool debugOn(uint t, uint l);
#endif

  void	analyze(R1&                    r,
		PrintMatchBoard const  board,
		bool                   direction,
		uint                   nPlies,
		uint                   nPliesVerify);

  void	analyze(R1&                    r,
		GNUbgBoard const       board,
		bool                   direction,
		uint                   nPlies,
		uint                   nPliesVerify);
  
  class Result {
  public:
    Result(uint maxMoves, float maxDiffFromBest, uint nr);
    
    ~Result();

    float	m(uint n) const;

    void	setBlunder(float range);

    uint const	maxMoves;
    
    float const	maxDiffFromBest;

    uint	nPlies;

    uint const 	moveRolloutGames;
    
    uint 	nMoves;

    uint 	actualMove;

    bool	blunder;

    bool	lucky;

    float	rollMinusPositionEquityDiff;

    float	preRollP;
    float	rollP;
    float	rollLuck;
    
    struct Move {
      int	desc[8];
    
      float*	probs;

      float	mwc;
    };

    bool	xonplay;
    
    // length maxMoves
    Move*	moves;

    bool	doubleAnaDone;
    
    R1		doubleAna;


    void 	sortByMatchMCW(bool xOnPlay);
  };
  
  bool		analyze(Result&                r,
			PrintMatchBoard const  board,
			bool                   xOnPlay,
			uint                   roll1,
			uint                   roll2,
			PrintMatchBoard const  after,
			bool                   firstMove);

  enum RolloutEndsAt {
    RACE,
    BEAROFF,
    OVER,
    AUTO
  };

  void  	rollout(PrintMatchBoard const b,
			bool                  xOnPlay,
			float                 arOutput[],
			float                 arStdDev[],
			uint                  nPlies,
			uint                  nTruncate,
			uint                  cGames,
			RolloutEndsAt         endsAt = AUTO);

  RolloutEndsAt rolloutTarget(GNUbgBoard const b) const;
  
  void  	rollout(GNUbgBoard const b,
			bool             xOnPlay,
			float            arOutput[],
			float            arStdDev[],
			uint             nPlies,
			uint             nTruncate,
			uint             cGames,
			int              nSeq,
			RolloutEndsAt    endsAt = AUTO);
  
  const float*	rolloutCubefull(GNUbgBoard const board,
				uint        nPlies,
				uint        nGames,
				bool        xOnPlay);

  void		probs(float*                 p,
		      PrintMatchBoard const  board,
		      bool                   xOnPlay,
		      uint                   nPlies,
		      uint                   n = 0,
		      bool                   getAuch = false) const;
  
  static bool	gameOn(GNUbgBoard const board);

  static void	set(PrintMatchBoard const  board,
		    GNUbgBoard             gBoard,
		    bool                   direction);
  
  static void   srandom(unsigned long l);
protected:

  void		rollMoves(Result& r, _movelist& ml, bool const xOnPlay);

  friend class R1;
  
  mutable float*		prr;
  mutable unsigned char*	pauch;

  bool		wide;

  float		matchRange;
  
  GetDice&	diceGen;
};


#if defined(DEBUG)
inline bool
Analyze::debugOn(uint const t, uint const l)
{
  if( t >= 32 ) {
    return false;
  }
    
  if( debugFlags & (0x1 << t) ) {
    return l <= level[t];
  }
  return false;
}
#endif

inline float
Analyze::R1::money(void) const
{
  return probs[0] + probs[1] + probs[2] -
    (probs[3] + probs[4] + probs[5]);
}

#endif
