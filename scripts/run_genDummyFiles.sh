#! /bin/bash

home=$1
scripts="$home/scripts"
expConfig="$home/CONFIG/exp.conf"
fileSizeConfig=($(sed -n '/^file_size/p' $expConfig))
fileCountConfig=($(sed -n '/^file_data_set/p' $expConfig))

fileSize=${fileSizeConfig[1]}
fileCount=${fileCountConfig[1]}
blockSize=1000
dataDir="/mnt/extra/data2"

echo $dataDir
echo $scripts

count=0
count=$(ls -la $dataDir | grep -c "F")

if [ "$count" -ge "$fileCount" ]
then
	echo "Data files already exist. Skipping data generation."
	exit 0
fi
mkdir $dataDir
cd $scripts
nohup python genDummyFiles.py $blockSize $fileSize  0 $fileCount $dataDir &
while [ $count -ne $fileCount ]; do
	sleep 40
        count=$(ls -la $dataDir | grep -c "F")
	echo "Hang on!!  Still Working.."
done
