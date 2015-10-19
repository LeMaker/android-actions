**************************************************************************
**   InvenSense MotionApps Solution - README.TXT
**************************************************************************

This file briefly explains how to use the InvenSense MotionApps solution 
and examples.  The release is conform to the Android KitKat sensor format.

The MPL contains the code for controlling the InvenSense MPU series devices, 
including activating and managing built in motion processing features.
All of the application source code is in ANSI C and can be compiled in C 
or C++ environments.  
This code is designed to work with all InvenSense MPU devices.

*************************************************************************

Build instructions :

the shared makefiles build against the Android Framework structure but do not 
use the underlying NDK structure.

These makefiles expect to receive:
- the location of the Android framework root in the ANDROID_ROOT variable;
- the location of the kernel root folder in the KERNEL_ROOT variable;
- the location of the Android cross compiling toolchain, in the CROSS variable;
- the target platform of the build, in the PRODUCT variable;
- the target OS, in the TARGET variable;
- the name of the advanced, binary library, as MPL_LIB_NAME=mplmpu;
- an optional VERBOSE variable to enable verbose output from the build.

See the top-level Makefile for details: 
- shared.mk
- common.mk

Example on how to run the build processes:

MAKE_CMD=" \
  make \
    VERBOSE=0 \
    TARGET=android \
    CROSS=/Android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi- \
    ANDROID_ROOT=/Android/ \
    KERNEL_ROOT=/Android/kernel \
    PRODUCT=pandaboard \
    MPL_LIB_NAME=mplmpu \
"

${MAKE_CMD} -C build/android -f shared.mk
${MAKE_CMD} -C build/android -f shared.mk clean

The file BUILD.sh shows a reference of the make command used to build the 
libraries and sample applications.

