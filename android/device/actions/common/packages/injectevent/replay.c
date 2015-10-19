
#define  LOG_TAG    "inject_event_replay"
#include "uinput.h"


static const char *EV_PREFIX  = PREFIX;
static const char *IN_FN = FN;

static int out_fds[NUM_DEVICES] = {0};
static int num_events = 0;
static int in_fd = -1;
static int device_num = 0;

static int init()
{
    char buf[256];
    int i, fd, size;

    device_num = 0;

    for(i = 0; i < NUM_DEVICES; i++) {
        if(!is_kbd_input(i)) {
            if(get_input_name(i))
                LOGI("%s is not kbd input", get_input_name(i));
            continue;
        }
        sprintf(buf, "%sevent%d", EV_PREFIX, i);
        fd = open(buf, O_WRONLY | O_NDELAY);
        if(fd >= 0) {
            out_fds[device_num] = fd;
            device_num ++;
        } else {
            LOGE("open input device fail: %s", buf);
        }
    }

    if((in_fd = open(IN_FN, O_RDONLY)) < 0) {
        LOGE("Couldn't open input:%s, errno:%d\n", IN_FN, errno);
        return 3;
    }

    size = lseek(in_fd, 0, SEEK_END);
    LOGI("file:%s, size:%d\n", IN_FN, size);
    num_events = size / (sizeof(struct input_event) + sizeof(int));
    LOGI("number of events: %d\n", num_events);
    i = lseek(in_fd, 0, SEEK_SET);
    LOGI("current pos:%d\n", i);

    return 0;
}

static int deinit()
{
    int i;
    for(i = 0; i < device_num; i++) {
        close(out_fds[i]);
    }

    if(in_fd > 0) {
        close(in_fd);
    }
    return 0;
}

static int replay()
{
    struct timeval tdiff;
    struct input_event event;
    int i, outputdev;

    timerclear(&tdiff);

    for(i = 0; i < num_events; i++) {
        if(get_state() != S_REPLAYING) {
            LOGI("event replay interrupted by user");
            break;
        }
        struct timeval now, tevent, tsleep;

        if(read(in_fd, &outputdev, sizeof(outputdev)) != sizeof(outputdev)) {
            LOGE("Input read error00\n");
            return 1;
        }

        if(read(in_fd, &event, sizeof(event)) != sizeof(event)) {
            LOGE("Input read error11\n");
            return 2;
        }

        gettimeofday(&now, NULL);
        if (!timerisset(&tdiff)) {
            timersub(&now, &event.time, &tdiff);
        }

        timeradd(&event.time, &tdiff, &tevent);
        timersub(&tevent, &now, &tsleep);
        if (tsleep.tv_sec > 0 || tsleep.tv_usec > 100)
            select(0, NULL, NULL, NULL, &tsleep);

        event.time = tevent;

        if(write(out_fds[outputdev], &event, sizeof(event)) != sizeof(event)) {
            LOGE("Output write error\n");
            return 2;
        }

        //LOGI("input %d, time %ld.%06ld, type %d, code %d, value %d\n", outputdev,
        //        event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
    }

    return 0;
}

int start_replay(void)
{
    if(init() != 0) {
        LOGE("init failed\n");
        deinit();
        return 1;
    }

    LOGI("......start event replay............\n");
    if(replay() != 0) {
        LOGE("replay failed\n");
    }
    LOGI("......exit event replay............\n");
    deinit();
    return 0;
}

