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

#ifndef ANDROID_HUB_SENSOR_H
#define ANDROID_HUB_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <zlib.h>
#include <time.h>
#include <private/android_filesystem_config.h>

#include "linux/stm401.h"

#include "nusensors.h"
#include "SensorBase.h"

/*****************************************************************************/

#define SENSORS_EVENT_T_SIZE sizeof(sensors_event_t);
#define MAG_CAL_FILE "/data/misc/akmd_set.txt"
#define DROPBOX_DIR "/data/system/dropbox-add"
#define DROPBOX_TAG "SENSOR_HUB"
#define SENSORHUB_DUMPFILE  "sensor_hub"
#define DROPBOX_FLAG_TEXT        2
#define DROPBOX_FLAG_GZIP        4
#define COPYSIZE 256

struct input_event;

class HubSensor : public SensorBase {
public:
            HubSensor();
    virtual ~HubSensor();

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t* data, int count);

private:
    int update_delay();
    uint32_t mEnabled;
    uint32_t mWakeEnabled;
    uint32_t mPendingMask;
    uint8_t mMagCal[STM_MAG_CAL_SIZE];
    gzFile open_dropbox_file(const char* timestamp, const char* dst, const int flags);
    short capture_dump(char* timestamp, const int id, const char* dst, const int flags);
};

/*****************************************************************************/

#endif  // ANDROID_HUB_SENSOR_H
