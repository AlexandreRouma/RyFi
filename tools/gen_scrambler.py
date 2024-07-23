import random
import sys

SCRAMBLER_LEN   = 4*255

data = []
for i in range(SCRAMBLER_LEN):
    if not (i % 16):
        sys.stdout.write('\n')
    sys.stdout.write('0x%02X, ' % random.randint(0, 255))
    
