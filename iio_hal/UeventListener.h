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

#ifndef UEVENT_LISTENER
#define UEVENT_LISTENER

#include <mutex>
#include <functional>
#include <string>
#include <memory>

struct Uevent;

/** This class listens for kernel UDEV events that indicate a dynamic sensor
 * was added to the system.

 * These are examples of iio uevents. The kernel separates each line with '\0'
 * and doesn't indent. Line breaks and indentation added just for readability.
 * Only one event is sent at a time.

<pre>
    add@/devices/iio:device4
      ACTION=add
      DEVPATH=/devices/iio:device4
      SUBSYSTEM=iio
      MAJOR=247
      MINOR=4
      DEVNAME=iio:device4
      DEVTYPE=iio_device
      SEQNUM=5183
    remove@/devices/iio:device2
      ACTION=remove
      DEVPATH=/devices/iio:device2
      SUBSYSTEM=iio
      MAJOR=247
      MINOR=2
      DEVNAME=iio:device2
      DEVTYPE=iio_device
      SEQNUM=5184
</pre>
*/
class UeventListener {
public:
    typedef std::lock_guard<std::recursive_mutex> AutoLock;
    /**
     * The handler will receive as its argument the name of the IIO device.
     * Using the example above, that would be "iio:device4" and "iio:device2".
     */
    typedef std::function<void(char*)> handler_type;

    /**
     * @param onAdd A function to call when a sensor addition is detected. May
     * be nullptr if no call-back is desired.
     * @param onRemove A function to call when a sensor removal is detected.
     * May be nullptr if no call-back is desired.
     */
    UeventListener(handler_type onAdd, handler_type onRemove);
    virtual ~UeventListener();
    std::shared_ptr<Uevent> readEvent();
    int getFd() const { return ueventFd; }

private:
    static const int MSG_LEN = 1024;
    int ueventFd;
    std::recursive_mutex mutex;

    /** Logs a UEvent for debug purposes. */
    void logEvent(char *event);
    /** A function that will be callled when a sensor is added. */
    handler_type onAdd;
    /** A function that will be callled when a sensor is removed. */
    handler_type onRemove;
};

/** An IIO sensor event. */
struct Uevent {
    enum struct EventType : uint8_t {
        Undefined, SensorAdd, SensorRemove
    };

    /// The device for which the event was generated. Ex: "iio:device4"
    std::string deviceName;
    /// The reason the event was generated.
    EventType eventType;
};

#endif // UEVENT_LISTENER
