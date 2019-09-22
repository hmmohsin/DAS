import sys
import os
import copy
import math
from datetime import datetime
import numpy as np

res_dir = "../RESULTS/RES/"
cdf_dir = "../RESULTS/CDF/"
sum_file = "../summary.txt"
conf_file = "../CONFIG/exp.conf"
resultFile = "../RESULTS/results.txt"
summaryFile = "../RESULTS/summary.txt"
metadataFile = "../METADATA/metadata.txt"
metadataStore={}

class Config:
	eid = ''
	schemes = []
	loads = []
	run_count = 0
	filesize = 0

def load_metadata():
	fh = open(metadataFile)
	for line in fh:
		oid,oSize,pri,sec,ter = line.rstrip().split(' ')
		metadataStore[int(oid)]=(int(oSize), int(pri), int(sec), int(ter))

def get_filesize(oid):
	filesize=metadataStore[oid][0]
	return filesize

def get_scheme_name(scheme_id):
	if (scheme_id == 0):
		return "BASE"
	elif (scheme_id == 1):
		return "DUP"
	elif (scheme_id == 2):
		return "PRIO"
	elif (scheme_id == 3):
		return "PURGE"
	elif (scheme_id == 4):
		return "DAS_Agg"
	elif (scheme_id == 5):
		return "HEDGED_REQ"
	elif (scheme_id == 7):
		return "APPTO"
	elif (scheme_id == 9):
		return "TIED"
	else:
		print "Invalid Scheme"
		exit(0)
	
exp_config = Config()

conf_file_handle = open(conf_file,'r')

for line in conf_file_handle:
	key, value = line.rstrip().split(' ')[0], line.rstrip().split(' ')[1:]
	if (key == "eid"):
		exp_config.eid = value[0]
	elif (key == "scheme"):
		exp_config.schemes = value
	elif (key == "pLoad"):
		exp_config.loads = value
	elif (key == "run_count"):
		exp_config.run_count = int(value[0])
	elif (key == "file_size"):
		exp_config.filesize = int(value[0])*1000
	elif (key=="cli_count"):
		exp_config.cli_count = int(value[0])

summaryHandle = open(summaryFile,"a+")
p50 = 499
p90 = 899
p95 = 949
p99 = 989
p999 = 998
p100 = 999

if len(sys.argv) > 1:
	range_st = int(sys.argv[1])
	range_end = int(sys.argv[2])

for load in exp_config.loads:
	summaryHandle.write("------ Experiment: "+exp_config.eid+ " at "+str(load)+"% load ------\n")
	load_metadata()
	for scheme in exp_config.schemes:
		scheme_name = get_scheme_name(int(scheme))
		
		jFTTAll = []
		jobCountAll = 0
		
		inputFile = res_dir+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
		resFile = res_dir+"summ_"+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
		CDFFile = cdf_dir+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
		rCDFFile = cdf_dir+"r_"+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
	

		print inputFile
		for run in range(0,exp_config.run_count):

			cli_count = exp_config.cli_count
			inputFilePrefix = str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)+"_"+str(run)+"_cli-"
			filesList = []
		
			print inputFilePrefix
			for i in os.listdir(res_dir):
				if i.startswith(inputFilePrefix):
					filesList.append(i)

			inputFileAgg = inputFile+"_"+str(run)
			inputFileAggHandler = open(inputFileAgg,"w")
			for filename in filesList:
				filepath = res_dir+filename
				filenameHandle = open(filepath)
				for line in filenameHandle:
					inputFileAggHandler.write(line)
				filenameHandle.close()
			inputFileAggHandler.close()
			


			count = 0
			jFTTList = []
			resFileHandle = open((resFile+"_"+str(run)),"w")
			priCount = 0
			secCount = 0
			sec_trigger = 0

			iFile = open ((inputFile+"_"+str(run)),'r')
			jobCount = 0

			for jobData in iFile:
				jPData = jobData.split('|')
				jInfo = jPData[0]
				jobID = int(jInfo.split(' ')[0])
				tij = int(jInfo.split(' ')[1])
		
				jFTT = 0
				incomplete = 0
		
				for i in range(1,tij+1):
					tPData = jPData[i].split(' ')
					tid = int(tPData[0])
					oid = int(tPData[1])
			
					filesize = int(tPData[2])
					
					if (len(sys.argv)>1):
						if (filesize<=range_st or filesize>range_end):
							continue
					
					priSer = int(tPData[3].split('_')[0])
					secSer = int(tPData[3].split('_')[1])
					
					priBytesTrans = int(tPData[4].split('_')[0].rstrip(' '))
					secBytesTrans = int(tPData[4].split('_')[1].rstrip(' '))
			
					priFTT = int(tPData[5].split('_')[0].rstrip(' '))
					secFTT = int(tPData[5].split('_')[1].rstrip(' '))
	
					if secBytesTrans>0:
						sec_trigger = sec_trigger + 1
	
					if scheme_name == "Single_Copy":			
						tFTT = priFTT
						priCount = priCount + 1
					elif (priBytesTrans == filesize) & (priFTT < secFTT):
						#print str(tPData) + "Pri transfered"
						tFTT = priFTT
						priCount = priCount + 1
			
					elif (secBytesTrans == filesize) & (secFTT < priFTT):
						tFTT = secFTT
						secCount = secCount + 1
				
					elif (priBytesTrans == filesize) & (priFTT == secFTT):
						tFTT = priFTT
						priCount = priCount + 1
					elif (priBytesTrans == filesize):
						tFTT = priFTT
						priCount = priCount + 1
					elif (secBytesTrans == filesize):
						tFTT = secFTT
						secCount = secCount + 1
					else:
						print str(priBytesTrans) + " " + str(secBytesTrans)
						tFTT = -1
						jFTT = -1
						break
			
					if tFTT > jFTT:
						jFTT = tFTT
		
				if jFTT == -1:
					continue
		
				jobCount = jobCount + 1
				jstr = jInfo + str(jFTT) + "\n"
				resFileHandle.write(jstr)
				jFTTList.append(jFTT)
			
			jobCountAll = jobCountAll + jobCount
			jFTTAll = jFTTAll + jFTTList

			timestamp = str(datetime.now())
			jFTTListCopy = copy.deepcopy(jFTTList)

			jFTTList.sort()
			jFTTListCopy.sort(reverse=1)

			CDFFileHandle = open((CDFFile+"_"+str(run)),"w")
			rCDFFileHandle = open((rCDFFile+"_"+str(run)),"w")
			resultHandle = open(resultFile,"a+")

			CDFList=[]
			rCDFList = []

			meanFTT = round((np.mean(jFTTList)/1000000),3)
			stdFTT = round((np.std(jFTTList)/1000000),3)

			print max(jFTTList)
			CDFFileHandle.write("0 0\n")
			for i in range(1,1001):
				
				idx = int(math.ceil(jobCount*(i)/1000.0))-1
                                str_tmp = str(i)+ " " + str(idx) + "\n"
				cdf_data = str(i)+" "+ str(jFTTList[idx]) + "\n"
			        rcdf_data = str(i)+" "+ str(jFTTListCopy[idx]) + "\n"
				CDFList.append(round((jFTTList[idx]/1000000.0),3))
			       	rCDFList.append(round((jFTTListCopy[idx]/1000000.0),3))
				CDFFileHandle.write(cdf_data)
			        rCDFFileHandle.write(rcdf_data)

			runResult = str(datetime.now())+" "+str(exp_config.eid)+" "+str(scheme_name)+" "+str(load)+" "+str(run)+" "+str(jobCount)+" "+str(sec_trigger)+" "+str(priCount)+" "+str(secCount)+" "+str(meanFTT)+" "+str(stdFTT)+" "+str(CDFList[p90])+" "+str(CDFList[p95])+" "+str(CDFList[p99])+" "+str(CDFList[p999])+" "+str(CDFList[p100])+"\n"
			resultHandle.write(runResult)
		

			iFile.close()
			resFileHandle.close()
			CDFFileHandle.close()
			rCDFFileHandle.close()
			resultHandle.close()
		
		CDFFileHandle = open((CDFFile+"_All"),"w")
		meanFTT = round((np.mean(jFTTAll)/1000000.0),3)
		stdFTT = round((np.std(jFTTAll)/1000000.0),3)
		CDFList = []
		jFTTAll.sort()	
	
		print max(jFTTAll)
		CDFFileHandle.write("0 0\n")
		for i in range(1,1001):
			idx = int(math.ceil(jobCountAll*(i)/1000.0))-1
			cdf_data = str(i)+" "+ str(jFTTAll[idx]) + "\n"
			CDFList.append(round((jFTTAll[idx]/1000000.0),3))
			CDFFileHandle.write(cdf_data)
		CDFFileHandle.close()

		r50 = str(CDFList[p50])
		r90 = str(CDFList[p90])
		r95 = str(CDFList[p95])
		r99 = str(CDFList[p99])
		r999 = str(CDFList[p999])
		r100 = str(CDFList[p100])

		if scheme_name == "BASE":
			scheme_name = "Single_Copy"
		elif scheme_name == "DUP":
			scheme_name = "Cloning"
		elif scheme_name == "PURGE":
			scheme_name = "DANS"
		elif scheme_name == "HEDGED_REQ":
			scheme_name = "Hedged_Req"
		summaryResult = str(scheme_name)+" "+str(stdFTT)+" "+str(meanFTT)+" "+str(CDFList[p50])+" "+str(CDFList[p90])+" "+str(CDFList[p95])+" "+str(CDFList[p99])+" "+str(CDFList[p999])+" "+str(CDFList[p100])+"\n"
		summaryHandle.write(summaryResult)

summaryHandle.close()
