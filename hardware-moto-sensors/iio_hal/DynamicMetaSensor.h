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

#ifndef DYNAMIC_META_SENSOR
#define DYNAMIC_META_SENSOR

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
#include <atomic>

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

class IioHal;

/* This is a "meta" driver, for a "meta" dynamic sensor sensor (a sensor that
 * senses dynamic sensors).
 */
class DynamicMetaSensor : public SensorBase {
public:
    DynamicMetaSensor(int handle, IioHal &iioHal);

    virtual int readEvents(sensors_event_t* data, int count);

    virtual bool hasPendingEvents() const override {
        return hasEvents || !pendingAdditions.empty() || !pendingRemovals.empty() || flushResponsesDue > 0;
    }

    virtual int getFd() const override {
        return ueventListener.getFd();
    }

    virtual int setEnable(int32_t handle, int enabled) {
        if (handle == myHandle) {
            this->enabled = enabled;
        }
        return 0;
    }
    virtual int batch(int32_t handle, int32_t flags, int64_t ns, int64_t timeout) {
                       UNUSED(handle); UNUSED(flags); UNUSED(ns); UNUSED(timeout);
        return 0;
    }
    virtual int flush(int32_t handle) override;
    virtual bool hasSensor(int handle) {
        return handle == myHandle;
    }
    virtual void getSensorsList(std::vector<struct sensor_t> &list) {
        S_LOGD("handle=%d", myHandle);
        list.push_back({
            .name               = "Greybus IIO Dynamic MetaSensor",
            .vendor             = "Motorola",
            .version            = 1,
            .handle             = myHandle,
            .type               = SENSOR_TYPE_DYNAMIC_SENSOR_META,
            .stringType         = SENSOR_STRING_TYPE_DYNAMIC_SENSOR_META,
            .requiredPermission = "",
            .flags              = SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP,
            .reserved           = {0, 0},
        });
    }

    /* Check any pending devices for the correct selinux context label before
     * adding them to the pendingAdditions list. */
    int checkPermsAndAddToPending(sensors_event_t* data, int & count);

    int enabled;
private:
    const int myHandle;

    /** The next handle we can assign to a new dynamic sensor. Handles can't be
     * re-used (even across re-connection of a dynamic sensor). The multihal
     * depends on this.
     */
    int nextHandle;

    UeventListener ueventListener;
    bool hasEvents;
    IioHal &iioHal;

    /* After receiving 'add' uevents, add devices to this list to check if selinux
     * permissions are okay before accessing them through iio. */
    std::list< std::string > pendingChecks;

    /** The list of new sensors that has not yet been reported to the framework */
    std::list< std::shared_ptr<IioSensor> > pendingAdditions;

    /** We keep a reference to removed sensors so they don't get destroyed
     * until the framework has been notified that they're no longer available.
     * This is needed because otherwise the IioSensor destructor will free
     * dynamically allocated strings (ex: name, vendor) which the framework
     * might dereference until it is notified that the sensor no longer exists.
     * */
    std::list< std::shared_ptr<SensorBase> > pendingRemovals;

    /** The count of flush requests for which we owe a response. Using an
     * atomic here since this variable is being modified by multiple threads
     * (flush() thread increments it, poll() thread decrements it). */
    std::atomic<unsigned int> flushResponsesDue;

    /** Gets the current set of IIO sensors as found in sysfs. */
    std::vector< std::shared_ptr<IioSensor> > getIioSensorList();

    int reportPendingSensors(sensors_event_t* data, int & count);

    /// Respond to silly flush() requests.
    int reportPendingFlushes(sensors_event_t* data, int & count);

    /** Generates a RFC4122 Name-based UUID and adds it to the dynamic sensor
     * meta event. */
    void generateUuid(const struct sensor_t &s, dynamic_sensor_meta_event_t &ds);

    /** Converts a 16-byte UUID value to the canonical string form.
     *
     * @param uuid An array of 16 bytes containing the UUID in big endian byte order.
     * @return UUID in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx form
     * */
    std::string toUuidStr(uint8_t *uuid);
};

#endif // DYNAMIC_META_SENSOR
