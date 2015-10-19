import time
import random
import sys
from config import *
import random

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0018'
print 'recording 500 times'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')
sleep(2)   

picTimes = 500
for i in range(picTimes):
    print 'recording No.%d'%i
    StartRecording();
    time.sleep(2);
    wait(random.randint(5,15));
    StopRecording();
    time.sleep(3);

wait(20)
exit()

#END OF FILE
