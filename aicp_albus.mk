#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017 The LineageOS Project
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


# Inherit from those products. Most specific first.
$(call inherit-product, device/motorola/albus/full_albus.mk)


# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/product_launched_with_n_mr1.mk)
$(call inherit-product, vendor/aicp/config/common_full_phone.mk) 

# Boot animation
TARGET_SCREEN_WIDTH := 1080
TARGET_SCREEN_HEIGHT := 1920
TARGET_BOOTANIMATION_HALF_RES := true

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := albus
PRODUCT_NAME := aicp_albus
PRODUCT_BRAND := Motorola
PRODUCT_MODEL := Moto Z2 Play
PRODUCT_MANUFACTURER := Motorola
PRODUCT_RELEASE_NAME := albus

PRODUCT_GMS_CLIENTID_BASE := android-motorola

PRODUCT_ENFORCE_RRO_TARGETS := \
    framework-res

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="albus_retail-user 9 PPS29.133-30 ab8b4 release-keys" \
    PRODUCT_NAME="Moto Z2 Play" \
    DEVICE_MAINTAINERS="marcost2"

BUILD_FINGERPRINT := "motorola/albus_retail/albus:9/PPS29.133-30/ab8b4:user/release-keys"
    # for specific
$(call inherit-product, vendor/motorola/albus/albus-vendor.mk)
