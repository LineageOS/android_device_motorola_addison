#
# Copyright (C) 2016 The CyanogenMod Project
#               2017-2022 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include vendor/motorola/addison/BoardConfigVendor.mk

DEVICE_PATH := device/motorola/addison

# BUILD_BROKEN_*
BUILD_BROKEN_DUP_RULES := true

# APEX
DEXPREOPT_GENERATE_APEX_IMAGE := true

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := cortex-a53

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic
TARGET_2ND_CPU_VARIANT_RUNTIME := cortex-a53

BOARD_USES_QCOM_HARDWARE := true
TARGET_BOARD_PLATFORM := msm8953
TARGET_BOOTLOADER_BOARD_NAME := MSM8953
TARGET_NO_BOOTLOADER := true

# Asserts
TARGET_OTA_ASSERT_DEVICE := addison,addison_retail

# Audio
BOARD_USES_ALSA_AUDIO := true
BOARD_SUPPORTS_SOUND_TRIGGER_HAL := true
TARGET_EXCLUDES_AUDIOFX := true

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(DEVICE_PATH)/bluetooth

# Camera
MALLOC_SVELTE_FOR_LIBC32 := true
TARGET_USES_QTI_CAMERA_DEVICE := true

# Charger
BACKLIGHT_PATH := /sys/class/leds/lcd-backlight/brightness

# Crypto
TARGET_HW_DISK_ENCRYPTION := true

# Display
OVERRIDE_RS_DRIVER := libRSDriver_adreno.so
TARGET_USES_ION := true
TARGET_USES_GRALLOC1 := true
TARGET_USES_HWC2 := true
TARGET_SCREEN_DENSITY := 420

# DRM
TARGET_ENABLE_MEDIADRM_64 := true

# GPS
LOC_HIDL_VERSION := 3.0

# HIDL
DEVICE_FRAMEWORK_MANIFEST_FILE := $(DEVICE_PATH)/framework_manifest.xml
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/manifest.xml
DEVICE_MATRIX_FILE := $(DEVICE_PATH)/compatibility_matrix.xml
TARGET_FS_CONFIG_GEN += \
    $(DEVICE_PATH)/config.fs \
    $(DEVICE_PATH)/mot_aids.fs

# Init
TARGET_INIT_VENDOR_LIB := //$(DEVICE_PATH):libinit_msm8953
TARGET_RECOVERY_DEVICE_MODULES := libinit_msm8953

# Include
TARGET_SPECIFIC_HEADER_PATH := $(DEVICE_PATH)/include

# Kernel
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 androidboot.hardware=qcom user_debug=30 msm_rtb.filter=0x237
BOARD_KERNEL_CMDLINE += ehci-hcd.park=3 androidboot.bootdevice=7824900.sdhci lpm_levels.sleep_disabled=1 vmalloc=350M
BOARD_KERNEL_CMDLINE += loop.max_part=7
BOARD_KERNEL_CMDLINE += sched_enable_hmp=1 sched_enable_power_aware=1 app_setting.use_32bit_app_setting=1 kpti=1
#BOARD_KERNEL_CMDLINE += androidboot.selinux=permissive
BOARD_KERNEL_BASE := 0x80000000
BOARD_KERNEL_IMAGE_NAME := Image.gz
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_SEPARATED_DT := true
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x01000000 --tags_offset 0x00000100
TARGET_KERNEL_CLANG_COMPILE := true
TARGET_KERNEL_CONFIG := addison_defconfig
TARGET_KERNEL_SOURCE := kernel/motorola/msm8953
TARGET_KERNEL_RECOVERY_CONFIG := recovery_addison_defconfig

# Lights
TARGET_PROVIDES_LIBLIGHT := true

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216        #    16384 * 1024 mmcblk0p37
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE := 268435456      #   262144 * 1024 mmcblk0p52
BOARD_FLASH_BLOCK_SIZE := 131072                  # (BOARD_KERNEL_PAGESIZE * 64)
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16879616    #    16484 * 1024 mmcblk0p38
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 4294967296    #  4194304 * 1024 mmcblk0p53
BOARD_USERDATAIMAGE_PARTITION_SIZE := 25614597632 # 25014255 * 1024 mmcblk0p54
BOARD_PERSISTIMAGE_PARTITION_SIZE := 33554432     #    32768 * 1024 mmcblk0p30
BOARD_RAMDISK_USE_XZ := true
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_PARTITION_SIZE := 805306368
TARGET_COPY_OUT_VENDOR := vendor
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

# Power
TARGET_HAS_NO_WLAN_STATS := true
BOARD_POWER_CUSTOM_BOARD_LIB := libpower_8953

# Recovery
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/root/fstab.qcom

# Root
BOARD_ROOT_EXTRA_FOLDERS := persist
BOARD_ROOT_EXTRA_SYMLINKS := \
    /mnt/vendor/persist:/persist

# SELinux
include device/qcom/sepolicy-legacy-um/SEPolicy.mk
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy-mods/vendor
BOARD_PLAT_PRIVATE_SEPOLICY_DIR += $(DEVICE_PATH)/sepolicy-mods/private
BOARD_PLAT_PUBLIC_SEPOLICY_DIR += $(DEVICE_PATH)/sepolicy-mods/public
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor
BOARD_PLAT_PRIVATE_SEPOLICY_DIR += $(DEVICE_PATH)/sepolicy/private

# Sensors
BOARD_USES_MOT_SENSOR_HUB := true
MOT_AP_SENSOR_HW_REARPROX := true
MOT_SENSOR_HUB_FEATURE_CAMFSYNC := true
MOT_SENSOR_HUB_FEATURE_CHOPCHOP := true
MOT_SENSOR_HUB_FEATURE_GR := true
MOT_SENSOR_HUB_FEATURE_LA := true
MOT_SENSOR_HUB_FEATURE_LIFT := true
MOT_SENSOR_HUB_FEATURE_PEDO := true
MOT_SENSOR_HUB_FEATURE_ULTRASOUND := true
MOT_SENSOR_HUB_HW_BMI160 := true
MOT_SENSOR_HUB_HW_TYPE_L4 := true

# Treble
BOARD_VNDK_VERSION := current
PRODUCT_FULL_TREBLE_OVERRIDE := true

# Vendor SPL
VENDOR_SECURITY_PATCH := 2020-10-17

# Wi-Fi
BOARD_WLAN_DEVICE := qcwcn
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_qcwcn
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_qcwcn
PRODUCT_VENDOR_MOVE_ENABLED := true
WIFI_DRIVER_FW_PATH_AP := "ap"
WIFI_DRIVER_FW_PATH_STA := "sta"
WIFI_DRIVER_FW_PATH_P2P := "p2p"
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
WPA_SUPPLICANT_VERSION := VER_0_8_X
