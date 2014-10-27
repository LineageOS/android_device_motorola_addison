/*
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "ct406_hal.h"

/*****************************************************************************/

LightProxSensor::LightProxSensor()
    : SensorBase(LIGHTPROXIMITY_DEVICE_NAME, "light-prox"),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(8),
      mFlushEnabled(0)
{
    memset(mPendingEvents, 0, sizeof(mPendingEvents));
    memset(&mFlushEvents, 0, sizeof(mFlushEvents));

    mPendingEvents[Light].version = sizeof(sensors_event_t);
    mPendingEvents[Light].sensor = ID_L;
    mPendingEvents[Light].type = SENSOR_TYPE_LIGHT;

    mPendingEvents[Prox ].version = sizeof(sensors_event_t);
    mPendingEvents[Prox ].sensor = ID_P;
    mPendingEvents[Prox ].type = SENSOR_TYPE_PROXIMITY;

    mFlushEvents[Light].version = META_DATA_VERSION;
    mFlushEvents[Light].sensor = 0;
    mFlushEvents[Light].type = SENSOR_TYPE_META_DATA;
    mFlushEvents[Light].reserved0 = 0;
    mFlushEvents[Light].timestamp = 0;
    mFlushEvents[Light].meta_data.what = META_DATA_FLUSH_COMPLETE;
    mFlushEvents[Light].meta_data.sensor = ID_L;

    mFlushEvents[Prox].version = META_DATA_VERSION;
    mFlushEvents[Prox].sensor = 0;
    mFlushEvents[Prox].type = SENSOR_TYPE_META_DATA;
    mFlushEvents[Prox].reserved0 = 0;
    mFlushEvents[Prox].timestamp = 0;
    mFlushEvents[Prox].meta_data.what = META_DATA_FLUSH_COMPLETE;
    mFlushEvents[Prox].meta_data.sensor = ID_P;

    for (int i=0 ; i<numSensors ; i++)
        mDelays[i] = 200000000; // 200 ms by default

    // read the actual value of all sensors if they're enabled already
    int fd;
    char buff[20];
    struct input_absinfo absinfo;

    fd = open("/sys/module/ct406/parameters/als_enable", O_RDONLY);
    if (fd >= 0) {
        read (fd, buff, 19);
        buff[19] = '\0';

        if (buff[0] == '\1') {
            mEnabled |= 1<<Light;
            if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_LIGHT), &absinfo)) {
                mPendingEvents[Light].light = absinfo.value;
            }
        }

        close (fd);
    }
    else
        ALOGE("CT406 error opening als_enable");

    fd = open("/sys/module/ct406/parameters/prox_enable", O_RDONLY);
    if (fd >= 0) {
        read (fd, buff, 19);
        buff[19] = '\0';

        if (buff[0] == '\1') {
            mEnabled |= 1<<Prox;
            if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_PROXIMITY), &absinfo)) {
                mPendingEvents[Prox].distance = absinfo.value * CONVERT_P;
            }
        }

        close (fd);
    }
    else
        ALOGE("CT406 error opening prox_enable");
}

LightProxSensor::~LightProxSensor() {
}

int LightProxSensor::setEnable(int32_t handle, int en)
{
    int what = -1;

    switch (handle) {
        case ID_L: what = Light; break;
        case ID_P: what = Prox;  break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState  = en ? 1 : 0;
    int err = 0;

    if ((uint32_t(newState)<<what) != (mEnabled & (1<<what))) {
        char buff[2] = {'0', '\0'};
        int fd;

        if (newState)
            buff[0] = '1';

        switch (what) {
            case Light:
                fd = open("/sys/module/ct406/parameters/als_enable", O_WRONLY);
                if (fd >= 0) {
                    mEnabled &= ~(1<<what);
                    mEnabled |= (uint32_t(newState)<<what);
                    write(fd, buff, 2);
                    close (fd);
                } else {
                    ALOGE("CT406 error opening als_enable");
                    err = -EIO;
                }
                break;
            case Prox:
                fd = open("/sys/module/ct406/parameters/prox_enable", O_WRONLY);
                if (fd >= 0) {
                    mEnabled &= ~(1<<what);
                    mEnabled |= (uint32_t(newState)<<what);
                    write(fd, buff, 2);
                    close (fd);
                } else {
                    ALOGE("CT406 error opening prox_enable");
                    err = -EIO;
                }
                break;
        }
    }
    return err;
}

int LightProxSensor::getEnable(int32_t handle)
{
        return mEnabled;
}

int LightProxSensor::setDelay(int32_t handle, int64_t ns)
{
    int delay;
    char buff[32];
    int num_chars;
    int fd;
    int what = -1;

    switch (handle) {
        case ID_L: what = Light; break;
        case ID_P: what = Prox;  break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    if (ns < 0)
        return -EINVAL;

    mDelays[what] = ns;

    delay = ns / 1000000;
    num_chars = snprintf (buff, (size_t)32, "%d", delay);
    if ((num_chars < 0) || (num_chars >= 32)) {
        ALOGE("ct406 invalid delay value %d\n", delay);
        return -EINVAL;
    }

    switch (what) {
        case Light:
            fd = open("/sys/module/ct406/parameters/als_delay", O_WRONLY);
            if (fd >= 0) {
                write(fd, buff, num_chars + 1);
                close (fd);
                ALOGE("CT406 set ALS poll interval: %s\n", buff);
            } else {
                ALOGE("CT406 error opening als_delay");
                return -EIO;
            }
            break;
        case Prox:
            break;
    }

    return 0;
}

int LightProxSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    int numEventReceived = 0;
    input_event const* event;

    if(count && mFlushEnabled & BIT_L) {
        mFlushEnabled &= ~(BIT_L);
        *data++ = mFlushEvents[Light];
        count--;
        numEventReceived++;
    }

    if(count && mFlushEnabled & BIT_P) {
        mFlushEnabled &= ~(BIT_P);
        *data++ = mFlushEvents[Prox];
        count--;
        numEventReceived++;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_MSC || type == EV_LED) {
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
                    if (mEnabled & (1<<j)) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;

                        if (j == Prox)
                            ALOGE("LightProxSensor: Prox value %f",
                                mPendingEvents[Prox].distance);
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            ALOGE("LightProxSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }

    return numEventReceived;
}

void LightProxSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_LIGHT:
            mPendingMask |= 1<<Light;
            mPendingEvents[Light].light = value;
            break;
        case EVENT_TYPE_PROXIMITY:
            mPendingMask |= 1<<Prox;
            mPendingEvents[Prox].distance = value * CONVERT_P;
            break;
    }
}

int LightProxSensor::flush(int32_t handle)
{
    int id=0;
    switch (handle) {
        case ID_L: id = BIT_L;  break;
        case ID_P: id = BIT_P;  break;
    }
    mFlushEnabled |= id;
    return 0;
}
