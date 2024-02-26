// -*- C++ -*-
/*
 * racebg.h
 *
 * by Joseph Heled <joseph@gnubg.org>, 2002
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

#if !defined( RACEBG_H )
#define RACEBG_H

#if defined(__cplusplus) || defined(__GNUC__)
#define CONST const
#else
#define CONST
#endif

#if defined( __cplusplus )
extern "C" {
#endif

CONST long*
getRaceBGprobs(int board[6]);

#if defined( __cplusplus )
static unsigned int const RBG_NPROBS = 5;
#else
#define RBG_NPROBS 5
#endif
  
#if defined( __cplusplus )
}
#endif


#endif
