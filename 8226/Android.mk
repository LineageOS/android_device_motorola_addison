# Copyright (C) 2009-2011 Motorola Mobility, Inc.
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Only for SC1 devices

LOCAL_PATH:= $(call my-dir)

ifneq ($(BOARD_USES_MOT_SENSOR_HUB), true)
    # For non SensorHub version of sensors (sensors connected directly to AP)

    ifeq ($(TARGET_BOARD_PLATFORM),msm8226)

        # HAL module implemenation, not prelinked, and stored in
        # hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
        include $(CLEAR_VARS)

        LOCAL_MODULE := sensors.msm8226

        LOCAL_MODULE_RELATIVE_PATH := hw

        LOCAL_MODULE_TAGS := optional

        LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

        LOCAL_SRC_FILES :=          \
            sensors.c               \
            nusensors.cpp           \
            InputEventReader.cpp    \
            SensorBase.cpp          \
            AkmSensor.cpp           \
            lis3dh_hal.cpp          \
            ct406_hal.cpp

        LOCAL_C_INCLUDES := bionic/libc/kernel/common

        ifeq ($(FEATURE_GYRO_L3D4200), true)
            LOCAL_SRC_FILES += l3g4200d_hal.cpp
            LOCAL_CFLAGS += -DFEATURE_GYRO_L3D4200_SUPPORTED
        endif

        LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
        LOCAL_PRELINK_MODULE := false

        include $(BUILD_SHARED_LIBRARY)

    endif # msm8226
endif # BOARD_USES_MOT_SENSOR_HUB
