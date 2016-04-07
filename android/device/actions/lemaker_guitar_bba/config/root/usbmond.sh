#!/system/bin/sh

k=1 
usb_gandroid0_enable="/sys/class/android_usb/android0/enable"

insmod_device_driver()
{
	USB_PORT=$1
	echo "\n---insmod_device_driver: $USB_PORT $usb_port_dwc3 $usb_port_usb2---\n" > /dev/console 
	if [ "$USB_PORT" = "$usb_port_dwc3" ]; then
		echo "USB_B_IN" > /sys/monitor/usb_port/config/usb_con_msg
	elif [ "$USB_PORT" = "$usb_port_usb2" ]; then
		#$install /misc/modules/udc.ko
		#$install /misc/modules/g_android.ko
		echo "USB_B_IN" >  /dev/console 
	else
		echo "unknown port!!!!" > /dev/console 
	fi
}

rmmod_device_driver()
{
	USB_PORT=$1
	#echo "rmmod_device_driver: $USB_PORT $usb_port_dwc3 $usb_port_usb2" > /dev/console 
	if [ "$USB_PORT" = "$usb_port_dwc3" ]; then
		echo "USB_B_OUT" >  /sys/monitor/usb_port/config/usb_con_msg
	elif [ "$USB_PORT" = "$usb_port_usb2" ]; then
		#echo "udc uninstall!!!!" > /dev/console 
		echo "USB_B_OUT" >  /dev/console 
	else
		echo "unknown port!!!!" > /dev/console 
		
	fi
	
	
}

insmod_host_driver()
{
	USB_PORT=$1
	if [ "$USB_PORT" = "$usb_port_dwc3" ]; then
		echo "USB_A_IN" >  /sys/monitor/usb_port/config/usb_con_msg
	elif [ "$USB_PORT" = "$usb_port_usb2" ]; then
		#$install /misc/modules/aotg_hcd.ko
		echo 'a' >/proc/acts_hcd
	else
		echo "unknown port!!!!" > /dev/console 
	fi
}

rmmod_host_driver()
{
	USB_PORT=$1
	if [ "$USB_PORT" = "$usb_port_dwc3" ]; then
		echo "USB_A_OUT" >  /sys/monitor/usb_port/config/usb_con_msg
	elif [ "$USB_PORT" = "$usb_port_usb2" ]; then
		echo 'b' >/proc/acts_hcd
		#$uninstall aotg_hcd	
	else
		echo "unknown port!!!!" > /dev/console 
	fi
}


param1=$1
param2=$2
param3=$3
install=insmod 
uninstall=rmmod
setproperty=setprop

adb_exist=1

usb_port_dwc3=0
usb_port_usb2=1
i=1    

echo "usbmond shell: $param1 $param2 $3"

if [ x$param1 = xUSB_B_IN ];then
	echo "\n------usbmond in B_IN: $param1 $param2 $3---\n" > /dev/console 
	if [ "$param2" = "$adb_exist" ];then
	  echo "------adbd stop in B_IN" > /dev/console 
	  /system/bin/stop adbd
	  sleep 1
	fi

	i=1
	until [ ! "$i" -le 4 ] 
	do   
		#echo "------num is $i"  > /dev/console 
		i=$((i+1)) 
		if [ -z `busybox pgrep -x adbd` ]; then
			#echo "------adbd not exist" > /dev/console
			i=100;
		else
			#echo "------adbd still exist" > /dev/console    
	  		/system/bin/stop adbd
			sleep 1
		fi
	done 
	
	

	insmod_device_driver $3
	if [ "$param2" = "$adb_exist" ];then
	  echo "------adb start in B_IN" > /dev/console 
	  /system/bin/start adbd
	fi  
fi

if [ x$param1 = xUSB_B_OUT ];then
echo "\n-----xUSB_B_OUT $1 $2 $3----------\n" > /dev/console
	#echo "------usbmond B_OUT: $param1 $param2 $3" > /dev/console 
	if [ "$param2" = "$adb_exist" ];then
	  echo "------adb stop in B_OUT" > /dev/console 
	  /system/bin/stop adbd
	  sleep 1
	fi  

	i=1
	until [ ! "$i" -le 4 ]
	do   
		#echo "------out_num is $i"  > /dev/console 
		i=$((i+1)) 
		if [ -z `busybox pgrep -x adbd` ]; then
			#echo "------adbd not exist" > /dev/console    
			i=100
		else
			#echo "------adbd still exist" > /dev/console
	  		/system/bin/stop adbd
			sleep 1
		fi
	done 

	rmmod_device_driver $3
	
	
	if [ "$param2" = "$adb_exist" ];then
	  echo "------adb start in B_OUT" > /dev/console 
	  /system/bin/start adbd
	fi  
fi

if [ x$param1 = xUSB_A_IN ];then
echo "\n-----xUSB_A_IN $1 $2 $3----------\n" > /dev/console 
    if [ -z `busybox pgrep rild` ]; then
       echo "------start rild" > /dev/console
        /system/bin/start ril-daemon
        sleep 1
    fi
	insmod_host_driver $3
	
	if [ -z `busybox pgrep -x sh` ]; then
		echo "------start console" > /dev/console
		/system/bin/start console
	fi
fi

if [ x$param1 = xUSB_A_OUT ];then
      echo "\n-----xUSB_A_OUT $1 $2 $3----------\n" > /dev/console 
	rmmod_host_driver $3
fi
