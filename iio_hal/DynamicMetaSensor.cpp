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
#include <vector>
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
#include "IioHal.h"

using namespace std;
using namespace android;


DynamicMetaSensor::DynamicMetaSensor(int handle, IioHal &iioHal) : SensorBase("", "", ""),
    myHandle(handle), nextHandle(handle + 1),
    ueventListener(nullptr, nullptr), hasEvents(false),
    iioHal(iioHal), pendingAdditions(iioHal.updateSensorList(getIioSensorList()))
{
}

int DynamicMetaSensor::readEvents(sensors_event_t* data, int count) {
    //S_LOGD("+");
    int ret;
    if (count == 0) {
        hasEvents = true;
        return 0;
    }

    // Safe to destroy sensors previously reported as removed
    pendingRemovals.clear();

    ret = reportPendingSensors(data, count);
    count -= ret;
    if (!count) {
        if (!pendingAdditions.empty()) {
            hasEvents = true;
        }
        return ret;
    }

    shared_ptr<Uevent> e = ueventListener.readEvent();
    if (e) {
        if (e->eventType == Uevent::EventType::SensorRemove) {
            S_LOGD("Removing %s", e->deviceName.c_str());
            std::shared_ptr<SensorBase> s = iioHal.removeSensor(e->deviceName.c_str());
            if (s) {
                // Hang onto this reference until next time around.
                pendingRemovals.push_back(s);

                // We know s is of type IioSensor
                IioSensor *iio = static_cast<IioSensor *>(s.get());
                struct sensor_t &s = iio->getHalSensor();

                sensors_event_t &d = data[ret];
                dynamic_sensor_meta_event_t &ds = d.dynamic_sensor_meta;

                d.version   = sizeof(struct sensors_event_t);
                d.sensor    = myHandle;
                d.type      = SENSOR_TYPE_DYNAMIC_SENSOR_META;
                d.timestamp = android::elapsedRealtimeNano();

                ds.connected    = 0;
                ds.handle       = s.handle;
                ds.sensor       = nullptr;
                memset(&(ds.uuid), 0, sizeof(ds.uuid));

                ret++;
            }
        } else if (e->eventType == Uevent::EventType::SensorAdd) {
            S_LOGD("Adding %s", e->deviceName.c_str());

            /* Even though we were only notified of a single device, we will
             * try to get all the physically attached devices and add them to
             * the pendingAdditions list at once. Hopefully that will mean that
             * when subsequent UEvents are received for the 2nd device on the
             * same mod, that device will already be in the pendingAdditions list
             * and would be using the same iio_context pointer, thus saving
             * some memory.
             */

            pendingAdditions.splice(pendingAdditions.end(), iioHal.updateSensorList(getIioSensorList()));
            ret += reportPendingSensors(data, count);
        }
    }

    hasEvents = false;

    return ret;
}

int DynamicMetaSensor::reportPendingSensors(sensors_event_t* data, int count) {
    int ret = 0;

    while (!pendingAdditions.empty() && count > 0) {
        struct sensor_t &s = pendingAdditions.front()->getHalSensor();

        sensors_event_t &d = data[ret];
        dynamic_sensor_meta_event_t &ds = d.dynamic_sensor_meta;

        d.version   = sizeof(struct sensors_event_t);
        d.sensor    = myHandle;
        d.type      = SENSOR_TYPE_DYNAMIC_SENSOR_META;
        d.timestamp = android::elapsedRealtimeNano();

        ds.connected    = 1;
        ds.handle       = s.handle;
        ds.sensor       = &s;
        memset(&(ds.uuid), 0, sizeof(ds.uuid));

        pendingAdditions.pop_front();
        count--;
        ret++;
    }

    return ret;
}

vector< shared_ptr<IioSensor> > DynamicMetaSensor::getIioSensorList() {
    S_LOGD("+");

    vector< shared_ptr<IioSensor> > currentSensors;
    shared_ptr<struct iio_context> iio_ctx = IioSensor::createIioContext();

    if (! iio_ctx.get()) return currentSensors;
    //S_LOGD("0x%08x", iio_ctx.get());

    int devCount = iio_context_get_devices_count(iio_ctx.get());

    for (int i = 0; i < devCount; ++i) {
        const struct iio_device *d = iio_context_get_device(iio_ctx.get(), i);
        S_LOGD("Found IIO device %s %s", iio_device_get_name(d), iio_device_get_id(d));

        if (IioSensor::isUsable(d)) {
            currentSensors.push_back(make_shared<IioSensor>(iio_ctx, d, nextHandle++));

            shared_ptr<IioSensor> s = currentSensors.back();
            struct sensor_t &sh = s->getHalSensor();
            S_LOGD("Adding greybus IIO device: handle=%d fd=%d %s",
                    sh.handle, s->getFd(), sh.name);
        } else {
            S_LOGD("Skipping non-greybus device");
        }
    }

    S_LOGD("Found %zd IIO Greybus sensors.", currentSensors.size());
    return currentSensors;
}

