LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ksu_susfs
LOCAL_SRC_FILES := main.c
#LOCAL_LDFLAGS := -static

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS += -llog -lz

include $(BUILD_EXECUTABLE)
