
import time
from time import sleep
import random
import sys
from config import *
from camera_smoketest_config  import *
import os;


############################################################
print '*******************************************************************'
print 'TakePicture with different scene '
print '*******************************************************************'
device.startActivity(component=runComponent)
wait(open_wait)

for s in range(2):
    RestoreDefaults()
    wait(2)
    for scene in ScnenKeyMaps.keys():
        SetSceneMode(scene);
        takeSnapshot(os.path.join(camera_smoketest_datapath, "scene_%s_camera%d.png"%(scene,s)));
        DoCapture()
        wait(2)
    
    SwitchCamera()
wait(2)
exit()   
