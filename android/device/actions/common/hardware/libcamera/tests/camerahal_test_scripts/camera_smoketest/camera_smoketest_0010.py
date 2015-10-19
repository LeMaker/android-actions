
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


############################################################
print '*******************************************************************'
print 'TakePicture with zoomin/ZoomOut/PicturSize '
print '*******************************************************************'

device.startActivity(component=runComponent)
wait(open_wait)

for s in range(2):
    RestoreDefaults()
    wait(2)
    OpenCameraSettings()
    for i in range(PictureSizeNum):
        device.touch( CaptureResolutionDownKey.x, CaptureResolutionDownKey.y, 'DOWN_AND_UP')
        time.sleep(1)
    DoBack()

    for size in range(PictureSizeNum):
        for z in range(SmoothZoomEnd[1],SmoothZoomStart[1])[::-60]:
            device.touch( SmoothZoomStart[0], z, 'DOWN_AND_UP')
            time.sleep(1);
            DoCapture();
            time.sleep(1);

        for z in range(SmoothZoomEnd[1],SmoothZoomStart[1])[::60]:
            device.touch( SmoothZoomStart[0], z, 'DOWN_AND_UP')
            time.sleep(1);
            DoCapture();
            time.sleep(1);

        OpenCameraSettings()
        device.touch( CaptureResolutionUpKey.x, CaptureResolutionUpKey.y, 'DOWN_AND_UP')
        time.sleep(1)
        DoBack()
        time.sleep(1)

    SwitchCamera()
    
wait(2)
exit()


