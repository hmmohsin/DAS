#!/bin/bash
sudo echo cfq > /sys/block/sdb/queue/scheduler
