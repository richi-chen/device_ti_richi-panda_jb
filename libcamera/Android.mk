LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/android-api.mk

include $(CLEAR_VARS)
# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_C_INCLUDES += $(LOCAL_PATH)\
	external/jpeg \
	external/jhead

LOCAL_SRC_FILES:= \
    SecCamera.cpp \
    SecCameraHWInterface.cpp \
    Encoder_jpeg.cpp

LOCAL_SHARED_LIBRARIES:= libutils \
			libcutils \
			libbinder \
			liblog \
			libcamera_client \
			libhardware \
			libjpeg \
			libexif

LOCAL_MODULE := camera.omap4

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += $(ANDROID_API_CFLAGS)
ifeq ($(MARVELL_SOFTWARE_ENCODER),true)
LOCAL_CFLAGS += -DMARVELL_SOFTWARE_ENCODER
endif

$(clear-android-api-vars)

include $(BUILD_SHARED_LIBRARY)
