LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	./dosfstools-3.0.12/boot.c\
	./dosfstools-3.0.12/check.c\
	./dosfstools-3.0.12/common.c\
	./dosfstools-3.0.12/fat.c\
	./dosfstools-3.0.12/file.c\
	./dosfstools-3.0.12/io.c\
	./dosfstools-3.0.12/lfn.c\
	./dosfstools-3.0.12/dosfslabel.c\

	
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE:= dosfslabel

include $(BUILD_EXECUTABLE)
