LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=               \
    id3_helper.cpp                \
    ape_id3parse.cpp              \
    mp3_id3parse.cpp              \
    ogg_id3parse.cpp              \
    wma_id3parse.cpp              \
    id3fileIO.cpp                 \
    flac_id3parse.cpp             \
    
LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include \
        $(TOP)/external/icu/icu4c/source/common \
        $(call include-path-for, alsp)
        
LOCAL_MODULE:= libid3parser
include $(BUILD_STATIC_LIBRARY)
