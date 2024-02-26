import gnubg 
from bgutil import swapSides,eq

def swapSidesId(id) :
  return gnubg.keyofboard(swapSides(gnubg.boardfromkey(id)))

def sign(x) :
  if x < 0 : return -1
  elif x > 0 : return 1
  else : return 0
  
def mcmp__(m1,m2) :
  return sign(m1[1] - m2[1])

def pruneBestMoves(pos, d1, d2, nMoves) :
  moves = gnubg.moves(pos, d1, d2)

  if len(moves) <= nMoves :
    return [swapSidesId(m) for m in moves]
  
  all = list()

  for m in gnubg.moves(pos, d1, d2) :
    pos = swapSides(gnubg.boardfromkey(m))
    
    pm = gnubg.probs(pos, gnubg.p_prune)
    e = eq(pm)
    all.append((gnubg.keyofboard(pos),e))

  all.sort(mcmp__)

  return [x[0] for x in all[0:nMoves]]

def stopGame(pos, targetClass) :
  """when looking for positions of type targetClass, can we stop at 'pos'"""
  
  t = gnubg.classify(pos)

  if targetClass == gnubg.c_contact  :
    return  t != gnubg.c_contact
  
  if targetClass == gnubg.c_crashed :
    return not (t == gnubg.c_contact or t == gnubg.c_crashed)

  try :
    if targetClass == gnubg.c_backcontain :
      return not (t == gnubg.c_contact or t == gnubg.c_crashed or \
                  t == gnubg.c_backcontain)
  except:
    pass
  
  if targetClass == gnubg.c_race :
    return not (t == gnubg.c_contact or t == gnubg.c_crashed \
                or t == gnubg.c_race)
  
  return 0

import operator

def OIsCrashed(pos) :
  N = 6
  tot = reduce(operator.add, [x for x in pos if x > 0], 0)
  if tot <= N:
    return 1
  if pos[-2] > 0 :
    p1 = pos[-2]
  else :
    p1 = 0
  if pos[-3] > 0 :
    p2 = pos[-3]
  else :
    p2 = 0
  if p1 > 1 :
    if (tot - p1) <= N :
      return 1
    if p2 > 1 and (1 + tot - (p1 + p2)) <= N :
      return 1
  else :
    if (tot - (p2 - 1)) <= N :
      return 1
  return 0
