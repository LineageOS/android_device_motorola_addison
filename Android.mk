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

###########################################
# Motorola SensorHub section only         #
# Sensors are connected to motorola       #
# internal sensorhub like STM401          #
###########################################
ifeq ($(BOARD_USES_MOT_SENSOR_HUB), true)

ifneq ($(TARGET_SIMULATOR),true)

###########################################
# Select sensorhub type based on platform #
###########################################
# 8974 / 8084
ifneq (, $(filter $(TARGET_BOARD_PLATFORM),msm8974 apq8084))
SH_MODULE := stm401
SH_PATH := STM401
SH_LOGTAG := \"STM401\"
#SH_CFLAGS += -D_ENABLE_PEDO
#SH_CFLAGS += -D_ENABLE_LA
#SH_CFLAGS += -D_ENABLE_GR
SH_CFLAGS += -D_ENABLE_CHOPCHOP
SH_CFLAGS += -D_ENABLE_LIFT
endif

# 8994
ifeq ($(call is-board-platform,msm8994),true)
SH_MODULE := motosh
SH_PATH := MOTOSH
SH_LOGTAG := \"MOTOSH\"
#SH_CFLAGS += -D_ENABLE_PEDO
SH_CFLAGS += -D_ENABLE_LA
SH_CFLAGS += -D_ENABLE_GR
SH_CFLAGS += -D_ENABLE_CHOPCHOP
SH_CFLAGS += -D_ENABLE_LIFT
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
# Expose IR raw data for non-user builds
SH_CFLAGS += -D_ENABLE_RAW_IR_DATA
endif
endif

# 8992
ifeq ($(call is-board-platform,msm8992),true)
SH_MODULE := motosh
SH_PATH := MOTOSH
SH_LOGTAG := \"MOTOSH\"
SH_CFLAGS += -D_ENABLE_LA
SH_CFLAGS += -D_ENABLE_GR
SH_CFLAGS += -D_ENABLE_CHOPCHOP
SH_CFLAGS += -D_ENABLE_LIFT
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
# Expose IR raw data for non-user builds
SH_CFLAGS += -D_ENABLE_RAW_IR_DATA
endif
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
ifneq (,$(filter lux_% osprey_%, $(strip $(TARGET_PRODUCT))))
SH_CFLAGS += -D_ENABLE_MAGNETOMETER
SH_CFLAGS += -D_ENABLE_CHOPCHOP
endif
endif

# 8610
ifeq ($(call is-board-platform,msm8610),true)
SH_MODULE := stml0xx
SH_PATH := STML0XX
SH_LOGTAG := \"STML0XX\"
ifneq (,$(filter userdebug eng,$(TARGET_BUILD_VARIANT)))
# Expose secondary accel for non-user builds
SH_CFLAGS += -D_ENABLE_ACCEL_SECONDARY
endif
endif

######################
# Sensors HAL module #
######################
include $(CLEAR_VARS)

LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"
LOCAL_CFLAGS += $(SH_CFLAGS)

ifneq (,$(filter surnia_% otus_% lux_% osprey_%, $(strip $(TARGET_PRODUCT))))
# Sensor HAL file for M0 hub products
LOCAL_SRC_FILES := \
                $(SH_PATH)/SensorBase.cpp \
                $(SH_PATH)/HubSensor.cpp \
		$(SH_PATH)/SensorHal.cpp

ifneq (,$(filter lux_% osprey_%, $(strip $(TARGET_PRODUCT))))
# Additional Sensor HAL file for M0 hub products with magnetometer
LOCAL_SRC_FILES += \
		$(SH_PATH)/AkmSensor.cpp  \
		$(SH_PATH)/InputEventReader.cpp
endif /* lux || osprey */

# This file must be last
LOCAL_SRC_FILES += \
		$(SH_PATH)/SensorsPollContext.cpp
else /* surnia || otus || lux || osprey */
# Sensor HAL files for M4 and L4 products
LOCAL_SRC_FILES := \
		SensorBase.cpp \
		$(SH_PATH)/nusensors.cpp \
		$(SH_PATH)/sensors.c \
		$(SH_PATH)/sensorhub_hal.cpp
endif /* surnia || otus || lux || osprey */

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SH_PATH)
LOCAL_C_INCLUDES += external/zlib

ifneq (,$(filter motosh stml0xx,$(SH_MODULE)))
	LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
	# Need the UAPI output directory to be populated with motosh.h/stml0xx.h
	LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
else
	LOCAL_C_INCLUDES += bionic/libc/kernel/common
endif # SH_MODULE == "motosh"/"stml0xx"


LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libcutils libz libdl libutils libstlport
include external/stlport/libstlport.mk
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

ifneq (,$(filter motosh stml0xx,$(SH_MODULE)))
	LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
	# Need the UAPI output directory to be populated with motosh.h/stml0xx.h
	LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
else
	LOCAL_C_INCLUDES += bionic/libc/kernel/common
endif # SH_MODULE == "motosh"/"stml0xx"

LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_MODULE := sensorhub.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

#########################
# AKM executable        #
#########################
include $(CLEAR_VARS)

ifneq (,$(filter lux_% osprey_%, $(strip $(TARGET_PRODUCT))))

AKM_PATH := 8916_ak09912_akmd_6D
SMARTCOMPASS_LIB := libSmartCompass

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE  := akmd09912

LOCAL_C_INCLUDES := \
        bionic/libc/kernel/uapi \
        $(LOCAL_PATH)/$(AKM_PATH) \
        $(LOCAL_PATH)/$(AKM_PATH)/$(SMARTCOMPASS_LIB)

LOCAL_SRC_FILES := \
        $(AKM_PATH)/AKMD_Driver.c \
        $(AKM_PATH)/DispMessage.c \
        $(AKM_PATH)/FileIO.c \
        $(AKM_PATH)/Measure.c \
        $(AKM_PATH)/main.c \
        $(AKM_PATH)/misc.c \
	$(AKM_PATH)/FST_AK09912.c \
	$(AKM_PATH)/Acc_aot.c

LOCAL_CFLAGS := -DAKMD_FOR_AK09912
LOCAL_CFLAGS += -DAKMD_AK099XX
LOCAL_CFLAGS += -DAKMD_ACC_EXTERNAL
LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -DENABLE_AKMDEBUG=1
#LOCAL_CFLAGS += -DAKM_LOG_ENABLE

LOCAL_STATIC_LIBRARIES := AK09912

LOCAL_FORCE_STATIC_EXECUTABLE := false
LOCAL_SHARED_LIBRARIES := libc libm libutils libcutils

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE        := AK09912
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_SRC_FILES_arm   := $(AKM_PATH)/$(SMARTCOMPASS_LIB)/arm/libAK09912.a
LOCAL_SRC_FILES_arm64 := $(AKM_PATH)/$(SMARTCOMPASS_LIB)/arm64/libAK09912.a
include $(BUILD_PREBUILT)

endif # lux osprey

###########################
# Sensor Hub Flash loader #
###########################
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES := sensorhub.$(TARGET_BOARD_PLATFORM)
LOCAL_REQUIRED_MODULES += sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=$(SH_LOGTAG)
LOCAL_SRC_FILES := $(SH_PATH)/$(SH_MODULE).cpp
LOCAL_MODULE:= $(SH_MODULE)
#LOCAL_CFLAGS+= -D_DEBUG
LOCAL_SHARED_LIBRARIES := libcutils libc
ifeq ($(SH_MODULE),motosh)
LOCAL_SRC_FILES += \
	$(SH_PATH)/CRC32.c
endif
ifneq (,$(filter motosh stml0xx,$(SH_MODULE)))
	LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
	# Need the UAPI output directory to be populated with motosh.h/stml0xx.h
	LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
else
	# For other flash loaders still relying on bionic
	LOCAL_C_INCLUDES += bionic/libc/kernel/common
endif # SH_MODULE == "motosh"/"stml0xx.h"

include $(BUILD_EXECUTABLE)

###########################################
# NO SensorHub section only               #
# Sensors are connected directly to AP    #
###########################################
else # For non sensorhub version of sensors

# 8226
ifeq ($(TARGET_BOARD_PLATFORM),msm8226)
SENSOR_PATH := 8226

# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

#LOCAL_MODULE := sensors.$(TARGET_DEVICE)
LOCAL_MODULE := sensors.msm8226
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

LOCAL_SRC_FILES := 						\
				$(SENSOR_PATH)/sensors.c 			\
				$(SENSOR_PATH)/nusensors.cpp 			\
				$(SENSOR_PATH)/InputEventReader.cpp		\
				$(SENSOR_PATH)/SensorBase.cpp			\
				$(SENSOR_PATH)/AkmSensor.cpp			\
				$(SENSOR_PATH)/lis3dh_hal.cpp			\
				$(SENSOR_PATH)/ct406_hal.cpp

LOCAL_C_INCLUDES := bionic/libc/kernel/common

ifeq ($(FEATURE_GYRO_L3D4200), true)
    LOCAL_SRC_FILES += $(SENSOR_PATH)/l3g4200d_hal.cpp
    LOCAL_CFLAGS += -DFEATURE_GYRO_L3D4200_SUPPORTED
endif

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

endif # TARGET_BOARD_PLATFORM == 8226

# 8610 / 8930 / 8960
ifneq ($(filter msm8610 msm8930 msm8960,$(TARGET_BOARD_PLATFORM)),)
SENSOR_PATH := 8610

# HAL module implemenation stored in
include $(CLEAR_VARS)

ifneq ($(filter msm8610,$(TARGET_BOARD_PLATFORM)),)
  LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)
  LOCAL_CFLAGS := -DTARGET_8610
else
  LOCAL_MODULE := sensors.msm8930
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := bionic/libc/kernel/common
LOCAL_CFLAGS += -DLOG_TAG=\"Sensors\"
ifeq ($(call is-board-platform,msm8960),true)
  LOCAL_CFLAGS += -DTARGET_8930
endif

LOCAL_SRC_FILES :=	\
		$(SENSOR_PATH)/sensors.cpp 			\
		$(SENSOR_PATH)/SensorBase.cpp			\
		$(SENSOR_PATH)/Accelerometer.cpp		\
		$(SENSOR_PATH)/InputEventReader.cpp		\
		$(SENSOR_PATH)/ct406_hal.cpp

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl

include $(BUILD_SHARED_LIBRARY)

endif # TARGET_BOARD_PLATFORM == msm8610 msm8930 msm8960

endif # BOARD_USES_MOT_SENSOR_HUB

endif # !BOARD_USES_QCOM_SENSOR_HUB
