LOCAL_PATH := $(call my-dir)

ifneq ($(BOARD_USES_MOT_SENSOR_HUB), true)
    # For non SensorHub version of sensors (sensors connected directly to AP)

    ifneq ($(filter msm8610 msm8930 msm8960,$(TARGET_BOARD_PLATFORM)),)
        # 8610 / 8930 / 8960

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

        LOCAL_SRC_FILES :=              \
                sensors.cpp             \
                SensorBase.cpp          \
                Accelerometer.cpp       \
                InputEventReader.cpp    \
                ct406_hal.cpp

        LOCAL_SHARED_LIBRARIES := liblog libcutils libdl

        include $(BUILD_SHARED_LIBRARY)

    endif # TARGET_BOARD_PLATFORM == msm8610 msm8930 msm8960
endif # BOARD_USES_MOT_SENSOR_HUB
