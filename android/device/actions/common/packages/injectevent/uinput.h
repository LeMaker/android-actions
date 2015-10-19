#ifndef __UINPUT_H__
#define __UINPUT_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/input.h>
#include <linux/time.h>
#include <android/log.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


#ifdef LOG_TAG
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI  printf
#define  LOGE  printf
#endif


#define PREFIX  "/dev/input/"
#define BASE_DIR "/data/injectevent"
#define FN      BASE_DIR"/events"

/* NB event4 is the compass -- not required for tests. */
//#define DEVICES {"event0", "event1", "event2", "event3", "event4"}
#define NUM_DEVICES 5  // event0-event4

// key value define
#define KEY_HOME        102
#define KEY_VOLUME_ADD  115
#define KEY_VOLUME_SUB  114

typedef enum {
    S_IDLE = 0,
    S_RECORDING,
    S_REPLAYING,
}RUN_STATE;

int empty_key(struct input_event *event);
int start_record();
int start_replay();
int get_state();
int is_kbd_input(int id);
char *get_input_name(int id);

#endif // #ifndef __UINPUT_H__

