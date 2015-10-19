#ifndef _BASE_V4L2_MODULE_H_
#define _BASE_V4L2_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ACT_OMX_IVCommon.h"
#include "omx_camera.h"
    //#include <videodev2.h>

#define MAX_MODULE_NUM 0x2

#define MODULE_NAME_0 "/dev/video0"
#define MODULE_NAME_1 "/dev/video1"
#define MODULE_NAME_3 "/dev/video3"

typedef enum {
    MODE_NONE = -1,
    MOD_YUV,
    MOD_RAW,
} MODULE_TYPE;

int camera_module_query(camera_module_info_t *mode_info, int id, int uvcmode);
void camera_module_release();

typedef struct CAMERA_MODULETYPE
{
    /** The size of this structure, in bytes. */
    unsigned int nSize;

    /** Module Handle */
    int pModuleHandle;
    /** pModulePrivate is a pointer to the Module private data area. */
    void *pModulePrivate;

    /** pApplicationPrivate is a pointer that is a parameter to application private value */
    void *pApplicationPrivate;
    int (*mod_open)(void *hComponent, int module_id, int uvcmode);
    int (*mod_close)(void *hComponent);
    int (*mod_set_res)(void *hComponent, int w, int h, int fmt);
    int (*mod_get_res)(void *hComponent, int *w, int *h, int *fmt);
    int (*mod_set_crop)(void *hComponent, int x, int y, int w, int h);
    int (*mod_get_crop)(void *hComponent, int *x, int *y, int *w, int *h);
    int (*mod_set_framerate)(void *hComponent, int framerate_Q16);
    int (*mod_get_framerate)(void *hComponent, int *framerate_Q16);
    int (*mod_set_ctl)(void *hComponent, int id, int value);
    int (*mod_get_ctl)(void *hComponent, int id, int *value);
    int (*mod_requestbuf)(void *hComponent, unsigned int *num_bufs);
    int (*mod_querybuf)(void *hComponent,  int index, int buflen, unsigned long userptr, unsigned long resver);
    int (*mod_qbuf)(void *hComponent, int index, int buflen, unsigned long userptr, unsigned long resver);
    int (*mod_dqbuf)(void *hComponent, int *index, unsigned long *userptr, unsigned long *resver, long long *timestamp);
    int (*mod_streamon)(void *hComponent);
    int (*mod_streamoff)(void *hComponent);
    int (*mod_queryctl)(void *hComponent, int id, int *min_v, int *max_v, int *defalt_v, int *step, int *flg);
    int (*mod_set_mode)(void *hComponent, int mode);
} CAMERA_MODULETYPE;

int Camera_Module_constructer(CAMERA_MODULETYPE *camera_module_type);
int Camera_Module_destructer(CAMERA_MODULETYPE *camera_module_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
