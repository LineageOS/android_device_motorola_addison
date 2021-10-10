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

#include <algorithm>
#include <cutils/uevent.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <fcntl.h>

#include "UeventListener.h"
#include "SensorsLog.h"

using namespace std;

/** Beware: The only way to open a UEvent socket from 2 places within the same
 * process is to allow the kernel to assign a unique value for the nl_pid
 * field. Since the sensors HAL runs within the system_server along with the
 * framework, and since both the sensor HAL and the framework need access to
 * the UEvent socket, both of them must open the NETLINK_KOBJECT_UEVENT socket
 * the same way, or else the 2nd one to attempt to open the socket will fail.
 * Specifically, this precludes the use of uevent_init() and friends from
 * hardware_legacy/uevent.h (libhardware_legacy). */
UeventListener::UeventListener(handler_type onAdd, handler_type onRemove)
    : ueventFd(-1), onAdd(onAdd), onRemove(onRemove)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0; // let the kernel assign a unique value
    addr.nl_groups = 0xffFFffFF;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0) {
        S_LOGD("Failed creating socket: %s", strerror(errno));
    } else {
        setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
        if (::bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            close(s);
            S_LOGD("Failed binding socket: %s", strerror(errno));
        } else {
            S_LOGD("Successfully created uevent socket");
            ueventFd = s;
            fcntl(ueventFd, F_SETFL, O_NONBLOCK);
        }
    }
}

UeventListener::~UeventListener() {
    if (ueventFd > 0) {
        close(ueventFd);
    }
}

void UeventListener::logEvent(char *event) {
    S_LOGD("Uevent: %s", event);
    event += strlen(event) + 1;
    while (event[0]) {
        S_LOGD("  %s", event);
        event += strlen(event) + 1;
    }
}

shared_ptr<Uevent> UeventListener::readEvent() {
    char msg[MSG_LEN + 2];

    AutoLock _l(mutex);

    int res = recv(ueventFd, msg, MSG_LEN, 0);
    if (res <= 0) return nullptr;
    if (res >= MSG_LEN) return nullptr; // Overflow. Discard.
    msg[res] = '\0';
    msg[res+1] = '\0';
    //logEvent(msg);

    char *action = strtok(msg, "@");
    if (action) {
        char *dev = strtok(NULL, "/");
        if (dev && !strcmp(dev, "devices")) {
            dev = strtok(NULL, "/");
            if (dev && !strncmp(dev, "iio:device", 10)) {
                //logEvent(msg); // msg was "corrupted" by strtok by now
                if (!strcmp(action, "add")) {
                    if (onAdd) onAdd(dev);
                    return make_shared<Uevent>(Uevent{dev, Uevent::EventType::SensorAdd});
                } else if (!strcmp(action, "remove")) {
                    if (onRemove) onRemove(dev);
                    return make_shared<Uevent>(Uevent{dev, Uevent::EventType::SensorRemove});
                }
            }
        }
    }

    return nullptr;
}
