
#define  LOG_TAG    "inject_event_record"
#include "uinput.h"

static const char *EV_PREFIX  = PREFIX;
static const char *OUT_FN = FN;

static struct pollfd in_fds[NUM_DEVICES];
static int eventIds[NUM_DEVICES];
/*
int out_fds[NUM_DEVICES];
*/
static int out_fd;
static int device_num = 0;

static int init()
{
    char buffer[256];
    int i, fd;

    out_fd = open(OUT_FN, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXO);
    if(out_fd < 0) {
        LOGE("Couldn't open output file:%s, errno:%d\n", OUT_FN, errno);
        return 1;
    }

    device_num = 0;

    for(i = 0; i < NUM_DEVICES; i++) {
        if(!is_kbd_input(i)) {
            if(get_input_name(i))
                LOGI("%s is not kbd input", get_input_name(i));
            continue;
        }
        sprintf(buffer, "%sevent%d", EV_PREFIX, i);
        fd = open(buffer, O_RDONLY | O_NDELAY);
        if(fd >= 0) {
            in_fds[device_num].events = POLLIN;
            in_fds[device_num].fd = fd;
            eventIds[device_num] = i;
            device_num ++;
        } else {
            LOGE("open input device fail: %s", buffer);
        }
    }
    return 0;
}

static int deinit()
{
    int i;
    for(i = 0; i < device_num; i++) {
        close(in_fds[i].fd);
    }

    if(out_fd > 0) {
        close(out_fd);
    }
    return 0;
}

static int record()
{
    int i, numread;
    struct input_event event;

    while(get_state() == S_RECORDING) {
        int pret = poll(in_fds, device_num, 500);
        if(pret <= 0) {
            continue;
        }

        for(i = 0; i < device_num; i++) {
            if(in_fds[i].revents & POLLIN) {
                /* Data available */
                numread = read(in_fds[i].fd, &event, sizeof(event));
                if(numread != sizeof(event)) {
                    LOGE("Read error00\n");
                    continue;
                }
                if(write(out_fd, &i, sizeof(i)) != sizeof(i)) {
                    LOGE("Write error11\n");
                    continue;
                }
                if(write(out_fd, &event, sizeof(event)) != sizeof(event)) {
                    LOGE("Write error22\n");
                    continue;
                }

                if(!empty_key(&event)) {
                	switch(event.value) {
                	case 0: // up
                		LOGI("key up! code:%d\n", event.code);
                		break;
                	case 1: // down
                		LOGI("key down! code:%d\n", event.code);
                		break;
                	case 2: // long down
                	    LOGI("key long down! code:%d\n", event.code);
                		break;
                	default:
                		break;
                	}
                }

                LOGI("%s time %ld.%06ld, type %d, code %d, value %d\n",
                        get_input_name(eventIds[i]),
                        event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);
            }
        }
    }
    return 0;
}

int start_record(void)
{
    if (init() != 0) {
        LOGE("Init failed");
        deinit();
        return 1;
    }

    LOGI("......start event record............\n");

    record();

    LOGI("......exit event record............\n");
    deinit();
    return 0;
}


