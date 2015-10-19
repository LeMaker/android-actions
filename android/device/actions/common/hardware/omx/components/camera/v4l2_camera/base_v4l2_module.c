#ifdef __cplusplus
extern "C" {
#endif

#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>      /* sem_wait, sem_post */
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdarg.h>
#include <linux/videodev2.h>
#include "base_v4l2_module.h"
#include "OMX_IVCommon.h"
#include "isp_camera.h"
#include "watch_dog.h"
#include "log.h"


#ifndef V4L2_CID_EXPOSURE_COMP
#define V4L2_CID_EXPOSURE_COMP      (V4L2_CID_CAMERA_CLASS_BASE+21)
#endif
#define V4L2_CID_CAM_CV_MODE  _IOW('v', BASE_VIDIOC_PRIVATE + 0, int)


//#define _USEDEF_EXPEV_


typedef struct
{
    CAMERA_MODULETYPE *camera_module_type;
    void *wdog_handle;
    int id;
    int mCameraHandle;
    int mCameraStatus;
    int bstreamon;
    int bopened;
    int nBufferNumInDrv;
} base_module_privateType;

static int gbase_sensor_fd[3] = { -1, -1, -1};

int gMin_Preview_Width[2] = {0xffff, 0xffff};
int gMin_Preview_Height[2] = {0xffff, 0xffff};
int gSupport_Preview_1080P[2] = {0, 0};
int gSupport_Preview_720P[2] = {0, 0};
int gSupport_Preview_VGA[2] = {0, 0};
int gSupport_Preview_QVGA[2] = {0, 0};
int gSupport_Preview_CIF[2] = {0, 0};
int gSupport_Preview_QCIF[2] = {0, 0};
int gSupport_Preview_720x576[2] = {0, 0};
int gSupport_Preview_720x480[2] = {0, 0};


static int trans_v4l2fmt2omxfmt(int v4l2fmt)
{
    unsigned int omxfmt = -1;

    switch(v4l2fmt)
    {
    case V4L2_PIX_FMT_YUV420:
        omxfmt = OMX_COLOR_FormatYUV420Planar;
        break;

    case V4L2_PIX_FMT_YVU420:
        omxfmt = OMX_COLOR_FormatYVU420Planar;
        break;

    case V4L2_PIX_FMT_YUV422P:
        omxfmt = OMX_COLOR_FormatYUV422Planar;
        break;

    case V4L2_PIX_FMT_NV12:
        omxfmt = OMX_COLOR_FormatYUV420SemiPlanar;
        break;

    case V4L2_PIX_FMT_NV21:
        omxfmt = OMX_COLOR_FormatYVU420SemiPlanar;
        break;

    case V4L2_PIX_FMT_YUYV:
        omxfmt = OMX_COLOR_FormatYCbYCr;
        break;

    case V4L2_PIX_FMT_YVYU:
        omxfmt = OMX_COLOR_FormatYCrYCb;
        break;

    case V4L2_PIX_FMT_NV16:
        omxfmt = OMX_COLOR_FormatYUV422SemiPlanar;
        break;

    case V4L2_PIX_FMT_NV61:
        omxfmt = OMX_COLOR_FormatYVU422SemiPlanar;
        break;

    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
        omxfmt = OMX_COLOR_FormatRawBayer8bit;
        break;

    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SRGGB10:
        omxfmt = OMX_COLOR_FormatRawBayer10bit;
        break;
    }

    V4L2DBUG(V4L2DBUG_VERB, "omxfmt %x\n", omxfmt);
    return omxfmt;
}


static unsigned int trans_omxfmt2v4l2fmt(unsigned int omxfmt)
{
    unsigned int v4l2fmt = -1;

    switch(omxfmt)
    {
    case OMX_COLOR_FormatYUV420Planar:
        v4l2fmt = V4L2_PIX_FMT_YUV420;
        V4L2DBUG(V4L2DBUG_ERR, "OMX_COLOR_FormatYUV420Planar %x\n", v4l2fmt);
        break;

    case OMX_COLOR_FormatYVU420Planar:
        v4l2fmt = V4L2_PIX_FMT_YVU420;
        V4L2DBUG(V4L2DBUG_ERR, "OMX_COLOR_FormatYVU420Planar %x\n", v4l2fmt);
        break;

    case OMX_COLOR_FormatYCbYCr:
        v4l2fmt = V4L2_PIX_FMT_YUYV;
        V4L2DBUG(V4L2DBUG_ERR, "OMX_COLOR_FormatYCbYCr %x\n", v4l2fmt);
        break;

    case OMX_COLOR_FormatYCrYCb:
        v4l2fmt = V4L2_PIX_FMT_YVYU;
        V4L2DBUG(V4L2DBUG_ERR, "OMX_COLOR_FormatYCrYCb %x\n", v4l2fmt);
        break;

    case OMX_COLOR_FormatYUV422Planar:
        v4l2fmt = V4L2_PIX_FMT_YUV422P;
        break;

    case OMX_COLOR_FormatYUV420SemiPlanar:
        v4l2fmt = V4L2_PIX_FMT_NV12;
        V4L2DBUG(V4L2DBUG_ERR, "OMX_COLOR_FormatYUV420SemiPlanar %x\n", v4l2fmt);
        break;

    case OMX_COLOR_FormatYVU420SemiPlanar:
        v4l2fmt = V4L2_PIX_FMT_NV21;
        V4L2DBUG(V4L2DBUG_ERR, "OMX_COLOR_FormatYVU420SemiPlanar %x\n", v4l2fmt);
        break;

    case OMX_COLOR_FormatYUV422SemiPlanar:
        v4l2fmt = V4L2_PIX_FMT_NV16;
        break;

    case OMX_COLOR_FormatYVU422SemiPlanar:
        v4l2fmt = V4L2_PIX_FMT_NV61;
        break;

    case OMX_COLOR_FormatRawBayer8bit:
        v4l2fmt = V4L2_PIX_FMT_SBGGR8;
        break;

    case OMX_COLOR_FormatRawBayer10bit:
        v4l2fmt = V4L2_PIX_FMT_SBGGR10;
        break;
    }

    V4L2DBUG(V4L2DBUG_VERB, "v4l2fmt %x\n", v4l2fmt);
    return v4l2fmt;
}

typedef struct
{
    int id;
    int min_val;
    int max_val;
    int step;
    int default_val;
} _v4l2_id_type_t;

_v4l2_id_type_t isp_module_type[] =
{
    {V4L2_CID_BRIGHTNESS, -6, 6, 1, 0},
    {V4L2_CID_WHITE_BALANCE_TEMPERATURE, 0, 7, 1, 1},
    {V4L2_CID_POWER_LINE_FREQUENCY, 0, 2, 1, 0},
    {V4L2_CID_EXPOSURE, 0, 2, 1, 1},
};

int color_fix[] = {0, 3, 0x7f000001};


int v4l2_camera_open(void *handle, int id, int uvcmode)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    base_module_privateType *camera_type = (base_module_privateType *)camera_module_type->pModulePrivate;
    //camera_type->mCameraHandle = 0xf;
    //int fid = open("/sys/bus/platform/devices/camera_flag_device.0/front_back",O_WRONLY);
    int fid = open("/sys/dualcam/dualmod", O_WRONLY);
    char *back = "0";
    char *front = "1";
     if(fid > 0)
    {
		if(id == 0)
		{
			if(uvcmode == OMX_UVC_AS_REAR){
				if(gbase_sensor_fd[id] != -1)
		        {
		            camera_type->mCameraHandle = gbase_sensor_fd[id];		
		        }else{
		            camera_type->mCameraHandle = open(MODULE_NAME_3, O_RDWR);
		        }

			}else{
				write(fid, back, 1);
				close(fid);
				if(gbase_sensor_fd[id] != -1)
		        {
		            camera_type->mCameraHandle = gbase_sensor_fd[id];	
		        }else{
					camera_type->mCameraHandle = open(MODULE_NAME_0, O_RDWR);
		        }
			}
			
		}else{
			if(uvcmode == OMX_UVC_AS_FRONT){
				if(gbase_sensor_fd[id] != -1)
		        {
		            camera_type->mCameraHandle = gbase_sensor_fd[id];	
		        }else{
					camera_type->mCameraHandle = open(MODULE_NAME_3, O_RDWR);
		        }
				
			}else{
				write(fid, front, 1);
				close(fid);
				
				if(gbase_sensor_fd[id] != -1)
		        {
		            camera_type->mCameraHandle = gbase_sensor_fd[id];
		        }else{
					camera_type->mCameraHandle = open(MODULE_NAME_0, O_RDWR);
		        }
			}
		}

    }
    else
    {
        V4L2DBUG(V4L2DBUG_ERR, "front_back not exit");

        if(id == 0)
        {
        	if(uvcmode == OMX_UVC_AS_REAR){
				if(gbase_sensor_fd[id] != -1)
	            {
	                camera_type->mCameraHandle = gbase_sensor_fd[id];
					
	            }else{
					camera_type->mCameraHandle = open(MODULE_NAME_3, O_RDWR);
	            }
				
			}else{
				if(gbase_sensor_fd[id] != -1)
	            {
	                camera_type->mCameraHandle = gbase_sensor_fd[id];
					
	            }else{
					camera_type->mCameraHandle = open(MODULE_NAME_0, O_RDWR);///
	            }
			}

        }else{
            if(uvcmode == OMX_UVC_AS_FRONT){
				if(gbase_sensor_fd[id] != -1)
	            {
	                camera_type->mCameraHandle = gbase_sensor_fd[id];
					
	            }else{
					camera_type->mCameraHandle = open(MODULE_NAME_3, O_RDWR);
	            }
				
			}else{
				if(gbase_sensor_fd[id] != -1)
	            {
	                camera_type->mCameraHandle = gbase_sensor_fd[id];
					
	            }else{
					camera_type->mCameraHandle = open(MODULE_NAME_1, O_RDWR);///
	            }
			}
           
        }
		
    }

    V4L2DBUG(V4L2DBUG_ERR, "open Device Id is %x,%d\n", camera_type->mCameraHandle, id);

    if(camera_type->mCameraHandle == -1)
    {
        V4L2DBUG(V4L2DBUG_ERR, "open Device failed!\n");
        return -1;
    }

    gbase_sensor_fd[id] = camera_type->mCameraHandle;
    camera_type->mCameraStatus = 0;
    camera_module_type->pModuleHandle = camera_type->mCameraHandle;
    camera_type->id = id;
    camera_type->bopened = 1;
    
//	    camera_type->wdog_handle = open_watch_dog();
//	    if(NULL == camera_type->wdog_handle) 
//	    { ; }
    
    return 0;
}

int v4l2_camera_close(void *handle)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    base_module_privateType *camera_type = (base_module_privateType *)camera_module_type->pModulePrivate;

    V4L2DBUG(V4L2DBUG_ERR, "close device fd %x,%x,%x\n", camera_type->mCameraHandle, gbase_sensor_fd[camera_type->id], camera_type->mCameraHandle);
    camera_type->mCameraStatus = -1;

    if(camera_type->bopened)
    {
        gbase_sensor_fd[camera_type->id] = -1;
        V4L2DBUG(V4L2DBUG_ERR, "device close %x", camera_type->mCameraHandle);
        close(camera_type->mCameraHandle);
    }

    camera_module_type->pModuleHandle =  NULL;
    camera_type->mCameraHandle = 0;
    camera_type->id = -1;

//	    if(NULL != camera_type->wdog_handle)
//	    {
//	        close_watch_dog(camera_type->wdog_handle);
//	        camera_type->wdog_handle = NULL;
//	    }
    
    return 0;
}

int v4l2_camera_s_fmt(void *handle, int width, int height, int fmt)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    int ret;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = width;
    pixfmt.height = height;

    if(fmt == OMX_COM_FORMAT_MJPG)
    { pixfmt.pixelformat = V4L2_PIX_FMT_MJPEG; }
    else
    { pixfmt.pixelformat = trans_omxfmt2v4l2fmt(fmt); }

    v4l2_fmt.fmt.pix = pixfmt;

    ret = ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt);
    V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_s_fmt %x,%x,%x %d\n", width, height, fmt, ret);
    return ret;
}

int v4l2_camera_g_fmt(void *handle, int *w, int *h, int *fmt)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    struct v4l2_format v4l2_fmt;
    //struct v4l2_pix_format pixfmt;
    int ret;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_G_FMT, v4l2_fmt);

    *w = v4l2_fmt.fmt.pix.width;
    *h = v4l2_fmt.fmt.pix.height;
    *fmt = trans_v4l2fmt2omxfmt(v4l2_fmt.fmt.pix.pixelformat);

    return ret;
}

int v4l2_camera_set_crop(void *handle, int x, int y, int w, int h)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    struct v4l2_crop crop;
    int ret = 0;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;

    memset(&crop, 0, sizeof(struct v4l2_crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_G_CROP, &crop);
    crop.c.left = x;
    crop.c.top = y;
    crop.c.width = w;
    crop.c.height = h;
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_S_CROP, &crop);

    if(ret != 0)
    { V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_set_crop %x,%x,%x,%x,%d\n", w, h, x, y, ret); }

    return 0;
}

int v4l2_camera_get_crop(void *handle, int *x, int *y, int *w, int *h)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    int ret;
    struct v4l2_crop crop;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    memset(&crop, 0, sizeof(struct v4l2_crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_G_CROP, &crop);

    if(ret == 0)
    {
        *x = crop.c.left;
        *y = crop.c.top;
        *w = crop.c.width;
        *h = crop.c.height;
    }
    else { V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_Get_crop"); }

    return 0;
}


int v4l2_camera_set_parm(void *handle, int framerate)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    struct v4l2_streamparm streamparm;
    int ret = 0;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;

    if(framerate > 0)
    {
        if(framerate > 30) { framerate = 30; }

        memset(&streamparm, 0, sizeof(struct v4l2_streamparm));
        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = framerate;

        ret = ioctl(fd, VIDIOC_S_PARM, &streamparm);
    }

    V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_set_parm %x %d\n", framerate, ret);
    return ret;
}

int v4l2_camera_get_parm(void *handle,  int *framerate)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    int ret;
    struct v4l2_streamparm streamparm;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;

    memset(&streamparm, 0, sizeof(struct v4l2_streamparm));

    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_G_PARM, &streamparm);

    if(ret < 0)
    {
        return ret;
    }

    *framerate = streamparm.parm.capture.timeperframe.denominator;
    return 0;
}

int v4l2_camera_g_ctrl(void *handle,   int id, int *value)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    struct v4l2_control ctrl;
    int ret;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;

    ctrl.id = id;

    ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);

    if(ret < 0)
    {
        return ret;
    }

    *value = ctrl.value;
    V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_g_ctrl %x\n", ctrl.value);
    
    return ret;
}

int v4l2_camera_s_ctrl(void *handle,  int id,  int value)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    struct v4l2_control ctrl;
    int ret;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    ctrl.id = id;
    ctrl.value = value;
    V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_s_ctrl %x,%x,%x\n", fd, id, value);
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);

    if(ret != 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "VIDIOC_S_CTRL err %d %x,%x\n", ret, id, value);
    }

    return ret;
}

int v4l2_camera_requestbuf(void *handle, unsigned int *num_bufs)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    struct v4l2_requestbuffers reqbuf;
    int ret;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_USERPTR;
    V4L2DBUG(V4L2DBUG_VERB, "reqbuf,%d\n", __LINE__);
    reqbuf.count = *num_bufs;
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);

    if(ret < 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "reqbuf err!!\n");
        return ret;
    }

    V4L2DBUG(V4L2DBUG_ERR, "reqbuf,%d\n", reqbuf.count);
    *num_bufs = reqbuf.count;

    return 0;
}

int v4l2_camera_qbuf(void *handle, int index, int buflen, unsigned long userptr, unsigned long resver)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    base_module_privateType *module_priv = (base_module_privateType *)camera_module_type->pModulePrivate;
    int fd = module_priv->mCameraHandle;
    struct v4l2_buffer buf;
    int ret = 0;
    
    V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_qbuf %x,%x,%x\n", fd, index, buflen);
    
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.index = index;
    buf.length = buflen;
    buf.m.userptr = userptr;
    buf.reserved = resver;
    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if(ret != 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "VIDIOC_QBUF err %d\n", ret);
    }
    else
    {
        module_priv->nBufferNumInDrv++;
        //V4L2DBUG(V4L2DBUG_PARAM, "VIDIOC_QBUF OK(%d) %d\n", __LINE__, module_priv->nBufferNumInDrv);
    }

    return ret;
}

int v4l2_camera_querybuf(void *handle, int index, int buflen, unsigned long userptr, unsigned long resver)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    int ret = 0;
    struct v4l2_buffer buf;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    V4L2DBUG(V4L2DBUG_ERR, "v4l2camera:v4l2_camera_querybuf %x,%x\n", index, (unsigned int)userptr);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.index = index;

    ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);

    if(ret != 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "VIDIOC_QUERYBUF err %d\n", ret);
    }

#if 0

    if(ret == 0)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = index;
        buf.length = buflen;
        buf.m.userptr = userptr;
        //buf.reserved = resver;
        V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_qbuf re %x,%x,%x\n", index, (unsigned int)userptr, buf.reserved);
        ret = ioctl(fd, VIDIOC_QBUF, &buf);
    }

#endif

    return ret;
}



int v4l2_camera_dqbuf(void *handle, int *index, unsigned long *userptr, unsigned long *resver, long long *timestamp)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    base_module_privateType *camera_type = (base_module_privateType *)camera_module_type->pModulePrivate;
    int fd = camera_type->mCameraHandle;
    struct v4l2_buffer buf;
    int ret;

    V4L2DBUG(V4L2DBUG_VERB, "in %s\n", __func__);
    if(0 == camera_type->nBufferNumInDrv)
    {
        V4L2DBUG(V4L2DBUG_ERR, "no buffer in drv, cannot dequeue\n");
        //if(resver) resver[0] = V4L2_BUF_FLAG_ERROR;
        return -1;
    }
    
//	    if(NULL != camera_type->wdog_handle)
//	    {
//	        char db_info[128];
//	        sprintf(db_info, "%s %d", __func__, __LINE__);
//	        tickle_watch_dog(camera_type->wdog_handle, db_info); 
//	    }
    
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    ret = ioctl(fd, VIDIOC_DQBUF, &buf);

//	    if(NULL != camera_type->wdog_handle)
//	    {
//	        char db_info[128];
//	        sprintf(db_info, "%s %d", __func__, __LINE__);
//	        tickle_watch_dog(camera_type->wdog_handle, db_info); 
//	    }

    if(ret == 0)
    {
        *index = buf.index;
        *userptr = buf.m.userptr;
        if(resver)
        {
            resver[0] = buf.flags;
            resver[1] = buf.bytesused;
            resver[3] = buf.reserved;
        }

        if(timestamp)
        {
            long long tv_sec = (long long)buf.timestamp.tv_sec;
            long long tv_usec = (long long)buf.timestamp.tv_usec;
            if(buf.timestamp.tv_sec < 0 || buf.timestamp.tv_usec < 0)
            {
                V4L2DBUG(V4L2DBUG_ERR, "timestamp warning err %lld,%lld\n", buf.timestamp.tv_sec, buf.timestamp.tv_usec);
            }

            *timestamp = tv_sec * 1000000 + tv_usec ;
            if(*timestamp < 0)
            {
                V4L2DBUG(V4L2DBUG_ERR, "timestamp warning %lld,%lld,%lld\n", *timestamp, tv_sec, tv_usec);
                *timestamp &= 0x7FFFFFFFFFFFFFFFLL;
            }
        }

        camera_type->nBufferNumInDrv--;
        //V4L2DBUG(V4L2DBUG_PARAM, "VIDIOC_DQBUF OK(%d) %d\n", __LINE__, camera_type->nBufferNumInDrv);
    }

    V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_dqbuf %d,%x,%x,%x\n", ret, (unsigned int)*index, (unsigned long)*userptr, (unsigned long)resver);
    return ret;
}

int v4l2_camera_streamon(void *handle)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    base_module_privateType *camera_type = (base_module_privateType *)camera_module_type->pModulePrivate;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = 0;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_streamon\n");
    ret = ioctl(fd, VIDIOC_STREAMON, &type);

    if(ret != 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_streamon err %d\n", ret);
    }

    ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraStatus = 1;
    ((base_module_privateType *)camera_module_type->pModulePrivate)->bstreamon = 1;

//	    if(NULL != camera_type->wdog_handle)
//	    { start_watch_dog(camera_type->wdog_handle); }
    
    return ret;
}


int v4l2_camera_streamoff(void *handle)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    base_module_privateType *camera_type = (base_module_privateType *)camera_module_type->pModulePrivate;
    int ret = 0;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_streamoff\n");
    ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraStatus = 0;

    if(((base_module_privateType *)camera_module_type->pModulePrivate)->bstreamon == 1)
    { ret = ioctl(fd, VIDIOC_STREAMOFF, &type); }

//	    if(NULL != camera_type->wdog_handle)
//	    { stop_watch_dog(camera_type->wdog_handle); }
    
    V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_streamoff OK\n");
    return ret;
}


/*
 YUV 驱动给出来的最大值增益和最小增益
 */
int v4l2_camera_queryctl(void *handle, int id, int *min_v, int *max_v, int *defalt_v, int *step, int *flg)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    int ret = -1;
    int err = 0;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    struct v4l2_queryctrl queryctrl;

    if(id == 0)
    {
        //CID_GAIN
        queryctrl.id = V4L2_CID_GAIN;
        queryctrl.minimum = 0;
        queryctrl.maximum = 0;
        queryctrl.step = 0;
        queryctrl.default_value = 0;
        queryctrl.flags = 0;
    }
    else if(id == 1)
    {
        queryctrl.id = V4L2_CID_EXPOSURE;
        queryctrl.minimum = 0;
        queryctrl.maximum = 0;
        queryctrl.step = 0;
        queryctrl.default_value = 0;
        queryctrl.flags = 0;
    }
    else
    {

        return ret;
    }

    err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

    if(err == 0)
    {
        *min_v = queryctrl.minimum;
        *max_v = queryctrl.maximum;
        *defalt_v = queryctrl.default_value;
        *step = queryctrl.step;
        *flg = queryctrl.flags;
        ret = 0;
    }

    return ret;
}

int v4l2_camera_set_mode(void *handle, int mode)
{
    CAMERA_MODULETYPE *camera_module_type = (CAMERA_MODULETYPE *)handle;
    int fd = ((base_module_privateType *)camera_module_type->pModulePrivate)->mCameraHandle;
    int ret;

    V4L2DBUG(V4L2DBUG_VERB, "v4l2_camera_set_mode: mode=0x%x\n", mode);
    ret = ioctl(fd, V4L2_CID_CAM_CV_MODE, &mode);

    if(ret)
    {
        V4L2DBUG(V4L2DBUG_ERR, "v4l2_camera_set_mode err!,ret =%d\n", ret);
    }

    return 0;//ret;
}

int Camera_Module_constructer(CAMERA_MODULETYPE *camera_module_type)
{
    base_module_privateType *base_module_type = (base_module_privateType *)malloc(sizeof(base_module_privateType));

    memset(base_module_type, 0, sizeof(base_module_privateType));

    if(camera_module_type == NULL)
    {
        free(base_module_type);
        base_module_type = NULL;
        return -1;
    }
    else
    {
        base_module_type->camera_module_type = camera_module_type;
    }

    base_module_type->id = -1;
    base_module_type->mCameraHandle = -1;
    base_module_type->camera_module_type = camera_module_type;
    base_module_type->nBufferNumInDrv = 0;

    camera_module_type->nSize = (unsigned int)sizeof(base_module_privateType);
    camera_module_type->pModulePrivate = (void *)base_module_type;
    camera_module_type->mod_open  = v4l2_camera_open;
    camera_module_type->mod_close = v4l2_camera_close;
    camera_module_type->mod_set_res = v4l2_camera_s_fmt ;
    camera_module_type->mod_get_res = v4l2_camera_g_fmt;
    camera_module_type->mod_set_crop = v4l2_camera_set_crop;
    camera_module_type->mod_get_crop = v4l2_camera_get_crop;
    camera_module_type->mod_set_framerate = v4l2_camera_set_parm;
    camera_module_type->mod_get_framerate = v4l2_camera_get_parm;
    camera_module_type->mod_set_ctl = v4l2_camera_s_ctrl;
    camera_module_type->mod_get_ctl = v4l2_camera_g_ctrl;
    camera_module_type->mod_requestbuf = v4l2_camera_requestbuf;
    camera_module_type->mod_querybuf = v4l2_camera_querybuf;
    camera_module_type->mod_qbuf = v4l2_camera_qbuf;
    camera_module_type->mod_dqbuf = v4l2_camera_dqbuf;
    camera_module_type->mod_streamon = v4l2_camera_streamon;
    camera_module_type->mod_streamoff = v4l2_camera_streamoff;
    camera_module_type->mod_queryctl = v4l2_camera_queryctl;
    camera_module_type->mod_set_mode = v4l2_camera_set_mode;

    return 0;
}


int Camera_Module_destructer(CAMERA_MODULETYPE *camera_module_type)
{
    base_module_privateType *base_module_type;

    if(camera_module_type == NULL)
    {
        return 0;
    }

    base_module_type = camera_module_type->pModulePrivate;

    if(base_module_type)
    {
        if(base_module_type->mCameraStatus)
        {
            v4l2_camera_streamoff(camera_module_type);
        }

        if(base_module_type->mCameraHandle > 0)
        {
            close(base_module_type->mCameraHandle);
            base_module_type->mCameraHandle = 0;
        }

        free(base_module_type);
        base_module_type = NULL;
    }

    return 0;
}

void camera_module_release()
{
    if(gbase_sensor_fd[0] != -1)
    {
        close(gbase_sensor_fd[0]);
        V4L2DBUG(V4L2DBUG_ERR, "Close Device Now-----%d\n", gbase_sensor_fd[0]);
        gbase_sensor_fd[0] = -1;
    }

    if(gbase_sensor_fd[1] != -1)
    {
        close(gbase_sensor_fd[1]);
        V4L2DBUG(V4L2DBUG_ERR, "Close Device Now-----%d\n", gbase_sensor_fd[1]);
        gbase_sensor_fd[1] = -1;
    }
}

int camera_module_query(camera_module_info_t *mode_info, int id, int uvcmode)
{
    struct v4l2_fmtdesc v4l2fd;
    //struct v4l2_format v4l2fmt;
    struct v4l2_frmsizeenum v4l2fsn;
    struct v4l2_frmivalenum v4l2fvn;
    struct v4l2_queryctrl queryctrl;
    struct v4l2_querymenu querymenu;
    int ret = 0;
    int fd = -1;
    int isRawISP = 0;
    int err = 0;
    int i = 0;
    //int id = 0;
    int support_id_num = 0;
    unsigned int module_support_pixelfmt[64];
    unsigned int module_support_pixelfmt_num = 0;
    OMX_ACT_CAPTYPE *pActCap0 = mode_info->supportParam[0];
    int bk_fps[64];
    int issuport_vga;
    int issuport_qvga;
    int issuport_cif;
    int preview_res = 0;
    int max_prev_w = 0;
    int max_prev_h = 0;
    int bSupportNV21 = 0;
    int bMJPEG_Enable = 0;
    int bFlashLedMode[3];
    

    if(mode_info == NULL)
    {
        return -1;
    }

    if(id == 2)
    {
        V4L2DBUG(V4L2DBUG_ERR, "Not support 3D now\n");
        return -1;
    }

    mode_info->supportType[id] = -1;

    memset(module_support_pixelfmt, 0, 64 * sizeof(unsigned int));
    memset(bFlashLedMode, 0, 3 * sizeof(int));
    
    //V4L2DBUG(V4L2DBUG_ERR,"OPEN Device In\n");
    //for(id = 0; id < 2; id++)
    {
        pActCap0 = mode_info->supportParam[id];

        //memset(pActCap0,0,sizeof(OMX_ACT_CAPTYPE));
        if(gbase_sensor_fd[id] != -1)
        {
            fd = gbase_sensor_fd[id];
        }
        else
        {
            /* module id 0 */
            //int fid = open("/sys/bus/platform/devices/camera_flag_device.0/front_back",O_WRONLY);
            int fid = open("/sys/dualcam/dualmod", O_WRONLY);
            char *back = "0";
            char *front = "1";

            if(gbase_sensor_fd[1 - id] != -1)
            {
                close(gbase_sensor_fd[1 - id]);
                V4L2DBUG(V4L2DBUG_ERR, "Close Device Now-----%d\n", gbase_sensor_fd[1 - id]);
                gbase_sensor_fd[1 - id] = -1;
            }

            if(fid > 0)
            {
                if(id == 0) //video 0
                {
                	if(uvcmode == OMX_UVC_AS_REAR){
						fd = open(MODULE_NAME_3, O_RDWR);
						
                	}else{
                    	write(fid, back, 1);
                    	close(fid);
						fd = open(MODULE_NAME_0, O_RDWR);
					}
					
                }else{
                    if(uvcmode == OMX_UVC_AS_FRONT){
						fd = open(MODULE_NAME_3, O_RDWR);
						
                	}else{
                		write(fid, front, 1);
                    	close(fid);
						fd = open(MODULE_NAME_0, O_RDWR);
                	}
                }

                
            }else{
            
                V4L2DBUG(V4L2DBUG_ERR, "front_back not exit %d", fid);

                if(id == 0)
                { 
                    if(uvcmode == OMX_UVC_AS_REAR){
						fd = open(MODULE_NAME_3, O_RDWR);
						
                	}else{
                		fd = open(MODULE_NAME_0, O_RDWR);///
                	}
                	
				}else{ 
                    if(uvcmode == OMX_UVC_AS_FRONT){
						fd = open(MODULE_NAME_3, O_RDWR);
						
                	}else{
                		fd = open(MODULE_NAME_1, O_RDWR);///
                	}
				}
				
            }
        }
        pActCap0->ulThumbResCount = 0;
        pActCap0->uvcmode = uvcmode;

        if(fd >= 0)
        {
            mode_info->mDgain_th[id] = -1;
            V4L2DBUG(V4L2DBUG_ERR, "Module_Query In %d\n", gbase_sensor_fd[id]);
            support_id_num++;
            /* check the format support yuv only or raw only ?*/
            i = 0;

            while(err == 0)
            {
                v4l2fd.index = i;
                v4l2fd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                err = ioctl(fd, VIDIOC_ENUM_FMT, &v4l2fd);
                i++;

                //V4L2DBUG(V4L2DBUG_ERR,"pixelformat %x\n",v4l2fd.pixelformat);
                if(err == 0)
                {
                    switch(v4l2fd.pixelformat)
                    {
                    case V4L2_PIX_FMT_YUV420:
                        V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YUV420 %d\n",OMX_COLOR_FormatYUV420Planar);
                        module_support_pixelfmt[OMX_COLOR_FormatYUV420Planar] = 1;
                        isRawISP = 0;
                        pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV420Planar;
                        pActCap0->ulImageFormatCount++;
                        pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYUV420Planar;
                        pActCap0->ulPreviewFormatCount++;
                        break;

                    case V4L2_PIX_FMT_YUYV:
                        V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YUYV %d\n", OMX_COLOR_FormatYCbYCr);
                        module_support_pixelfmt[OMX_COLOR_FormatYCbYCr] = 1;
                        isRawISP = 0;
                        pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYCbYCr;
                        pActCap0->ulImageFormatCount++;
                        pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYCbYCr;
                        pActCap0->ulPreviewFormatCount++;
                        break;

                    case V4L2_PIX_FMT_YVYU:
                        V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YVYU %d\n", OMX_COLOR_FormatYCrYCb);
                        module_support_pixelfmt[OMX_COLOR_FormatYCrYCb] = 1;
                        isRawISP = 0;
                        pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYCrYCb;
                        pActCap0->ulImageFormatCount++;
                        pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYCrYCb;
                        pActCap0->ulPreviewFormatCount++;
                        break;

                    case V4L2_PIX_FMT_YVU420:
                        V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YVU420 %d\n", OMX_COLOR_FormatYVU420Planar);
                        module_support_pixelfmt[OMX_COLOR_FormatYVU420Planar] = 1;
                        isRawISP = 0;
                        pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYVU420Planar;
                        pActCap0->ulImageFormatCount++;
                        pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYVU420Planar;
                        pActCap0->ulPreviewFormatCount++;
                        break;

                    case V4L2_PIX_FMT_YUV422P:
                        V4L2DBUG(V4L2DBUG_PARAM,"V4L2_PIX_FMT_YUV422P %d\n",OMX_COLOR_FormatYUV422Planar);
                        module_support_pixelfmt[OMX_COLOR_FormatYUV422Planar] = 1;
                        isRawISP = 0;
                        //pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV422Planar;
                        //pActCap0->ulImageFormatCount++;
                        break;

                    case V4L2_PIX_FMT_NV12:
                        V4L2DBUG(V4L2DBUG_PARAM,"V4L2_PIX_FMT_NV12 %d\n",OMX_COLOR_FormatYUV420SemiPlanar);
                        module_support_pixelfmt[OMX_COLOR_FormatYUV420SemiPlanar] = 1;
                        isRawISP = 0;

                        if(bSupportNV21 == 0)
                        {
                            pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV420SemiPlanar;
                            pActCap0->ulImageFormatCount++;
                            pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYUV420SemiPlanar;
                            pActCap0->ulPreviewFormatCount++;
                            bSupportNV21 = 1;
                            //pActCap0->ulThumbResCount = 1;
                        }

                        break;

                    case V4L2_PIX_FMT_NV21:
                        V4L2DBUG(V4L2DBUG_PARAM,"V4L2_PIX_FMT_NV21 %d\n", OMX_COLOR_FormatYVU420SemiPlanar);
                        module_support_pixelfmt[46] = 1;
                        isRawISP = 0;
                        pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYVU420SemiPlanar;
                        pActCap0->ulImageFormatCount++;
                        pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYVU420SemiPlanar;
                        pActCap0->ulPreviewFormatCount++;
                        break;

                    case V4L2_PIX_FMT_NV16:
                        V4L2DBUG(V4L2DBUG_PARAM,"V4L2_PIX_FMT_NV16 %d\n", OMX_COLOR_FormatYUV422SemiPlanar);
                        module_support_pixelfmt[OMX_COLOR_FormatYUV422SemiPlanar] = 1;
                        isRawISP = 0;
                        //pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV422SemiPlanar;
                        //pActCap0->ulImageFormatCount++;
                        break;

                    case V4L2_PIX_FMT_NV61:
                        V4L2DBUG(V4L2DBUG_PARAM,"V4L2_PIX_FMT_NV61 %d\n", OMX_COLOR_FormatYVU422SemiPlanar);
                        module_support_pixelfmt[47] = 1;
                        isRawISP = 0;
                        break;

                    case V4L2_PIX_FMT_MJPEG:
                        V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_MJPEG %d\n", OMX_COLOR_FormatYUV420SemiPlanar);

                        if(bSupportNV21 == 0)
                        {
                            module_support_pixelfmt[OMX_COLOR_FormatYUV420SemiPlanar] = 1;
                            pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV420SemiPlanar;
                            pActCap0->ulImageFormatCount++;
                            pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYUV420SemiPlanar;
                            pActCap0->ulPreviewFormatCount++;
                            bSupportNV21 = 1;
                            pActCap0->ulThumbResCount = 1;
                        }

                        bMJPEG_Enable = 1;
                        isRawISP = 0;
                        break;

                    case V4L2_PIX_FMT_SBGGR8:
                    case V4L2_PIX_FMT_SGBRG8:
                    case V4L2_PIX_FMT_SGRBG8:
                    case V4L2_PIX_FMT_SRGGB8:
                        V4L2DBUG(V4L2DBUG_PARAM, "Raw 8bit %d\n", OMX_COLOR_FormatRawBayer8bit);
                        isRawISP = 1;
                        module_support_pixelfmt[OMX_COLOR_FormatRawBayer8bit] = v4l2fd.pixelformat;
                        break;

                    case V4L2_PIX_FMT_SBGGR10:
                    case V4L2_PIX_FMT_SGBRG10:
                    case V4L2_PIX_FMT_SGRBG10:
                    case V4L2_PIX_FMT_SRGGB10:
                        V4L2DBUG(V4L2DBUG_PARAM, "Raw 10bit %d\n", OMX_COLOR_FormatRawBayer10bit);
                        isRawISP = 1;
                        module_support_pixelfmt[OMX_COLOR_FormatRawBayer10bit] = v4l2fd.pixelformat;
                        break;
                    }

                    module_support_pixelfmt_num++;
                }
            }

            ////fmt check ok
            //V4L2DBUG(V4L2DBUG_PARAM, "Fmt check Finished !!!\n");
            isRawISP = 0;

            /* Preview Format, Use One Format to confirm preivew size*/
            if(module_support_pixelfmt[OMX_COLOR_FormatRawBayer8bit])
            {
                isRawISP = 1;
                //fix as driver support
                v4l2fsn.pixel_format = V4L2_PIX_FMT_SBGGR8;//module_support_pixelfmt[OMX_COLOR_FormatRawBayer8bit];
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatRawBayer10bit])
            {
                isRawISP = 1;
                //fix as driver support
                v4l2fsn.pixel_format = V4L2_PIX_FMT_SBGGR8;//module_support_pixelfmt[OMX_COLOR_FormatRawBayer10bit];
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYCbYCr])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_YUYV \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YUYV;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYCrYCb])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_YVYU \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YVYU;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYUV420Planar])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_YUV420 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YUV420;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYUV420SemiPlanar])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_NV12 \n");

                if(bMJPEG_Enable == 1)
                {
                    v4l2fsn.pixel_format = V4L2_PIX_FMT_MJPEG;
                }
                else
                { v4l2fsn.pixel_format = V4L2_PIX_FMT_NV12; }
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYVU420Planar])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_YVU420 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YVU420;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYVU420SemiPlanar])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_NV21 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_NV21;
            }
            else if(module_support_pixelfmt[46])
            {
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_PIX_FMT_NV21 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_NV21;
            }

            /* Video Preview Resolution */
            //V4L2DBUG(V4L2DBUG_ERR,"Module_Query %d,%d\n",__LINE__,pActCap0->ulPreviewResCount);
            err = 0;
            i = 0;
            gSupport_Preview_1080P[id] = 0;
            gSupport_Preview_720P[id] = 0;
            gSupport_Preview_VGA[id] = 0;
            gSupport_Preview_QVGA[id] = 0;
            gSupport_Preview_CIF[id] = 0;
            gSupport_Preview_QCIF[id] = 0;
            gSupport_Preview_720x576[id] = 0;
            gSupport_Preview_720x480[id] = 0;

            while(err == 0)
            {
                v4l2fsn.index = i;
                err = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &v4l2fsn);
                i++;

                if(err == 0)
                {
                    if(v4l2fsn.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    {
                        if(v4l2fsn.reserved[0] != 1 && 
                           v4l2fsn.discrete.width <= 2048 && v4l2fsn.discrete.height <= 1536)
                        {
                            if(bMJPEG_Enable == 1)
                            {
                                if(v4l2fsn.discrete.width & 0xf)
                                {
                                    v4l2fsn.discrete.width = (v4l2fsn.discrete.width & (~0xf)) + 16;
                                }

                                if(v4l2fsn.discrete.height & 0xf)
                                {
                                    v4l2fsn.discrete.height = (v4l2fsn.discrete.height & (~0xf)) + 16;
                                }
                            }

                            pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = v4l2fsn.discrete.width;
                            pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = v4l2fsn.discrete.height;
                            pActCap0->ulPreviewResCount++;
                            V4L2DBUG(V4L2DBUG_PARAM, "Preview Res %d,%d\n", v4l2fsn.discrete.width, v4l2fsn.discrete.height);

                            if(gMin_Preview_Width[id] > v4l2fsn.discrete.width)
                            {
                                gMin_Preview_Width[id] = v4l2fsn.discrete.width;
                                gMin_Preview_Height[id] = v4l2fsn.discrete.height;
                            }

                            if(v4l2fsn.discrete.width > max_prev_w)
                            {
                                max_prev_w = v4l2fsn.discrete.width;
                                max_prev_h = v4l2fsn.discrete.height;
                            }

                            if(v4l2fsn.discrete.width == 1920 && v4l2fsn.discrete.height == 1080)
                            {
                                gSupport_Preview_1080P[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 1280 && v4l2fsn.discrete.height == 720)
                            {
                                gSupport_Preview_720P[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 640 && v4l2fsn.discrete.height == 480)
                            {
                                gSupport_Preview_VGA[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 320 && v4l2fsn.discrete.height == 240)
                            {
                                gSupport_Preview_QVGA[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 352 && v4l2fsn.discrete.height == 288)
                            {
                                gSupport_Preview_CIF[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 176 && v4l2fsn.discrete.height == 144)
                            {
                                gSupport_Preview_QCIF[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 720 && v4l2fsn.discrete.height == 576)
                            {
                                gSupport_Preview_720x576[id] = 1;
                            }

                            if(v4l2fsn.discrete.width == 720 && v4l2fsn.discrete.height == 480)
                            {
                                gSupport_Preview_720x480[id] = 1;
                            }
                        }
                    }
                    else if(v4l2fsn.type == V4L2_FRMIVAL_TYPE_CONTINUOUS || \
                            v4l2fsn.type == V4L2_FRMIVAL_TYPE_STEPWISE)
                    {
                        struct v4l2_frmsize_stepwise *v4l2_frm_step = &v4l2fsn.stepwise;
                        int j = 0;
                        int step_start = 0;
                        int step_end = 0;
                        int step_step = 0;

                        if(v4l2_frm_step->step_width > (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 8)
                        {
                            v4l2_frm_step->step_width = (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 8;
                        }

                        if(v4l2_frm_step->step_height > (v4l2_frm_step->max_height - v4l2_frm_step->min_height) / 8)
                        {
                            v4l2_frm_step->step_height = (v4l2_frm_step->max_height - v4l2_frm_step->min_height) / 8;
                        }

                        if(v4l2_frm_step->step_width > v4l2_frm_step->step_height)
                        {
                            step_step = v4l2_frm_step->step_height;
                            step_start = v4l2_frm_step->min_height;
                            step_end = v4l2_frm_step->max_height;
                        }
                        else
                        {
                            step_step = v4l2_frm_step->step_width;
                            step_start = v4l2_frm_step->min_width;
                            step_end = v4l2_frm_step->max_width;
                        }

                        if(v4l2_frm_step->step_height == 1 || v4l2_frm_step->step_width == 1)
                        {
                            step_step = (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 8;
                            step_start = v4l2_frm_step->min_width;
                            step_end = v4l2_frm_step->max_width;
                            v4l2_frm_step->step_width = step_step;
                            v4l2_frm_step->step_height = (v4l2_frm_step->max_height - v4l2_frm_step->min_height) / 8;
                        }

                        for(j = step_start; j <= step_end; j += step_step)
                        {

                            pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = v4l2_frm_step->min_width + pActCap0->ulPreviewResCount * v4l2_frm_step->step_width;
                            pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = v4l2_frm_step->min_height + pActCap0->ulPreviewResCount * v4l2_frm_step->step_height;

                            if(bMJPEG_Enable == 1)
                            {
                                if(pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth & 0xf)
                                {
                                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = (pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth & (~0xf)) + 16;
                                }

                                if(pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight & 0xf)
                                {
                                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = (pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight & (~0xf)) + 16;
                                }
                            }

                            if(pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth <= 1920 && pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight <= 1088)
                            {
                                pActCap0->ulPreviewResCount++;

                            }

                            break;
                        }
                    }
                }
            }

            preview_res = 0;
            if(bMJPEG_Enable == 0)
            {
//	                if(gSupport_Preview_1080P[id] == 0 && max_prev_w >= 1280 && max_prev_h >= 720)
//	                {
//	                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 1920;
//	                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 1080;
//	                    pActCap0->ulPreviewResCount++;
//	                    preview_res++;
//	                    V4L2DBUG(V4L2DBUG_PARAM, "Video res 1080P !!!\n");
//	                }
//	    	
//	                if(gSupport_Preview_720P[id] == 0 && max_prev_w >= 1280 && max_prev_h >= 720)
//	                {
//	                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 1280;
//	                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 720;
//	                    pActCap0->ulPreviewResCount++;
//	                    preview_res++;
//	                    V4L2DBUG(V4L2DBUG_PARAM, "Video res 720P !!!\n");
//	                }

                if(gSupport_Preview_VGA[id] == 0 && max_prev_w >= 640 && max_prev_h >= 480)
                {
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 640;
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 480;
                    pActCap0->ulPreviewResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Video res VGA !!!\n");
                }

                if(gSupport_Preview_QVGA[id] == 0  && max_prev_w >= 320 && max_prev_h >= 240)
                {
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 320;
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 240;
                    pActCap0->ulPreviewResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Video res QVGA !!!\n");
                }

                if(gSupport_Preview_CIF[id] == 0 && max_prev_w >= 352 && max_prev_h >= 288)
                {
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 352;
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 288;
                    pActCap0->ulPreviewResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Video res CIF !!!\n");
                }

                if(gSupport_Preview_QCIF[id] == 0 && max_prev_w >= 176 && max_prev_h >= 144)
                {
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 176;
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 144;
                    pActCap0->ulPreviewResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Video res QCIF !!!\n");
                } 

                if(gSupport_Preview_720x576[id] == 0 && max_prev_w >= 720 && max_prev_h >= 576)
                {
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 720;
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 576;
                    pActCap0->ulPreviewResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Video res 720x576 !!!\n");
                } 

                if(gSupport_Preview_720x480[id] == 0 && max_prev_w >= 720 && max_prev_h >= 480)
                {
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = 720;
                    pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nHeight = 480;
                    pActCap0->ulPreviewResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Video res 720x480 !!!\n");
                }
            }
            pActCap0->tPreviewRes[pActCap0->ulPreviewResCount].nWidth = preview_res;
            V4L2DBUG(V4L2DBUG_PARAM, "Video res check Finished !!! %d\n", pActCap0->ulPreviewResCount);

            if(module_support_pixelfmt[OMX_COLOR_FormatRawBayer8bit])
            {
                //fix as driver support
                v4l2fsn.pixel_format = V4L2_PIX_FMT_SBGGR8;//module_support_pixelfmt[OMX_COLOR_FormatRawBayer8bit];
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatRawBayer10bit])
            {
                //fix as driver support
                v4l2fsn.pixel_format = V4L2_PIX_FMT_SBGGR8;//module_support_pixelfmt[OMX_COLOR_FormatRawBayer10bit];
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYUV420Planar])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YUV420 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YUV420;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYUV420SemiPlanar])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_NV12 \n");

                if(bMJPEG_Enable == 1)
                {
                    v4l2fsn.pixel_format = V4L2_PIX_FMT_MJPEG;
                }
                else
                { v4l2fsn.pixel_format = V4L2_PIX_FMT_NV12; }
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYVU420Planar])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YVU420 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YVU420;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYVU420SemiPlanar])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_NV21 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_NV21;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYCbYCr])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YUYV \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YUYV;
            }
            else if(module_support_pixelfmt[OMX_COLOR_FormatYCrYCb])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_YVYU \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_YVYU;
            }
            else if(module_support_pixelfmt[46])
            {
                V4L2DBUG(V4L2DBUG_PARAM, "V4L2_PIX_FMT_NV21 \n");
                v4l2fsn.pixel_format = V4L2_PIX_FMT_NV21;
            }

            /* Image Capture Resolution */
            err = 0;
            i = 0;

            //V4L2DBUG(V4L2DBUG_ERR,"Module_Query %d,%d\n",__LINE__,pActCap0->ulImageResCount);
            while(err == 0)
            {
                v4l2fsn.index = i;
                err = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &v4l2fsn);

                if(err != 0) { break; }

                i++;

                if(err == 0)
                {
                    if(v4l2fsn.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    {
                        if(bMJPEG_Enable == 1)
                        {
                            if(v4l2fsn.discrete.width & 0xf)
                            {
                                v4l2fsn.discrete.width = (v4l2fsn.discrete.width & (~0xf)) + 16;
                            }

                            if(v4l2fsn.discrete.height & 0xf)
                            {
                                v4l2fsn.discrete.height = (v4l2fsn.discrete.height & (~0xf)) + 16;
                            }
                        }

                        pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth = v4l2fsn.discrete.width;
                        pActCap0->tImageRes[pActCap0->ulImageResCount].nHeight = v4l2fsn.discrete.height;
                        pActCap0->ulImageResCount++;
                        V4L2DBUG(V4L2DBUG_PARAM,"Capture Resolution: %dx%d\n",v4l2fsn.discrete.width,v4l2fsn.discrete.height);
                    }
                    else if(v4l2fsn.type == V4L2_FRMIVAL_TYPE_CONTINUOUS || \
                            v4l2fsn.type == V4L2_FRMIVAL_TYPE_STEPWISE)
                    {
                        struct v4l2_frmsize_stepwise *v4l2_frm_step = &v4l2fsn.stepwise;
                        int j = 0;
                        int step_start = 0;
                        int step_end = 0;
                        int step_step = 0;

                        if(v4l2_frm_step->step_width > (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 64)
                        {
                            v4l2_frm_step->step_width = (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 64;
                        }

                        if(v4l2_frm_step->step_height > (v4l2_frm_step->max_height - v4l2_frm_step->min_height) / 64)
                        {
                            v4l2_frm_step->step_height = (v4l2_frm_step->max_height - v4l2_frm_step->min_height) / 64;
                        }

                        if(v4l2_frm_step->step_width > v4l2_frm_step->step_height)
                        {
                            step_step = v4l2_frm_step->step_height;
                            step_start = v4l2_frm_step->min_height;
                            step_end = v4l2_frm_step->max_height;
                        }
                        else
                        {
                            step_step = v4l2_frm_step->step_width;
                            step_start = v4l2_frm_step->min_width;
                            step_end = v4l2_frm_step->max_width;
                        }

                        if(v4l2_frm_step->step_width > (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 64)
                        {
                            v4l2_frm_step->step_width = (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 64;
                        }

                        if(v4l2_frm_step->step_height == 1 || v4l2_frm_step->step_width == 1)
                        {
                            step_step = (v4l2_frm_step->max_width - v4l2_frm_step->min_width) / 64;
                            step_start = v4l2_frm_step->min_width;
                            step_end = v4l2_frm_step->max_width;
                            v4l2_frm_step->step_width = step_step;
                            v4l2_frm_step->step_height = (v4l2_frm_step->max_height - v4l2_frm_step->min_height) / 8;
                        }

                        for(j = step_start; j <= step_end; j += step_step)
                        {
                            pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth = v4l2_frm_step->min_width + pActCap0->ulImageResCount * v4l2_frm_step->step_width;
                            pActCap0->tImageRes[pActCap0->ulImageResCount].nHeight = v4l2_frm_step->min_height + pActCap0->ulImageResCount * v4l2_frm_step->step_height;

                            if(bMJPEG_Enable == 1)
                            {
                                if(pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth & 0xf)
                                {
                                    pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth = (pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth & (~0xf)) + 16;
                                }

                                if(pActCap0->tImageRes[pActCap0->ulImageResCount].nHeight & 0xf)
                                {
                                    pActCap0->tImageRes[pActCap0->ulImageResCount].nHeight = (pActCap0->tImageRes[pActCap0->ulImageResCount].nHeight & (~0xf)) + 16;
                                }
                            }

                            pActCap0->ulImageResCount++;
                        }

                        break;
                    }
                }
            }

            preview_res = 0;
            if(bMJPEG_Enable == 0)
            {
                if(gSupport_Preview_VGA[id] == 0 && max_prev_w >= 640 && max_prev_h >= 480)
                {
                    pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth = 640;
                    pActCap0->tImageRes[pActCap0->ulImageResCount].nHeight = 480;
                    pActCap0->ulImageResCount++;
                    preview_res++;
                    V4L2DBUG(V4L2DBUG_PARAM, "Image res VGA !!!\n");
                }
            }
            
            pActCap0->tImageRes[pActCap0->ulImageResCount].nWidth = preview_res;
            V4L2DBUG(V4L2DBUG_PARAM, "Image res check Finished !!! %d\n", pActCap0->ulImageResCount);

            //V4L2DBUG(V4L2DBUG_ERR,"Module_Query %d\n",__LINE__);
            /* Fps query preview support */
            v4l2fvn.index = 0;

            if(isRawISP == 0)
            { v4l2fvn.pixel_format = v4l2fsn.pixel_format; }//V4L2_PIX_FMT_YUV420;
            else
            { v4l2fvn.pixel_format = v4l2fsn.pixel_format; }

            err = 0;

            for(i = 0; i < pActCap0->ulPreviewResCount; i++)
            {
                int j = 0;
                int k = 0;
                int l = 0;
                int max_fps = 0;
                int min_fps = 12000000;
                v4l2fvn.index = 0;
                v4l2fvn.width  = pActCap0->tPreviewRes[i].nWidth;
                v4l2fvn.height = pActCap0->tPreviewRes[i].nHeight;

                //V4L2DBUG(V4L2DBUG_ERR,"%d,w %d,h %d\n",i,v4l2fvn.width,v4l2fvn.height);
                while(err == 0)
                {
                    v4l2fvn.index = j;
                    err = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &v4l2fvn);

                    //V4L2DBUG(V4L2DBUG_ERR,"%d,",v4l2fvn.index);
                    if(err != 0)
                    {
                        V4L2DBUG(V4L2DBUG_ERR, "VIDIOC_ENUM_FRAMEINTERVALS %d\n", err);
                        break;
                    }


                    if(v4l2fvn.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    {
                        bk_fps[j] = v4l2fvn.discrete.denominator / v4l2fvn.discrete.numerator;
                        bk_fps[j] = bk_fps[j];

                        //V4L2DBUG(V4L2DBUG_ERR,"fps %d\n",bk_fps[j]);
                        if(bk_fps[j] > max_fps)
                        {
                            max_fps = bk_fps[j];
                        }

                        if(min_fps > bk_fps[j])
                        {
                            min_fps = bk_fps[j];
                        }

                        j++;
                    }
                    else if(v4l2fvn.type == V4L2_FRMIVAL_TYPE_CONTINUOUS || \
                            v4l2fvn.type == V4L2_FRMIVAL_TYPE_STEPWISE)
                    {
                        struct v4l2_frmival_stepwise *v4l2_frm_step = &v4l2fvn.stepwise;
                        struct v4l2_fract v4l2_step;
                        v4l2_step = v4l2_frm_step->min;

                        while(v4l2_step.denominator != v4l2_frm_step->max.denominator)
                        {
                            bk_fps[j] = v4l2_step.denominator / v4l2_step.numerator;
                            bk_fps[j] = bk_fps[j];
                            v4l2_step.denominator += v4l2_frm_step->step.denominator;
                            v4l2_step.numerator += v4l2_frm_step->step.numerator;

                            if(bk_fps[j] > max_fps)
                            {
                                max_fps = bk_fps[j];
                            }

                            if(min_fps > bk_fps[j])
                            {
                                min_fps = bk_fps[j];
                            }

                            j++;
                        }

                        break;
                    }
                }

                if(j == 0)
                {
                    pActCap0->tPreviewRes[i].nMaxFps = 30;
                }
                else
                { pActCap0->tPreviewRes[i].nMaxFps = max_fps; }

                for(k = 0; k < j; k++)
                {
                    int isSupported = -100;

                    for(l = 0; l < pActCap0->ulPrvVarFPSModesCount; l++)
                    {
                        if(pActCap0->tPrvVarFPSModes[l].nVarFPSMax != (unsigned int)bk_fps[k])
                        {
                            isSupported++;
                        }
                    }

                    if(isSupported == pActCap0->ulPrvVarFPSModesCount)
                    {
                        pActCap0->tPrvVarFPSModes[pActCap0->ulPrvVarFPSModesCount].nVarFPSMax = (unsigned int)bk_fps[k];
                        pActCap0->tPrvVarFPSModes[pActCap0->ulPrvVarFPSModesCount].nVarFPSMin = (unsigned int)bk_fps[k];
                        pActCap0->ulPrvVarFPSModesCount++;
                    }
                }
            }

            if(pActCap0->ulPrvVarFPSModesCount == 0)
            {
                pActCap0->tPrvVarFPSModes[pActCap0->ulPrvVarFPSModesCount].nVarFPSMax = 30;
                pActCap0->tPrvVarFPSModes[pActCap0->ulPrvVarFPSModesCount].nVarFPSMin = 30;
                pActCap0->ulPrvVarFPSModesCount++;
            }

            V4L2DBUG(V4L2DBUG_ERR, "Module_Query %d,%d\n", __LINE__, pActCap0->ulPrvVarFPSModesCount);
            /* Fps query capture support */
            v4l2fvn.index = 0;

            if(isRawISP == 0)
            { v4l2fvn.pixel_format = v4l2fsn.pixel_format; }//V4L2_PIX_FMT_YUV422P;
            else
            { v4l2fvn.pixel_format = v4l2fsn.pixel_format; }

            err = 0;

            for(i = 0; i < pActCap0->ulImageResCount; i++)
            {
                int j = 0;
                int k = 0;
                int l = 0;
                int max_fps = 0;
                int min_fps = 12000000;
                v4l2fvn.index = 0;
                v4l2fvn.width  = pActCap0->tImageRes[i].nWidth;
                v4l2fvn.height = pActCap0->tImageRes[i].nHeight;

                //V4L2DBUG(V4L2DBUG_ERR,"%d,w %d,h %d\n",i,v4l2fvn.width,v4l2fvn.height);
                while(err == 0)
                {
                    v4l2fvn.index = j;
                    err = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &v4l2fvn);

                    //V4L2DBUG(V4L2DBUG_ERR,"s %d,",v4l2fvn.index);
                    if(err != 0) { break; }

                    if(v4l2fvn.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    {
                        bk_fps[j] = v4l2fvn.discrete.denominator / v4l2fvn.discrete.numerator;
                        bk_fps[j] = bk_fps[j];

                        if(bk_fps[j] > max_fps)
                        {
                            max_fps = bk_fps[j];
                        }

                        if(min_fps > bk_fps[j])
                        {
                            min_fps = bk_fps[j];
                        }

                        j++;
                    }
                    else if(v4l2fvn.type == V4L2_FRMIVAL_TYPE_CONTINUOUS || \
                            v4l2fvn.type == V4L2_FRMIVAL_TYPE_STEPWISE)
                    {
                        struct v4l2_frmival_stepwise *v4l2_frm_step = &v4l2fvn.stepwise;
                        struct v4l2_fract v4l2_step;
                        v4l2_step = v4l2_frm_step->min;

                        while(v4l2_step.denominator != v4l2_frm_step->max.denominator)
                        {
                            bk_fps[j] = v4l2_step.denominator / v4l2_step.numerator;
                            bk_fps[j] = bk_fps[j];
                            v4l2_step.denominator += v4l2_frm_step->step.denominator;
                            v4l2_step.numerator += v4l2_frm_step->step.numerator;

                            if(bk_fps[j] > max_fps)
                            {
                                max_fps = bk_fps[j];
                            }

                            if(min_fps > bk_fps[j])
                            {
                                min_fps = bk_fps[j];
                            }

                            j++;
                        }

                        break;
                    }
                }

                if(j == 0)
                {
                    pActCap0->tImageRes[i].nMaxFps = 5;
                }
                else
                { pActCap0->tImageRes[i].nMaxFps = max_fps; }

                for(k = 0; k < j; k++)
                {
                    int isSupported = -100;

                    for(l = 0; l < pActCap0->ulCapVarFPSModesCount; l++)
                    {
                        if(pActCap0->tCapVarFPSModes[l].nVarFPSMax != (unsigned int)bk_fps[k])
                        {
                            isSupported++;
                        }
                    }

                    if(isSupported == pActCap0->ulCapVarFPSModesCount)
                    {
                        pActCap0->tCapVarFPSModes[pActCap0->ulCapVarFPSModesCount].nVarFPSMax = (unsigned int)bk_fps[k];
                        pActCap0->tCapVarFPSModes[pActCap0->ulCapVarFPSModesCount].nVarFPSMin = (unsigned int)bk_fps[k];
                        pActCap0->ulCapVarFPSModesCount++;
                    }
                }
            }

            if(pActCap0->ulCapVarFPSModesCount == 0)
            {
                pActCap0->tCapVarFPSModes[pActCap0->ulCapVarFPSModesCount].nVarFPSMax = 5;
                pActCap0->tCapVarFPSModes[pActCap0->ulCapVarFPSModesCount].nVarFPSMin = 5;
                pActCap0->ulCapVarFPSModesCount++;
            }

            //isRawISP == 0;
            //V4L2DBUG(V4L2DBUG_ERR,"Module_Query %d,%d\n",__LINE__,pActCap0->ulCapVarFPSModesCount);
            if(isRawISP == 0)
            {
                mode_info->supportType[id] = YUV;
            }
            else
            {
                mode_info->supportType[id] = RAW8;
            }

            //V4L2DBUG(V4L2DBUG_ERR,"Module_Type %d,%d\n",__LINE__,mode_info->supportType[id]);
            pActCap0->xEVCompensationMin = 0;
            pActCap0->xEVCompensationMax =  0;

            queryctrl.id = V4L2_CID_FLASH_LED_MODE;
            queryctrl.minimum = 0;
            queryctrl.maximum = 0;
            queryctrl.step = 0;
            queryctrl.default_value = 0;
            queryctrl.flags = 0;

            err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

            if(err == 0)
            {
//	                for(i = queryctrl.minimum; i <= queryctrl.maximum; i += queryctrl.step)
//	                {
//	//	                    OMX_IMAGE_FlashControlOn = 0,
//	//	                    OMX_IMAGE_FlashControlOff,
//	//	                    OMX_IMAGE_FlashControlAuto,
//	//	                    OMX_IMAGE_FlashControlRedEyeReduction,
//	//	                    OMX_IMAGE_FlashControlFillin,
//	//	                    OMX_IMAGE_FlashControlTorch,
//	                    
//	                    if(i == 0)
//	                    {
//	                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlOff;
//	                        pActCap0->ulFlashCount++;
//	                        //V4L2DBUG(V4L2DBUG_ERR,"Flash %d,%d\n",pActCap0->ulFlashCount,OMX_IMAGE_FlashControlOff);
//	                    }
//	                    else if(i == 1)
//	                    {
//	                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlOn;
//	                        pActCap0->ulFlashCount++;
//	                        //V4L2DBUG(V4L2DBUG_ERR,"Flash %d,%d\n",pActCap0->ulFlashCount,OMX_IMAGE_FlashControlOn);
//	                    }
//	                    else if(i == 2)
//	                    {
//	                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlTorch;
//	                        pActCap0->ulFlashCount++;
//	                        //V4L2DBUG(V4L2DBUG_ERR,"Flash %d,%d\n",pActCap0->ulFlashCount,OMX_IMAGE_FlashControlTorch);
//	                    }
//	                    else if(i == 3)
//	                    {
//	                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlAuto;
//	                        pActCap0->ulFlashCount++;
//	                        //V4L2DBUG(V4L2DBUG_ERR,"Flash %d,%d\n",pActCap0->ulFlashCount,OMX_IMAGE_FlashControlAuto);
//	                    }
//	                }

                V4L2DBUG(V4L2DBUG_PARAM, "%s, type %d, min %d, max %d, step %d\n", queryctrl.name, queryctrl.type, queryctrl.minimum, queryctrl.maximum, queryctrl.step);
                if(V4L2_CTRL_TYPE_MENU == queryctrl.type)
                {
                    int idx;
                    querymenu.id = queryctrl.id;
                    for(idx = queryctrl.minimum; idx <= queryctrl.maximum; idx++)
                    {
                        querymenu.index = idx;
                        if(ioctl(fd, VIDIOC_QUERYMENU, &querymenu) == 0)
                        {
                            V4L2DBUG(V4L2DBUG_PARAM, "%d, %s\n", querymenu.index, querymenu.name);
                            bFlashLedMode[idx] = 1;
                        }
                    }

                    if(bFlashLedMode[V4L2_FLASH_LED_MODE_TORCH] || bFlashLedMode[V4L2_FLASH_LED_MODE_FLASH])
                    {
                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlOff;
                        pActCap0->ulFlashCount++;
                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlOn;
                        pActCap0->ulFlashCount++;
                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlAuto;
                        pActCap0->ulFlashCount++;
                    }

                    if(bFlashLedMode[V4L2_FLASH_LED_MODE_TORCH])
                    {
                        pActCap0->eFlashModes[pActCap0->ulFlashCount] = OMX_IMAGE_FlashControlTorch;
                        pActCap0->ulFlashCount++;
                    }
                }
            }

#if 1
            if(isRawISP == 0)
            {
                int ex_com_support = 0;
                queryctrl.id = V4L2_CID_EXPOSURE_COMP;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);
                if(err == 0)
                {
                    pActCap0->xEVCompensationMin = queryctrl.minimum << 16;
                    pActCap0->xEVCompensationMax = queryctrl.maximum << 16;
                    pActCap0->xEVCompensationStep = queryctrl.step << 16;
                    V4L2DBUG(V4L2DBUG_PARAM, "support ev: %d - %d\n", queryctrl.minimum, queryctrl.maximum);

                    if(pActCap0->xEVCompensationMin < 0 &&  pActCap0->xEVCompensationMax > 0)
                    {
                        ex_com_support = 1;
                    }
                }

                /* query brightness support */
                queryctrl.id = V4L2_CID_BRIGHTNESS;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    pActCap0->bBrightnessSupported = OMX_TRUE;
                    pActCap0->xBrightnessLevel.nMinVal = queryctrl.minimum;
                    pActCap0->xBrightnessLevel.nMaxVal = queryctrl.maximum;
                    pActCap0->xBrightnessLevel.nStep = (queryctrl.step == 0) ? 1 : queryctrl.step;
                }

                //pActCap0->xEVCompensationMin = pActCap0->xBrightnessLevel.nMinVal<<16;
                //pActCap0->xEVCompensationMax = pActCap0->xBrightnessLevel.nMaxVal<<16;

                /* query brightness support */
                queryctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    pActCap0->ulWhiteBalanceCount++;
                    pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = OMX_WhiteBalControlAuto;
                    pActCap0->ulWhiteBalanceCount++;
                }

                queryctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    int nwbTrans[] = {7, 6, 2, 3, 0, 9, 4}; //{-1,-1,2,3,6,-1,1,0,-1,5}

                    for(i = queryctrl.minimum; i <= queryctrl.maximum; i += queryctrl.step)
                    {
                        /*
                         * V4L2_WHITE_BALANCE_INCANDESCENT      = 0, // 白炽光
                        *   V4L2_WHITE_BALANCE_FLUORESCENT      = 1, // 荧光灯
                        *   V4L2_WHITE_BALANCE_DAYLIGHT         = 2, // 日光
                        *   V4L2_WHITE_BALANCE_CLOUDY_DAYLIGHT  = 3, // 多云
                        *   V4L2_WHITE_BALANCE_WARM_FLUORESCENT = 4, // 暖荧光灯
                        *   V4L2_WHITE_BALANCE_TWILIGHT         = 5, // 黄昏
                        *   V4L2_WHITE_BALANCE_SHADE            = 6, // 阴影
                        *   OMX_WhiteBalControlSunLight,2
                        *   OMX_WhiteBalControlCloudy,3
                        *   OMX_WhiteBalControlShade,4
                        *   OMX_WhiteBalControlTungsten,5
                        *   OMX_WhiteBalControlFluorescent,6
                        *   OMX_WhiteBalControlIncandescent,7
                        *   OMX_WhiteBalControlFlash,8
                        *   OMX_WhiteBalControlHorizon 9
                         */
                        if(nwbTrans[i])
                        {
                            pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = nwbTrans[i];
                            pActCap0->ulWhiteBalanceCount++;
                            //V4L2DBUG(V4L2DBUG_ERR,"WhiteBalanceModes %d,%d\n",pActCap0->ulWhiteBalanceCount,nwbTrans[i]);
                        }
                    }
                }

                queryctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                pActCap0->ulFlickerCount = 0;
                if(err == 0)
                {
                    if(queryctrl.minimum > 0)
                    { pActCap0->ulFlickerCount++; }

                    for(i = queryctrl.minimum; i <= queryctrl.maximum; i += queryctrl.step)
                    {
                        if(i >= 0)
                        {
                            pActCap0->eFlicker[pActCap0->ulFlickerCount] = i;
                            pActCap0->ulFlickerCount++;
                            //V4L2DBUG(V4L2DBUG_ERR,"Flicker %d,%d\n",pActCap0->ulFlickerCount,i+1);
                        }
                    }
                }




                queryctrl.id = V4L2_CID_GAIN;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    mode_info->mDgain_th[id] = queryctrl.default_value;
                }

                V4L2DBUG(V4L2DBUG_ERR, "mDgain_th:%d,%d\n", id, queryctrl.default_value);

                queryctrl.id = V4L2_CID_COLORFX;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    /*
                     *
                     *  V4L2_COLORFX_NONE   = 0,
                        V4L2_COLORFX_BW     = 1,
                        V4L2_COLORFX_SEPIA  = 2,
                        V4L2_COLORFX_NEGATIVE = 3,
                        V4L2_COLORFX_EMBOSS = 4,
                        V4L2_COLORFX_SKETCH = 5,
                        V4L2_COLORFX_SKY_BLUE = 6,
                        V4L2_COLORFX_GRASS_GREEN = 7,
                        V4L2_COLORFX_SKIN_WHITEN = 8,
                        V4L2_COLORFX_VIVID = 9,
                         OMX_ImageFilterNone,
                        OMX_ImageFilterNoise,
                        OMX_ImageFilterEmboss,
                        OMX_ImageFilterNegative,
                        OMX_ImageFilterSketch,
                        OMX_ImageFilterOilPaint,
                        OMX_ImageFilterHatch,
                        OMX_ImageFilterGpen,
                        OMX_ImageFilterAntialias,
                        OMX_ImageFilterDeRing,
                        OMX_ImageFilterSolarize,
                        OMX_ImageFilterACTBW = 0x7F000001, 1
                        OMX_ImageFilterACTSEPIA, 2
                        OMX_ImageFilterACTSKY_BLUE, 6
                        OMX_ImageFilterACTGRASS_GREEN,7
                        OMX_ImageFilterACTSKIN_WHITEN,8
                        OMX_ImageFilterACTVIVID,9
                     */
                    int idx_map[] =
                    {
                        /*0x0,
                        0x7F000001,
                        0x7F000002,
                        3,
                        2,
                        4,
                        0x7F000003,
                        0x7F000004,
                        0x7F000005,*/
                        0, 0x7F000001, 0x7F000002, 3, 2, 4, 0x7F000003, 0x7F000004, 0x7F000005, 0x7F000006
                    };//{0,-1,4,3,5,-1,-1,-1,-1,-1,-1}

                    if(queryctrl.minimum > 0)
                    { pActCap0->ulColorEffectCount++; }

                    for(i = queryctrl.minimum; i <= queryctrl.maximum; i += queryctrl.step)
                    {
                        if(idx_map[i] >= 0)
                        {
                            pActCap0->eColorEffects[pActCap0->ulColorEffectCount] = idx_map[i];
                            pActCap0->ulColorEffectCount++;
                            //V4L2DBUG(V4L2DBUG_ERR,"ColorEffects %d,%x\n",pActCap0->ulColorEffectCount,idx_map[i]);
                        }
                    }
                }

                queryctrl.id = V4L2_CID_EXPOSURE_AUTO;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    //pActCap0->ulExposureModeCount++;

                    for(i = queryctrl.minimum; i <= queryctrl.maximum; i += queryctrl.step)
                    {
                        /*
                         * enum  v4l2_scene_exposure_type {
                        V4L2_SCENE_MODE_HOUSE           = 0,
                        V4L2_SCENE_MODE_SUNSET          = 1,
                        V4L2_SCENE_MODE_ACTION          = 2,
                        V4L2_SCENE_MODE_PORTRAIT        = 3,
                        V4L2_SCENE_MODE_LANDSCAPE       = 4,
                        V4L2_SCENE_MODE_NIGHT           = 5,
                        V4L2_SCENE_MODE_NIGHT_PORTRAIT  = 6,
                        V4L2_SCENE_MODE_THEATRE         = 7,
                        V4L2_SCENE_MODE_BEACH           = 8,
                        V4L2_SCENE_MODE_SNOW            = 9,
                        V4L2_SCENE_MODE_STEADYPHOTO     = 10,
                        V4L2_SCENE_MODE_FIREWORKS       = 11,
                        V4L2_SCENE_MODE_SPORTS          = 12,
                        V4L2_SCENE_MODE_PARTY           = 13,
                        V4L2_SCENE_MODE_CANDLELIGHT     = 14,
                        V4L2_SCENE_MODE_BARCODE         = 15,
                        };
                          OMX_ExposureControlOff = 0,
                            OMX_ExposureControlAuto,
                            OMX_ExposureControlNight,
                            OMX_ExposureControlBackLight,
                            OMX_ExposureControlSpotLight,
                            OMX_ExposureControlSports,
                            OMX_ExposureControlSnow,
                            OMX_ExposureControlBeach,
                            OMX_ExposureControlLargeAperture,
                            OMX_ExposureControlSmallApperture,
                         */
                        pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = i;
                        pActCap0->ulExposureModeCount++;
                        //V4L2DBUG(V4L2DBUG_ERR,"eExposureModes %d,%x\n",pActCap0->ulExposureModeCount,i);
                    }
                }

                queryctrl.id = V4L2_CID_SCENE_EXPOSURE;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;

                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    int exptrs[] = {0x7F000001, 0x7F000002, 0x7F000003, 0x7F000004, 0x7F000005, \
                                    2, 0x7F000006, 0x7F000007, 7, 6, 0x7F000008, 0x7F000009, 5, 0x7F00000a, 0x7F00000b, 0x7F00000c
                                   };

                    for(i = queryctrl.minimum; i <= queryctrl.maximum; i += queryctrl.step)
                    {
                        /*
                         * enum  v4l2_scene_exposure_type {
                        V4L2_SCENE_MODE_HOUSE           = 0,
                        V4L2_SCENE_MODE_SUNSET          = 1,
                        V4L2_SCENE_MODE_ACTION          = 2,
                        V4L2_SCENE_MODE_PORTRAIT        = 3,
                        V4L2_SCENE_MODE_LANDSCAPE       = 4,
                        V4L2_SCENE_MODE_NIGHT           = 5,
                        V4L2_SCENE_MODE_NIGHT_PORTRAIT  = 6,
                        V4L2_SCENE_MODE_THEATRE         = 7,
                        V4L2_SCENE_MODE_BEACH           = 8,
                        V4L2_SCENE_MODE_SNOW            = 9,
                        V4L2_SCENE_MODE_STEADYPHOTO     = 10,
                        V4L2_SCENE_MODE_FIREWORKS       = 11,
                        V4L2_SCENE_MODE_SPORTS          = 12,
                        V4L2_SCENE_MODE_PARTY           = 13,
                        V4L2_SCENE_MODE_CANDLELIGHT     = 14,
                        V4L2_SCENE_MODE_BARCODE         = 15,
                        };
                          OMX_ExposureControlOff = 0,
                            OMX_ExposureControlAuto,
                            OMX_ExposureControlNight,
                            OMX_ExposureControlBackLight,
                            OMX_ExposureControlSpotLight,
                            OMX_ExposureControlSports,
                            OMX_ExposureControlSnow,
                            OMX_ExposureControlBeach,
                            OMX_ExposureControlLargeAperture,
                            OMX_ExposureControlSmallApperture,
                            OMX__ExposureControlActHouse = 0x7F000001,
                                OMX_ExposureControlActSunset,
                                OMX_ExposureControlActAction,
                                OMX_ExposureControlActPortrait,
                                OMX_ExposureControlActLandscape,
                                OMX_ExposureControlActNight_Portrait,
                                OMX_ExposureControlActTheatre,
                                OMX_ExposureControlActStreadyPhoto,
                                OMX_ExposureControlActFireworks,
                                OMX_ExposureControlActParty,
                                OMX_ExposureControlActCandlelight,
                                OMX_ExposureControlActBarcode,
                         */
                        if(exptrs[i] > 0)
                        {
                            pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = exptrs[i];
                            pActCap0->ulExposureModeCount++;
                            //V4L2DBUG(V4L2DBUG_ERR,"eExposureModes %d,%x\n",pActCap0->ulExposureModeCount,exptrs[i]);
                        }
                    }
                }
                else
                {
                    V4L2DBUG(V4L2DBUG_ERR, "V4L2_CID_SCENE_EXPOSURE is not support!!!!\n");
                }

                ///////////////////////////////////
                queryctrl.id = V4L2_CID_SCENE_MODE;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);
                if(err == 0)
                {
                    V4L2DBUG(V4L2DBUG_PARAM, "%s, type %d, min %d, max %d, step %d\n", queryctrl.name, queryctrl.type, queryctrl.minimum, queryctrl.maximum, queryctrl.step);
                    if(V4L2_CTRL_TYPE_MENU == queryctrl.type)
                    {
                        int idx;
                        for(idx = queryctrl.minimum; idx <= queryctrl.maximum; idx++)
                        {
                            querymenu.id = queryctrl.id;
                            querymenu.index = idx;
                            if(ioctl(fd, VIDIOC_QUERYMENU, &querymenu) == 0)
                            {
                                V4L2DBUG(V4L2DBUG_PARAM, "%d, %s\n", querymenu.index, querymenu.name);
                            }
                        }
                    }
                }

                // The condition need to be improved. It's a temporary solution.
                if(ex_com_support && (queryctrl.maximum == 14))
                {
                    V4L2DBUG(V4L2DBUG_PARAM, "support bracket exposure to support HDR mode\n");
                    pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = OMX_ExposureControlActHDR;
                    pActCap0->ulExposureModeCount++;
                }

#if 1
                /* query focus */
                queryctrl.id = V4L2_CID_AF_MODE;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                pActCap0->ulFocusModeCount = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                /*
                enum af_mode{
                NONE = 0,            //不支持任何对焦模式
                SINGLE_AF = (0x1<<1), //单次对焦
                CONTINUE_AF = (0x1<<2), //连续对焦
                ZONE_AF = (0x1<<3),     //区域对焦
                MACRO_AF = (0x1<<4),    //微距对焦
                MANUAL_AF = (0x1<<5),   //手动对焦
                FACE_AF = (0x1<<6),     //面部对焦
                };
                */
                if(err == 0)
                {
                    if(queryctrl.default_value)
                    {
                        if(queryctrl.default_value & 0xc)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlAuto;
                            pActCap0->ulFocusModeCount++;
                            V4L2DBUG(V4L2DBUG_ERR, "auto Focus");
                        }

                        if(queryctrl.default_value & 0x2)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlSingle;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(queryctrl.default_value & 0x20)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlOn;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(queryctrl.default_value & 0x10)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlMacro;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(queryctrl.default_value & 0x40)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlFACE;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(pActCap0->ulFocusModeCount > 0)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlOff;
                            pActCap0->ulFocusModeCount++;
                            V4L2DBUG(V4L2DBUG_ERR, "Focus disable");
                        }
                    }

                    V4L2DBUG(V4L2DBUG_ERR, "Focus %x,%x\n", queryctrl.default_value, pActCap0->ulFocusModeCount);
                }

                queryctrl.id = V4L2_CID_AF_REGION;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                pActCap0->ulAreasFocusCount = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                if(err == 0)
                {
                    pActCap0->ulAreasFocusCount = queryctrl.default_value;
                    V4L2DBUG(V4L2DBUG_ERR, "Focus Area %x\n", queryctrl.default_value);
                }

#else
                pActCap0->ulFocusModeCount = 0;
                pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlAuto;
                pActCap0->ulFocusModeCount++;
                pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlOff;
                pActCap0->ulFocusModeCount++;
                pActCap0->ulAreasFocusCount = 0;
#endif

                pActCap0->bExposureLockSupported = 1;
                pActCap0->bFocusLockSupported = 1;
                pActCap0->bWhiteBalanceLockSupported = 1;
            }
            else
            {
                int ex_com_support = 0;
                
                queryctrl.id = V4L2_CID_GAIN;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);
                if(err == 0)
                {
                    mode_info->mDgain_th[id] = queryctrl.default_value * 16;
                }
                V4L2DBUG(V4L2DBUG_PARAM, "mDgain_th:%d,%d\n", id, queryctrl.default_value);
                
                pActCap0->xEVCompensationMin = 0 << 16;
                pActCap0->xEVCompensationMax = 0 << 16;
                pActCap0->xEVCompensationStep = 0 << 16;
                
                pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYUV420Planar;
                pActCap0->ulPreviewFormatCount++;
                pActCap0->ePreviewFormats[pActCap0->ulPreviewFormatCount] = OMX_COLOR_FormatYUV420SemiPlanar;
                pActCap0->ulPreviewFormatCount++;

                pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV420Planar;
                pActCap0->ulImageFormatCount++;
                pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV420SemiPlanar;
                pActCap0->ulImageFormatCount++;
                pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV422Planar;
                pActCap0->ulImageFormatCount++;
                pActCap0->eImageFormats[pActCap0->ulImageFormatCount] = OMX_COLOR_FormatYUV422SemiPlanar;
                pActCap0->ulImageFormatCount++;

                pActCap0->ulExposureModeCount = 0;
                pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = OMX_ExposureControlOff;
                pActCap0->ulExposureModeCount++;
                pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = OMX_ExposureControlAuto;
                pActCap0->ulExposureModeCount++;
                pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = OMX_ExposureControlNight;
                pActCap0->ulExposureModeCount++;
                pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = OMX_ExposureControlActSunset;
                pActCap0->ulExposureModeCount++;

                if(ex_com_support)
                {
                    pActCap0->eExposureModes[pActCap0->ulExposureModeCount] = OMX_ExposureControlActHDR;
                    pActCap0->ulExposureModeCount++;
                }

                pActCap0->ulColorEffectCount++;
                pActCap0->eColorEffects[pActCap0->ulColorEffectCount] = OMX_ImageFilterNegative;//nagative
                pActCap0->ulColorEffectCount++;
                pActCap0->eColorEffects[pActCap0->ulColorEffectCount] = OMX_ImageFilterACTBW;//B&W
                pActCap0->ulColorEffectCount++;
                //pActCap0->eColorEffects[pActCap0->ulColorEffectCount] = color_fix[2];
                //pActCap0->ulColorEffectCount++;

                //不支持亮度设置
                pActCap0->bBrightnessSupported = OMX_FALSE;
                pActCap0->xBrightnessLevel.nMinVal = isp_module_type[0].min_val;
                pActCap0->xBrightnessLevel.nMaxVal = isp_module_type[0].max_val;
                pActCap0->xBrightnessLevel.nStep = isp_module_type[0].step;

                pActCap0->ulFlickerCount = 3;
                pActCap0->eFlicker[0] = OMX_FlickerRejectionOff;
                pActCap0->eFlicker[1] = OMX_FlickerRejection50;
                pActCap0->eFlicker[2] = OMX_FlickerRejection60;

                //pActCap0->xEVCompensationMin = -1*(1<<16);
                //pActCap0->xEVCompensationMax = (1<<16);

                /*  OMX_WhiteBalControlSunLight,2
                *   OMX_WhiteBalControlCloudy,3
                *   OMX_WhiteBalControlShade,4
                *   OMX_WhiteBalControlTungsten,5
                *   OMX_WhiteBalControlFluorescent,6
                *   OMX_WhiteBalControlIncandescent,7
                *   OMX_WhiteBalControlFlash,8
                *   OMX_WhiteBalControlHorizon 9
                 */

                pActCap0->ulWhiteBalanceCount++;
                pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = OMX_WhiteBalControlAuto;
                pActCap0->ulWhiteBalanceCount++;
                pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = OMX_WhiteBalControlSunLight;
                pActCap0->ulWhiteBalanceCount++;
                pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = OMX_WhiteBalControlTungsten;
                pActCap0->ulWhiteBalanceCount++;
                pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = OMX_WhiteBalControlIncandescent;
                pActCap0->ulWhiteBalanceCount++;
                pActCap0->eWhiteBalanceModes[pActCap0->ulWhiteBalanceCount] = OMX_WhiteBalControlHorizon;
                pActCap0->ulWhiteBalanceCount++;
                V4L2DBUG(V4L2DBUG_ERR, "Mode Nums %d,%d,%d,%d\n", pActCap0->ulExposureModeCount, pActCap0->ulWhiteBalanceCount, \
                         pActCap0->ulFlickerCount, pActCap0->ulExposureModeCount);
                pActCap0->ulAreasFocusCount = 1;

                /* ===== FIX Here if focus supported ==== */
                pActCap0->ulFocusModeCount = 0;
                pActCap0->eFocusModes[0] = OMX_IMAGE_FocusControlOff;
                pActCap0->eFocusModes[1] = OMX_IMAGE_FocusControlAuto;
                pActCap0->eFocusModes[2] = OMX_IMAGE_FocusControlSingle;
#if 1
                queryctrl.id = V4L2_CID_AF_MODE;
                queryctrl.minimum = 0;
                queryctrl.maximum = 0;
                queryctrl.step = 0;
                queryctrl.default_value = 0;
                queryctrl.flags = 0;
                pActCap0->ulFocusModeCount = 0;
                err = ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);

                /*
                    enum af_mode{
                    NONE = 0,            //不支持任何对焦模式
                    SINGLE_AF = (0x1<<1), //单次对焦
                    CONTINUE_AF = (0x1<<2), //连续对焦
                    ZONE_AF = (0x1<<3),     //区域对焦
                    MACRO_AF = (0x1<<4),    //微距对焦
                    MANUAL_AF = (0x1<<5),   //手动对焦
                    FACE_AF = (0x1<<6),     //面部对焦
                  };
                */
                if(err == 0)
                {
                    if(queryctrl.default_value)
                    {
                        if(queryctrl.default_value & 0xc)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlAuto;
                            pActCap0->ulFocusModeCount++;
                            V4L2DBUG(V4L2DBUG_ERR, "auto Focus");
                        }

                        if(queryctrl.default_value & 0x2)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlSingle;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(queryctrl.default_value & 0x20)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlOn;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(queryctrl.default_value & 0x10)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlMacro;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(queryctrl.default_value & 0x40)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlFACE;
                            pActCap0->ulFocusModeCount++;
                        }

                        if(pActCap0->ulFocusModeCount > 0)
                        {
                            pActCap0->eFocusModes[pActCap0->ulFocusModeCount] = OMX_IMAGE_FocusControlOff;
                            pActCap0->ulFocusModeCount++;
                            V4L2DBUG(V4L2DBUG_ERR, "Focus disable");
                        }

                        if(queryctrl.default_value == (1 << 30))
                        {
                            pActCap0->ulFocusModeCount = 3;
                            pActCap0->eFocusModes[0] = OMX_IMAGE_FocusControlOff;
                            pActCap0->eFocusModes[1] = OMX_IMAGE_FocusControlAuto;
                            pActCap0->eFocusModes[2] = OMX_IMAGE_FocusControlSingle;
                            V4L2DBUG(V4L2DBUG_ERR, "Focus Enable outer ");
                        }

                    }

                    V4L2DBUG(V4L2DBUG_ERR, "Focus %x,%x\n", queryctrl.default_value, pActCap0->ulFocusModeCount);
                }
#endif

                pActCap0->bExposureLockSupported = 1;
                pActCap0->bFocusLockSupported = 1;
                pActCap0->bWhiteBalanceLockSupported = 1;
            }

#endif
            /*if(gbase_sensor_fd[1-id]!=-1){
                close(fd);
                V4L2DBUG(V4L2DBUG_ERR,"Close Device Now-----%d\n",gbase_sensor_fd[id]);
                gbase_sensor_fd[id] == -1;
            }
            else*/
            {
                gbase_sensor_fd[id] = fd;
                //V4L2DBUG(V4L2DBUG_ERR,"Close Device back-----%d\n",gbase_sensor_fd[id]);
            }
            fd = 0;
        }
        else
        {
            V4L2DBUG(V4L2DBUG_ERR, "open failed Module ID %d,%d\n", id, gbase_sensor_fd[id]);
            ret = -1;
        }
    }

    if(support_id_num == 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "Query Sensor Module is failed!\n");
        ret = -1;
    }

    //V4L2DBUG(V4L2DBUG_ERR,"Module_Query Out\n");
    return ret;
}

#ifdef __cplusplus
}
#endif
