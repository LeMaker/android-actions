#!/bin/bash
#


BUILD_MODULE=$1
TOP_DIR=$(cd "$(dirname "$0")"; pwd)


if [ "$BUILD_MODULE" = "config" ]; then
	cd $TOP_DIR/owl
	./config.sh
	exit
elif [ ! -f "$TOP_DIR/owl/.config" ]; then
	cd $TOP_DIR/owl
	./config.sh
fi

if [ "$BUILD_MODULE" = "kernel" ]; then
	cd $TOP_DIR/owl
	make kernel

elif [ "$BUILD_MODULE" = "modules" ]; then
	cd $TOP_DIR/owl
	make modules

elif [ "$BUILD_MODULE" = "u-boot" ]; then
	cd $TOP_DIR/owl
	make u-boot

elif [ "$BUILD_MODULE" = "rootfs" ]; then
	cd $TOP_DIR/owl
	make rootfs

else
	cd $TOP_DIR/owl
	make
fi
