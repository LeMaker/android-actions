/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>


#include "AccelerationSensor.h"


/*****************************************************************************/
AccelerationSensor::AccelerationSensor(char* name, float resolution, int minDelay)
    : SensorBase(NULL, name),
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false),
      
      mName(name),
      mResolution(resolution),
      mMinDelay(minDelay)
{
    ALOGD("AccelerationSensor::AccelerationSensor()");
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    	
    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);

        //enable(0, 1);
    }
}

AccelerationSensor::~AccelerationSensor() {

    ALOGD("AccelerationSensor::~AccelerationSensor()");
    if (mEnabled) {
        enable(0, 0);
    }
}



int AccelerationSensor::enable(int32_t, int en) {

	   
    ALOGD("AccelerationSensor::~enable(0, %d)", en);
    int flags = en ? 1 : 0;
    if (flags != mEnabled) {
        int fd;
        strcpy(&input_sysfs_path[input_sysfs_path_len], "enable");
        fd = open(input_sysfs_path, O_RDWR);
        if (fd >= 0) {
            char buf[2];
            int err;
            buf[1] = 0;
            if (flags) {
                buf[0] = '1';
            } else {
                buf[0] = '0';
            }
            err = write(fd, buf, sizeof(buf));
            close(fd);
            mEnabled = flags;
            //setInitialState();
            return 0;
        }
        return -1;        
    }
    return 0;
}


bool AccelerationSensor::hasPendingEvents() const {
    /* FIXME probably here should be returning mEnabled but instead
	mHasPendingEvents. It does not work, so we cheat.*/
    //ALOGD("AccelerationSensor::~hasPendingEvents %d", mHasPendingEvent ? 1 : 0 );
    return mHasPendingEvent;
}


int AccelerationSensor::setDelay(int32_t handle, int64_t ns)
{
    ALOGD("AccelerationSensor::~setDelay(%d, %lld)", handle, ns);

    int fd;

    if (ns < (mMinDelay * 1000)) {
        ns = (mMinDelay * 1000); // Minimum on stock
    }

    strcpy(&input_sysfs_path[input_sysfs_path_len], "delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns / 10000000 * 10); // Some flooring to match stock value
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}


int AccelerationSensor::readEvents(sensors_event_t* data, int count)
{
    //ALOGD("AccelerationSensor::~readEvents() %d", count);
    if (count < 1)
        return -EINVAL;
        
    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }
        
    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;
	
    while (count && mInputReader.readEvent(&event)) {

	 if(event == NULL)
	 {
	     ALOGE("llw AccelerationSensor::readEvents()  null pointer!!!" );
	 }
	 else
	 {	
            int type = event->type;
            if (type == EV_ABS) {
                float value = event->value;
                if (event->code == EVENT_TYPE_ACCEL_X) {
                    mPendingEvent.acceleration.x = value * mResolution;
                } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                    mPendingEvent.acceleration.y = value * mResolution;
                } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                    mPendingEvent.acceleration.z = value * mResolution;
                }
            } else if (type == EV_SYN) {
                mPendingEvent.timestamp = timevalToNano(event->time);
                if (mEnabled) {
                    *data++ = mPendingEvent;
                    count--;
                    numEventReceived++;
                }
            } else {
                ALOGE("AccelerationSensor: unknown event (type=%d, code=%d)",
                        type, event->code);
            }
    	 }
        mInputReader.next();
    }
 
	//ALOGD("AccelerationSensor::~readEvents() numEventReceived = %d", numEventReceived);
	return numEventReceived;
		
}
