import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'Preview with different awb '
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
switchTimes = 50

for i in range(switchTimes):
    for awb in AWBKeyMaps.keys():
        setWhitebalance(awb);
        wait(10)
    
wait(20)
exit()

#END OF FILE
