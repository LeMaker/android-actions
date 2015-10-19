import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0002'
print 'TakePicture with different scene '
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()

wait(2)
switchTimes = 50

for i in range(switchTimes):
    for scene in ScnenKeyMaps.keys():
        SetSceneMode(scene);
        DoCapture();
        wait(2)
    
wait(20)
exit()

#END OF FILE
