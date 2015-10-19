import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'video recorder quickly'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')

wait(2)

picTimes = 2000
for i in range(picTimes):
    print 'camerarecorder No.%d'%i
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    time.sleep(1)
    
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    time.sleep(1)
    

wait(20)
exit()

#END OF FILE
