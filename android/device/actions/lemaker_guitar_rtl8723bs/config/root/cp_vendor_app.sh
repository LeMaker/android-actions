#!/system/bin/sh
first_boot_file="/data/system/entropy.dat"
if [ -f ${first_boot_file} ];then
    echo "not first boot"
else
    echo "first boot"
    cd /vendor/app/app/
    for file in *.apk ; do
        busybox cp $file /data/app/$file.tmp
        busybox chown 1000 /data/app/$file.tmp
        busybox chgrp 1000 /data/app/$file.tmp
        busybox chmod 644 /data/app/$file.tmp
        busybox mv /data/app/$file.tmp /data/app/$file
    done
fi