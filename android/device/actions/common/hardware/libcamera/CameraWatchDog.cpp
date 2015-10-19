#include "CameraWatchDog.h"

#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "CameraHal"

#include <utils/Log.h>

namespace android
{
static  unsigned long long get_ms()
{
    struct timeval time;
    unsigned long long ms;

    gettimeofday(&time, NULL);
    ms = time.tv_sec;
    ms *= 1000;
    ms += time.tv_usec /1000;

    return ms;
}      
    

void CameraWatchDog::Init(void *obj, WatchdogCallback cb)
{
    mCallback = cb;
    mObject=obj;
    mLastTickle = 0;
    mPeriodCount = 0;
    mState = WATCHDOG_IDLE;
}

void CameraWatchDog::Init()
{
    Init(NULL, NULL);
}

CameraWatchDog::CameraWatchDog():Thread(false)
{
    Init();
}
CameraWatchDog::~CameraWatchDog()
{
    if(mState != WATCHDOG_IDLE)
    {
        ALOGE("Camera WatchDog is not stopped!");
    }
}

void CameraWatchDog::tickle()
{
    Mutex::Autolock lock(mLock);

    if(mState == WATCHDOG_RUNNING)
    {
        mLastTickle = get_ms();
        //ALOGD("watchdog tickle mLastTickle =%llu",mLastTickle);
    }

}

bool CameraWatchDog::startWatchDog(void *obj, WatchdogCallback cb)
{
    bool ret = true;
    Mutex::Autolock lock(mLock);
    if(mState == WATCHDOG_IDLE)
    {
        Init(obj, cb);
        mLastTickle = get_ms();
        mState = WATCHDOG_RUNNING;
        run();
        ALOGD("start watchdog");
    }
    else
    {
        ALOGE("watch dog was already running");
        ret = false;
    }
    return ret;
}
bool CameraWatchDog::stopWatchDog()
{
    mLock.lock();
    if(mState == WATCHDOG_RUNNING)
    {
        mLock.unlock();
        requestExitAndWait();
        mLock.lock();
        mState = WATCHDOG_IDLE;
    }
    mLock.unlock();
    ALOGD("stop watchdog");
    return true;

}
bool CameraWatchDog::Handler()
{
    mLock.lock();
    if(mState == WATCHDOG_RUNNING)
    {
        mPeriodCount++;
        if(mPeriodCount >= CAMERA_WATCHDOG_PERIOD_MAX_COUNT)
        {
            unsigned long long delta;
            unsigned long long now;
            mPeriodCount = 0;

            now = get_ms();
            //ALOGD("watchdog now =%llu, mLastTickle=%llu",now,mLastTickle);

            delta = now - mLastTickle;
            if(delta > CAMERA_WATCHDOG_TIMEOUT)
            {
                ALOGE("Camera watchdog timeout =%llu, %d",delta, CAMERA_WATCHDOG_TIMEOUT);
                if(mCallback)
                {
                    //Don't hold the lock when call mCallback
                    mLock.unlock();
                    mCallback(mObject, WATCHDOG_MSG_TIMEOUT);
                    mLock.lock();
                }
                //restart timer
                mLastTickle = get_ms();
            }
            //ALOGD("watchdog delta =%llu",delta);
        }
    }
    mLock.unlock();

    usleep(CAMERA_WATCHDOG_SNAP_TIME*1000);
    return true;
}

};
