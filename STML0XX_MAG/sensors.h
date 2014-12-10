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

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>
#include <linux/limits.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <hardware/mot_sensorhub_stml0xx.h>

__BEGIN_DECLS

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MIN_SENSOR_ID (0 + SENSORS_HANDLE_BASE)
#define ID_A  (0 + SENSORS_HANDLE_BASE)  /* Accelerometer */
#define ID_M  (1 + SENSORS_HANDLE_BASE)  /* Magnetometer */
#define ID_L  (2 + SENSORS_HANDLE_BASE)  /* Light */
#define ID_P  (3 + SENSORS_HANDLE_BASE)  /* Proximity */
#define ID_DR (4 + SENSORS_HANDLE_BASE)  /* Display Rotate */
#define ID_FU (5 + SENSORS_HANDLE_BASE)  /* Flat Up */
#define ID_FD (6 + SENSORS_HANDLE_BASE)  /* Flat Down */
#define ID_S  (7 + SENSORS_HANDLE_BASE)  /* Stowed */
#define ID_CA (8 + SENSORS_HANDLE_BASE)  /* Camera Activate */
#define ID_A2 (9 + SENSORS_HANDLE_BASE)  /* Secondary Accel */
#define MAX_SENSOR_ID (9 + SENSORS_HANDLE_BASE)

/*****************************************************************************/

/* KXTJ2 configured to +/-8G 14-bit mode
 * 14-bit Register Data range -8192 ~ +8191
 * 1024 LSG = 1G
*/
#define RANGE_G                     (8.0f)
#define LSG                         (1024.0f)

/* Conversion of acceleration data to SI units (m/s^2) */
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

/* Magnetometer event types */
#define EVENT_TYPE_MAGV_X           ABS_RX
#define EVENT_TYPE_MAGV_Y           ABS_RY
#define EVENT_TYPE_MAGV_Z           ABS_RZ
#define EVENT_TYPE_MAGV_STATUS      ABS_RUDDER

/* Conversion of magnetic data to uT units */
#define CONVERT_M                   (0.06f)

/* Proximity uncovered and covered values */
#define PROX_UNCOVERED              (100.0f)
#define PROX_COVERED                (3.0f)
#define PROX_SATURATED              (1.0f)

/* Display rotate values */
#define DISP_FLAT                   (0x10)
#define DISP_UNKNOWN                (-1.0f)

/* Flat up / down values */
#define FLAT_NOTDETECTED            (0.0f)
#define FLAT_DETECTED               (1.0f)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
