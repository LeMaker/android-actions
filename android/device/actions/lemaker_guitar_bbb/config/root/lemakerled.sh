#!/system/bin/sh

mount -o remount /system
chmod -R 0777 /system/test

while :;
do

if [ -e /mnt/media_rw/sdcard/Movies ]; then
	
	if [ -e /data/f2 ]; then
		rm -r /system/test
		rm /mnt/media_rw/sdcard/Movies/test.avi
		pm uninstall org.cocos2dx.FishingJoy2
		break
	elif [ -e /data/f0 ]; then
		cp /system/test/AgingTestPreferences.xml /mnt/media_rw/sdcard
		touch /data/f1
		rm /data/f0
		echo 255 > /sys/class/leds/blue:GPIOB31/brightness
		break;
	elif [ -e /data/f1 ]; then
		cp /system/test/AgingTestPreferences.xml /mnt/media_rw/sdcard
		rm /data/f1
		touch /data/f2
		echo 255 > /sys/class/leds/blue:GPIOB31/brightness
		break
	elif [ ! -e /data/f0 ]; then
		if [ -e /system/test/by.apk ]; then
                        pm install -r /system/test/by.apk
                        touch /data/f0
                fi
		cp /system/test/test.avi /mnt/media_rw/sdcard/Movies
		cp /system/test/AgingTestPreferences.xml /mnt/media_rw/sdcard
		echo 255 > /sys/class/leds/blue:GPIOB31/brightness
		break
	fi
	
fi

done
