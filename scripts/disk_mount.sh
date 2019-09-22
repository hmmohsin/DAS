#! /bin/bash

if [ $1 == "10.1.1.12" ]; then
	echo "Hello"
	mkdir /mnt/data
	mkfs.ext4 /dev/sda4
	mount /dev/sda4 /mnt/data

else
	rm -rf /mnt/extra
	echo -e 'o\nn\np\n1\n\n\nw' | fdisk /dev/sdb
	mkfs.ext4 /dev/sdb1
	mkdir /mnt/extra
	mount /dev/sdb1 /mnt/extra
fi
