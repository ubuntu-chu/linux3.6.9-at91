#!/bin/sh

cp .config config_now
make -j2 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- uImage
echo "cp arch/arm/boot/uImage /opt/tftpboot/uImage"
cp arch/arm/boot/uImage /opt/tftpboot/uImage
echo "cp arch/arm/boot/uImage /mnt/hgfs/windown-linux/board_9g25/weishui_master_images"
cp arch/arm/boot/uImage /mnt/hgfs/windown-linux/board_9g25/weishui_master_images

echo "cp arch/arm/boot/uImage /mnt/hgfs/windown-linux/board_9g25/rfid_comm_manager_images"
cp arch/arm/boot/uImage /mnt/hgfs/windown-linux/board_9g25/rfid_comm_manager_images



