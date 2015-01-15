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

#include <float.h>

#include <cutils/log.h>

#include <hardware/sensors.h>
#include <hardware/mot_sensorhub_stml0xx.h>

#include "sensors.h"

/*****************************************************************************/

/* The SENSORS Module */
static const struct sensor_t sSensorList[] = {
	{ "KXTJ2 3-axis Accelerometer",
		"Kionix",
		1,
		ID_A,
		SENSOR_TYPE_ACCELEROMETER,
		RANGE_G * GRAVITY_EARTH,
		GRAVITY_EARTH / LSG,
		0.25f,
		10000,
		0,
		0,
		"",
		"",
		200000,
		SENSOR_FLAG_CONTINUOUS_MODE,
		{} },
	{ "AK09912 3-axis Magnetic field sensor",
		"Asahi Kasei Microdevices",
		1,
		ID_M,
		SENSOR_TYPE_MAGNETIC_FIELD,
		4912.0f,
		CONVERT_M,
		0.10f,
		10000,
		0,
		0,
		"",
		"",
		200000,
		SENSOR_FLAG_CONTINUOUS_MODE,
		{} },
	{ "AKM Orientation sensor",
		"Asahi Kasei Microdevices",
		1,
		ID_OR,
		SENSOR_TYPE_ORIENTATION,
		360.0f,
		CONVERT_OR,
		0.425f,
		10000,
		0,
		0,
		"",
		"",
		200000,
		SENSOR_FLAG_CONTINUOUS_MODE,
		{} },
	{ "CT406 Light sensor",
		"TAOS",
		1,
		ID_L,
		SENSOR_TYPE_LIGHT,
		27000.0f,
		1.0f,
		0.175f,
		0,
		0,
		0,
		"",
                "",
		0,
		SENSOR_FLAG_ON_CHANGE_MODE,
		{} },
	{ "CT406 Proximity sensor",
		"TAOS",
		1,
		ID_P,
		SENSOR_TYPE_PROXIMITY,
		100.0f,
		100.0f,
		3.0f,
		0,
		0,
		0,
		"",
                "",
		0,
		SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		{} },
	{ "Display Rotation sensor",
		"Motorola",
		1,
		ID_DR,
		SENSOR_TYPE_DISPLAY_ROTATE,
		4.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		"com.motorola.sensor.display_rotate",
		"",
		0,
		SENSOR_FLAG_ON_CHANGE_MODE,
		{} },
	{ "Flat Up",
		"Motorola",
		1,
		ID_FU,
		SENSOR_TYPE_FLAT_UP,
		1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		"com.motorola.sensor.flat_up",
		"",
		0,
		SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		{} },
	{ "Flat Down",
		"Motorola",
		1,
		ID_FD,
		SENSOR_TYPE_FLAT_DOWN,
		1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		"com.motorola.sensor.flat_down",
		"",
		0,
		SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		{} },
	{ "Stowed",
		"Motorola",
		1,
		ID_S,
		SENSOR_TYPE_STOWED,
		1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		"com.motorola.sensor.stowed",
		"",
		0,
		SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
		{} },
	{ "Camera Activation sensor",
		"Motorola",
		1,
		ID_CA,
		SENSOR_TYPE_CAMERA_ACTIVATE,
		1.0f,
		1.0f,
		0.0f,
		0,
		0,
		0,
		"com.motorola.sensor.camera_activate",
		"",
		0,
		SENSOR_FLAG_SPECIAL_REPORTING_MODE | SENSOR_FLAG_WAKE_UP,
		{} },
#ifdef _ENABLE_ACCEL_SECONDARY
	{ "KXTJ2 3-axis Accelerometer, Secondary",
		"Kionix",
		1,
		ID_A2,
		SENSOR_TYPE_ACCELEROMETER,
		RANGE_G * GRAVITY_EARTH,
		GRAVITY_EARTH / LSG,
		0.25f,
		10000,
		0,
		0,
		"",
		"",
		200000,
		SENSOR_FLAG_CONTINUOUS_MODE,
		{} },
#endif
};


static int open_sensors(const struct hw_module_t* module, const char* id,
			struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
				     struct sensor_t const** list) 
{
	(void)module;
	*list = sSensorList;
	return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
	open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: SENSORS_HARDWARE_MODULE_ID,
		name: "Motorola Sensors Module",
		author: "Motorola",
		methods: &sensors_module_methods,
		dso: NULL,
		reserved: {0},
	},
	get_sensors_list: sensors__get_sensors_list
};

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
			struct hw_device_t** device)
{
	(void)id;
	return init_sensors(module, device);
}
