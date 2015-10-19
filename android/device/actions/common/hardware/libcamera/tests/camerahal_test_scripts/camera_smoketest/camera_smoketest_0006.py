
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


##########################################################
print '*******************************************************************'
print "TakePicture with different size"
print '*******************************************************************'

device.startActivity(component=runComponent)
wait(open_wait)

for s in range(2):
    RestoreDefaults()
    OpenCameraSettings()
    for i in range(PictureSizeNum):
        device.touch( CaptureResolutionDownKey.x, CaptureResolutionDownKey.y, 'DOWN_AND_UP')
        time.sleep(1)
    DoBack()

    device.touch( ImageMode.x, ImageMode.y, 'DOWN_AND_UP')
    wait(3)
    for i in range(TestIterTimes):
        for j in range(PictureSizeNum):
            DoCapture()
            OpenCameraSettings()
            if(i%2==0):
                device.touch( CaptureResolutionUpKey.x, CaptureResolutionUpKey.y, 'DOWN_AND_UP')
            else:
                device.touch( CaptureResolutionDownKey.x, CaptureResolutionDownKey.y, 'DOWN_AND_UP')
                
            time.sleep(1)
            DoBack()
            time.sleep(1)

    SwitchCamera()

wait(2)
exit()

