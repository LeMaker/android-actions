#!/bin/sh

#use wboot.sh  bootdev  diskdev

MBREC=/tmp/mbrec.bin
UBOOT=/tmp/uboot.bin
BOOTDEV=$1
DISKDEV=$2

handle_mmc() {
	
	if [ -f ${MBREC} ] 
	then
		echo "write ${MBREC}"
		dd if=${MBREC} of=${DISKDEV} bs=512 seek=4097
	fi

	if [ -f ${UBOOT} ] 
	then
		echo "write ${UBOOT}"
		dd if=${UBOOT} of=${DISKDEV} bs=1024 seek=3072
	fi
}
handle_spinor() {
	if [ -f ${MBREC} ] 
	then
		echo "write ${MBREC}"
		dd if=${MBREC} of=${DISKDEV} bs=512 seek=2
	fi

	if [ -f ${UBOOT} ] 
	then
		echo "write ${UBOOT}"
		dd if=${UBOOT} of=${DISKDEV} bs=1024 seek=224
	fi
}
handle_nandflash() {
	echo "not use"
}

echo  "bootdev = ${BOOTDEV}, disk= ${DISKDEV}"
case $BOOTDEV in
sd0 )
	handle_mmc
	;;
sd2 )
	handle_mmc
	;;
nand )
	handle_nandflash
	;;
nor )
	handle_spinor
	;;
* )
	echo "ignore, not boot write"
	;;
esac

