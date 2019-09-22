#!/bin/bash
path=$1"/CONFIG/hadoopConfig"
echo $path
sudo cp $path/*-site.xml /usr/local/hadoop/etc/hadoop
sudo cp $path/hadoop-env.sh /usr/local/hadoop/etc/hadoop
sudo cp $path/slaves /usr/local/hadoop/etc/hadoop/
echo "10.1.1.12 HadoopMaster" >> /etc/hosts
sudo mkdir -p /mnt/extra/hadoop_store/hdfs/namenode
sudo mkdir -p /mnt/extra/hadoop_store/hdfs/datanode
