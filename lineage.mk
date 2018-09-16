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

# Inherit some common lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Boot animation
TARGET_SCREEN_WIDTH := 1080
TARGET_SCREEN_HEIGHT := 1920

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := albus
PRODUCT_NAME := lineage_albus
PRODUCT_BRAND := motorola
PRODUCT_MODEL := Moto Z2 Play
PRODUCT_MANUFACTURER := motorola
PRODUCT_RELEASE_NAME := albus

PRODUCT_ENFORCE_RRO_TARGETS := \
    framework-res

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=albus \
    PRIVATE_BUILD_DESC="albus-user 8.0.0 OPSS27.76-12-25-3 4 release-keys"

BUILD_FINGERPRINT := motorola/albus/albus:8.0.0/OPSS27.76-12-25-3/4:user/release-keys
