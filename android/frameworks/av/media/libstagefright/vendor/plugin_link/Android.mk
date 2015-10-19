LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    link_decoder.c
    
LOCAL_MODULE:= liblink_decoder
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    link_parser.c

LOCAL_MODULE:= liblink_parser
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    link_encoder.c

LOCAL_MODULE:= liblink_encoder
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    link_video_decoder.c

LOCAL_MODULE:= liblink_video_decoder
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    link_video_parser.c

LOCAL_MODULE:= liblink_video_parser
include $(BUILD_STATIC_LIBRARY)
