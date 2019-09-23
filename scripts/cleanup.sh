#!/bin/bash
ip=$1
if [ "$ip" == "10.1.1.12" ]
then
	stop-all.sh
fi

sudo sed -i '/Hadoop/d' /etc/hosts > /dev/null
sudo rm -rf /mnt/extra/*
sudo mkdir -p /mnt/extra/hadoop_store/hdfs/namenode
sudo mkdir -p /mnt/extra/hadoop_store/hdfs/datanode
