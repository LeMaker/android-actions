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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>


#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"

#include "LightSensor.h"
//#include "ProximitySensor.h"
//#include "BoschYamaha.h"
#include "AccelerationSensor.h"
#include "CompassSensor.h"
#include "GyroSensor.h"
#include "TemperatureSensor.h"
//#include "OrientationSensor.h"

/*****************************************************************************/

#define DELAY_OUT_TIME 0x7FFFFFFF

#define LIGHT_SENSOR_POLLTIME    2000000000


#define SENSORS_ACCELERATION     (1<<ID_A)
#define SENSORS_MAGNETIC_FIELD   (1<<ID_M)
#define SENSORS_ORIENTATION      (1<<ID_O)
#define SENSORS_LIGHT            (1<<ID_L)
#define SENSORS_PROXIMITY        (1<<ID_P)
#define SENSORS_GYROSCOPE        (1<<ID_GY)
#define SENSORS_TEMPERATURE      (1<<ID_T)

#define SENSORS_ACCELERATION_HANDLE     ID_A
#define SENSORS_MAGNETIC_FIELD_HANDLE   ID_M
#define SENSORS_ORIENTATION_HANDLE      ID_O
#define SENSORS_LIGHT_HANDLE            ID_L
#define SENSORS_PROXIMITY_HANDLE        ID_P
#define SENSORS_GYROSCOPE_HANDLE        ID_GY
#define SENSORS_TEMPERATURE_HANDLE      ID_T

#define AKM_FTRACE 0
#define AKM_DEBUG 0
#define AKM_DATA 0

#define MAX_SENSOR_NUM                  16

/*****************************************************************************/

/* Support SENSORS Module */
static const struct sensor_t sSensorSupportList[] = {   
	{ 
          "STK8312 Thermometer",
          "SensorTek",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (6.0f * GRAVITY_EARTH), 
          9.81f/21.34f,    // -8G ~ +8G, 14 bit
          0.177f, 10000, 0,0,
          NULL, NULL, 0, 0,
          { (void*)"stk8312" }    // reserved[0] --> input device name
        },
        { 
          "BMA220 3-axis Accelerometer",
          "Bosch Sensortec",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 64.0f,     // -2G ~ +2G, 6 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"bma220" }    // reserved[0] --> input device name
        }, 
        { 
          "BMA222 3-axis Accelerometer",
          "Bosch Sensortec",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 256.0f,     // -2G ~ +2G, 8 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"bma222" }    // reserved[0] --> input device name
        },
        { 
          "BMA223 3-axis Accelerometer",
          "Bosch Sensortec",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 256.0f,     // -2G ~ +2G, 8 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"bma223" }    // reserved[0] --> input device name
        },
        {
          "LIS3DH 3-axis Accelerometer",
          "lis3dh Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER,
          (4.0f * GRAVITY_EARTH),
          (4.0f * GRAVITY_EARTH) / 4096.0f,     // -2G ~ +2G, 12 bit
          0.20f, 10000, 0, 0,
          NULL, NULL, 0, 0,
          { (void*)"lis3dh_acc" }    // reserved[0] --> input device name
        },
        { 
          "LIS3DH 3-axis Accelerometer",
          "lis3dh Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 4096.0f,     // -2G ~ +2G, 12 bit
          0.20f, 10000, 0,0,
          NULL, NULL, 0, 0,
          { (void*)"lis3dh" }    // reserved[0] --> input device name
        },
         {
         	"kionix 3-axis Accelerometer",
          "kionix Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 4096.0f,     // -2G ~ +2G, 12 bit
          0.20f, 10000, 0, 0,
          NULL, NULL, 0, 0,
         { (void*)"kionix_accel" }    // reserved[0] --> input device name
        },
        { 
          "BMA250 3-axis Accelerometer",
          "Bosch Sensortec",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 1024.0f,     // -2G ~ +2G, 10 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"bma250" }    // reserved[0] --> input device name
        },
        { 
          "MMA7660 3-axis Accelerometer",
          "Freescale Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (3.0f * GRAVITY_EARTH), 
          (3.0f * GRAVITY_EARTH) / 64.0f,     // -1.5G ~ +1.5G, 6 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mma7660" }    // reserved[0] --> input device name
        },
        { 
          "MMA8452 3-axis Accelerometer",
          "Freescale Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 4096.0f,     // -2G ~ +2G, 12 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mma8452" }    // reserved[0] --> input device name
        },
        { 
          "LIS3DH 3-axis Accelerometer",
          "lis3dh Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 4096.0f,     // -2G ~ +2G, 12 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"lis3dh_acc" }    // reserved[0] --> input device name
        },
        { 
          "DMARD10 3-axis Accelerometer",
          "DMT",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (8.0f * GRAVITY_EARTH), 
          (8.0f * GRAVITY_EARTH) / 1024.0f,     // -4G ~ +4G, 10 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"dmard10" }    // reserved[0] --> input device name
        },
        { 
          "MC3230 3-axis Accelerometer",
          "mCube",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (3.0f * GRAVITY_EARTH), 
          (3.0f * GRAVITY_EARTH) / 256.0f,    // -1.5G ~ +1.5G, 8 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mc3230" }    // reserved[0] --> input device name
        },
        { 
          "MC3210 3-axis Accelerometer",
          "mCube",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (16.0f * GRAVITY_EARTH), 
          (16.0f * GRAVITY_EARTH) / 16384.0f,    // -8G ~ +8G, 14 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mc3210" }    // reserved[0] --> input device name
        },
        { 
          "MC3236 3-axis Accelerometer",
          "mCube",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (4.0f * GRAVITY_EARTH), 
          (4.0f * GRAVITY_EARTH) / 256.0f,    // -2G ~ +2G, 8 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mc3236" }    // reserved[0] --> input device name

        },
        { 
          "STK8312 3-axis Accelerometer",
          "STK",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (6.0f * GRAVITY_EARTH), 
          (1.0f * GRAVITY_EARTH) / 21.34f,    // -1.5G ~ +1.5G, 8 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"stk8312" }    // reserved[0] --> input device name
        },

        { 
          "STK8313 3-axis Accelerometer",
          "STK",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER, 
          (8.0f * GRAVITY_EARTH), 
          (1.0f * GRAVITY_EARTH) / 256.0f,    // -8G ~ +8G, 14 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"stk8313" }    // reserved[0] --> input device name
        },

        { 
          "LTR-301 Light sensor",
          "LITE-ON",
          1, SENSORS_LIGHT_HANDLE,
          SENSOR_TYPE_LIGHT, 
          64000.0f,     // 2 ~ 64k
          1.0f,
          0.20f, 500, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"ltr301" }    // reserved[0] --> input device name
        },
        { 
          "MC6420 3-axis Magnetic field sensor",
          "mCube",
          1, SENSORS_MAGNETIC_FIELD_HANDLE,
          SENSOR_TYPE_MAGNETIC_FIELD, 
          400.0f,           // -200uT ~ +200uT
          1.0f / 80.0f,    // 80 LSB/uT
          0.5f, 10, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mc6420" }    // reserved[0] --> input device name
        },	
        { 
          "L3G4200D Gyroscope sensor",
          "ST Microelectronics",
          1, SENSORS_GYROSCOPE_HANDLE,
          SENSOR_TYPE_GYROSCOPE, 
          (4000.0f*(float)M_PI/180.0f),         // -2000dps ~ +2000dps
          ((70.0f / 1000.0f) * ((float)M_PI / 180.0f)),    // 70 mdps/LSB
          6.1f, 10, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"l3g4200d" }    // reserved[0] --> input device name
        },
        { 
          "MPU3050C Gyroscope sensor",
          "InvenSense Inc.",
          1, SENSORS_GYROSCOPE_HANDLE,
          SENSOR_TYPE_GYROSCOPE, 
          (4000.0f*(float)M_PI/180.0f),         // -2000dps ~ +2000dps
          ((1.0f / 16.4f) * ((float)M_PI / 180.0f)),    // 1/16.4 dps/LSB
          6.1f, 10, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"mpu3050c" }    // reserved[0] --> input device name
        },
        {
          "Mir3da 3-axis Accelerometer",
          "MiraMEMS Semi",
          1, SENSORS_ACCELERATION_HANDLE,
          SENSOR_TYPE_ACCELEROMETER,
          (4.0f * GRAVITY_EARTH),
          (4.0f * GRAVITY_EARTH) / 4096.0f,     // -2G ~ +2G, 12 bit
          0.20f, 10000,0,0,
          NULL, NULL, 0, 0,
          { (void*)"mir3da" }    // reserved[0] --> input device name
        },
	{ 
          "BMA250 Thermometer",
          "Bosch Sensortec",
          1, SENSORS_TEMPERATURE_HANDLE,
          SENSOR_TYPE_TEMPERATURE, 
          128.0f,        // -40 ~ +87.5 centigrade
          0.5f,     // 0.5 centigrade/LSB, 8 bit
          0.20f, 10000, 0, 0, 
          NULL, NULL, 0, 0,
          { (void*)"bma250t" }    // reserved[0] --> input device name
        },
	
};

/* Current SENSORS Module */
static struct sensor_t sSensorList[MAX_SENSOR_NUM] = {};
static int sSensorListNum = 0;

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors_detect_devices(const struct sensor_t* slist, int ssize,
                                struct sensor_t* clist, int csize)
{
        const char *dirname = "/dev/input";
        char devname[PATH_MAX];
        char *filename;
        int fd = -1;
        DIR *dir;
        struct dirent *de;
        int count = 0;
        int idx = 0;
        
        dir = opendir(dirname);
        if(dir == NULL)
            return 0;
        
        strcpy(devname, dirname);
        filename = devname + strlen(devname);
        *filename++ = '/';
        
        while((de = readdir(dir))) {
            if(de->d_name[0] == '.' &&
                    (de->d_name[1] == '\0' ||
                            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
                continue;
                
            strcpy(filename, de->d_name);
            
            fd = open(devname, O_RDONLY);
            if (fd>=0) {
                char name[80];
                if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                    name[0] = '\0';
                }
                for (idx = 0; idx < ssize; idx++) {
                    if (!strcmp(name, (char*)(slist[idx].reserved[0]))) {
                        memcpy(&clist[count], &slist[idx], sizeof(struct sensor_t));
                        count ++;
                        break;
                    }
                }                
            }
            close(fd);
            fd = -1;
            
            if (count >= csize)
                break;
        }
        closedir(dir);
        return count;
}

static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list) 
{
				module = module;
        *list = sSensorList;        
        return sSensorListNum;
}

static struct hw_module_methods_t sensors_module_methods = {
        open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        common: {
                tag: HARDWARE_MODULE_TAG,
                version_major: 1,
                version_minor: 0,
                id: SENSORS_HARDWARE_MODULE_ID,
                name: "Actions Sensors Module",
                author: "The Android Open Source Project",
                methods: &sensors_module_methods,
                dso:NULL,
                reserved: {0}
        },
        get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        accel   = 0,
        magnetic,
        orietation,
        light,
        proximity,
        gyroscope,
        temperature,
        numSensorDrivers,   // max sensor num
        numFds,     // max fd num
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    // For keeping track of usage (only count from system)
    bool mAccelActive;
    bool mMagnetActive;
    bool mOrientationActive;

    int real_activate(int handle, int enabled);

    int handleToDriver(int handle) const {
        switch (handle) {           
            case SENSORS_ACCELERATION_HANDLE:
                return accel;
                
    	    case SENSORS_MAGNETIC_FIELD_HANDLE:
                return magnetic;
                
    	    case SENSORS_ORIENTATION_HANDLE:
                return orietation;
                
    	    case SENSORS_LIGHT_HANDLE:
                return light;
                
            case SENSORS_PROXIMITY_HANDLE:
                return proximity;
                
            case SENSORS_GYROSCOPE_HANDLE:
                return gyroscope;
                
            case SENSORS_TEMPERATURE_HANDLE:
                return temperature;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    int index = 0;
    struct sensor_t* ss = NULL;
    
    // clear sensors
    for (index = 0; index < numSensorDrivers; index++) {
        mSensors[index] = NULL;
    }
    
    // clear mPollFds
    for (index = 0; index < numFds; index++) {
        memset(&(mPollFds[index]), 0, sizeof(pollfd));
        mPollFds[index].fd = -1;
    }
    
    // detect sensors
    if (sSensorListNum <= 0) {
        sSensorListNum = sensors_detect_devices(sSensorSupportList, 
                                    ARRAY_SIZE(sSensorSupportList),
                                    sSensorList, ARRAY_SIZE(sSensorList));
    }
    
    // create sensors
    for (index = 0; index < sSensorListNum; index++) {
         ss = &sSensorList[index];
         
        switch(ss->type) {
            case SENSOR_TYPE_ACCELEROMETER:
                if( mSensors[accel] == NULL) {
                    mSensors[accel] = new AccelerationSensor((char*)ss->reserved[0], 
                                                ss->resolution, ss->minDelay);
                    mPollFds[accel].fd = mSensors[accel]->getFd();
                    mPollFds[accel].events = POLLIN;
                    mPollFds[accel].revents = 0;
                }
                break;
                
            case SENSOR_TYPE_MAGNETIC_FIELD:
                if( mSensors[magnetic] == NULL) {
                    mSensors[magnetic] = new CompassSensor((char*)ss->reserved[0], 
                                                ss->resolution, ss->minDelay);
                    mPollFds[magnetic].fd = mSensors[magnetic]->getFd();
                    mPollFds[magnetic].events = POLLIN;
                    mPollFds[magnetic].revents = 0;
                }
                break;
                
            case SENSOR_TYPE_ORIENTATION:
                if( mSensors[orietation] == NULL) {
/*
                    mSensors[orientation] = new OrientationSensor();
                    mPollFds[orientation].fd = mSensors[orientation]->getFd();
                    mPollFds[orientation].events = POLLIN;
                    mPollFds[orientation].revents = 0;
*/
                }
                break;
                
            case SENSOR_TYPE_LIGHT:
                if( mSensors[light] == NULL) {
                    mSensors[light] = new LightSensor((char*)ss->reserved[0], 
                                                ss->resolution, ss->minDelay);
                    mPollFds[light].fd = mSensors[light]->getFd();
                    mPollFds[light].events = POLLIN;
                    mPollFds[light].revents = 0;
                }
                break;
                
            case SENSOR_TYPE_PROXIMITY:
                if( mSensors[proximity] == NULL) {
/*
                    mSensors[proximity] = new ProximitySensor();
                    mPollFds[proximity].fd = mSensors[proximity]->getFd();
                    mPollFds[proximity].events = POLLIN;
                    mPollFds[proximity].revents = 0;
*/
                }
                break;
                
            case SENSOR_TYPE_GYROSCOPE:
                if( mSensors[gyroscope] == NULL) {
                    mSensors[gyroscope] = new GyroSensor((char*)ss->reserved[0], 
                                                ss->resolution, ss->minDelay);
                    mPollFds[gyroscope].fd = mSensors[gyroscope]->getFd();
                    mPollFds[gyroscope].events = POLLIN;
                    mPollFds[gyroscope].revents = 0;
                }
                break;
                
            case SENSOR_TYPE_TEMPERATURE:
                if( mSensors[temperature] == NULL) {
                    mSensors[temperature] = new TemperatureSensor((char*)ss->reserved[0], 
                                                ss->resolution, ss->minDelay);
                    mPollFds[temperature].fd = mSensors[temperature]->getFd();
                    mPollFds[temperature].events = POLLIN;
                    mPollFds[temperature].revents = 0;
                }
                break;
        }
    }
    
    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

    mAccelActive = false;
    mMagnetActive = false;
    mOrientationActive = false;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        if (mSensors[i] != NULL) {
            delete mSensors[i];
        }
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int err;

    // Orientation requires accelerometer and magnetic sensor
    if (handle == ID_O) {
        mOrientationActive = enabled ? true : false;
        if (!mAccelActive) {
            err = real_activate(ID_A, enabled);
            if (err) return err;
        }
        if (!mMagnetActive) {
            err = real_activate(ID_M, enabled);
            if (err) return err;
        }
    }
    // Keep track of magnetic and accelerometer use from system
    else if (handle == ID_A) {
        mAccelActive = enabled ? true : false;
        // No need to enable or disable if orientation sensor is active as that will handle it
        if (mOrientationActive) return 0;
    }
    else if (handle == ID_M) {
        mMagnetActive = enabled ? true : false;
        // No need to enable or disable if orientation sensor is active as that will handle it
        if (mOrientationActive) return 0;
    }

    return real_activate(handle, enabled);
}

int sensors_poll_context_t::real_activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if (sensor == NULL) {
                continue;
            }
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            do {
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
            } while (n < 0 && errno == EINTR);
            if (n<0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                return -errno;
            }
            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));

                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
        int handle, int enabled) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
        int handle, int64_t ns) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
        int status = -EINVAL;
        sensors_poll_context_t *dev = new sensors_poll_context_t();

				id = id;
        memset(&dev->device, 0, sizeof(sensors_poll_device_t));

        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = 0;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = poll__close;
        dev->device.activate        = poll__activate;
        dev->device.setDelay        = poll__setDelay;
        dev->device.poll            = poll__poll;

        *device = &dev->device.common;
        status = 0;

        return status;
}

