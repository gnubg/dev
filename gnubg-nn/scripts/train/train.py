#!/usr/bin/env pygnubg 
""" train [-a alpha -l low-alpha -b benchnark -v -n] dat-file net-base-name"""

import sys, string, os, time, glob, getopt

from bgutil import *
from referr import benchmarkError


def checkForUserInput() :
  if anyInput() :
    what = sys.stdin.readline().strip()
    try :
      exec what in globals()
    except SystemExit:
      sys.exit()
    except :
      print >> sys.stderr, "failed to eval '",what,"'"

alphaStart = 20
alphaLow = 0.1
improveFactor = 0.995
minDec = 0.5
doDec = 1
nStartAlpha = 0
keepOrder = 0
nt = 1
verbose = 0
targetClass = gnubg.c_contact
benchmarkFile = None
iTrain = list()
ignoreBG = 0

optlist, args = getopt.getopt(sys.argv[1:], "a:l:nvb:i:", \
                              ["class=", "ignorebg"])

for o, a in optlist:
  if o == '-a':
    alphaStart = float(a)
  elif o == '-v':
    verbose = 1
  elif o == '-l':
    alphaLow = float(a)
  elif o == '-n' :
    doDec = 0
  elif o == '-b' :
    benchmarkFile = a
  elif o == '--class':
    exec "targetClass = gnubg.c_" + a
  elif o == '--ignorebg' :
    ignoreBG = 1
  elif o == '-i':
    iTrain = [int(x) for x in a.split()]
    


try: 
  dataFileName,baseName = args[0:2]
except :
  print >> sys.stderr, "Usage:", sys.argv[0],"dat-file net-base-name"
  sys.exit(1)
  
if verbose:
  print "reading training data file"
  
data = readData(dataFileName)

nPos = len(data)

# Should training ignore backgammons?
ignoreBGs = ignoreBG or targetClass == gnubg.c_race

# Third argument is true if you want to train the small pruning nets.
# iTrain is a list of input positions to train. Part of my experiments (JH)
#
trainer = gnubg.trainer(data, ignoreBGs, 0, iTrain)
  
del data

alpha = alphaStart

prever = 100
preveqerrmax = 100
startAlphaCounter = nStartAlpha

order = randPer(nPos)

ncy = 0

checkBenchmark = 0

while 1 :
  checkForUserInput()
    
  if verbose : print "\ncycle", ncy, ": checking"

  cl = time.time()
  
  # bg errors are from imperfect evaluation not from net
  terrors = trainer.errors()
  if ignoreBGs :
    er,eqerrmax = terrors[4:6]
  else :
    er,eqerrmax = terrors[0:2]
  
  if verbose :
    t = max(time.time() - cl, 0.00001)
    print "eqerr %.5lf, Max %.5lf (%s %d)" % \
            (er, eqerrmax, ftime(t), int(nPos / t))

  e = int(0.5 + 10000 * er)
  m = int(0.5 + 1000 * eqerrmax)

  doSave = 1

  others = glob.glob(baseName + ".save.*-*")
  for o in others :
    e1,m1 = [int(x) for x in o.split('.')[-1].split('-')]
    
    if e < e1 and m < m1 :
      if verbose :
        print "removing", o
      os.remove(o)
      continue
	
    if e1 < e and m1 < m :
      doSave = 0
      break

  if doSave :
    others = glob.glob(baseName + ".save." + str(e) + "-*")

    for o in others :
      m1 = int(o.split('-')[-1])

      if m1 >= m :
        if verbose :
          print "removing", o
	os.remove(o)
	continue

      if m1 < m :
	doSave = 0
	break

    others = glob.glob(baseName + ".save.*-" + str(m))
    for o in others :
      e1 = int(o.split('.')[-1].split('-')[0])
      # print "e1",e1
      
      if e1 >= e :
        if verbose :
          print "removing", o
	os.remove(o)
	continue

      if e1 < e :
	doSave = 0
	break
    
  if doSave :
    saveFile = baseName + ".save." + str(e) + '-' + str(m)
    if verbose :
      print "creating", saveFile
    
    gnubg.net.save(saveFile)
    
  if checkBenchmark and benchmarkFile :
    if verbose : print "computing benchmark error"

    berr = benchmarkError(benchmarkFile)

    for o in glob.glob(baseName + "b-*-*") :
      x1,x2 = o.split('-')[1:]
      if berr[0] <= float(x1) and berr[1] <= float(x2) :
        if verbose : print "removing", o
	os.remove(o)

    name = baseName + ("b-%.7lf-%.7lf" % (berr[0],berr[1]))

    if verbose : print "saving as",name

    gnubg.net.save(name)
    
    checkBenchmark = 0

  if not ((er < improveFactor * prever) or \
          (eqerrmax < (1 - 2 * (1 - improveFactor)) * preveqerrmax)) :
    if doDec and startAlphaCounter == 0 :
      alpha = min(alpha * 0.9, alpha - minDec)
      if alpha < alphaLow :
	alpha = alphaLow - 0.00001

    if not keepOrder and er > prever :
      if verbose :
        print "** new order"
      order = randPer(nPos)
      orderTotalGain = 0.0
    
  prever = er
  preveqerrmax = eqerrmax

  ncy += 1

  for nTrain in range(nt) :
    if verbose :
      if nt > 1 :
        print "cycle %d.%d: training (%.3lf)" % (ncy,nTrain,alpha),
      else :
        print "cycle %d: training (%.3lf)" % (ncy,alpha),

      cstart = time.time()
    
    trainer.train(alpha, order)
    
    if verbose :
      nsec = time.time() - cstart
      print nPos,"positions in", ftime(nsec)
      sys.stdout.flush()
    
    if alpha < alphaLow :
      alpha = alphaStart / 0.9
      startAlphaCounter = nStartAlpha
      checkBenchmark = 1

del trainer
