#! /bin/bash

path=$1"/scripts"
count=0
fileCount=10000
count=$(ls -la /mnt/data | grep -c "F")

if [ "$count" -ge "$fileCount" ]
then
	echo "Data files already exist. Skipping data generation."
	exit 0
fi
mkdir /mnt/data
cd $path
nohup python genDummyFiles.py 1000 10000 0 $fileCount /mnt/data &
while [ $count -ne $fileCount ]; do
	sleep 40
        count=$(ls -la /mnt/data | grep -c "F")
	echo "Hang on!!  Still Working.."
done

