import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'Switch between capture mode and camcorder mode'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
switchTimes = 500
for i in range(switchTimes):
    print 'switch to capture mode No.%d'%i
    device.touch( ImageMode.x, ImageMode.y, 'DOWN_AND_UP')
    wait(4)
    print 'switch to camcorder mode No.%d'%i
    device.touch( CamcorderMode.x, CamcorderMode.y, 'DOWN_AND_UP')
    wait(4)

wait(20)
exit()

#END OF FILE
