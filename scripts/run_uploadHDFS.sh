#! /bin/bash

home=$1
expConfig="$home/CONFIG/exp.conf"
fileCountConfig=($(sed -n '/^file_data_set/p' $expConfig))
fileCount=${fileCountConfig[1]}
count=0
hadoop fs -rm -r /data
hadoop fs -mkdir /data
nohup hadoop fs -put /mnt/extra/data/F* /data &
while [[ $count -lt $fileCount ]]; do
        sleep 5
        count=$(hadoop fs -ls /data | grep -c "F")
        echo "Hang on!!  Uploading data to hadoop cluster.."
done
