import HDFSHelper
import os

class MetaData(object):
        def __init__(self, namenode, port, dnIPAddr):
                self.nameNodeHelper = HDFSHelper.NameNodeHelper(namenode, port)
                self.files = []
                self.blocksInfo = {}
		self.repsInfo = {}
		self.ipAddr = dnIPAddr		
		print "Input: %s %s %s" %(namenode, port, dnIPAddr)

        def loadFilesList(self, path):
		self.files = self.nameNodeHelper.getFilesList(path)
		return self.files

	def fileExist(self, fileName):
		if fileName in self.files:
			return True
		return False

	def loadFilesBlocksList(self):
		#logFile = "filesList_"+self.ipAddr+".log"
		#fh = open("list.log","w")
		for fileName in self.files:
			fileBlocksList = self.nameNodeHelper.getFileBlocksList(fileName)
			'''Assuming each file is composed of a single block
			but since getFileBlocksList method returns a list, so
			we have to iterate over the list. There should only be
			a single block info element in the list.
			'''
			for blk in fileBlocksList:
				blockInfo = self.nameNodeHelper.getBlockInfo(fileName, blk['blockID'])
				repCount = 0
				for blockRep in blockInfo:
					self.blocksInfo[fileName] = blk
					if blockRep['ipAddr'] == self.ipAddr:
						#logStr="ipAddr:%s File:%s repCount=%d\n" \
						#	%(self.ipAddr, fileName, repCount)
						#fh.write(logStr)
						self.repsInfo[(fileName, repCount)] = blockRep
		#fh.close()
	
	def loadMetaData(self, dirPath):
		self.loadFilesList(dirPath)
		self.loadFilesBlocksList()
		print "Data loaded"	
	def getFileBlocks(self, fileName):
		fileBlock = self.blocksInfo[fileName]
		return fileBlock
				 
	def getBlockInfo(self, fileName, repID):
		blockInfo = self.blocksInfo[(fileName, repID)]
		return blockInfo
	
	def getFileMeta(self, fileName):
		fileMeta = {}
		fileBlock = self.blocksInfo[fileName]
		blockRep = self.repsInfo[(fileName, 0)]
		fileMeta['poolID'] = fileBlock['poolID']
		fileMeta['blockID'] = fileBlock['blockID']
		fileMeta['numBytes'] = fileBlock['numBytes']
		fileMeta['generationStamp'] = fileBlock['generationStamp']
		fileMeta['blockToken'] = blockRep['blockToken']
		fileMeta['ipAddr'] = blockRep['ipAddr']
		fileMeta['txPort'] = blockRep['txPort']
		return fileMeta
