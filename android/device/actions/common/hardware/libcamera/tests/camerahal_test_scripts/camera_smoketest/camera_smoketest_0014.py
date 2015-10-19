
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


############################################################
print '*******************************************************************'
print 'Take pictures quickly'
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

    wait(2)

    for j in range(PictureSizeNum):
        for i in range(TestIterTimes):
            print 'takePicture No.%d'%i
            device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
            time.sleep(0.5)
        
        time.sleep(3)
        OpenCameraSettings()
        device.touch( CaptureResolutionUpKey.x, CaptureResolutionUpKey.y, 'DOWN_AND_UP')
        time.sleep(1)
        DoBack()
        time.sleep(1)

    SwitchCamera()

wait(2)
exit()      
