/*
 * Copyright (C) 2011-2014 Motorola, Inc.
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
#include <dlfcn.h>

#include <linux/akm8975.h>
#include <linux/stml0xx.h>

#include <cutils/log.h>

#include <hardware/mot_sensorhub_stml0xx.h>

#include "sensorhub_hal.h"

/*****************************************************************************/

HubSensor::HubSensor()
: SensorBase(SENSORHUB_DEVICE_NAME, SENSORHUB_AS_DATA_NAME),
      mEnabled(0),
      mWakeEnabled(0),
      mPendingMask(0)
{
    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;
    short flags = 0;
    FILE *fp;
    int i;
    int err = 0;

    memset(mErrorCnt, 0, sizeof(mErrorCnt));

    open_device();

    if (!ioctl(dev_fd, STML0XX_IOCTL_GET_SENSORS, &flags))  {
        mEnabled = flags;
    }

    if (!ioctl(dev_fd, STML0XX_IOCTL_GET_WAKESENSORS, &flags))  {
        mWakeEnabled = flags;
    }
}

HubSensor::~HubSensor()
{
}

int HubSensor::enable(int32_t handle, int en)
{
    int newState  = en ? 1 : 0;
    uint32_t new_enabled;
    int found = 0;
    int err = 0;

    new_enabled = mEnabled;
    switch (handle) {
        case ID_A:
            new_enabled &= ~M_ACCEL;
            if (newState)
                new_enabled |= M_ACCEL;
            found = 1;
            break;
        case ID_L:
            new_enabled &= ~M_ALS;
            if (newState)
                new_enabled |= M_ALS;
            found = 1;
            break;
        case ID_DR:
            new_enabled &= ~M_DISP_ROTATE;
            if (newState)
                new_enabled |= M_DISP_ROTATE;
            found = 1;
            break;
        case ID_A2:
            new_enabled &= ~M_ACCEL2;
            if (newState)
                new_enabled |= M_ACCEL2;
            found = 1;
            break;
    }

    if (found && (new_enabled != mEnabled)) {
        err = ioctl(dev_fd, STML0XX_IOCTL_SET_SENSORS, &new_enabled);
        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        if (!err) {
            mEnabled = new_enabled;
        }
    }

    new_enabled = mWakeEnabled;
    found = 0;
    switch (handle) {
        case ID_P:
            new_enabled &= ~M_PROXIMITY;
            if (newState)
                new_enabled |= M_PROXIMITY;
            found = 1;
            break;
        case ID_FU:
            new_enabled &= ~M_FLATUP;
            if (newState)
                new_enabled |= M_FLATUP;
            found = 1;
            break;
        case ID_FD:
            new_enabled &= ~M_FLATDOWN;
            if (newState)
                new_enabled |= M_FLATDOWN;
            found = 1;
            break;
        case ID_S:
            new_enabled &= ~M_STOWED;
            if (newState)
                new_enabled |= M_STOWED;
            found = 1;
            break;
        case ID_CA:
            new_enabled &= ~M_CAMERA_ACT;
            if (newState)
                new_enabled |= M_CAMERA_ACT;
            found = 1;
            break;
    }

    if (found && (new_enabled != mWakeEnabled)) {
        err = ioctl(dev_fd, STML0XX_IOCTL_SET_WAKESENSORS, &new_enabled);
        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        if (!err) {
            mWakeEnabled = new_enabled;
        }
    }

    return err;
}

int HubSensor::setDelay(int32_t handle, int64_t ns)
{
    int status = -EINVAL;

    if (ns < 0)
        return -EINVAL;

    unsigned short delay = int64_t(ns) / 1000000;
    switch (handle) {
        case ID_A: status = ioctl(dev_fd,  STML0XX_IOCTL_SET_ACC_DELAY, &delay);  break;
        case ID_A2: status = ioctl(dev_fd, STML0XX_IOCTL_SET_ACC2_DELAY, &delay); break;
        case ID_L: status = 0;                                                    break;
        case ID_DR: status = 0;                                                   break;
        case ID_P: status = 0;                                                    break;
        case ID_FU: status = 0;                                                   break;
        case ID_FD: status = 0;                                                   break;
        case ID_S: status = 0;                                                    break;
        case ID_CA: status = 0;                                                   break;
    }
    return status;
}

int HubSensor::readEvents(sensors_event_t* data, int count)
{
    int numEventReceived = 0;
    struct stml0xx_android_sensor_data buff;
    int ret;
    char timeBuf[32];
    struct tm* ptm = NULL;
    struct timeval timeutc;
    static long int sent_bug2go_sec = 0;

    if (count < 1)
        return -EINVAL;

    while (count && ((ret = read(data_fd, &buff, sizeof(struct stml0xx_android_sensor_data))) != 0)) {
        /* Sensorhub reset occurred, upload a bug2go if its been at least 10mins since previous bug2go*/
        /* remove this if-clause when corruption issue resolved */
        switch (buff.type) {
            case DT_FLUSH:
                data->version = META_DATA_VERSION;
                data->sensor = 0;
                data->type = SENSOR_TYPE_META_DATA;
                data->reserved0 = 0;
                data->timestamp = 0;
                data->meta_data.what = META_DATA_FLUSH_COMPLETE;
                data->meta_data.sensor = STM32TOH(buff.data + FLUSH);
                *data++;
                count--;
                numEventReceived++;
                break;
            case DT_ACCEL:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_A;
                data->type = SENSOR_TYPE_ACCELEROMETER;
                data->acceleration.x = STM16TOH(buff.data+ACCEL_X) * CONVERT_A_X;
                data->acceleration.y = STM16TOH(buff.data+ACCEL_Y) * CONVERT_A_Y;
                data->acceleration.z = STM16TOH(buff.data+ACCEL_Z) * CONVERT_A_Z;
                data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_ACCEL2:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_A2;
                data->type = SENSOR_TYPE_ACCELEROMETER;
                data->acceleration.x = STM16TOH(buff.data+ACCEL_X) * CONVERT_A_X;
                data->acceleration.y = STM16TOH(buff.data+ACCEL_Y) * CONVERT_A_Y;
                data->acceleration.z = STM16TOH(buff.data+ACCEL_Z) * CONVERT_A_Z;
                data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_ALS:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_L;
                data->type = SENSOR_TYPE_LIGHT;
                data->light = (uint16_t)STM16TOH(buff.data + LIGHT_LIGHT);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_DISP_ROTATE:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_DR;
                data->type = SENSOR_TYPE_DISPLAY_ROTATE;
                if (buff.data[ROTATE_ROTATE] == DISP_FLAT)
                    data->data[0] = DISP_UNKNOWN;
                else
                    data->data[0] = buff.data[ROTATE_ROTATE];

                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_PROX:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_P;
                data->type = SENSOR_TYPE_PROXIMITY;
                if (buff.data[PROXIMITY_PROXIMITY] == 0) {
                    data->distance = PROX_UNCOVERED;
                    ALOGE("Proximity uncovered");
                } else if (buff.data[PROXIMITY_PROXIMITY] == 1) {
                    data->distance = PROX_COVERED;
                    ALOGE("Proximity covered 1");
                } else {
                    data->distance = PROX_SATURATED;
                    ALOGE("Proximity covered 2");
                }
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_FLAT_UP:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_FU;
                data->type = SENSOR_TYPE_FLAT_UP;
                if (buff.data[FLAT_FLAT] == 0x01)
                    data->data[0] = FLAT_DETECTED;
                else
                    data->data[0] = FLAT_NOTDETECTED;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_FLAT_DOWN:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_FD;
                data->type = SENSOR_TYPE_FLAT_DOWN;
                if (buff.data[FLAT_FLAT] == 0x02)
                    data->data[0] = FLAT_DETECTED;
                else
                    data->data[0] = FLAT_NOTDETECTED;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_STOWED:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_S;
                data->type = SENSOR_TYPE_STOWED;
                data->data[0] = buff.data[STOWED_STOWED];
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_CAMERA_ACT:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_CA;
                data->type = SENSOR_TYPE_CAMERA_ACTIVATE;
                data->data[0] = STML0XX_CAMERA_DATA;
                data->data[1] = STM16TOH(buff.data + CAMERA_CAMERA);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_RESET:
                count--;
                time(&timeutc.tv_sec);
                if (buff.data[0] > 0 && buff.data[0] <= ERROR_TYPES)
                    mErrorCnt[buff.data[0] - 1]++;
                if ((sent_bug2go_sec == 0) ||
                    (timeutc.tv_sec - sent_bug2go_sec > 24*60*60)) {
                    // put timestamp in dropbox file
                    ptm = localtime(&(timeutc.tv_sec));
                    if (ptm != NULL) {
                        strftime(timeBuf, sizeof(timeBuf), "%m-%d %H:%M:%S", ptm);
                        capture_dump(timeBuf, buff.type, SENSORHUB_DUMPFILE,
                            DROPBOX_FLAG_TEXT | DROPBOX_FLAG_GZIP);
                    }
                    sent_bug2go_sec = timeutc.tv_sec;
                }
                break;
            default:
                break;
        }
    }

    return numEventReceived;
}

int HubSensor::flush(int32_t handle)
{
    int ret = 0;
    if (handle >= MIN_SENSOR_ID && handle <= MAX_SENSOR_ID) {
        ret = ioctl(dev_fd,  STML0XX_IOCTL_SET_FLUSH, &handle);
    }
    return ret;
}

gzFile HubSensor::open_dropbox_file(const char* timestamp, const char* dst, const int flags)
{
    char dropbox_path[128];
    pid_t pid = getpid();

    snprintf(dropbox_path, sizeof(dropbox_path), "%s/%s:%d:%u-%s",
             DROPBOX_DIR, DROPBOX_TAG, flags, pid, timestamp);
    ALOGD("stml0xx - dumping to dropbox file[%s]...\n", dropbox_path);

    return gzopen(dropbox_path, "wb");
}

short HubSensor::capture_dump(char* timestamp, const int id, const char* dst, const int flags)
{
    char buffer[COPYSIZE] = {0};
    int rc = 0, i = 0;
    gzFile dropbox_file = NULL;

    dropbox_file = open_dropbox_file(timestamp, dst, flags);
    if(dropbox_file == NULL) {
        ALOGE("ERROR! unable to open dropbox file[errno:%d(%s)]\n", errno, strerror(errno));
    } else {
        // put timestamp in dropbox file
        rc = snprintf(buffer, COPYSIZE, "timestamp:%s\n", timestamp);
        gzwrite(dropbox_file, buffer, rc);
        rc = snprintf(buffer, COPYSIZE, "reason:%02d\n", id);
        gzwrite(dropbox_file, buffer, rc);

        for (i = 0; i < ERROR_TYPES; i++) {
            rc = snprintf(buffer, COPYSIZE, "[%d]:%d\n", i+1, mErrorCnt[i]);
            gzwrite(dropbox_file, buffer, rc);
        }
        memset(mErrorCnt, 0, sizeof(mErrorCnt));

        gzclose(dropbox_file);
        // to commit buffer cache to disk
        sync();
    }

    return 0;
}
