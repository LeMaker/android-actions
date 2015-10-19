/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_PERFORMANCESERVICE_H
#define ANDROID_PERFORMANCESERVICE_H

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/List.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <binder/IServiceManager.h>

#include "performance/IPerformanceService.h"

#ifdef __cplusplus
namespace android {
#endif

/* obtain structures and constants from the kernel header */
#include "PerformanceState.h"

#ifdef __cplusplus
}   // namespace android
#endif

#define MAX_HARD_FREQ_POINTS_SUM 31

namespace android {


enum {
    START_MODE_NORMAL = 0,
    START_MODE_BENCHMARK,
    START_MODE_GAME,
    START_MODE_IMMD,
};

typedef struct __PolicyTableItem
{
    int core;       // the number of core used in the scene
    int cpuLevel;   // the cpu frequency level
    int gpuLevel;   // the gpu frequency level
    int boost;      // boost?
    int startMode; // is Benchmark?
    int timeOpt;    // timer?
    String8 scene;  // string for indicate the scene (currently is activity)
}PolicyTableItem_t;

typedef struct __PolicyParam{
    int cpu_core;   // the number of core used in the scene
    int cpuLevel;   // the cpu frequency level
    int gpuLevel;   // the gpu frequency level
    int startMode; // is in benchmark mode
    bool boost;     // string for indicate the scene (currently is activity)
    int timeOpt;    // timer?
}PolicyParam_t;

class PerformanceService : public BnPerformanceService,  public Thread, public MessageHandler
{
    public:

    static  sp<PerformanceService>  singleInstantiate();

    enum{
        CPU_PLUG_CURRENT = 0,
        CPU_PLUG_1,
        CPU_PLUG_2,
        CPU_PLUG_3,
        CPU_PLUG_4
    };

    enum{
        FREQ_LEVEL_CURRENT = 0,
        FREQ_LEVEL_1,
        FREQ_LEVEL_2,
        FREQ_LEVEL_3,
        FREQ_LEVEL_4,
        FREQ_LEVEL_5, // temprarily, level 5 is set to 1308000
        FREQ_LEVEL_6,
        FREQ_LEVEL_7
    };

    enum{
        PERF_POLICY_CMD_SCENE_NOTIFY = 0,
        PERF_POLICY_CMD_ANTUTU_X,
        PERF_POLICY_CMD_KILL_APP,
    };

    enum{
        MODE_SYSTEM_DEFAULT  = 0,
        MODE_AUTO_POLICY,
        MODE_USER,
    };
    enum {
        MSG_SET_SCENE_POLICY = 1,
    };

    /**** Policy Control ****/
    virtual bool enableAutoPolicy();
    virtual bool disableAutoPolicy();
    virtual bool boostProcesses(int core);   // call this will also disable autoPolicy, need to enable autoPolicy when you want to exit this mode
    virtual bool restoreProcesses();   // call this will also disable autoPolicy, need to enable autoPolicy when you want to exit this mode
    virtual bool enbleAutoAdjustBacklight();
    virtual bool disableAutoAdjustBacklight();

    virtual bool massStorageOptimizeBegin();
    virtual bool massStorageOptimizeEnd();


    /*** System cleanup related ***/
    virtual bool cleanAllVmCaches(); // Drop vm caches
    virtual bool cleanAllBackgroundApps();// kill background apps to reclair memory and other resource
    virtual bool syncDisk(); // sync disk write. this can prevent damage when abnormal cut of power supply


    virtual bool setCpuFreqRange(const sp<IBinder> who, int min, int max);
    virtual bool setCpuPerformanceLevel(const sp<IBinder> who, int level, int core, bool boost);
    virtual bool setGpuPerformanceLevel(const sp<IBinder> who, int level);
    virtual bool restoreCpuFreqRange(const sp<IBinder> who);
    virtual bool restoreCpuPerformanceLevel(const sp<IBinder> who);
    virtual bool restoreGpuPerformanceLevel(const sp<IBinder> who);

	virtual bool sceneSet(const sp<IBinder> who, int scene);
	virtual bool sceneExit(const sp<IBinder> who);
	
    /** performance notifier **/
    bool notifier(int cmd, int pid, const String16 &payload);

    // version 2:
    virtual bool appStart();
    virtual bool appExit();
    virtual bool speedupRotation();
    virtual bool appSetupName(int pid, const char*name);

    // version 3:
    virtual bool inflateFile(const String16 &src, const String16 &dst);

    virtual status_t dump(int fd, const Vector<String16>& args);


    /** Here we add the thread interface to do the Power consumption and temp tuning **/
    /** BE SURE all the tunning number is an Experience Value, we have no sensor!!! **/
    virtual bool threadLoop();
    virtual status_t readyToRun();
    virtual void onFirstRef();

    /* reset to default */
    void freqResetForNow();

    void handleMessage(const Message& message);

private:
    PerformanceService();
    void cpuFreqRangInit();
    int roundFreq(int freq);
    bool loadPolicyWhiteList_l();
    virtual    ~PerformanceService();
    bool findSpecDealApp(int mCtlFd, String8 &pattern);		
    bool findSpecPowerDealApp(String8 &pattern);	
    bool findCurrentPolicyParam_l(String8 &pattern);
    bool autoPolicyTune_l();
    bool init_gpu_freq_policy();
    bool setPerfPolicy_l(PolicyParam_t &param);
    bool setPerfPolicyDefault_l();
    int cpuLevel2Freq(int level){
        if(level<=0 || level > FREQ_LEVEL_7)
            return mCpuLevelTable[0];
        else  return mCpuLevelTable[level -1];
    }

    void initSoceket_l();

    int mMaxCpuFreq;
    int mMinCpuFreq;
    int maxPerformaceCount;
    int maxPerformaceOpen;

    int mCpuFreqCount;
    int mCpuLevelTable[FREQ_LEVEL_7];
    int mCpuFreqTable[MAX_HARD_FREQ_POINTS_SUM];
    Vector<PolicyTableItem_t*> mFullPattenTable;
    Vector<PolicyTableItem_t*> mPartialPattenTable;
    Vector<PolicyTableItem_t*> mSpecPattenTable;
    Vector<PolicyTableItem_t*> mPowsPattenTable;
    int mMode;
    PolicyParam_t mCurPolicyParam;
    bool mTimeSetFlag;
    
    mutable Mutex mLock;
    bool mBurstState;
    int last_mode_opt;
    int mPpmFd;
    sp<Looper> mLooper;
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif
