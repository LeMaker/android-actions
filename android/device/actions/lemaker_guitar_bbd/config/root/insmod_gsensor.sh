#!/system/bin/sh

DETECT_MOD=gsensor_detect
DETECT_KO=/misc/modules/${DETECT_MOD}.ko
DETECT_FILE=/sys/${DETECT_MOD}/name
DETECT_OFFSET=/sys/${DETECT_MOD}/offset
DETECT_SAVE=/data/${DETECT_MOD}.txt

echo "enter insmod-gsensor shell script."

SCAN_START=0

if [ -e ${DETECT_SAVE} ];then
 	SCAN_START=($(cat ${DETECT_SAVE}))
	echo "data is ready,SCAN_START is:${DETECT_KO}"
fi

echo "now insmod ${DETECT_KO}"
insmod ${DETECT_KO} scan_start=${SCAN_START}
if [ $? -ne 0 ]; then
 	insmod ${DETECT_KO} scan_start=0
fi

if [ -e ${DETECT_FILE} ];then
 	if [ -s ${DETECT_FILE} ];then
        	echo "found the file:${DETECT_FILE} .It isn't empty."
	else
		echo "found the file:${DETECT_FILE} .It is empty,so exit."
	fi
else
        echo "${DETECT_FILE} is not exist, so exit."
        rmmod ${DETECT_MOD}
        exit
fi

GSENSOR_KO_NAME=($(cat ${DETECT_FILE}))
GSENSOR_KO_OFFSET=($(cat ${DETECT_OFFSET}))

if [ ${#GSENSOR_KO_NAME} -gt 3 ] && [ $(echo ${GSENSOR_KO_NAME##*.})="ko" ]
then
        FULL_NAME="/misc/modules/"${GSENSOR_KO_NAME}
        echo "now insmod $FULL_NAME"
        insmod ${FULL_NAME}
        echo ${GSENSOR_KO_OFFSET} > ${DETECT_SAVE}
else
        echo "cannot find right gsensor device."
fi

echo "now rmmod ${DETECT_MOD}"
rmmod ${DETECT_MOD}

echo "exit insmod-gsensor shell script."
