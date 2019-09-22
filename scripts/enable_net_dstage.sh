#!/bin/bash
ifname=$(sudo cat /var/emulab/boot/ifmap | awk '{split($0,a," "); print a[1]}')
sudo tc qdisc del dev "$ifname" root
sudo tc qdisc add dev "$ifname" root handle 1: htb default 10
sudo tc class add dev "$ifname" parent 1: classid 1:1 htb rate 1000mbit ceil 1000mbit
sudo tc class add dev "$ifname" parent 1:1 classid 1:10 htb rate 1000mbit ceil 1000mbit
sudo tc class add dev "$ifname" parent 1:1 classid 1:11 htb rate 10kbit ceil 1000mbit

sudo tc filter add dev "$ifname" protocol ip parent 1:0 prio 0 u32 match ip tos 4 0xff flowid 1:10
sudo tc filter add dev "$ifname" protocol ip parent 1:0 prio 1 u32 match ip tos 8 0xff flowid 1:11
