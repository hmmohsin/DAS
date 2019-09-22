#!/bin/bash

path=$1"/../bin"
cd $path
sudo screen -X -S server quit
sudo screen -S server -d -m ./server $2 $3 $4 $5
