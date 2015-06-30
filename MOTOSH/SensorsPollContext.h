/*
 * Copyright (C) 2009-2015 Motorola Mobility
 *
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

#ifndef SENSORS_POLL_CONTEXT_H
#define SENSORS_POLL_CONTEXT_H

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <new>

#include <poll.h>
#include <pthread.h>

#include <linux/input.h>

#include <cutils/atomic.h>
#include <cutils/log.h>

#include <sys/select.h>

#include "Sensors.h"
#include "HubSensors.h"

/*****************************************************************************/

class SensorsPollContext {
public:
    /* sensors_poll_device_1_t must be the first entry here
     * because of the way Google designed the sensot HAL interface.
     * This class will be cast to a sensors_poll_device_1_t type
     * in the SensorManager code.
     */
    sensors_poll_device_1_t device; // must be first

    static SensorsPollContext* getInstance();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t ns, int64_t timeout);
    int flush(int handle);

private:
    SensorsPollContext();
    ~SensorsPollContext();
    enum {
        sensor_hub    = 0,
        numSensorDrivers,
    };

    static SensorsPollContext self;
    SensorBase* mSensors[numSensorDrivers];
    struct pollfd mPollFds[numSensorDrivers];
    int handleToDriver(int handle);
};

#endif /* SENSORS_POLL_CONTEXT_H */
