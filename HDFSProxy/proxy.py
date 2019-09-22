import socket
import select
import os
import sys
import config
import worker
import helper
import metadata

configFile = "config.txt"

HDRLEN = 20
REQUEST_GET = 1
REQUEST_ACCEPT = 11
REQUEST_REJECT = 12
REQUEST_NOTFOUND = 13
REQUEST_PURGE = 2

def runProxy(ipAddr):
	conf = config.config(configFile)
	conf.load_config()
	
	lport = conf.get_config('lport')
	#ipAddr = conf.get_config('ipAddr')

	nnIPAddr = conf.get_config('nnIPAddr')
	nnPort = conf.get_config('nnPort')
	HDFSDataDir = conf.get_config('HDFSDataDir')
	
	metaDataHandle = metadata.MetaData(nnIPAddr, nnPort, ipAddr)
	metaDataHandle.loadMetaData(HDFSDataDir)

	print "Ready"
	servSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	servSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	servSock.setblocking(0)
	servSock.bind((ipAddr, lport))
	servSock.listen(10)
	connList = {}
	reqMsgList = {}
	

	epoll = select.epoll()
	epoll.register(servSock.fileno(), select.EPOLLIN)

	try:
		while True:
			events = epoll.poll(1)
			for sockDesc, event in events:
				if sockDesc == servSock.fileno():
					print "Received new connection"
					conn, addr = servSock.accept()
					conn.setblocking(0)
					epoll.register(conn.fileno(), select.EPOLLIN)
					connList[conn.fileno()] = conn
				elif event & select.EPOLLHUP:
					epoll.unregister(sockDesc)
					connList[sockDesc].close()
					del connList[sockDesc]
				elif event & select.EPOLLIN:
					cSock = connList[sockDesc]
					data = cSock.recv(1024)
					if len(data) == 0:
						print "Connection closed"
						epoll.unregister(sockDesc)
                                        	connList[sockDesc].close()
                                        	del connList[sockDesc]
						continue
					if sockDesc in reqMsgList.keys():
						reqMsgList[sockDesc] += data
					else:
						reqMsgList[sockDesc] = data

					if len(reqMsgList[sockDesc]) == HDRLEN:
						epoll.unregister(sockDesc)
						msgHdr = reqMsgList[sockDesc]
						msg = helper.parse(msgHdr)
						if msg['reqType'] == REQUEST_GET:

                                                        del connList[sockDesc]
							del reqMsgList[sockDesc]

							blocksInfo = {}
                                                        filePath = helper.getObjPath(msg['objectID'], HDFSDataDir)
                                                        
							print "Received new request for file %s" %filePath
							if (metaDataHandle.fileExist(filePath)):
								
								fileMeta = metaDataHandle.getFileMeta(filePath)
								responseMsg = helper.createResponse(REQUEST_ACCEPT, msg['priority'], msg['objectID'], 0, fileMeta['numBytes'])
								cSock.send(responseMsg)
                                                        	newWorker = worker.worker(filePath, fileMeta, cSock)
								newWorker.start()
							else:
								print "File %s not found" %filePath
								responseMsg = helper.createResponse(REQUEST_NOTFOUND, msg['priority'], msg['objectID'], 0, 0)
								cSock.send(responseMsg)
	except socket.error, msg:
		print "Some issue in running proxy. %s\n" %msg

if __name__ == "__main__":
	ipAddr = sys.argv[1]
	runProxy(ipAddr)
