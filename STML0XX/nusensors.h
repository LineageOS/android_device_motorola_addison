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

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device);

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define MIN_SENSOR_ID (0)
#define ID_A  (0)  /* Accelerometer */
#define ID_L  (1)  /* Light */
#define ID_DR (2)  /* Display Rotate */
#define ID_P  (3)  /* Proximity */
#define ID_FU (4)  /* Flat Up */
#define ID_FD (5)  /* Flat Down */
#define ID_S  (6)  /* Stowed */
#define ID_CA (7)  /* Camera Activate */
#define ID_SIM (8) /* Significant motion */
#define ID_A2 (9) /* Secondary Accel */
#define MAX_SENSOR_ID (9)
/*****************************************************************************/

/*
 * The SENSORS Module
 */

/*****************************************************************************/

// KXTJ2 configured to +/-8G 14-bit mode
// 14-bit Register Data range -8192 ~ +8191
// 1024 LSG = 1G
#define RANGE_G                     (8.0f)
#define LSG                         (1024.0f)

// conversion of acceleration data to SI units (m/s^2)
#define CONVERT_A                   (GRAVITY_EARTH / LSG)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// proximity uncovered and covered values
#define PROX_UNCOVERED              (100.0f)
#define PROX_COVERED                (3.0f)
#define PROX_SATURATED              (1.0f)

// flat up / down values
#define FLAT_NOTDETECTED            (0.0f)
#define FLAT_DETECTED               (1.0f)

// Display rotate values
#define DISP_FLAT                   0x10
#define DISP_UNKNOWN                (-1.0f)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
