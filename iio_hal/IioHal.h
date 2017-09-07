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

#ifndef IIO_HAL
#define IIO_HAL

#include <assert.h>
#include <functional>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <list>
#include <map>
#include <type_traits>
#include <cstdint>
#include <cinttypes>

#include <mutex>

#include <cutils/atomic.h>
#include <android-base/macros.h>
#include <utils/Mutex.h>

#include <sys/select.h>

#include <hardware/sensors.h>
#include <utils/SystemClock.h>

#include "IioSensor.h"
#include "BaseHal.h"
#include "SensorBase.h"
#include "SensorsLog.h"
#include "UeventListener.h"
#include "DynamicMetaSensor.h"

#include <assert.h>

/**
 * This is a HAL dedicated to handling dynamic sensors exposed through GreyBus
 * and IIO. It reports a single static sensor (the DynamicMetaSensor class)
 * through which dynamic sensor additions/removals are reported to the framework.
 */
class IioHal : public BaseHal {
public:
    /// IIO sensor container
    typedef std::vector< std::shared_ptr<IioSensor> > IioSensorCont;

    IioHal();
    virtual void init() override;

    virtual ~IioHal() = default;

    virtual int poll(sensors_event_t* data, int count) override;
    virtual int activate(int handle, int enabled) override;

    /** IIO dynamic sensor addition/removal is reported from the poll() call.
     * There is only 1 non-dynamic sensor reported by this HAL: the Dynamic
     * Meta Sensor. */
    virtual int getSensorsList(struct sensor_t const** list) {
        //S_LOGD("+ drivers=%d", drivers.size());
        static std::vector<struct sensor_t> sensorsList;

        sensorsList.clear();
        drivers[0]->getSensorsList(sensorsList);
        *list = &sensorsList[0];
        // The Dynamic Meta Sensor has a handle of 0.
        assert(sensorsList[0].handle == 0);
        //S_LOGD("handle=%d name=%s", sensorsList[0].handle, sensorsList[0].name);
        return sensorsList.size();
    }

    /** Removes the sensor that matches the given name from all internal lists
     * and returns it to the caller to dispose of. This function is called when
     * the kernel notifies us that an IIO sensor was removed.
     *
     * @param name The name of the sensor to remove. Ex: "iio:device4"
     * @returns A sensor object matching the given name, or null if no such
     * sensor exists. */
    std::shared_ptr<SensorBase> removeSensor(const char *name);

    /**
     * Goes through the list of the given sensors, and adds any new ones to the
     * internal list. Since this function is typically called with a list of
     * all currently available IIO sensors (both old sensors, and newly added
     * ones), the function returns a list of the ones that were actually added.
     *
     * @param currSensors This is typically the list of all currently available
     * sensors. Note: it is essential that this argument be a const-ref or else
     * calling this function with a temporary (returned by another function)
     * would result in a dangling reference and fail.
     *
     * @return A list of sensors that were added from the provided list.
     */
    std::list< std::shared_ptr<IioSensor> > updateSensorList(const IioSensorCont & currSensors);

    enum struct ReleaseReason : uint8_t {
        Undefined, Activate, SensorAdd, SensorRemove, FlushResponseDue
    };

    /** Makes the ::poll() inside IioHal::poll() release so that the locks held
     * by that function are released. */
    void releasePoll(ReleaseReason reason = ReleaseReason::Undefined) {
        write(pipeFd[1], &reason, 1);
        fsync(pipeFd[1]);
    }

    // start a timer using the timerFd
    void delay(void);

private:
    DISALLOW_COPY_AND_ASSIGN(IioHal);

    typedef std::lock_guard<std::recursive_mutex> AutoLock;

    /**
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

    /** Used to prevent the attempt by IioHal::poll() to lock driversLock and
     * call poll(). Typically used to update the pollFds vector. */
    std::recursive_mutex pollLock;
    /** @} */

    // Pipe to communicate with the poll() thread.
    int pipeFd[2];

    // Timer FD used to delay after dynamic sensor addition to allow selinux
    // labels to switch before sensor is accessed.
    int timerFd;

    // File descriptors we're listening on
    std::vector<struct pollfd> pollFds;

    // Used to map file descriptors to corresponding drivers
    std::map<int, std::shared_ptr<SensorBase>> fd2driver;

    /** Updates all the containers that change when sensors are added/removed
     * or enabled/disabled. */
    void updateFdLists();

    // This is a brittle hack. Need some kind of pseudo RTTI.
    bool isIioSensor(std::shared_ptr<SensorBase> sensor) {
        // The DynamicMetaSensor has handle 0. All other sensors should be
        // IioSensor types and have non-0 handles.
        return !sensor->hasSensor(0);
    }

    /**
     * Checks to see if a sensor has a given IIO ID. Since we don't have RTTI,
     * this is a bit of a hack.
     *
     * @param s A sensor object to test. Since we don't have RTTI we assume the
     * caller only calls this for IioSensor types (or sensors that return false
     * for hasSensor(0)).
     * @param iioId A sensor IIO ID to check for. Ex: "iio:device3"
     * @return true if s is an IioSensor and has the given ID.
     */
    static bool sameSensor(const std::shared_ptr<SensorBase> & s, const char *iioId);
};

#endif // IIO_HAL
