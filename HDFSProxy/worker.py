import socket
import HDFSHelper
import threading
import datetime

class worker(threading.Thread):
        def __init__(self, fileName, fileMeta, reqSocket):
                threading.Thread.__init__(self)
		self.fileName = fileName
                self.fileMeta = fileMeta
                self.reqSocket = reqSocket

	def run(self):
		fileName = self.fileName
		dnAgent = HDFSHelper.DataNodeHelper(self.fileMeta['ipAddr'], self.fileMeta['txPort'])
		
		tStart = datetime.datetime.now()
		dnAgent.getBlockData(self.fileMeta['poolID'], \
				self.fileMeta['blockID'], \
				self.fileMeta['numBytes'], \
				self.fileMeta['generationStamp'], \
				self.fileMeta['blockToken'], \
				self.reqSocket)
		tEnd = datetime.datetime.now()
		tTotal = tEnd - tStart
		print tTotal.total_seconds()
		#print "File: %s time %d" %(fileName, tTotal.total_seconds())
		self.reqSocket.close()
