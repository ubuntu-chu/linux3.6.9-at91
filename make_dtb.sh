#!/bin/sh

make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- dtbs
echo "cp arch/arm/boot/at91sam9g25ek.dtb /opt/tftpboot/at91sam9g25ek.dtb"
cp arch/arm/boot/at91sam9g25ek.dtb /opt/tftpboot/at91sam9g25ek.dtb
echo "cp arch/arm/boot/at91sam9g25ek.dtb /mnt/hgfs/windown-linux/board_9g25/weishui_master_images"
cp arch/arm/boot/at91sam9g25ek.dtb /mnt/hgfs/windown-linux/board_9g25/weishui_master_images


echo "cp arch/arm/boot/at91sam9g25ek.dtb /mnt/hgfs/windown-linux/board_9g25/rfid_comm_manager_images"
cp arch/arm/boot/at91sam9g25ek.dtb /mnt/hgfs/windown-linux/board_9g25/rfid_comm_manager_images


