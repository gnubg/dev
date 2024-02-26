#!/usr/bin/env pygnubg 
""" synopsis: play a series of matchs, output in fibs format
 usage: matchplay [-m matchLen=5] [-ply ply=0] [-plyx N] [-plyo N]
               [-l] [-q]
               [-netx name] [-neto name]  nMatches """

import sys, string, time, glob, getopt

from bgutil import *

# indexes for players info
O = 0
X = 1

def sideAsChar(side) :
  if side == X :
    return 'X'
  return 'O'

def select(c, v1, v2) :
  if c :
    return v1
  return v2

nMatches = 1
matchLen = 5

ply = [0,0]
shortcuts = 0

net = [None, None]
et = [None, None]

name = ["",""]

log = 0
qrand = 0
qrand1 = 0

def otherSide(side) :
  return 1 - side

def setFor(side) :
  if net[X] != net[O] :
    gnubg.net.set( net[side] )
  
  if et[X] != et[O] :
    gnubg.set.equities(et[side])


def doublesBi(pos, side, np) :
  info = gnubg.doubleroll(pos, n = np, i = 1, s = sideAsChar(side))
  
  return info
 
def doublesX(pos) :
  return doublesBi(swapSides(pos), X, 0)

def doublesO(pos) :
  return doublesBi(pos, O, 0)

def acceptsX(pos) :
  return doublesO(pos)[1]

def acceptsO(pos) :
  return doublesX(pos)[1]

def doublesXDrop(pos) :
  i = doublesX(pos)
  if i[0] and i[1] and gnubg.classify(pos) == gnubg.c_race:
    if (i[4] - i[3]) <= 0.4 * (i[5]-i[3]) :
      return (0,1)
  return i

def acceptsXDrop(pos) :
  i = doublesO(pos)
  if i[1] :
    if (i[4] - i[3]) > (i[5]-i[3]) * 0.9 :
      return 0
    
  return i[1]

doubles = [None,None]
doubles[X] = doublesX
doubles[O] = doublesO

accepts = [None,None]
accepts[X] = acceptsX
accepts[O] = acceptsO

matches = [0,0]

optlist, args = getopt.getopt(sys.argv[1:], "lm:p:q", \
  [ 'plyx=', 'plyo=', 'netx=', 'neto=', 'etx=', 'eto=', 'shortcuts', \
    'seed=', 'xdstrat=', 'q1', 'cont=', 'log=' ] )

for o, a in optlist:
  if o == '-l':
    log = 1
  elif o == '--log' :
    log = 1
    logNameBase = a
  elif o == '-p':
    ply[X] = int(a)
    ply[O] = ply[X]
  elif o == '-m':
    matchLen = int(a)
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
  elif o == '--shortcuts' : 
    shortcuts = 10
    gnubg.set.shortcuts(1, shortcuts)
  elif o == '-q' :
    qrand = 1
  elif o == '--q1' :
    qrand = 1
    qrand1 = 1
  elif o == '--seed' :
    gnubg.set.seed(int(a))
    random.seed(int(a))
  elif o == '--xdstrat' :
    exec "accepts[X] = acceptsX" + a
  elif o == '--cont' :
    matches[X],matches[O] = [int(d) for d in a.split()]

name[X] = "gnu" + str(ply[X]) + "ply" + name[X]
name[O] = "gnu" + str(ply[O]) + "ply" + name[O]

if et[X] != et[O] :
  name[X] += "-" + et[X]
  name[O] += "-" + et[O]

if name[X] == name[O] :
  name[X] += "x"
  name[O] += "o"

defaultNet = gnubg.net.current()

if shortcuts :
  for s in (X,O) :
    if net[s] and net[s] != defaultNet :
      gnubg.net.set(net[s])
      gnubg.set.shortcuts(1, shortcuts)
  gnubg.net.set(defaultNet)
  
if net[X] and not net[O]:
  net[O] = defaultNet

if net[O] and not net[X] :
  net[X] = defaultNet

del defaultNet


if len(args) :
  nMatches = int(args[0])

nStart = matches[X] + matches[O]
seedList = list()
if qrand :
  for i in range((nMatches+1)/2) :
    seedList.append(random.randint(-sys.maxint,sys.maxint-1))

  if nStart :
    seedList = seedList[nStart/2:]

  if qrand1 :
    xx,xo,oo = 0,0,0
    nx,no = 0,0
    
for n in range(nStart, nMatches) :
  score = [0,0]
  crawford = 0
  
  if seedList :
    if qrand1 :
      matchSeeds = list()
      random.seed(seedList[0])

      for i in range(2 * matchLen - 1) :
        matchSeeds.append(random.randint(-sys.maxint,sys.maxint-1))
    else :
      gnubg.set.seed(seedList[0])
      
    if n & 1 :
      seedList = seedList[1:]
  
  while score[X] < matchLen and score[O] < matchLen :
    if qrand1 :
      gnubg.set.seed(matchSeeds[0])
      matchSeeds = matchSeeds[1:]
      
    gnubg.set.score(matchLen - score[O], matchLen - score[X], crawford)
    
    cube = 1
    cubeOwner = None
    
    nMove = 0
  
    pos = startPos

    dice = gnubg.roll()
	
    while dice[0] == dice[1] :
      dice = gnubg.roll()
      
    if (dice[0] > dice[1]) == select(qrand, n & 0x1, 1) :
      who = X
    else :
      who = O

    if log :
      print  "Score is %d-%d in a %d point match." \
            % (score[X], score[O], matchLen)
      print name[X],"is X -",name[O],"is O"


    while gameOn(pos) :
      who = otherSide(who)

      setFor(who)
    
      if not crawford and nMove and (cube == 1 or cubeOwner == who) and \
         score[who] + cube < matchLen :
        db,tk = doubles[who](pos)[0:2]
        if db :
          if net[X] != net[O] or et[X] != et[O] :
            op = otherSide(who)
            setFor( op )
            tk = accepts[op](pos)
            setFor( who )
	
          if log :
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

      if log :
        print "%s: (%d %d)" % (sideAsChar(who), dice[0],dice[1]),
        if len(move) == 0 :
          print "can't move"
        else :
          for m1,m2 in move :
            if False and nMove & 1 :
              m1,m2 = 25-m1,25-m2
              if m1 == 0 : m1 = "bar"
              if m2 == 25 : m2 = "off"
              
            print "%s-%s" % (str(m1), str(m2)),

          print

      dice = gnubg.roll()

    if gameOn(pos) :
      p = cube
    else :
      p = cube * int(-eq(gnubg.probs(pos,0)) + 0.5)

    if log :
      print "%s: wins %d points" % (sideAsChar(who),p)

    s = min(p + score[who], matchLen)
    score[who] = s

    # if whoever won now is 1-away and op is not -> crawford game
    # can happen only once
    
    if score[who] == matchLen - 1 and score[otherSide(who)] != matchLen - 1 :
      crawford = 1
    else :
      crawford = 0

  if score[X] == matchLen :
    matches[X] += 1
    if qrand1 : nx += 1
  else :
    matches[O] += 1
    if qrand1 : no += 1

  qstr = ""
  if qrand1 :
    if (n&1) :
      xx,xo,oo = xx + nx/2,xo + nx*no,oo + no/2
      nx,no = 0,0
      qstr = "(%d %d %d)" % (xo,xx,oo)
      
  per = (100.0 * matches[X])/(matches[X] + matches[O])
  fper = "%.2lf" % per
  sn = '-'
  if per >= float(fper) :
    sn = '+'
  print >> sys.stderr, "# X -",matches[X]," O -",matches[O],\
        "(" + fper + sn + "%)",qstr

