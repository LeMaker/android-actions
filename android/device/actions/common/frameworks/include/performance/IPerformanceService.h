/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_IPERFORMANCESERVICE_H
#define ANDROID_IPERFORMANCESERVICE_H

#include <utils/Errors.h>  // for status_t
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IServiceManager.h>

namespace android {

enum scene_mode{
 SCENE_LOCAL_VIDEO = 0x00000001,
 SCENE_ILDE_FORCE =  0x00000010,
};

class IPerformanceService: public IInterface
{
public:
    DECLARE_META_INTERFACE(PerformanceService);

    static inline sp<IPerformanceService> connect()
   	{
       	sp<IPerformanceService> PerformanceService;
   			ALOGE("IPerformanceService Connect\n");

        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;

        do {
            binder = sm->getService(String16("performanceservice"));
            if (binder != 0)    break;

            ALOGW("performanceservice not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);

        PerformanceService = interface_cast<IPerformanceService>(binder);
        ALOGE_IF(PerformanceService == 0, "no PerformanceService!?");
        return PerformanceService;
    }

    /** performance notifier **/
    virtual bool notifier(int cmd, int pid,  const String16&payload)= 0;

    /**** Policy Control ****/
    virtual bool enableAutoPolicy()= 0;
    virtual bool disableAutoPolicy()= 0;
    virtual bool boostProcesses(int core)= 0;   // call this will also disable autoPolicy, need to enable autoPolicy when you want to exit this mode
    virtual bool restoreProcesses()= 0;   // call this will also disable autoPolicy, need to enable autoPolicy when you want to exit this mode
    virtual bool enbleAutoAdjustBacklight()= 0;
    virtual bool disableAutoAdjustBacklight()= 0;

    virtual bool massStorageOptimizeBegin() = 0;
    virtual bool massStorageOptimizeEnd() = 0;
    

    /*** System cleanup related ***/
    virtual bool cleanAllVmCaches()= 0; // Drop vm caches
    virtual bool cleanAllBackgroundApps()= 0;// kill background apps to reclair memory and other resource
    virtual bool syncDisk()= 0; // sync disk write. this can prevent damage when abnormal cut of power supply


    virtual bool setCpuFreqRange(const sp<IBinder> who, int min, int max)= 0;
    virtual bool setCpuPerformanceLevel(const sp<IBinder> who, int level, int core, bool boost)= 0;
    virtual bool setGpuPerformanceLevel(const sp<IBinder> who, int level)= 0;
    virtual bool restoreCpuFreqRange(const sp<IBinder> who)= 0;
    virtual bool restoreCpuPerformanceLevel(const sp<IBinder> who)= 0;
    virtual bool restoreGpuPerformanceLevel(const sp<IBinder> who)= 0;

    virtual bool sceneSet(const sp<IBinder> who, int scene) = 0;
    virtual bool sceneExit(const sp<IBinder> who) = 0;
	
    //Version 2: spot for app to trigger
    virtual bool appStart() = 0;
    virtual bool appExit() = 0;
    virtual bool speedupRotation() = 0;
    virtual bool appSetupName(int pid, const char*name) = 0;

    //Version 3
    virtual bool inflateFile(const String16 &src, const String16 &dst) = 0;
};

// ----------------------------------------------------------------------------

class BnPerformanceService: public BnInterface<IPerformanceService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif
