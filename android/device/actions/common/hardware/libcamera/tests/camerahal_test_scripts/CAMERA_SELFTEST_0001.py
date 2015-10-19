import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test CAMERA_SELF_TEST_0001'
print 'click shutter button recorder quickly'
print '*******************************************************************'



device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')

picTimes = 20000
for i in range(picTimes):
    print 'camerarecorder No.%d'%i
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    time.sleep(0.05)
    

wait(20)
exit()

#END OF FILE
