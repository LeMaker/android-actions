/*
* Copyright (C) 2012 Invensense, Inc.
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

#undef  NDEBUG
#define NDEBUG 0

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <pthread.h>

#include <cutils/log.h>
#include <utils/KeyedVector.h>
#include <linux/input.h>

#include "invensense.h"

#include "MPLSensorSysApi.h"

/*******************************************************************************/
/* Gyro Driver Specific SYSFS Attribute                                        */
/*******************************************************************************/
#define GYRO_SENSOR_SYSFS_PATH  "/sys/class/input/input0/"
#define GYRO_SENSOR_SELFTEST "device/inv_gyro/self_test"

MplSys_Interface* getSysInterfaceObject()
{
    MPLSensorSysApi* s = static_cast<MPLSensorSysApi*>(MPLSensor::gMPLSensor);
    return static_cast<MplSys_Interface*>(s);
}

MPLSensorSysApi::MPLSensorSysApi() : MPLSensor()
{

}

MPLSensorSysApi::~MPLSensorSysApi()
{

}

MPLSensorSysApi::MplSys_Interface::~MplSys_Interface()
{

}

/* Should be getting from hardware self-test */
int MPLSensorSysApi::getBiases(float *b)
{
    FUNC_LOG;
    int rv = INV_SUCCESS;
    long val[3];
    long temp;
    LOGV("get biases\n");
    pthread_mutex_lock(&mMplMutex);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::setBiases(float *b)
{
    FUNC_LOG;
    int rv = INV_SUCCESS;
    pthread_mutex_lock(&mMplMutex);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::setBiasUpdateFunc(long f)
{
    FUNC_LOG;
    LOGW("SysApi :: setBiasUpdateFunc is OBSOLETE and ineffective");
    return 0;
}

int MPLSensorSysApi::setSensors(long s)
{
    FUNC_LOG;
    int rv = INV_SUCCESS;

    pthread_mutex_lock(&mMplMutex);
    mMasterSensorMask = s;
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::getSensors(long* s)
{
    FUNC_LOG;
    int rv = INV_SUCCESS;
    pthread_mutex_lock(&mMplMutex);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::resetCal()
{
    FUNC_LOG;
    int rv = INV_SUCCESS;
    LOGI("SysApi :: resetCal is OBSOLETE");
    return rv;
}

int MPLSensorSysApi::selfTest()
{
    FUNC_LOG;
    int rv = INV_SUCCESS;
    char buf[50];

    LOGV_IF(EXTRA_VERBOSE, "gyro set delay path: %s", GYRO_SENSOR_SELF_TEST);

    int fd = open(mpu.self_test, O_RDONLY);
    if( fd < 0 ) {
        LOGE("Error opening gyro self-test");
        return INV_ERROR;
    }
    pthread_mutex_lock(&mMplMutex);
    char x[15], y[15], z[15];
    char result[2];
    do {
        memset(buf, 0, sizeof(buf));
        int count = read_attribute_sensor(fd, buf, sizeof(buf));
        if( count < 1 ) {
            LOGE("Error reading gyro self-test"); 
			pthread_mutex_unlock(&mMplMutex);
            return INV_ERROR;
        }
        sscanf(buf, "%[^','],%[^','],%[^','],%[^',']", x, y, z, result);
        LOGI("Bias: X:Y:Z (%ld, %ld, %ld)", atol(x), atol(y), atol(z));
        if ( atoi(result) ) {
            LOGE("self test passed");
        }
        else {
            LOGE("error self-test failed");
            break;
        }
    } while (0);
    pthread_mutex_unlock(&mMplMutex);
    return rv;
}

int MPLSensorSysApi::setLocalMagField(float x, float y, float z)
{
    FUNC_LOG;
    int rv = INV_SUCCESS;
    LOGI("SysApi :: setLocalMagField is OBSOLETE");
    return rv;
}

