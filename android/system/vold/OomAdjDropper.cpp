
#define LOG_TAG "usbmond"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <IOomAdjDropper.h>

using namespace android;

static sp<IBinder> iBinder; 
static sp<IOomAdjDropper> oomAdjDropper;
static int enablePhoneOomAdj=0;
static int firstRequest=0; 
  
void oomAdjDropperInit(void){
    char prop_value[PROP_VALUE_MAX];
    property_get("ro.phone.dynamicoomadj", prop_value, "0");
    enablePhoneOomAdj=atoi(prop_value);
    if(!enablePhoneOomAdj)
      return;
    ALOGD("enablePhoneOomAdj=%d", enablePhoneOomAdj);
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();
}
static void oomAdjServiceInit(void){
      if(!enablePhoneOomAdj)
      return;
    if(iBinder==0){
      iBinder = defaultServiceManager()->getService(String16("oomadjust"));
  	  if(iBinder==0){
  		  ALOGE("error  cannot get  oomadjust  service");
  		  return ;
  	  }
  	}
  	if(oomAdjDropper==0){
      	oomAdjDropper=interface_cast<IOomAdjDropper>(iBinder);
      	if(oomAdjDropper==0){
      		ALOGE("error interface_cast IOomAdjDropper failed");
      		return ;	
      	}	
     }
}

void  dropPhoneOomAdj(void){
  if(!enablePhoneOomAdj)
    return;
  ALOGD("dropPhoneOomAdj called");  
  oomAdjServiceInit();
  if(oomAdjDropper!=0){
      oomAdjDropper->adjustOom(1);
  }
}

void raisePhoneOomAdj(void){
  if(!enablePhoneOomAdj)
    return;
   ALOGD("raisePhoneOomAdj called");    
  oomAdjServiceInit();
   if(oomAdjDropper!=0){
      oomAdjDropper->adjustOom(0);
  }else{
    //maybe oomadjust service has not ready yet, wait....
    while(firstRequest++<5){
        oomAdjServiceInit();
        ALOGD("retry wait oomadjust service to ready");    
        sleep(5);
        if(oomAdjDropper!=0){
            oomAdjDropper->adjustOom(0);
            break;
         }
    }
     ALOGE("retry wait oomadjust service to ready timeout");    
    
  }
}