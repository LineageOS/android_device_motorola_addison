/*
 * Copyright (C) 2011 Motorola Mobility, Inc.
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

#include <linux/lis3dh_mot.h>

#include <cutils/log.h>

#include "lis3dh_hal.h"

extern bool is_device_moving;
extern bool is_gyro_calibrated;
static unsigned int mCurrentDataNumber = 0;
static unsigned int mLastLowNumber = 0;
static unsigned int mLastHighNumber = 0;
static unsigned long mm_mmoveme_time = 10; // init to max

#define MM_ALLOWED_STD_DEV  0.03f

/*****************************************************************************/

AccelerationSensor::AccelerationSensor()
    : SensorBase(ACCELEROMETER_DEVICE_NAME, "accelerometer"),
      mEnabled(0),
      mState(0),
      mAccelDelay(-1),
      mOrientDelay(-1),
      mDelay(-1),
      mInputReader(32),
      mPendingMask(0),
      mFlushEnabled(0)
{
    memset(&mPendingEvent, 0, sizeof(mPendingEvent));
    memset(&mFlushEvents, 0, sizeof(mFlushEvents));

    mPendingEvent[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvent[Accelerometer].sensor = ID_A;
    mPendingEvent[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent[Accelerometer].acceleration.status =
					SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvent[DisplayRotate].version = sizeof(sensors_event_t);
    mPendingEvent[DisplayRotate].sensor = ID_DR;
    mPendingEvent[DisplayRotate].type = SENSOR_TYPE_DISPLAY_ROTATE;
    mPendingEvent[DisplayRotate].acceleration.status =
					SENSOR_STATUS_ACCURACY_HIGH;

    mFlushEvents[Accelerometer].version = META_DATA_VERSION;
    mFlushEvents[Accelerometer].sensor = 0;
    mFlushEvents[Accelerometer].type = SENSOR_TYPE_META_DATA;
    mFlushEvents[Accelerometer].reserved0 = 0;
    mFlushEvents[Accelerometer].timestamp = 0;
    mFlushEvents[Accelerometer].meta_data.what = META_DATA_FLUSH_COMPLETE;
    mFlushEvents[Accelerometer].meta_data.sensor = ID_A;

    mFlushEvents[DisplayRotate].version = META_DATA_VERSION;
    mFlushEvents[DisplayRotate].sensor = 0;
    mFlushEvents[DisplayRotate].type = SENSOR_TYPE_META_DATA;
    mFlushEvents[DisplayRotate].reserved0 = 0;
    mFlushEvents[DisplayRotate].timestamp = 0;
    mFlushEvents[DisplayRotate].meta_data.what = META_DATA_FLUSH_COMPLETE;
    mFlushEvents[DisplayRotate].meta_data.sensor = ID_DR;

    open_device();

    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;
    int flags = 0;
    if (!ioctl(dev_fd, LIS3DH_IOCTL_GET_ENABLE, &flags)) {
        if (flags)  {
            mEnabled = 1;
            if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
                mPendingEvent[Accelerometer].acceleration.y =
						absinfo.value * CONVERT_A_X;
            }
            if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
                mPendingEvent[Accelerometer].acceleration.x =
						absinfo.value * CONVERT_A_Y;
            }
            if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
                mPendingEvent[Accelerometer].acceleration.z =
						absinfo.value * CONVERT_A_Z;
            }
            if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_DISPLAY_ROTATE),
								&absinfo)) {
                mPendingEvent[DisplayRotate].data[0] = absinfo.value;
            }
        }
    }
    if (!mEnabled) {
        close_device();
    }
}

AccelerationSensor::~AccelerationSensor() {
}

int AccelerationSensor::setEnable(int32_t handle, int en)
{
    int newEnabled;
    int enable = en ? 1 : 0;
    int newState = 0;
    int err = 0;

    newEnabled = mEnabled;

    switch (handle) {
        case ID_A:
            newEnabled &= (BIT_O | BIT_DR);
            newEnabled |= enable << Accelerometer;
            break;
        case ID_O:
            newEnabled &= (BIT_A | BIT_DR);
            newEnabled |= enable << Orientation;
            break;
        case ID_DR:
            newEnabled &= (BIT_A | BIT_O);
            newEnabled |= enable << DisplayRotate;
            break;
        default:
            return 0;
    }

    if ((newEnabled & BIT_A) || (newEnabled & BIT_O))
        newState |= BIT_A;
    if (newEnabled & BIT_DR)
        newState |= BIT_DR;
    ALOGD("AccelerationSensor: Set Sensors state 0x%x",newState);

    if ((mState == 0) && (newState != 0))
        open_device();

    if (mState != newState)
    {
        err = ioctl(dev_fd, LIS3DH_IOCTL_SET_ENABLE, &newState);
        err = err<0 ? -errno : 0;
        ALOGE_IF(err, "LIS3DH_IOCTL_SET_ENABLE failed (%s)", strerror(-err));
    }

    if ((mState != 0) && (newState == 0))
        close_device();

    mState = newState;
    mEnabled = newEnabled;

    if (!err)
        selectDelay(handle);

    return err;
}

int AccelerationSensor::getEnable(int32_t handle)
{
    int id=0;
    switch (handle) {
        case ID_A:  id = BIT_A;   break;
        case ID_O:  id = BIT_O;   break;
        case ID_DR: id = BIT_DR;  break;
    }
    return mEnabled & id ? 1 : 0;
}

int AccelerationSensor::setDelay(int32_t handle, int64_t ns)
{
    int delay = ns / 1000000;

    if (ns < 0)
        return -EINVAL;

    switch (handle) {
        case ID_A:
            mAccelDelay = delay;
            break;
        case ID_O:
            mOrientDelay = delay;
            break;
    }

    return selectDelay(handle);
}

int64_t AccelerationSensor::getDelay(int32_t handle)
{
        return (handle == ID_A) ? mDelay : 0;
}

int AccelerationSensor::selectDelay(int32_t handle)
{
    int err = 0;

    switch (mEnabled) {
        case 0:
            return 0;
        case BIT_DR:
            mDelay = 0;
            break;
        case BIT_A:
        case BIT_A | BIT_DR:
            mDelay = mAccelDelay;
            break;
        case BIT_O:
        case BIT_O | BIT_DR:
            mDelay = mOrientDelay;
            break;
        case BIT_A | BIT_O:
        case BIT_A | BIT_O | BIT_DR:
            if (mAccelDelay < mOrientDelay)
                mDelay = mAccelDelay;
            else
                mDelay = mOrientDelay;
            break;
    }

    ALOGD("AccelerationSensor: set delay %d", (int)mDelay);
    err = ioctl(dev_fd, LIS3DH_IOCTL_SET_DELAY, &mDelay);
    err = err<0 ? -errno : 0;
    ALOGE_IF(err, "LIS3DH_IOCTL_SET_DELAY failed (%s)", strerror(-err));

    return err;
}

int AccelerationSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    int numEventReceived = 0;
    input_event const* event;

    if(count && mFlushEnabled & BIT_A) {
        mFlushEnabled &= ~(BIT_A);
        *data++ = mFlushEvents[Accelerometer];
        count--;
        numEventReceived++;
    }

    if(count && mFlushEnabled & BIT_DR) {
        mFlushEnabled &= ~(BIT_DR);
        *data++ = mFlushEvents[DisplayRotate];
        count--;
        numEventReceived++;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS || type == EV_MSC) {
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
            if(is_gyro_calibrated == false) {
                Process_MeaningfulMovement((double)mPendingEvent[Accelerometer].acceleration.x,
                    (double)mPendingEvent[Accelerometer].acceleration.y,
                    (double)mPendingEvent[Accelerometer].acceleration.z);
            }
#endif
            for (int j=0 ; count && mPendingMask && j<(numSensors-1) ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvent[j].timestamp = time;
                    if (mState & (1<<j)) {
                        *data++ = mPendingEvent[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            ALOGE("AccelerationSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }

    return numEventReceived;
}

void AccelerationSensor::client_mm(double nowStdDev)
{
	mCurrentDataNumber++;

    if( nowStdDev < MM_ALLOWED_STD_DEV )
    {
        mLastLowNumber = mCurrentDataNumber;
    }
    else
    {
        mLastHighNumber = mCurrentDataNumber;
    }
    if((mCurrentDataNumber - mLastLowNumber) >= 1 )
    {
        is_device_moving = true;
    }
    else if( (mCurrentDataNumber - mLastHighNumber) >= 5 )
    {
        is_device_moving = false;
    }
}

void AccelerationSensor::Process_MeaningfulMovement(double acc_x, double acc_y, double acc_z)
{
    static unsigned int cnt = 0;
    static double mean_x = 0, mean_y = 0, mean_z = 0;
    static double s_x = 0, s_y = 0, s_z = 0;
    double delta = 0, std_dev_x = 0, std_dev_y = 0, std_dev_xyz = 0;

    cnt++;

    delta = acc_x - mean_x;
    mean_x += delta / cnt;
    s_x += delta * (acc_x - mean_x);

    delta = acc_y - mean_y;
    mean_y += delta / cnt;
    s_y += delta * (acc_y - mean_y);

    delta = acc_z - mean_z;
    mean_z += delta / cnt;
    s_z += delta * (acc_z - mean_z);

    //Caution, arrayloc could go higher than move_time when sample rate is being changed to a lower rate
    //move_time can NEVER be less than 2!
    if (cnt >= mm_mmoveme_time) {
       std_dev_x =  s_x / (cnt-1);
       std_dev_y =  s_y / (cnt-1);
       // sum std deviations, z contribution is capped at max of x or y contribution to suppress audio effect
       std_dev_xyz = fmin(s_z / (cnt-1), fmax(std_dev_x, std_dev_y)) + std_dev_x + std_dev_y;

       client_mm(std_dev_xyz);

	   cnt = 0;
	   mean_x = 0;
	   mean_y = 0;
	   mean_z = 0;
	   s_x = 0;
	   s_y = 0;
	   s_z = 0;
	}
}

void AccelerationSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_ACCEL_X:
            mPendingMask |= BIT_A;
            mPendingEvent[Accelerometer].acceleration.y = value * CONVERT_A_X;
            break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingMask |= BIT_A;
            mPendingEvent[Accelerometer].acceleration.x = value * CONVERT_A_Y;
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingMask |= BIT_A;
            mPendingEvent[Accelerometer].acceleration.z = value * CONVERT_A_Z;
            break;
        case EVENT_TYPE_DISPLAY_ROTATE:
            mPendingMask |= BIT_DR;
            mPendingEvent[DisplayRotate].data[0] = value;
            break;
    }
}

int AccelerationSensor::flush(int32_t handle)
{
    int id=0;
    switch (handle) {
        case ID_A:  id = BIT_A;   break;
        case ID_DR: id = BIT_DR;  break;
    }
    mFlushEnabled |= id;
    return 0;
}
