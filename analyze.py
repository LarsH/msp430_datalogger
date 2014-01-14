#import matplotlib
#matplotlib.use('GTKAgg') # do this before importing pylab
#import matplotlib.pyplot as plt

def debug(s):
   print s
   pass

def openlogfile(fname):
   debug('Loading file')
   b = open(fname).read()
   debug('Splitting')
   l = b.split('<STARTING>')
   debug('Split done')
   assert len(l) != 1, "Could not find starting point in logfile"

   if len(l) > 2:
      print "Warning: More than one start found in logfile, using first only"
   b = l[1]

   samples = []

   t = 0                   # Accumulated time
   oldVal = oldTar = 0     # Holds values from previous sample during iteration

   debug('Starting iteration...')

   i = 0 # index counter
   while i+3 <= len(b):
      flags, t1, t2 = ord(b[i]), ord(b[i+1]), ord(b[i+2])

      if i % 10000 == 0:
         debug('%u / %u (%f)'%(i, len(b), i/float(len(b))))

      # Mask out measured value
      val = flags & 1

      # Handle escaped flow control characters
      if flags & 8:
         t2 |= 1
      if flags & 16:
         t1 |= 1

      # Is the current sampling from a timer?
      isTime = flags & 2

      # Decode then stored TAR register
      tar = t1 + (t2*256)
      dt = tar - oldTar

      if oldTar >= tar:
         # The TAR register wrapped around, adjust for that
         dt += 0x10000

      # Accumulate time
      t += dt

      samples += [(t, val)]
      i += 3

   return samples

'''
plt.plot(x,y,'*-')
plt.show()
'''

from sys import argv
if __name__ == '__main__':
   if len(argv) != 2:
      print "Usage: %s logfile" % argv[0]
   else:
      f = openlogfile(argv[1])
