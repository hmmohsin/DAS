#!/bin/bash

if [ $# -ne 3 ]; then
	print "Usage: ./hadoopSetup.sh start-id end-id"
else
	path=$1"/scripts"
	sid=$2
	lid=$3
	while [ $sid -le $lid ]; do
		ip="10.1.1."$sid
                ssh -o StrictHostKeyChecking=no -n -f $ip "cd $path; ./cleanup.sh $ip"
		let sid=sid+1
	done
fi
