/*
 * * Copyright (C) 2013 Motorola, Inc.
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

#include "nusensors.h"
#include "AkmSensor.h"
#include "lis3dh_hal.h"
#include "ct406_hal.h"

#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
#include "l3g4200d_hal.h"
#endif
/*****************************************************************************/

bool is_device_moving = true;
bool is_gyro_calibrated = true;

struct sensors_poll_context_t {
    sensors_poll_device_1_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int setDelay_sub(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t ns, int64_t timeout);
    int flush(int handle);

private:
    enum {
        acceleration    = 0,
        lightprox       = 1,
        akm             = 2,
#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
        gyro            = 3,
#endif
        numSensorDrivers,
        numFds,
    };

    static const size_t wake = numFds - 1;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
            case ID_DR:
                return acceleration;
            case ID_M:
            case ID_O:
                return akm;
            case ID_P:
            case ID_L:
            case ID_IP:
                return lightprox;
#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
            case ID_G:
                return gyro;
#endif
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    mSensors[acceleration] = new AccelerationSensor();
    mPollFds[acceleration].fd = mSensors[acceleration]->getFd();
    mPollFds[acceleration].events = POLLIN;
    mPollFds[acceleration].revents = 0;

    mSensors[lightprox] = new LightProxSensor();
    mPollFds[lightprox].fd = mSensors[lightprox]->getFd();
    mPollFds[lightprox].events = POLLIN;
    mPollFds[lightprox].revents = 0;

    mSensors[akm] = new AkmSensor();
    mPollFds[akm].fd = mSensors[akm]->getFd();
    mPollFds[akm].events = POLLIN;
    mPollFds[akm].revents = 0;

#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
    mSensors[gyro] = new GyroSensor();
    mPollFds[gyro].fd = mSensors[gyro]->getFd();
    mPollFds[gyro].events = POLLIN;
    mPollFds[gyro].revents = 0;
#endif

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
    int index = handleToDriver(handle);
    int err;

    if (index < 0) return index;

    err =  mSensors[index]->setEnable(handle, enabled);
    if ((!err) && (handle == ID_O))
        err =  mSensors[handleToDriver(ID_A)]->setEnable(handle, enabled);

    if (enabled && !err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }

    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {
    int err;
    int drv = handleToDriver(handle);

    if (handle == ID_O) {
        setDelay_sub(ID_O, ns);
        err = mSensors[handleToDriver(ID_A)]->setDelay(handle, ns);
    } else if (handle == ID_M)
        err = setDelay_sub(handle, ns);
    else
        err = mSensors[drv]->setDelay(handle, ns);

    ALOGE_IF(0 != err, "setDelay(%d) failed (%s)", handle, strerror(-err));
    return err;
}

int sensors_poll_context_t::setDelay_sub(int handle, int64_t ns) {
    int drv = handleToDriver(handle);
    int en = mSensors[drv]->getEnable(handle);
    int64_t cur = mSensors[drv]->getDelay(handle);
    int err = 0;

    if (en <= 1) {
            /* no dependencies */
            if (cur != ns) {
                    err = mSensors[drv]->setDelay(handle, ns);
            }
    } else {
            /* has dependencies, choose shorter interval */
            if (cur > ns) {
                    err = mSensors[drv]->setDelay(handle, ns);
            }
    }
    return err;
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int n = 0;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                if ((0 != nb) && (data[nb-1].type == SENSOR_TYPE_ACCELEROMETER)) {
                   ((AkmSensor*)(mSensors[akm]))->setAccel(&data[nb-1]);
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
            n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
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

int sensors_poll_context_t::batch(int handle, int flags, int64_t ns, int64_t timeout)
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
        dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_0;
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
