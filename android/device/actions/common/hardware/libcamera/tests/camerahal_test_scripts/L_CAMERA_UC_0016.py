import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'TakePicture with zoomin and ZoomOut '
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
switchTimes = 10

for i in range(switchTimes):
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
    
wait(20)
exit()

#END OF FILE
