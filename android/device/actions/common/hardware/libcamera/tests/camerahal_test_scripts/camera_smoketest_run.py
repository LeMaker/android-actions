import time
from time import sleep
import random
import sys
import os
import re
from imp import load_module,find_module
from camera_logcat import *

#Debug = True
Debug = False
testcaseids=[7]

#TestTimes = 2000
TestTimes = 1

testStartTime = time.time();

def DeleteFile(dirName):
    """
    delete the all dirs/files in the directory  and itself
    """
    if dirName == "" or not os.path.exists(dirName):
        print "%s is a null string or not exist, input param error! " % dirName
        return
    try: 
        if os.path.isfile(dirName):
            os.remove(dirName)
        else:
            for file in os.listdir(dirName):
                DeleteFile(os.path.join(dirName, file))
            os.rmdir(dirName)
        #print "deleted the file/dir :%s" % dirName
    except:
        print "there is a Error in deleting files/dires: %s" % dirName
        print sys.exc_info()
    
def DeleteFilesInDir(dirName):
    if os.path.isdir(dirName) :
        for file in os.listdir(dirName):
            DeleteFile(os.path.join(dirName, file))
    else:
        print "%s is not a directory" % dirName


print '*******************************************************************'
print 'smoketest for camera'
print '*******************************************************************'

filepath = os.path.join(os.path.dirname(os.path.abspath(__file__)),'camera_smoketest');

testcases=[];

if Debug:
    for id in testcaseids:
        testcases.append('camera_smoketest_%04d' % id);

else:

    for f in os.listdir(filepath):
        no = re.match(r"(camera_smoketest_\d*)\.py",f);
        if no is not None:
            if no.groups()[0]:
                testcases.append(no.groups()[0]);

    testcases.sort();


datapath = os.path.join(filepath,"camera_smoketest_data");
try:
    DeleteFile(datapath);
except Exception,e:
    pass

try:
    os.mkdir(datapath);
except Exception,e:
    pass


for i in range(TestTimes):
    for t in testcases:
        print "\n\n\n"
        print "-----------------Now Test:%s----------------------" %t;
        clearLogcat();
        fp,pathname,description = find_module(( t));
        load_module(t,fp,pathname,description);
        saveLogcat(os.path.join(datapath,"%s.log"%t));
        print "//////////////////End of Test:%s/////////////////////" %t;
        print "\n\n\n"

testEndTime = time.time();
testDuration = testEndTime-testStartTime;
print "\n\n\n"
print "Test Duration = %dh:%dm:%ds " % (testDuration//3600,(testDuration%3600)//60, testDuration%60) 

#END OF FILE
