#
# Copyright (C) 2011 Texas Instruments Inc.
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

# define OMAP_ENHANCEMENT variables
include device/ti/richi-panda/Config.mk

# These two variables are set first, so they can be overridden
# by BoardConfigVendor.mk
BOARD_USES_GENERIC_AUDIO := true
USE_CAMERA_STUB := false

# Default values, possibly overridden by BoardConfigVendor.mk
TARGET_BOARD_INFO_FILE := device/ti/richi-panda/board-info.txt

OMAP_ENHANCEMENT_MULTIGPU := true

ENHANCED_DOMX := true
BLTSVILLE_ENHANCEMENT :=true
BOARD_USES_DVP := true
BOARD_USES_ARX := true
#USE_ITTIAM_AAC := true
# Use the non-open-source parts, if they're present
-include vendor/ti/panda/BoardConfigVendor.mk

# kernel config
KERNEL_CONFIG := richi-panda_defconfig

# TI config
CFG_FM_SERVICE_TI := yes
TI_OMAP4_CAMERAHAL_VARIANT := true

# TI's Bluetooth stack based on BlueZ
BLUETI_ENHANCEMENT := true

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH:= arm
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

BOARD_HAVE_BLUETOOTH := true
TARGET_NO_BOOTLOADER := true

BOARD_KERNEL_BASE := 0x80000000
# BOARD_KERNEL_CMDLINE

TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := omap4
TARGET_BOOTLOADER_BOARD_NAME := panda 

BOARD_EGL_CFG := device/ti/richi-panda/egl.cfg

#BOARD_USES_HGL := true
#BOARD_USES_OVERLAY := true
USE_OPENGL_RENDERER := true

# Recovery
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
TARGET_RECOVERY_UI_LIB := librecovery_ui_panda
# device-specific extensions to the updater binary
TARGET_RELEASETOOLS_EXTENSIONS := device/ti/richi-panda

BOARD_USES_SECURE_SERVICES := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
#BOARD_SYSTEMIMAGE_PARTITION_SIZE := 268435456
#BOARD_USERDATAIMAGE_PARTITION_SIZE := 536870912
BOARD_USERDATAIMAGE_PARTITION_SIZE := 7125073920
#BOARD_CACHEIMAGE_PARTITION_SIZE := 268435456
#BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 4096

#TARGET_PROVIDES_INIT_RC := true
#TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

#NFC
NFC_TI_DEVICE := true

# Connectivity - Wi-Fi
USES_TI_MAC80211 := true
ifdef USES_TI_MAC80211
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
WPA_SUPPLICANT_VERSION           := VER_0_8_X_TI
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_WLAN_DEVICE                := wl12xx_mac80211
BOARD_SOFTAP_DEVICE              := wl12xx_mac80211
WIFI_DRIVER_MODULE_PATH          := "/system/lib/modules/wl12xx_sdio.ko"
WIFI_DRIVER_MODULE_NAME          := "wl12xx_sdio"
WIFI_FIRMWARE_LOADER             := ""
COMMON_GLOBAL_CFLAGS += -DUSES_TI_MAC80211
endif

ifdef BLUETI_ENHANCEMENT
COMMON_GLOBAL_CFLAGS += -DBLUETI_ENHANCEMENT
endif
ifdef NFC_TI_DEVICE
COMMON_GLOBAL_CFLAGS += -DNFC_JNI_TI_DEVICE
endif
#Set 32 byte cache line to true
ARCH_ARM_HAVE_32_BYTE_CACHE_LINES := true

BOARD_LIB_DUMPSTATE := libdumpstate.panda

BOARD_VENDOR_TI_GPS_HARDWARE := omap4
BOARD_GPS_LIBRARIES := libgps


# Common device independent definitions
include device/ti/common-open/BoardConfig.mk
