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

#include <float.h>
#include <math.h>
#include <string.h>
#include <cutils/log.h>
#include "LinearAccelGravity.h"
#include "Quaternion.h"
#include "SensorList.h"

LinearAccelGravity LinearAccelGravity::self;

LinearAccelGravity::LinearAccelGravity()
: gameRVData(), gameRVts(0)
{
}

LinearAccelGravity::~LinearAccelGravity()
{
}

LinearAccelGravity *LinearAccelGravity::getInstance()
{
    return &self;
}

void LinearAccelGravity::setGameRVData(float x, float y, float z, float ext1, int64_t ts)
{
    gameRVData[0] = x;
    gameRVData[1] = y;
    gameRVData[2] = z;
    gameRVData[3] = ext1;
    gameRVts = ts;
}

void LinearAccelGravity::processFusion()
{
    /*
     * In the android-defined coordinate system (x east, y north, z up),
     * gravity is along the z axis.
     *
     * So, we need to convert the z vector (0,0,1) to a quaternion, then
     * rotate by the 9-axis vector to get the current gravity. To do this,
     * first embed the z vector in a quaternion:
     *
     *     qz := 0*i + 0*j + 1*k + 0.
     *
     * Then, supposing q is the current 9-axis rv:
     *
     *     qGrav = q^(-1)*qz*q,
     *
     * where q^(-1) is the inverse of q. Then, we extract the resulting vector
     * (gx, gy, gz) from qGrav:
     *
     *     qGrav = gx*i + gy*j + gz*k + 0.
     */

    float qz[4] = {0.f, 0.f, 1.f, 0.f};
    float qGrav[4] = {0.f, 0.f, 0.f, 1.f};
    float mag = 0.f;

    /* qGrav = gameRV^{-1} * qz * gameRV */
    Quaternion::quatInv(qGrav, gameRVData);
    Quaternion::quatMul(qz, qz, gameRVData);
    Quaternion::quatMul_noRenormalize(qGrav, qGrav, qz);

    /* We explicitly requested the last quatMul not to renormalize. The reason
     * is that theoretically qGrav[3] == 0, but due to numerical issues, it will be
     * some very small positive or NEGATIVE number, which can cause the gravity
     * vector to flip signs. Handle the normalization explicitly here without
     * sign changes.
     */
    mag = sqrtf(qGrav[0]*qGrav[0] + qGrav[1]*qGrav[1] + qGrav[2]*qGrav[2] + qGrav[3]*qGrav[3]);
    qGrav[0] = (qGrav[0] / mag) * GRAVITY_EARTH;
    qGrav[1] = (qGrav[1] / mag) * GRAVITY_EARTH;
    qGrav[2] = (qGrav[2] / mag) * GRAVITY_EARTH;

    /* Fill Linear Acceleration */
    fusionData[0] = accelData[0] - qGrav[0];
    fusionData[1] = accelData[1] - qGrav[1];
    fusionData[2] = accelData[2] - qGrav[2];

    /* Fill Gravity */
    memcpy(&fusionData[3], qGrav, 3*sizeof(float));

    fusionTs = gameRVts;
}
