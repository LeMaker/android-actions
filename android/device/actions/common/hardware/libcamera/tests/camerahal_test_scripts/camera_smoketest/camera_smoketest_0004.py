
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *

########################################################
print '*******************************************************************'
print 'Switch image and camrecoder mode for %d times'% TestIterTimes
print '*******************************************************************'
device.startActivity(component=runComponent)
wait(open_wait)
for s in range(2):
    RestoreDefaults()
    for i in range(TestIterTimes):
        print 'switch to capture mode No.%d'%i
        device.touch( ImageMode.x, ImageMode.y, 'DOWN_AND_UP')
        wait(3)
        print 'switch to camcorder mode No.%d'%i
        device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')
        wait(3)

    SwitchCamera()
wait(2)
exit()  
