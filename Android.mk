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

LOCAL_TOP_DIR := $(call my-dir)
LOCAL_PATH := $(LOCAL_TOP_DIR)

# If other Android.mk files are included explicitly, this must be called before
# including those.
include $(call all-subdir-makefiles)

# Restore LOCAL_PATH. Other makefiles probably modified it.
LOCAL_PATH := $(LOCAL_TOP_DIR)

# If the board uses QCOM sensors then don't compile MOTO sensors.
ifeq (, $(filter true,$(BOARD_USES_QCOM_SENSOR_HUB) $(PRODUCT_HAS_QCOMSENSORS)))

###########################################
# Motorola SensorHub section only         #
# Sensors are connected to motorola       #
# internal sensorhub like STM             #
###########################################
ifeq ($(BOARD_USES_MOT_SENSOR_HUB), true)

    ifneq ($(TARGET_SIMULATOR),true)

        ###########################################
        # Select sensorhub type based on platform #
        ###########################################
        # 8996
        ifeq ($(call is-board-platform,msm8996),true)
            SH_MODULE := motosh
            SH_PATH := motosh_hal
            SH_LOGTAG := \"MOTOSH\"
            SH_CFLAGS += -D_ENABLE_LA
            SH_CFLAGS += -D_ENABLE_GR
            SH_CFLAGS += -D_ENABLE_CHOPCHOP
            SH_CFLAGS += -D_ENABLE_LIFT
            SH_CFLAGS += -D_USES_BMI160_ACCGYR
            SH_CFLAGS += -D_ENABLE_PEDO
            ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
                # Expose IR raw data for non-user builds
                SH_CFLAGS += -D_ENABLE_RAW_IR_DATA
            endif
        endif

        # 8952
        ifeq ($(call is-board-platform,msm8952),true)
            SH_MODULE := stml0xx
            SH_PATH := stml0xx_hal
            SH_LOGTAG := \"STML0XX\"
            SH_CFLAGS += -D_ENABLE_BMI160
            # Game RV, Linear Accel, Gravity supported by default with gyroscope
            SH_CFLAGS += -D_ENABLE_GYROSCOPE
            SH_CFLAGS += -D_ENABLE_CHOPCHOP
            SH_CFLAGS += -D_ENABLE_REARPROX
            SH_CFLAGS += -D_ENABLE_PEDO
        endif

        ######################
        # Sensors HAL module #
        ######################
        include $(CLEAR_VARS)

        LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"
        LOCAL_CFLAGS += $(SH_CFLAGS)

        LOCAL_SRC_FILES :=              \
            SensorBase.cpp   \
            $(SH_PATH)/SensorHal.cpp    \
            $(SH_PATH)/HubSensors.cpp

        ifneq (,$(filter athene_%, $(strip $(TARGET_PRODUCT))))
            # Sensor HAL file for M0 hub (low-tier) products (athene, etc...)
            LOCAL_SRC_FILES += \
                $(SH_PATH)/RearProxSensor.cpp \
                InputEventReader.cpp \
                $(SH_PATH)/FusionSensorBase.cpp \
                $(SH_PATH)/Quaternion.cpp \
                $(SH_PATH)/GameRotationVector.cpp \
                $(SH_PATH)/LinearAccelGravity.cpp
        else
            # Sensor HAL files for M4 and L4 (high-tier) products (vector, etc...)
            LOCAL_SRC_FILES += \
                $(SH_PATH)/SensorList.cpp
        endif

        # This file must be last, for some mysterious reason
        LOCAL_SRC_FILES += \
            $(SH_PATH)/SensorsPollContext.cpp

        LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SH_PATH)
        LOCAL_C_INCLUDES += external/zlib

        LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
        # Need the UAPI output directory to be populated with motosh.h/stml0xx.h
        LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

        LOCAL_PRELINK_MODULE := false
        LOCAL_MODULE_RELATIVE_PATH := hw
        LOCAL_MODULE_TAGS := optional
        LOCAL_SHARED_LIBRARIES := liblog libcutils libz libdl libutils
        LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

        include $(BUILD_SHARED_LIBRARY)

    endif # !TARGET_SIMULATOR

    #########################
    # Sensor Hub HAL module #
    #########################
    include $(CLEAR_VARS)

    LOCAL_PRELINK_MODULE := false
    LOCAL_MODULE_RELATIVE_PATH := hw
    LOCAL_SRC_FILES := $(SH_PATH)/sensorhub.c

    LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
    # Need the UAPI output directory to be populated with motosh.h/stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

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
    LOCAL_CFLAGS := -DLOG_TAG=$(SH_LOGTAG) -DMODULE_$(SH_MODULE)
    LOCAL_MODULE := $(SH_MODULE)
    #LOCAL_CFLAGS+= -D_DEBUG
    LOCAL_CFLAGS += -Wall -Wextra
    # Added by top level make files: -std=gnu++11
    LOCAL_CXXFLAGS += -Weffc++
    LOCAL_SHARED_LIBRARIES := libcutils libc libsensorhub

    LOCAL_SRC_FILES := \
        motosh_bin/motosh.cpp \
        motosh_bin/CRC32.c
    LOCAL_REQUIRED_MODULES += sensorhub-blacklist.txt

    LOCAL_C_INCLUDES := \
        $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
        hardware/moto/sensors/libsensorhub

    # Need the UAPI output directory to be populated with motosh.h/stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

    include $(BUILD_EXECUTABLE)

    # ** Firmware BlackList **********************************************************
    include $(CLEAR_VARS)
    LOCAL_MODULE        := sensorhub-blacklist.txt
    LOCAL_MODULE_TAGS   := optional
    LOCAL_MODULE_CLASS  := ETC
    LOCAL_MODULE_PATH   := $(TARGET_OUT)/etc/firmware
    LOCAL_SRC_FILES     := motosh_bin/sensorhub-blacklist.txt
    include $(BUILD_PREBUILT)
    # ********************************************************************************

else # For non sensorhub version of sensors
    ###########################################
    # No-SensorHub section only               #
    # Sensors are connected directly to AP    #
    ###########################################

endif # BOARD_USES_MOT_SENSOR_HUB

endif # !BOARD_USES_QCOM_SENSOR_HUB

