
import time
from time import sleep
import random
import sys
from config import * 
from camera_smoketest_config  import *

#########################################################

print '*******************************************************************'
print 'Start camera app for %d times' % TestIterTimes
print '*******************************************************************'
for s in range(2):
    for i in range(TestIterTimes):
        device.startActivity(component=runComponent)
        wait(open_wait)
        print 'start camera app No.%d'%i
        exit()
    device.startActivity(component=runComponent)
    wait(open_wait)
    SwitchCamera()
    exit()     
