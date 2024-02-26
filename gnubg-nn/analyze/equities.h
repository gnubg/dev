// -*- C++ -*-
#if !defined( EQUITIES_H )
#define EQUITIES_H

/*
 * equities.h
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

#include <assert.h>

#include "defs.h"

/// Handles match score information.
//
class MatchState {
public:
  MatchState(void);

  /// Set match score.
  //
  bool	set(uint  xAway,
	    uint  oAway,
	    uint  cube,
	    bool  xOwns,
	    int   crawford);

  /// Reset to money
  //
  void	reset(void);
  
  // Return true if next game is crawford, false if not.
  //  When @arg{x}, X won @arg{nPoints}, otherwise O.
  //
  //bool	nextCrawford(bool x, uint nPoints);

  /// Equity range for current score and cube between a win by X and win by O.
  //
  float	range(void) const;

  ///
  inline bool cubeDead(void) const;
  
  /// X away in match.
  uint	xAway;
  
  /// O away in match.
  uint  oAway;

  /// Current cube.
  uint	cube;

  /// True when X owns cube.
  bool  xOwns;

  /// True when crawford game.
  bool	crawfordGame;
};


inline
MatchState::MatchState(void) :
  xAway(0),
  oAway(0),
  cube(1),
  xOwns(false),
  crawfordGame(false)
{}


inline bool
MatchState::cubeDead(void) const
{
  return ( cube != 1 && cube >= (xOwns ? xAway : oAway) );
}

namespace Equities {
  /// Equities graph.
  //  x axis is probability (x @in [01]), y is equity (y @in [-1 +1]).
  //
  //  Valid between @arg{xLow} and @arg{xHigh}.
  //
  //  Given as 2 points of a stright line. The low point is X drop point, the
  //  high is X cash-out point.
  //  
  struct Es {
    //@ low point

    ///
    float xLow;
    ///
    float yLow;

    //@ High point.

    ///
    float xHigh;
    ///
    float yHigh;

    /// Equity at @arg{x}.
    //  @arg{x} must be inside market window.
    //
    float y(float x) const;

    /// Probability at which equity is @arg{y}.
    //
    float x(float y) const;

    /// Equity at @arg{x}, for any @arg{x}.
    //  Return value of cash/drop-point for @arg{x} outside of market window.
    //
    float e(float x) const;

    /// @arg{x} as percent in window.
    //
    float r(float x) const;
    
    /// Reverse graph to O point of view.
    //
    void	reverse(void);
  };

  float		money(const float* p);
  
  /// Get equity at current and doubled cube for non crawford games.
  //
  //@multitable @arg
  //@item now       @tab  current equity
  //@item doubled   @tab  doubled equity (when not null)
  //@item  xAway
  //@itemx oAway    @tab  Match score
  //@item  cube     @tab  Cube value. When cube > 1, X holds it.
  //@item  xGammonRatio
  //@itemx oGammonRatio @tab Gammon rate as ratio of total wins (r @in [01])
  //@end
  //
  // If any away is 1, it is assumed to be a post crawford score.
  //
  void	get(Es&    now,
	    Es*    doubled,
	    uint   xAway,
	    uint   oAway,
	    uint   cube,
	    float  xGammonRatio,
	    float  oGammonRatio
#if defined( DEBUG )
	    ,int   debug
#endif
	    );

  /// Get equity at current and doubled cube for any score.
  //  Same as above, with @arg{crawfordGame} to indicate a crawford game.
  //
  void	get(Es&    now,
	    Es*    doubled,
	    uint   xAway,
	    uint   oAway,
	    uint   cube,
	    float  xGammonRatio,
	    float  oGammonRatio,
	    bool   crawfordGame
#if defined( DEBUG )
	    ,int   debug
#endif
	    );
  
  void	getCrawfordEq(Es& now, uint away, float gammonRatio);
  
  void  getCrawfordEq(Es&          e,
		      uint const   xAway,
		      uint const   oAway,
		      float const  xgr,
		      float const  ogr);

  /// Get current equity (approximated).
  //
  //  See @ref{get} for arguments. @arg{xOwns} is assumed to be always true.
  //
  //  The approximation is faster but less accurate. It uses the exact values
  //  at a grid spaced at 5% intervals, and computes a weighted average for
  //  the point using values of the four corners of the square it is in.
  //
  //  For example, @arg{xGammonRatio}=12.5%, @arg{oGammonRatio}=17.5% is
  //  exactly in the center of,
  //
  //@example
  //   10%,20%                     15%,20%
  //
  //             12.5%,17.5%
  //
  //   10%,15%                     15%,15%
  //@end
  //
  // And so can be computed as the simple average of those 4 points.
  //
  void	getApproximat(Es&    now,
		      Es&    doubled,
		      uint   xAway,
		      uint   oAway,
		      uint   cube,
		      float  xGammonRatio,
		      float  oGammonRatio
#if defined( DEBUG )
		      ,bool debug
#endif 
      );
  
  /// Match equity at @arg{xAway},@arg{oAway}.
  //  A 0 (or negative) away value is same as @dfn{win}.
  //
  inline float value(int xAway, int oAway);
  
  /// Match winning probablity at @arg{xAway},@arg{oAway}.
  //
  inline float prob(int xAway, int oAway);

  /// Convert probablity to equity.
  //
  inline float probToEquity(float p);

  /// Convert  equity to probablity.
  //
  inline float equityToProb(float e);

  /// Match winning probablity at @arg{xAway},@arg{oAway}.
  //  If !@arg{crawford} and any away is 1, return probablity for a post
  //  crawford score.
  //
  float prob(int xAway, int oAway, bool crawford);

  /// Convert cubeless probablities to match wins for current score.
  //  Use a simple model. Compute market window for match score using gammon
  //  rates from evaluation, then get value from window.
  //
  //  If outside of window, assume play without turning the cube, when
  //  cashing out at the appropriate point.
  //
  float 	mwc(const float* p, bool xOnPlay);

  /// Current match score.
  //
  extern MatchState	match;
  
  /// Compute match equity of @arg{probs}, with @arg{xOnPlay} to play.
  //
  float		normalizedEquity(const float* probs, bool xOnPlay);

  /// Probability of winning (xAway,1-away), post crawford
  //
  float		probPost(int xAway);
  
  /// Equity of winning (@arg{away},1-away), post crawford.
  //
  float		ePost(int away);

  /// X Equity, assuming he wins, at xAway,oAway, with given cube and gammon
  /// ratio.
  //
  float		eqWhenWin(float  xGammonRatio,
			  uint   xAway,
			  uint   oAway,
			  uint   cube);

  float		eqWhenWin(float  xGammonRatio,
			  float  xBGammonRatio,
			  uint   xAway,
			  uint   oAway,
			  uint   cube);

  float		eqWhenLose(float  oGammonRatio,
			   uint   xAway,
			   uint   oAway,
			   uint   cube);
  
  float		eqWhenLose(float  oGammonRatio,
			   float  oBGammonRatio,
			   uint   xAway,
			   uint   oAway,
			   uint   cube);
  
  float		eWhenWinPost(float  xGammonRatio,
			     uint   xAway,
			     uint   cube);

  float		eWhenWinPost(float  xGammonRatio,
			     float  xBGammonRatio,
			     uint   xAway,
			     uint   cube);

  float		eWhenWin(float  xGammonRatio,
			 uint   xAway,
			 uint   oAway,
			 uint   cube);

  float		eWhenWin(float  xGammonRatio,
			 float  xBGammonRatio,
			 uint   xAway,
			 uint   oAway,
			 uint   cube);
  
  float		eWhenLose(float  oGammonRatio,
			  uint   xAway,
			  uint   oAway,
			  uint   cube);
  
  float		eWhenLose(float  oGammonRatio,
			  float  oBGammonRatio,
			  uint   xAway,
			  uint   oAway,
			  uint   cube);

  // typedef float EqTable[25][25];
  
  //
  extern float equitiesTable[25][25];

  typedef float (*EqTable)[25];
  extern EqTable	curEquities;
  
  EqTable	getTable(float wpf, float gr);
  
  void set(float wpf, float gr);

  void	push(const EqTable eTable);
  
  const EqTable	pop(void);

  
  template <typename T>
  void set(T equities) {
    uint const n = sizeof(equities[0])/sizeof(equities[0][0]);

    for(uint k = 0; k < n; ++k) {
      for(uint k1 = 0; k1 < n; ++k1) {
	equitiesTable[k][k1] = equities[k][k1];
      }
    }
  }

  void set(uint xAway, uint oAway, float val);

  // Jacobs match equity table.
  extern float Jacobs[25][25];
  
  // Woolsey-Heinrich match equity table.
  extern float WoolseyHeinrich[15][15];

  extern float Snowie[15][15];

  extern float gnur[15][15];

  // mec match equity table.
  extern float mec26[25][25];

  extern float mec57[15][15];
};

inline float
Equities::money(const float* const probs)
{
  return (2*probs[0] - 1) + probs[1] + probs[2] - (probs[3] + probs[4]);
}


inline float
Equities::Es::y(float const x) const
{
  {                                        assert( xLow <= x && x <= xHigh ); }

  return yLow + ((yHigh - yLow)/(xHigh - xLow)) * (x - xLow);
}



inline float
Equities::Es::x(float const y) const
{
  {                                        assert( yLow <= y && y <= yHigh ); }
  
  return xLow + (y - yLow) * ((xHigh - xLow) / (yHigh - yLow));
}

inline float
Equities::Es::e(float const x) const
{
  if( x <= xLow ) {
    return yLow;
  }

  if( x >= xHigh ) {
    return yHigh;
  }

  return y(x);
}

inline float
Equities::Es::r(float const x) const
{
  return (x - xLow) / (xHigh - xLow);
}

inline void
Equities::Es::reverse(void)
{
  float const xl = xLow;
  xLow = 1.0 - xHigh;
  xHigh = 1.0 - xl;

  float const yl = yLow;

  yLow = -yHigh;
  yHigh = -yl;
}


inline float
Equities::probToEquity(float const p)
{
  return 2.0 * p - 1.0;
}

inline float
Equities::equityToProb(float const e)
{
  return (1 + e) / 2.0;
}


inline float
Equities::prob(int const a1, int const a2)
{
  if( a1 <= 0 ) {
    return 1.0;
  }

  if( a2 <= 0 ) {
    return 0.0;
  }
  {                                           assert( a1 <= 25 && a2 <= 25 ); }

  if( curEquities ) {
    return curEquities[a1-1][a2-1];
  }
  
  return equitiesTable[a1-1][a2-1];
}

inline float
Equities::value(int const a1, int const a2)
{
  return probToEquity( prob(a1, a2) );
}  

inline float
Equities::eqWhenWin(float const  xGammonRatio,
		    uint const   xAway,
		    uint const   oAway,
		    uint const   cube)
{
  {                                       assert( xAway != 1 && oAway != 1 ); }
  
  return ((1 - xGammonRatio) * value(xAway - cube, oAway) +
	  xGammonRatio * value(xAway - 2*cube, oAway));
}

inline float
Equities::eqWhenWin(float const  xGammonRatio,
		    float const  xBGammonRatio,
		    uint const   xAway,
		    uint const   oAway,
		    uint const   cube)
{
  {                                       assert( xAway != 1 && oAway != 1 ); }
  
  return ((1 - xGammonRatio) * value(xAway - cube, oAway) +
	  xGammonRatio *
	  ( (1-xBGammonRatio) * value(xAway - 2*cube, oAway) +
	    xBGammonRatio * value(xAway - 3*cube, oAway)));
}

inline float
Equities::eqWhenLose(float const  oGammonRatio,
		     uint const   xAway,
		     uint const   oAway,
		     uint const   cube)
{
  {                                       assert( xAway != 1 && oAway != 1 ); }
  
  return ((1 - oGammonRatio) * value(xAway, oAway - cube) +
	  oGammonRatio * value(xAway, oAway - 2*cube));
}

inline float
Equities::eqWhenLose(float const  oGammonRatio,
		     float const  oBGammonRatio,
		     uint const   xAway,
		     uint const   oAway,
		     uint const   cube)
{
  {                                       assert( xAway != 1 && oAway != 1 ); }
  
  return ((1 - oGammonRatio) * value(xAway, oAway - cube) +
	  oGammonRatio *
	  ( (1-oBGammonRatio) * value(xAway, oAway - 2*cube) +
	    oBGammonRatio * value(xAway, oAway - 3*cube)));
}

inline float
Equities::ePost(int const away)
{
  return 2.0 * probPost(away) - 1.0;
}

inline float
Equities::eWhenWinPost(float const  xGammonRatio,
		       uint const   xAway,
		       uint const   cube)
{
  float const e1 = ePost(xAway - cube);
  float const e2 = ePost(xAway - 2*cube);
  
  return (1 - xGammonRatio) * e1 + xGammonRatio * e2;
}

inline float
Equities::eWhenWinPost(float const  xGammonRatio,
		       float const  xBGammonRatio,
		       uint const   xAway,
		       uint const   cube)
{
  float const e1 = ePost(xAway - cube);
  float const e2 = ePost(xAway - 2*cube);
  float const e3 = ePost(xAway - 3*cube);

  return ((1 - xGammonRatio) * e1 +
	  xGammonRatio * ((1-xBGammonRatio) * e2 + xBGammonRatio * e3) );
}

inline float
Equities::eWhenWin(float const  xGammonRatio,
		   uint const   xAway,
		   uint const   oAway,
		   uint const   cube)
{
  if( xAway == 1 ) {
    return 1;
  }
  
  if( oAway == 1 ) {
    return eWhenWinPost(xGammonRatio, xAway, cube);
  }
  return eqWhenWin(xGammonRatio, xAway, oAway, cube);
}

inline float
Equities::eWhenWin(float const  xGammonRatio,
		   float const  xBGammonRatio,
		   uint const   xAway,
		   uint const   oAway,
		   uint const   cube)
{
  if( xAway == 1 ) {
    return 1;
  }
  
  if( oAway == 1 ) {
    return eWhenWinPost(xGammonRatio, xBGammonRatio, xAway, cube);
  }
  return eqWhenWin(xGammonRatio, xBGammonRatio, xAway, oAway, cube);
}

inline float
Equities::eWhenLose(float const  oGammonRatio,
		    uint const   xAway,
		    uint const   oAway,
		    uint const   cube)
{
  if( oAway == 1 ) {
    return -1;
  }
  
  if( xAway == 1 ) {
    // assumes symetry
    return -eWhenWinPost(oGammonRatio, oAway, cube);
  }
  return eqWhenLose(oGammonRatio, xAway, oAway, cube);
}


inline float
Equities::eWhenLose(float const  oGammonRatio,
		    float const  oBGammonRatio,
		    uint const   xAway,
		    uint const   oAway,
		    uint const   cube)
{
  if( oAway == 1 ) {
    return -1;
  }
  
  if( xAway == 1 ) {
    // assumes symetry
    return -eWhenWinPost(oGammonRatio, oBGammonRatio, oAway, cube);
  }
  return eqWhenLose(oGammonRatio, oBGammonRatio, xAway, oAway, cube);
}

#if defined( DEBUG )
#include <iosfwd>

std::ostream&
operator <<(std::ostream& s, Equities::Es const& e);
#endif

#endif
