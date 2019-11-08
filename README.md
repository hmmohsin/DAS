This repository contain DAS codebase for storage experiment. For emulab experiment, a public profile is setup which can be accessed using a valid emulab account. 

### Emulab-Setup-Configuration
To setup DAS on Emulab, use the emulab setup file *www.github.com/hmmohsin/DAS/scripts/emulab.ns*. This file can be modified to provision desired number and types of nodes as follow:\
**Cluster Size:**\
The default configuration file prvisions a 4 node cluster: 1 Client, and 3 Servers. You can change the number of clients and servers by changing *maxServers* and *maxClients* variables.\
\
**Cluster Operating System:**\
We have created an OS image *HM-Hadoop-Single* that contains hadoop setup so that you dont have to set it up on every node on your own.\
\
**Machine Types:**\
The default Configuration file will provision machines of type *pc3000* which are often abundantly available. However, you can change machine types for clients or servers or both by changing *tb-set-hardware $S($i)* to d430, d710 or d820. For our experiment results reported in paper, we used d430 machines.\
\
**Machine Availability:**\
Before attempting to swap in, we recommend checking resource availability. For that, on emulab classic portal, click on *Experimentation* and choose *Resource Availability*.\
\
**Link Capacities:**
By default all the machines will be connected with cluster via 1 Gbps link. We recommend 10 Gbps for client link. However, only d430 and d820 type machines have 10Gbps link support. Given that you have d430 or d820 type machines, you can update client link bandwidth by changing *tb-set-node-lan-bandwidth $C($i) $lan0 1Gb* to *tb-set-node-lan-bandwidth $C($i) $lan0 10Gb*.\
\
**Storage Capacities:**
Each server has 100GB storage block-store attached. For d430, and d710, this limit can be increased upto 1000GB.\


## Emulab Setup
1- Login to your [emulab account](https://www.emulab.net).\
2- From top left of the page, click on *Experiments* and choose *Emulab Classic* from the drop down list.\
3- Click *Experimentation* and choose *Begin an Experiment*.\
4- Fill the required fields, craete a local copy of *www.github.com/hmmohsin/DAS/scripts/emulab.ns*, upload the local .ns file, and click on *submit*. The swap-in will take few minutes. If swapin fails because of resource availability, Follow the *Emulab-Setup-Configuration* discussed above to modify number and/or type of nodes and try swapping in again.\
5- Once the experiment is successfully swapped-in, an email will be sent to your account containing Qualified names against all the nodes. Use Qualified name against C-1 to SSH the node.\
6- Change user to root *sudo su root*.\
7- Clone DAS code *git clone https://github.com/hmmohsin/DAS.git* under your emulab home directory (e.g. /users/peter/)\
8- *cd DAS* and *make*\
9- *cd run && ./run.sh* This script will setup hadoop cluster, generate dummy data files and upload that data into hdfs cluster. With defualt settings the script will generate 1000 files of 1MB each. This step may take upto 10 minutes. To change the number of files or size of files, check the *Experiment-Configuration* section.\
10- *cd ../METADATA && python gen_metaFile.py hdfs* This will fetch files metadata from HDFS namenode to local node, bypassing the need to contact namenode for each get request.


### Experiment-Configuration
To evaluate different duplication schemes under different settings, *CONFIG* directory contains *exp.conf* file which provides various configuration parameters. Most of these parameters take an integer input. The following config are some of the parameters supported to cover a range of experiments:\
\
**dn_count:[number of datanodes]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Specifies the total number of datanodes in Hadoop cluster. Once you choose the number of datanodes, you need to specify datanode information in *servers.list* under *CONFIG* directory. For each datanode you are required to provide *ID IPAddress Pri-Port Sec-Port*; where ID is a unique identifier. We recommend keeping the Pri-Port and Sec-Port to 5010 and 5011 respectively. The IP Addresses of datanodes (servers) can be checked from emulab interface by clicking on the node. For our setup, the datanode (server) identifiers are S-1, S-2 and so on and their respective local IP Addresses are 10.1.1.2, 10.1.1.3 and so on.\
\
**cli_count:[number of clients]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Total number of clients generating get requests. Similar to previous step, you will need to speicify client's information in *clients.list* under CONFIG dir. For now we have added support for single client. However, you may need to change client IP. You can check the local ip address of client by logging into C-1. For default configuration the client IP Address is 10.1.1.5 and you don't need to change it.\
\
**btlnk_bw:[Avg Bottleneck BW]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Avg bottleneck bandwidth is used in load calculation. For the scope of paper, the bottleneck can either be network or Disk. For disk, we benchmarked avg random read throughput offline. For d430 type nodes on emulab, the avh throughput is 40MBps.\
\
**pLoad:[Percent load]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Specifies the percent load to generate get requests. For Figure7 in the paper, *10 40 70* values corresponds to low, medium, and high loads.\
\
**scheme:[Scheme ID]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Scheme to evaluate. 0, 1, 3, 5, 7, and 9 represent *Single-Copy, Cloning, DAS, HEDGED, AppTO, Tied* respectively.\
\
**sim_time:[Time in Seconds]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Specifies the number of seconds single run of each experiment should run.\
\
**run_count:[Number of Runs]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;pecifies the number of runs for each experiment.\
\
**eid:[experiment name]**\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;The name of experiment.\
\
**hedged_wait:[95th percentile at low load]**\
Specifies the amount of delay duplicates needs to wait before getting dispatched. For emulab settings we benchmarked this number in the base case (e.g. Single-Copy) at low load.
## Sample Configuration
The default *exp.conf* file evaluates *Single-Copy, Cloning, DAS, Hedged, AppTO, and Tied at 10% load (e.g. low load). The duration of each run is set to 500 sec with three runs for each scheme.

------
## Running the Experiment
change current directory to bin and run the client. client takes client ID as input. 

*cd ../bin && ./client 1*

------
## Compiling Results
RESULTS/RES Directory contains all the result files for each run of every experiment. The filenames are structured based on exp.conf file as *eid_scheme_pLoad_run_cli_client-id*. For example from the sample config file, the DAS result filename will be *default_test_DAS_10_0_cli-1*.\
\
The structure of result file is as follow(Pri is Primary, Sec is Secondary):\
*job-id tasksPerJob |taskID objectID priReplicaID_SecReplicaID objectSizeFetchedFromPri_objectSizeFetchedFromSec RCTPri_RCTSec qLenBeforeDispPri-QlenBeforeDispSec*\
\
To get results summary you can use compile_res.py script in *scripts* dir. *cd ../scripts && python compile_res.py* This will update *RESULTS/summary.txt*. The summary.txt file is structured as follow:\
*Scheme RCT-std RCT-mean RCT-median RCT-p90 RCT-p95 RCT-p99 RCT-P99.9 RCT-Max*\
In our paper we have only reported p99 for our main experiment.

## Reproducing Paper Results
For our experiment in section 6.1, we used 12 nodes of type d430. You need 12 d430 type machines. Change *maxServers* to 10, and *maxClients* to 2, change *tb-set-hardware* for both server and clients from *pc3000* to *d430*, and change client links bandwidth to 10Gb as discussed above. 


## Caveats and Debugging
The emulab experiment profile (set-up for this experiment) assumes specific type of machine (d430), however 12xd430 may not be available at any time. We strongly recommend either *d430* or *d710* type machines for servers (s1-s9) and only *d430* for clients (c1-c2). To change the machine type edit the profile *(edit->edit topology)*, client on node, click on *hardware type*, and choose machine type from drop down list. With other types of machines you should be able to run the experiment but it may lead to different results.\
\
During your emulab setup phase, do not stop or kill the script. If somehoe you end up stopping the script before it finishes, clean it up and re-run it from start. To clean up use *./run-cleanup.sh* from *scripts* directory.
\
To validate that hadoop is setup properly and the data files have been uploaded to cluster, you can use *hadoop fs -ls /data | grep -c "F"*. This will show you the number of files uploaded into cluster under /data directory.\
\
Each dataonde runs two process to run hdfs proxy: *hdfs* and *server*. You can check if both these process are running on each datanode before starting experiment. You can start/restart these process using *./run_proxy.sh <path_to_DAS_HOME> 2 12* and *./run_dn_start.sh <path_to_DAS_HOME> 2 12*. The last two parameters are custom to this setup.\
