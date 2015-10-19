import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'Take pictures quickly with 5MP'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

OpenCameraSettings()
for i in range(PictureSizeNum):
    device.touch( CaptureResolutionUpKey.x, CaptureResolutionUpKey.y, 'DOWN_AND_UP')
    time.sleep(1)

DoBack()

wait(2)

picTimes = 1000
for i in range(picTimes):
    print 'takePicture No.%d'%i
    device.touch( CaptureKey.x, CaptureKey.y, 'DOWN_AND_UP')
    print 'Capture ongoing ...'
    time.sleep(0.5)
wait(20)
exit()

#END OF FILE
