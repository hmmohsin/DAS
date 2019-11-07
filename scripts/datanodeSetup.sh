#!/bin/bash
home=$1
hadoopConfig="$home/CONFIG/hadoopConfig"
serversList="$home/CONFIG/servers.list"
clientsList="$home/CONFIG/clients.list"
slavesList="$hadoopConfig/slaves"
hdfsSites="$hadoopConfig/hdfs-site.xml"
expConfigFile="$home/CONFIG/exp.conf"
hdfsProxy="$home/HDFSProxy/config.txt"
metaConfig="$home/METADATA/gen_meta_config.txt"

echo $hadoopConfig

#setting up HDFS slaves list
> $slavesList
dnCount=0
while IFS= read -r line
do
        lineVar=($line)
        ip=${lineVar[1]}
        echo $ip >> $slavesList
        let dnCount=dnCount+1
done < "$serversList"
if [[ $dnCount -ge 3 ]]; then
        dnCount=3
fi

#setting up HDFS replication based on available number of data nodes
sed -i "/<name>dfs.replication<\/name>/!b;n;c<value>$dnCount<\/value>" "$hdfsSites"

#setting up first client as hadoop Namenode
line=$(head -n 1 $clientsList)
clientInfo=($line)
hadoopMaster=${clientInfo[1]}

#pushing updated hadoop config to config dir
sudo cp $hadoopConfig/*-site.xml /usr/local/hadoop/etc/hadoop
sudo cp $hadoopConfig/hadoop-env.sh /usr/local/hadoop/etc/hadoop
sudo cp $hadoopConfig/slaves /usr/local/hadoop/etc/hadoop/


#setting up hadoopMaster
sed -i '/HadoopMaster/d' /etc/hosts > /dev/null
echo "$hadoopMaster HadoopMaster" >> /etc/hosts
sed -i "s/nnIPAddr.*/nnIPAddr $hadoopMaster/" "$hdfsProxy"
sed -i "s/nnIPAddr.*/nnIPAddr $hadoopMaster/" "$metaConfig"


#creating hadoop Directories
sudo mkdir -p /mnt/extra/hadoop_store/hdfs/namenode
sudo mkdir -p /mnt/extra/hadoop_store/hdfs/datanode
