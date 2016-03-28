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

#include <mutex>
#include <functional>

/** This class listens for kernel UDEV events that indicate a dynamic sensor
 * was added to the system.

 * These are examples of iio uevents. The kernel separates each line with '\0'
 * and doesn't indent. Line breaks and indentation added just for readability.
 * Only one event is sent at a time.

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
*/
class UeventListener {
public:
    typedef std::lock_guard<std::recursive_mutex> AutoLock;
    typedef std::function<void(char*)> handler_type;

    /**
     * @param onAdd A function to call when a sensor addition is detected.
     * @param onRemove A function to call when a sensor removal is detected.
     */
    UeventListener(handler_type onAdd, handler_type onRemove);
    virtual ~UeventListener();
    void readEvents();
    int getFd() { return ueventFd; }

private:
    static const int MSG_LEN = 1024;
    int ueventFd;
    std::recursive_mutex mutex;

    void logEvent(char *event);
    /** A function that will be callled when a sensor is added. */
    handler_type onAdd;
    /** A function that will be callled when a sensor is removed. */
    handler_type onRemove;
};
