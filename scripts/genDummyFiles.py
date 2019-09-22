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

if len(sys.argv) != 6:
        print 'usage: python genFile.py blockSize blockCount startIDX fileCount dataDir'
        print 'defualt: blockSize=1000(bytes) blockCount=10000 startIDX=0 fileCount=5000 dataDir=/mnt/extra/data/'
	print 'continue with default(Y/N)?\r'
	uInput = sys.stdin.read(1)
	if uInput == 'N' or uInput == 'n':
		exit(0)
	elif uInput == 'Y' or uInput == 'y':
		dd = '/mnt/extra/data/'
		genFile(1000, 10000, 0, 5000, dd)
	else:
		print 'Invalid option! Exiting now.'
		exit(0)
else:
	bs = sys.argv[1]
	bc = sys.argv[2]
	si = sys.argv[3]
	fc = sys.argv[4]
	dd = sys.argv[5]
	genFile(bs, bc, si, fc, dd)

