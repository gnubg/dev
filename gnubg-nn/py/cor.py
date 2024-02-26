from math import sqrt

class Correlation :
  "Collect and compute correlation between x_i and y_i"
  
  def __init__(self) :
    self.n = 0
    self.sx = 0.0
    self.sy = 0.0
    self.x2 = 0.0
    self.y2 = 0.0
    self.xy = 0.0

  def __repr__(self) :
    return \
      "[%ld,%g,%g,%g,%g,%g]" % (self.n,self.sx,self.sy,self.x2,self.y2,self.xy)
  
  def add(self, x, y) :
    self.n  += 1
    self.sx += x
    self.sy += y
    self.x2 += x*x
    self.y2 += y*y
    self.xy += x*y

  def compute(self) :
    if self.n > 1  :
    
      ax = self.sx / self.n
      vx = (self.x2 / self.n) - ax * ax

      ay = self.sy / self.n
      vy = (self.y2 / self.n) - ay * ay;

      if vx > 0 and vy > 0 :
        return (self.xy / self.n - ax * ay) / sqrt(vx * vy);

    return 0.0
    
  def coef(self) :
    d = self.n * self.x2 - self.x * self.x
    da = self.n * self.xy - self.x * self.y
    a = da / a
    b = (a * self.x - self.y) / self.n

    return (a,b)
    
