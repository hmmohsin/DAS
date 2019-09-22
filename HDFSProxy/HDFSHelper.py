from snakebite.config import HDFSConfig
from snakebite.namenode import Namenode as HDFSNameNode
from snakebite.channel import DataXceiverChannel
import snakebite.protobuf.ClientNamenodeProtocol_pb2 as SBClientProto
import snakebite.client as SBClient
import Queue
import pickle
import socket

class NameNodeHelper(object):
	def __init__(self, namenode, port):
		self.client = self.createClient(namenode, port)
	
	def createClient(self, namenode, port):
		p = 'namenode='+str(namenode)+" port="+str(port)
		print p
		return SBClient.Client(namenode, port=port)
	
	def getFilesList(self, path='/hadoop'):
		filesList=[]
		for file in self.client.ls([path]):
			fileName = file['path']
			fileType = file['file_type']
			if fileType == 'd':
				filesListTmp = self.getFilesList(fileName)
				filesList = filesList + filesListTmp
			else:
				filesList.append(fileName)
		return filesList

	def getFileInfo(self, fileName):
		fileMetaInfo = self.client._get_file_info(fileName)
		blockCount = int(fileMetaInfo.fs.length)/int(fileMetaInfo.fs.blocksize)
		if blockCount*fileMetaInfo.fs.blocksize < fileMetaInfo.fs.length:
			blockCount = blockCount + 1
		fileInfo = {
			"filePath": fileName,
			"fileID": fileMetaInfo.fs.fileId,
			"fileOwner": fileMetaInfo.fs.owner,
			"fileSize": fileMetaInfo.fs.length,
			"fileBlockSize": fileMetaInfo.fs.blocksize,
			"fileBlockCount": blockCount,
			"fileReplication": fileMetaInfo.fs.block_replication
		}
		return fileInfo
		
	def getFileBlocksList(self, fileName):
		blockMetaInfo = {}
		blockMetaInfoList = []
		fileMetaInfo = self.client._get_file_info(fileName)
		fileDataInfo = fileMetaInfo.fs
		fileSize = fileDataInfo.length
		blockLocationReq = SBClientProto.GetBlockLocationsRequestProto()
		blockLocationReq.src = fileName
		blockLocationReq.length = fileSize
		blockLocationReq.offset = 0L
		blockInfoList = self.client.service.getBlockLocations(blockLocationReq)
		
		blockCount = int(fileMetaInfo.fs.length)/int(fileMetaInfo.fs.blocksize)
                if blockCount*fileMetaInfo.fs.blocksize < fileMetaInfo.fs.length:
                        blockCount = blockCount + 1
		
		for i in range(0,blockCount-1):
			block =  blockInfoList.locations.blocks[i]
			#print block
			blockMetaInfo = {
				"blockID": block.b.blockId,
				"numBytes": block.b.numBytes,
				"poolID": block.b.poolId,
				"generationStamp": block.b.generationStamp
			}
			blockMetaInfoList.append(blockMetaInfo)
		
		block = blockInfoList.locations.lastBlock
		blockMetaInfo = {
                	"blockID": block.b.blockId,
               		"numBytes": block.b.numBytes,
                        "poolID": block.b.poolId,
			"generationStamp": block.b.generationStamp,
			"offset": block.offset
             	}
                blockMetaInfoList.append(blockMetaInfo)
		return blockMetaInfoList

	def getBlockInfo(self, fileName, blockID):
		#print 'FileName: %s'%fileName
		fileMetaInfo = self.client._get_file_info(fileName)
		#print 'FileMetaInfo: %s'%fileMetaInfo
		fileDataInfo = fileMetaInfo.fs
		fileSize = fileDataInfo.length
		fileReplication = fileMetaInfo.fs.block_replication
		
		blockLocationReq = SBClientProto.GetBlockLocationsRequestProto()
		blockLocationReq.src = fileName
		blockLocationReq.length = fileSize
		blockLocationReq.offset = 0L
		blockInfoList = self.client.service.getBlockLocations(blockLocationReq)

                blockCount = int(fileMetaInfo.fs.length)/int(fileMetaInfo.fs.blocksize)
		if blockCount*fileMetaInfo.fs.blocksize < fileMetaInfo.fs.length:
                        blockCount = blockCount + 1
               
		blockReplicaInfoList = [] 
		for i in range(0,blockCount-1):
			block =  blockInfoList.locations.blocks[i]
			if blockID == block.b.blockId:
				for replica in range(0,int(fileReplication)):
					blockReplicaInfo = {
						"replicaID": replica,
						"storageID": block.locs[replica].id.storageID,
						"ipAddr": block.locs[replica].id.ipAddr,
						"hostName": block.locs[replica].id.hostName,
						"txPort": block.locs[replica].id.xferPort,
						"infoPort": block.locs[replica].id.infoPort,
						"ipcPort": block.locs[replica].id.ipcPort,
						"blockToken": block.blockToken
					}
					blockReplicaInfoList.append(blockReplicaInfo)

                block = blockInfoList.locations.lastBlock
		
		if blockID == block.b.blockId:
			#There is an issue in fetching all replica's Information
			#Need Debugging and review of the Snakebite API
                	#for replica in range(0,int(fileReplication)):
                	for replica in range(0,int(fileReplication)):
				blockReplicaInfo = {
                	                "replicaID": replica,
                                        "storageID": block.locs[replica].id.storageID,
                                        "ipAddr": block.locs[replica].id.ipAddr,
                                        "hostName": block.locs[replica].id.hostName,
                                        "txPort": block.locs[replica].id.xferPort,
                                        "infoPort": block.locs[replica].id.infoPort,
                                        "ipcPort": block.locs[replica].id.ipcPort,
					"blockToken": block.blockToken
                                }
				blockReplicaInfoList.append(blockReplicaInfo)
		#print blockReplicaInfoList
		return blockReplicaInfoList			

class DataNodeHelper(object):
        def __init__(self, datanode, port):
                self.client = self.createClient(datanode, port)

        def createClient(self, datanode, port):
                return DataXceiverChannel(datanode, port)

        def getBlockData(self, poolID, blockID, blockSize, genStamp, blockToken, proxySock=None):
                blockData = ""
                total_bytes_sent = 0
		if not self.client.connect():
                        return
                for data in self.client.readBlock(blockSize, poolID, blockID, genStamp, 0, blockToken,False):
			if proxySock is not None:
				proxySock.setblocking(1)
				try:
					sock_id = proxySock.fileno()
					bytes_sent = proxySock.send(data)
					total_bytes_sent += bytes_sent
				except socket.error as e:
					print "Error: socket error on sock %d. sent %d bytes" %(sock_id, total_bytes_sent)
					break
		
		print "Info: File transfered. sent %d bytes" %total_bytes_sent
                return blockData
