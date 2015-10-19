import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test CAMERA_SELF_TEST_0002'
print 'switch between camera and video activity quickly'
print '*******************************************************************'



switchTimes = 1000
for i in range(switchTimes):
    #device.startActivity(component=cameraComponent) 
    #device.startActivity(component=videoCameraComponent) 
    device.shell(('am start -n %s'% cameraComponent)) 
    device.shell(('am start -n %s'% videoCameraComponent)) 
    

exit()

#END OF FILE
