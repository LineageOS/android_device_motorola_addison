/*
 * Copyright (C) 2009-2012 Motorola, Inc.
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
#include <hardware/mot_sensorhub_motosh.h>
#include <float.h>
#include <limits.h>

#include "nusensors.h"

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/* Vendor names */
#define VENDOR_AK   "Asahi Kasei"
#define VENDOR_INVN "InvenSense"
#define VENDOR_MOT  "Motorola"
#define VENDOR_TAOS "TAOS"

/* Part numbers to use in sensor names */
#define ACCEL_PART_NO "ICM20645"
#define GYRO_PART_NO  "ICM20645"
#define MAG_PART_NO   "AKM09912"
#define ALS_PART_NO   "CT406"
#define PROX_PART_NO  "CT406"

/* Various current draw figures from spec sheets in mA */
#define ICM20645_6AXIS_MA          3.4f
#define ICM20645_GYRO_MA           3.2f
#define ICM20645_ACCEL_MA          0.45f
#define ICM20645_ACCEL_LOWPOWER_MA 0.019f
#define AK09912_MA                 1.0f
#define CT406_ALS_MA               0.25f
#define CT406_PROX_MA              3.0f

/* Estimated algorithm current draw in mA*/
#define ORIENT_ALGO_MA  1.0f
#define CAM_ACT_ALGO_MA 1.0f
#define MOT_9AXIS_MA    1.0f
#define MOT_6AXIS_MA    1.0f

static const struct sensor_t sSensorList[] = {
    { .name = ACCEL_PART_NO " 3-axis Accelerometer",
                .vendor = VENDOR_INVN,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_A,
                .type = SENSOR_TYPE_ACCELEROMETER,
                .maxRange = 16.0f*9.81f,
                .resolution = 9.81f/2048.0f,
                .power = ICM20645_ACCEL_MA,
                .minDelay = 5000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = GYRO_PART_NO " Gyroscope",
                .vendor = VENDOR_INVN,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_G,
                .type = SENSOR_TYPE_GYROSCOPE,
                .maxRange = 2000.0f,
                .resolution = 1.0f,
                .power = ICM20645_GYRO_MA,
                .minDelay = 5000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = MAG_PART_NO " 3-axis Magnetometer",
                .vendor = VENDOR_AK,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_M,
                .type = SENSOR_TYPE_MAGNETIC_FIELD,
                .maxRange = 2000.0f,
                .resolution = 1.0f/10.0f,
                .power = AK09912_MA,
                .minDelay = 20000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = MAG_PART_NO " Orientation",
                .vendor = VENDOR_AK,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_O,
                .type = SENSOR_TYPE_ORIENTATION,
                .maxRange = 360.0f,
                .resolution = 1.0f/64.0f,
                .power = AK09912_MA + ICM20645_ACCEL_MA + ORIENT_ALGO_MA,
                .minDelay = 20000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = ALS_PART_NO " Ambient Light",
                .vendor = VENDOR_TAOS,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_L,
                .type = SENSOR_TYPE_LIGHT,
                .maxRange = 27000.0f,
                .resolution = 1.0f,
                .power = CT406_ALS_MA,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE,
                .reserved = {0,0} },
    { .name = "Display Rotation",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_DR,
                .type = SENSOR_TYPE_DISPLAY_ROTATE,
                .maxRange = 4.0f,
                .resolution = 1.0f,
                .power = ICM20645_ACCEL_MA,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "com.motorola.sensor.display_rotate",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE,
                .reserved = {0,0} },
    { .name = PROX_PART_NO " Proximity",
                .vendor = VENDOR_TAOS,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_P,
                .type = SENSOR_TYPE_PROXIMITY,
                .maxRange = 100.0f,
                .resolution = 100.0f,
                .power = CT406_PROX_MA,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0}},
    { .name = "Flat Up",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_FU,
                .type = SENSOR_TYPE_FLAT_UP,
                .maxRange = 1.0f,
                .resolution = 1.0f,
                .power = ICM20645_ACCEL_MA,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "com.motorola.sensor.flat_up",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0}},
    { .name = "Flat Down",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_FD,
                .type = SENSOR_TYPE_FLAT_DOWN,
                .maxRange = 1.0f,
                .resolution = 1.0f,
                .power = ICM20645_ACCEL_MA,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "com.motorola.sensor.flat_down",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0}},
    { .name = "Stowed",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_S,
                .type = SENSOR_TYPE_STOWED,
                .maxRange = 1.0f,
                .resolution = 1.0f,
                .power = CT406_ALS_MA + CT406_PROX_MA,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "com.motorola.sensor.stowed",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0}},
    { .name = "Camera Activation",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_CA,
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
                .reserved = {0,0}},
    { .name = "IR Gestures",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_IR_GESTURE,
                .type = SENSOR_TYPE_IR_GESTURE,
                .maxRange = 1.0f,
                .resolution = 1.0f,
                .power = 1.0f,
                .minDelay = 0,
                .fifoReservedEventCount = 8,
                .fifoMaxEventCount = 8,
                .stringType = "com.motorola.sensor.ir_gesture",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0}},
    { .name = "IR Raw Data",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_IR_RAW,
                .type = SENSOR_TYPE_IR_RAW,
                .maxRange = 4096.0f,
                .resolution = 1.0f,
                .power = 1.0f,
                .minDelay = 10000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "com.motorola.sensor.ir_raw",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = "IR Object Detect",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_IR_OBJECT,
                .type = SENSOR_TYPE_IR_OBJECT,
                .maxRange = 1.0f,
                .resolution = 1.0f,
                .power = 1.0f,
                .minDelay = -1,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "com.motorola.sensor.ir_object",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ONE_SHOT_MODE,
                .reserved = {0,0} },
    { .name = "Significant Motion",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_SIM,
                .type = SENSOR_TYPE_SIGNIFICANT_MOTION,
                .maxRange = 1.0f,
                .resolution = 1.0f,
                .power = 3.0f,
                .minDelay = -1,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ONE_SHOT_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0}},
#ifdef _ENABLE_PEDO
    { .name = "Step Detector sensor",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_STEP_DETECTOR,
                .type = SENSOR_TYPE_STEP_DETECTOR,
                .maxRange = 1.0f,
                .resolution = 0,
                .power = 0,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE,
                .reserved = {0,0} },

    { .name = "Step Counter sensor",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_STEP_COUNTER,
                .type = SENSOR_TYPE_STEP_COUNTER,
                .maxRange = FLT_MAX,
                .resolution = 0,
                .power = 0,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_SPECIAL_REPORTING_MODE,
                .reserved = {0,0} },
#endif
    { .name = GYRO_PART_NO " Uncalibrated Gyroscope",
                .vendor = VENDOR_INVN,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_UNCALIB_GYRO,
                .type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
                .maxRange = 2000.0f,
                .resolution = 1.0f,
                .power = ICM20645_GYRO_MA,
                .minDelay = 20000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = MAG_PART_NO " 3-axis Uncalibrated Magnetometer",
                .vendor = VENDOR_AK,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_UNCALIB_MAG,
                .type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
                .maxRange = 2000.0f,
                .resolution = 1.0f/10.0f,
                .power = AK09912_MA,
                .minDelay = 20000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 200000,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
#ifdef _ENABLE_CHOPCHOP
    { .name = "ChopChop Gesture",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_CHOPCHOP_GESTURE,
                .type = SENSOR_TYPE_CHOPCHOP_GESTURE,
                .maxRange = USHRT_MAX*1.0f,
                .resolution = 1.0f,
                .power = 0.0f,
                .minDelay = 0,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP,
                .reserved = {0,0} },
#endif
    { .name = "Rotation Vector",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_QUAT_9AXIS,
                .type = SENSOR_TYPE_ROTATION_VECTOR,
                .maxRange = 1.0f,
                .resolution = 1.0f/32767.f,
                .power = ICM20645_6AXIS_MA + AK09912_MA + MOT_9AXIS_MA,
                .minDelay = 10000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
    { .name = "Geomagnetic Rotation Vector",
                .vendor = VENDOR_MOT,
                .version = 1,
                .handle = SENSORS_HANDLE_BASE+ID_QUAT_6AXIS,
                .type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR,
                .maxRange = 1.0f,
                .resolution = 1.0f/32767.f,
                .power = ICM20645_6AXIS_MA + MOT_6AXIS_MA,
                .minDelay = 10000,
                .fifoReservedEventCount = 0,
                .fifoMaxEventCount = 0,
                .stringType = "",
                .requiredPermission = "",
                .maxDelay = 0,
                .flags = SENSOR_FLAG_CONTINUOUS_MODE,
                .reserved = {0,0} },
};

/* Clean up definitions */
#undef VENDOR_AK
#undef VENDOR_INVN
#undef VENDOR_MOT
#undef VENDOR_TAOS

#undef ACCEL_PART_NO
#undef GYRO_PART_NO
#undef MAG_PART_NO
#undef ALS_PART_NO

#undef ICM20645_6AXIS_MA
#undef ICM20645_GYRO_MA
#undef ICM20645_ACCEL_MA
#undef ICM20645_ACCEL_LOWPOWER_MA
#undef AK09912_MA
#undef CT406_ALS_MA
#undef CT406_PROX_MA

#undef ORIENT_ALGO_MA
#undef CAM_ACT_ALGO_MA
#undef MOT_9AXIS_MA

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    (void)module;
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 2,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Motorola Sensors Module",
        .author = "Motorola",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = sensors__get_sensors_list
};

/*****************************************************************************/

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    (void)name;
    return init_nusensors(module, device);
}
