// -*- C++ -*-
#if !defined( BMS_H )
#define BMS_H

/*
 * bms.h
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


#include "bgdefs.h"

void
findBestMoves(movelist& pml, int nPlies,
	      int nDice0, int nDice1, int anBoard[ 2 ][ 25 ],
	      unsigned char* moveAuch,
	      bool xOnPlay,
	      unsigned int maxMoves, float maxDiff);

class SortMoves {
public:
  int operator ()(move const& i1, move const& i2) const {
    return i1.rScore > i2.rScore;
  }
};

extern void
scoreMoves(movelist&   pml,
	   int const   nPlies,
	   bool const  xOnPlay);

#endif
