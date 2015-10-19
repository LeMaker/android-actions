
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


############################################################
print '*******************************************************************'
print 'video recorder quickly'
print '*******************************************************************'

device.startActivity(component=runComponent)
wait(open_wait)

for s in range(2):
    RestoreDefaults()

    device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')

    wait(2)

    for i in range(TestIterTimes):
        print 'camerarecorder No.%d'%i
        device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
        time.sleep(1)
        device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
        time.sleep(1)
    
    SwitchCamera()

wait(2)
exit()           
