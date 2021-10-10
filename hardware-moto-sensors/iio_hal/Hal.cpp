/*
 * Copyright (C) 2016 Motorola Mobility
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

#include "IioHal.h"

/** This is it. The entry point to this HAL. Every hardware module must have a
 * data structure named HAL_MODULE_INFO_SYM. IioHal provides the specialization
 * necessary to make this a Dynamic IIO Sensors HAL. */
SensorsModuleT< IioHal > HAL_MODULE_INFO_SYM("Dynamic IIO Sensors Module");

