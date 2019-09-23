#! /bin/bash
rm -rf /mnt/extra
echo -e 'o\nn\np\n1\n\n\nw' | fdisk /dev/sdb
mkfs.ext4 /dev/sdb1
mkdir /mnt/extra
mount /dev/sdb1 /mnt/extra
