#!/bin/bash
ipAddr=$1
path=$2
#cd /users/hmmohsin/git/RANS/HDFSProxy/proxy
echo $path
cd $path
sudo screen -X -S hdfs quit
sudo screen -S hdfs -d -m python proxy.py $1
