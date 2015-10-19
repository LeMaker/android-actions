import os, sys;
import imp;

if __name__ == '__main__':
    curpath = os.path.dirname(os.path.abspath(__file__)) 
    sys.path.append(curpath);
    smoketestpath = os.path.join(curpath,'camera_smoketest');
    sys.path.append(smoketestpath);

    if len(sys.argv) <2:
        print 'usage: monkeyrunner_scriptrun.py L_CAMERA_UC_XXXX.py'
        sys.exit(0);

    else:
        try:
            testcaseModule = os.path.splitext(sys.argv[1])[0];
            from imp import load_module
            fp,pathname,description = imp.find_module(testcaseModule);
            load_module(testcaseModule,fp,pathname,description);
            #from runpy import run_module
            #run_module(sys.argv[1]);
        except Exception,e:
            print e
            print 'loader error'

