
import time
from time import sleep
import random
import sys
from config import *
import camera_smoketest_config

#########################################################
print '*******************************************************************'
print 'Camera restore to defaults and preview' 
print '*******************************************************************'
device.startActivity(component=runComponent)
wait(open_wait)
RestoreDefaults()
wait(5)
exit() 
