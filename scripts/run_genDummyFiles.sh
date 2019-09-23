#! /bin/bash

path=$1"/scripts"
fileCount=$2
echo "fileCount="
echo $fileCount
count=0
count=$(ls -la /mnt/extra/data | grep -c "F")

if [ "$count" -ge "$fileCount" ]
then
	echo "Data files already exist. Skipping data generation."
	exit 0
fi
mkdir /mnt/extra/data
cd $path
nohup python genDummyFiles.py 1000 10000 0 $fileCount /mnt/extra/data &
while [ $count -ne $fileCount ]; do
	sleep 40
        count=$(ls -la /mnt/extra/data | grep -c "F")
	echo "Hang on!!  Still Working.."
done

