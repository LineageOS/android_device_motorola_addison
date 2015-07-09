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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>

#include <cutils/atomic.h>
#include <cutils/log.h>

#include <linux/input.h>

#include <sys/select.h>

#include <hardware/sensors.h>

#include "SensorsPollContext.h"

/*****************************************************************************/

SensorsPollContext SensorsPollContext::self;

SensorsPollContext::SensorsPollContext()
{
    mSensors[sensor_hub] = HubSensors::getInstance();
    mPollFds[sensor_hub].fd = mSensors[sensor_hub]->getFd();
    mPollFds[sensor_hub].events = POLLIN;
    mPollFds[sensor_hub].revents = 0;
}

SensorsPollContext::~SensorsPollContext()
{
    for (int i=0 ; i<numSensorDrivers ; i++) {
        mSensors[i] = NULL;
    }
}

SensorsPollContext *SensorsPollContext::getInstance()
{
    return &self;
}

int SensorsPollContext::handleToDriver(int handle)
{
    if (mSensors[sensor_hub]->hasSensor(handle))
        return sensor_hub;

    return -EINVAL;
}

int SensorsPollContext::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setEnable(handle, enabled);
}

int SensorsPollContext::setDelay(int handle, int64_t ns) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int SensorsPollContext::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int ret;
    int err;

    while (true) {
        ret = poll(mPollFds, numSensorDrivers, nbEvents ? 0 : -1);
        err = errno;
        // Success
        if (ret >= 0)
            break;
        ALOGE("poll() failed (%s)", strerror(err));
        // EINTR is OK
        if (err == EINTR)
            continue;
        else
            return -err;
    }

    if(mPollFds[sensor_hub].revents & POLLIN) {
        SensorBase* const sensor(mSensors[sensor_hub]);
        int nb = sensor->readEvents(data, count);
        // Need to relay any errors upward.
        if (nb < 0)
            return nb;
        count -= nb;
        nbEvents += nb;
        data += nb;
        mPollFds[sensor_hub].revents = 0;
    }

    return nbEvents;
}

int SensorsPollContext::batch(int handle, int flags, int64_t ns, int64_t timeout)
{
    (void)flags;
    (void)timeout;
    return setDelay(handle, ns);
}

int SensorsPollContext::flush(int handle)
{
    int drv = handleToDriver(handle);
    return mSensors[drv]->flush(handle);
}
