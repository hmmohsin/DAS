import sys
import os
import copy
from datetime import datetime
import numpy as np

class Config:
	eid = ''
	schemes = []
	loads = []
	run_count = 0
	filesize = 0

def get_scheme_name(scheme_id):
	if (scheme_id == 0):
		return "BASE"
	elif (scheme_id == 1):
		return "DUP"
	elif (scheme_id == 2):
		return "Cloning_Prio"
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
	
res_dir = "RESULTS/RES/"
cdf_dir = "RESULTS/CDF/"
sum_file = "summary.txt"
conf_file = "CONFIG/config.txt"
resultFile = "results.txt"
summaryFile = "summary.txt"

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
for load in exp_config.loads:
	summaryHandle.write("------ Experiment: "+exp_config.eid+ " at "+str(load)+"% load ------\n")
	for scheme in exp_config.schemes:
		scheme_name = get_scheme_name(int(scheme))
		
		jFTTAll = []
		jobCountAll = 0
		
		inputFile = res_dir+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
		resFile = res_dir+"summ_"+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
		CDFFile = cdf_dir+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
		rCDFFile = cdf_dir+"r_"+str(exp_config.eid)+"_"+str(scheme_name)+"_"+str(load)
	
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
			
					priSer = int(tPData[2].split('_')[0])
					secSer = int(tPData[2].split('_')[1])
			
					priBytesTrans = int(tPData[3].split('_')[0].rstrip(' '))
					secBytesTrans = int(tPData[3].split('_')[1].rstrip(' '))
			
					priFTT = int(tPData[4].split('_')[0].rstrip(' '))
					secFTT = int(tPData[4].split('_')[1].rstrip(' '))
	
					if secBytesTrans>0:
						sec_trigger = sec_trigger + 1
	
					if scheme_name == "Single_Copy":			
						tFTT = priFTT
						priCount = priCount + 1
					elif (priBytesTrans == exp_config.filesize) & (priFTT < secFTT):
						#print str(tPData) + "Pri transfered"
						tFTT = priFTT
						priCount = priCount + 1
			
					elif (secBytesTrans == exp_config.filesize) & (secFTT < priFTT):
						tFTT = secFTT
						secCount = secCount + 1
				
					elif (priBytesTrans == exp_config.filesize) & (priFTT == secFTT):
						tFTT = priFTT
						priCount = priCount + 1
					elif (priBytesTrans == exp_config.filesize):
						tFTT = priFTT
						priCount = priCount + 1
					elif (secBytesTrans == exp_config.filesize):
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
			CDFFileHandle.write("0 0\n")
			rCDFFileHandle.write("0 0\n")

			meanFTT = round((np.mean(jFTTList)/1000000),3)
			stdFTT = round((np.std(jFTTList)/1000000),3)

			for i in range(0,1000):
				
				idx = int(jobCount*(i+1)/1000.0)-1
				cdf_data = str(i+1)+" "+ str(jFTTList[idx]) + "\n"
			        rcdf_data = str(i+1)+" "+ str(jFTTListCopy[idx]) + "\n"
				CDFList.append(round((jFTTList[idx]/1000000.0),3))
			       	rCDFList.append(round((jFTTListCopy[idx]/1000000.0),3))
				CDFFileHandle.write(cdf_data)
			        rCDFFileHandle.write(rcdf_data)

			runResult = str(datetime.now())+" "+str(exp_config.eid)+" "+str(scheme_name)+" "+str(load)+" "+str(run)+" "+str(jobCount)+" "+str(sec_trigger)+" "+str(priCount)+" "+str(secCount)+" "+str(meanFTT)+" "+str(stdFTT)+" "+str(CDFList[899])+" "+str(CDFList[949])+" "+str(CDFList[989])+" "+str(CDFList[998])+"\n"
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
		for i in range(0,101):
			idx = int(jobCountAll*(i)/100.0)
			if idx > 0:
				idx=idx-1
		       	cdf_data = str(i)+" "+ str(jFTTAll[idx]) + "\n"
			CDFList.append(round((jFTTAll[idx]/1000000.0),3))
			CDFFileHandle.write(cdf_data)
		CDFFileHandle.close()

		p50 = str(CDFList[50])
		p90 = str(CDFList[90])
		p95 = str(CDFList[95])
		p99 = str(CDFList[99])
		#p999 = str(CDFList[998])

		if scheme_name == "BASE":
			scheme_name = "Single_Copy"
		elif scheme_name == "DUP":
			scheme_name = "Cloning"
		elif scheme_name == "PURGE":
			scheme_name = "DANS"
		elif scheme_name == "HEDGED_REQ":
			scheme_name = "Hedged_Req"
		summaryResult = str(scheme_name)+" "+str(stdFTT)+" "+str(meanFTT)+" "+p50+" "+p90+" "+p95+" "+p99+"\n"
		summaryHandle.write(summaryResult)

summaryHandle.close()
