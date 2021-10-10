LOCAL_PATH := $(call my-dir)

ifeq ($(MOT_SENSOR_HUB_HW_TYPE_L4), true)

################################################################################
## lsiio #######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := lsiio
LOCAL_CFLAGS += -Wall -Wextra

LOCAL_SRC_FILES := lsiio.c

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
    kernel/include

include $(BUILD_EXECUTABLE)

################################################################################
## IIO Event Monitor ###########################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := iio_event_monitor
LOCAL_CFLAGS += -Wall -Wextra

LOCAL_SRC_FILES := iio_event_monitor.c

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
    kernel/include

include $(BUILD_EXECUTABLE)

################################################################################
## Generic Buffer ##############################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := generic_buffer
LOCAL_CFLAGS += -Wall -Wextra

LOCAL_SRC_FILES := generic_buffer.c

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
    kernel/include

include $(BUILD_EXECUTABLE)

endif # MOT_SENSOR_HUB_HW_TYPE_L4
