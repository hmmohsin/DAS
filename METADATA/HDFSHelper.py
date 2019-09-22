from snakebite.config import HDFSConfig
from snakebite.namenode import Namenode as HDFSNameNode
from snakebite.channel import DataXceiverChannel
import snakebite.protobuf.ClientNamenodeProtocol_pb2 as SBClientProto
import snakebite.client as SBClient
import Queue
import pickle
import datetime
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
					"fileSize": fileSize,
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
        	dataBuff = ""
		total_bytes_sent = 0
		if not self.client.connect():
                        return
		t1 = datetime.datetime.now()
                for data in self.client.readBlock(blockSize, poolID, blockID, genStamp, 0, blockToken,False):
			total_bytes_sent = total_bytes_sent + len(data)
			if proxySock is not None:
				sock_id = proxySock.fileno()
				try:
					bytes_sent = proxySock.send(dataBuff)
					total_bytes_sent = total_bytes_sent+bytes_sent
				except Exception as errorMesage:
					print "Error: socket error (%s) on sock %d. sent %d bytes" %(str(errorMesage), sock_id, len(blockData))
					break
                        blockData += data
	
		t2 = datetime.datetime.now()
		t = t2 - t1
		print "Fetch Time taken is " + str(t.total_seconds())

                return blockData
