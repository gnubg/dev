#!/usr/bin/env pygnubg 

import sys, string, os.path, re, getopt, time, random

from bgutil import *
from bgutil1 import *

l2s = listToString


def loadStarts(f) :
  global startPosList

  startPosList = list()
  
  s = file(f)
  for line in s :
    if line.isspace() or line[0] == '#' : continue
    
    startPosList.append(line.split()[0])
    
verbose = 0

gnubg.set.ps(2,10,0,0.0)
gnubg.set.ps(1,12,0,0.0)

gnubg.set.shortcuts(1,10)

all1p = 0
onep = 1
cubeErrOnly = 1

maxCycle = 200
trainCycleTime = 30*60
maxNbottom = 20
minNbottom = 3

nPosCycle = 5000
targetClass = gnubg.c_contact
eqErrTh = 0.01
moveToThFactor = 2

alphaStart = 20
alphaLow = 0.1
improveFactor = 0.995
minDec = 0.5
passStart = 0
method_p1 = 0
method_1ponly = 0
do1p = 1
p1Th = 0.05
trainOnly = 0
startPosList = None
movesOnly = 0
maxNtrains = 2

moveImprovment = 0.95
playActualPly = 0
refMovesPlyDefault = 2

optlist, args = getopt.getopt(sys.argv[1:], "vf:N:1", \
  ['class=' , 'th=', 'pass=', 'm1', 'm2', 'no1p', 'nco', 'to', 'mi=', \
   "starts=", "moves", "use1p", "maxtrain="] )

for o, a in optlist:
  if o == '-f':
    moveToThFactor = float(a)
  elif o == '-v':
    verbose = 1
  elif o == '-N':
    maxCycle = int(a)
  elif o == '--class' :
    exec "targetClass = gnubg.c_" + a
  elif o == '--th':
    eqErrTh = float(a)
  elif o == '--pass':
    passStart = int(a)
  elif o == '--m1':
    method_p1 = 1
  elif o == '--m2':
    method_1ponly = 1
  elif o == '--no1p':
    do1p = 0
  elif o == '--nco':
    cubeErrOnly = 0
  elif o == '--to':
    trainOnly = 1
  elif o == '--mi':
    moveImprovment = float(a)
  elif o == '--starts':
    loadStarts(a)
  elif o == '--moves':
    movesOnly = 1
  elif o == '-1':
    all1p = 1
  elif o == '--use1p':
    playActualPly = 1
    refMovesPlyDefault = 1
  elif o == '--maxtrain':
    maxNtrains = int(a)

refNetFileName, dataFileName, benchFileName = args

scores = list()

# don't use database when trying to train a race
if targetClass == gnubg.c_race :
  gnubg.set.osdb(False)

if cubeErrOnly :
  for s in ( (7,7), (3,3), (5,5), (9,9), (25,25), (2,3), (2,4), \
             (2,5), (2,6), (2,7), (3,4), (3,5),   (3,6), (3,7), (4,5), \
             (4,6), (4,7), (5,7) ) :
    scores.append(s)
    if s[0] != s[1] :
      scores.append((s[1],s[0]))


def positionsToAdd(cycleNumber) :
  
  if trainOnly : return 0
  
  try:
    return (1000,2000,2000)[cycleNumber]
  except:
    return nPosCycle

def cubeActionDesc(info) :
  db,tk,tg = info[0:3]

  if tg :
    return "too-good/pass"
  
  return ("??","no-double/take","double/pass","double/take")[2 * db + tk]



def myProbs (pos) :
  if targetClass == gnubg.c_bearoff :
     return gnubg.probs(pos, gnubg.p_1sbear)

  if targetClass == gnubg.c_race :
    gnubg.set.osdb(True)
    p = ()
    try :
      # See if possible to eval with database 
      p = gnubg.probs(pos, gnubg.p_1srace);
    except:
      pass
    gnubg.set.osdb(True)
    if len(p) :
      return p
    
    if longRace(pos) :
      return gnubg.probs(pos, gnubg.p_osr, 1296)
   
  return gnubg.probs(pos, 2)


def addToDataFile(pos, poskey, dataFile, p2 = tuple()) :

  if len(p2) == 0 :
    p2 = myProbs(pos)
  
  print >> dataFile, poskey,formatedp(p2,"%.5lf",1)

def useIt (pos, pNet, pRef) :
  if not cubeErrOnly :
    return 1
  
  use = 0
  for s in scores :
    gnubg.set.score(s[0], s[1])

    di  = gnubg.doubleroll(pos, i = 1, p = pNet)
    rdi = gnubg.doubleroll(pos, i = 1, p = pRef)

    if rdi[0:3] != di[0:3] :
      if verbose :
	print "failed at (" + l2s(s) + ")", l2s(pos)
	print "cube new 0p ", cubeActionDesc(di),\
              " ref e2p", cubeActionDesc(rdi)
      
      use = 1
      break
    
  gnubg.set.score(0,0)
  return use


def getStartPos() :
  if startPosList :
    return gnubg.board(random.choice(startPosList))
  return startPos
    
    
def addPositions(nPosToAdd) :
  global allData  
  
  dataFile = file(dataFileName, 'a+')

  if positionsToAdd(cycleNumber) == nPosToAdd :
    print>> dataFile, "# Positions added in cycle",cycleNumber

  refMovesPly = refMovesPlyDefault
  if targetClass == gnubg.c_bearoff :
    refMovesPly = 0
    
  while nPosToAdd > 0 :

    pos = getStartPos()
    
    if verbose and pos != startPos :
      print "Starting from",l2s(pos)
      
    while 1 :
      dice = gnubg.roll()

      if dice[0] != dice[1] :
	break

    while gameOn(pos) and not stopGame(pos, targetClass) :
      d0,d1 = dice
      if d0 < d1 :
	d1,d0 = dice

      #print "looking at",pos
      
      if method_p1 :

        if gnubg.classify(pos) == targetClass :
	  poskey = gnubg.keyofboard(pos)
	
          if not poskey in allData :
	
	    gnubg.net.set(newNet)
	    pr0 = gnubg.probs(pos, 0)
	    pr1 = gnubg.probs(pos, 1)
	    
	    gnubg.net.set(refNet)

	    err =  abs(eq(pr0) - eq(pr1))

            if err >= eqErrTh :
	      p2ref = myProbs(pos)
	      dif = abs(eq(p2ref) - eq(pr0))
              
              if dif >= 0.005 and useIt(pos, pr0, p2ref) :
	      
		addToDataFile(pos, poskey, dataFile, p2ref)

		allData[poskey] = 1
                nPosToAdd += -1

                if verbose :
		  print "Added,",nPosToAdd,"to go ("\
                        ,"%.5lf %.5lf" % (err,dif),")",l2s(pos)
		

		if nPosToAdd <= 0 :
		  break
	
      elif method_1ponly :

        if gnubg.classify(pos) == targetClass :
	  gnubg.net.set(newNet)
          mvignore,m1b = gnubg.bestmove(pos, d0, d1, 1, b = 1)

	  gnubg.net.set(refNet)
	  mvignore,m0b = gnubg.bestmove(pos, d0, d1, 0, b = 1)

          if m1b != m0b :
	    m1pos = gnubg.boardfromkey(m1b)
            m0pos = gnubg.boardfromkey(m0b)
	    
            m1p = myProbs(m1pos)
            m0p = myProbs(m0pos)

            eErr = abs(eq(m1p) - eq(m0p))
            if eErr > eqErrTh :
              if verbose :
		print " err %.4lf (%d %d) %s" % (eErr,d0,d1, l2s(pos))
		print " 0p", formatedeq(m0p), l2s(m0pos)
		print " 1p", formatedeq(m1p), l2s(m1pos)
	      
              for mb in (m0pos,m1pos) :
                for id0 in range(1,7) :
                  for id1 in range(id0,7) :
		    gnubg.net.set(newNet)
		    ignore,bm = gnubg.bestmove(mb, id0, id1, 0, b = 1)

		    b = gnubg.board(bm)

		    if gnubg.classify(b) != targetClass or bm in allData :
		      continue
		    
		    p0 = gnubg.probs(b, 0)

		    gnubg.net.set(refNet)
		    p2ref = myProbs(b)

		    err = abs(eq(p0) - eq(p2ref))
                    
                    if err >= eqErrTh * 72 :
		      addToDataFile(b, bm, dataFile, p2ref)
		      allData[bm] = 1
                      nPosToAdd += -1

                      if verbose :
			print "Added 1 (err",err,") {",l2s(b), "}",\
                              nPosToAdd, " to go"

              dataFile.flush()
      else :
        useCube = 0
        useMoves = 0

        if gnubg.classify(pos) == targetClass :

          m = gnubg.moves(pos, d0, d1)
	
          if len(m) > 1 :
            # Move choices
            # At least one option has same class as target

            for mv in m :
              if gnubg.classify(mv) == targetClass :
                useMoves = 1
                break
	

          poskey = gnubg.keyofboard(pos)

          if gnubg.classify(pos) == targetClass and pos != startPos and \
             not movesOnly :
            useCube = 1

        if useMoves or useCube :
          if verbose :
            sys.stdout.flush()
            print " testing (" + l2s(dice) + ")",l2s(pos)
	

          if useCube :
            addIt = 0

            if cubeErrOnly :
              gnubg.net.set(newNet)
              pr0 = gnubg.probs(pos,0)
	    
              gnubg.net.set(refNet)
              p2ref = myProbs(pos)

              dif = abs(eq(p2ref) - eq(pr0))

              if dif >= 0.005 and useIt(pos, pr0, p2ref) :
                addIt = 1
	    
            else :
              gnubg.net.set(newNet)
              gnubg.set.score(7,7)
              diNew0p = gnubg.doubleroll(pos,n = 0, i = 1)
              p0n = gnubg.probs(pos, 0)
	    
              gnubg.net.set(refNet)
              p2ref = gnubg.probs(pos, 2)
              diRef2p = gnubg.doubleroll(pos, n = 0, i = 1, p = p2ref)

              gnubg.set.score(0,0)

              if diNew0p[0:3] != diRef2p[0:3] and \
                 abs(eq(p2ref) - eq(p0net)) >= 0.002 :
                addIt = 1
	  
	    if addIt and not poskey in allData :

              addToDataFile(pos, poskey, dataFile, p2ref)

              allData[poskey] = 1
              nPosToAdd += -1

              if verbose :
                if not cubeErrOnly :
		  print "cube new 0p ", cubeActionDesc(diNew0p), \
                        "ref e2p", cubeActionDesc(diRef2p), \
                        ", added position,",nPosToAdd, "to go"
                else :
                  print "1 added position,", nPosToAdd, "to go"

	  if useMoves :
      
            gnubg.net.set(newNet)
            ignore,m0b = gnubg.bestmove(pos, d0, d1, 0, b = 1)

            gnubg.net.set(refNet)
            ignore,m2b = gnubg.bestmove(pos, d0, d1, refMovesPly, b = 1)

            if m0b != m2b :
              m0p = gnubg.boardfromkey(m0b)
              pm0 = gnubg.probs(m0p, refMovesPly)

              m2p = gnubg.boardfromkey(m2b)
              pm2 = gnubg.probs(m2p, refMovesPly)
	      
              e = abs(eq(pm0) - eq(pm2))

              if verbose :
                print " 0ply error", "%.6lf" % e
	    
	    
              if e >= eqErrTh :
                added = 0
                if gnubg.classify(m0p) == targetClass and not m0b in allData :

                  addToDataFile(gnubg.board(m0b), m0b, dataFile, pm0)
		
                  allData[m0b] = 1
                  nPosToAdd += -1
                  added += 1
	      
                if gnubg.classify(m2p) == targetClass and not m2b in allData :
                  addToDataFile(gnubg.board(m2b), m2b, dataFile, pm2)

                  allData[m2b] = 1
                  nPosToAdd += -1
                  added += 1

                  if verbose :
                    print " added",added, "positions,",nPosToAdd,"to go"
	      
	    
	    else :
              if verbose :
                print ", OK"

	    if do1p :
              gnubg.net.set(newNet)
              ignore,m1b = gnubg.bestmove(pos, d0, d1, 1, b = 1)

              if m1b != m2b :
                m1pos = gnubg.boardfromkey(m1b)
                m2pos = gnubg.boardfromkey(m2b)
	    
                m1p = myProbs(m1pos)
                m2p = myProbs(m2pos)

                if abs(eq(m1p) - eq(m2p)) > eqErrTh :
                  if verbose :
                    print "(" + l2s((d0,d1)) + ")", l2s(pos)
                    print " 1p", formatedeq(m1p), l2s(m1pos)
                    print " 2p", formatedeq(m2p), l2s(m2pos)

                  for mb in (m2pos, m1pos) :
                    for id0 in range(1,7) :
                      for id1 in range(id0, 7) :
                        gnubg.net.set(newNet)
                        ignore,bm = gnubg.bestmove(mb, id0, id1, 0, b = 1)

                        b = gnubg.board(bm)

                        if gnubg.classify(bm) != targetClass or bm in allData :
                          continue
		      
		        p0 = gnubg.probs(b, 0)

                        gnubg.net.set(refNet)
                        p2ref = myProbs(b)

                        err = abs(eq(p0) - eq(p2ref))

                        #if verbose :
                        #  print "(%d %d) %.5lf" % (id0,id1,err)
                        
                        if err >= eqErrTh * 72 :
                          addToDataFile(b, bm, dataFile, p2ref)
                          allData[bm] = 1
                          nPosToAdd += -1

                          if verbose :
                            print "err",err,"{" + l2s(b) + "} added,", \
                                  nPosToAdd, "to go"
              gnubg.net.set(refNet)

	  dataFile.flush()
	
          if nPosToAdd <= 0 :
	    break
      
      if onep :
	gnubg.set.score(1, 1)

      plyToUse = playActualPly
      if plyToUse > 0 and gnubg.classify(pos) != targetClass :
        plyToUse = 0

      ignore,b0 = gnubg.bestmove(pos, d0, d1, plyToUse, b = 1)
      
      if onep :
	gnubg.set.score(0, 0)
      
      pos = gnubg.boardfromkey(b0)
      dice = gnubg.roll()

  dataFile.close()


def train() :
  global newNet

  if verbose :
    print "training for cycle", cycleNumber
    print "reading training data file"
  
  data = readData(dataFileName)

  nPos = len(data)

  trainer = gnubg.trainer(data, targetClass == gnubg.c_race)
  
  del data

  ncy = 0
  alpha = alphaStart

  prever = 100
  preveqerrmax = 100

  order = randPer(nPos)

  saveFile = errFile(newNetFileNameBase, cycleNumber)
  
  gnubg.net.set(newNet)

  # bg errors are from imperfect evaluation not from net
  terrors = trainer.errors()
  if targetClass == gnubg.c_race :
    besterr,bestmax = terrors[4:6]
  else :
    besterr,bestmax = terrors[0:2]
  
  nBottom = 0
  timeStarted = time.time()
  
  while nBottom < maxNbottom :

    # checkForUserInput
    
    if verbose : print "\ncycle", ncy, ": checking"
    cl = time.time()
  
    terrors = trainer.errors()
    if targetClass == gnubg.c_race :
      er,eqerrmax = terrors[4:6]
    else :
      er,eqerrmax = terrors[0:2]
  
    if verbose :
      t = max(time.time() - cl, 0.00001)
      print "eqerr %.5lf, Max %.5lf (%s %d)" % \
            (er, eqerrmax, ftime(t), int(nPos / t))
    

    if er <= besterr :
      if verbose :
	print "** saving net as best (",er,"<=", besterr," ", \
              eqerrmax, (">=","<")[eqerrmax < bestmax],bestmax,")"
      
      besterr = er
      bestmax = eqerrmax
      
      gnubg.net.save(saveFile)

    if alphaStart == alpha and nBottom >= minNbottom and \
       (time.time() - timeStarted) >= trainCycleTime :
      break
    

    if not ((er < improveFactor * prever) or \
	(eqerrmax < improveFactor * preveqerrmax)) :
      decby = max(alpha * 0.1, minDec)
      
      alpha = max(alpha - decby, alphaLow)

      if er > prever :
        if verbose : print "** new order"
	order = randPer(nPos)

    prever = er
    preveqerrmax = eqerrmax
    
    ncy += 1
    if verbose :
      print "cycle",ncy,": training ", "(" + ("%.3lf" % alpha) + ")"
      cstart = time.time()

    trainer.train(alpha, order)
    
    if verbose :
      nsec = time.time() - cstart
      print nPos,"positions in", ftime(nsec)
    
    if alpha <= alphaLow :
      alpha = alphaStart
      nBottom += 1

      if verbose :
	print "  Hit bottom",nBottom,"after",ftime(time.time() - timeStarted)

    if verbose : sys.stdout.flush()

  del newNet
  newNet = gnubg.net.get(saveFile)

  del trainer 

execfile(os.path.dirname(sys.argv[0]) + '/referr.py')

# error of newNet on ref data
def checkError(net) :
  gnubg.net.set(net)
  return benchmarkError(benchFileName)



cycleNumber = 0
nPosToAdd = positionsToAdd(cycleNumber)
allData = dict()

# code to read dataFile and establish cycleNumber and nPosToAdd

if os.path.exists(dataFileName) :
  refFile = file(dataFileName)
  for line in refFile :
    if line.isspace() : continue

    m = re.match('# Positions added in cycle ([0-9]+)', line)
    if m :
      cycleNumber = int(m.group(1))
      nPosToAdd = positionsToAdd(cycleNumber)
      continue

    if line[0] == "#" : continue

    allData[line.split()[0]] = 1
    
    nPosToAdd += -1
  
  refFile.close()

  if verbose :
    print "Continue cycle",cycleNumber,",",nPosToAdd, "positions to go"
  

def errFile(f, n) :
  return f  + '.' + ("%.2d" % n)

print "loading nets"

try :
  refNet = gnubg.net.get(refNetFileName)
except :
  print >> sys.stderr, "failed to load", refNetFileName
  sys.exit(1)

newNetFileNameBase = 'gnubg.weights.best'
newNetFileName = errFile(newNetFileNameBase, cycleNumber)
try:
  newNet = gnubg.net.get(newNetFileName)
except :
  print >> sys.stderr, "failed to load", newNetFileName
  sys.exit(1)

def errFileName (netName, dataName) :
  return 'error-' + os.path.basename(netName) +\
         '-' + os.path.basename(dataName)

def write_file(name, data) :
  p = file(name, 'w')
  p.writelines(data)
  p.close()

def read_file(name) :
  p = file(name)
  data = p.readlines()
  p.close()
  return data

def readError(name) :
  return [float(x) for x in string.split(read_file(name)[0])]

refErrName = errFileName(refNetFileName, dataFileName)

if not os.path.exists(refErrName) :
  if verbose :
    print "Calculating error for reference net"
  
  refErr = checkError(refNet)
  
  write_file(refErrName, l2s(refErr))
else :
  refErr = readError(refErrName)


if verbose :
  print "Reference error",refNetFileName,"-",l2s(refErr)

  
netErrName = errFileName(newNetFileName,dataFileName)

if not os.path.exists(netErrName) :
  if verbose :
    print "Calculating error for",newNetFileName
  
  netErr = checkError(newNet)
  write_file(netErrName, l2s(netErr))
else :
  netErr = readError(netErrName)

print "Net error",netErrName,"-",l2s(netErr)

moveErr,dErr,tErr = netErr 

if not method_p1 :
  eqErrTh = moveToThFactor * moveErr
if eqErrTh == 0 :
  eqErrTh = 0.01

if verbose :
  print "Error Th set to", "%.5lf" % eqErrTh

while cycleNumber <= maxCycle :
  if verbose :
    if nPosToAdd == positionsToAdd(cycleNumber) :
      print "Start cycle",cycleNumber

  if nPosToAdd > 0 :
    if verbose :
      print "seraching for",nPosToAdd,"positions to add"
      
    onep = (cycleNumber & 0x1) or all1p
    if verbose and onep :
      print "Playing at DMP"
    
    addPositions(nPosToAdd)
  
  cycleNumber += 1

  nTrain = maxNtrains
  for k in range(passStart,nTrain) :

    saveFile = errFile(newNetFileNameBase, cycleNumber)

    # when stopped in middle of training
    if os.path.exists(saveFile) :
      if verbose :
	print "loading", saveFile

      # del newNet
      newNet = gnubg.net.get(saveFile)
    
    train()

    if verbose :
      print "Check error for trained net, pass",k,",",
    

    el = checkError(newNet)
    nmoveErr = el[0]

    if nmoveErr >= moveImprovment * moveErr :
      if verbose :
	print "Move error is",nmoveErr,"(prev",moveErr,")"
	print "cube error",el[1],el[2]

      if k + 1 < nTrain :
	continue

    write_file(errFileName(saveFile, dataFileName), l2s(el))
    
    moveErr,dErr,tErr = el 

    if not method_p1 :
      eqErrTh = moveToThFactor * moveErr
    if eqErrTh == 0 :
      eqErrTh = 0.01
    
    if verbose :
      print "Error Th set to", "%.5lf" % eqErrTh
      print "Move Error %.6lf, Cube %.6lf %.6lf" % (moveErr,dErr,tErr)

    break

  passStart = 0
  
  nPosToAdd = positionsToAdd(cycleNumber)

