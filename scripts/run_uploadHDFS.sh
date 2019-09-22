#! /bin/bash

fileCount=10000
count=$(hadoop fs -ls /data/ | grep -c "F")
if [[ $count -ge $fileCount ]]
then
	echo "Data files already exist. Skipping HDFS upload.."
	echo $count
	echo $fileCount
	exit 0
fi
hadoop fs -mkdir /data
nohup hadoop fs -put /mnt/data/F* /data &
while [[ $count -lt $fileCount ]]; do
	sleep 5
        count=$(hadoop fs -ls /data | grep -c "F")
	echo "Hang on!!  Still Working.."
done
