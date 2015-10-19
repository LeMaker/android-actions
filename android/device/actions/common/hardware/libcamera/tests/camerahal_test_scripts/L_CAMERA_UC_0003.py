import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'Take 500 pictures'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
picTimes = 500
for i in range(picTimes):
    print 'takePicture No.%d'%i
    DoCapture()
    wait(2)

wait(20)
exit()

#END OF FILE
