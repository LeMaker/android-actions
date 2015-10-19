
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


############################################################
print '*******************************************************************'
print 'TakePicture with different zoom '
print '*******************************************************************'

device.startActivity(component=runComponent)
wait(open_wait)

for s in range(2):
    RestoreDefaults()
    wait(2)
    for z in range(SmoothZoomEnd[1],SmoothZoomStart[1])[::-30]:
        device.touch( SmoothZoomStart[0], z, 'DOWN_AND_UP')
        time.sleep(1);
        DoCapture();
        time.sleep(1);
    
    SwitchCamera()
wait(2)
exit()
         
