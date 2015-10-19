
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


########################################################
print '*******************************************************************'
print 'Switch front and back camera for %d times' % TestIterTimes 
print '*******************************************************************'
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()
wait(2)

for i in range((TestIterTimes+1//2)*2):
    print 'switch camera No.%d'%i
    SwitchCamera()

wait(2)
exit()  
