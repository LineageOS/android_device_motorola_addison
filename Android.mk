# Copyright (C) 2009-2014 Motorola Mobility, Inc.
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

LOCAL_PATH:= $(call my-dir)

# If the board uses QCOM sensors then don't compile MOTO sensors.
ifeq (, $(filter true,$(BOARD_USES_QCOM_SENSOR_HUB) $(PRODUCT_HAS_QCOMSENSORS)))
ifneq ($(TARGET_SIMULATOR),true)

###########################################
# Select sensorhub type based on platform #
###########################################
# 8974 / 8084
ifneq (, $(filter $(TARGET_BOARD_PLATFORM),msm8974 apq8084),true)
SH_MODULE := stm401
SH_PATH := STM401
SH_LOGTAG := \"STM401\"
#SH_CFLAGS += -D_ENABLE_PEDO
#SH_CFLAGS += -D_ENABLE_NFC
#SH_CFLAGS += -D_ENABLE_DB
#SH_CFLAGS += -D_ENABLE_DOCK
#SH_CFLAGS += -D_ENABLE_LA
#SH_CFLAGS += -D_ENABLE_GR
#SH_CFLAGS += -D_ENABLE_QUATERNION
endif

# 8916
ifeq ($(call is-board-platform,msm8916),true)
SH_MODULE := stml0xx
SH_PATH := STML0XX
SH_LOGTAG := \"STML0XX\"
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
# Expose secondary accel for non-user builds
SH_CFLAGS += -D_ENABLE_ACCEL_SECONDARY
endif
endif

# 8610
ifeq ($(call is-board-platform,msm8610),true)
ifeq ($(BOARD_USES_MOT_SENSOR_HUB), true)
SH_MODULE := stml0xx
SH_PATH := STML0XX
SH_LOGTAG := \"STML0XX\"
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
# Expose secondary accel for non-user builds
SH_CFLAGS += -D_ENABLE_ACCEL_SECONDARY
endif
endif
endif

######################
# Sensors HAL module #
######################
include $(CLEAR_VARS)

LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"
LOCAL_CFLAGS += $(SH_CFLAGS)

LOCAL_SRC_FILES := \
                SensorBase.cpp \
                $(SH_PATH)/nusensors.cpp \
                $(SH_PATH)/sensors.c \
                $(SH_PATH)/sensorhub_hal.cpp \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SH_PATH) \
                    external/zlib

LOCAL_C_INCLUDES += bionic/libc/kernel/common

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libcutils libz
LOCAL_C_INCLUDES := external/zlib \
                    bionic/libc/kernel/common
LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

include $(BUILD_SHARED_LIBRARY)

endif # !TARGET_SIMULATOR

#########################
# Sensor Hub HAL module #
#########################
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := $(SH_PATH)/sensorhub.c
LOCAL_C_INCLUDES := bionic/libc/kernel/common

LOCAL_C_INCLUDES := bionic/libc/kernel/common

LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_MODULE := sensorhub.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

###########################
# Sensor Hub Flash loader #
###########################
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES := sensorhub.$(TARGET_BOARD_PLATFORM)
LOCAL_REQUIRED_MODULES += sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=$(SH_LOGTAG)
LOCAL_SRC_FILES:= $(SH_PATH)/$(SH_MODULE).cpp
LOCAL_MODULE:= $(SH_MODULE)
#LOCAL_CFLAGS+= -D_DEBUG
LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_C_INCLUDES := bionic/libc/kernel/common
include $(BUILD_EXECUTABLE)

endif # !BOARD_USES_QCOM_SENSOR_HUB
