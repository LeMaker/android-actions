#ifndef __CAMERA_WATCHDOG__
#define __CAMERA_WATCHDOG__

#include <utils/Thread.h>

namespace android
{

#define CAMERA_WATCHDOG_SNAP_TIME (40) //40ms
#define CAMERA_WATCHDOG_PERIOD (800) //800ms

#define CAMERA_WATCHDOG_TIMEOUT (8*1000) //8s

#define CAMERA_WATCHDOG_PERIOD_MAX_COUNT (CAMERA_WATCHDOG_PERIOD/CAMERA_WATCHDOG_SNAP_TIME)

typedef void (*WatchdogCallback)(void *obj, unsigned int msg);

class CameraWatchDog : public Thread
{
public:
    CameraWatchDog();
    virtual ~CameraWatchDog();
    virtual bool threadLoop()
    {
        bool ret;
        ret = Handler();
        return ret;
    }

    void tickle();
    bool startWatchDog(void *obj, WatchdogCallback cb);
    bool stopWatchDog();

    enum WatchDogState
    {
        WATCHDOG_IDLE = 0,
        WATCHDOG_RUNNING,
    };

    enum WatchDogMsg
    {
        WATCHDOG_MSG_TIMEOUT = 0,
    };
private:
    bool Handler();
    void Init();
    void Init(void *obj, WatchdogCallback cb);
    WatchdogCallback mCallback;
    void *mObject;
    unsigned long long mLastTickle;
    int mState;
    int mPeriodCount;
    Mutex mLock;
};

};

#endif
