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

        UTILS_PATH := utils

        ###################################
        # Select sensorhub processor type #
        ###################################
        ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L4), true)
            SH_MODULE := motosh
            SH_PATH := motosh_hal
            ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
                # Expose IR raw data for non-user builds
                SH_CFLAGS += -D_ENABLE_RAW_IR_DATA
            endif
        else ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L0), true)
            SH_MODULE := stml0xx
            SH_PATH := stml0xx_hal
        endif

        ##########################
        # Select sensor hardware #
        ##########################
        ifeq ($(MOT_SENSOR_HUB_HW_BMI160), true)
            ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L4), true)
                SH_CFLAGS += -D_USES_BMI160_ACCGYR
            else ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L0), true)
                SH_CFLAGS += -D_ENABLE_BMI160
                SH_CFLAGS += -D_ENABLE_GYROSCOPE
            endif
        endif

        ifeq ($(MOT_AP_SENSOR_HW_REARPROX), true)
            SH_CFLAGS += -D_ENABLE_REARPROX
        endif

        ifeq ($(MOT_SENSOR_HUB_HW_IR), true)
            SH_CFLAGS += -D_ENABLE_IR
        endif

        ##########################
        # Select sensorhub algos #
        ##########################
        ifeq ($(MOT_SENSOR_HUB_FEATURE_CHOPCHOP), true)
            SH_CFLAGS += -D_ENABLE_CHOPCHOP
        endif

        ifeq ($(MOT_SENSOR_HUB_FEATURE_LIFT), true)
            SH_CFLAGS += -D_ENABLE_LIFT
        endif

        ifeq ($(MOT_SENSOR_HUB_FEATURE_PEDO), true)
            SH_CFLAGS += -D_ENABLE_PEDO
        endif

        ifeq ($(MOT_SENSOR_HUB_FEATURE_LA), true)
            SH_CFLAGS += -D_ENABLE_LA
        endif

        ifeq ($(MOT_SENSOR_HUB_FEATURE_GR), true)
            SH_CFLAGS += -D_ENABLE_GR
        endif
        ifeq ($(MOT_SENSOR_HUB_FEATURE_CAMFSYNC), true)
            SH_CFLAGS += -D_CAMFSYNC
        endif
        ifeq ($(MOT_SENSOR_HUB_FEATURE_ULTRASOUND), true)
            SH_CFLAGS += -D_ENABLE_ULTRASOUND
        endif

        ######################
        # Sensors HAL module #
        ######################
        include $(CLEAR_VARS)

        LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"
        LOCAL_CFLAGS += $(SH_CFLAGS)
        LOCAL_CXX_FLAGS += -std=c++14

        LOCAL_SRC_FILES :=              \
            $(SH_PATH)/SensorBase.cpp   \
            $(SH_PATH)/SensorHal.cpp    \
            $(SH_PATH)/HubSensors.cpp   \
            $(SH_PATH)/SensorList.cpp

        ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L0), true)
            # Sensor HAL file for M0 hub (low-tier) products (athene, etc...)
            LOCAL_SRC_FILES += \
                $(SH_PATH)/FusionSensorBase.cpp \
                $(SH_PATH)/Quaternion.cpp \
                $(SH_PATH)/GameRotationVector.cpp \
                $(SH_PATH)/LinearAccelGravity.cpp
        endif

        ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L4), true)
            LOCAL_SHARED_LIBRARIES += libiio
            LOCAL_SRC_FILES += \
                $(SH_PATH)/IioSensor.cpp \
                $(SH_PATH)/UeventListener.cpp
            LOCAL_C_INCLUDES += motorola/external/libiio
        endif

        ifeq ($(MOT_AP_SENSOR_HW_REARPROX), true)
            LOCAL_SRC_FILES += \
                $(SH_PATH)/RearProxSensor.cpp \
                InputEventReader.cpp
        endif

        # This file must be last, for some mysterious reason
        LOCAL_SRC_FILES += \
            $(SH_PATH)/SensorsPollContext.cpp

        LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SH_PATH)
        LOCAL_C_INCLUDES += external/zlib

        LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
                            system/core/base/include

        # Needs to be added after KERNEL_OBJ/usr/include
        ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L4), true)
            LOCAL_C_INCLUDES += $(ANDROID_BUILD_TOP)/kernel/include
        endif

        # Need the UAPI output directory to be populated with motosh.h/stml0xx.h
        LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

        LOCAL_PRELINK_MODULE := false
        LOCAL_MODULE_RELATIVE_PATH := hw
        LOCAL_MODULE_TAGS := optional
        LOCAL_SHARED_LIBRARIES += liblog libcutils libz libdl libutils
        LOCAL_CLANG := false
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
    LOCAL_SRC_FILES += $(UTILS_PATH)/sensor_time.cpp

    LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
    # Need the UAPI output directory to be populated with motosh.h/stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

    LOCAL_SHARED_LIBRARIES := libcutils libc libutils
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
    LOCAL_CFLAGS := -DLOG_TAG=\"MOTOSH\" -DMODULE_$(SH_MODULE)
    LOCAL_MODULE := motosh
    #LOCAL_CFLAGS+= -D_DEBUG
    LOCAL_CFLAGS += -Wall -Wextra
    # Added by top level make files: -std=gnu++11
    LOCAL_CXXFLAGS += -Weffc++
    LOCAL_SHARED_LIBRARIES := libcutils libc libsensorhub

    LOCAL_SRC_FILES := \
        motosh_bin/motosh.cpp \
        motosh_bin/CRC32.c
    LOCAL_REQUIRED_MODULES += sensorhub-blacklist.txt

    ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L4), true)
        ifneq ($(TARGET_BUILD_VARIANT),user)
            # Build libiio.so
            LOCAL_REQUIRED_MODULES += libiio

            # Build libiio tests/utilities
            LOCAL_REQUIRED_MODULES += iio_genxml iio_info iio_readdev iio_reg
            # Build the kernel provided IIO Utilities
            #LOCAL_REQUIRED_MODULES += generic_buffer lsiio iio_event_monitor
        endif
    endif

    LOCAL_C_INCLUDES := \
        $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
        hardware/moto/sensors/libsensorhub

    # Need the UAPI output directory to be populated with motosh.h/stml0xx.h
    LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

    include $(BUILD_EXECUTABLE)

    ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L0), true)
        # This creates a link from stml0xx to motosh so that code that
        # uses the old name will still work. This can be removed once
        # everything has been updated to the new name.
        OLD_SH_BIN := stml0xx
        SH_SYMLINK := $(addprefix $(TARGET_OUT)/bin/,$(OLD_SH_BIN))
        $(SH_SYMLINK): NEW_SH_BIN := $(LOCAL_MODULE)
        $(SH_SYMLINK): $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
        # WARNING - the below lines must be indented with a TAB, not spaces
		@echo "Symlink: $@ -> $(NEW_SH_BIN)"
		@mkdir -p $(dir $@)
		@rm -rf $@
		$(hide) ln -sf $(NEW_SH_BIN) $@
        ALL_DEFAULT_INSTALLED_MODULES += $(SH_SYMLINK)
    endif

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

