import re, subprocess

with open('nestest/nestest.log', 'r') as srcFile:
   lines = srcFile.readlines()

insts = [re.search('([0-9A-F]{4})  ([0-9A-F]{2}) [^:]+:([0-9A-F]{2}) X:([0-9A-F]{2}) Y:([0-9A-F]{2}) P:([0-9A-F]{2}) SP:([0-9A-F]{2})', l) for l in lines]

p = subprocess.Popen(["./DonoNES", 'nestest/nestest.nes'], stdout = subprocess.PIPE, stdin = subprocess.PIPE)
try:
   for inst in insts:
      line = p.stdout.readline()
      groups = re.search('0x([0-9A-F]{4}): 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2})', line)

      expected = [inst.group(  n+1) for n in xrange(7)]
      got      = [groups.group(n+1) for n in xrange(7)]

      pc = inst.group(1)
      op = inst.group(2)
      A  = inst.group(3)
      X  = inst.group(4)
      Y  = inst.group(5)
      P  = inst.group(6)
      SP = inst.group(7)

      if int(pc, 16) == int('C708', 16):
         while got[0] != expected[0]:
            p.stdin.write("\n")
            line = p.stdout.readline()
            groups = re.search('0x([0-9A-F]{4}): 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2}) 0x([0-9A-F]{2})', line)
            got      = [groups.group(n+1) for n in xrange(7)]

      print line
      if int(pc, 16) >= int('C6B8', 16) and int(pc, 16) < int('C708', 16):
         pass
      else:
         for n in xrange(7):
            if expected[n] != got[n]:
               print 'Error'
               print 'Expected', " ".join(expected)
               exit()

      p.stdin.write("\n")
finally:
   p.kill()
