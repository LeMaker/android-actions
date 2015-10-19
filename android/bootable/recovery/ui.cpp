/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cutils/android_reboot.h>

/*add by zhangc for hotplug start 20150529*/
#include <stdarg.h>
#include <cutils/uevent.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/fb.h>
#include <sys/poll.h>
#define MAXEVENT 32 // max event number
#define UEVENT_MSG_LEN  1024
/*add by zhangc for hotplug end 20150529*/

#include "common.h"
#include "roots.h"
#include "device.h"
#include "minui/minui.h"
#include "screen_ui.h"
#include "ui.h"
//ActionsCode(author:liaotianyang, type:newcode, comment:support tp)
#include "cutils/properties.h"

#define UI_WAIT_KEY_TIMEOUT_SEC    120

// There's only (at most) one of these objects, and global callbacks
// (for pthread_create, and the input event system) need to find it,
// so use a global variable.
static RecoveryUI* self = NULL;

RecoveryUI::RecoveryUI() :
    key_queue_len(0),
    key_last_down(-1),
    key_long_press(false),
    key_down_count(0),
    enable_reboot(true),
    consecutive_power_keys(0),
    consecutive_alternate_keys(0),
    last_key(-1) {
    pthread_mutex_init(&key_queue_mutex, NULL);
    pthread_cond_init(&key_queue_cond, NULL);
    self = this;
    memset(key_pressed, 0, sizeof(key_pressed));
    //ActionsCode(author:liaotianyang, type:newcode, comment:support tp)
    abs_scale[0] = abs_scale[1] = 0;	
}

void RecoveryUI::Init() {
    //ActionsCode(author:liaotianyang, type:changecode, comment:support tp)
    ev_init(input_callback, abs_scale);
	/*add by zhangc for hotplug start 20150529*/
    rec_uevent_init();
	/*add by zhangc for hotplug end 20150529*/
    pthread_create(&input_t, NULL, input_thread, NULL);
}


int RecoveryUI::input_callback(int fd, uint32_t epevents, void* data)
{
    struct input_event ev;
    int ret;

    ret = ev_get_input(fd, epevents, &ev);
    if (ret)
        return -1;

    if (ev.type == EV_SYN) {
    	//ActionsCode(author:liaotianyang, type:newcode, comment:support tp)
	 	self->abs_flag = 0;
        return 0;
    } else if (ev.type == EV_REL) {
        if (ev.code == REL_Y) {
            // accumulate the up or down motion reported by
            // the trackball.  When it exceeds a threshold
            // (positive or negative), fake an up/down
            // key event.
            self->rel_sum += ev.value;
			/*modify by zhangc 20150514 start*/
            if (self->rel_sum > 30) {
                self->process_key(KEY_DOWN, 1);   // press down key
                self->process_key(KEY_DOWN, 0);   // and release it
                self->rel_sum = 0;
            } else if (self->rel_sum < -30) {
                self->process_key(KEY_UP, 1);     // press up key
                self->process_key(KEY_UP, 0);     // and release it
                self->rel_sum = 0;
			/*modify by zhangc 20150514 end*/
            }
        }
    } else {
        self->rel_sum = 0;
		//ActionsCode(author:liaotianyang, type:newcode, comment:support tp)
		if ( ev.type == EV_ABS) {
			if ( ev.code == ABS_MT_POSITION_X	 || ev.code == ABS_X ) {
				self->abs_x = ev.value;
				self->abs_flag |= 0x01;
			} else if ( ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y ) {
				self->abs_y = ev.value;
				self->abs_flag |= 0x02;
			} else {
				self->abs_flag = 0;
			}	 
			if ( self->abs_flag == 0x03 ) {
				 //self->Print("x=%d,y=%d\n",self->abs_x, self->abs_y);
				 if ( self->tp_x != self->abs_x || self->tp_y != self->abs_y ) {
				     self->tp_x = self->abs_x;
				     self->tp_y = self->abs_y;			 	
				     self->process_key(BTN_TOUCH, 1);     // press up key
				     self->process_key(BTN_TOUCH, 0);     // and release it
				 }
			     self->abs_flag = 0;
			}		
		}
		// end 	 ActionsCode
    }

    if (ev.type == EV_KEY && ev.code <= KEY_MAX)
        self->process_key(ev.code, ev.value);

    return 0;
}

//ActionsCode(author:liaotianyang, type:newcode, comment:support tp)
static int recoveryMode = -1;
int get_recovery_mode(void)
{
    if(recoveryMode < 0) {
        char value[PROPERTY_VALUE_MAX+1];
        
        property_get("ro.boot.mode", value, NULL);
        if(memcmp(value, "recovery2", 9) == 0)
            recoveryMode = 1;
        else
            recoveryMode = 0;
    }
    
    return recoveryMode;
}
// end ActionsCode

// Process a key-up or -down event.  A key is "registered" when it is
// pressed and then released, with no other keypresses or releases in
// between.  Registered keys are passed to CheckKey() to see if it
// should trigger a visibility toggle, an immediate reboot, or be
// queued to be processed next time the foreground thread wants a key
// (eg, for the menu).
//
// We also keep track of which keys are currently down so that
// CheckKey can call IsKeyPressed to see what other keys are held when
// a key is registered.
//
// updown == 1 for key down events; 0 for key up events
void RecoveryUI::process_key(int key_code, int updown) {
    //ActionsCode(author:wurui, type:newcode, comment:support one key)
    struct timeval now;
    unsigned int msec;
    bool register_key = false;
    bool long_press = false;
    bool reboot_enabled;

    pthread_mutex_lock(&key_queue_mutex);
    key_pressed[key_code] = updown;
    if (updown) {		
        ++key_down_count;
        key_last_down = key_code;
        key_long_press = false;
        pthread_t th;
        key_timer_t* info = new key_timer_t;
        info->ui = this;
        info->key_code = key_code;
        info->count = key_down_count;
        pthread_create(&th, NULL, &RecoveryUI::time_key_helper, info);
        pthread_detach(th);
        //ActionsCode(author:liaotianyang, type:newcode, comment:support one key)
        if(get_recovery_mode() == 1 && key_last_down == KEY_POWER)
            gettimeofday(&key_power_down, NULL);
		// end ActionsCode
    } else {
        if (key_last_down == key_code) {
            long_press = key_long_press;
            register_key = true;			
            //ActionsCode(author:wurui, type:newcode, comment:support one key)
            if(get_recovery_mode() == 1 && key_code == KEY_POWER) {
                gettimeofday(&now, NULL);
                msec = ((now.tv_sec - key_power_down.tv_sec) * 1000) 
                    + ((now.tv_usec - key_power_down.tv_usec) / 1000);		
                if(msec > 2000)
                    key_code = KEY_POWER;
                else
                    key_code = KEY_DOWN;
            }
			//end ActionsCode
	/*add by zhangc 20150514 start support for mouse and keyboard*/
			if(key_code == BTN_LEFT || key_code == BTN_RIGHT || key_code == KEY_ENTER)
			{
				key_code = KEY_POWER;
			}
	/*add by zhangc 20150514 end*/
        }
        key_last_down = -1;
    }
    reboot_enabled = enable_reboot;
    pthread_mutex_unlock(&key_queue_mutex);

    if (register_key) {
        NextCheckKeyIsLong(long_press);
        switch (CheckKey(key_code)) {
          case RecoveryUI::IGNORE:
            break;

          case RecoveryUI::TOGGLE:
            ShowText(!IsTextVisible());
            break;

          case RecoveryUI::REBOOT:
            if (reboot_enabled) {
            	//ActionsCode(author:wurui, type:changecode, comment:support reboot)
            	//android_reboot(ANDROID_RB_RESTART, 0, 0);
            	android_reboot(ANDROID_RB_RESTART2, 0, (char*)"reboot");
            }
            break;

          case RecoveryUI::ENQUEUE:
            EnqueueKey(key_code);
            break;

          case RecoveryUI::MOUNT_SYSTEM:
#ifndef NO_RECOVERY_MOUNT
            ensure_path_mounted("/system");
            Print("Mounted /system.");
#endif
            break;
        }
    }
}

void* RecoveryUI::time_key_helper(void* cookie) {
    key_timer_t* info = (key_timer_t*) cookie;
    info->ui->time_key(info->key_code, info->count);
    delete info;
    return NULL;
}

void RecoveryUI::time_key(int key_code, int count) {
    usleep(750000);  // 750 ms == "long"
    bool long_press = false;
    pthread_mutex_lock(&key_queue_mutex);
    if (key_last_down == key_code && key_down_count == count) {
        long_press = key_long_press = true;
    }
    pthread_mutex_unlock(&key_queue_mutex);
    if (long_press) KeyLongPress(key_code);
}

void RecoveryUI::EnqueueKey(int key_code) {
    pthread_mutex_lock(&key_queue_mutex);
    const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
    if (key_queue_len < queue_max) {
        key_queue[key_queue_len++] = key_code;
        pthread_cond_signal(&key_queue_cond);
    }
    pthread_mutex_unlock(&key_queue_mutex);
}


// Reads input events, handles special hot keys, and adds to the key queue.
void* RecoveryUI::input_thread(void *cookie)
{
    for (;;) {
        if (!ev_wait(-1))
            ev_dispatch();
    }
    return NULL;
}

int RecoveryUI::WaitKey()
{
    pthread_mutex_lock(&key_queue_mutex);

    // Time out after UI_WAIT_KEY_TIMEOUT_SEC, unless a USB cable is
    // plugged in.
    do {
        struct timeval now;
        struct timespec timeout;
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec;
        timeout.tv_nsec = now.tv_usec * 1000;
        timeout.tv_sec += UI_WAIT_KEY_TIMEOUT_SEC;

        int rc = 0;
        while (key_queue_len == 0 && rc != ETIMEDOUT) {
            rc = pthread_cond_timedwait(&key_queue_cond, &key_queue_mutex,
                                        &timeout);
        }
    } while (usb_connected() && key_queue_len == 0);

    int key = -1;
    if (key_queue_len > 0) {
        key = key_queue[0];
        memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    }
    pthread_mutex_unlock(&key_queue_mutex);
    return key;
}

// Return true if USB is connected.
bool RecoveryUI::usb_connected() {
    int fd = open("/sys/class/android_usb/android0/state", O_RDONLY);
    if (fd < 0) {
        printf("failed to open /sys/class/android_usb/android0/state: %s\n",
               strerror(errno));
        return 0;
    }

    char buf;
    /* USB is connected if android_usb state is CONNECTED or CONFIGURED */
    int connected = (read(fd, &buf, 1) == 1) && (buf == 'C');
    if (close(fd) < 0) {
        printf("failed to close /sys/class/android_usb/android0/state: %s\n",
               strerror(errno));
    }
    return connected;
}

bool RecoveryUI::IsKeyPressed(int key)
{
    pthread_mutex_lock(&key_queue_mutex);
    int pressed = key_pressed[key];
    pthread_mutex_unlock(&key_queue_mutex);
    return pressed;
}

void RecoveryUI::FlushKeys() {
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}

// The default CheckKey implementation assumes the device has power,
// volume up, and volume down keys.
//
// - Hold power and press vol-up to toggle display.
// - Press power seven times in a row to reboot.
// - Alternate vol-up and vol-down seven times to mount /system.
RecoveryUI::KeyAction RecoveryUI::CheckKey(int key) {

	
	#if 0
    if ((IsKeyPressed(KEY_POWER) && key == KEY_VOLUMEUP) || key == KEY_HOME) {
        return TOGGLE;
    }

    if (key == KEY_POWER) {
        pthread_mutex_lock(&key_queue_mutex);
        bool reboot_enabled = enable_reboot;
        pthread_mutex_unlock(&key_queue_mutex);

        if (reboot_enabled) {
            ++consecutive_power_keys;
            if (consecutive_power_keys >= 7) {
                return REBOOT;
            }
        }
    } else {
        consecutive_power_keys = 0;
    }

    if ((key == KEY_VOLUMEUP &&
         (last_key == KEY_VOLUMEDOWN || last_key == -1)) ||
        (key == KEY_VOLUMEDOWN &&
         (last_key == KEY_VOLUMEUP || last_key == -1))) {
        ++consecutive_alternate_keys;
        if (consecutive_alternate_keys >= 7) {
            consecutive_alternate_keys = 0;
            return MOUNT_SYSTEM;
        }
    } else {
        consecutive_alternate_keys = 0;
    }
    last_key = key;
	#endif

    return ENQUEUE;
}

void RecoveryUI::NextCheckKeyIsLong(bool is_long_press) {
}

void RecoveryUI::KeyLongPress(int key) {
}

void RecoveryUI::SetEnableReboot(bool enabled) {
    pthread_mutex_lock(&key_queue_mutex);
    enable_reboot = enabled;
    pthread_mutex_unlock(&key_queue_mutex);
}


/*add by zhangc for hotplug start 20150529*/

void RecoveryUI::parse_rec_uevent(const char *msg, rec_uevent *uevent)
{
	uevent->action = "";
    uevent->subsystem = "";
    uevent->dev_name = "";

    /* currently ignoring SEQNUM */
    while (*msg) {
#ifdef DEBUG_UEVENTS
       LOGV("uevent str: %s\n", msg);
#endif
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if (!strncmp(msg, "DEVNAME=", 8)) {
            msg += 8;
            uevent->dev_name = msg;
        }

        /* advance to after the next \0 */
        while (*msg++)
            ;
    }

    //printf("event { '%s', '%s', '%s' }\n", uevent->action, uevent->subsystem, uevent->dev_name);
}

void RecoveryUI::process_ps_rec_uevent(rec_uevent *uevent)
{    
    int fd[MAXEVENT];
    int fd_num;
    char filepath[32];
	int i;
    int retry = 3;	
	usleep(200);//wait file site create
	
    if(!strncmp(uevent->dev_name,"input/event",11))
		//i want to find /dev/input/event*
	{
		sprintf(filepath, "/dev/%s", uevent->dev_name);
		if (!strcmp(uevent->action, "add"))
		{
			printf("##pull in !!!\n");
			fd_num = atoi(&uevent->dev_name[11]);			
			for(i=0;i<retry;i++)
			{
				fd[fd_num] = open(filepath,O_RDONLY);
				if(fd[fd_num] < 0)
				{
					printf("open %s fail!!! retry=%d\n",filepath,i);
					usleep(200);
				}			
				else{
					int ret = ev_add_fd(fd[fd_num], input_callback, NULL);
					printf("open %s success!!!\nev_add_fd return:%d\n",filepath, ret);
					break;
				}
			}
		}
		
		if (!strcmp(uevent->action, "remove"))
		{
			fd_num = atoi(&uevent->dev_name[11]);
			printf("##pull out!!\nclose %s !!!\n",filepath);
			close(fd[fd_num]);
		}
			
	}
}

void RecoveryUI::process_rec_uevent(rec_uevent *uevent)
{
    if (!strcmp(uevent->subsystem, "input"))
	{
		LOGI("event { '%s', '%s', '%s' }\n", uevent->action, uevent->subsystem, uevent->dev_name);
        process_ps_rec_uevent(uevent);
	}

}

int RecoveryUI::handle_rec_uevent_fd(int fd)
{
    char msg[UEVENT_MSG_LEN+2];
    int n;

    if (fd < 0)
        return -1;

    while (true) {
        rec_uevent uevent;

        n = uevent_kernel_multicast_recv(fd, msg, UEVENT_MSG_LEN);
        if (n <= 0)
            break;
        if (n >= UEVENT_MSG_LEN)   /* overflow -- discard */
            continue;

        msg[n] = '\0';
        msg[n+1] = '\0';

        parse_rec_uevent(msg, &uevent);
        process_rec_uevent(&uevent);
    }

    return 0;
}

int RecoveryUI::rec_uevent_callback(int fd, uint32_t revents, void *data)
{

    if (!(revents & POLLIN))
        return -1;
    return handle_rec_uevent_fd(fd);
}

int RecoveryUI::rec_uevent_init(void)
{
	int fd;
    fd = uevent_open_socket(64*1024, true);
    if (fd >= 0) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        ev_add_fd(fd, rec_uevent_callback, NULL);
    }
	return 0;
}
/*add by zhangc for hotplug end 20150529*/