// -*- C++ -*-
/*
 * misc.h
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

#if !defined( MISC_H )
#define MISC_H

#include "bgdefs.h"

inline bool
isRace(int const board[2][25])
{
  int nOppBack;
  
  for(nOppBack = 24; nOppBack >= 0; --nOppBack) {
    if( board[1][nOppBack] ) {
      break;
    }
  }
    
  nOppBack = 23 - nOppBack;

  for(int i = nOppBack + 1; i < 25; i++ ) {
    if( board[0][i] ) {
      return false;
    }
  }

  return true;
}

inline bool
posIsRace(const unsigned char* const auch)
{
  int b[2][25];
  
  PositionFromKey(b, const_cast<unsigned char*>(auch));

  return isRace(b);
}

inline bool
isBearoff(int const board[2][25])
{
  for(int i = 24; i >= 6; --i) {
    if( board[0][i] > 0 || board[1][i] > 0 ) {
      return false;
    }
  }

  return true;
}

#endif
