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
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

AICP_ROOT="${MY_DIR}/../../.."

HELPER="${AICP_ROOT}/vendor/aicp/build/tools/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true
SECTION=
KANG=

while [ "$1" != "" ]; do
    case "$1" in
        -n | --no-cleanup )     CLEAN_VENDOR=false
                                ;;
        -k | --kang)            KANG="--kang"
                                ;;
        -s | --section )        shift
                                SECTION="$1"
                                CLEAN_VENDOR=false
                                ;;
        * )                     SRC="$1"
                                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC=adb
fi

function blob_fixup() {
    case "${1}" in
         
    # Fix fingerprint UHID
    vendor/etc/init/android.hardware.biometrics.fingerprint@2.1-service.rc)
        sed -i 's/group system input 9015/group system uhid input 9015/' "${2}"
        ;;

    vendor/lib/libmmcamera2_sensor_modules.so)
        sed -i "s|/system/etc/camera/|/vendor/etc/camera/|g" "${2}"
        ;;

    vendor/lib/libmmcamera_vstab_module.so | vendor/lib/libmmcamera2_stats_modules.so)
        patchelf --remove-needed libandroid.so "${2}"
        ;;

    vendor/lib/lib_mottof.so | vendor/lib/libmmcamera_vstab_module.so | vendor/lib/libjscore.so | vendor/lib/libmmcamera_ppeiscore.so | vendor/lib/libmmcamera2_stats_modules.so)
        sed -i "s/libgui/libwui/" "${2}"
        ;;
            
    vendor/lib/libcamerabgprocservice.so)
        patchelf --remove-needed libcamera_client.so "${2}"
        ;;
    
    # Patch libcutils dep into audio HAL
    vendor/lib/hw/audio.primary.msm8953.so)
        patchelf --replace-needed "libcutils.so" "libprocessgroup.so" "${2}"
        ;;

    vendor/lib/hw/camera.msm8953.so)
        sed -i "s|service.bootanim.exit|service.bootanim.hold|g" "${2}"
        ;;

    vendor/lib/libzaf_core.so)
        sed -i "s|/system/etc/zaf|/vendor/etc/zaf|g" "${2}"
        ;;

    esac
}

# Initialize the helper
setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

extract "${MY_DIR}/proprietary-files_nash.txt" "${SRC}" ${KANG} --section "${SECTION}"

"${MY_DIR}/setup-makefiles.sh"