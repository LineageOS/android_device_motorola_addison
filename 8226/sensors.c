/*
 * Copyright (C) 2009-2010 Motorola, Inc.
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

#include <hardware/sensors.h>

#include "nusensors.h"

/*****************************************************************************/

/*
 * The SENSORS Module
 */

static const struct sensor_t sSensorList[] = {
    { "LIS3DH 3-axis Accelerometer",
                "ST Micro",
                1, ID_A,
                SENSOR_TYPE_ACCELEROMETER, 4.0f*9.81f, 9.81f/1000.0f, 0.25f, 10000, 0, 0, { 0 },
                "", 200000, SENSOR_FLAG_CONTINUOUS_MODE },
    { "AK8963 3-axis Magnetic field sensor",
                "Asahi Kasei",
                1, ID_M,
                SENSOR_TYPE_MAGNETIC_FIELD, 2000.0f, 1.0f/16.0f, 6.8f, 20000, 0, 0, { 0 },
                "", 200000, SENSOR_FLAG_CONTINUOUS_MODE },
    { "AK8963 Orientation sensor",
                "Asahi Kasei",
                1, ID_O,
                SENSOR_TYPE_ORIENTATION, 360.0f, 1.0f/64.0f, 7.05f, 10000, 0, 0, { 0 },
                "", 200000, SENSOR_FLAG_CONTINUOUS_MODE },
    { "CT406 Proximity sensor",
                "TAOS",
                1, ID_P,
                SENSOR_TYPE_PROXIMITY, 100.0f, 100.0f, 3.0f, 0, 0, 0, { 0 },
                "", 0, SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP },
    { "CT406 Light sensor",
                "TAOS",
                1, ID_L,
                SENSOR_TYPE_LIGHT, 27000.0f, 1.0f, 0.175f, 20000, 0, 0, { 0 },
                "", 0, SENSOR_FLAG_ON_CHANGE_MODE },
    { "Display Rotation sensor",
                "Motorola",
                1, ID_DR,
                SENSOR_TYPE_DISPLAY_ROTATE, 4.0f, 1.0f, 0.0f, 0, 0, 0, { 0 },
                "", 0, SENSOR_FLAG_ON_CHANGE_MODE },
#ifdef FEATURE_GYRO_L3D4200_SUPPORTED
    { "L3G4200D Gyroscope sensor",
                "ST Micro",
                1, ID_G,
                SENSOR_TYPE_GYROSCOPE, 2000.0f, 1.0f, 0.0f, 10000, 0, 0, { 0 },
                "", 200000, SENSOR_FLAG_CONTINUOUS_MODE },
#endif
};

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "SC1 SENSORS Module",
        .author = "Motorola",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = sensors__get_sensors_list
};

/*****************************************************************************/

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    return init_nusensors(module, device);
}
