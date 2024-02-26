// -*- C++ -*-

/*
 * bearoffgammon.h
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

#if !defined( BEAROFFGAMMON_H )
#define BEAROFFGAMMON_H

#if defined( __GNUG__ )
#pragma interface
#endif

#if defined(__cplusplus) || defined(__GNUC__)
#define CONST const
#else
#define CONST
#endif

// pack for space
struct GammonProbs {
  unsigned long p1 : 16;
  unsigned long p2 : 16;
  unsigned long p3 : 24;
  unsigned long p0 : 8;
};

#if defined( __cplusplus )
extern "C" {
#endif

CONST struct GammonProbs*
getBearoffGammonProbs(CONST int b[6]);
  
#if defined( __cplusplus )
}
#endif

#endif
