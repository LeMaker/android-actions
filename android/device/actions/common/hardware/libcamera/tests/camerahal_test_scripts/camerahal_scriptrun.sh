#!/bin/bash

script_path=`pwd`
echo $1
cat L_CAMERAHAL_$1.txt

adb remount
#make it just in case
adb shell mkdir /system/bin/ch/ > /dev/null
#push our script there
adb push L_CAMERAHAL_$1.txt /system/bin/ch/
#echo "$?"

if [ "$?" != 0 ]; then 
printf "L_CAMERAHAL_$1.txt is n/a \n\n\n\n"
exit 1
else 
printf "L_CAMERAHAL_$1.txt is pushed \n\n\n\n"
fi

#echo $1 adb shell mkdir sdcard ;
echo -e "\e]2;$1 is running!!\007\e]1;\007"

# make tmp dir localy and delete any raw lefovers
mkdir -p tmp  
rm  -f scriptrun*.txt 
rm -f tmp/*raw
rm -rf tmp/"L_CAMERAHAL_$1"/  

adb shell rm  -r /sdcard/* 
adb shell mkdir -p /sdcard/videos 

#start the scripts
adb shell "cd /system/bin/ch ; camera_test sn L_CAMERAHAL_$1.txt" > scriptrun$1.txt  
cat scriptrun$1.txt 

echo -e "\e]2;$1 is done ?!?\007\e]1;\007" 
adb shell sync 
mkdir tmp/"L_CAMERAHAL_$1" 
adb shell mv /sdcard/*raw /sdcard/L_CAMERAHAL_$1/ > /dev/null
adb shell mv /sdcard/*yuv /sdcard/L_CAMERAHAL_$1/ > /dev/null
adb pull /sdcard/L_CAMERAHAL_$1/ tmp/L_CAMERAHAL_$1/ > /dev/null
adb pull /sdcard/videos/ tmp/"L_CAMERAHAL_$1"/ 
mv scriptrun$1.txt  tmp/"L_CAMERAHAL_$1" 

## rename the picutres with the script name
cd tmp/"L_CAMERAHAL_$1"
for i in  img* ; do mv $i "$1_$i";  done
for i in  *3gp ; do mv $i "$1_$i"; done

echo "convert raw and yuv to bmp..."
#for i in  *raw ; do  python ${script_path}"/raw2bmp_byname.py" "$i"; done
#for i in  *yuv ; do  python ${script_path}"/raw2bmp_byname.py" "$i"; done
cd - 
#killall xterm
echo -e "\e]2;Terminal\007\e]1;\007"

##show us the pictures
ls tmp/"L_CAMERAHAL_$1"/*jpg

if [ "$?" == 0 ]; then 
    echo " "
#mirage tmp/"L_CAMERAHAL_$1"/*jpg &
fi

##show us the videos
ls tmp/"L_CAMERAHAL_$1"/*3gp

if [ "$?" == 0 ]; then 
    echo " "
#totem tmp/"L_CAMERAHAL_$1"/*3gp &
fi






