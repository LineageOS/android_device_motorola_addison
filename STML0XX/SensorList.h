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

/*
 * Copyright (C) 2015 Motorola Mobility LLC
 */

#include <hardware/sensors.h>
#include <hardware/mot_sensorhub_stml0xx.h>

#include "Sensors.h"

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/* Accel/Gyro conditional compilation choices */
#ifdef _ENABLE_BMI160
#define VENDOR_ACCEL  "Bosch"
#define VENDOR_GYRO   "Bosch"
/* Current draw figures in mA */
#define ACCEL_MA      0.55f
#define GYRO_MA       0.90f
#endif

#if _ENABLE_KXCJ9
#define VENDOR_ACCEL  "Kionix"
/* Current draw figures in mA */
#define ACCEL_MA      0.12f
#endif

/* Vendor names */
#define VENDOR_MAG     "Asahi Kasei"
#define VENDOR_MOT     "Motorola"
#define VENDOR_PROXALS "TAOS"

/* Range settings */
#define ACCEL_FULLSCALE_G  RANGE_G
#define GYRO_FULLSCALE_DPS (2000.f)
#define MAG_FULLSCALE_UT   (4912.f)
#define ALS_FULLSCALE_LUX  ((float)UINT16_MAX)

/* Resolution settings */
#define GYRO_QUANTIZATION_LEVELS  (INT16_MAX)
#define ALS_QUANTIZATION_LEVELS   (UINT16_MAX)

/* Min delays */
#define ACCEL_MIN_DELAY_US 10000
#define GYRO_MIN_DELAY_US  5000
#define MAG_MIN_DELAY_US   20000

/* Max delays */
#define ACCEL_MAX_DELAY_US 200000
#define GYRO_MAX_DELAY_US  200000
#define MAG_MAX_DELAY_US   1000000

/* Various current draw figures in mA */
#define MAG_MA    0.10f
#define ALS_MA    0.25f
#define PROX_MA   0.0467f /* 100mA (LED drive) * (7us/pulse * 4 pulses)/60ms */

/* Estimated algorithm current draw in mA*/
#define ORIENT_ALGO_MA    1.0f
#define CAM_ACT_ALGO_MA   1.0f
#define CHOP_CHOP_ALGO_MA 1.0f
#define LIFT_ALGO_MA      1.0f

static const struct sensor_t sSensorList[] = {
	{ .name = "3-axis Accelerometer",
		.vendor = VENDOR_ACCEL,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_A,
		.type = SENSOR_TYPE_ACCELEROMETER,
		.maxRange = ACCEL_FULLSCALE_G * GRAVITY_EARTH,
		.resolution = GRAVITY_EARTH / LSG,
		.power = ACCEL_MA,
		.minDelay = ACCEL_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = ACCEL_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
#ifdef _ENABLE_BMI160
	{ .name = "3-axis Gyroscope",
		.vendor = VENDOR_GYRO,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_G,
		.type = SENSOR_TYPE_GYROSCOPE,
		.maxRange = GYRO_FULLSCALE_DPS,
		.resolution = GYRO_FULLSCALE_DPS / GYRO_QUANTIZATION_LEVELS,
		.power = GYRO_MA,
		.minDelay = GYRO_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = GYRO_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
	{ .name = "3-axis Uncalibrated Gyroscope",
		.vendor = VENDOR_GYRO,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_UNCALIB_GYRO,
		.type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
		.maxRange = GYRO_FULLSCALE_DPS,
		.resolution = GYRO_FULLSCALE_DPS / GYRO_QUANTIZATION_LEVELS,
		.power = GYRO_MA,
		.minDelay = GYRO_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = GYRO_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
#endif
#ifdef _ENABLE_MAGNETOMETER
	{ .name = "3-axis Calibrated Magnetic field sensor",
		.vendor = VENDOR_MAG,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_M,
		.type = SENSOR_TYPE_MAGNETIC_FIELD,
		.maxRange = MAG_FULLSCALE_UT,
		.resolution = CONVERT_M,
		.power = MAG_MA,
		.minDelay = MAG_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = MAG_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
	{ .name = "3-axis Uncalibrated Magnetic field sensor",
		.vendor = VENDOR_MAG,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_UM,
		.type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
		.maxRange = MAG_FULLSCALE_UT,
		.resolution = CONVERT_M,
		.power = MAG_MA
		.minDelay = MAG_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = MAG_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
	{ .name = "Orientation sensor",
		.vendor = VENDOR_MAG,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_OR,
		.type = SENSOR_TYPE_ORIENTATION,
		.maxRange = 360.0f,
		.resolution = CONVERT_OR,
		.power = MAG_MA + ACCEL_MA + ORIENT_ALGO_MA,
		.minDelay = MAG_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = MAG_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
#endif /* _ENABLE_MAGNETOMETER */
	{ .name = "Ambient Light sensor",
		.vendor = VENDOR_PROXALS,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_L,
		.type = SENSOR_TYPE_LIGHT,
		.maxRange = ALS_FULLSCALE_LUX,
		.resolution = ALS_FULLSCALE_LUX / ALS_QUANTIZATION_LEVELS,
		.power = ALS_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_ON_CHANGE_MODE,
		.reserved = {0,0} },
	{ .name = "Proximity sensor",
		.vendor = VENDOR_PROXALS,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_P,
		.type = SENSOR_TYPE_PROXIMITY,
		.maxRange = 100.0f,
		.resolution = 100.0f,
		.power = PROX_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
	{ .name = "Display Rotation sensor",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_DR,
		.type = SENSOR_TYPE_DISPLAY_ROTATE,
		.maxRange = 4.0f,
		.resolution = 1.0f,
		.power = ACCEL_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.display_rotate",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_ON_CHANGE_MODE,
		.reserved = {0,0} },
	{ .name = "Flat Up",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_FU,
		.type = SENSOR_TYPE_FLAT_UP,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = ACCEL_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.flat_up",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
	{ .name = "Flat Down",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_FD,
		.type = SENSOR_TYPE_FLAT_DOWN,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = ACCEL_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.flat_down",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
	{ .name = "Stowed",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_S,
		.type = SENSOR_TYPE_STOWED,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = PROX_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.stowed",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
	{ .name = "Camera Activation sensor",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_CA,
		.type = SENSOR_TYPE_CAMERA_ACTIVATE,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = CAM_ACT_ALGO_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.camera_activate",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
#ifdef _ENABLE_CHOPCHOP
	{ .name = "ChopChop Gesture",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_CC,
		.type = SENSOR_TYPE_CHOPCHOP_GESTURE,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = CHOP_CHOP_ALGO_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.chopchop",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
#endif
#ifdef _ENABLE_LIFT
	{ .name = "Lift Gesture Virtual Sensor",
		.vendor = VENDOR_MOT,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_LF,
		.type = SENSOR_TYPE_LIFT_GESTURE,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = LIFT_ALGO_MA,
		.minDelay = 0,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "com.motorola.sensor.lift",
		.requiredPermission = "",
		.maxDelay = 0,
		.flags = SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP,
		.reserved = {0,0} },
#endif
#ifdef _ENABLE_ACCEL_SECONDARY
	{ .name = "3-axis Accelerometer, Secondary",
		.vendor = VENDOR_ACCEL,
		.version = 1,
		.handle = SENSORS_HANDLE_BASE + ID_A2,
		.type = SENSOR_TYPE_ACCELEROMETER,
		.maxRange = ACCEL_FULLSCALE_G * GRAVITY_EARTH,
		.resolution = GRAVITY_EARTH / LSG,
		.power = ACCEL_MA,
		.minDelay = ACCEL_MIN_DELAY_US,
		.fifoReservedEventCount = 0,
		.fifoMaxEventCount = 0,
		.stringType = "",
		.requiredPermission = "",
		.maxDelay = ACCEL_MAX_DELAY_US,
		.flags = SENSOR_FLAG_CONTINUOUS_MODE,
		.reserved = {0,0} },
#endif
};
