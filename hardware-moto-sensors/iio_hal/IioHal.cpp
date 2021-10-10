/*
 * Copyright (C) 2016 Motorola Mobility
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

#include <functional>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <map>
#include <type_traits>
#include <cstdint>
#include <cinttypes>

#include <mutex>

#include <cutils/atomic.h>
#include <android-base/macros.h>
#include <utils/Mutex.h>

#include <sys/select.h>
#include <sys/timerfd.h>

#include <hardware/sensors.h>
#include <utils/SystemClock.h>

#include "IioSensor.h"
#include "BaseHal.h"
#include "SensorBase.h"
#include "SensorsLog.h"
#include "UeventListener.h"
#include "IioHal.h"

using namespace std;
using namespace android;

IioHal::IioHal() : pipeFd{0, 0}, timerFd(-1) {
    //S_LOGD("this=%08" PRIxPTR, this);
    S_LOGD("+");

    if (pipe(pipeFd)) {
        S_LOGE("Unable to create pipe: %s", strerror(errno));
        pipeFd[0] = pipeFd[1] = 0;
    }

    timerFd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerFd == -1) {
        S_LOGE("Unable to create timer fd: %s", strerror(errno));
    }
}

void IioHal::init() {
    S_LOGD("+");
    std::shared_ptr<SensorBase> dynamic = std::make_shared<DynamicMetaSensor>(0, *this);

    // Using emplace() because getSensorsList() assumes the Meta Sensor is
    // the first driver in the list. At this point the Meta Sensor may have
    // already added some IIO sensor drivers to the list in its constructor.
    drivers.emplace(drivers.begin(), dynamic);

    // Setup an initial timer to do a first check of attached sensors after a
    // short delay.
    delay();

    updateFdLists();
    S_LOGD("-");
}

void IioHal::delay() {
    //S_LOGD("delay on Fd %d", timerFd);

    struct itimerspec delay = { .it_interval = {0,0},
                                .it_value = {0, 100000000 /* 100 ms */} };

    if (timerfd_settime(timerFd, 0 /* relative */, &delay, NULL) == -1)
        S_LOGE("error setting time on timer Fd: %d", errno);
}

int IioHal::activate(int handle, int enabled) {
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

shared_ptr<SensorBase> IioHal::removeSensor(const char *name) {
    if (!name) return nullptr;

    AutoLock _p(pollLock);
    releasePoll(ReleaseReason::SensorRemove);
    AutoLock _d(driversLock);

    auto it = find_if(begin(drivers), end(drivers),
            [name](const auto &d) {
                if (d->hasSensor(0)) // Ignore the non-IIO dynamic meta sensor
                    return false;
                // No dynamic_cast, but if we know for sure d is IioSensor we can do:
                IioSensor *iio = static_cast<IioSensor *>(d.get());
                return 0 == strcmp(iio->getIioId(), name);
            });

    if (it == drivers.end()) return nullptr;

    shared_ptr<SensorBase> s = *it;

    drivers.erase(it);

    bool wasEnabled = (s->getFd() >= 0);
    // updateFdLists() will only remove it if it's disabled (fd < 0)
    s->setEnable(-1, 0);
    if (wasEnabled) {
        updateFdLists();
    }

    return s;
}

int IioHal::poll(sensors_event_t* data, int count) {
    int pollRes, eventsRead = 0;
    //S_LOGD("+");

    if (!data || count < 1) {
        S_LOGE("poll failed. Event count %d.", count);
        return -EINVAL;
    }

    auto updateCounts = [&](int evtCount) {
        if (evtCount > 0) {
            count -= evtCount;
            data += evtCount;
            eventsRead += evtCount;
        }
        return evtCount;
    };

    // See if we have any pending events before blocking on poll()
    driversLock.lock();
    for (const auto& d : drivers) {
        if (d->hasPendingEvents()) {
            updateCounts(d->readEvents(data, count, -1));
        }
    }
    driversLock.unlock();

    if (eventsRead) return eventsRead;

    pollLock.lock();
    AutoLock _d(driversLock);
    pollLock.unlock();

    pollRes = TEMP_FAILURE_RETRY(::poll(&pollFds.front(), pollFds.size(), -1));

    if (pollRes >= 0) {
        // Success
        for (const auto& p : pollFds) {
            if (p.revents & POLLIN && p.fd >= 0) {
                if (p.fd == pipeFd[0]) { // Someone needs the driversLock
                    uint8_t reason = 255;
                    read(pipeFd[0], &reason, sizeof(reason));
                    //S_LOGD("Released poll because %d", reason);
                } else if (p.fd == timerFd) { // Delay timer expired
                    uint64_t expirations = 0;
                    ssize_t size;
                    size = read(timerFd, &expirations, sizeof(expirations));

                    //S_LOGD("timer read size %d with %ld expirations", size, expirations);
                    std::shared_ptr<DynamicMetaSensor> dyn =
                        static_pointer_cast<DynamicMetaSensor>(drivers[0]);
                    if (dyn && dyn->hasSensor(0) &&
                        size == sizeof(uint64_t) && expirations > 0)
                        updateCounts(dyn->checkPermsAndAddToPending(data, count));
                } else {
                    int res = updateCounts(fd2driver[p.fd]->readEvents(data, count, p.fd));
                    // Need to relay any errors upward.
                    if (res < 0) {
                        S_LOGE("reading events failed fd=%d nb=%d", p.fd, res);
                        return 0;
                    }
                }
            }
        }
    } else {
        S_LOGE("poll() failed with %d (%s)", errno, strerror(errno));
    }

    return eventsRead;
}

/** Since there are different strategies for acquiring the driversLock, caller
 * must acquire the lock before calling this function. */
void IioHal::updateFdLists() {
    fd2driver.clear();
    pollFds.clear();

    if (pipeFd[0] > 0) {
        pollFds.push_back({.fd = pipeFd[0], .events = POLLIN, .revents = 0});
    }

    if (timerFd > 0)
        pollFds.push_back({.fd = timerFd, .events = POLLIN, .revents = 0});

    for (const auto& driver : drivers) {
        if (!driver) {
            S_LOGE("Null driver");
            continue;
        }
        int fd = driver->getFd();
        //S_LOGD("Adding fd=%d for driver=0x%08x @ %d", fd, driver.get(), pollFds.size());
        if (fd >= 0) {
            fd2driver[fd] = driver;
            pollFds.push_back({
                    .fd = fd,
                    .events = POLLIN,
                    .revents = 0
            });
        }
    }

    for (const auto& driver : drivers) {
        if (isIioSensor(driver)) {
            IioSensor *sensor = static_cast<IioSensor *>(driver.get());
            int fd = sensor->getEventFd();
            if (fd >= 0 && sensor->getFd() >= 0) {
                S_LOGD("Add eventFd %d", fd);
                fd2driver[fd] = driver;
                pollFds.push_back({
                        .fd = fd,
                        .events = POLLIN,
                        .revents = 0
                });
            }
        }
    }
}

list< shared_ptr<IioSensor> > IioHal::updateSensorList(const IioHal::IioSensorCont & currSensors) {
    list< shared_ptr<IioSensor> > newSensors;
    S_LOGD("+");

    for (const auto& sensor : currSensors) {
        const char *id = sensor->getIioId();
        auto it = find_if(begin(drivers), end(drivers),
                [id](const auto &d){ return sameSensor(d, id); });
        if (it == drivers.end()) {
            // This is a new sensor.
            S_LOGD("%s is a new sensor.", id);
            drivers.push_back(sensor);
            newSensors.push_back(sensor);
        } else {
            S_LOGD("%s already present.", id);
        }
    }

    return newSensors;
}

bool IioHal::sameSensor(const shared_ptr<SensorBase> & s, const char *iioId) {
    if (s->hasSensor(0)) // Ignore the non-IIO dynamic meta sensor
        return false;
    if (!iioId)
        return false;
    // No dynamic_cast, but if we know for sure d is IioSensor we can do:
    IioSensor *iio = static_cast<IioSensor *>(s.get());
    return 0 == strcmp(iio->getIioId(), iioId);
}

