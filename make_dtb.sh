#!/bin/sh

make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- dtbs
cp arch/arm/boot/at91sam9g25ek.dtb /opt/tftpboot/at91sam9g25ek.dtb

