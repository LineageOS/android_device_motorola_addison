# Copyright (C) 2009-2012 Motorola Mobility, Inc.
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

ifneq ($(PRODUCT_HAS_QCOMSENSORS),true)

ifneq ($(TARGET_SIMULATOR),true)

# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.board.platform>.so
include $(CLEAR_VARS)

LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"

LOCAL_SRC_FILES := 						\
				SensorBase.cpp			\
				sensors.c 			\
				nusensors.cpp 			\
				stm401_hal.cpp

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libcutils libz
LOCAL_C_INCLUDES := external/zlib

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

include $(BUILD_SHARED_LIBRARY)

endif # !TARGET_SIMULATOR
################################################################
include $(CLEAR_VARS)

# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.board.platform>.so
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SRC_FILES := sensorhub.c

LOCAL_SHARED_LIBRARIES := libcutils libc

LOCAL_MODULE := sensorhub.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

#################################################################
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES := sensorhub.$(TARGET_BOARD_PLATFORM)
LOCAL_REQUIRED_MODULES += sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"STM401\"
#LOCAL_CFLAGS+= -D_STM401_PEDO
#LOCAL_CFLAGS+= -D_STM401_NFC
#LOCAL_CFLAGS+= -D_STM401_DB
#LOCAL_CFLAGS+= -D_STM401_DOCK
#LOCAL_CFLAGS+= -D_STM401_LA
#LOCAL_CFLAGS+= -D_STM401_GR
#LOCAL_CFLAGS+= -D_STM401_QUATERNION
LOCAL_SRC_FILES:= stm401.cpp
LOCAL_MODULE:= stm401
#LOCAL_CFLAGS+= -D_DEBUG
LOCAL_SHARED_LIBRARIES := libcutils libc
include $(BUILD_EXECUTABLE)

endif # !PRODUCT_HAS_QCOMSENSORS
