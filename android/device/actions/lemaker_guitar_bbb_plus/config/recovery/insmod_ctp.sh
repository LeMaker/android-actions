#!/sbin/sh

DETECT_MOD=ctp_detect
DETECT_KO=/lib/modules/${DETECT_MOD}.ko
DETECT_FILE=/sys/${DETECT_MOD}/name
DETECT_OFFSET=/sys/${DETECT_MOD}/offset
DETECT_SAVE=/data/${DETECT_MOD}.txt

echo "enter insmod-ctp shell script."

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
        #rmmod ${DETECT_MOD}
        #exit
fi
CTP_KO_NAME=($(cat ${DETECT_FILE}))
until [ ${#CTP_KO_NAME} -gt 3 -a $(echo ${CTP_KO_NAME##*.})="ko" ]; do
	sleep 1
    CTP_KO_NAME=($(cat ${DETECT_FILE}))
    echo "detect $CTP_KO_NAME"
done;

CTP_KO_NAME=($(cat ${DETECT_FILE}))
CTP_KO_OFFSET=($(cat ${DETECT_OFFSET}))

if [ ${#CTP_KO_NAME} -gt 3 ] && [ $(echo ${CTP_KO_NAME##*.})="ko" ]
then
		echo "now rmmod ${DETECT_MOD}"
		rmmod ${DETECT_MOD}
        FULL_NAME="/lib/modules/"${CTP_KO_NAME}
        echo "now insmod $FULL_NAME"
        insmod ${FULL_NAME}
        echo ${CTP_KO_OFFSET} > ${DETECT_SAVE}
else
        echo "cannot find right ctp device."
		echo "now rmmod ${DETECT_MOD}"
		rmmod ${DETECT_MOD}
fi

echo "exit insmod-ctp shell script."
