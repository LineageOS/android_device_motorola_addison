#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

DEVICE=addison
VENDOR=motorola

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

KANG=
SECTION=

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

    # Fix xml version
    product/etc/permissions/vendor.qti.hardware.data.connection-V1.0-java.xml | product/etc/permissions/vendor.qti.hardware.data.connection-V1.1-java.xml)
        sed -i 's|xml version="2.0"|xml version="1.0"|g' "${2}"
        ;;

    # Load wrapped shim
    vendor/lib64/libmdmcutback.so)
        for LIBQSAP_SHIM in $(grep -L "libqsapshim.so" "${2}"); do
            "${PATCHELF}" --add-needed "libqsapshim.so" "$LIBQSAP_SHIM"
        done
        ;;

    # Fix missing symbols
    vendor/lib64/libril-qc-qmi-1.so)
        for LIBCUTILS_SHIM in $(grep -L "libcutils_shim.so" "${2}"); do
            "${PATCHELF}" --add-needed "libcutils_shim.so" "$LIBCUTILS_SHIM"
        done
        ;;

    vendor/lib/libwvhidl.so)
        "${PATCHELF}" --replace-needed "libprotobuf-cpp-lite.so" "libprotobuf-cpp-lite-v28.so" "${2}"
        ;;

    # Fix thermal engine config path
    vendor/bin/thermal-engine)
        sed -i "s|/system/etc/thermal|/vendor/etc/thermal|g" "${2}"
        ;;

    #libwui patch
    vendor/lib/libmot_gpu_mapper.so)
        sed -i "s/libgui/libwui/" "${2}"
        ;;

    esac
}
# Initialize the helper
setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

extract "${MY_DIR}/proprietary-files.txt" "${SRC}" ${KANG} --section "${SECTION}"


"${MY_DIR}/setup-makefiles.sh"
