#!/usr/bin/env pygnubg 
"""synopsis: show moves error from a sagnubg output.
usage:  perr [-v level] [-np list=0] [-nc] [-nosr N=1296] [-list]
[-nc] [-nm] [-ini] [-is file-name] [-lo file] [-c cat-file] benchmark-file"""

# --cache : log file name (--log) from previous run (only ply 1 1/2 now)
# --log   : log ply > 0 evaluations

import sys, string, getopt, array, os.path, operator
from bgutil1 import OIsCrashed,sign
from bgutil import *

def me(i,l) :
  db,tk = l[0:2]

  if not db :
    return i[0]
  if tk :
    return i[1]

  return i[2]

def inter(x, w) :
  c0,v0,c1,v1 = w

  a = (x - c0) / (c1 - c0)

  return a*v1 + (1-a)*v0


def load(name) :
  global catg, cerr

  f = file(name)

  for l in f :

    if l[0] == '#' or l.isspace() :
      continue

    k,c = l.split()

    catg[k] = c
    cerr[c] = [0.0,0]

  f.close()

def categoryFromLoadedFile(p) :
  return catg[p]

# per Albert Silver
def holdingCategory(pb) :
  p = gnubg.board(pb)
  if p[3] > 1 or p[4] > 1 or p[5] > 1 or p[6] > 1  or p[7] > 1 \
     or p[22] < -1 or p[21] < -1 or p[20] < -1 or p[19] < -1 or p[18] < -1 :
    return "holding"

  return "nonholding"

def closedCategory(pb) :
  p = gnubg.board(pb)

  if p[0] == 0 and p[-1] == 0 :
    return "nonclosed"

  if p[0] > 0 :
    c = 1
    for i in range(1,7) :
      if p[i] >= -1 :
        c = 0
        break
    if c:
      return "closed"
  
  if p[-1] < 0 :
    for i in range(25-6,25) :
      if p[i] <= 1 :
        return "nonclosed"
  
    return "closed"

  return "nonclosed"
  
def backcCategory(pb) :
  m = 8
  n = reduce(operator.add, [x for x in pos[0:13] if x > 0], 0)
  if n >= m :
    return 'backc'
  n = -reduce(operator.add, [x for x in pos[13:26] if x < 0], 0)
  if n >= m :
    return 'backc'
  
  return 'nonbackc'

def backcoCategory(pb) :
  if gnubg.classify(pb) == gnubg.c_backcontain :
    return 'backco'
  return 'nonbackco'

  
def forwCategory(pb) :
  pos = gnubg.board(pb)
  if not OIsCrashed(pos) :
    pos = swapSides(pos)

  totXout = -reduce(operator.add, [x for x in pos[13:] if x < 0], 0)

  if totXout > 4:
    return 'forw'
  return 'nonforw'


def srt1(x,y) :
  e1 = eq(x[1])
  e2 = eq(y[1])
  return sign(e1 - e2)

def bestmove15(posIn, d0, d1) :
  mv, moveb, p0list = gnubg.bestmove(posIn, d0, d1, 0, b = 1, list = 1)

  sp0list = list(p0list)
  sp0list.sort(srt1)

  p15list = list()
  w = 0.5

  c = dict()
  try :
    while 1 :
      l = p1evalsfile.next()
      if l[0:4] == "#E 1" :
        l = l.split()
        c[l[2]] = l[3:]
      elif l[0] == '#' or l.isspace() :
        pass
      else :
        break
  except :
    pass
  
  for pos,pr in sp0list[0:8] :
    if pos in c :
      p1 = [float(x) for x in c[pos]]
    else :
      p1 = gnubg.probs(pos, 1)
      print "#",gnubg.keyofboard(posIn), d0, d1
      print "#E 1 ", pos, formatedp(p1, '%.5lf', 1)
      
    ep1 = eq(p1)
    ep0 = eq(pr)

    p15list.append( (pos, ep1*w15 + ep0 * (1-w15)) )

  e = 10
  for pos,epos in p15list :
    if epos < e :
      e = epos
      p15b = pos

  return p15b
  

l2s = listToString

verbose = 0
doCube = 1
np = (0,)
nOsr = 1296
osrPly0Filter = 20
listAll = 0
removeNonInteresting = 1
nNonInteresting = 0
cubeAway = 7
doMoves = 1
showErrorPerScore = 0
proportionalCubeError = 1
interestingFileName = ""
progressReport = 1
recordEvaluations = 0
doLogOut = 0
w15 = 0.5
fCategory = None


gnubg.set.ps(2, 4, 2, 0.1)
gnubg.set.ps(3, 4, 2, 0.1)
             
optlist, args = getopt.getopt(sys.argv[1:], "v:sc:", \
 [ 'pr=', 'np=', 'list', 'nc', 'nm', 'cube=', 'ini', \
   'is=', 'nosr=', 'lo=', 'per-score', 'cache=', "log=", "wp15=" ] )

for o, a in optlist:
  if o == '-v':
    verbose = int(a)
  elif o == '-s' :
    print "# using shortcuts (10)"
    gnubg.set.shortcuts(1,10)
  elif o == '-c' :
    if os.path.exists(a) :
      load(a)
    else :
      cerr = dict()
      for t in (a, 'non'+a) :
        cerr[t] = [0.0,0]
      exec('fCategory = ' + a + 'Category')
  elif o == '--pr' :
    progressReport = a
  elif o == '--np' :
    np = [int(x) for x in string.split(a.replace("osr","gnubg.p_osr"))]
  elif o == '--nc' :
    doCube = 0
  elif o == '--nm' :
    doMoves = 0
  elif o == '--cube' :
    cubeAway = int(a)
  elif o == '--list' :
    listAll = 1
    recordEvaluations = 1
  elif o == '--ini' :
    removeNonInteresting = 0
  elif o == '--is' :
    interestingFileName = a
  elif o == '--nosr' :
    nOsr = int(a)
  elif o == '--lo' :
    pass
  elif o == '--per-score' :
    pass
  elif o == '--cache' :
    p1evalsfile = file(a)
  elif o == '--log' :
    logOut = file(a, 'w')
    doLogOut = 1
  elif o == '--wp15' :
    w15 = float(a)
    assert 0 <= w15 <= 1.0



if listAll :
    print "# plys",l2s(np)

tot = 0

nOut = dict()
nErr = dict()
toterr = dict()
for ply in np :
  nOut[ply] = 0
  nErr[ply] = 0
  toterr[ply] = 0.0

ver = ""
nCubePos = 0

if removeNonInteresting :
  cubeErrorEMeasure = array.array('d', [0.0,]*2)
  cubeErrorEerror = array.array('d', [0.0,]*2)
  inCubePos = array.array('i', [0,]*2)
  inCubeError = array.array('i', [0,]*2)

  allr = dict()

  if interestingFileName != "":
    interestingFiles = [file(interestingFileName+'-no',"w"), \
                        file(interestingFileName+'-yes',"w")]
else :
  cubeErrorEMeasure = 0.0
  cubeErrorEerror = 0.0
  
nCubeErrs = 0
scores = list()

for s in ( (7,7), (3,3), (5,5), (9,9), (25,25), (2,3), (2,4), \
           (2,5), (2,6), (2,7), (3,4), (3,5),   (3,6), (3,7), (4,5), \
           (4,6), (4,7), (5,7) ) :
  scores.append(s)
  if s[0] != s[1] :
    scores.append((s[1],s[0]))

if showErrorPerScore :
  nErrPerScore = dict()
  evalError = dict()

  for s in scores :
    nErrPerScore[s] = 0
    evalError[s] = 0.0

gnubg.set.equities('mec26')

if 'catg' in globals() :
  fCategory = categoryFromLoadedFile

if len(args) :
  movesFile = file(args[0])
else :
  movesFile = sys.stdin


if progressReport :
  import time
  startTime = time.time()
  checkPoint = 1

for line in movesFile :
  if progressReport :
    now = time.time()
    if now - startTime > 60*checkPoint :
      if nCubePos :
        print >> sys.stderr, nCubePos,"Cube ",
      if tot :
        print >> sys.stderr, tot,"Moves",
      print >> sys.stderr, " in",ftime(now - startTime)
      
      checkPoint += 1

  if line[0] == '#' :
    if removeNonInteresting and doMoves and line[1:3] == "R " :
      l = string.split(line)
      try :
        allr[l[1]] = l[2:]
      except:
        print "error", l
        os.exit(1)
        
    continue

  if line.isspace() :
    continue

  if line[0] == 's' :
    i = line.find("version ")
    if i >= 0 :
      ver = line[i + len("version "):].split()[0]
    continue

  if doCube and line[0] == 'o' :
    nCubePos += 1

    l = line.split();

    opr,pb = l[0:2]
    proll = [float(x) for x in l[2:7]]

    pos = gnubg.boardfromkey(pb)

    if removeNonInteresting :
      interesting = (eq(proll) % 1) != 0
      inCubePos[interesting] += 1

      if interestingFileName != "" :
        print >> interestingFiles[interesting], line,

    eProbs = gnubg.probs(pos, np[0], nOsr)

    hasErr = 0
    for s in scores :
      gnubg.set.score(s[0], s[1])

      di = gnubg.doubleroll(pos, i = 1, p = eProbs)

      rdi = gnubg.doubleroll(pos, i = 1, p = proll)

      if rdi[0:3] == di[0:3] :
        if verbose > 1 :
          print "OK",pb,"(" + l2s(s) + ")"
      else :
        hasErr = 1

        err = abs(eq(proll) - eq(eProbs))

        if proportionalCubeError :
          aNet = 0   # the erroneous one
          aRoll = 1  # the correct one

          if verbose > 1 :
            print "di =", l2s(di), "rdi =", l2s(rdi)
            print "roll=",formatedp(proll, "%9.6lf"),\
                  "prob=",formatedp(eProbs, "%9.6lf")

          for it in range(3) :
            a = (aNet + aRoll) / 2.0

            newp = map(lambda x,y : x*(1-a) + a*y, eProbs, proll)

            ndi = gnubg.doubleroll(pos, i = 1, p = newp)

            if verbose > 1 :
              print "a =", a, "newp =", \
                    "(" + formatedp(newp, "%9.6lf") + ")","ndi =",ndi
            if rdi[0:3] == ndi[0:3] :
              aRoll = a
            else :
              aNet = a

          err *= aRoll

        eNet =  me(rdi[3:7], di)
        eRoll = me(rdi[3:7], rdi)
        eqerr = abs(eRoll - eNet)

        if verbose :
          print "abs(",eRoll,"-",eNet,") eqerr =",eqerr

        if removeNonInteresting :
          cubeErrorEMeasure[interesting] += err
          cubeErrorEerror[interesting] += eqerr
        else :
          cubeErrorEMeasure += err
          cubeErrorEerror += eqerr

        if removeNonInteresting and not interesting :
          if eqerr != 0 :
            print "+++ non interesting", line

        if verbose :
          if removeNonInteresting :
            i = ('N','I')[interesting]
          else :
            i = ""

          print l2s(s),i,"err",err,"(" + l2s(pos) + ")"
          print "proll =",formatedp(proll),"ep =",formatedp(eProbs)

        if showErrorPerScore :
          nErrPerScore[s] += 1
          evalError[s] += err


    nCubeErrs += hasErr
    if removeNonInteresting :
      inCubeError[interesting] += hasErr

    gnubg.set.score(0,0)

    continue

  if line[0] == 'r' and  doLogOut :
    lastRline = line

  if not doMoves or line[0] != 'm' :
    continue

  l = line.split()

  opr,pb,d0,d1 = l[0:4]
  moves = l[4:]
  pos = gnubg.boardfromkey(pb)

  if removeNonInteresting :
    interesting = 0

    if not moves[0] in allr :
      print "***",moves[0],"not in"
      print "*** line", line
      interesting = 1
    else :
      mr = allr[moves[0]]

      for i in range(len(moves)/2 - 1) :
        if not (moves[2*i+2] in allr) :
          print "***",moves[2*i+2],"not in"
          print "*** line", line
          interesting = 1
          break

        if mr != allr[moves[2*i+2]] :
          interesting = 1
          break

    allr = dict()
    nNonInteresting += 1 - interesting

    if interestingFileName != "" :
      print >> interestingFiles[interesting], line,


    if not interesting :
      continue

  if listAll :
    lineOut = [pb,d0+d1]
    anyOut = 0

  tot += 1

  d0 = int(d0)
  d1 = int(d1)
  for ply in np :
    if ply == gnubg.p_osr :
      moveb = bestMoveOSR(pos, d0, d1, nOsr, osrPly0Filter)
    elif ply == 15 :
      moveb = bestmove15(pos, d0, d1)
    else :
      all = gnubg.bestmove(pos, d0, d1, ply, b = 1, list = recordEvaluations)
      moveb = all[1]

      if recordEvaluations :
        for e in all[2] :
          print "#E", ply, e[0], formatedp(e[1], '%.5lf', 1)
        
    if moveb == moves[0] :
      loss = 0.0
    else :
      i = find(moveb, moves)

      if i >= 0 :
        loss = float(moves[i+1])
      else :
        loss = -1

    if loss == -1 :
      if doLogOut :
        if len(np) > 1 :
          print >> logOut, "# ply",ply

        print >> logOut, lastRline,
        print >> logOut, 'o', moveb

      nOut[ply] += 1
      isOut = 1

      if listAll :
        anyOut = 1

      loss = float(moves[-1])
    else :
      isOut = 0


    if listAll :
      lineOut.append(loss)

    if loss < 0.0 :
      print >> sys.stderr, line
    assert loss >= 0.0
    
    toterr[ply] += loss

    if fCategory :
      c = fCategory(pb)
      e,t = cerr[c]
      e += loss
      t += 1
      cerr[c] = [e, t]


    if loss != 0.0 :
      nErr[ply] += 1

    if verbose :
      if loss != 0.0 :

        print pb,d0,d1,"loss =",'%.5lf' % loss, ("","*")[isOut]
        if verbose > 1 :
          if fCategory :
            print  " ", fCategory(pb),"-",
          print ply," ply move is",moveb


        if ply != 0  :
          print "--",tot,"done"
      elif verbose > 1 :
        print pb,d0,d1,"OK"

      if verbose > 1 :
        print "error", nErr[ply],"of",tot,"(nOut=",nOut[ply],")",
        print "%.2lf%% %.5lf" % \
              ((100.0 * nErr[ply]) / tot, toterr[ply] / tot)

  if listAll :
    print l2s(lineOut),
    
    if anyOut :
      print " +++",

    print


movesFile.close()

if removeNonInteresting and nNonInteresting :
  print nNonInteresting,"Non interesting,",tot,"considered for moves."

if tot > 0 :
  for ply in np :
    print str(ply) + "p errors",nErr[ply],"of",tot,"avg",toterr[ply]/tot
    print "n-out (", nOut[ply], ")" , "%.2f%%" % ((100.0 * nOut[ply])/tot)



if nCubePos > 0 :
  print nCubeErrs,"errors of",nCubePos

  if removeNonInteresting :
    if inCubePos[1] :
      ctot = inCubePos[1] * len(scores)
      print "cube errors interesting",inCubeError[1],"of",inCubePos[1]
      print "  me", cubeErrorEMeasure[1]/ctot,\
            "eq",cubeErrorEerror[1]/ctot

    if inCubePos[0] :
      ctot = inCubePos[0] * len(scores)
      print "cube errors non interesting",\
            inCubeError[0],"of",inCubePos[0]
      print "  me", cubeErrorEMeasure[0]/ctot,\
            "eq",cubeErrorEerror[0]/ctot


  else :
    ctot = nCubePos * len(scores)
    print "cube err",nCubePos,"me",cubeErrorEMeasure/ctot,\
          "eq",cubeErrorEerror/ctot

    if showErrorPerScore :
      for s in scores :
        ne = nErrPerScore[s]
        print "Score",s,":",ne,\
              "(",'%.1lf' % 100.0*ne / nCubePos,") errors"
        print "  average error",evalError[s] / nCubePos




if fCategory :
  print
  for c in cerr :
    e,t = cerr[c]
    if t != 0 :
      print "%10s (%d %f%%) %lf" % (c, t, (t*100.0)/tot, e/t)

if doLogOut :
  logOut.close()
