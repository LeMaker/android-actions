#include "watch_dog.h"
#include "log.h"
#include "perf.h"
#include <pthread.h>

typedef enum
{
    WDOG_IDLE,
    WDOG_BUSY,
    WDOG_DESTROY,
} dog_stat_t;

typedef struct
{
    pthread_t thread_loop;
    pthread_mutex_t cmd_mutex;
    dog_stat_t stat;
    int snap_cnt;
    long long last_tickle_time;
    char last_tickle_info[128];
} dog_t;


static void *watch_dog_loop(void *param)
{
    dog_t *dog_param = (dog_t *)param;
    long long current_time;
    long long delta_time;

    while(1)
    {
        pthread_mutex_lock(&dog_param->cmd_mutex);
        
        if(WDOG_BUSY == dog_param->stat)
        {
            dog_param->snap_cnt++;
            if(dog_param->snap_cnt > WDOG_WARN_SNAP_NUM)
            {
                current_time = get_current_time();
                delta_time = current_time - dog_param->last_tickle_time;

                OMXDBUG(OMXDBUG_PARAM, "dog warning, %s, %lld(us)\n", dog_param->last_tickle_info, delta_time);
                if(delta_time > WDOG_TIME_OUT * 1000)
                {
                    OMXDBUG(OMXDBUG_ERR, "dog timeout, %s!\n", dog_param->last_tickle_info);
                    dog_param->last_tickle_time = current_time;
                    dog_param->snap_cnt = 0;
                }
            }
        }
        else if(WDOG_DESTROY == dog_param->stat)
        {
            OMXDBUG(OMXDBUG_PARAM, "dog stopped, loop out\n");
            pthread_mutex_unlock(&dog_param->cmd_mutex);
            break;
        }

        pthread_mutex_unlock(&dog_param->cmd_mutex);

        usleep(WDOG_SNAP_TIME * 1000);
    }

    return NULL;
}

int tickle_watch_dog(void *handle, char *info)
{
    dog_t *dog_param = (dog_t *)handle;
    if(NULL == dog_param)
    {
        OMXDBUG(OMXDBUG_ERR, "not open yet!\n");
        return -1;
    }

    pthread_mutex_lock(&dog_param->cmd_mutex);
    dog_param->last_tickle_time = get_current_time();
    dog_param->snap_cnt = 0;
    memcpy(dog_param->last_tickle_info, info, strlen(info) + 1);
    pthread_mutex_unlock(&dog_param->cmd_mutex);

    OMXDBUG(OMXDBUG_VERB, "%s in %s\n", __func__, dog_param->last_tickle_info);
    return 0;
}

void *open_watch_dog(void)
{
    pthread_attr_t thread_attr;
    struct sched_param thread_params;
    dog_t *dog_param = (dog_t *)calloc(1, sizeof(dog_t));
    if(NULL == dog_param)
    {
        OMXDBUG(OMXDBUG_ERR, "alloc failed!\n");
        return NULL;
    }

    dog_param->stat = WDOG_IDLE;
    pthread_mutex_init(&dog_param->cmd_mutex, NULL);

    pthread_attr_init(&thread_attr);
    //pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_getschedparam(&thread_attr, &thread_params);
    thread_params.sched_priority = (int) - 9;
    pthread_attr_setschedparam(&thread_attr, &thread_params);
    pthread_create(&dog_param->thread_loop, NULL/*&thread_attr*/, watch_dog_loop, dog_param);

    // performance test
    {
//	        long long t0 = get_current_time();
//	        long long t1;
//	        int loop_cnt = 1000;
//	        while(loop_cnt--)
//	        { get_current_time(); }
//	        t1 = get_current_time();
//	        OMXDBUG(OMXDBUG_PARAM, "watch dog performance: %lld!\n", t1 - t0);

//	        double t0 = now_ms();
//	        double t1;
//	        int loop_cnt = 1000;
//	        while(loop_cnt--)
//	        { now_ms(); }
//	        t1 = now_ms();
//	        OMXDBUG(OMXDBUG_PARAM, "watch dog performance: %g ms!\n", t1 - t0);
    }

    OMXDBUG(OMXDBUG_PARAM, "watch dog opened!\n");
    return (void *)dog_param;
}

int close_watch_dog(void *handle)
{
    dog_t *dog_param = (dog_t *)handle;
    if(NULL == dog_param)
    {
        OMXDBUG(OMXDBUG_ERR, "not open yet!\n");
        return -1;
    }

    pthread_mutex_lock(&dog_param->cmd_mutex);
    dog_param->stat = WDOG_DESTROY;
    pthread_mutex_unlock(&dog_param->cmd_mutex);

    pthread_join(dog_param->thread_loop, NULL);     // 
    pthread_mutex_destroy(&dog_param->cmd_mutex);

    free(dog_param); // set app's handle to NULL!!!

    OMXDBUG(OMXDBUG_PARAM, "watch dog closed!\n");
    return 0;
}

int start_watch_dog(void *handle)
{
    dog_t *dog_param = (dog_t *)handle;
    if(NULL == dog_param)
    {
        OMXDBUG(OMXDBUG_ERR, "not open yet!\n");
        return -1;
    }

    pthread_mutex_lock(&dog_param->cmd_mutex);
    dog_param->last_tickle_time = get_current_time();
    dog_param->snap_cnt = 0;
    dog_param->stat = WDOG_BUSY;
    dog_param->last_tickle_info[0] = '\0';
    pthread_mutex_unlock(&dog_param->cmd_mutex);

    OMXDBUG(OMXDBUG_PARAM, "watch dog started!\n");
    return 0;
}

int stop_watch_dog(void *handle)
{
    dog_t *dog_param = (dog_t *)handle;
    if(NULL == dog_param)
    {
        OMXDBUG(OMXDBUG_ERR, "not open yet!\n");
        return -1;
    }
    
    pthread_mutex_lock(&dog_param->cmd_mutex);
    dog_param->stat = WDOG_IDLE;
    pthread_mutex_unlock(&dog_param->cmd_mutex);

    OMXDBUG(OMXDBUG_PARAM, "watch dog stopped!\n");
    return 0;
}

