#!/bin/bash

if [ $# -ne 3 ]; then
	print "Error: Missing arguments.."
else
	path=$1"/scripts"
	sid=$2
	lid=$3
	while [ $sid -le $lid ]; do
		ip="10.1.1."$sid
		echo $ip
		ssh -o StrictHostKeyChecking=no -n -f $ip "sh -c 'cd $path; ./enable_net_dstage -s $ip -i $sid'"
		
		let sid=sid+1
	done
fi