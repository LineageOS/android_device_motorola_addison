/*
 * Copyright (C) 2009-2015 Motorola Mobility
 *
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

#ifndef SENSORLIST_H
#define SENSORLIST_H

#include <hardware/sensors.h>
#include <hardware/mot_sensorhub_motosh.h>
#include <float.h>
#include <limits.h>

#include "Sensors.h"

extern const struct sensor_t sSensorList[];
extern const int sSensorListSize;

#endif // SENSORLIST_H
