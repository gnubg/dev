// -*- C++ -*-
/*
 * player.cc
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

#if !defined( PLAYER_H )
#define PLAYER_H

#include "equities.h"
#include "analyze.h"

class Player : public Analyze {
public:
  Player(void);
  ~Player();
  
  //float		acceptCubeVig;
  
  R1 const&	rollOrDouble(PrintMatchBoard const  board,
			     bool                   xOnPlay,
			     double                 odf,
			     bool                   APopt,
			     bool                   APshortcuts,
			     const float*           prb) const;

  bool		accept(PrintMatchBoard const  board,
		       bool                   xOnPlay,
		       uint                   nPlies,
		       double                 advantage,
		       Analyze::R1&           r) const;
  
  int		offerResign(uint                   nPlies,
			    uint                   nPliesVerify,
			    PrintMatchBoard const  board,
			    bool                   xOnPlay) const;
  
  bool		resigns(PrintMatchBoard const  b,
			uint                   nPoints,
			bool                   xOnPlay,
			uint                   nPlies,
			float*                 gammonChances) const;

  bool		resignsOpAtRoll(PrintMatchBoard const  board,
				uint const             nPoints,
				bool const             xOnPlay,
				uint const             nPlies,
				float* const           gammonChances) const;
  
//    bool		accept(PrintMatchBoard const  board,
//  		       float                  xwpf,
//  		       bool                   xOnPlay,
//  		       uint                   nPlies,
//		       Analyze::R1&           r) const;
  
private:
  void 		setEqTables(float xwpf) const;

  mutable float			lastXwpf;
  mutable Equities::EqTable	xwpfEtable;
  mutable Equities::EqTable	owpfEtable;

  mutable R1			doubleInfo;
};


#endif
