#!/bin/bash

echo "###### Executing ADB commands to get ROOT privilege ######"
adb root
adb remount

echo ""
echo "###### Pushing \"rtwpriv\" to DUT ######"
if [ -f rtwpriv ]; then
echo "Sending to \"/system/bin/\" ..."
adb push rtwpriv /system/bin/
adb shell chmod 755 /system/bin/rtwpriv
else
echo "\"rtwpriv\" is not found !!!"
exit 0
fi

echo ""
echo "##### Installing \"RtkWiFiTest.apk\" on DUT ######"
echo "Sending to DUT ..."
if [ -f RtkWiFiTest.apk ]; then
adb install -r RtkWiFiTest.apk
else
echo "\"RtkWiFiTest.apk\" is not found !!!"
exit 0
fi
