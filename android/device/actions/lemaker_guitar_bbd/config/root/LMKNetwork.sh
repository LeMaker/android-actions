#!/system/bin/sh
if [ $1 == "eth0" ] || [ $1 == "eth1" ]; then

if [ -e /sys/class/net/eth1 ]; then
        if [ $1 == "eth1" ]; then
                netcfg $1 up
        fi
elif [ -e /sys/class/net/eth0 ]; then
        if [ $1 == "eth0" ]; then
                netcfg $1 up
        fi
else
        echo "Not find ethernet devices."
fi

if [ $2 == "start" ];then
	sleep 2
	netcfg $1 dhcp
else
	netcfg $1 down
fi

else
	if [ $1 == "hdmi" ];then
		echo 0 > /sys/class/switch/h2w/state
	else
		echo 2 > /sys/class/switch/h2w/state
	fi
fi

