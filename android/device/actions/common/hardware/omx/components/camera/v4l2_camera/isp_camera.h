#ifndef _ISP_CAMERA_H_
#define _ISP_CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <pthread.h>        /* pthread_cond_*, pthread_mutex_* */
#include <linux/videodev2.h>
#include "omx_classmagic.h"
#include "base_v4l2_module.h"
#include "ispctl_adapter.h"
#include "isp_ctl.h"
#include "omx_camera.h"


/**
 * @brief the base descriptor for a ST component
 */
CLASS(camera_isp_base_PrivateType)
#define camera_isp_base_PrivateType_FIELDS \
    /** @param a very nice parameter */ \
    CAMERA_MODULETYPE *StandModule; \
    /** @param a very nice parameter */ \
    char module_name[8]; \
    /** @param create by client,From ISP_OPEN */ \
    ispctl_handle_t *ipp_handle; \
    /** @param 0 or 1,for camera ID */ \
    int id; \
    /* add muxlock here.....*/\
    int preview_width;\
    int preview_height;\
    int preview_fmt;\
    int preview_fps;\
    int preview_addr_offset;\
    int capture_width;\
    int capture_height;\
    int capture_fmt;\
    int capture_fps;\
    int capture_addr_offset;\
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
    cam_mode_info_t mPreviewInfo;\
    int cur_preview_num;\
    int cur_capture_num;\
    pthread_mutex_t camera_mutex;\
    cam_mode_info_t mCaptureInfo;\
    int bStreaming;\
    int mwork_mode;\
    int mmodle_type;\
    int nIndex;\
    act_af_status_info_t af_status;\
    act_af_region_info_t af_region; \
    int color_fixtype;\
    int64_t nPreviewSeq;\
    int64_t nCaptureSeq;\
    int64_t bkTimeStamp;\
    int64_t nOffset_Time;\
    int64_t nCurrent_Time;\
    int bAdjustFlag;\
    unsigned int cpnum;\
    unsigned int exp_time;\
    unsigned int exp_gain;\
    int preview_idx;\
    int capture_idx;\
    int ev_ratio;\
    int hdr_ev_info[3];\
    int hdr_exp[3];\
    int hdr_gain[3];\
    int hdr_mode;
ENDCLASS(camera_isp_base_PrivateType)

int camera_isp_base_Constructor(OMX_CAMERATYPE *omx_camera);
int camera_isp_base_Destructor(OMX_CAMERATYPE *omx_camera);

#ifdef __cplusplus
}
#endif

#endif
