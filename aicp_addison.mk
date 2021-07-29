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
$(call inherit-product, device/motorola/addison/full_addison.mk)


# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/product_launched_with_m.mk)
$(call inherit-product, vendor/aicp/config/common_full_phone.mk) 

# Boot animation
TARGET_SCREEN_WIDTH := 1080
TARGET_SCREEN_HEIGHT := 1920
TARGET_BOOTANIMATION_HALF_RES := true

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := addison
PRODUCT_NAME := aicp_addison
PRODUCT_BRAND := Motorola
PRODUCT_MODEL := Moto Z Play
PRODUCT_MANUFACTURER := Motorola
PRODUCT_RELEASE_NAME := addison

PRODUCT_GMS_CLIENTID_BASE := android-motorola

PRODUCT_ENFORCE_RRO_TARGETS := \
    framework-res

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="addison_retail-user 8.0.0 OPNS27.76-12-22-9 10 release-keys" \
    PRODUCT_NAME="Moto Z Play" \
    DEVICE_MAINTAINERS="marcost2"

BUILD_FINGERPRINT := "motorola/addison_retail/addison:8.0.0/OPNS27.76-12-22-9/10:user/release-keys"
    # for specific
$(call inherit-product, vendor/motorola/addison/addison-vendor.mk)
