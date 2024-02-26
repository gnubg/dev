#!/usr/bin/env pygnubg 
from __future__ import division

import optparse
from bgutil import swapSides, eq

parser = optparse.OptionParser("%prog [OPTIONS] main-bm update-bm")
options, args = parser.parse_args()


f = file(args[1])
# remove header
f.readline()

upd = dict()
for line in f:
  line = line.strip()
  if line[0] == 'r' :
    lastSeed = line[2:]
  elif line[0] == 'o' :
    assert lastSeed not in upd,lastSeed
    upd[lastSeed] = line.split(' ')[1:]
  elif line[0] == '#' :
    # skip "# ply <n>" lines
    continue
  else :
    raise line
  
mainBM = file(args[0])
moveWithSeed = None

for line in mainBM :
  line = line.strip()
  if line[0] in '#so' :
    pass
  elif line[0] == 'r' :
    sd = line[2:]
    if sd in upd :
      moveWithSeed = upd[sd]
    else :
      moveWithSeed = None
  elif line[0] == 'm' :
    if moveWithSeed is not None: 
      l = line.split(' ')
      board = l[1]
      d1,d2 = [int(x) for x in l[2:4]]
      mm = gnubg.moves(board, d1, d2)
      mmr = [gnubg.keyofboard(swapSides(gnubg.boardfromkey(x))) for x in mm]
      
      if moveWithSeed[0] in mmr and moveWithSeed[0] not in l[4::2]:
        # combine
        print '#R'," ".join(moveWithSeed)
        moveEQ = -eq([float(x) for x in moveWithSeed[1:]])
        lm = l[4:]
        a = float(lm[1])
        sm = [(lm[0],a)] + [(x,a - float(e)) for x,e in zip(lm[2::2], lm[3::2])] \
             + [(moveWithSeed[0],moveEQ)]
        sm = sorted(sm, key = lambda x : x[1], reverse=1)
        print " ".join(l[:4]),

        print sm[0][0],"%.7f" % sm[0][1],
        for x,e in sm[1:] :
          print x,"%.7f" % (sm[0][1] - e),
        print
        line = ""

  if len(line) :
    print line
    
