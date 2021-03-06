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
include device/ti/blaze_tablet/Config.mk


DEVICE_PACKAGE_OVERLAYS := device/ti/richi-panda/overlay

PRODUCT_PACKAGES := \
    ti_omap4_ducati_bins
#    libOMX_Core \
#    libOMX.TI.DUCATI1.VIDEO.DECODER

# Tiler
#PRODUCT_PACKAGES += \
#    libtimemmgr

#Lib Skia test
#PRODUCT_PACKAGES += \
#    SkLibTiJpeg_Test

# Camera
ifdef OMAP_ENHANCEMENT_CPCAM
PRODUCT_PACKAGES += \
    libcpcam_jni \
    com.ti.omap.android.cpcam

PRODUCT_COPY_FILES += \
    hardware/ti/omap4xxx/cpcam/com.ti.omap.android.cpcam.xml:system/etc/permissions/com.ti.omap.android.cpcam.xml
endif

PRODUCT_PACKAGES += \
    CameraOMAP \
    Camera \
    camera_test

# VTC test
#PRODUCT_PACKAGES += \
#    VTCTestApp \
#    VTCLoopbackTest \
#    CameraHardwareInterfaceTest

ifeq ($(TARGET_PREBUILT_KERNEL),)
$(call inherit-product-if-exists, device/ti/richi-panda/kernel.mk)
LOCAL_KERNEL := device/ti/richi-panda/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

#to flow down to ti-wpan-products.mk
BLUETI_ENHANCEMENT := true

#Need to revisit the fastboot copy files
PRODUCT_COPY_FILES += \
	$(LOCAL_KERNEL):kernel \
	device/ti/richi-panda/init.omap4pandaboard.rc:root/init.omap4pandaboard.rc \
	device/ti/richi-panda/init.omap4pandaboard.usb.rc:root/init.omap4pandaboard.usb.rc \
	device/ti/richi-panda/ueventd.omap4pandaboard.rc:root/ueventd.omap4pandaboard.rc \
	device/ti/richi-panda/bootanimation.zip:/system/media/bootanimation.zip \
	device/ti/richi-panda/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
	device/ti/richi-panda/audio.aosp/audio_policy.conf:system/etc/audio_policy.conf \
	device/ti/richi-panda/media_profiles.xml:system/etc/media_profiles.xml \
	device/ti/richi-panda/media_codecs.xml:system/etc/media_codecs.xml \
	device/ti/richi-panda/cyttsp4-i2c.idc:system/usr/idc/cyttsp4-i2c.idc \
	device/ti/richi-panda/wallpaper_info.xml:data/system/wallpaper_info.xml \
	frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
	device/ti/richi-panda/Vendor_0471_Product_0815.kl:system/usr/keylayout/Vendor_0471_Product_0815.kl \
	device/ti/richi-panda/Vendor_0609_Product_031d.kl:system/usr/keylayout/Vendor_0609_Product_031d.kl
#	frameworks/base/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
#	frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
#	frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
#	device/ti/blaze_tablet/qtouch-touchscreen.idc:system/usr/idc/qtouch-touchscreen.idc \
#	device/ti/blaze_tablet/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
#	device/ti/blaze_tablet/twl6030_pwrbutton.kl:system/usr/keylayout/twl6030_pwrbutton.kl \
#	device/ti/common-open/audio/audio_policy.conf:system/etc/audio_policy.conf \

# to mount the external storage (sdcard)
PRODUCT_COPY_FILES += \
	device/ti/richi-panda/vold.fstab:system/etc/vold.fstab

#PRODUCT_PACKAGES += \
#	lights.blaze_tablet

#Remove this as it freezes at boot. Will re-enable once fixed
#PRODUCT_PACKAGES += \
#	sensors.blaze_tablet \
#	sensor.test

PRODUCT_PACKAGES += \
       boardidentity \
       libboardidentity \
       libboard_idJNI \
       Board_id

PRODUCT_PACKAGES += \
	com.android.future.usb.accessory

# Live Wallpapers
PRODUCT_PACKAGES += \
        LiveWallpapers \
        LiveWallpapersPicker \
        MagicSmokeWallpapers \
        VisualizationWallpapers \
        librs_jni

#PRODUCT_PACKAGES += \
#	VideoEditorGoogle

PRODUCT_PROPERTY_OVERRIDES := \
	hwui.render_dirty_regions=false

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=160

PRODUCT_PROPERTY_OVERRIDES += \
	persist.hwc.mirroring.region=0:0:1280:720

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
	librs_jni \
	com.android.future.usb.accessory

PRODUCT_PACKAGES += \
	audio.primary.panda \
	audio.a2dp.default \
	libaudioutils

# WI-Fi
PRODUCT_PACKAGES += \
	dhcpcd.conf \
	hostapd.conf \
	wifical.sh \
	wilink7.sh \
	TQS_D_1.7.ini \
	TQS_D_1.7_127x.ini \
	crda \
	regulatory.bin \
	calibrator

PRODUCT_PROPERTY_OVERRIDES += \
	wifi.interface=wlan0

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs

# Audio HAL module
#PRODUCT_PACKAGES += audio.primary.omap4
#PRODUCT_PACKAGES += audio.hdmi.omap4

# Dolby DD+ Decoder
ifdef OMAP_ENHANCEMENT
PRODUCT_PACKAGES += \
        libstagefright_soft_ddpdec
endif

# Audioout libs
#PRODUCT_PACKAGES += libaudioutils

# for bugmailer
PRODUCT_PACKAGES += send_bug
PRODUCT_COPY_FILES += \
	system/extras/bugmailer/bugmailer.sh:system/bin/bugmailer.sh \
	system/extras/bugmailer/send_bug:system/bin/send_bug

# tinyalsa utils
PRODUCT_PACKAGES += \
	tinymix \
	tinyplay \
	tinycap

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
    device/ti/richi-panda/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
        frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
        frameworks/base/nfc-extras/com.android.nfc_extras.xml:system/etc/permissions/com.android.nfcextras.xml \
        frameworks/native/data/etc/com.nxp.mifare.xml:system/etc/permissions/com.nxp.mifare.xml \
        device/ti/omap5sevm/nfcee_access.xml:system/etc/nfcee_access.xml \
#	frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
#	frameworks/base/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
#	frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
#    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
#    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
#    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
#    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
#    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \

# BlueZ a2dp Audio HAL module
#PRODUCT_PACKAGES += audio.a2dp.default

# BlueZ test tools
PRODUCT_PACKAGES += \
	hciconfig \
	hcitool

# SMC components for secure services like crypto, secure storage
PRODUCT_PACKAGES += \
        smc_pa.ift \
        smc_normal_world_android_cfg.ini \
        libsmapi.so \
        libtf_crypto_sst.so \
        libtfsw_jce_provider.so \
        tfsw_jce_provider.jar \
        tfctrl

# Enable AAC 5.1 decode (decoder)
PRODUCT_PROPERTY_OVERRIDES += \
	media.aac_51_output_enabled=true

# Linaro extra
include $(LOCAL_PATH)/ZeroXBenchmark.mk
PRODUCT_PACKAGES += \
    	ZeroXBenchmark \
    	$(ZEROXBENCHMARK_NATIVE_APPS) \
		libglmark2-android \
		GLMark2 \
		LinaroWallpaper \
		powertop \
		gatord

# external packages
PRODUCT_PACKAGES += \
	AndroidTerm \
	FileManager \
	Superuser \
	replicaisland

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)
$(call inherit-product, hardware/ti/omap4xxx/omap4.mk)
$(call inherit-product-if-exists, vendor/ti/proprietary/omap4/ti-omap4-vendor.mk)
$(call inherit-product-if-exists, hardware/ti/wpan/ti-wpan-products.mk)
$(call inherit-product-if-exists, device/ti/proprietary-open/omap4/ti-omap4-vendor.mk)
#$(call inherit-product-if-exists, vendor/ti/panda/device-vendor.mk)
$(call inherit-product, device/ti/richi-panda/proprietary-open/install-binaries.mk)
$(call inherit-product, device/ti/proprietary-open/wl12xx/wlan/wl12xx-wlan-fw-products.mk)
$(call inherit-product-if-exists, device/ti/common-open/s3d/s3d-products.mk)
#$(call inherit-product-if-exists, device/ti/proprietary-open/omap4/ducati-blaze_tablet.mk)
#$(call inherit-product-if-exists, device/ti/proprietary-open/omap4/dsp_fw.mk)
$(call inherit-product-if-exists, device/ti/richi-panda/proprietary-open/ducati-full_richi_panda.mk)
$(call inherit-product-if-exists, hardware/ti/dvp/dvp-products.mk)
$(call inherit-product-if-exists, hardware/ti/arx/arx-products.mk)

# clear OMAP_ENHANCEMENT variables
$(call ti-clear-vars)
