
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


############################################################
print '*******************************************************************'
print 'recording %d times'% TestIterTimes
print '*******************************************************************'

device.startActivity(component=runComponent)
wait(open_wait)


for s in range(2):
    RestoreDefaults()

    wait(2)
    device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')
    sleep(2)   

    for i in range(TestIterTimes):
        print 'recording No.%d'%i
        StartRecording();
        wait(random.randint(1,5));
        StopRecording();

    SwitchCamera()
wait(2)
exit()  
