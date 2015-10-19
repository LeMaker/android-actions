#!/bin/bash

function check_directory()
{
    #tmp=`echo $1 | grep "device/actions$"`
    if [[ ! $current_dir =~ device/actions$ ]]; then
        echo "Sorry! must be in "ANDROID/device/actions" directory to execute the shell"
        exit
    fi

	return 0
}

function check_all_devices()
{
	local dev 
	local real_devices 
	for dev in ${all_devices[@]} ; do
		vendor_setup="$current_dir/$dev/vendorsetup.sh"
		android_product="$current_dir/$dev/AndroidProducts.mk"
		dev_mk="$current_dir/$dev/$dev.mk"
		if [ -f "$vendor_setup" -a -f "$android_product" -a -f "$dev_mk" ]; then
		    real_devices=(${real_devices[@]} $dev)
		fi
	done

    if [ ${#real_devices[@]} -le 0 ]; then
        echo "Sorry! no exist devices, you need to generate a device manully!"
        exit
    fi

	unset all_devices
	all_devices=(${real_devices[@]})

	return 0
}

function is_valid()
{
	local tmp
	for tmp in ${all_devices[@]}
	do
		if [ $tmp == $1 ]; then
			return 1
		fi
	done

	return 0
}

function select_src_device()
{
	local i=1
	local tmp
	for tmp in ${all_devices[@]}
	do
		echo "    $i.$tmp"
		i=$(($i + 1))
	done

	echo -n "Which board would you like to copy? "
	read answer

    if [ -z "$answer" ]; then
        src_device=$1
    elif (echo -n $answer | grep -q -e "^[0-9][0-9]*$") then
        if [ $answer -le ${#all_devices[@]} ]; then
            src_device=${all_devices[$(($answer - 1))]}
		else
			src_device=""
        fi
    else
        src_device=$answer
    fi

	return 0 
}

function check_src_device()
{
    local tag=0
    if [ "x$src_device" == "x" ]; then 
    	tag=1
    else
    	is_valid $src_device
    	if [ "$?" == "0" ]; then
    		echo -e "Sorry! [$src_device] does not exist!\n"	
    		tag=1
    	fi
    fi
    if [ $tag == 1 ]; then
    	select_src_device $1
    	if [ -z "$src_device" ]; then
    		echo "Sorry! Invalid choice!"	
    		exit
    	else
    		is_valid $src_device
    		if [ "$?" == "0" ]; then
    			echo "Sorry! [$src_device] does not exist!"	
    			exit
    		fi
    	fi
    fi

	return 0
}

function check_dst_device()
{
    if [ "x$dst_device" == "x" ]; then 
        echo -n "Please input the new device name [$1]: "
        read dst_device
        if [ "x$dst_device" == "x" ]; then 
            dst_device=$1
        fi
    fi
    if [ -d $dst_device ]; then
        echo "Sorry! [$dst_device] already exists!"	
        exit
    fi

	return 0
}

function create_dst_device()
{
    cp $1/$2 -ar $1/$3
    sed -i "s/$2/$3/g" `grep $2 -rl ./$3/` 
    mv $1/$3/$2.mk $1/$3/$3.mk
    echo "Create new device: [$3], from: [$2] successfully!"

	return 0
}


#====start from here ==============

current_dir=`pwd`
check_directory
	
all_devices=(`ls -l $current_dir | awk '/^d/ {print $NF}'`)
check_all_devices

DEFAULT_SRC_DEVICE=${all_devices[0]}
DEFAULT_DST_DEVICE="new_device"
src_device=$1
dst_device=$2

check_src_device $DEFAULT_SRC_DEVICE
check_dst_device $DEFAULT_DST_DEVICE
create_dst_device $current_dir $src_device $dst_device

