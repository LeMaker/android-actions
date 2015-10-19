import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'takePicture with different PictureSize'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
switchTimes = 50
OpenCameraSettings()
for i in range(PictureSizeNum):
    device.touch( CaptureResolutionDownKey.x, CaptureResolutionDownKey.y, 'DOWN_AND_UP')
    time.sleep(1)

DoBack()

for i in range(switchTimes):
    for j in range(PictureSizeNum):
        print 'picture size up No.(%d,%d)'% (i,j)
        DoCapture()
        OpenCameraSettings()
        device.touch( CaptureResolutionUpKey.x, CaptureResolutionUpKey.y, 'DOWN_AND_UP')
        time.sleep(1)
        DoBack()
        time.sleep(1)

    for j in range(PictureSizeNum):
        print 'picture size down No.(%d,%d)'% (i,j)
        DoCapture()
        OpenCameraSettings()
        device.touch( CaptureResolutionDownKey.x, CaptureResolutionDownKey.y, 'DOWN_AND_UP')
        time.sleep(1)
        DoBack()
        time.sleep(1)
wait(20)
exit()

#END OF FILE
