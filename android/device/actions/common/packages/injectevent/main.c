
#define  LOG_TAG    "inject_event_main"
#include "uinput.h"
#include <semaphore.h>

#define CHECK_INTERVAL_US  300000
#define SEM_NAME       "IndectEvent_sem"

static sem_t s_sem;
static pthread_t s_thread_id = -1;
static int s_cur_state = S_IDLE;

int empty_key(struct input_event *event)
{
    //return (event->type == 0 && event->code == 0 && event->value == 0);
    return (event->type == 0);
}

int get_state()
{
    int ret = S_IDLE;
    sem_wait(&s_sem);
    ret = s_cur_state;
    sem_post(&s_sem);
    return ret;
}

static void set_state(int state)
{
    sem_wait(&s_sem);
    s_cur_state = state;
    sem_post(&s_sem);
}

static void *check_state(void *ptr)
{
    LOGI("check state thread start");
    while(1) {
        int fd = open(BASE_DIR"/state", O_RDONLY);
        if(fd < 0) {
            LOGI("open"BASE_DIR"/state fail, errno:%d", errno);
            usleep(CHECK_INTERVAL_US);
            continue;
        }
        char sstete[4] = {0};
        memset(sstete, 0, sizeof(sstete));
        read(fd, sstete, 2);
        close(fd);
        sstete[1] = 0;
        int state = atoi(sstete);
        if(state != S_RECORDING && state != S_REPLAYING) {
            state = S_IDLE;
        }
        set_state(state);
        usleep(CHECK_INTERVAL_US);
    }
    return NULL;
}


static int init()
{
    system("mkdir "BASE_DIR" > /dev/null");
    if (sem_init(&s_sem, 0, 1) == -1) {
        LOGE("init sem fail!, errno:%d\n", errno);
        return -1;
    }
    pthread_create(&s_thread_id, NULL, check_state, NULL);
    LOGI("thread id:%d", (int)s_thread_id);
    return 0;
}

int main(void)
{
    LOGI("start inject event");
    if(init() < 0) {
        LOGE("init error, end inject event");
        return -1;
    }

    while(1) {
        int state = get_state();
        switch(state) {
        case S_RECORDING:
            if(start_record()) {
                usleep(500000);
            }
            break;
        case S_REPLAYING:
            if(start_replay()) {
                usleep(500000);
            }
            break;
        case S_IDLE:
        default:
            usleep(500000);
            break;
        }
    }

    LOGI("end inject event");
    return 0;
}


