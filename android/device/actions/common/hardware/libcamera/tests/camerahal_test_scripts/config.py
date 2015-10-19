from time import sleep
import random
import sys

# Imports the monkeyrunner modules used by this program
from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice



class coordinates:
     def __init__(self, pointX, pointY):
         self.x = pointX
         self.y = pointY

open_wait = 3
firsttime = 0
attempts = 0
iteration = 0

BlankKey = coordinates(0, 0)     #ok
CaptureKey = coordinates(1130, 367)     #ok

CaptureResolutionDownKey = coordinates(690, 404)    #ok
CaptureResolutionUpKey = coordinates(842, 404)      #ok
manueKey = coordinates(933, 674)   #ok
CamSettingsKey = coordinates(933, 690)  #ok
RestoreKey = coordinates(668, 472)      #ok
SitchCameraKey = coordinates(938, 60)   #ok
BackMenuKey = coordinates(936, 50)    #ok
DoubleTapZoomKey = coordinates(200, 500)
#SmoothZoomKey = 932,400              #ok
SmoothZoomIn = 933,134                #ok
SmoothZoomOut = 933,604               #ok
SmoothZoomStart = 933,565                #ok
SmoothZoomEnd = 933,172               #ok
CamcorderMode = coordinates(1136,684) #ok
ImageMode = coordinates(1068,684)     #ok

#White Balance 
AWBKey = coordinates(936,518)         #ok
AWBAutoKey = coordinates(574,253)     #ok
AWBFIncandescentKey = coordinates(574,327) #ok
AWBDaylightKey = coordinates(574,400)    #ok
AWBFluorescentKey = coordinates(574,473) #ok
AWBCloudydaylightKey = coordinates(574,547)   #ok


#Scene Mode
SceneKey = coordinates(933,605)          #ok
SceneAutoKey = coordinates(570,328)      #ok
SceneNightKey = coordinates(570,400)     #ok
SceneLandscapeKey = coordinates(470,474) #ok

#TimeLapseKey
TimeLapseKey = coordinates(933,605)          #ok
TimeLapseKey_off = coordinates(570,144)      #ok
TimeLapseKey_1s = coordinates(570,221)      #ok
TimeLapseKey_1p5s = coordinates(570,291)     #ok
TimeLapseKey_2s = coordinates(470,365) #ok
TimeLapseKey_2p5s = coordinates(470,439) #ok
TimeLapseKey_3s = coordinates(470,509) #ok
TimeLapseKey_5s = coordinates(470,582) #ok
TimeLapseKey_10s = coordinates(470,657) #ok


ConfermRestoreKey = coordinates(715, 400)        #ok

# sets the name of the component to start
runComponent = 'com.android.gallery3d/com.android.camera.Camera'
cameraComponent = 'com.android.gallery3d/com.android.camera.Camera'
videoCameraComponent = 'com.android.gallery3d/com.android.camera.VideoCamera'


#Supported PictureSize Num
PictureSizeNum = 4

ScnenKeyMaps = {
        "auto":SceneAutoKey,
        "night":SceneNightKey,
        "landscape":SceneLandscapeKey
        }

TimeLapseMaps = {
        "off":TimeLapseKey_off,
        "1s":TimeLapseKey_1s,
        "1.5s":TimeLapseKey_1p5s,
        "2s":TimeLapseKey_2s,
        "2.5s":TimeLapseKey_2p5s,
        "3s":TimeLapseKey_3s,
        "5s":TimeLapseKey_5s,
        "10s":TimeLapseKey_10s,
        }

AWBKeyMaps = {
        "auto":AWBAutoKey,
        "fIncandescent":AWBFIncandescentKey,
        "daylight":AWBDaylightKey,
        "fluorescent":AWBFluorescentKey,
        "cloudy-daylight":AWBCloudydaylightKey,
        }

print 'Connecting...'
# Connects to the current device, returning a MonkeyDevice object
device = MonkeyRunner.waitForConnection()
print 'Connecting Done.'

def SwitchCamera():
    sleep(1)
    print 'Switch Sensor'
    device.touch( SitchCameraKey.x, SitchCameraKey.y, 'DOWN_AND_UP')
    sleep(3)

def ZoomIn():
    print'Zooming In...'
    device.drag(SmoothZoomStart,SmoothZoomEnd, 5, 10)
    sleep(1)

def ZoomOut():
    print'Zooming Out...'
    device.drag(SmoothZoomEnd,SmoothZoomStart, 5, 10)
    sleep(1)

def RestoreDefaults():
    device.touch( ImageMode.x, ImageMode.y, 'DOWN_AND_UP')
    sleep(2)
    OpenCameraSettings()
    sleep(1)
    print'Reset settings to Default'
    device.touch( RestoreKey.x, RestoreKey.y, 'DOWN_AND_UP')
    sleep(1)
    device.touch( ConfermRestoreKey.x, ConfermRestoreKey.y, 'DOWN_AND_UP')
    sleep(1)
    DoBack()

def OpenCameraSettings():
    device.touch( manueKey.x, manueKey.y, 'DOWN_AND_UP')
    sleep(1)
    print 'Open Camera Settings'
    device.touch( CamSettingsKey.x, CamSettingsKey.y, 'DOWN_AND_UP')
    sleep(1)

def OpenWhitebalanceSettings():
    device.touch( manueKey.x, manueKey.y, 'DOWN_AND_UP')
    sleep(1)
    print 'Open White balance'
    device.touch( AWBKey.x, AWBKey.y, 'DOWN_AND_UP')
    sleep(1)

def setWhitebalance(wb):
    print 'setWhitebalance %s' % wb;
    OpenWhitebalanceSettings();
    device.touch( AWBKeyMaps[wb].x, AWBKeyMaps[wb].y, 'DOWN_AND_UP')
    sleep(1)
    DoBack();

def OpenTimeLapseSettings():
    device.touch( manueKey.x, manueKey.y, 'DOWN_AND_UP')
    sleep(1)
    print 'Open Time Lapse'
    device.touch( TimeLapseKey.x, TimeLapseKey.y, 'DOWN_AND_UP')
    sleep(1)

def setTimeLapse(t):
    print 'setTimeLapse %s' % t;
    OpenTimeLapseSettings();
    device.touch( TimeLapseMaps[t].x, TimeLapseMaps[t].y, 'DOWN_AND_UP')
    sleep(1)
    DoBack();

def DoCapture():
    print 'DoCapture ...'
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    sleep(2)

def DoBack():
    print'Backkey...'
    device.touch( BackMenuKey.x, BackMenuKey.y, 'DOWN_AND_UP')
    sleep(1)


def DragMenuDown():
    print 'Drag the manu Down.'
    #device.drag( DragPoinStart, DragPointStop, 1, 10)
    sleep(1)

def DragMenuUp():
    print 'Drag the manu Up.'
    #device.drag(DragPointStop, DragPoinStart, 1, 10)
    sleep(1)   

def wait(wait_value):  
    for i in range(wait_value + 1):
        x = wait_value - i        
        print '\rWaiting: %s seconds  '%x,
        sys.stdout.flush()
        sleep(1)
    print

def OpenSceneMode():
    device.touch( manueKey.x, manueKey.y, 'DOWN_AND_UP')
    sleep(1)
    device.touch( SceneKey.x, SceneKey.y, 'DOWN_AND_UP')
    sleep(1)

def SetSceneMode(mode):
    print 'set scenemode %s'% mode
    OpenSceneMode()
    device.touch( ScnenKeyMaps[mode].x, ScnenKeyMaps[mode].y, 'DOWN_AND_UP')
    sleep(1)
    DoBack();

def OpenFlashMode():
    device.touch( manueKey.x, manueKey.y, 'DOWN_AND_UP')
    sleep(1)
    #device.touch( FlashMenuKey.x, FlashMenuKey.y, 'DOWN_AND_UP')
    sleep(1)    

def StartRecording():
    print'Start recording'
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    sleep(3)

def StopRecording():
    print'Stop recording'
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    sleep(3)

def exit():
    print 'Exit application.'
    device.press('KEYCODE_HOME', 'DOWN_AND_UP')
    sleep(1)

def takeSnapshot(path):
    image = device.takeSnapshot ();
    image.writeToFile(path,'png');



    







