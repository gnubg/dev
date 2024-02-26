#!/usr/bin/env pygnubg 
"""getth [-np PLY] [-th VAL] [-1] dat-file
Find positions whose evaluation at ply PLY is different by more than TH from
values in dat-file"""

import sys, getopt

from bgutil import *

th = 0.2
verbose = 0
np = 0
do01Dif = 0

optlist, args = getopt.getopt(sys.argv[1:], "v:1", [ 'np=', 'th=' ] )

for o, a in optlist:
  if o == '-v':
    verbose = int(a)
  elif o == '--np' :
    np = int(a)
  elif o == '--th':
    th = float(a)
  elif o == '-1':
    do01Dif = 1

if len(args) != 1 :
  print >> sys.stderr, "Usage:",sys.argv[0]," [flags] dat-file"
  sys.exit(1)
  
f = file(args[0])
for line in f :
  
  if line[0] == '#' or line.isspace() :
    continue

  l = line.split()

  pos = gnubg.boardfromkey(l[0])
  probs = [float(p) for p in l[1:]]

  if len(probs) != 5 :
    print line
    print probs
    
  if do01Dif :
    p0 = gnubg.probs(pos, 0)
    p1 = gnubg.probs(pos, 1)
    err = abs(eq(p0) - eq(p1))
    if err >= th :
      print "# err %.5lf, pos = {%s}" % (err, listToString(pos))
      print "# probs(0) =", formatedp(p0)
      print "# probs(1) =", formatedp(p1)
      print line
  else :
    p = gnubg.probs(pos, np)

    err = eqError(p, probs)

    if err >= th :
      print "# err %.5lf, pos = {%s}" % (err, listToString(pos))
      print "# probs =",formatedp(probs)
      print "# ply" + str(np) + "  =",formatedp(p)
      print line

f.close()
