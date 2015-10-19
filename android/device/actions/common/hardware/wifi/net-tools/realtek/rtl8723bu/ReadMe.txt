Auto installation step:
1. Execute "Install.bat" for MS-Windows.
2. Execute "Install.sh" for Linux.

Manual installation steps:
1. adb root
2. adb remount
3. adb push rtwpriv /system/bin/
4. adb shell chmod 755 /system/bin/rtwpriv 
6. adb install -r RtkWiFiTest.apk

P.S. 
1. RtkWiFiTest.apk supports both MP test, CTA test, and CMCC test functions.
After installed, you will see a "WiFi Test" App on the phone.
