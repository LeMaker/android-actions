#!/system/bin/sh

if [ $1 == "bba" ]
then
	mount -o rw,remount /misc
    mount -o rw,remount /system
	cd  /misc
    rm kernel.dtb
    cp lemaker_guitar_bba.dtb  kernel.dtb
    cd /system
    cp build_bba_plus.prop  build.prop
    reboot
elif [ $1 == "bbb" ]
then
	mount -o rw,remount /misc
    mount -o rw,remount /system
	cd  /misc
    rm kernel.dtb
    cp lemaker_guitar_bbb.dtb  kernel.dtb
    cd /system
    cp build_bbb.prop  build.prop
    reboot
elif [ $1 == "bbbplus" ]
then
	mount -o rw,remount /misc
    mount -o rw,remount /system
	cd  /misc
    rm kernel.dtb
    cp lemaker_guitar_bbb_plus.dtb  kernel.dtb
    cd /system
    cp build_bbb_plus.prop  build.prop
    reboot
elif [ $1 == "bbc" ]
then
	mount -o rw,remount /misc
    mount -o rw,remount /system
	cd  /misc
    rm kernel.dtb
    cp lemaker_guitar_bbc.dtb  kernel.dtb
    cd /system
    cp build_bbc.prop  build.prop
    reboot
elif [ $1 == "bbd" ]
then
	mount -o rw,remount /misc
    mount -o rw,remount /system
	cd  /misc
    rm kernel.dtb
    cp lemaker_guitar_bbd.dtb  kernel.dtb
    cd /system
    cp build_bbd.prop  build.prop
    reboot
else
   echo "Board Select fail"
fi