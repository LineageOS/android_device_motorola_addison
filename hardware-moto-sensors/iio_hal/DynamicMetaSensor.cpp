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
#include <unistd.h>

#include <mutex>

#include <cutils/atomic.h>
#include <android-base/macros.h>
#include <android-base/stringprintf.h>
#include <utils/Mutex.h>

#include <sys/select.h>

#include <hardware/sensors.h>
#include <utils/SystemClock.h>
#include <openssl/sha.h>
#include <selinux/android.h>

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
    iioHal(iioHal),
    flushResponsesDue(0)
{
}

int DynamicMetaSensor::flush(int32_t handle) {
                              UNUSED(handle);
    flushResponsesDue++;
    iioHal.releasePoll(IioHal::ReleaseReason::FlushResponseDue);
    // FYI, it's possible for us to report flush-complete here (from the poll()
    // thread), before this function returns.
    return 0;
}

int DynamicMetaSensor::readEvents(sensors_event_t* data, int count) {
    //S_LOGD("+");
    int ret = 0;
    if (count == 0) {
        hasEvents = true;
        return 0;
    }

    // Safe to destroy sensors previously reported as removed
    pendingRemovals.clear();

    ret += reportPendingFlushes(&data[ret], count);
    ret += reportPendingSensors(&data[ret], count);
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
                // remove from pendingChecks, if it is there.
                pendingChecks.remove_if([e](std::string s){ return s.compare(e->deviceName) == 0; });

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
            S_LOGD("Adding as pending %s", e->deviceName.c_str());
            pendingChecks.push_back(e->deviceName);

            ret += checkPermsAndAddToPending(data, count);
        }
    }

    hasEvents = false;

    return ret;
}

/* IioHAL runs in parallel with ueventd and both monitor the uevent
 * socket for new device additions.  Ueventd (based on ueventd.rc
 * rules) modifies the permissions, owner and group of the new sysfs
 * directories and files in /sys/devices/iio:deviceX.  The files
 * start with a default label of 'sysfs', but are changed to
 * 'sysfs_sensors' by ueventd.  If the labeling is too slow, there
 * can be a race condition where IioHAL accesses the device before
 * its label is updated.  hal_sensors does not have permission to
 * read/write directory entries that are generic label 'sysfs'.
 * Therefore, the access will be denied.  Fix this by ensuring that
 * the device in sysfs is relabeled here before continuing on to
 * actually access it.
 */
int DynamicMetaSensor::checkPermsAndAddToPending(sensors_event_t* data, int & count) {
    int ret = 0;
    char device_sysfs_path[PATH_MAX];
    char *fcon = nullptr;

    // Assume that the newly added devices have the proper permissions for access
    // to start, though they probably don't yet.
    bool allPendingOkay = true;

    //S_LOGD("Checking %zd pending checks...", pendingChecks.size());

    while (!pendingChecks.empty()) {
        strncpy(device_sysfs_path, "/sys/devices/", sizeof(device_sysfs_path) - 1);
        strcat(device_sysfs_path, pendingChecks.front().c_str());
        //S_LOGD("Checking device %s", device_sysfs_path);

        if (access(device_sysfs_path, F_OK) == 0) {
            fcon = nullptr;
            int rc = getfilecon(device_sysfs_path, &fcon);
            S_LOGD("Read context of %s as %s %d", device_sysfs_path, fcon, rc);
            if (rc < 0 || strstr(fcon, "sysfs_sensors") == NULL) {
                // context is not correct yet so try again later
                S_LOGD("device not ready");
                if (fcon)
                    freecon(fcon);
                allPendingOkay = false;
                iioHal.delay();
                break;
            }
            else {
                freecon(fcon);
                // Remove top of permission checking list!
                S_LOGD("device is okay to access");
                pendingChecks.pop_front();
            }
        }
        else {
            S_LOGE("Couldn't access device to check security context!");
            pendingChecks.pop_front();
        }
    }

    if (allPendingOkay) {
        /* Even though we are only notified of a single device at a time, we
         * will try to get all the physically attached devices and add them to
         * the pendingAdditions list at once. Hopefully that will mean that
         * when subsequent UEvents are received for the 2nd device on the
         * same mod, that device will already be in the pendingAdditions list
         * and would be using the same iio_context pointer, thus saving
         * some memory.
         */

        pendingAdditions.splice(pendingAdditions.end(), iioHal.updateSensorList(getIioSensorList()));
        ret += reportPendingSensors(data, count);
    }

    return ret;
}

int DynamicMetaSensor::reportPendingFlushes(sensors_event_t* data, int & count) {
    int ret = 0;

    while (flushResponsesDue > 0 && count > 0) {
        sensors_event_t &d = data[ret];
        d.version = META_DATA_VERSION;
        d.sensor = 0;
        d.type = SENSOR_TYPE_META_DATA;
        d.reserved0 = 0;
        d.timestamp = 0;
        d.meta_data.what = META_DATA_FLUSH_COMPLETE;
        d.meta_data.sensor = myHandle;

        flushResponsesDue--;
        count--;
        ret++;
    }

    return ret;
}

int DynamicMetaSensor::reportPendingSensors(sensors_event_t* data, int & count) {
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
        generateUuid(s, ds);

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

void DynamicMetaSensor::generateUuid(const struct sensor_t &s,
        dynamic_sensor_meta_event_t &ds) {

    // Start off with a Name Space ID - See RFC 4122: Appendix C
    // We'll call this the "Dynamic Sensor Name Space ID"
    vector<uint8_t> input = {
        0x6b, 0xa7, 0xb8, 0x93, /* Made up last byte here. */
        0x9d, 0xad, 0x11, 0xd1,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };

    // TODO: Add mod model or serial number
    string sensorId = android::base::StringPrintf("motorola.com:%s:%s:%d:%d",
            s.name, s.vendor, s.version, s.type);
    input.insert(input.end(), sensorId.begin(), sensorId.end());

    uint8_t sha1[SHA_DIGEST_LENGTH];
    SHA1(&input[0], input.size(), sha1);

    // UUID is 16 bytes. SHA1 is 20 bytes
    static_assert(SHA_DIGEST_LENGTH >= arraysize(ds.uuid), "SHA1 digest length too short");
    static_assert(arraysize(ds.uuid) > 8, "Sensor UUID too short"); // We index blindly to byte 9

    memcpy(ds.uuid, sha1, min(sizeof(sha1), sizeof(ds.uuid)));

    // Encode the version: Name-based version that uses SHA1 hashing (section 4.1.3)
    ds.uuid[6] = (ds.uuid[6] & 0x0f) | 0x50;

    // Encode the variant: 2 MSB bits (6 & 7) must be 0 and 1 respectively
    ds.uuid[8] = (ds.uuid[8] & 0b0011'1111) | 0b1000'0000;

    S_LOGD("sensorId=%s UUID=%s", sensorId.c_str(), toUuidStr(ds.uuid).c_str());
}

string DynamicMetaSensor::toUuidStr(uint8_t *uuid) {
    uint8_t *u = uuid;
    return android::base::StringPrintf(
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             u[0], u[1], u[2], u[3],
             u[4], u[5],
             u[6], u[7],
             u[8], u[9],
             u[10], u[11], u[12], u[13], u[14], u[15]);
}
