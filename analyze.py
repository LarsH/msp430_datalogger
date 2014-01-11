#import matplotlib
#matplotlib.use('GTKAgg') # do this before importing pylab
import matplotlib.pyplot as plt

b = open('sample.bin').read()
l = b.split('<STARTING>')
k = map(len, l)
print k
b = l[1]
print len(l) - 1

x = [0,0]
y = [-1,3]
t = 0
oldTar = 0
oldC = 0

b = map(ord, b)
l1 = b[0::3]
l2 = b[1::3]
l3 = b[2::3]

for f, t1, t2 in zip(l1, l2, l3):

   c = f & 1
   if f & 8:
      t2 |= 1
   if f & 16:
      t1 |= 1
   isTime = f & 2

   tar = t1 + (t2*256)
   dt = tar - oldTar
   if oldTar >= tar:
      dt += 0x10000
   t += dt

   #print "%2u %8u %1u %u" %(c, tar, isTime, dt)

   if c != oldC or isTime:
      x += [t]
      y += [oldC]
   if f & 2:
      x += [t]
      y += [2]
   x += [t]
   y += [c]
   oldC = c
   oldTar = tar
   if len(x) > 10000:
      break

plt.plot(x,y,'*-')
plt.show()
