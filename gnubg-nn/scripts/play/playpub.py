#!/usr/bin/env pygnubg 

import gnubg
import random

from bgutil import *

O = 0
X = 1
show = 1

def rboard(b) :
  return tuple(-b for b in reversed(gnubg.boardfromkey(b)))

def pubMove(br, d1, d2) :
  mm = gnubg.moves(br, d1, d2, True)
  if len(mm) == 0 :
    return ([], gnubg.keyofboard(rboard(br)))
  
  s = [(gnubg.pubevalscore(rboard(b)),d,b) for b,d in mm]
  m = max(s)[1:]
  return (m[0], gnubg.keyofboard(rboard(m[1])))
  
def sideName(side) :
  if side == X :
    return 'gnup0'
  if side == O :
    return 'pubeval'
  assert 0

def sideAsChar(side) :
  if side == X :
    return 'X'
  if side == O :
    return 'O'
  assert 0

def otherSide(side) :
  if side == X :
    return O
  return X

random.seed()

money = False


def playOne(gameTo = None, money = False, ply = 0) :
  if money :
    gnubg.set.score(0, 0, 0)
  else :
    gnubg.set.score(1, 1, 0)
    
  pos = startPos

  if gameTo :
    print >> gameTo, "Score is 0-0" + ("." if money else "in a 1 point match.")
    print >> gameTo, sideName(X),"is X -",sideName(O),"is O"
    
  dice = gnubg.roll()
    
  while dice[0] == dice[1] :
    dice = gnubg.roll()

  if dice[0] > dice[1] :
    who = X
  else :
    who = O

  while gameOn(pos) :
    who = otherSide(who)

    if who == X :
      move, b = gnubg.bestmove(pos, dice[0], dice[1], n = ply, b=1)
    else :
      #move, b = pubMove(pos, dice[0], dice[1])
      move, b = gnubg.pubbestmove(pos, dice[0], dice[1])      
      #print "#",gnubg.keyofboard(pos),dice,move,b
    pos = gnubg.boardfromkey(b)

    if gameTo :
      print >> gameTo, "%s: (%d %d)" % (sideAsChar(who), dice[0],dice[1]),
      if len(move) == 0 :
	print >> gameTo
        # " can't move"
      else :
        for m1,m2 in  move :
	  print  >> gameTo, " %d-%d" % (m1, m2),

        print  >> gameTo
    dice = gnubg.roll()

  if money :
    p = 1 * int(-eq(gnubg.probs(pos,0)) + 0.5)
  else :
    p = 1
  if gameTo :
    print  >> gameTo, "%s: wins %d points" % (sideAsChar(who),p)

  return who,p
