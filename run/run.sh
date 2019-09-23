#!/bin/bash

fileCount=1000
startIPOct=2
endIPOct=12
path=$(dirname $(pwd))
echo $path
../scripts/./run_disk_mount.sh $path $startIPOct $endIPOct
../scripts/./run_cleanup.sh $path $startIPOct $endIPOct
../scripts/./run_datanodeSetup.sh $path $startIPOct $endIPOct

sid=1
lid=10
while [ $sid -le $lid ]; do
	ip="S-"$sid
        ssh -o StrictHostKeyChecking=no $ip "exit"
        let sid=sid+1
done
ssh -o StrictHostKeyChecking=no hadoopMaster "exit"
ssh -o StrictHostKeyChecking=no 0.0.0.0 "exit"


hadoop namenode -format
start-dfs.sh

../scripts/./run_genDummyFiles.sh $path $fileCount
../scripts/./run_uploadHDFS.sh $fileCount
../scripts/./run_enable_storage_dstage.sh $path $startIP $endIPOct
../scripts/./run_enable_net_dstage.sh $path $startIPOct $endIPOct
../scripts/./run_proxy.sh $path $startIPOct $endIPOct
../scripts/./run_dn_start.sh $path $startIPOct $endIPOct
