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
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#include <linux/input.h>

#include <cutils/log.h>

#include <sys/select.h>

#include "Sensors.h"
#include "HubSensors.h"
#include "UeventListener.h"

/*****************************************************************************/

class SensorsPollContext {
public:
    typedef std::lock_guard<std::recursive_mutex> AutoLock;

    sensors_poll_device_1_t device;

    static SensorsPollContext* getInstance();
    int activate(int handle, int enabled);
    int pollEvents(sensors_event_t* data, int count);
    int batch(int handle, int flags, int64_t ns, int64_t timeout);
    int flush(int handle);

    virtual void getSensorsList(std::vector<struct sensor_t> &list) {
        AutoLock _p(pollLock);
        releasePoll();
        AutoLock _d(driversLock);
        for (auto driver : drivers) {
            driver->getSensorsList(list);
        }
    }

    int getSensorCount() {
        std::vector<struct sensor_t> sensors;
        getSensorsList(sensors);
        return sensors.size();
    }

    /** How many of the sensors we manage are non-dynamic sensors. */
    int getStaticSensorCount() {
        return staticSensorCount;
    }

    /** @defgroup Synchronization Locks
     * @{
     *
     * There are 2 threads that need to be synchronized. 1. The thread that
     * calls pollEvents()/poll(). 2. The thread that calls activate(). The
     * activate() thread needs to modify the pollFds container and add/remove
     * file descriptors for sensors that have been enabled/disabled and the
     * pollEvents() thread is using that data for the poll() call.
     *
     * We need to ensure that the pollFds vector doesn't change while poll() is
     * blocked. We also need to make sure that once poll() releases it doesn't
     * reacquire the driversLock before the activate() thread has had a chance
     * to modify pollFds.
     *
     * This is a simplified diagram of the order of events:
     * @verbatim

     pollLock.lock()
     driversLock.lock()
     pollLock.unlock()
     poll()
                                pollLock.lock()
                                releasePoll()
                                driversLock.lock()
    poll() exit
    driversLock.unlock()
    pollLock.lock()             driversLock acquired
                                modify pollFds
                                driversLock.unlock()
                                pollLock.unlock()
    pollLock acquired
    driversLock.lock()
    pollLock.unlock()
    poll()

    @endverbatim */

    /** Used to lock the list of drivers and associated file descriptors (the
     * drivers, pollFds, and fd2driver containers). */
    std::recursive_mutex driversLock;

    /** Used to prevent the attempt by pollEvents() to lock driversLock and
     * call poll(). Typically used to update the pollFds vector. */
    std::recursive_mutex pollLock;
    /** @} */

    enum struct ReleaseReason : uint8_t {
        Undefined, Activate, SensorAdd, SensorRemove
    };

    /** Makes the poll() inside pollEvents() release so that the locks held by
     * that code are released. */
    void releasePoll(ReleaseReason reason = ReleaseReason::Undefined) {
        write(pipeFd[1], &reason, 1);
        fsync(pipeFd[1]);
    }

private:
    SensorsPollContext();
    virtual ~SensorsPollContext();
    static SensorsPollContext self;

    // Keeps track of how many non-dynamic sensors we're managing.
    int staticSensorCount;
    // Keeps track of how many non-dynamic sensor drivers we're managing.
    int staticDriverCount;

    // Pipe to communicate with the poll() thread.
    int pipeFd[2];

    UeventListener ueventListener;

    void addIioSensors();

    /** Updates all the containers that change when sensors are added/removed
     * or enabled/disabled. */
    void updateFdLists();

    std::vector<std::shared_ptr<SensorBase>> drivers;
    std::vector<struct pollfd> pollFds;

    // Used to map file descriptors to corresponding drivers
    std::map<int, std::shared_ptr<SensorBase>> fd2driver;

    // Map sensor handle/id to the corresponding driver
    std::shared_ptr<SensorBase> handleToDriver(int handle);

    // When a sensor is added or removed
    void onSensorAddRemove(char *d);
};

#endif /* SENSORS_POLL_CONTEXT_H */
