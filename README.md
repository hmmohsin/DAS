This repository contain DAS codebase for storage experiment. For emulab experiment, a public profile is setup which can be accessed using a valid emulab account. 

## Emulab Setup
To simplify DAS setup on Emulab, we have created a 12 node Hadoop setup profile which contains all the required packages. Following are the steps to get the emulab setup.

1- Login to your [emulab account](https://www.emulab.net).
2- Instantiate [experiment profile](https://www.emulab.net/portal/instantiate.php?profile=Hadoop-Single-New&project=TuftsCC&version=7) by choosing **next**, **next**, and then **finish**. The swap-in will take few seconds.
3- Once the experiment is ready, click on the *List View*. This will show the list of nodes provisioned for this experiment. 
4- Use ssh command information and your favourit ssh client to login to node with ID *C-1*
5- clone DAS code *git clone https://github.com/hmmohsin/DAS.git*
6- *cd DAS* and *make*
7- *cd run && ./run.sh* This script will setup hadoop cluster, generate dummy data and upload that data into hdfs cluster. This step will take upto 20 minutes, make sure not to interrupt the process. 
8- *cd ../METADATA && python gen_metaFile.py hdfs* This will fetch hdfs metadata to local node, bypassing the need to contact namenode for each get request. 
------------
### Configuration
To evaluate different duplication schemes under different settings, *CONFIG* directory contains *exp.conf* file which provides various configuration parameters. Most of these parameters take an integer input. The following config are some of the parameters supported to cover a range of experiments:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;dn_count:[number of datanodes]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Specifies the total number of datanodes in Hadoop cluster. For section 6.1, this number is set to 9.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;cli_count:[number of clients]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Total number of clients generating get requests. For section 6.1 in paper, this value is set to 1.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;btlnk_bw:[Avg Bottleneck BW]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Avg bottleneck bandwidth is used in load calculation. For the scope of paper, the bottleneck can either be network or Disk. For disk, we benchmarked avg random read throughput offline. For d430 type nodes on emulab, the avh throughput is 40MBps.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;pLoad:[Percent load]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Specifies the percent load to generate get requests. For Figure7 in the paper, *10 40 70* values corresponds to low, medium, and high loads.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;scheme:[Scheme ID]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Scheme to evaluate. 0, 1, 3, 5, 7, and 9 represent *Single-Copy, Cloning, DAS, HEDGED, AppTO, Tied* respectively.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;sim_time:[Time in Seconds]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Specifies the number of seconds single run of each experiment should run.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;run_count:[Number of Runs]: 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Specifies the number of runs for each experiment.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;eid:[experiment name]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; The name of experiment.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;hedged_wait:[95th percentile at low load]
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Specifies the amount of delay duplicates needs to wait before getting dispatched. For emulab settings we benchmarked this number in the base case (e.g. Single-Copy) at low load.
The default *exp.conf* file evaluates *Single-Copy, Cloning, DAS, Hedged, AppTO, and Tied at 10% load (e.g. low load). The duration of each run is set to 500 sec with three runs for each scheme.

-------
## Sample Configuration
The default *exp.conf* file evaluates *Single-Copy, Cloning, DAS, Hedged, AppTO, and Tied at 10% load (e.g. low load). The duration of each run is set to 500 sec with three runs for each scheme.

------
## Running the Experiment
change current directory to bin and run the client. client takes client ID as input. 
*cd ../bin && ./client 1*

------
## Compiling Results

