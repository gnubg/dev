// -*- C++ -*-

/*
 * dice_gen.h
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

#if !defined( DICE_GEN_H )
#define DICE_GEN_H

#if defined( __GNUG__ )
#pragma interface
#endif

class GetDice {
public:
  GetDice(bool semiRand = true);
  ~GetDice();
  
  void 	get(int dice[2]);

  void	startSave(uint nSeq);
  
  void	startRetrive(void);

  void 	endSave(uint nSeq = 0);
  
  void	next(void);

  uint	curNseq(void) const;

  unsigned long  curSeed(void) const;
  
private:
  bool		semiRand;
  int		semiRandCounter;
  
  enum Mode {
    SAVING,
    RETRIVING,
    NONE,
  };
  
  struct OneSeq {
    OneSeq(void);
    ~OneSeq();

    void	roll(int dice[2], int& semiRandCounter);
    void	add(int dice[2]);
    void	get(int dice[2]);

    void	start(void);

    bool	empty(void) const;
    
    unsigned long const seed;
  private:
    
    uint	len;
    uint	allocated;
    int*	numbers;

    uint	cur;
    bool	synchronized : 8;
    bool	semi : 8;
  };

  uint		nSeq;
  OneSeq*	seqs;

  Mode		mode;
  uint		cur;
};

inline uint
GetDice::curNseq(void) const
{
  return nSeq;
}

inline unsigned long
GetDice::curSeed(void) const
{
  return seqs[cur].seed;
}

#endif
