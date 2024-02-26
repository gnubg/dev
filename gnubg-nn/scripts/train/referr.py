import sys
import gnubg
from bgutil import find

def matchEqutiy(i,l) :
  db,tk = l[0:2]
  
  if not db :
    return i[3]
  
  if tk:
    return i[4]

  return i[5]


def benchmarkError(refFileName, verbose = 0) :
  """ error of current net on ref data"""
  
  nCubePos = 0
  dcubeErr = 0.0
  tcubeErr = 0.0

  nMovePos = 0
  moveErr = 0.0

  refFile = file(refFileName)
  for line in refFile :
  
    if line[0] == 'o' :
      nCubePos += 1

      l = line.split()
      
      pb = l[1]
      proll = [float(x) for x in l[2:]]

      pos = gnubg.boardfromkey(pb)

      gnubg.set.score(7,7)

      di = gnubg.doubleroll(pos, n = 0, i = 1)

      rdi = gnubg.doubleroll(pos, n = 0, i = 1, p = proll)

      if rdi[0:3] != di[0:3] :
        eNet = matchEqutiy(rdi, di)
        eRoll = matchEqutiy(rdi, rdi)

        err = abs(eRoll - eNet)
	  
        if di[0] != rdi[0] :
          dErr = err
        else :
          dErr = 0.0

        if di[1] != rdi[1] :
          tErr = err
        else :
          tErr = 0.0

        dcubeErr += dErr
        tcubeErr += tErr

      gnubg.set.score(0,0)

      if verbose and (nCubePos % 5000) == 0 :
        print >> sys.stderr, "c",
      
    elif line[0] == 'm' :
      l = line.split()
      
      pb,d0,d1 = l[1:4]
      moves = l[4:]

      pos = gnubg.boardfromkey(pb)

      moveb = gnubg.bestmove(pos, int(d0), int(d1), 0, b = 1) [1]

      if moveb == moves[0] :
        loss = 0.0
      else :
        i = find(moveb, moves)

        if i >= 0 :
          loss = float(moves[i+1])
        else :
          loss = float(moves[-1])
	
      nMovePos += 1
      moveErr += loss
      if verbose and (nMovePos % 5000) == 0 :
        print >> sys.stderr, "m",
      
  refFile.close()

  if nMovePos == 0 :
    return (0, 0, 0)
  
  if nCubePos == 0 :
    return (moveErr/nMovePos, 0, 0)
  
  return (moveErr/nMovePos, dcubeErr/nCubePos, tcubeErr/nCubePos)
