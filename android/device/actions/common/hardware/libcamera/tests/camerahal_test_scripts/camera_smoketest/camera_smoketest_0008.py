
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *


############################################################
print '*******************************************************************'
print 'TakePicture with different awb '
print '*******************************************************************'

device.startActivity(component=runComponent)
wait(open_wait)
for s in range(2):
    RestoreDefaults()
    wait(2)
    for awb in AWBKeyMaps.keys():
        setWhitebalance(awb);
        takeSnapshot(os.path.join(camera_smoketest_datapath, "scene_%s_camera%d.png"%(awb,s)));
        DoCapture();
        wait(2)
    SwitchCamera()
    
wait(2)
exit()   



