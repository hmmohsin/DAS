import struct
import os

def parse(msgHdr):
        msg = {}
	data = struct.unpack('iiiii',msgHdr)
        msg['reqType'] = data[0]
	msg['priority'] = data[1]
	msg['objectID'] = data[2]
	msg['startIdx'] = data[3]
	msg['length']	= data[4]
        return msg

def createResponse(msgType, prio, fileID, startIdx, size):
	msgHdr = struct.pack('iiiii',msgType,prio,fileID,startIdx,size)
	return msgHdr

def getObjPath(objID, dataDir):
	path = dataDir+"/F"+str(objID)
	return path 
