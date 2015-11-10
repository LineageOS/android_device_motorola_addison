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
#include "GameRotationVector.h"
#include "Quaternion.h"
#include "SensorList.h"

/*!
 * \brief Normalized LP filter constant for the 6-axis RV
 *
 * Acceptable range for this parameter is (0,1).
 *
 * The cutoff frequency is
 * \f[
 *    f_c = \alpha \frac{f_s}{2},
 * \f]
 * where \f$ f_s \f$ is the sampling frequency (in Hz).
 *
 * This should be chosen to balance noise and responsiveness.
 */
#define SIX_AXIS_FILTER_ALPHA (0.016f)
#define NINE_AXIS_FILTER_ALPHA (0.002f)

GameRotationVector GameRotationVector::self;

GameRotationVector::GameRotationVector()
{
}

GameRotationVector::~GameRotationVector()
{
}

GameRotationVector *GameRotationVector::getInstance()
{
    return &self;
}

int GameRotationVector::permSort(size_t* p, float* a, size_t const len)
{
    int swaps = 0;
    float maxVal;
    size_t i, j, k;
    float _ftmp;
    size_t _itmp;
#define _flt_swap(a,b) \
    do { \
        _ftmp = a; \
        a = b; \
        b = _ftmp; \
    } while (0);
#define _szt_swap(a,b) \
    do { \
        _itmp = a; \
        a = b; \
        b = _itmp; \
    } while (0);

    // Initialize permutation to the identity p[i] = i
    for( i = 0; i < len; ++i )
        p[i] = i;
    // Selection sort, keeping track of permutation
    for(i = 0; i < len; ++i) {
        maxVal = -FLT_MAX;
        k = i;
        for(j = i; j < len; ++j) {
            if( fabs(a[j]) > maxVal )
            {
                k = j;
                maxVal = fabs(a[j]);
            }
        }
        _flt_swap(a[i], a[k]);
        _szt_swap(p[i], p[k]);
        swaps += (i == k) ? 0 : 1;
    }
#undef _szt_swap
#undef _flt_swap

    return (swaps % 2)? -1 : 1;
}

void GameRotationVector::gyroIntegration(
    struct GyroIntegrationState* gis,
    float* rvOut,
    float const* rvIn,
    float* rvGyroDelta
)
{
    // Time (s) from the last call to this function.
    float dt;
    // Last gyro sample (rad/s)
    float gyroX;
    float gyroY;
    float gyroZ;
    // Magnitude of gyro vector above.
    float gyroMag;
    // \theta/2, where \theta is the rotation angle implied by the gyro
    // vector above.
    float halfTheta;
    // Trig
    float sinHalfTheta;
    float cosHalfTheta;
    // Incremental rotation quaternion implied by (gyroX,gyroY,gyroZ).
    float quatGyroDelta[4];

    // If it becomes necessary to reset the RV, provide a call
    // to set gis->initialized=0 and done.
    if ( !gis->initialized ) {
        gis->quatGyro[0] = rvIn[0];
        gis->quatGyro[1] = rvIn[1];
        gis->quatGyro[2] = rvIn[2];
        gis->quatGyro[3] = rvIn[3];
        gis->ts_ns = gyroTs;
        gis->initialized = 1;
    }

    // 1) Forward-integrate the gyro
    if (gyroTs != gis->ts_ns) {
        dt = (float)(gyroTs - gis->ts_ns) / 1000000000.f;
        gis->ts_ns = gyroTs;
    } else {
        dt = (float)GYRO_MIN_DELAY_US / 1000000.f;
    }

    gyroX = gyroData[0];
    gyroY = gyroData[1];
    gyroZ = gyroData[2];
    // Normalize the gyro vector if we can do so with single precision.
    gyroMag = gyroX*gyroX + gyroY*gyroY + gyroZ*gyroZ;
    // NOTE: the tolerance should be at least approximately machineEps^2,
    //       since we will take a sqrt.
    if (gyroMag > (float)1e-5) {
        gyroMag = sqrtf(gyroMag);
        gyroX /= gyroMag;
        gyroY /= gyroMag;
        gyroZ /= gyroMag;
        halfTheta = dt * gyroMag / 2.f;
    } else {
        // If it is too small to be normalized, let's just say there is
        // no rotation.
        halfTheta = 0.f;
    }

    // Construct incremental gyro rotation quaternion
    sinHalfTheta = sinf(halfTheta);
    cosHalfTheta = cosf(halfTheta);
    quatGyroDelta[0] = sinHalfTheta * gyroX;
    quatGyroDelta[1] = sinHalfTheta * gyroY;
    quatGyroDelta[2] = sinHalfTheta * gyroZ;
    quatGyroDelta[3] = cosHalfTheta;
    Quaternion::quatRenormalize(quatGyroDelta);

    // Multiply to apply incremental rotation (integration). The order is
    // first to rotate by quatGyro, then apply incremental rotation
    // quatGyroDelta. So, we update quatGyro with the argument
    // quatGyroDelta on the right, and quatGyro on the left.
    //   newQuatGyro = quatGyro * quatGyroDelta
    if (Quaternion::quatMul(gis->quatGyro, gis->quatGyro, quatGyroDelta)) {
        ALOGD("gyroIntegration: quatMul bad");
        gis->initialized = 0;
        return;
    }

    // 3) Fuse gyro quaternion to six-axis quaternion, and feedback result
    //    to the gyro quaternion. Voila!

    Quaternion::quatLinInterp(gis->quatGyro, rvIn, gis->quatGyro, NINE_AXIS_FILTER_ALPHA);

    // Copy out
    memcpy( rvOut, gis->quatGyro, 4*sizeof(float) );
    if (rvGyroDelta)
        memcpy( rvGyroDelta, quatGyroDelta, 4*sizeof(float) );
}

void GameRotationVector::processFusion()
{
    /************************Internal state variables*************************/
    static struct GyroIntegrationState gis;
    static int initialized;
    static float nRaw[3];
    static float h[3];
    /*************************************************************************/

    float *n = accelData;
    float *quatGame = fusionData;
    float m[3];
    size_t nPerm[3];

    // Filter the accel for stability.
    if (initialized) {
        nRaw[0] = SIX_AXIS_FILTER_ALPHA*n[0] + (1.f-SIX_AXIS_FILTER_ALPHA)*nRaw[0];
        nRaw[1] = SIX_AXIS_FILTER_ALPHA*n[1] + (1.f-SIX_AXIS_FILTER_ALPHA)*nRaw[1];
        nRaw[2] = SIX_AXIS_FILTER_ALPHA*n[2] + (1.f-SIX_AXIS_FILTER_ALPHA)*nRaw[2];

    } else {
        nRaw[0] = n[0];
        nRaw[1] = n[1];
        nRaw[2] = n[2];

        // Generate arbitrary vector orthogonal to n. Sort the n values so the
        // division is numerically stable.
        permSort(nPerm, n, 3);
        if (fabs(nRaw[nPerm[0]]) <= 2*FLT_EPSILON)
        {
            ALOGD("GameRotationVector: freefall");
            return;
        }
        h[nPerm[2]] = 1.f;
        h[nPerm[1]] = 1.f;
        h[nPerm[0]] = -( nRaw[nPerm[2]] + nRaw[nPerm[1]] ) / nRaw[nPerm[0]];

        initialized = 1;
    }

    // Normalize gravity vector
    float tmpNorm = sqrtf(nRaw[0]*nRaw[0] + nRaw[1]*nRaw[1] + nRaw[2]*nRaw[2]);
    if (tmpNorm < 2*FLT_EPSILON)
    {
        ALOGD("GameRotationVector: unrenormalizable gravity");
        return;
    }
    n[0] = nRaw[0]/tmpNorm;
    n[1] = nRaw[1]/tmpNorm;
    n[2] = nRaw[2]/tmpNorm;

    // Orthogonalize/normalize h ("east") vector
    float projection = h[0]*n[0] + h[1]*n[1] + h[2]*n[2];
    h[0] -= projection*n[0];
    h[1] -= projection*n[1];
    h[2] -= projection*n[2];
    tmpNorm = sqrtf(h[0]*h[0]+h[1]*h[1]+h[2]*h[2]);
    if (tmpNorm < 2*FLT_EPSILON)
    {
        ALOGD("GameRotationVector: unrenormalizable east");
        return;
    }
    h[0] /= tmpNorm;
    h[1] /= tmpNorm;
    h[2] /= tmpNorm;

    // m = n x h. <m,h> == 0 and <m,n> == 0.
    // This means m points "north" parallel to the ground.
    Quaternion::cross3( m, n, h );

    // Now, we have an orthogonal world-coordinate system (h,m,n):
    // x -> h ("east")
    // y -> m ("north")
    // z -> n (up)

    // The rotation matrix from device to world coordinates:
    //     [ h[0] h[1] h[2] ]
    // Q = [ m[0] m[1] m[2] ]
    //     [ n[0] n[1] n[2] ]

    // Use numerically-stable algorithm to recover the quaternion from the
    // rotation matrix. I am sorry for all the expensive sqrtf() calls, but
    // that's the price for numerical stability.
    // - http://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
    // NOTE: the fabs() are to prevent NaNs, as occasionally the argument may
    //       evaluate to small negative numbers (-5e-8) due to the nature of
    //       single-precision.
    float r = sqrtf(fabs(1.0f + h[0] + m[1] + n[2]));
    quatGame[0] = copysignf(0.5f*sqrtf(fabs(1.0f+h[0]-m[1]-n[2])), n[1]-m[2]);
    quatGame[1] = copysignf(0.5f*sqrtf(fabs(1.0f-h[0]+m[1]-n[2])), h[2]-n[0]);
    quatGame[2] = copysignf(0.5f*sqrtf(fabs(1.0f-h[0]-m[1]+n[2])), m[0]-h[1]);
    quatGame[3] = 0.5f*r;
    quatGame[4] = -1;

    // Gyrate a la Chuck Berry?
    gyroIntegration( &gis, quatGame, quatGame, 0 );

    // Notice here that quatGame[3] >= 0 by construction, implying the rotation
    // angle encoded by the quaternion is in [-pi/2, pi/2).

    // See the documentation in the linear accel/gravity vectors to
    // understand what this does. Basically, embed the x "east" vector
    // into a quaternion and extract it from quatGame. This is how we keep
    // the h vector up-to-date using gyro integration
    //
    // qEast = quatGame^{-1} * qx * quatGame
    float qx[4] = {1.f, 0.f, 0.f, 0.f};
    float qEast[4] = {0.f};
    Quaternion::quatInv( qEast, quatGame );
    Quaternion::quatMul( qx, qx, quatGame );
    Quaternion::quatMul_noRenormalize(qEast, qEast, qx);

    h[0] = qEast[0];
    h[1] = qEast[1];
    h[2] = qEast[2];

    fusionTs = gyroTs;
}

#undef NINE_AXIS_FILTER_ALPHA
#undef SIX_AXIS_FILTER_ALPHA
