/*
 * Copyright (C) 2011 Motorola Mobility, Inc.
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

#ifndef ANDROID_ACCELERATION_SENSOR_H
#define ANDROID_ACCELERATION_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#include "nusensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class AccelerationSensor : public SensorBase {
    int mEnabled;
    int mState;
    int64_t mAccelDelay;
    int64_t mOrientDelay;
    int64_t mDelay;
    InputEventCircularReader mInputReader;
    enum {
        Accelerometer = 0,
        DisplayRotate,
        Orientation,   /* This needs to be at the end of the list */
        numSensors
    };
    sensors_event_t mPendingEvent[numSensors - 1];
    sensors_event_t mFlushEvents[numSensors - 1];
    uint32_t mPendingMask;
    uint32_t mFlushEnabled;
public:
            AccelerationSensor();
    virtual ~AccelerationSensor();

    virtual int readEvents(sensors_event_t* data, int count);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int getEnable(int32_t handle);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int64_t getDelay(int32_t handle);
    virtual int selectDelay(int32_t handle);
    virtual int flush(int32_t handle);

    int enableOrientation(int enabled);
    void processEvent(int code, int value);
    void Process_MeaningfulMovement(double acc_x, double acc_y, double acc_z);
    void client_mm(double nowStdDev);
};

#define BIT_A   (1 << Accelerometer)
#define BIT_DR  (1 << DisplayRotate)
#define BIT_O   (1 << Orientation)

/*****************************************************************************/

#endif  // ANDROID_ACCELERATION_SENSOR_H
