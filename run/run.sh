#!/bin/bash

path=$(dirname $(pwd))
echo $path
../scripts/./run_disk_mount.sh $path
../scripts/./run_cleanup.sh $path
../scripts/./run_datanodeSetup.sh $path
../scripts/./run_check_conn.sh $path

hadoop namenode -format
start-dfs.sh
hadoop fs -mkdir /data
../scripts/./run_genDummyFiles.sh $path
../scripts/./run_uploadHDFS.sh $fileCount
../scripts/./run_enable_storage_dstage.sh $path
../scripts/./run_enable_net_dstage.sh $path
../scripts/./run_proxy.sh $path
../scripts/./run_dn_start.sh $path
