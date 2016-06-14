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

elif [ "$BUILD_MODULE" = "sd_boot" ]; then
        cd $TOP_DIR/owl
        make sd_boot

elif [ "$BUILD_MODULE" = "user" ]; then
	cd $TOP_DIR/owl
	#cp config-user.sh config-mode.sh
	cat config-user.sh >> config-mode.sh
	./config-mode.sh
	exit

elif [ "$BUILD_MODULE" = "userdebug" ]; then
	cd $TOP_DIR/owl
	#cp config-userdebug.sh config-mode.sh
	cat config-userdebug.sh >> config-mode.sh
	./config-mode.sh
	exit

elif [ "$BUILD_MODULE" = "eng" ]; then
	cd $TOP_DIR/owl
	#cp config-eng.sh config-mode.sh
	cat config-eng.sh >> config-mode.sh
	./config-mode.sh
	exit

else
	cd $TOP_DIR/owl
	make
fi
