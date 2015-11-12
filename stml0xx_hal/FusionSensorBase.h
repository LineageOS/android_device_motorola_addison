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

#ifndef FUSION_SENSOR_BASE_H
#define FUSION_SENSOR_BASE_H

#include <stdint.h>

class FusionSensorBase {
public:
    FusionSensorBase();

    virtual ~FusionSensorBase() {};

    // Set raw sensor input data
    void setAccelData(float x, float y, float z, int64_t ts);
    void setGyroData(float x, float y, float z, int64_t ts);
    void setMagData(float x, float y, float z, int64_t ts);

    // Get output data
    float getFusionX() { return fusionData[0]; }
    float getFusionY() { return fusionData[1]; }
    float getFusionZ() { return fusionData[2]; }
    float getFusionExt1() { return fusionData[3]; }
    float getFusionExt2() { return fusionData[4]; }
    float getFusionExt3() { return fusionData[5]; }
    int64_t getFusionTs() { return fusionTs; }

    // Process fusion using current sensor data
    virtual void processFusion() = 0;

protected:
    // Sensor data
    float accelData[3];
    float gyroData[3];
    float magData[3];
    float fusionData[6];

    // Timestamps in nanoseconds
    int64_t accelTs;
    int64_t gyroTs;
    int64_t magTs;
    int64_t fusionTs;
};

#endif // FUSION_SENSOR_BASE_H