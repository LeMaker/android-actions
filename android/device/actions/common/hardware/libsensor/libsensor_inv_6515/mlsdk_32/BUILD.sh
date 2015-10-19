#!/bin/bash

# This is a sample of the command line make used to build
#   the libraries and binaries for the Pandaboard.
# Please customize this path to match the location of your
#   Android source tree. Other variables may also need to
#   be customized such as:
#     $CROSS, $PRODUCT, $KERNEL_ROOT


export ANDROID_BASE=/svdisk/zhangyanzhong/workspace/GS705A_0909

make -C software/build/android \
	VERBOSE=0 \
	TARGET=android \
	ANDROID_ROOT=${ANDROID_BASE}/android \
	KERNEL_ROOT=${ANDROID_BASE}/android/kernel \
	CROSS=${ANDROID_BASE}/android/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.7/bin/arm-linux-androideabi- \
	PRODUCT=gs705a \
	OUT=out_atm7059tc_hr820ac \
	MPL_LIB_NAME=mplmpu \
	echo_in_colors=echo \
	-f shared.mk



