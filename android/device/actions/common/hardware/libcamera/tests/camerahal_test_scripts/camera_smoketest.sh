#!/bin/sh

#change the lcd desity to 213
echo "change lcd density to 213"

lcddensity=`adb shell "busybox grep -e 'ro\.sf\.lcd_density=213' /system/build.prop"`
echo $lcddensity
if [ -z $lcddensity ];then
    adb root
    sleep 5s
    adb remount


    adb shell "busybox sed -i 's/ro.sf.lcd_density=[0-9]\+/ro.sf.lcd_density=213/g' /system/build.prop"
    sleep 5s
    adb shell sync

    adb shell "mount -o ro,remount /system"

    adb shell sync

    echo "ready to reboot device"
    sleep 5s
    echo "reboot device"
    adb shell reboot

    echo "sleep 5s"
    sleep 5s

    echo "wait-for-device"
    adb wait-for-device


    echo "sleep 30s"
    sleep 30s
fi

echo "delete all files in /sdcard/DCIM/Camera/"
adb shell rm -r /sdcard/DCIM/Camera/*

echo "unlock the keyguard"
adb shell input keyevent 82


echo "run camera_smoketest.py"
./mk_scriptrun.sh "camera_smoketest_run.py" $@

