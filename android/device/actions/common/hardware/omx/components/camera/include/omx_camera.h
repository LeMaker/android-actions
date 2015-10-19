#ifndef _OMX_CAMERA_H_
#define _OMX_CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ACT_OMX_IVCommon.h"
#include "omx_callback.h"


#define MX_BUF_NUM 16
#define OMX_COM_FORMAT_MJPG 0x7f100000

enum
{
    RAW8 = 0x0,
    RAW10_12 = 0x2,
    RAW_3D_NORMAL = 0x3,
    RAW_3D_MIPI = 0x5,
    YUV,
};

enum
{
    OMX_PREVIEW_MODE,
    OMX_CAPTURE_MODE,
    OMX_PREVIEW_CAP,
    OMX_CAPTURE_PREP,
};

enum
{
    OWN_BY_CLIENT,
    OWN_BY_MODULE,
    OWN_BY_CLITOMOD,
};
enum
{
    CAM_IDLE,
    CAM_PREVIEW_RUNNING,
    CAM_PREVIEW_PAUSE,
    CAM_CAPTURE_RUNNING,
    CAM_UNLOADED,
    CAM_PREVIEW_READY,
};

typedef struct
{
    unsigned long pPhyAddr;
    unsigned long pVirAddr;
} cam_stat_addr_t;

typedef struct
{
    int nw;
    int nh;
    int nfps;
    int nbufNum;
    int pBufIndex[MX_BUF_NUM];
    int pBufOwner[MX_BUF_NUM];
    unsigned long pBufPhy[MX_BUF_NUM];
    unsigned long pBufVir[MX_BUF_NUM];
    unsigned long pBufPhy_Stat[MX_BUF_NUM];
    unsigned long pBufVir_Stat[MX_BUF_NUM];
    int64_t nTimeStamp[MX_BUF_NUM];
    unsigned int nPreVSize;
    /* Buffer queued Flag */
    int pBufFlag[MX_BUF_NUM];
} cam_mode_info_t;

typedef enum
{
    CAM_GET_SENSOR_ID,
    CAM_SET_WB_TYPE,/*OMX_WHITEBALCONTROLTYPE*/
    CAM_SET_EXP_TYPE,/*OMX_EXPOSURECONTROLTYPE*/
    CAM_SET_MET_AREA,
    CAM_SET_AF_TYPE,/*OMX_IMAGE_FOCUSCONTROLTYPE*/
    CAM_SET_BRIGHTNESS_LEVEL,
    CAM_SET_CONSTRAST_LEVEL,
    CAM_SET_DNS_LEVEL,
    CAM_SET_SHARP_LEVEL,
    CAM_SET_AF_REGION,
    CAM_SET_FOCUS_MANUL,
    CAM_SET_FLICKER_TYPE,/*OMX_COMMONFLICKERCANCELTYPE*/
    CAM_SET_OPTIC_ZOOM,/*OMX_CONFIG_SCALEFACTORTYPE*/
    CAM_SET_BLIT_CMP,
    CAM_SET_COLORFIX,/*OMX_IMAGEFILTERTYPE*/
    CAM_SET_FLIP,/*OMX_MIRRORTYPE*/
    CAM_GET_FOCUS_DIS,/*OMX_ACT_CONFIG_FOCUSDISTANCETYPE*/
    CAM_GET_FOCUS_STATUS,
    CAM_SET_COLORMTR,
    CAM_SET_GAMMA,
    CAM_SET_STA,
    CAM_SET_AWBLOCK,
    CAM_SET_AELOCK,
    CAM_SET_AFLOCK,
    CAM_SET_COLORFIX_MANUAL,/*OMX_IMAGEFILTERTYPE*/
    CAM_SET_EV,
    CAM_GET_SHUTTER,
    CAM_GET_GAIN,
    CAM_SET_FLASH_TYPE,
    CAM_SET_FLASH_STROBE_MODE,
    CAM_SET_HDR,
    CAM_GET_EVS,
    CAM_SET_HDR_EV,
} camera_cmd_t;

typedef struct
{
    int support_num;
    int supportType[3];
    OMX_ACT_CAPTYPE *supportParam[3];
    int mDgain_th[3];
} camera_module_info_t;

typedef struct OMX_CAMERATYPE
{
    /** The size of this structure, in bytes. */
    unsigned int nSize;

    /** pModulePrivate is a pointer to the Module private data area. */
    void *pCameraPrivate;

    /** pIppHnale is a pointer to the ISP private data area. */
    void *pIppHnale;

    /** pApplicationPrivate is a pointer that is a parameter to application private value */
    void *pApplicationPrivate;

    /* */
    wdog_cb_t omx_wdog_cb;

    int (*omx_camera_open)(void *hComponent, int id, int mode_type, int uvcmode);
    int (*omx_camera_close)(void *hComponent);
    int (*omx_camera_setres)(void *hComponent, int w, int h, int fmt, int work_mode);
    int (*omx_camera_getres)(void *hComponent, int *w, int *h, int *fmt, int work_mode);
    int (*omx_camera_setzoom)(void *hComponent, int x, int y, int w, int h, int work_mode);    // not used yet
    int (*omx_camera_getzoom)(void *hComponent, int *x, int *y, int *w, int *h, int work_mode);    // not used yet
    int (*omx_camera_setfps)(void *hComponent, int framerate_Q16, int work_mode);
    int (*omx_camera_getfps)(void *hComponent, int *framerate_Q16, int work_mode);
    int (*omx_camera_setctl)(void *hComponent, unsigned int id, unsigned long value, int work_mode);
    int (*omx_camera_getctl)(void *hComponent, unsigned int id, unsigned int *value, int work_mode);
    int (*omx_camera_requestbuf)(void *hComponent, unsigned int *num_bufs, int work_mode);
    int (*omx_camera_querybuf)(void *hComponent,  int index, int buflen, unsigned long userptr, unsigned long resver, int work_mode);
    int (*omx_camera_qbuf)(void *hComponent, int index, int buflen, unsigned long userptr, unsigned long resver, int work_mode);
    int (*omx_camera_dqbuf)(void *hComponent, int *index, unsigned long *userptr, unsigned long *resver, long long *timestamp, int work_mode);
    int (*omx_camera_streamon)(void *hComponent, int work_mode);
    int (*omx_camera_streamoff)(void *hComponent, int work_mode);
    int (*omx_camera_setcb)(void *hComponent, wdog_cb_t *cb);
} OMX_CAMERATYPE;


#ifdef __cplusplus
}
#endif

#endif
