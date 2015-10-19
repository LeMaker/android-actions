import time
import random
import sys
from config import *

print '*******************************************************************'
print 'Functional test L_CAMERA_UC_0001'
print 'Start Preview with default setting and Close appplication after 1 min'
print '*******************************************************************'

print 'Opening camera application' 
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()
print 'Preview running 1 min'
wait(60)

exit()

#END OF FILE
