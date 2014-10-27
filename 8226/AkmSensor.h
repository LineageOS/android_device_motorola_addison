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

#ifndef ANDROID_AKM_SENSOR_H
#define ANDROID_AKM_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#include "nusensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class AkmSensor : public SensorBase {
public:
    AkmSensor();
    virtual ~AkmSensor();

    enum {
        Accelerometer = 0,
        MagneticField,
        Orientation,
        numSensors
    };

    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int64_t getDelay(int32_t handle);
    virtual int getEnable(int32_t handle);
    int setAccel(sensors_event_t* data);
    virtual int flush(int32_t handle);

private:
    int mEnabled[numSensors];
    int64_t mDelay[numSensors];
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    sensors_event_t mFlushEvents[numSensors];
    char input_sysfs_path[PATH_MAX];
    int input_sysfs_path_len;
    uint32_t mFlushEnabled;

    int handle2id(int32_t handle);
    void processEvent(int code, int value);
};

#define BIT_A   (1 << Accelerometer)
#define BIT_M   (1 << MagneticField)
#define BIT_O   (1 << Orientation)
/*****************************************************************************/

#endif  // ANDROID_AKM_SENSOR_H
