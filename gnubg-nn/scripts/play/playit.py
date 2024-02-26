#!/usr/bin/env pygnubg 
""" synopsis: play a position several times, output in fibs format
usage: playit [-n games=10] [-p ply=0] [--plyx N] [--plyo N]
              [-s away|xscore oscore nmatch]
              [--use-cube] [-f] [-q]
              [--netx name] [--neto name]  pos
 -q - give X and O same dice rolls in alternating games"""

import sys, string, time, glob, getopt, operator

from bgutil import *


def usage() :
  print >> sys.stderr, sys.argv[0],


nGames = 10

# indexes for players info
O = 0
X = 1

def sideAsChar(side) :
  if side == X :
    return 'X'
  if side == O :
    return 'O'
  assert 0

verbose = 0

ply = [0,0]
sc = [0,0]
nMatch = 0

cube = 1
useCube = 0
cubeOwner = None
crawford = 0

net = [None,None]
et = [None,None]

name = ["",""]

show = 0
qrand = 0

Eager = [0,0]

startDice = None
rseed = -1
tStat = 0
nGameStart = 0
startingPlayer = X

optlist, args = getopt.getopt(sys.argv[1:], "fp:n:s:qd:", \
  [ 'plyx=', 'plyo=', 'netx=', 'neto=', 'etx=', 'eto=', 'player=', \
    'shortcuts', 'use-cube', 'seed=', 'tstat', 'cont=' ] )

for o, a in optlist:
  if o == '-f':
    show = 1
  elif o == '-p':
    ply[X] = int(a)
    ply[O] = ply[X]
  elif o == '-n':
    nGames = int(a)
  elif o == '-s' :
    s = [int(x) for x in a.split()]
    if len(s) == 1 :
      nMatch = int(s[0])
      scx = 0
      sco = 0
    else :
      sc[X],sc[O],nMatch = s
    del s
  elif o == '--plyx' :
    side = X
    ply[X] = int(a)
  elif o == '--plyo' :
    side = O
    ply[O] = int(a)
  elif o == '--netx' :
    try :
      net[X] = gnubg.net.get(a)
      name[X] = '(' + a + ')'
    except :
      print >> sys.stderr, "faild to load net",a
      sys.exit(1)
  elif o == '--neto' :
    try :
      net[O] = gnubg.net.get(a)
      name[O] = '(' + a + ')'
    except :
      print >> sys.stderr, "faild to load net",a
      sys.exit(1)
  elif o == '--etx' :
    et[X] = a
  elif o == '--eto' :
    et[O] = a
  elif o == '-d' :
    startDice = a.split()[0:2]
  elif o == '--use-cube' : 
    useCube = 1
  elif o == '--shortcuts' : 
    gnubg.set.shortcuts(1, 10)
  elif o == '--seed' : 
    rseed = long(a)
  elif o == '-q' :
    qrand = 1
  elif o == '--tstat' :
    tStat = 1
    tstat = []
    tstats = [0.0,]*5
  elif o == '--cont' :
    score = dict()
    s = [int(p) for p in a.split()]
    score[X,1],score[X,2],score[X,3],\
    score[O,1],score[O,2],score[O,3] = s
    nGameStart = reduce(operator.add, s)
  elif o == '--player' :
    # reverse since loop reverse at the start
    if a == 'X' :
      startingPlayer = O
    elif a == 'O' :
      startingPlayer = X
    else :
      assert 0
    
if nMatch == 1 :
  gnubg.set.score(1, 1, 0)
  nMatch = 0
  
if nMatch == 0 :
  # skip if continuing
  if 'score' not in globals() :
    score = dict()
    for s in range(1,4) :
      for w in (X,O) :
        score[w,s] = 0
else :
  count = dict()
  for x in range(sc[X], nMatch + 1) :
    for o in range(sc[O], nMatch + 1) :
      count[x,o] = 0


name[X] = "gnu" + str(ply[X]) + "ply" + name[X]
name[O] = "gnu" + str(ply[O]) + "ply" + name[O]

if name[X] == name[O] :
  name[X] = name[X] + 'X'
  name[O] = name[O] + 'O'
  
if net[X] and not net[O]:
  net[O] = gnubg.net.current()


if net[O]  and not net[X] :
  net[X] = gnubg.net.current()

displayEvery = 0x3f
if ply[X] > 1 or ply[O] > 1 :
  displayEvery = 0x0
elif ply[X] > 0 or ply[O] > 0 :
  displayEvery = 0x7
  
def otherSide(side) :
  if side == X :
    return O
  return X


def setFor(side) :
  if net[X] != net[O] :
    gnubg.net.set( net[side] )
  
  if et[X] != et[O] :
    gnubg.set.equities(et[side])
  

logDoubles = 0

def doublesBi(pos, side, np) :
  info = gnubg.doubleroll(pos, n = np, i = 1, s = side)
  
  if info[0] :
    if logDoubles and cube == 1 :
      print >> sys.stderr, \
            "gnubg.doubleroll(",pos,",n =",np,",s =",side,"i=1)"
  return info
  
 
l2s = listToString

def doublesX(pos) :
  return doublesBi(swapSides(pos), 'X', 0)

def doublesO(pos) :
  return doublesBi(pos,'O', 0)

if len(args) :
  b = args[0]
  if len(b.split()) == 26 :
    initialPos = s2t(b)
  else :  
    initialPos = gnubg.board(b)
else :
  initialPos = startPos

started = [0,0]
if qrand and nGameStart :
  assert nGameStart % 1 == 0
  started = [nGameStart/2, nGameStart/2]

initialCube = cube
initialOwner = cubeOwner

seedList = list()
if qrand :
  if rseed >= 0 :
    random.seed(rseed)
  for i in range((nGames+1)/2) :
    seedList.append(random.randint(-sys.maxint,sys.maxint-1))
    
  if nGameStart :
    seedList = seedList[(nGameStart+1)/2 :]
    
for n in range(nGameStart, nGames) :

  if rseed >= 0 :
    gnubg.set.seed(rseed + 256*n)
    
  if nMatch > 0 :
    cube = initialCube
    cubeOwner = initialOwner
    
    gnubg.set.score(nMatch - sc[O], nMatch - sc[X], crawford)
    if cube > 1 :
      gnubg.set.cube(cube, sideAsChar(cubeOwner))
    
  nMove = 0
  
  pos = initialPos

  if show :
    if nMatch > 0 :
      print "Score is %d-%d in a %d point match." % (sc[X],sc[O],nMatch)
    else :
      print "Score is 0-0 in a money game."
    
    print name[X],"is X -",name[O],"is O"
    if pos != startPos :
      print "Board:", l2s( pos[1:-1] )

    ob = pos[0]
    xb = pos[-1]
    if xb != 0 or ob != 0 :
      print "Bar: O-%d X-%d" % (abs(ob),abs(xb))

    if initialCube > 1 :
      print "Cube:",initialCube,"Owner:",initialOwner

  if pos == startPos :
    if seedList :
      gnubg.set.seed(seedList[0])

    dice = gnubg.roll()
    
    while dice[0] == dice[1] :
      dice = gnubg.roll()

    if seedList :
      if n & 1 :
        seedList = seedList[1:]
	who = X
      else :
	who = O
    else :
      
      if dice[0] > dice[1] :
	who = X
      else :
	who = O

    started[who] += 1
  else :
    if not startDice :
      dice = gnubg.roll()
    else :
      dice = startDice

    who = startingPlayer
    if who == O :
      pos = swapSides(pos)
      
  if tStat :
    del tstat
    
  while gameOn(pos) :
    who = otherSide(who)

    setFor(who)

    if tStat and not "tstat" in globals():
      if gnubg.classify(pos) == gnubg.c_bearoff :
        tstat = gnubg.probs(pos, 0)
        if who == O :
          tstat = [1-tstat[0],tstat[3],tstat[4],tstat[1],tstat[2]]
        if verbose:
          print l2s(tstat)
          print "B",sideAsChar(who),l2s(pos)
        
    if useCube and not crawford and nMove and (cube == 1 or cubeOwner == who) :
      f = (doublesX,doublesO)[who == O]
      
      db,tk = f(pos) [0:2] 
      if db :
        if net[X] != net[O] :
	  setFor( otherSide(who) )
	  tk = f(pos) [1]
	  setFor( who )
	
	if show :
	  print "%s: doubles" % sideAsChar(who)

	  print  "%s: %s" % \
                (sideAsChar(otherSide(who)), ("rejects","accepts")[tk])
	
        if not tk :
	  break

	cube *= 2
	cubeOwner = otherSide(who)
	gnubg.set.cube(cube, sideAsChar(cubeOwner))

    nMove += 1

    move, b = gnubg.bestmove(pos, dice[0], dice[1], \
                          n = ply[who], s = sideAsChar(who), b = 1)

    pos = gnubg.boardfromkey(b)

    if show :
      print "%s: (%d %d)" % (sideAsChar(who), dice[0],dice[1]),
      if len(move) == 0 :
	print " can't move"
      else :
        for m1,m2 in  move :
	  print " %d-%d" % (m1, m2),

        print

    dice = gnubg.roll()

  if gameOn(pos) :
    p = cube
  else :
    p = cube * int(-eq(gnubg.probs(pos,0)) + 0.5)

  if show :
    print "%s: wins %d points" % (sideAsChar(who),p)

  if nMatch == 0 :
    score[who,p] += 1

    if tStat and not "tstat" in globals():
      tstat = gnubg.probs(pos, 0)

      if who == X :
        tstat = [1-tstat[0],tstat[3],tstat[4],tstat[1],tstat[2]]

    if tStat :
       tstats = map(operator.add, tstats, tstat)
       
    if verbose:
      print sideAsChar(who),"won"
    
    if not show :

      t = [0,0]
      t[X] = score[X,1] + score[X,2] + score[X,3]
      t[O] = score[O,1] + score[O,2] + score[O,3]
      total = t[X] + t[O]

      if total == nGames or (total & displayEvery) == 0 :
        if started[X] != 0 or started[O] != 0 :
	  print "x",started[X],"o",started[O]

        tot = [0,0]
        tot[X] = score[X,1] + 2*score[X,2] + 3*score[X,3]
        tot[O] = score[O,1] + 2*score[O,2] + 3*score[O,3]
	
	print "%d, %.5lf ppg" % (total, (tot[X] - tot[O])/float(total))

	print "%.2lf %.2lf %.2lf %.2lf %.2lf %.2lf" % \
              ( (100.0 * t[X]) / total, \
                (100.0 * (score[X,2] + score[X,3])) / total, \
                (100.0 * score[X,3]) / total , \
                (100.0 * t[O]) / total , \
                (100.0 * (score[O,2] + score[O,3])) / total , \
                (100.0 * score[O,3]) / total )
	
        for w in (X, O) :
	  print sideAsChar(w),
          for s in range(1,4) :
	    print score[w,s],

          print "   (%6d/%6d)" % (tot[w], t[w])

        if tStat :
          for f in tstats :
            print "%.5lf" % (f/total),
          print

        sys.stdout.flush()
        
  else :
    if who == X :
      count[min(sc[X] + p,nMatch),sc[O]] += 1
    else :
      count[sc[X], min(sc[O] + p, nMatch)] += 1

    if n + 1 == nGames or ((n + 1) & 0x3f) == 0 :

      print (n + 1),"games"
      
      t = 0.0
      for x in range(sc[X], nMatch + 1) :
        for o in range(sc[O], nMatch + 1) :
          if count[x,o] > 0 :
	    f = (count[x,o]/ (n + 1.0))
	    e = gnubg.equities.value(nMatch - x, nMatch - o)
	    e = (1+e)/2.0
	    
	    print "%d,%d %d %g * %g" % (x,o,count[x,o],f,e)

	    t += f * e
            
      print "tot", t

