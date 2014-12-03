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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <hardware/mot_sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device);

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0 + SENSORS_HANDLE_BASE)
#define ID_M  (1 + SENSORS_HANDLE_BASE)
#define ID_O  (2 + SENSORS_HANDLE_BASE)
#define ID_T  (3 + SENSORS_HANDLE_BASE)
#define ID_P  (4 + SENSORS_HANDLE_BASE)
#define ID_L  (5 + SENSORS_HANDLE_BASE)
#define ID_B  (6 + SENSORS_HANDLE_BASE)
#define ID_G  (7 + SENSORS_HANDLE_BASE)
#define ID_IP (8 + SENSORS_HANDLE_BASE)
#define ID_DR (9 + SENSORS_HANDLE_BASE) /* Display Rotate */

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/*****************************************************************************/

#define ACCELEROMETER_DEVICE_NAME   "/dev/lis3dh"
#define LIGHTPROXIMITY_DEVICE_NAME  "/dev/ct406"

#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
#define GYROSCOPE_DEVICE_NAME       "/dev/l3g4200d"
#endif

#define EVENT_TYPE_ACCEL_X          ABS_X
#define EVENT_TYPE_ACCEL_Y          ABS_Y
#define EVENT_TYPE_ACCEL_Z          ABS_Z
#define EVENT_TYPE_ACCEL_STATUS     ABS_WHEEL

#define EVENT_TYPE_MAGV_X           ABS_RX
#define EVENT_TYPE_MAGV_Y           ABS_RY
#define EVENT_TYPE_MAGV_Z           ABS_RZ
#define EVENT_TYPE_MAGV_STATUS      ABS_RUDDER

#define EVENT_TYPE_YAW              ABS_HAT0X
#define EVENT_TYPE_PITCH            ABS_HAT0Y
#define EVENT_TYPE_ROLL             ABS_HAT1X
#define EVENT_TYPE_ORIENT_STATUS    ABS_HAT1Y

#define EVENT_TYPE_TEMPERATURE      ABS_THROTTLE
#define EVENT_TYPE_PROXIMITY        MSC_RAW
#define EVENT_TYPE_LIGHT            LED_MISC
#define EVENT_TYPE_INPOCKET         MSC_PULSELED
#define EVENT_TYPE_PRESSURE         ABS_PRESSURE

#define EVENT_TYPE_GYRO_P           REL_RX
#define EVENT_TYPE_GYRO_R           REL_RY
#define EVENT_TYPE_GYRO_Y           REL_RZ

#define EVENT_TYPE_DISPLAY_ROTATE   MSC_RAW

// 1000 LSG = 1G
#define LSG                         (1000.0f)
#define AKSC_LSG                    (720.0f)

// conversion of acceleration data to SI units (m/s^2)
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 -(CONVERT_A)
#define CONVERT_A_Y                 -(CONVERT_A)
#define CONVERT_A_Z                 -(CONVERT_A)

// conversion of magnetic data to uT units
#define CONVERT_M                   (1.0f/16.0f)

#define CONVERT_O                   (1.0f/64.0f)

#define CONVERT_P                   (1.0f/10.0f)

// Display rotate values
#define DISP_FLAT                   0x10
#define DISP_UNKNOWN                (-1.0f)

// conversion of angular velocity(millidegrees/second) to rad/s
#define MAX_RANGE_G                 (2000.0f * ((float)(M_PI/180.0f)))
#define CONVERT_G                   ((70.0f/1000.0f) * ((float)(M_PI/180.0f)))
#define CONVERT_G_P                 (-CONVERT_G)
#define CONVERT_G_R                 (-CONVERT_G)
#define CONVERT_G_Y                 (-CONVERT_G)

#define CONVERT_B                   (1.0f/100.0f)

#define SENSOR_STATE_MASK           (0x7FFF)

#define ONE_DAY                     24*60*60
/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
