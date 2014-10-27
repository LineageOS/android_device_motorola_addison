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

#ifndef ANDROID_LIGHTPROX_SENSOR_H
#define ANDROID_LIGHTPROX_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#include "nusensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class LightProxSensor : public SensorBase {
public:
            LightProxSensor();
    virtual ~LightProxSensor();

    enum {
        Light  = 0,
        Prox   = 1,
        numSensors
    };

    virtual int setEnable(int32_t handle, int enabled);
    virtual int getEnable(int32_t handle);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int readEvents(sensors_event_t* data, int count);
    void processEvent(int code, int value);
    virtual int flush(int32_t handle);

private:
    uint32_t mEnabled;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    sensors_event_t mFlushEvents[numSensors];
    uint64_t mDelays[numSensors];
    uint32_t mFlushEnabled;
};

#define BIT_L   (1 << Light)
#define BIT_P   (1 << Prox)
/*****************************************************************************/

#endif  // ANDROID_LIGHTPROX_SENSOR_H
