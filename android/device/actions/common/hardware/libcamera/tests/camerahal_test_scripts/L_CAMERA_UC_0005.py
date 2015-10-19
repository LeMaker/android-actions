import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'Switch between back camera and front camera'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
switchTimes = 500
for i in range(switchTimes):
    print 'switch camera No.%d'%i
    SwitchCamera()

wait(20)
exit()

#END OF FILE
