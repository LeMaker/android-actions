#ifndef _V4L2_CAMERA_H_
#define _V4L2_CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <pthread.h>        /* pthread_cond_*, pthread_mutex_* */

#include "omx_classmagic.h"
#include "base_v4l2_module.h"
#include "omx_camera.h"
#include "ispctl_adapter.h"

/**
 * @brief the base descriptor for a ST component
 */
CLASS(camera_direct_base_PrivateType)
#define camera_direct_base_PrivateType_FIELDS \
    /** @param a very nice parameter */ \
    CAMERA_MODULETYPE *StandModule; \
    /** @param a very nice parameter */ \
    char module_name[16]; \
    ispctl_handle_t *ipp_handle;\
    int id;\
    /* add muxlock here.....*/\
    int preview_width;\
    int preview_height;\
    int preview_fmt;\
    int preview_fps;\
    int capture_width;\
    int capture_height;\
    int capture_fmt;\
    int capture_fps;\
    int eprev_status;\
    int ecap_status;\
    int awb_type;\
    int ae_type;\
    int af_type;\
    int awb_lock;\
    int ae_lock;\
    int af_lock;\
    int bright_level;\
    int dns_level;\
    int contrast_level;\
    int sharp_level;\
    int flicker_type;\
    int mirror_type;\
    int color_fixtype;\
    int cmp_ev;\
    cam_mode_info_t mPreviewInfo;\
    int cur_preview_num;\
    int cur_capture_num;\
    int nPreviewSeq;\
    int nCaptureSeq;\
    int bStreaming;\
    int mwork_mode;\
    int nIndex;\
    int64_t bkTimeStamp;\
    int64_t nOffset_Time;\
    int64_t nCurrent_Time;\
    int bAdjustFlag;\
    pthread_mutex_t camera_mutex;\
    cam_mode_info_t mCaptureInfo;\
    int hdr_mode;\
    int cpnum;\
    int hdr_ev_info[3];\
    int hdr_ev_param[3];\
    int ev_auto;\
    int exp_time;\
    int exp_gain;\
    int focus_type;\
    int max_gain;\
    int min_gain;\
    int max_exp;\
    int min_exp;
ENDCLASS(camera_direct_base_PrivateType)

int camera_direct_base_Constructor(OMX_CAMERATYPE *omx_camera);
int camera_direct_base_Destructor(OMX_CAMERATYPE *omx_camera);

#ifdef __cplusplus
}
#endif

#endif
