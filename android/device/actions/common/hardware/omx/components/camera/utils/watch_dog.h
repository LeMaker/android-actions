#ifndef OMX_CAMERA_WATCH_DOG
#define OMX_CAMERA_WATCH_DOG


#define WDOG_SNAP_TIME 40
#define WDOG_WARN_TIME 1000
#define WDOG_TIME_OUT  3000
#define WDOG_WARN_SNAP_NUM (WDOG_WARN_TIME/WDOG_SNAP_TIME)

void *open_watch_dog(void);
int close_watch_dog(void *handle);
int start_watch_dog(void *handle);
int stop_watch_dog(void *handle);
int tickle_watch_dog(void *handle, char *info);

#endif
