LOCAL_PATH:= $(call my-dir)
     
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := ../../kernel/include
LOCAL_SRC_FILES := src/spi-config.c
LOCAL_MODULE := spi-config
LOCAL_CPPFLAGS += -DANDROID
LOCAL_CFLAGS += -DVERSION
include $(BUILD_EXECUTABLE)
     
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := ../../kernel/include
LOCAL_SRC_FILES := src/spi-pipe.c
LOCAL_MODULE := spi-pipe
LOCAL_CPPFLAGS += -DANDROID
LOCAL_CFLAGS += -DVERSION
include $(BUILD_EXECUTABLE)  

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := ../../kernel/include
LOCAL_SRC_FILES := src/spidev_test.c
LOCAL_MODULE := spi-test
LOCAL_CPPFLAGS += -DANDROID
LOCAL_CFLAGS += -DVERSION
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := ../../kernel/include
LOCAL_SRC_FILES := src/spidev_fdx.c
LOCAL_MODULE := spi-fdx
LOCAL_CPPFLAGS += -DANDROID
LOCAL_CFLAGS += -DVERSION
include $(BUILD_EXECUTABLE)
