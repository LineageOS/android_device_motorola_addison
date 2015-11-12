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

#include "FusionSensorBase.h"

FusionSensorBase::FusionSensorBase()
: accelData(), gyroData(), magData(), fusionData(),
  accelTs(0), gyroTs(0), magTs(0), fusionTs(0)
{
}

void FusionSensorBase::setAccelData(float x, float y, float z, int64_t ts)
{
    accelData[0] = x;
    accelData[1] = y;
    accelData[2] = z;
    accelTs = ts;
}

void FusionSensorBase::setGyroData(float x, float y, float z, int64_t ts)
{
    gyroData[0] = x;
    gyroData[1] = y;
    gyroData[2] = z;
    gyroTs = ts;
}

void FusionSensorBase::setMagData(float x, float y, float z, int64_t ts)
{
    magData[0] = x;
    magData[1] = y;
    magData[2] = z;
    magTs = ts;
}
