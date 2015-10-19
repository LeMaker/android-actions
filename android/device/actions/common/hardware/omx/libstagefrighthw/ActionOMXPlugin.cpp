/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 
#define LOG_NDEBUG 0
#define LOG_TAG "ActionOMXPlugin"
#include <utils/Log.h>

#include "ActionOMXPlugin.h"

#include <dlfcn.h>

#include <HardwareAPI.h>
//#include <MediaDebug.h>
#define LOGV ALOGD
#define LOGD ALOGD
#define LOGE ALOGD

namespace android {

OMXPluginBase *createOMXPlugin() {
    return new ActionOMXPlugin;
}

ActionOMXPlugin::ActionOMXPlugin()
    : mLibHandle(dlopen("libOMX_Core.so", RTLD_NOW)),
      mInit(NULL),
      mDeinit(NULL),
      mComponentNameEnum(NULL),
      mGetHandle(NULL),
      mFreeHandle(NULL),
      mGetRolesOfComponentHandle(NULL) { 

	  ALOGE("In ActionOMXPlugin: libOMX_Core.so is openning!");
    if (mLibHandle != NULL) {
		
	 ALOGE("In ActionOMXPlugin: libAction_OMX_Core.so is openning!");
        mInit = (InitFunc)dlsym(mLibHandle, "OMX_Init");
        mDeinit = (DeinitFunc)dlsym(mLibHandle, "OMX_Deinit");

        mComponentNameEnum =
            (ComponentNameEnumFunc)dlsym(mLibHandle, "OMX_ComponentNameEnum");

        mGetHandle = (GetHandleFunc)dlsym(mLibHandle, "OMX_GetHandle");
        mFreeHandle = (FreeHandleFunc)dlsym(mLibHandle, "OMX_FreeHandle");

        mGetRolesOfComponentHandle =
            (GetRolesOfComponentFunc)dlsym(
                    mLibHandle, "OMX_GetRolesOfComponent");

        (*mInit)();
    }else
    	{
    	  ALOGE("In ActionOMXPlugin: open libAction_OMX_Core.so error");
    	}
	
}

ActionOMXPlugin::~ActionOMXPlugin() {
    if (mLibHandle != NULL) {
        (*mDeinit)();

        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

OMX_ERRORTYPE ActionOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component) {

    ALOGV("Entry ActionOMXPlugin::makeComponentInstance");
	
    if (mLibHandle == NULL) {
    		LOGV("Entry makeComponentInstance err");
        return OMX_ErrorUndefined;
    }

    return (*mGetHandle)(
            reinterpret_cast<OMX_HANDLETYPE *>(component),
            const_cast<char *>(name),
            appData, const_cast<OMX_CALLBACKTYPE *>(callbacks));
}

OMX_ERRORTYPE ActionOMXPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE *component) {
    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mFreeHandle)(reinterpret_cast<OMX_HANDLETYPE *>(component));
}

OMX_ERRORTYPE ActionOMXPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mComponentNameEnum)(name, size, index);
}

OMX_ERRORTYPE ActionOMXPlugin::getRolesOfComponent(
        const char *name,
        Vector<String8> *roles) {

	roles->clear();

    	if (mLibHandle == NULL) {
        	return OMX_ErrorUndefined;
    	}	

    	OMX_U32 numRoles;
    	OMX_ERRORTYPE err = (*mGetRolesOfComponentHandle)(
            	const_cast<OMX_STRING>(name), &numRoles, NULL);

    	if (err != OMX_ErrorNone) {
		return err;
    	}

	if (numRoles > 0) {
		OMX_U8 **array = new OMX_U8 *[numRoles];
        	for (OMX_U32 i = 0; i < numRoles; ++i) {
            		array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
        	}

        	OMX_U32 numRoles2;
        	err = (*mGetRolesOfComponentHandle)(
                	const_cast<OMX_STRING>(name), &numRoles2, array);
            
            if(err != OMX_ErrorNone)
            {
                return err;
            }
            
            if(numRoles != numRoles2)
            {
                return OMX_ErrorUndefined;
            }
        	//CHECK_EQ((status_t)err, (status_t)OMX_ErrorNone);
        	//CHECK_EQ(numRoles, numRoles2);

		for (OMX_U32 i = 0; i < numRoles; ++i) {
            		String8 s((const char *)array[i]);
            		roles->push(s);

            		delete[] array[i];
            		array[i] = NULL;
        	}

        	delete[] array;
        	array = NULL;
    	}

    return OMX_ErrorNone;
}

}  // namespace android
