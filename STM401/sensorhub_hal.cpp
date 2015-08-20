/*
 * Copyright (C) 2011- 2012 Motorola, Inc.
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
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <linux/akm8975.h>
#include <linux/stm401.h>

#include <cutils/log.h>

#include <hardware/mot_sensorhub_stm401.h>

#include "sensorhub_hal.h"

/*****************************************************************************/

HubSensor::HubSensor()
: SensorBase(SENSORHUB_DEVICE_NAME, SENSORHUB_AS_DATA_NAME),
      mEnabled(0),
      mWakeEnabled(0),
      mPendingMask(0),
      mOrientEnabled(0),
      mMagEnabled(0),
      mUncalMagEnabled(0),
      mMagReqDelay(USHRT_MAX),
      mUncalMagReqDelay(USHRT_MAX),
      mOrientReqDelay(USHRT_MAX),
      mEcompassDelay(USHRT_MAX)
{
    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;
    short flags = 0;
    FILE *fp;
    int i;
    int err = 0;

    memset(mMagCal, 0, sizeof(mMagCal));
    memset(mErrorCnt, 0, sizeof(mErrorCnt));

    open_device();

    if (!ioctl(dev_fd, STM401_IOCTL_GET_SENSORS, &flags))  {
        mEnabled = flags;
    }

    if (!ioctl(dev_fd, STM401_IOCTL_GET_WAKESENSORS, &flags))  {
        mWakeEnabled = flags;
    }

    if ((fp = fopen(MAG_CAL_FILE, "r")) != NULL) {
        for (i=0; i<STM401_MAG_CAL_SIZE; i++) {
            mMagCal[i] = fgetc(fp);
        }
        fclose(fp);
        err = ioctl(dev_fd, STM401_IOCTL_SET_MAG_CAL, &mMagCal);
        if (err < 0) {
           ALOGE("Can't send Mag Cal data");
        }
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
        case ID_G:
            new_enabled &= ~M_GYRO;
            if (newState)
                new_enabled |= M_GYRO;
            found = 1;
            break;
        case ID_PR:
            new_enabled &= ~M_PRESSURE;
            if (newState)
                new_enabled |= M_PRESSURE;
            found = 1;
            break;
        case ID_M:
            // Mag and orientation are tied together via ecompass sensor. Must
            // set new_enabled only after checking both mMagEnabled and mOrientEnabled.
            mMagEnabled = newState;
            if( !mMagEnabled )
            {
                // Reset mag requested rate if not enabled
                mMagReqDelay = USHRT_MAX;
            }
            found = 1;
            break;
        case ID_O:
            // Mag and orientation are tied together via ecompass sensor. Must
            // set new_enabled only after checking both mMagEnabled and mOrientEnabled.
            mOrientEnabled = newState;
            if( !mOrientEnabled )
            {
                // Reset orientation requested rate if not enabled
                mOrientReqDelay = USHRT_MAX;
            }
            found = 1;
            break;
        case ID_T:
            new_enabled &= ~M_TEMPERATURE;
            if (newState)
                new_enabled |= M_TEMPERATURE;
            found = 1;
            break;
        case ID_L:
            new_enabled &= ~M_ALS;
            if (newState)
                new_enabled |= M_ALS;
            found = 1;
            break;
#ifdef _ENABLE_LA
        case ID_LA:
            new_enabled &= ~M_LIN_ACCEL;
            if (newState)
                new_enabled |= M_LIN_ACCEL;
            found = 1;
            break;
#endif
#ifdef _ENABLE_GR
        case ID_GR:
            new_enabled &= ~M_GRAVITY;
            if (newState)
                new_enabled |= M_GRAVITY;
            found = 1;
            break;
#endif
        case ID_DR:
            new_enabled &= ~M_DISP_ROTATE;
            if (newState)
                new_enabled |= M_DISP_ROTATE;
            found = 1;
            break;
	case ID_IR_GESTURE:
            new_enabled &= ~M_IR_GESTURE;
            if (newState)
                new_enabled |= M_IR_GESTURE;
            found = 1;
            break;
        case ID_IR_RAW:
            new_enabled &= ~M_IR_RAW;
            if (newState)
                new_enabled |= M_IR_RAW;
            found = 1;
            break;
        case ID_IR_OBJECT:
            new_enabled &= ~M_IR_OBJECT;
            if (newState)
                new_enabled |= M_IR_OBJECT;
            found = 1;
            break;
        case ID_UNCALIB_GYRO:
            new_enabled &= ~M_UNCALIB_GYRO;
            if (newState)
                new_enabled |= M_UNCALIB_GYRO;
            found = 1;
            break;
	case ID_UNCALIB_MAG:
            mUncalMagEnabled = newState;
            new_enabled &= ~M_UNCALIB_MAG;
            if (newState)
                new_enabled |= M_UNCALIB_MAG;
            else
            {
                // Reset uncal mag requested rate if disabled
                mUncalMagReqDelay = USHRT_MAX;
            }
            found = 1;
            break;
#ifdef _ENABLE_PEDO
	case ID_STEP_COUNTER:
            new_enabled &= ~M_STEP_COUNTER;
            if (newState)
                new_enabled |= M_STEP_COUNTER;
            found = 1;
            break;
	case ID_STEP_DETECTOR:
            new_enabled &= ~M_STEP_DETECTOR;
            if (newState)
                new_enabled |= M_STEP_DETECTOR;
            found = 1;
            break;
#endif
        case ID_QUAT_6AXIS:
            new_enabled &= ~M_QUAT_6AXIS;
            if (newState)
                new_enabled |= M_QUAT_6AXIS;
            found = 1;
            break;
        case ID_QUAT_9AXIS:
            new_enabled &= ~M_QUAT_9AXIS;
            if (newState)
                new_enabled |= M_QUAT_9AXIS;
            found = 1;
            break;
    } // end switch(handle)

    // Mag and orientation are tied to same physical sensor
    if( handle == ID_M || handle == ID_O || handle == ID_UNCALIB_MAG )
    {
        // May need to update the rate
        updateEcompassRate();

        // Turn off ecompass sensor only if both mag and orientation are deregistered
        if( !mMagEnabled && !mOrientEnabled )
        {
            new_enabled &= ~M_ECOMPASS;

            // Also read calibration data at this time
            FILE *fp;
            int i;

            err = ioctl(dev_fd, STM401_IOCTL_GET_MAG_CAL, &mMagCal);
            if (err < 0) {
                ALOGE("Can't read Mag Cal data");
            } else {
                if ((fp = fopen(MAG_CAL_FILE, "w")) == NULL) {
                    ALOGE("Can't open Mag Cal file");
                } else {
                    for (i=0; i<STM401_MAG_CAL_SIZE; i++) {
                        fputc(mMagCal[i], fp);
                    }
                    fclose(fp);
                }
            }
        } // Otherwise, we are enabling either mag or orientation, so turn on ecompass
        else
            new_enabled |= M_ECOMPASS;
    } // end if( handle == ID_M || handle == ID_O )

    if (found && (new_enabled != mEnabled)) {
        err = ioctl(dev_fd, STM401_IOCTL_SET_SENSORS, &new_enabled);
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
        case ID_SIM:
            new_enabled &= ~M_SIM;
            if (newState)
                new_enabled |= M_SIM;
            found = 1;
            break;
#ifdef _ENABLE_CHOPCHOP
        case ID_CHOPCHOP_GESTURE:
            new_enabled &= ~M_CHOPCHOP;
            if (newState)
                new_enabled |= M_CHOPCHOP;
            found = 1;
            break;
#endif
#ifdef _ENABLE_LIFT
        case ID_LIFT_GESTURE:
            new_enabled &= ~M_LIFT;
            if (newState)
                new_enabled |= M_LIFT;
            found = 1;
            break;
#endif
    }

    if (found && (new_enabled != mWakeEnabled)) {
        err = ioctl(dev_fd, STM401_IOCTL_SET_WAKESENSORS, &new_enabled);
        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        if (!err) {
            mWakeEnabled = new_enabled;
        }
    }

    return err;
}

int HubSensor::setDelay(int32_t handle, int64_t ns)
{
    int rateFd;
    int status = -EINVAL;

    if (ns < 0)
        return -EINVAL;

    unsigned short delay = int64_t(ns) / 1000000;
    switch (handle) {
        case ID_A: status = ioctl(dev_fd,  STM401_IOCTL_SET_ACC_DELAY, &delay);   break;
        case ID_G: status = ioctl(dev_fd,  STM401_IOCTL_SET_GYRO_DELAY, &delay);  break;
        case ID_PR: status = ioctl(dev_fd,  STM401_IOCTL_SET_PRES_DELAY, &delay); break;
        case ID_M:
            mMagReqDelay = delay;
            break;
        case ID_O:
            mOrientReqDelay = delay;
            break;
        case ID_T: status = 0;                                                    break;
        case ID_L: status = 0;                                                    break;
#ifdef _ENABLE_LA
        case ID_LA: status = 0;                                                   break;
#endif
#ifdef _ENABLE_GR
        case ID_GR: status = 0;                                                   break;
#endif
        case ID_DR: status = 0;                                                   break;
        case ID_P: status = 0;                                                    break;
        case ID_FU: status = 0;                                                   break;
        case ID_FD: status = 0;                                                   break;
        case ID_S: status = 0;                                                    break;
        case ID_CA: status = 0;                                                   break;
        case ID_IR_GESTURE: status = ioctl(dev_fd, STM401_IOCTL_SET_IR_GESTURE_DELAY, &delay); break;
        case ID_IR_RAW: status = ioctl(dev_fd, STM401_IOCTL_SET_IR_RAW_DELAY, &delay); break;
        case ID_IR_OBJECT: status = 0;                                            break;
        case ID_SIM: status = 0;                                                  break;
        case ID_UNCALIB_GYRO: status = ioctl(dev_fd,  STM401_IOCTL_SET_GYRO_DELAY, &delay); break;
        case ID_UNCALIB_MAG:
            mUncalMagReqDelay = delay;
            break;
#ifdef _ENABLE_PEDO
        case ID_STEP_COUNTER:
		    delay /= 1000; // convert to seconds for pedometer rate
		    if (delay == 0)
		        delay = 1;
		    status = ioctl(dev_fd,  STM401_IOCTL_SET_STEP_COUNTER_DELAY, &delay);
		    break;
        case ID_STEP_DETECTOR:status = 0;                                         break;
#endif
#ifdef _ENABLE_CHOPCHOP
        case ID_CHOPCHOP_GESTURE: status = 0;                                     break;
#endif
#ifdef _ENABLE_LIFT
        case ID_LIFT_GESTURE: status = 0;                                         break;
#endif
        case ID_QUAT_6AXIS:
            rateFd = open(QUAT_6AXIS_RATE_ATTR_NAME, O_WRONLY);
            if (rateFd < 0) {
                status = rateFd;
                break;
            }
            status = write(rateFd, &delay, sizeof(uint8_t));
            if (status == sizeof(uint8_t))
                status = 0;
            close(rateFd);
            break;
        case ID_QUAT_9AXIS:
            rateFd = open(QUAT_9AXIS_RATE_ATTR_NAME, O_WRONLY);
            if (rateFd < 0) {
                status = rateFd;
                break;
            }
            status = write(rateFd, &delay, sizeof(uint8_t));
            if (status == sizeof(uint8_t))
                status = 0;
            close(rateFd);
            break;
    }

    if( handle == ID_M || handle == ID_O || handle == ID_UNCALIB_MAG )
        status = updateEcompassRate();

    return status;
}

void HubSensor::logAlsEvent(int16_t lux, int64_t ts_ns) {
    static int16_t last_logged_val = -1;
    static int64_t last_logged_ts_ns;
    int16_t luxDelta = abs(lux - last_logged_val);
    if (last_logged_val == -1 ||
        (luxDelta > last_logged_val * 0.15 && luxDelta >= 5 &&
         ts_ns - last_logged_ts_ns >= 1000000000LL)) {
        ALOGD("ALS %d", lux);
        last_logged_val = lux;
        last_logged_ts_ns = ts_ns;
    }
}

int HubSensor::readEvents(sensors_event_t* data, int count)
{
    int numEventReceived = 0;
    struct stm401_android_sensor_data buff;
    int ret;
    char timeBuf[32];
    struct tm* ptm = NULL;
    struct timeval timeutc;
    static long int sent_bug2go_sec = 0;

    if (!data) {
        ALOGE("HubSensor::readEvents - null data buffer");
        return -EINVAL;
    }
    if (count < 1) {
        ALOGE("HubSensor::readEvents - bad count %d", count);
        return -EINVAL;
    }

    while (count && ((ret = read(data_fd, &buff, sizeof(struct stm401_android_sensor_data))) != 0)) {
        /* these sensors are not supported, upload a bug2go if its been at least 10mins since previous bug2go*/
        /* remove this if-clause when corruption issue resolved */
        if (buff.type == DT_PRESSURE || buff.type == DT_TEMP || buff.type == DT_LIN_ACCEL ||
            buff.type == DT_GRAVITY || buff.type == DT_DOCK ||
            buff.type == DT_NFC || buff.type == DT_RESET) {
            count--;
            if (buff.type == DT_RESET) {
                //reset reason should be between 1 and 4
                if (buff.data[0] >= 1 && buff.data[0] <= 4)
                    mErrorCnt[buff.data[0]]++;
            } else {
                //index 0 is for counting invalid sensor type occurrences
                mErrorCnt[0]++;
            }

            time(&timeutc.tv_sec);

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
            continue;
        }

        switch (buff.type) {
            case DT_FLUSH:
                data->version = META_DATA_VERSION;
                data->sensor = 0;
                data->type = SENSOR_TYPE_META_DATA;
                data->reserved0 = 0;
                data->timestamp = 0;
                data->meta_data.what = META_DATA_FLUSH_COMPLETE;
                data->meta_data.sensor = STM32TOH(buff.data + FLUSH_FLUSH);
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
            case DT_GYRO:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_G;
                data->type = SENSOR_TYPE_GYROSCOPE;
                data->gyro.x = STM16TOH(buff.data + GYRO_X) * CONVERT_G_P;
                data->gyro.y = STM16TOH(buff.data + GYRO_Y) * CONVERT_G_R;
                data->gyro.z = STM16TOH(buff.data + GYRO_Z) * CONVERT_G_Y;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_UNCALIB_GYRO:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_UNCALIB_GYRO;
                data->type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
                data->uncalibrated_gyro.x_uncalib = STM16TOH(buff.data + UNCALIB_GYRO_X) * CONVERT_G_P;
                data->uncalibrated_gyro.y_uncalib = STM16TOH(buff.data + UNCALIB_GYRO_Y) * CONVERT_G_R;
                data->uncalibrated_gyro.z_uncalib = STM16TOH(buff.data + UNCALIB_GYRO_Z) * CONVERT_G_Y;
                data->uncalibrated_gyro.x_bias = STM16TOH(buff.data + UNCALIB_GYRO_X_BIAS) * CONVERT_BIAS_G_P;
                data->uncalibrated_gyro.y_bias = STM16TOH(buff.data + UNCALIB_GYRO_Y_BIAS) * CONVERT_BIAS_G_R;
                data->uncalibrated_gyro.z_bias = STM16TOH(buff.data + UNCALIB_GYRO_Z_BIAS) * CONVERT_BIAS_G_Y;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_UNCALIB_MAG:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_UNCALIB_MAG;
                data->type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
                data->uncalibrated_magnetic.x_uncalib = STM16TOH(buff.data + UNCALIB_MAGNETIC_X) * CONVERT_M_X;
                data->uncalibrated_magnetic.y_uncalib = STM16TOH(buff.data + UNCALIB_MAGNETIC_Y) * CONVERT_M_Y;
                data->uncalibrated_magnetic.z_uncalib = STM16TOH(buff.data + UNCALIB_MAGNETIC_Z) * CONVERT_M_Z;
                data->uncalibrated_magnetic.x_bias = STM16TOH(buff.data + UNCALIB_MAGNETIC_X_BIAS) * CONVERT_BIAS_M_X;
                data->uncalibrated_magnetic.y_bias = STM16TOH(buff.data + UNCALIB_MAGNETIC_Y_BIAS) * CONVERT_BIAS_M_Y;
                data->uncalibrated_magnetic.z_bias = STM16TOH(buff.data + UNCALIB_MAGNETIC_Z_BIAS) * CONVERT_BIAS_M_Z;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_QUAT_6AXIS:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_QUAT_6AXIS;
                data->type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
                data->data[0] = STM16TOH(buff.data + QUAT_6AXIS_A) * CONVERT_RV;
                data->data[1] = STM16TOH(buff.data + QUAT_6AXIS_B) * CONVERT_RV;
                data->data[2] = STM16TOH(buff.data + QUAT_6AXIS_C) * CONVERT_RV;
                data->data[3] = STM16TOH(buff.data + QUAT_6AXIS_W) * CONVERT_RV;
                data->data[4] = 5.f * (3.14159f/180.f); // 5 degrees accuracy?
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_QUAT_9AXIS:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_QUAT_9AXIS;
                data->type = SENSOR_TYPE_ROTATION_VECTOR;
                data->data[0] = STM16TOH(buff.data + QUAT_9AXIS_A) * CONVERT_RV;
                data->data[1] = STM16TOH(buff.data + QUAT_9AXIS_B) * CONVERT_RV;
                data->data[2] = STM16TOH(buff.data + QUAT_9AXIS_C) * CONVERT_RV;
                data->data[3] = STM16TOH(buff.data + QUAT_9AXIS_W) * CONVERT_RV;
                data->data[4] = 5.f * (3.14159f/180.f); // 5 degrees accuracy?
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
#ifdef _ENABLE_PEDO
            case DT_STEP_COUNTER:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_STEP_COUNTER;
                data->type = SENSOR_TYPE_STEP_COUNTER;
                data->u64.step_counter =  (
                        (((uint64_t)STM16TOH(buff.data + STEP_COUNTER_3)) << 48) |
                        (((uint64_t)STM16TOH(buff.data + STEP_COUNTER_2)) << 32) |
                        (((uint64_t)STM16TOH(buff.data + STEP_COUNTER_1)) << 16) |
                        (((uint64_t)STM16TOH(buff.data + STEP_COUNTER_0))) );
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_STEP_DETECTOR:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_STEP_DETECTOR;
                data->type = SENSOR_TYPE_STEP_DETECTOR;
                data->data[0] = STM16TOH(buff.data + STEP_DETECTOR);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
#endif
            case DT_PRESSURE:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_PR;
                data->type = SENSOR_TYPE_PRESSURE;
                data->pressure = STM32TOH(buff.data + PRESSURE_PRESSURE) * CONVERT_B;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_MAG:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_M;
                data->type = SENSOR_TYPE_MAGNETIC_FIELD;
                data->magnetic.x = STM16TOH(buff.data + MAGNETIC_X) * CONVERT_M_X;
                data->magnetic.y = STM16TOH(buff.data + MAGNETIC_Y) * CONVERT_M_Y;
                data->magnetic.z = STM16TOH(buff.data + MAGNETIC_Z) * CONVERT_M_Z;
                data->magnetic.status = buff.status;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_ORIENT:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_O;
                data->type = SENSOR_TYPE_ORIENTATION;
                data->orientation.azimuth = STM16TOH(buff.data + ORIENTATION_AZIMUTH) * CONVERT_O_Y;
                data->orientation.pitch = STM16TOH(buff.data + ORIENTATION_PITCH) * CONVERT_O_P;
                // Roll value needs to be negated.
                data->orientation.roll = -STM16TOH(buff.data + ORIENTATION_ROLL) * CONVERT_O_R;
                data->orientation.status = buff.status;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_TEMP:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_T;
                data->type = SENSOR_TYPE_TEMPERATURE;
                data->temperature = STM16TOH(buff.data + TEMPERATURE_TEMPERATURE) * CONVERT_T;
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
                logAlsEvent((int16_t)data->light, data->timestamp);
                data++;
                count--;
                numEventReceived++;
                break;
#ifdef _ENABLE_LA
            case DT_LIN_ACCEL:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_LA;
                data->type = SENSOR_TYPE_LINEAR_ACCELERATION;
                data->acceleration.x = STM16TOH(buff.data + ACCEL_X) * CONVERT_A_LIN;
                data->acceleration.y = STM16TOH(buff.data + ACCEL_Y) * CONVERT_A_LIN;
                data->acceleration.z = STM16TOH(buff.data + ACCEL_Z) * CONVERT_A_LIN;
                data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
#endif
#ifdef _ENABLE_GR
            case DT_GRAVITY:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_GR;
                data->type = SENSOR_TYPE_GRAVITY;
                data->acceleration.x = STM16TOH(buff.data + ACCEL_X) * CONVERT_A_GRAV;
                data->acceleration.y = STM16TOH(buff.data + ACCEL_Y) * CONVERT_A_GRAV;
                data->acceleration.z = STM16TOH(buff.data + ACCEL_Z) * CONVERT_A_GRAV;
                data->acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
#endif
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
                data->data[0] = STM401_CAMERA_DATA;
                data->data[1] = STM16TOH(buff.data + CAMERA_CAMERA);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_IR_GESTURE:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_IR_GESTURE;
                data->type = SENSOR_TYPE_IR_GESTURE;
                data->ir_gesture.event_id = buff.data[IR_EVENT];
                data->ir_gesture.gesture_id = buff.data[IR_GESTURE];
                data->ir_gesture.direction = buff.data[IR_DIRECTION] & 0x0F;
                data->ir_gesture.magnitude = buff.data[IR_MAGNITUDE] >> 4;
                data->ir_gesture.motion = buff.data[IR_MOTION];
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_IR_RAW:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_IR_RAW;
                data->type = SENSOR_TYPE_IR_RAW;
                data->ir_raw.top_right_high = STM16TOH(buff.data + IR_TR_H);
                data->ir_raw.bottom_left_high = STM16TOH(buff.data + IR_BL_H);
                data->ir_raw.bottom_right_high = STM16TOH(buff.data + IR_BR_H);
                data->ir_raw.bottom_both_high = STM16TOH(buff.data + IR_BB_H);
                data->ir_raw.top_right_low = STM16TOH(buff.data + IR_TR_L);
                data->ir_raw.bottom_left_low = STM16TOH(buff.data + IR_BL_L);
                data->ir_raw.bottom_right_low = STM16TOH(buff.data + IR_BR_L);
                data->ir_raw.bottom_both_low = STM16TOH(buff.data + IR_BB_L);
                data->ir_raw.ambient_high = STM16TOH(buff.data + IR_AMBIENT_H);
                data->ir_raw.ambient_low = STM16TOH(buff.data + IR_AMBIENT_L);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
            case DT_IR_OBJECT:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_IR_OBJECT;
                data->type = SENSOR_TYPE_IR_OBJECT;
                data->data[0] = (*(buff.data + IR_OBJ) >> IR_OBJ_SHIFT) & 0x01;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                enable(ID_IR_OBJECT, 0); /* One-shot sensor. Disable now */
                break;
            case DT_SIM:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_SIM;
                data->type = SENSOR_TYPE_SIGNIFICANT_MOTION;
                data->data[0] = 1;
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                enable(ID_SIM, 0);
                break;
#ifdef _ENABLE_CHOPCHOP
            case DT_CHOPCHOP:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_CHOPCHOP_GESTURE;
                data->type = SENSOR_TYPE_CHOPCHOP_GESTURE;
                data->data[0] = STM16TOH(buff.data + CHOPCHOP_CHOPCHOP);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
#endif
#ifdef _ENABLE_LIFT
            case DT_LIFT:
                data->version = SENSORS_EVENT_T_SIZE;
                data->sensor = ID_LIFT_GESTURE;
                data->type = SENSOR_TYPE_LIFT_GESTURE;
                data->data[0] = STM32TOH(buff.data + LIFT_DISTANCE);
                data->data[1] = STM32TOH(buff.data + LIFT_ROTATION);
                data->data[2] = STM32TOH(buff.data + LIFT_GRAV_DIFF);
                data->timestamp = buff.timestamp;
                data++;
                count--;
                numEventReceived++;
                break;
#endif
            case DT_RESET:
                count--;
                // put timestamp in dropbox file
                time(&timeutc.tv_sec);
                ptm = localtime(&(timeutc.tv_sec));
                if (ptm != NULL) {
                    strftime(timeBuf, sizeof(timeBuf), "%m-%d %H:%M:%S", ptm);
                    capture_dump(timeBuf, buff.data[0], SENSORHUB_DUMPFILE,
                         DROPBOX_FLAG_TEXT | DROPBOX_FLAG_GZIP);
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
        ret = ioctl(dev_fd,  STM401_IOCTL_SET_FLUSH, &handle);
    }
    return ret;
}

gzFile HubSensor::open_dropbox_file(const char* timestamp, const char* dst, const int flags)
{
    char dropbox_path[128];
    pid_t pid = getpid();

    snprintf(dropbox_path, sizeof(dropbox_path), "%s/%s:%d:%u-%s",
             DROPBOX_DIR, DROPBOX_TAG, flags, pid, timestamp);
    ALOGD("stm401 - dumping to dropbox file[%s]...\n", dropbox_path);

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
            rc = snprintf(buffer, COPYSIZE, "[%d]:%d\n", i, mErrorCnt[i]);
            gzwrite(dropbox_file, buffer, rc);
        }
        memset(mErrorCnt, 0, sizeof(mErrorCnt));

        gzclose(dropbox_file);
        // to commit buffer cache to disk
        sync();
    }

    return 0;
}

int HubSensor::updateEcompassRate()
{
    int ret = 0;

    const size_t reqDelays_len = 3;
    unsigned short reqDelays[reqDelays_len] = {
        mMagEnabled      ? mMagReqDelay      : (unsigned short)USHRT_MAX,
        mOrientEnabled   ? mOrientReqDelay   : (unsigned short)USHRT_MAX,
        mUncalMagEnabled ? mUncalMagReqDelay : (unsigned short)USHRT_MAX
    };
    unsigned short minReqDelay = USHRT_MAX;

    // Get minReqDelay
    for( size_t i = 0; i < reqDelays_len; ++i )
    {
        if( reqDelays[i] < minReqDelay )
            minReqDelay = reqDelays[i];
    }

    // Do ioctl if we have a valid delay and it's different
    if( minReqDelay == USHRT_MAX )
        mEcompassDelay = USHRT_MAX;
    else if( mEcompassDelay != minReqDelay )
    {
        mEcompassDelay = minReqDelay;
        ret = ioctl(dev_fd,  STM401_IOCTL_SET_MAG_DELAY, &mEcompassDelay);
    }

    return ret;
}
