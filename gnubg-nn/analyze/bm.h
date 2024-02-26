// -*- C++ -*-
#if !defined( BM_H )
#define BM_H

/*
 * bm.h
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

#include <vector>

struct MoveRecord {
  unsigned char pos[10];

  int 		move[8];
  
  float		probs[5];

  float		matchScore;
};

#if 0
// unused code
enum EvalLevel {
  PNONE,
  
  PALL,

  P33,
};

extern int
FindBestMove_1_33(int anMove[8], int nDice0, int nDice1,
		  int anBoard[2][25], bool direction, EvalLevel l);
#endif

void
setPlyBounds(uint np, uint nm, uint na, float th);

extern int
findBestMove(int        anMove[8],
	     int const  nDice0,
	     int const  nDice1,
	     int        anBoard[2][25],
	     bool const direction,
	     uint const nPlies,
	     std::vector<MoveRecord>*	mRecord = 0,
	     bool reduced = false);

#endif
