#!/bin/sh

cp .config config_now
make -j2 ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- uImage
cp arch/arm/boot/uImage /opt/tftpboot/uImage


