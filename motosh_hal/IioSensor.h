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

#include <vector>
#include <memory>
#include <chrono>

#include <hardware/sensors.h>
#include <base/macros.h>

#include "Sensors.h" // To get the SENSORS_HANDLE_BASE_IIO
#include "SensorBase.h"
#include "SensorsLog.h"
#include "iio.h"

class IioSensor : public SensorBase {
public:
    IioSensor(std::shared_ptr<struct iio_context> iio_ctx,
            const struct iio_device* dev, int handle);
    DISALLOW_IMPLICIT_CONSTRUCTORS(IioSensor);
    virtual ~IioSensor();

    virtual int readEvents(sensors_event_t* data, int count) override;

    virtual int readEvents(sensors_event_t* data, int count, int fd) override {
        if (fd == eventFd) {
            return readIioEvents(data, count);
        } else {
            return readEvents(data, count);
        }
    }

    virtual bool hasPendingEvents() const override {
        return remaining_samples > 0;
    }
    virtual int getFd() const override;

    virtual int setEnable(int32_t handle, int enabled) override;
    /** This function may called after setEnabled() */
    virtual int batch(int32_t handle, int flags,
            int64_t sampling_period_ns, int64_t max_report_latency_ns) override;
    virtual int flush(int32_t handle) override;
    virtual bool hasSensor(int handle) override;

    /** Creates and configures a local IIO context. This will only return a
     * valid context the first time it is called. The first caller must pass
     * the context around as needed.
     *
     * @return the local context, or a NULL pointer if the local context is not
     * accessible (SELinux or file permissions preventing libiio from reading
     * relevant sysfs entries). */
    static std::shared_ptr<struct iio_context> createIioContext();

    /** Updates the list of IIO sensors managed by this class.
     *
     * Caller must hold driversLock before calling this function.
     *
     * TODO: Call this when a mod is attached/detached
     */
    static void updateSensorList(std::shared_ptr<struct iio_context> iio_ctx);

    /** Checks to see if a given IIO device can be handled by this class */
    static bool isUsable(const struct iio_device *dev);

    static std::vector<std::shared_ptr<IioSensor>> &getSensors() {
        return sensors;
    }

    struct sensor_t &getHalSensor() {
        return sensor;
    }
    virtual void getSensorsList(std::vector<struct sensor_t> &list) {
        //S_LOGD("handle=%d", sensor.handle);
        list.push_back(sensor);
    }

    virtual int getEventFd() const {
        return eventFd;
    }

protected:
    static std::vector<std::shared_ptr<IioSensor>> sensors;

    /// Default IIO buffer length. The number of samples the IIO buffer can hold.
    static const int BUFFER_LEN = 5;

    /// This is the maximum number of channels handled by the Android framework.
    static const int MAX_CHANNELS = 16;

    /** When readEvents() is called, if the destination can not hold all the
     * samples we've acquired, we keep track of how many more we have to read
     * from the current buffer before refilling it. */
    int remaining_samples;

    // File descriptor on which we listen for events.
    int eventFd;

    /** Must be set to slightly longer than the fastest sample rate.
     * The same value is used for all devices/sensors. */
    static std::chrono::milliseconds timeout;

    static const int FIRST_HANDLE = SENSORS_HANDLE_BASE_IIO;

    /** @{
     * iiolib handles
     */
    std::shared_ptr<struct iio_context> iio_ctx;
    const struct iio_device *iio_dev;
    struct iio_buffer *iio_buf;
    /** @} */

    struct sensor_t sensor;

    double iio_scale, iio_offset;

    /** Converts an unscaled value to float, applying the device's offset/scale. */
    inline double convVal(int32_t raw) {
        return (static_cast<double>(raw) + iio_offset) * iio_scale;
    }

    /** Read an attribute that is of string type from sysfs.
     *
     * @param lenAttr The name of an integer attribute that tells us the size
     * of the "string" we want to read. We're assuming the string may contain
     * nulls so we need to know the length.
     *
     * @param strAttr The name of the string attribute to read.
     *
     * @param defaultVal A default value to return if reading the strAttr
     * failed.
     *
     * @return The caller is responsible for freeing the string returned by
     * this function.
     */
    const char *readIioStr(const char* lenAttr, const char* strAttr, const char* defaultVal);

    /** Read an attribute of integral type from sysfs.
     *
     * @param attr Attribute name
     * @param defaultVal A default value to return if reading from sysfs failed.
     * @return The value of the attribute, or the default value if the read failed.
     */
    template<typename T> T readIioInt(const char* attr, T defaultVal) {
        long long intAttr;
        if (0 == iio_device_attr_read_longlong(iio_dev, attr, &intAttr)) {
            return intAttr;
        } else {
            return defaultVal;
        }
    }

    /** Computes the offset of each channel's data, relative to the start of a
     * data sample (or event). */
    void computeChannelOffsets(void);

    /** The offset of each channel's data relative to the start of a data
     * sample. */
    std::vector<ptrdiff_t> channel_offset;

    virtual int readIioEvents(sensors_event_t* data, int count);
};
