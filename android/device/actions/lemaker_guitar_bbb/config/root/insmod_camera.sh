#!/system/bin/sh

DETECT_MOD=sensor_detect;
DETECT_KO=/misc/modules/${DETECT_MOD}.ko;
DETECT_FRONT_FILE=/sys/front_camera/front_name;
DETECT_REAR_FILE=/sys/rear_camera/rear_name;
DETECT_FRONT_OFFSET=/sys/front_camera/front_offset;
DETECT_REAR_OFFSET=/sys/rear_camera/rear_offset;
DETECT_STATUS=/sys/rear_camera/status;
DETECT_SAVE=/data/${DETECT_MOD}.txt;
echo "insmod usb camera"
insmod /misc/modules/videobuf2-vmalloc.ko ;
insmod /misc/modules/uvcvideo.ko   video_device_node=3;

#####
## usage: 
##     insmod_camera.sh gl5203
######
if [ $# -ne 0 ]; then
    echo "asoc:" $1
    HOSTDRV=$1_camera.ko;
else
    HOSTDRV=owl_camera.ko
fi;
echo "HOSTDRV:" ${HOSTDRV}

echo "enter insmod-camera shell script.";

if [ -e ${DETECT_SAVE} ]; then
    SAVE_OFFSET=$(cat ${DETECT_SAVE});
    FRONT_SCAN_START=$(echo ${SAVE_OFFSET%,*});
    if [ ! ${FRONT_SCAN_START} ]; then
        FRONT_SCAN_START=0;
    fi;

    REAR_SCAN_START=$(echo ${SAVE_OFFSET#*,});
    if [ ! ${REAR_SCAN_START} ]; then
        REAR_SCAN_START=0;
    fi;
fi;

echo "now insmod ${DETECT_KO}";
insmod ${DETECT_KO} camera_front_start=${FRONT_SCAN_START} camera_rear_start=${REAR_SCAN_START};
if [ $? -ne 0 ]; then
    insmod ${DETECT_KO} camera_front_start=0 camera_rear_start=0;
fi;

until [ $(cat ${DETECT_STATUS}) -eq 1 ]; do
	sleep 3;
done;

CAMERA_FRONT_KO_NAME=$(cat ${DETECT_FRONT_FILE});
CAMERA_REAR_KO_NAME=$(cat ${DETECT_REAR_FILE});
echo "detect front $CAMERA_FRONT_KO_NAME, rear $CAMERA_REAR_KO_NAME";

CAMERA_KO_OFFSET=;
if [ ${#CAMERA_FRONT_KO_NAME} -gt 5 ] && [ $(echo ${CAMERA_FRONT_KO_NAME##*.})="ko" ]; then
    CAMERA_FRONT_EXIST=1;
    CAMERA_KO_OFFSET=$(cat ${DETECT_FRONT_OFFSET});
else
    CAMERA_FRONT_EXIST=0;
fi;

if [ ${#CAMERA_REAR_KO_NAME} -gt 5 ] && [ $(echo ${CAMERA_REAR_KO_NAME##*.})="ko" ]; then
    CAMERA_REAR_EXIST=1;
    CAMERA_KO_OFFSET="$CAMERA_KO_OFFSET"",""$(cat ${DETECT_REAR_OFFSET})";
else
    CAMERA_REAR_EXIST=0;
fi;

if [ ${CAMERA_FRONT_EXIST} -eq 1 ] && [ ${CAMERA_REAR_EXIST} -eq 1 ]; then
    if [ $(echo ${CAMERA_FRONT_KO_NAME%.*}) = $(echo ${CAMERA_REAR_KO_NAME%.*}) ]; then
        FULL_NAME="/misc/modules/"${CAMERA_FRONT_KO_NAME};
        echo "now insmod two same module ${CAMERA_FRONT_KO_NAME%.*}";
        insmod ${FULL_NAME} dual=1;
    else
        FULL_NAME="/misc/modules/"${CAMERA_REAR_KO_NAME};
        echo "now insmod rear module ${CAMERA_REAR_KO_NAME%.*}";
        insmod ${FULL_NAME} rear=1;
        FULL_NAME="/misc/modules/"${CAMERA_FRONT_KO_NAME};
        echo "now insmod front module ${CAMERA_FRONT_KO_NAME%.*}";
        insmod ${FULL_NAME} rear=0;
    fi;
else
    if [ ${CAMERA_REAR_EXIST} -eq 1 ]; then
        FULL_NAME="/misc/modules/"${CAMERA_REAR_KO_NAME};
        echo "now insmod single rear module ${CAMERA_REAR_KO_NAME%.*}";
        insmod ${FULL_NAME} rear=1;
    elif [ ${CAMERA_FRONT_EXIST} -eq 1 ]; then
        FULL_NAME="/misc/modules/"${CAMERA_FRONT_KO_NAME};
        echo "now insmod single front module ${CAMERA_FRONT_KO_NAME%.*}";
        insmod ${FULL_NAME} rear=0;
    else
        echo "cannot find camera device.";
    fi
fi;

FULL_NAME="/misc/modules/${HOSTDRV}";
echo "now insmod $FULL_NAME";
insmod ${FULL_NAME};

echo ${CAMERA_KO_OFFSET} > ${DETECT_SAVE};

#echo "now rmmod ${DETECT_MOD}";
#rmmod ${DETECT_MOD};

echo "exit insmod-camera shell script.";
