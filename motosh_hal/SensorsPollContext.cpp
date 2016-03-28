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
#include <assert.h>
#include <signal.h>

#include <cutils/atomic.h>

#include <linux/input.h>

#include <sys/select.h>
#include <sys/types.h>

#include <hardware/sensors.h>

#include "SensorsPollContext.h"
#include "SensorsLog.h"

#if defined(_ENABLE_REARPROX)
#include "SensorBase.h"
#include "RearProxSensor.h"
#endif
#include "IioSensor.h"

using namespace std;

/*****************************************************************************/

SensorsPollContext SensorsPollContext::self;

SensorsPollContext::SensorsPollContext() :
    pipeFd{0, 0},
    ueventListener( [this](char*d){this->onSensorAddRemove(d);},
                    [this](char*d){this->onSensorAddRemove(d);})
{
    S_LOGD("+ pid=%d tid=%d", getpid(), gettid());

    if (pipe(pipeFd)) {
        S_LOGE("Unable to create pipe: %s", strerror(errno));
        pipeFd[0] = pipeFd[1] = 0;
    }

    AutoLock _l(driversLock);

    drivers.push_back(make_shared<HubSensors>());

#ifdef _ENABLE_REARPROX
    drivers.push_back(make_shared<RearProxSensor>());
#endif

    // All the static sensors have been added above.
    staticSensorCount = getSensorCount();
    staticDriverCount = drivers.size();
    S_LOGD("Static drivers %d", drivers.size());
    // Now we can add dynamic sensors

    addIioSensors();
    updateFdLists();
}

/** Since there are different strategies for acquiring the driversLock, caller
 * must acquire the lock before calling this function. */
void SensorsPollContext::updateFdLists() {
    fd2driver.clear();
    pollFds.clear();

    if (pipeFd[0] > 0) {
        pollFds.push_back({.fd = pipeFd[0], .events = POLLIN, .revents = 0});
    }

    if (ueventListener.getFd() > 0) {
        pollFds.push_back({
                .fd = ueventListener.getFd(),
                .events = POLLIN,
                .revents = 0});
    } else {
        S_LOGD("Skipping ueventListener");
    }

    for (auto driver : drivers) {
        if (!driver) {
            S_LOGE("Null driver");
            continue;
        }
        int fd = driver->getFd();
        //S_LOGD("Adding fd=%d for driver=0x%08x @ %d", fd, driver.get(), pollFds.size());
        if (fd > 0) {
            fd2driver[fd] = driver;
            pollFds.push_back({
                    .fd = fd,
                    .events = POLLIN,
                    .revents = 0
            });
        }
    }
}


SensorsPollContext::~SensorsPollContext()
{
    pollFds.clear();
    fd2driver.clear();
    drivers.clear();
}

SensorsPollContext *SensorsPollContext::getInstance()
{
    return &self;
}

void SensorsPollContext::onSensorAddRemove(char *device) {
    S_LOGD("Adding/Removing %s", device);

    AutoLock _p(pollLock);
    releasePoll(ReleaseReason::SensorAdd);
    AutoLock _d(driversLock);

    S_LOGD("statics: %d/%d", staticDriverCount, drivers.size());
    if (end(drivers) == (begin(drivers) + staticDriverCount)) {
        S_LOGD("No dynamic sensors");
    } else {
        S_LOGD("Got dynamic sensors");
    }

    drivers.erase(begin(drivers) + staticDriverCount, end(drivers));

    S_LOGD("Erased dynamic sensors: %d", drivers.size());
    addIioSensors();
    updateFdLists();
}

void SensorsPollContext::addIioSensors() {
    S_LOGD("+");
    shared_ptr<struct iio_context> iio_ctx = IioSensor::createIioContext();

    if (iio_ctx) {
        IioSensor::updateSensorList(iio_ctx);

        for (auto sensor : IioSensor::getSensors()) {
            S_LOGD("Adding driver 0x%08x", sensor.get());
            drivers.push_back(sensor);
        }
    }
}

shared_ptr<SensorBase> SensorsPollContext::handleToDriver(int handle)
{
    for (auto d : drivers) {
        if (d->hasSensor(handle)) return d;
    }

    S_LOGE("No driver for handle %d", handle);
    return nullptr;
}

int SensorsPollContext::activate(int handle, int enabled) {
    int ret = -EBADFD;

    S_LOGD("handle=%d enabled=%d pid=%d tid=%d", handle, enabled, getpid(), gettid());

    shared_ptr<SensorBase> s = handleToDriver(handle);
    if (s == nullptr) {
        S_LOGE("No driver found for handle %d (en=%d)", handle, enabled);
        return -EINVAL;
    }

    // Try to stop the poll() so we can modify underlying structures
    AutoLock _p(pollLock);
    releasePoll(ReleaseReason::Activate);
    AutoLock _d(driversLock);

    ret = s->setEnable(handle, enabled);
    updateFdLists();

    return ret;
}

int SensorsPollContext::pollEvents(sensors_event_t* data, int count)
{
    int nbEvents = 0;
    int ret;
    int err;

    //S_LOGD("count=%d pid=%d tid=%d", count, getpid(), gettid());

    auto eventReader = [&](shared_ptr<SensorBase> d) {
        int nb = d->readEvents(data, count);
        if (nb > 0) {
            count -= nb;
            nbEvents += nb;
            data += nb;
        }
        return nb;
    };

    // See if we have any pending events before blocking on poll()
    driversLock.lock();
    for (auto d : drivers) {
        if (d->hasPendingEvents()) eventReader(d);
    }
    driversLock.unlock();

    if (nbEvents) return nbEvents;

    pollLock.lock();
    AutoLock _d(driversLock);
    pollLock.unlock();

    ret = poll(&pollFds.front(), pollFds.size(), nbEvents ? 0 : -1);
    err = errno;

    // Success
    if (ret >= 0) {
        for (auto p : pollFds) {
            if (p.revents & POLLIN) {
                if (p.fd == pipeFd[0]) { // Someone needs the driversLock
                    uint8_t reason = 255;
                    read(pipeFd[0], &reason, sizeof(reason));
                    //S_LOGD("Released poll because %d", reason);
                } else if (ueventListener.getFd() > 0 && p.fd == ueventListener.getFd()) {
                    //S_LOGD("Got udev uevent");
                    ueventListener.readEvents();
                } else {
                    int nb = eventReader(fd2driver[p.fd]);
                    // Need to relay any errors upward.
                    if (nb < 0) {
                        S_LOGE("fd=%d nb=%d", p.fd, nb);
                        return nb;
                    }
                }
            }
        }
    } else {
        S_LOGE("poll() failed with %d (%s)", err, strerror(err));
        nbEvents = (err == EINTR ? 0 : -err); // EINTR is OK
    }

    return nbEvents;
}

int SensorsPollContext::batch(int handle, int flags, int64_t ns, int64_t timeout)
{
    shared_ptr<SensorBase> s = handleToDriver(handle);
    if (s == nullptr) return -EINVAL;
    return s->batch(handle, flags, ns, timeout);
}

int SensorsPollContext::flush(int handle)
{

    shared_ptr<SensorBase> s = handleToDriver(handle);
#ifdef _ENABLE_REARPROX
    // to use sensorhub driver to handle flush for rearprox,
    // this is a workaround for rearprox and flush should be
    // implemented by rearprox driver ideally
    if (handle == SENSORS_HANDLE_BASE + ID_RP) {
        s = handleToDriver(SENSORS_HANDLE_BASE + ID_A);
    }
#endif

    if (s == nullptr) return -EINVAL;
    return s->flush(handle);
}
