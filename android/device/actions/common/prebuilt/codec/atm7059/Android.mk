LOCAL_PATH := $(call my-dir)

#ifneq ($(filter $(TARGET_PRODUCT),full_gs705a),)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libsub.so libbmp.a id_jpg.so libskia_actions_opt.a libdrm_algorithm.a
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := adAC3.so  adAPE.so adCOOK.so adDTS.so adMP3.so  adPCM.so adWMALSL.so adWMAPRO.so adWMASTD.so \
apAC3.so apAPE.so apDTS.so apMP3.so apRMA.so apWAV.so apWMA.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := avd_avi.so avd_flv.so  avd_mpg.so avd_ts.so avd_rm.so avd_wmv.so \
vd_flv1.so vd_h263.so vd_h264.so vd_hevc.so vd_hevc_91.so vd_mjpg.so vd_mpeg.so \
vd_msm4.so vd_rv34.so vd_vc1.so vd_vp6.so vd_vp8.so vd_vp9.so vd_wmv2.so vd_xvid.so \
libACT_EncAPI.so libACT_VceResize.so libACT_FD.so libAutoFocus.so libvde_core.so \
libbacklight.so libimg_en.so  libhdri.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ALTestPlatform.apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_PATH := $(TARGET_OUT_APPS)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_CERTIFICATE := PRESIGNED
include $(BUILD_PREBUILT)

#endif

