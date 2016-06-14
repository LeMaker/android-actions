#!/bin/bash -e
#
# (C) Copyright 2015, actions Limited
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.

echo  "change to build userdebug mode for android code, please clean andoid/out and owl/out, or rename them to out-user"
if [ "$BOARD_NAME" =  "lemaker_guitar_bbb" ];then
		echo "lemaker_guitar_bbb-------------"
		cp ../android/device/actions/lemaker_guitar_bbb/config/recovery/init-userdebug.rc  ../android/device/actions/lemaker_guitar_bbb/config/recovery/init.rc
		
		cp ../android/device/actions/lemaker_guitar_bbb/device-userdebug.mk  ../android/device/actions/lemaker_guitar_bbb/device.mk
		
		cp ../android/device/actions/lemaker_guitar_bbb/init-userdebug.rc ../android/device/actions/lemaker_guitar_bbb/init.rc
		
		cp ../android/device/actions/lemaker_guitar_bbb/system-userdebug.prop ../android/device/actions/lemaker_guitar_bbb/system.prop
		
		#cp ../android/system/extras/su/Android-userdebug.mk   ../android/system/extras/su/Android.mk
		
		#cp ../android/device/actions/common/prebuilt/apk/thirdparty/superuser/Android-userdebug.mk  ../android/device/actions/common/prebuilt/apk/thirdparty/superuser/Android.mk
		
		#cp ../kernel/arch/arm/boot/dts/lemaker_guitar_bbb-userdebug.dts  ../kernel/arch/arm/boot/dts/lemaker_guitar_bbb.dts
		
		cp ../owl/s500/boards/android/lemaker_guitar_bbb/os-userdebug.mk   ../owl/s500/boards/android/lemaker_guitar_bbb/os.mk 
		
		cp  ../owl/s500/boards/android/lemaker_guitar_bbb/uenv-userdebug.txt  ../owl/s500/boards/android/lemaker_guitar_bbb/uenv.txt
		
		mv ../android/out  ../android/out-user
		
		mv ../owl/out  ../owl/out-user

elif [ "lemaker_guitar_bbb_plus" = "$BOARD_NAME" ];then 
		echo "lemaker_guitar_bbb_plus-------------"
		cp ../android/device/actions/lemaker_guitar_bbb_plus/config/recovery/init-userdebug.rc  ../android/device/actions/lemaker_guitar_bbb_plus/config/recovery/init.rc
		
		cp ../android/device/actions/lemaker_guitar_bbb_plus/device-userdebug.mk  ../android/device/actions/lemaker_guitar_bbb_plus/device.mk
		
		cp ../android/device/actions/lemaker_guitar_bbb_plus/init-userdebug.rc ../android/device/actions/lemaker_guitar_bbb_plus/init.rc
		
		cp ../android/device/actions/lemaker_guitar_bbb_plus/system-userdebug.prop ../android/device/actions/lemaker_guitar_bbb_plus/system.prop
		
		#cp ../android/system/extras/su/Android-userdebug.mk   ../android/system/extras/su/Android.mk
		
		#cp ../android/device/actions/common/prebuilt/apk/thirdparty/superuser/Android-userdebug.mk  ../android/device/actions/common/prebuilt/apk/thirdparty/superuser/Android.mk
		
		#cp ../kernel/arch/arm/boot/dts/lemaker_guitar_bbb_plus-userdebug.dts  ../kernel/arch/arm/boot/dts/lemaker_guitar_bbb_plus.dts
		
		cp ../owl/s500/boards/android/lemaker_guitar_bbb_plus/os-userdebug.mk   ../owl/s500/boards/android/lemaker_guitar_bbb_plus/os.mk 
		
		cp  ../owl/s500/boards/android/lemaker_guitar_bbb_plus/uenv-userdebug.txt  ../owl/s500/boards/android/lemaker_guitar_bbb_plus/uenv.txt
		
		mv ../android/out  ../android/out-user
		
		mv ../owl/out  ../owl/out-user		
fi

