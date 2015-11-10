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

#include <math.h>
#include <float.h>
#include "Quaternion.h"

void Quaternion::cross3(float* out, float* u, float* v)
{
        out[0] = u[1]*v[2] - u[2]*v[1];
        out[1] = u[2]*v[0] - u[0]*v[2];
        out[2] = u[0]*v[1] - u[1]*v[0];
}

/*!
 * \brief out = q^(-1)
 *
 * Produces the inverse quaternion, i.e.
 * \f[
 *   qq^{-1} = q^{-1}q = 1
 * \f]
 *
 * \param[out] out inverse of \c q
 * \param[in] q quaternion to invert
 */
void Quaternion::quatInv(float* out, float const* q)
{
        out[0] = -q[0];
        out[1] = -q[1];
        out[2] = -q[2];
        out[3] =  q[3];
}

/*!
 * \brief q = q1*q2
 *
 * The result is automatically normalized.
 * Quaternion multiplication is not commutative, i.e.
 * \f[
 *    q1*q2 \neq q2*q1.
 * \f]
 *
 * \note{It is OK if \c q is aliased with \c q1 or \c q2.}
 *
 * \see quatRenormalize()
 * \returns 0 on success, 1 on failure
 */
int Quaternion::quatMul(float* q, float const* q1, float const* q2)
{
        float out[4];
        int ret;

        out[0] = q1[3]*q2[0] + q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1];
        out[1] = q1[3]*q2[1] - q1[0]*q2[2] + q1[1]*q2[3] + q1[2]*q2[0];
        out[2] = q1[3]*q2[2] + q1[0]*q2[1] - q1[1]*q2[0] + q1[2]*q2[3];
        out[3] = q1[3]*q2[3] - q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2];
        ret = quatRenormalize(out);

        q[0] = out[0];
        q[1] = out[1];
        q[2] = out[2];
        q[3] = out[3];

        return ret;
}

/*!
 * \brief Renormalize quaternion if needed
 *
 * If \f$ ||q|| \f$ is not a unit vector, make it one.
 * Also force q[3] >= 0 so that the encoded angle is in \f$ [-pi/2, pi/2) \f$.
 *
 * \returns 0 on success, 1 if \c q is non-renormalizable
 */
int Quaternion::quatRenormalize(float* q)
{
        // Square magnitude
        float mag = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];

#define MAG_TOL (0.0001f)
        if (mag < MAG_TOL) {
                // This is bad. Quaternion is not renormalizable.
                q[0] = 0.f;
                q[1] = 0.f;
                q[2] = 0.f;
                q[3] = 1.f;
                return 1;
        } else if (mag > 1.f + MAG_TOL || mag < 1.f - MAG_TOL) {
                // Not only do we want to normalize, but we want to keep
                // q[3] >= 0 so that the encoded angle is in [-pi/2, pi/2)
                mag = copysignf( sqrtf(mag), q[3] );
                q[0] /= mag;
                q[1] /= mag;
                q[2] /= mag;
                q[3] /= mag;
        }
#undef MAG_TOL

        return 0;
}

/*!
 * \brief q = q1*q2
 *
 * Same as \c quatMul() without normalization.
 *
 * \see quatMul()
 */
void Quaternion::quatMul_noRenormalize(float* q, float const* q1, float const* q2)
{
        float out[4];

        out[0] = q1[3]*q2[0] + q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1];
        out[1] = q1[3]*q2[1] - q1[0]*q2[2] + q1[1]*q2[3] + q1[2]*q2[0];
        out[2] = q1[3]*q2[2] + q1[0]*q2[1] - q1[1]*q2[0] + q1[2]*q2[3];
        out[3] = q1[3]*q2[3] - q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2];

        q[0] = out[0];
        q[1] = out[1];
        q[2] = out[2];
        q[3] = out[3];
}

//! \brief Squared distance between two quaternions
float Quaternion::quatDist(float const* q1, float const* q2)
{
        return (q1[0]-q2[0])*(q1[0]-q2[0]) + (q1[1]-q2[1])*(q1[1]-q2[1]) +
                   (q1[2]-q2[2])*(q1[2]-q2[2]) + (q1[3]-q2[3])*(q1[3]-q2[3]);
}

/*!
 * \brief out = quatRenormalize(alpha*q1 + (1-alpha)*q2)
 *
 * Forms a linear interpolant of q1 and q2 along the shortest path from q1 to
 * q2, and renormalizes.
 *
 * There are other ways to interpolate quaternions, e.g.
 * SLERP (spherical-linear), but those are more complicated, computationally
 * expensive, and the comparative advantages (constant angular speed) small.
 *
 * \note{It is OK if \c out is aliased with \c q1 or \c q2.}
 *
 * \param out[out] Resultant quaternion (normalized)
 * \param q1[in] First input to interpolation
 * \param q2[in] Second input to interpolation
 * \param alpha[in] Interpolation factor in [0,1]
 *
 * \returns 0 on success, 1 on failure
 */
int Quaternion::quatLinInterp(float* out, float const* q1, float const* q2, float const alpha)
{
        float minusQ2[4];
        float oneMinusAlpha = 1.f-alpha;

        // There are always 2 ways to get from q1 to q2, just like there are always
        // 2 paths between any two places on Earth (the short arc, and the long arc).
        // We will test to see which way is shorter by constructing -q2.
        minusQ2[0] = -q2[0];
        minusQ2[1] = -q2[1];
        minusQ2[2] = -q2[2];
        minusQ2[3] = -q2[3];

        // Looks like the opposite way will be shorter...
        if ( quatDist(q1, minusQ2) < quatDist(q1, q2) )
                oneMinusAlpha = -oneMinusAlpha;

        out[0] = alpha*q1[0] + oneMinusAlpha*q2[0];
        out[1] = alpha*q1[1] + oneMinusAlpha*q2[1];
        out[2] = alpha*q1[2] + oneMinusAlpha*q2[2];
        out[3] = alpha*q1[3] + oneMinusAlpha*q2[3];

        return quatRenormalize(out);
}
