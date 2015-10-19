@ECHO OFF

ECHO ###### Executing ADB commands to get ROOT privilege ######
adb root
adb remount

REM blank line
ECHO.	
ECHO ###### Pushing "rtwpriv" to DUT ######
IF EXIST rtwpriv (
ECHO Sending to "/system/bin/" ...
adb push rtwpriv /system/bin/
adb shell chmod 755 /system/bin/rtwpriv
) ELSE (
ECHO "rtwpriv" is not found !!!
GOTO done
)

REM blank line
ECHO.
ECHO ##### Installing "RtkWiFiTest_20140714A.apk" on DUT ######
IF EXIST RtkWiFiTest_20140714A.apk (
ECHO Sending to DUT ...
adb install -r RtkWiFiTest_20140714A.apk
) ELSE (
ECHO "RtkWiFiTest.apk" is not found !!!
GOTO done
)

:done
pause & exit