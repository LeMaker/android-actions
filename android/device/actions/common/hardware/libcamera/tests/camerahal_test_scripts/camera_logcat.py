import shlex,subprocess
import os
import signal
import popen2

def clearLogcat():
    os.system("adb logcat -c");

def saveLogcat(logfile):
    logcommand = 'adb logcat -v time -d'
    savefd = open(logfile,'w+');
    logargs = shlex.split(logcommand);
    proc = subprocess.Popen(logargs, stdout=savefd);
    proc.communicate();
    proc.wait()
    savefd.close();
    
