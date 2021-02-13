#!/bin/bash
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
#

set -e

DEVICE=albus
VENDOR=motorola

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$MY_DIR" ]]; then MY_DIR="$PWD"; fi

LINEAGE_ROOT="$MY_DIR"/../../..

HELPER="$LINEAGE_ROOT"/vendor/lineage/build/tools/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
. "$HELPER"

while [ "$1" != "" ]; do
    case $1 in
        -n | --no-cleanup )     CLEAN_VENDOR=false
                                ;;
        -s | --section )        shift
                                SECTION=$1
                                CLEAN_VENDOR=false
                                ;;
        * )                     SRC=$1
                                ;;
    esac
    shift
done

if [ -z "$SRC" ]; then
    SRC=adb
fi

# Initialize the helper
setup_vendor "$DEVICE" "$VENDOR" "$LINEAGE_ROOT" false "$CLEAN_VENDOR"

extract "$MY_DIR"/proprietary-files.txt "$SRC" "$SECTION"

BLOB_ROOT="$LINEAGE_ROOT"/vendor/"$VENDOR"/"$DEVICE"/proprietary

vendor/lib/hw/camera.msm8996.so)
    sed -i "s|service.bootanim.exit|service.bootanim.hold|g" "${2}"
    ;;

readonly MMCAMERA=(
   vendor/lib/libmmcamera_vstab_module.so
   vendor/lib/libmmcamera2_stats_modules.so
)

for i in "${MMCAMERA[@]}"; do
  patchelf --remove-needed libandroid.so "$BLOB_ROOT"/${i}
done

# Load camera configs from vendor
CAMERA2_SENSOR_MODULES="$BLOB_ROOT"/vendor/lib/libmmcamera2_sensor_modules.so
sed -i "s|/system/etc/camera/|/vendor/etc/camera/|g" "$CAMERA2_SENSOR_MODULES"

PROC_SERVICE="$BLOB_ROOT"/vendor/lib/libcamerabgprocservice.so
patchelf --remove-needed libcamera_client.so "$PROC_SERVICE"

readonly LIBWUI_FIXUP=(
   vendor/lib/libmmcamera_vstab_module.so
   vendor/lib/libmmcamera_ppeiscore.so
   vendor/lib/libmmcamera2_stats_modules.so
   vendor/lib/libjscore.so
   vendor/lib/lib_mottof.so
)

for i in "${LIBWUI_FIXUP[@]}"; do
  sed -i "s/libgui/libwui/" "$BLOB_ROOT"/${i}
done

# Load ZAF configs from vendor
ZAF_CORE="$BLOB_ROOT"/vendor/lib/libzaf_core.so
sed -i "s|/system/etc/zaf|/vendor/etc/zaf|g" "$ZAF_CORE"


# Load camera metadata shim
CAMERAHAL="$BLOB_ROOT"/vendor/lib/hw/camera.msm8953.so
patchelf --replace-needed libcamera_client.so libcamera_metadata_helper.so "$CAMERAHAL"

# Patch libcutils dep into audio HAL
vendor/lib/hw/audio.primary.msm8953.so)
    patchelf --replace-needed "libcutils.so" "libprocessgroup.so" "${2}"
    ;;

"$MY_DIR"/setup-makefiles.sh
