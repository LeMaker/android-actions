#!/bin/bash

# This is a sample of the command line make used to build
#   the libraries and binaries for the Pandaboard.
# Please customize this path to match the location of your
#   Android source tree. Other variables may also need to
#   be customized such as:
#     $CROSS, $PRODUCT, $KERNEL_ROOT


export ANDROID_BASE=/home/local/ACTIONS/zhouqijia/5001

make -C software/build/android \
	VERBOSE=0 \
	TARGET=android \
	ANDROID_ROOT=${ANDROID_BASE}/android \
	KERNEL_ROOT=${ANDROID_BASE}/android/kernel \
	CROSS=${ANDROID_BASE}/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android- \
	PRODUCT=gs900a \
	OUT=out_atm9009_evb \
	MPL_LIB_NAME=mplmpu \
	echo_in_colors=echo \
	-f shared.mk 



