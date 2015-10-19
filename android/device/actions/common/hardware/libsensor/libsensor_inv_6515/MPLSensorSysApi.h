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

#ifndef INV_HAL_SYS_API_H
#define INV_HAL_SYS_API_H

#include "sensors.h"
#include "MPLSensor.h"
#include <gui/MplInterfaces.h>

class MPLSensorSysApi : public MPLSensor, public MplSys_Interface {

public:
    MPLSensorSysApi();
    virtual ~MPLSensorSysApi();

    virtual int getBiases(float *b);
    virtual int setBiases(float *b);
    virtual int setBiasUpdateFunc(long f);
    virtual int setSensors(long s);
    virtual int getSensors(long* s);
    virtual int resetCal();
    virtual int selfTest();
    virtual int setLocalMagField(float x, float y, float z);

private:
    int write_attribute_sensor(int fd, unsigned char* buf, size_t count);

};

extern "C" {
MplSys_Interface* getSysInterfaceObject();
}

#endif

