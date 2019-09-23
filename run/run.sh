#!/bin/bash

startIP=2
endIP=12
path=$(dirname $(pwd))
echo $path
#../scripts/./run_disk_mount.sh $path $startIP $endIP
#../scripts/./run_datanodeSetup.sh $path $startIP $endIP

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

../scripts/./run_genDummyFiles.sh $path
../scripts/./run_uploadHDFS.sh
../scripts/./run_enable_storage_dstage.sh $path $startIP $endIP
../scripts/./run_enable_net_dstage.sh $path $startIP $endIP
../scripts/./run_proxy.sh $path $startIP $endIP
../scripts/./run_dn_start.sh $path $startIP $endIP
