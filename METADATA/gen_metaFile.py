import sys
import os
import HDFSHelper
import random

config = dict()
serversList = {}

def getConfig():
	fh = open("gen_meta_config.txt","r")
	for sTuple in fh:
		key, value = sTuple.rstrip().split(' ')
		config[key] = value
	fh.close()

def getServersList():
	fh = open("../CONFIG/servers.list", "r")
	for sTuple in fh:
		sid, sIP, priPort, secPort = sTuple.rstrip('\n').split(' ')
		serversList[sid] = sIP
	fh.close()

def getServerID(ipAddr):
	for key, value in serversList.iteritems():
		if (value == ipAddr):
			return key
	return -1 

def genHDFSMeta(namenode, port, path, metaFile):
	
	nnHelper = HDFSHelper.NameNodeHelper(namenode, int(port))
	files = nnHelper.getFilesList(path)

	fh = open(metaFile,'w')
	for filename in files:
		blockCount=0
		fileID = filename.split('/')[-1].lstrip('F')
		fileBlocks = nnHelper.getFileBlocksList(filename)
		for block in fileBlocks:
			blockInfo = nnHelper.getBlockInfo(filename, block['blockID'])
			#repInfo = filename.split('/')[-1]
			
			repInfo = fileID
			
			for blockRep in blockInfo:
				if (blockCount == 0):
					fileSize = blockRep['fileSize']
					repInfo = repInfo + " " + str(fileSize)
					blockCount = blockCount + 1
				sid = getServerID(blockRep['ipAddr'])
				if (blockRep['ipAddr'] == "10.1.1.11"):
					print "s-11"
				repInfo = repInfo + " " + str(sid)
				#repInfo = repInfo + " " + blockRep['ipAddr']
				
			repInfo = repInfo + "\n"
		fh.write(repInfo)
	fh.close()

if __name__ == "__main__":

	if len(sys.argv) < 2:
		print "Usage: python genMeta.py <hdfs/disk>"
		exit(0)
	mode = sys.argv[1]
	if mode == "hdfs":
		getConfig()
		nnIPAddr = config['nnIPAddr']
		nnPort = config['nnPort']
		hdfsPath = config['hdfsPath']
		metaFile = config['metaFile']
	

		getServersList()
		genHDFSMeta(nnIPAddr, nnPort, hdfsPath, metaFile)

	elif mode == "disk":
		getConfig()
		getServersList()
		server_count = len(serversList)
		
		metaFile = config['metaFile']
		fh = open(metaFile,'w')
		for i in range(0,5000):
			if server_count == 1:
				pri = 0
				sec = 0
				ter = 0
			else:
				pri = random.randint(0,server_count-1)
				sec = pri
				while sec == pri:
					sec = random.randint(0,server_count-1)
				ter = pri
				while ter == pri or ter == sec:
					ter = random.randint(0,server_count-1)
			metaInfo = str(i) + " " + str(pri) + " " +str(sec) + " " + str(ter)+ "\n"
			fh.write(metaInfo)
		fh.close()
