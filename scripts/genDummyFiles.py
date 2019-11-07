import time
from random import random
import os
import sys

def genFile(bs, bc, si, fc, dd):
	try:
		os.stat(dd)
	except:
		os.mkdir(dd)
	
	if not dd.endswith('/'):
		dd += '/'

	for idx in range(int(si),int(fc)+int(si)):
	        filename = dd+"F"+str(idx)
	        
		cmd = 'dd if=/dev/zero of='+str(filename)+' bs=' +str(bs)+' count='+str(bc)
	        os.system(cmd)

bs = sys.argv[1]
bc = sys.argv[2]
si = sys.argv[3]
fc = sys.argv[4]
dd = sys.argv[5]
genFile(bs, bc, si, fc, dd)
