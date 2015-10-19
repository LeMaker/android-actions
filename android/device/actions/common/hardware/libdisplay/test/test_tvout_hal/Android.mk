LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	src/tvout_display_test.cpp \
	src/DisplayParameters.cpp \
	main.cpp 
	
base := $(LOCAL_PATH)/../../frameworks
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/inc \
	$(base)/include
	
LOCAL_SHARED_LIBRARIES := \
	libandroid_runtime \
	libnativehelper \
	libcutils \
	libutils \
	libbinder \
	libhardware

#-----     整理增加了一个编译宏 AVOID_COMPILE_UNSTABLE_CODE 	
#LOCAL_CFLAGS:=-DAVOID_COMPILE_UNSTABLE_CODE

#---  可以定义一些编译c++相关的option，如果LOCAL_CPPFLAGS定义了与LOCAL_CFLAGS重复的option
# LOCAL_CPPFLAGS中的配置会覆盖LOCAL_CFLAGS
#LOCAL_CPPFLAGS:= 
#-- 定义c++文件的后缀，例如.cpp
LOCAL_CPP_EXTENSION :=.cpp

# 指定链接器ld的参数
LOCAL_LDFLAGS :=
#指定额外的链接库，如LOCAL_LDLIBS += -lcurses -lpthread	
LOCAL_LDLIBS := 

#指定c++编译器的名字,arm-linux-android-androideabi-gcc是默认的编译器
#LOCAL_CXX :=$(TOP)/prebuilt/linux-x86/toolchain/arm-linux-androideabi-4.4.x/bin/arm-linux-androideabi-gcc
#LOCAL_CXX :=$(TOP)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-gcc

	
LOCAL_MODULE:= test_tvout_hal
LOCAL_MODULE_TAGS := tests
#---------对于rootfs中的一些可执行文件，需要时静态链接的，本例如何想编译成静态链接
#需要同时打开 LOCAL_FORCE_STATIC_EXECUTABLE, 及LOCAL_STATIC_LIBRARIES
#LOCAL_FORCE_STATIC_EXECUTABLE:=true
#LOCAL_STATIC_LIBRARIES :=  libc libutils


#默认情况下，test_tvout_display安装到system/bin目录下，这里希望system根目录下
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/data
LOCAL_PRELINK_MODULE := false
include $(BUILD_EXECUTABLE)
