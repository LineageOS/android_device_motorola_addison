
LOCAL_PATH := $(call my-dir)

# ** Firmware BlackList **********************************************************
include $(CLEAR_VARS)
LOCAL_MODULE        := sensorhub-blacklist.txt
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_PATH   := $(TARGET_OUT)/etc/firmware
LOCAL_SRC_FILES     := sensorhub-blacklist.txt
include $(BUILD_PREBUILT)
# ********************************************************************************

