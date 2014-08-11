#!/bin/sh 

cp .config config_now
make modules
if [ $? -eq 0 ] ; then
echo "barnard" | sudo -S make modules_install INSTALL_MOD_PATH=/home/barnard/work/board_9G25/rootfs
echo "barnard" | sudo -S make modules_install INSTALL_MOD_PATH=/home/barnard/work/board_9G25/rootfs_rfid
fi

