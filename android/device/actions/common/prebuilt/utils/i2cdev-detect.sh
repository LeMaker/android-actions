#!/system/bin/sh
TP_MODS=`busybox sed -n "1p" /misc/tplist`
TP_DRIVER=`busybox sed -n "2p" /misc/tplist`
GSENSOR_MODS=`busybox sed -n "1p" /misc/gsensorlist`
GSENSOR_DRI=`busybox sed -n "2p" /misc/gsensorlist`

I2C_DEVICE_PATH=/sys/bus/i2c/devices
SYS_INPUT=/sys/class/input
MODULES_PATH=/misc/modules/
TP_CONFIG_FILE=/data/tp-load
GSENSOR_CONFIG_FILE=/data/gsensor-load


detect_tp() {
    echo "detect tp modules"
    rm ${TP_CONFIG_FILE}
    config_cnt=1
    forcnta=0
    for module in ${TP_MODS}
    do
	echo "Trying insmod ${module}......"
	forcnta=`busybox expr $forcnta + 1`
        i2c_cnt_before=`ls -l ${I2C_DEVICE_PATH} | grep -v "^d" | busybox wc -l`
        input_cnt_before=`ls -l ${SYS_INPUT} | grep -v "^d" | busybox wc -l`
        insmod ${MODULES_PATH}${module}
        i2c_cnt_after=`ls -l ${I2C_DEVICE_PATH}| grep -v "^d" | busybox wc -l`
        input_cnt_after=`ls -l ${SYS_INPUT}| grep -v "^d" | busybox wc -l`
        if [ "$i2c_cnt_before" -ne "$i2c_cnt_after" ] && [ "$input_cnt_before" -ne "$input_cnt_after" ]
        then
            echo "module ${module} found"
            echo ${module} >> ${TP_CONFIG_FILE}
			break
		else
            echo "module ${module} not found"
			forcntb=0
			for driname in ${TP_DRIVER}
			do
			forcntb=`busybox expr $forcntb+1`
				if [ $forcnta -eq $forcntb ]; then	
							#echo not found ${driname},trying rmmod ${driname}...
							rmmod ${driname}
				fi
			done
        fi
    done    
}

detect_gsensor() {
    echo "detect gsensor modules"
    rm ${GSENSOR_CONFIG_FILE}
    config_cnt=1
    forcnta=0
    for module in ${GSENSOR_MODS}
    do
	echo "Trying insmod ${module}......"
	forcnta=`busybox expr $forcnta + 1`
        i2c_cnt_before=`ls -l ${I2C_DEVICE_PATH} | grep -v "^d" | busybox wc -l`
        input_cnt_before=`ls -l ${SYS_INPUT} | grep -v "^d" | busybox wc -l`
        insmod ${MODULES_PATH}${module}
        i2c_cnt_after=`ls -l ${I2C_DEVICE_PATH}| grep -v "^d" | busybox wc -l`
        input_cnt_after=`ls -l ${SYS_INPUT}| grep -v "^d" | busybox wc -l`
        if [ "$i2c_cnt_before" -ne "$i2c_cnt_after" ] && [ "$input_cnt_before" -ne "$input_cnt_after" ]
        then
            echo "module ${module} found"
            echo ${module} >> ${GSENSOR_CONFIG_FILE}
			break
        else
            echo "module ${module} not found"
			forcntb=0
			for driname in ${GSENSOR_DRI}
			do
			forcntb=`busybox expr $forcntb+1`
				if [ $forcnta -eq $forcntb ]; then	
							#echo not found ${driname},trying rmmod ${driname}...
							rmmod ${driname}
				fi
			done
        fi
    done    
}

load_tp_modules(){
    echo "load_tp_modules"
    if [ -f ${TP_CONFIG_FILE} ]
    then
        pcnt=`grep -Fc "" ${TP_CONFIG_FILE}`
		tmpcnt=0;
        mod=`busybox sed -n "${tmpcnt}p" ${TP_CONFIG_FILE}`
    	while [ $tmpcnt -lt $pcnt ]
    	do 
    		insmod ${MODULES_PATH}${mod}
    		tmpcnt=`busybox expr $tmpcnt + 1`
    		mod=`busybox sed -n "${tmpcnt}p" ${TP_CONFIG_FILE}`
    	done
	else
		echo Not found any tp module detected,will detect again...
		detect_tp
    fi
    return 1
}

load_gsensor_modules(){
    echo "load_gsensor_modules"
    if [ -f ${GSENSOR_CONFIG_FILE} ]
    then
        pcnt=`grep -Fc "" ${GSENSOR_CONFIG_FILE}`
		tmpcnt=0;
        mod=`busybox sed -n "${tmpcnt}p" ${GSENSOR_CONFIG_FILE}`
    	while [ $tmpcnt -lt $pcnt ]
    	do 
    		insmod ${MODULES_PATH}${mod}
    		tmpcnt=`busybox expr $tmpcnt + 1`
    		mod=`busybox sed -n "${tmpcnt}p" ${GSENSOR_CONFIG_FILE}`
    	done
	else
		echo Not found any gsensor module detected,will detect again...
		detect_gsensor
    fi
    return 1
}

load_gsensor_modules
load_tp_modules
