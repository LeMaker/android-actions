import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'TakePicture with zoomin/ZoomOut/PicturSize '
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)

OpenCameraSettings()
for i in range(PictureSizeNum):
    device.touch( CaptureResolutionDownKey.x, CaptureResolutionDownKey.y, 'DOWN_AND_UP')
    time.sleep(1)

DoBack()

for size in range(PictureSizeNum):

    for z in range(SmoothZoomEnd[1],SmoothZoomStart[1])[::-15]:
        device.touch( SmoothZoomStart[0], z, 'DOWN_AND_UP')
        time.sleep(1);
        DoCapture();
        time.sleep(1);
    for z in range(SmoothZoomEnd[1],SmoothZoomStart[1])[::15]:
        device.touch( SmoothZoomStart[0], z, 'DOWN_AND_UP')
        time.sleep(1);
        DoCapture();
        time.sleep(1);

    OpenCameraSettings()
    device.touch( CaptureResolutionUpKey.x, CaptureResolutionUpKey.y, 'DOWN_AND_UP')
    time.sleep(1)
    DoBack()
    time.sleep(1)
    
wait(20)
exit()

#END OF FILE
