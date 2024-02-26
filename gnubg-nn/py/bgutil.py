# generic stuff

import random, string, select, gnubg, sys

random.seed()

def find(i, l) :
  """Index of item in list. return -1 if item is not in list"""
  
  try :
    return l.index(i)
  except :
    return -1


def listToString(l) :
  return string.join([str(x) for x in l])

def s2t(s) :
  """String s to tuple of ints"""
  return tuple([int(i) for i in s.split()])

def randPer(n) :
  """Random permutation of 1..n"""
  
  p = range(n)
  random.shuffle(p)
  return p

def ftime(nsec) :
  """Format seconds for printout"""

  nsec = int(nsec)
  
  if nsec >= 3600 :
    h = nsec / 3600
    nsec = nsec - h * 3600
    return "%d:%02d:%02d" % (h, nsec / 60, nsec % 60)

  return "%d:%02d" % (nsec / 60, nsec % 60)


def anyInput() :
  """return true if input waiting on stdin"""
  
  p = select.poll()
  p.register(sys.stdin.fileno())

  l = p.poll(0)
  if l :
    i,s = l[0]
    s = s & select.POLLIN
  else :
    s = 0
  del p
  
  return s

# BG stuff

def swapSides(b) :
  """Swap sides of board"""
  x = list(b)
  x.reverse()
  x = [-p for p in x]
  return tuple(x)


def eq(p) :
  """Equity from probablities"""
  
  p0,p1,p2,p3,p4 = p

  return 2*p0 - 1 + p1 + p2 - p3 - p4


def eqError(px,py) :
  """Equity difference of px and py"""
  
  px0,px1,px2,px3,px4 = px
  py0,py1,py2,py3,py4 = py

  return 2 * abs(px0 - py0) + abs(px1 - py1) + abs(px2 - py2) + \
         abs(px3 - py3) + abs(px4 - py4)


def formatedp(p, f = "%5.2lf", fact = 100) :
  return string.join([f % (fact*x) for x in p], ' ')

def formatedeq (p) :
  return "%.4lf (%s)" % (eq(p), formatedp(p))


def gameOn(pos) :
  return gnubg.classify(pos) != gnubg.c_over


def bestMoveOSR(pos, d1, d2, n = 1296, ply0filter = 0) :
  rb = sys.maxint + random.randint(-sys.maxint,sys.maxint-1)

  if ply0filter > 0 :
    sm = gnubg.sortedmoves(pos, d1, d2, 0, ply0filter)
    moves = [x[0] for x in sm]
  else :
    moves = gnubg.moves(pos, d1, d2)
  
  best = 10
  for m in moves :
    pos = swapSides(gnubg.boardfromkey(m))
    gnubg.set.seed(rb)
    
    pm = gnubg.probs(pos, gnubg.p_osr, n)
    e = eq(pm)
    if e < best :
      best = e
      bestm = gnubg.keyofboard(pos)
    
  return bestm


def longRace(pos) :
  xpip = 0
  opip = 0
    
  for i in range(26) :
    c = pos[i]

    if c > 0 :
      if i <= 18 :
	xpip += (25 - i) * c
      
    elif c < 0 :
      if i > 6 :
	opip += i * - c

  if xpip + opip > 20 :
    return 1

  return 0

def pipcount(pos) :
  xpip = 0
  opip = 0
  
  for i in range(26) :
    c = pos[i]

    if c > 0 :
      xpip += (25 - i) * c
    elif c < 0 :
      opip += i * - c

  return (xpip, opip)


def readData(name) :
  try :
    if 'readlines' in dir(name) :
      f = name
    else :
      f = file(name)
      
    return [l for l in f.readlines() if l[0].isupper()]
  except:
    print >> sys.stderr, "failed to read",name
    sys.exit(1)

def readPly(name) :
  try :
    return int(name)
  except:
    return eval("gnubg.p_" + name)

startPos = s2t("0 2 0 0 0 0 -5 0 -3 0 0 0 5 -5 0 0 0 3 0 5 0 0 0 0 -2 0")

