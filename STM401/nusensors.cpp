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

#include "nusensors.h"
#include "sensorhub_hal.h"

/*****************************************************************************/

struct sensors_poll_context_t {
    sensors_poll_device_1_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t ns, int64_t timeout);
    int flush(int handle);

private:
    enum {
        accelgyromag    = 0,
        numSensorDrivers,
    };

    SensorBase* mSensors[numSensorDrivers];
    struct pollfd mPollFds[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
            case ID_G:
            case ID_PR:
            case ID_M:
            case ID_O:
            case ID_T:
            case ID_L:
#ifdef _ENABLE_LA
            case ID_LA:
#endif
#ifdef _ENABLE_GR
            case ID_GR:
#endif
            case ID_DR:
            case ID_P:
            case ID_FU:
            case ID_FD:
            case ID_S:
            case ID_CA:
            case ID_IR_GESTURE:
            case ID_IR_RAW:
            case ID_IR_OBJECT:
            case ID_SIM:
#ifdef _ENABLE_PEDO
            case ID_STEP_DETECTOR:
            case ID_STEP_COUNTER:
#endif
            case ID_UNCALIB_GYRO:
            case ID_UNCALIB_MAG:
#ifdef _ENABLE_CHOPCHOP
            case ID_CHOPCHOP_GESTURE:
#endif
#ifdef _ENABLE_LIFT
            case ID_LIFT_GESTURE:
#endif
            case ID_QUAT_6AXIS:
            case ID_QUAT_9AXIS:
                return accelgyromag;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[accelgyromag] = new HubSensor();
    mPollFds[accelgyromag].fd = mSensors[accelgyromag]->getFd();
    mPollFds[accelgyromag].events = POLLIN;
    mPollFds[accelgyromag].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->enable(handle, enabled);
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
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

    if(mPollFds[accelgyromag].revents & POLLIN) {
        SensorBase* const sensor(mSensors[accelgyromag]);
        int nb = sensor->readEvents(data, count);
        // Need to relay any errors upward.
        if (nb < 0)
            return nb;
        count -= nb;
        nbEvents += nb;
        data += nb;
        mPollFds[accelgyromag].revents = 0;
    }

    return nbEvents;
}

int sensors_poll_context_t::batch(int handle, int /*flags*/, int64_t ns, int64_t /*timeout*/)
{
    return setDelay(handle, ns);
}

int sensors_poll_context_t::flush(int handle)
{
    int drv = handleToDriver(handle);
    return mSensors[drv]->flush(handle);
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

/*!
 * \brief Implement Android HAL poll()
 *
 * From [source.android.com](https://source.android.com/devices/sensors/hal-interface.html)
 *
 * Returns an array of sensor data by filling the data argument. This function
 * must block until events are available. It will return the number of events
 * read on success, or a negative error number in case of an error.
 *
 * The number of events returned in data must be less or equal to the count
 * argument. This function shall never return 0 (no event).
 *
 * \param[in]  dev   the device to poll
 * \param[out] data  the returned data items
 * \param[in]  count the maximum number of returned data items
 *
 * \returns negative on failure, the number of returned data items on success,
 *          and never 0.
 */
static int poll__poll(struct sensors_poll_device_t *dev,
        sensors_event_t* data, int count) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    int ret = 0;
    do {
        ret = ctx->pollEvents(data, count);
    } while( ret == 0 );
    return ret;
}

static int poll__batch(sensors_poll_device_1_t *dev,
        int handle, int flags, int64_t ns, int64_t timeout) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->batch(handle, flags, ns, timeout);
}

static int poll__flush(sensors_poll_device_1_t *dev,
        int handle) {
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->flush(handle);
}

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device)
{
    int status = -EINVAL;

    sensors_poll_context_t *dev = new(std::nothrow) sensors_poll_context_t();

    if (dev) {
        memset(&dev->device, 0, sizeof(sensors_poll_device_1_t));

        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_3;
        dev->device.common.module   = const_cast<hw_module_t*>(module);
        dev->device.common.close    = poll__close;
        dev->device.activate        = poll__activate;
        dev->device.setDelay        = poll__setDelay;
        dev->device.poll            = poll__poll;
        dev->device.batch           = poll__batch;
        dev->device.flush           = poll__flush;

        *device = &dev->device.common;
        status = 0;
    } else {
        ALOGE("out of memory: new failed for sensors_poll_context_t");
    }

    return status;
}
