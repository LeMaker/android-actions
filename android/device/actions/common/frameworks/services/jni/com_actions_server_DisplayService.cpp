#include <utils/Log.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <cutils/properties.h>

#include <jni.h>
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "display.h"

namespace android {

static int initialized = 0;

static struct owldisp_device_t * mDisplayManager = NULL;

int owl_hdmi_mode2vid(char* mode)
{
	int i = 0;
	int rc = -1;
	for(i = 0; i < HDMI_SUPPORT_MODE_NUMER; i++)
	{	ALOGD("%s memcmp %s length %d length %d",mode,owlfb_hdmi_mode_table[i].mode,strlen(mode),strlen(owlfb_hdmi_mode_table[i].mode));
		if(memcmp(mode,owlfb_hdmi_mode_table[i].mode,strlen(owlfb_hdmi_mode_table[i].mode)) == 0)
		{
			ALOGD("%s memcmp %s length %d equed",mode,owlfb_hdmi_mode_table[i].mode,strlen(mode));
			rc = owlfb_hdmi_mode_table[i].vid;
			break;
		}
	}
	return rc ;
}

char* owl_hdmi_vid2mode(int vid)
{
	int i = 0;
	char * rc = NULL;
	for(i = 0; i < HDMI_SUPPORT_MODE_NUMER; i++)
	{
		ALOGD("%d =? %d",vid,owlfb_hdmi_mode_table[i].vid);
		if(vid == owlfb_hdmi_mode_table[i].vid)
		{
			rc = (char *)owlfb_hdmi_mode_table[i].mode;
			break;
		}
	}
	return rc;
}

static jboolean
actions_server_DisplayService_init(JNIEnv *env, jclass clazz) {
	owldisp_module_t * module;
	if (initialized)
	{
		return true;
	}
	ALOGD("enter actions_server_DisplayService_init\n");

	if (hw_get_module(DM_HARDWARE_MODULE_ID, (const hw_module_t**) &module) == 0)
	{
		ALOGI(": display engine found.");
		if (owldisp_manager_open(&module->common, &mDisplayManager) == 0) {
			ALOGI(": got display engine operations.mDisplayManager:%lx",(unsigned long) mDisplayManager);
		} else {
			mDisplayManager = NULL;
			return false;
		}
	} else {
		ALOGI("display engine found not find %s",DM_HARDWARE_MODULE_ID);
		mDisplayManager = NULL;
		return false;
	} 


	initialized = 1;

	return true;

}

static void
actions_server_DisplayService_setHdmiEnable(JNIEnv *env, jclass clazz, jboolean enable){
    ALOGD("actions_server_DisplayService_setHdmiEnable");
    if(mDisplayManager != NULL && mDisplayManager->set_hdmi_enable != NULL)
    {
    	ALOGD("set hdmi function state %s",enable? "enable" : "disable");
    	mDisplayManager->set_hdmi_enable(mDisplayManager,enable);
    }   
}

static jboolean
actions_server_DisplayService_getHdmiEnable(JNIEnv *env, jclass clazz){ 
    ALOGD("actions_server_DisplayService_getHdmiEnable");
    if(mDisplayManager != NULL && mDisplayManager->get_hdmi_enable != NULL)
    {
    	ALOGD("get hdmi function state ");
    	return mDisplayManager->get_hdmi_enable(mDisplayManager);
    }
    return false;   
}

static jboolean
actions_server_DisplayService_setHdmiMode(JNIEnv *env, jclass clazz, jstring modeObj){
    ALOGD("actions_server_DisplayService_setHdmiMode");
    const char * modeStr = env->GetStringUTFChars(modeObj, NULL);
    if(modeStr != NULL)
    {
    	int vid = owl_hdmi_mode2vid((char*)modeStr);
    	ALOGD("modeStr %s vid %d mDisplayManager ~~ %p",modeStr,vid,mDisplayManager);
        if(mDisplayManager != NULL && mDisplayManager->set_hdmi_vid != NULL)
	    {
	    	ALOGD("set hdmi mode  %d",vid);
	    	mDisplayManager->set_hdmi_vid(mDisplayManager,vid);
	    } 
    }
    env->ReleaseStringUTFChars(modeObj, modeStr);
    return true;

}

static jstring
actions_server_DisplayService_getHdmiMode(JNIEnv *env, jclass clazz){
    ALOGD("actions_server_DisplayService_getHdmiMode mDisplayManager %p",mDisplayManager);
    char * mode = NULL;
    if(mDisplayManager != NULL && mDisplayManager->set_hdmi_enable != NULL)
	{
		int vid = mDisplayManager->get_hdmi_vid(mDisplayManager);
		
		mode = owl_hdmi_vid2mode(vid);
		ALOGD("actions_server_DisplayService_getHdmiMode vid %d mode %s ",vid,mode);
	} 
	if(mode != NULL){
		return env->NewStringUTF(mode);
	}
    return env->NewStringUTF("");
}


//CVBS setting


static jboolean
actions_server_DisplayService_setCvbsMode(JNIEnv *env, jclass clazz, jstring modeObj){
    ALOGD("actions_server_DisplayService_setCvbsMode");
    const char * modeStr = env->GetStringUTFChars(modeObj, NULL);
    if(modeStr != NULL)
    {
    	int vid = owl_hdmi_mode2vid((char*)modeStr);
    	ALOGD("modeStr %s vid %d mDisplayManager ~~ %p",modeStr,vid,mDisplayManager);
        if(mDisplayManager != NULL && mDisplayManager->set_cvbs_vid != NULL)
	    {
	    	ALOGD("set cvbs mode  %d",vid);
	    	mDisplayManager->set_cvbs_vid(mDisplayManager,vid);
	    } 
    }
    env->ReleaseStringUTFChars(modeObj, modeStr);
    return true;

}


static void
actions_server_DisplayService_setCvbsEnable(JNIEnv *env, jclass clazz, jboolean enable){
    ALOGD("actions_server_DisplayService_setCvbsEnable");
    if(mDisplayManager != NULL && mDisplayManager->set_cvbs_enable != NULL)
    {
    	ALOGD("set cvbs function state %s",enable? "enable" : "disable");
    	mDisplayManager->set_cvbs_enable(mDisplayManager,enable);
    }   
}

static jboolean
actions_server_DisplayService_getCvbsEnable(JNIEnv *env, jclass clazz){ 
    ALOGD("actions_server_DisplayService_getCvbsEnable");
    if(mDisplayManager != NULL && mDisplayManager->get_cvbs_enable != NULL)
    {
    	ALOGD("get hdmi function state ");
    	return mDisplayManager->get_cvbs_enable(mDisplayManager);
    }
    return false;   
}

static jstring
actions_server_DisplayService_getCvbsMode(JNIEnv *env, jclass clazz){
    ALOGD("actions_server_DisplayService_getCvbsMode mDisplayManager %p",mDisplayManager);
    char * mode = NULL;
    if(mDisplayManager != NULL && mDisplayManager->get_cvbs_vid != NULL)
	{
		int vid = mDisplayManager->get_cvbs_vid(mDisplayManager);
		
		mode = owl_hdmi_vid2mode(vid);
		ALOGD("actions_server_DisplayService_getHdmiMode vid %d mode %s ",vid,mode);
	} 
	if(mode != NULL){
		return env->NewStringUTF(mode);
	}
    return env->NewStringUTF("");
}



static jobjectArray
actions_server_DisplayService_getHdmiSupportedModesList(JNIEnv *env, jclass clazz){
    ALOGD("actions_server_DisplayService_getHdmiSupportedModesList");
    int vid_table[HDMI_SUPPORT_MODE_NUMER];
    int vid_number;
    int i = 0;
    jobjectArray array = NULL;
    
    if(mDisplayManager != NULL && mDisplayManager->get_hdmi_supported_vid_list != NULL)
	{
		vid_number = mDisplayManager->get_hdmi_supported_vid_list(mDisplayManager,&vid_table[0]);		
	} 
	if(vid_number > 0)
	{
		int count = 0;
		jclass cls = env->FindClass("java/lang/String");
	    array = env->NewObjectArray(vid_number, cls, NULL);
		for(i = 0; i < vid_number; i++)
		{
			char * mode = NULL;
			mode = owl_hdmi_vid2mode(vid_table[i]);
			jstring str = NULL;
            str = env->NewStringUTF(mode);
            env->SetObjectArrayElement(array, count++, str);
            env->DeleteLocalRef(str);
		}
	}
    return array;
}

static void
actions_server_DisplayService_setHmdiViewFrameSize(JNIEnv *env, jclass clazz, jint dx, jint dy){
    ALOGD("actions_server_DisplayService_setHmdiViewFrameSize");
    if(mDisplayManager != NULL && mDisplayManager->set_hdmi_size != NULL)
    {
    	ALOGD("set hdmi View Frame Size %d %d", dx, dy);
    	mDisplayManager->set_hdmi_size(mDisplayManager,dx,dy);
    }
}

static jint
actions_server_DisplayService_getHmdiViewFrameSize(JNIEnv *env, jclass clazz, jintArray dx_dy){
    ALOGD("actions_server_DisplayService_getHmdiViewFrameSize");
    jint flag = 0;
	jint _exception = 0;
	jint _remaining;
	const char * _exceptionType = NULL;
	const char * _exceptionMessage = NULL;
	jint * buffer = (jint *) 0;
    jint *buffer_base = (jint *) 0;
    if(mDisplayManager != NULL && mDisplayManager->set_hdmi_size != NULL)
    {	 
	    if (!dx_dy) {
	        _exception = 1;
	        _exceptionType = "java/lang/IllegalArgumentException";
	        _exceptionMessage = "textures == null";
	        goto exit;
	    }
	    _remaining = env->GetArrayLength(dx_dy);
	    if (_remaining < 0) {
	        _exception = 1;
	        _exceptionType = "java/lang/IllegalArgumentException";
	        _exceptionMessage = "length - offset < n < needed";
	        goto exit;
	    }
	    
	    buffer_base = (jint *) env->GetPrimitiveArrayCritical(dx_dy, (jboolean *)0);
    	mDisplayManager->get_hdmi_size(mDisplayManager,buffer_base);
    }
exit:
    if (buffer_base) {
        env->ReleasePrimitiveArrayCritical(dx_dy, buffer_base, _exception?JNI_ABORT:0);
    }
    if (_exception) {
        jniThrowException(env, _exceptionType, _exceptionMessage);
    }
	return flag;
}

static jboolean
actions_server_DisplayService_getHdmiCableState(JNIEnv *env, jclass clazz){
    ALOGD("actions_server_DisplayService_getHdmiCableState");
    if(mDisplayManager != NULL && mDisplayManager->get_hdmi_cable_state != NULL)
	{
		if(mDisplayManager->get_hdmi_cable_state(mDisplayManager))		
			return true;
		else
		    return false;
	} 
    return JNI_FALSE;
}

static jboolean
actions_server_DisplayService_setHdmiFitScreen(JNIEnv *env, jclass clazz, int value){
    ALOGD("actions_server_DisplayService_setHdmiFitScreen value %d", value);
    if(mDisplayManager != NULL && mDisplayManager->set_hdmi_fitscreen != NULL)
    {
    	ALOGD("set hdmi Hdmi Fit Screen value %d",value);
    	mDisplayManager->set_hdmi_fitscreen(mDisplayManager,value);
    }
    return JNI_TRUE;
}

static int 
actions_server_DisplayService_getHdmiFitScreen(JNIEnv *env, jclass clazz){
    ALOGD("actions_server_DisplayService_getHdmiFitScreen");
    int value = 0;
    if(mDisplayManager != NULL && mDisplayManager->get_hdmi_fitscreen != NULL)
    {    	
    	value = mDisplayManager->get_hdmi_fitscreen(mDisplayManager);
    }
    return value;
}

static jint actions_server_DisplayService_getDisplayInfo(JNIEnv *env,jclass clazz,jint display, jintArray info_buf,jint offset) {
	jint flag = 0;
    jint _exception = 0;
    const char * _exceptionType = NULL;
    const char * _exceptionMessage = NULL;
    jint _remaining;
    jint * buffer = (jint *) 0;
    jint *buffer_base = (jint *) 0;
 
    if (!info_buf) {
        _exception = 1;
        _exceptionType = "java/lang/IllegalArgumentException";
        _exceptionMessage = "textures == null";
        goto exit;
    }
    if (offset < 0) {
        _exception = 1;
        _exceptionType = "java/lang/IllegalArgumentException";
        _exceptionMessage = "offset < 0";
        goto exit;
    }
    _remaining = env->GetArrayLength(info_buf) - offset;
    if (_remaining < 0) {
        _exception = 1;
        _exceptionType = "java/lang/IllegalArgumentException";
        _exceptionMessage = "length - offset < n < needed";
        goto exit;
    }
    
    buffer_base = (jint *) env->GetPrimitiveArrayCritical(info_buf, (jboolean *)0);
    buffer = buffer_base + offset;
    // op buffer  
    ALOGE("actions_server_DisplayService_getDisplayInfo buffer_base %p buffer %p _remaining %d",buffer_base,buffer,_remaining);   
    mDisplayManager->get_disp_info(mDisplayManager,display,buffer);    
        
    //end op buffer 
exit:
    if (buffer_base) {
        env->ReleasePrimitiveArrayCritical(info_buf, buffer_base, _exception?JNI_ABORT:0);
    }
    if (_exception) {
        jniThrowException(env, _exceptionType, _exceptionMessage);
    }
	return flag;
}

static jint actions_server_DisplayService_setDisplayInfo(JNIEnv *env, jclass clazz,jint display, jintArray info_buf,jint offset) {
	jint flag = 0;
    jint _exception = 0;
    const char * _exceptionType = NULL;
    const char * _exceptionMessage = NULL;
    jint _remaining;
    jint * buffer = (jint *) 0;
    jint *buffer_base = (jint *) 0;
 
    if (!info_buf) {
        _exception = 1;
        _exceptionType = "java/lang/IllegalArgumentException";
        _exceptionMessage = "textures == null";
        goto exit;
    }
    if (offset < 0) {
        _exception = 1;
        _exceptionType = "java/lang/IllegalArgumentException";
        _exceptionMessage = "offset < 0";
        goto exit;
    }
    _remaining = env->GetArrayLength(info_buf) - offset;
    if (_remaining < 0) {
        _exception = 1;
        _exceptionType = "java/lang/IllegalArgumentException";
        _exceptionMessage = "length - offset < n < needed";
        goto exit;
    }
    buffer_base = (jint *) env->GetPrimitiveArrayCritical(info_buf, (jboolean *)0);
    buffer = buffer_base + offset;
    
    // op buffer 
    ALOGE("actions_server_DisplayService_setDisplayInfo buffer_base %p buffer %p _remaining %d",buffer_base,buffer,_remaining);  
    mDisplayManager->set_disp_info(mDisplayManager,display,buffer);
    
    //end op buffer 
exit:
    if (buffer_base) {
        env->ReleasePrimitiveArrayCritical(info_buf, buffer_base, _exception?JNI_ABORT:0);
    }
    if (_exception) {
        jniThrowException(env, _exceptionType, _exceptionMessage);
    }
	return flag;
}

/*
 * Array of methods.
 *
 * Each entry has three fields: the name of the method, the method
 * signature, and a pointer to the native implementation.
 */
static const JNINativeMethod displayServiceMethods[] = { 
{ "_init", "()Z",(void*) actions_server_DisplayService_init },
{ "_setHdmiEnable", "(Z)V",(void*) actions_server_DisplayService_setHdmiEnable },
{ "_getHdmiEnable", "()Z",(void*) actions_server_DisplayService_getHdmiEnable },
{ "_setHdmiMode", "(Ljava/lang/String;)Z",(void*) actions_server_DisplayService_setHdmiMode },
{ "_getHdmiMode", "()Ljava/lang/String;",(void*) actions_server_DisplayService_getHdmiMode },
{ "_getHdmiSupportedModesList", "()[Ljava/lang/String;",(void*) actions_server_DisplayService_getHdmiSupportedModesList },
{ "_setHdmiViewFrameSize", "(II)V",(void*) actions_server_DisplayService_setHmdiViewFrameSize },
{ "_getHdmiViewFrameSize", "([I)I",(void*) actions_server_DisplayService_getHmdiViewFrameSize },
{ "_getHdmiCableState", "()Z",(void*) actions_server_DisplayService_getHdmiCableState },
{ "_setHdmiFitScreen", "(I)Z",(void*) actions_server_DisplayService_setHdmiFitScreen },
{ "_getHdmiFitScreen", "()I",(void*) actions_server_DisplayService_getHdmiFitScreen },
{"_getDisplayInfo", "(I[II)I",(void *) actions_server_DisplayService_getDisplayInfo},	
{"_setDisplayInfo", "(I[II)I",(void *) actions_server_DisplayService_setDisplayInfo},
{"_getCvbsMode", "()Ljava/lang/String;",(void*) actions_server_DisplayService_getCvbsMode },
{"_setCvbsMode", "(Ljava/lang/String;)Z",(void*) actions_server_DisplayService_setCvbsMode },
{ "_setCvbsEnable", "(Z)V",(void*) actions_server_DisplayService_setCvbsEnable },
{ "_getCvbsEnable", "()Z",(void*) actions_server_DisplayService_getCvbsEnable },
};

/*
 * This is called by the VM when the shared library is first loaded.
 */
int register_android_server_DisplayService(JNIEnv *env) {
	int result = 0;
	jclass clazz;

	if (jniRegisterNativeMethods(env, "com/actions/server/DisplayService",
			displayServiceMethods, NELEM(displayServiceMethods)) != 0) {
		ALOGE("Register method to com/actions/server/DisplayService failed!");
		return -1;
	}

	return 0;
}

};// namespace

