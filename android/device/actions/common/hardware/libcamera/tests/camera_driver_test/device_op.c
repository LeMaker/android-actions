/*
 *         camera device operate
 *  mainly including device open close ctl_set.
 *
 *
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>        /* for videodev2.h */
#include <linux/videodev2.h> 
#include <linux/fcntl.h>  
#include <semaphore.h> 
#include <pthread.h> 

#include "display.h"
#include "buf_op.h"
#include "device_op.h"
#include "linux/videodev2.h"
#include <fcntl.h>

struct v4l2_queryctrl query_array[]=
{
    {.id = V4L2_CID_AUTO_WHITE_BALANCE},
    {.id = (V4L2_CID_EXPOSURE_AUTO + 18)}, //V4L2_CID_SCENE_EXPOSURE
    {.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE},
    {.id = V4L2_CID_COLORFX},
    {.id = V4L2_CID_EXPOSURE_AUTO},
};

struct v4l2_frmsizeenum framesize_array[]=
{
    {.index = 0,
     },
    {.index = 1,
     },
    {.index = 2,
     },
    {.index = 3,
     },
    {.index = 4,
     },
    {.index = 5,
     },
};

struct v4l2_frmivalenum frame_interval_array[]=
{
    {.index = 0,
     },
    {.index = 1,
     },
    {.index = 2,
     },
    {.index = 3,
     },
    {.index = 4,
     },
    {.index = 5,
     },
    {.index = 6,
    },
};







int fd_camera;
option_mode OPTION_TESTMODE;
int FRAME_FORMAT;
int FRAME_SIZE_ID;
int FRAME_RATE_ID;

char *DEV_CAMERA_NAME = NULL;
char *DEV_CAMERA_FRONT_NAME = NULL;
char *DEV_CAMERA_BACK_NAME = NULL;

int query_array_size = sizeof(query_array)/sizeof(struct v4l2_queryctrl);
static  int framesize_array_size = sizeof(framesize_array)/sizeof(struct v4l2_frmsizeenum);
static  int frame_interval_array_size = sizeof(frame_interval_array)/sizeof(struct v4l2_frmivalenum);

//camera_info_t camera_info{
//    }


void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}





int close_device_camera(void)
{
    int ret;
    if (-1 == close(fd_camera))
    return -1;
    ret = uninit_buf();
    return ret;

}

int open_device_camera(void)
{
    fd_camera = open(DEV_CAMERA_NAME, O_RDWR /* required */  | O_NONBLOCK, 0);
    if (-1 == fd_camera) {
        return -1;
    }
    return 0;
}


int queryctrl_camera(struct v4l2_queryctrl* query)
{
    int ret;
    ret = ioctl(fd_camera, VIDIOC_QUERYCTRL, query);
    return ret;
}

int setctrl_camera(struct v4l2_queryctrl *query_value, option_mode mode)
{
    int ret;
    struct v4l2_control ctl_val;
    ctl_val.id = query_value->id;
    printf("ctrl_id 0x%x",ctl_val.id);
    switch(mode)
    {
        case MODE_DEFAULT_SLOW:
        case MODE_DEFAULT_FAST:
            ctl_val.value = query_value->default_value;
            ret = ioctl(fd_camera, VIDIOC_S_CTRL, &ctl_val);
            break;
        case MODE_ALL_SLOW:
        case MODE_ALL_FAST:
            for(ctl_val.value = query_value->minimum; ctl_val.value <= query_value->maximum; ctl_val.value+=query_value->step)
            {
                printf("ctrl_val 0x%x\n",ctl_val.value);
                ret = ioctl(fd_camera, VIDIOC_S_CTRL, &ctl_val);
                if (ret)
                break;
                if (mode == MODE_ALL_SLOW)
                {
                    sleep(5);
                }
            }
            break;
        default:
            printf("OPTION_TESTMODE error in %s()\n",__FUNCTION__);

    }
    return ret;
}

void init_framesize_array(void)
{
    int i =0;
    while(i < framesize_array_size)
    {
        framesize_array[i].pixel_format = FRAME_FORMAT;
        i++;
    }
}

void init_frame_interval_array(void)
{
    int i =0;
    while(i < frame_interval_array_size)
    {
        frame_interval_array[i].width = framesize_array[FRAME_SIZE_ID].discrete.width;
        frame_interval_array[i].height = framesize_array[FRAME_SIZE_ID].discrete.height;
        frame_interval_array[i].pixel_format = framesize_array[FRAME_SIZE_ID].pixel_format;
        i++;
    }
}




int enum_framesize_camera(struct v4l2_frmsizeenum* framesize)
{
    int ret;
    framesize->pixel_format = FRAME_FORMAT;
    ret = ioctl(fd_camera, VIDIOC_ENUM_FRAMESIZES, framesize);
    if (ret) {
        printf("1:enum frame size failed\n");
        return ret;
    }
    switch (framesize->type)
    {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
            do{
            framesize->pixel_format = FRAME_FORMAT;
            ret = ioctl(fd_camera, VIDIOC_ENUM_FRAMESIZES, framesize);
            printf("2:enum frame size failed\n");
            framesize++;
            }while(!ret);
            break;
        case V4L2_FRMSIZE_TYPE_STEPWISE:
            break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
            break;
    }
    return 0;
}


int enum_frameinterval_camera(struct v4l2_frmivalenum* frame_interval)
{
    int ret;
    ret = ioctl(fd_camera, VIDIOC_ENUM_FRAMEINTERVALS, frame_interval);
    if (ret) {
                printf("1:driver support frame rate %dfps\n",(frame_interval->discrete.denominator) / (frame_interval->discrete.numerator));
                frame_interval++;
        return ret;
    }
    switch (frame_interval->type)
    {
        case V4L2_FRMIVAL_TYPE_DISCRETE:
            do{
                printf("driver support frame rate %dfps\n",(frame_interval->discrete.denominator) / (frame_interval->discrete.numerator));
                frame_interval++;
                ret = ioctl(fd_camera, VIDIOC_ENUM_FRAMEINTERVALS, frame_interval);
            }while(!ret);
            break;
        case V4L2_FRMIVAL_TYPE_CONTINUOUS:
            break;
        case V4L2_FRMIVAL_TYPE_STEPWISE:
            break;
    }
    printf("frame rate detect over.\n");
    return 0;
}




int set_format_camera(struct v4l2_format *format)
{
    int ret;
    unsigned int size;
    ret = ioctl(fd_camera, VIDIOC_S_FMT, format);

    if(!ret)
    {
        printf("calc_buffer_size..\n");
        size = calc_buffer_size(format->fmt.pix.width, format->fmt.pix.height, format->fmt.pix.pixelformat);
        ret = init_buf(size);
        printf("init_buf ret %d\n",ret);
    }
    return ret;
}

int set_frame_rate_camera(struct v4l2_streamparm *parms)
{
    int ret;
    ret = ioctl(fd_camera, VIDIOC_S_PARM, parms);
    return ret;
}



int stream_on_camera(void)
{
    int ret = 0;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    frame_num = CAPTURE_NUM;

     printf("%d %s()\n",__LINE__,__FUNCTION__);
    pthread_mutex_init(&list_semi, NULL);
    printf("%d %s()\n",__LINE__,__FUNCTION__);
    ret = ioctl(fd_camera, VIDIOC_STREAMON, &type);
    printf("%d %s()\n",__LINE__,__FUNCTION__);
    if (ret)
    return ret;
    thread_state = THREAD_RUN;
    ret = pthread_create(&dq_thread_id, NULL, dq_thread_routine, NULL);
    printf("%d %s()\n",__LINE__,__FUNCTION__);
    if (ret)
    {
        thread_state = THREAD_STOP;
        return ret;
  }
    ret = pthread_create(&q_thread_id, NULL, q_thread_routine, NULL);
    printf("%d %s()\n",__LINE__,__FUNCTION__);
    if (ret)
    {
        thread_state = THREAD_STOP;
        pthread_join(dq_thread_id, NULL);
        return ret;
  }
  printf("EXIT %s\n",__FUNCTION__);
  return ret;

}

int stream_off_camera(void)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    pthread_join(dq_thread_id, NULL);
    pthread_join(q_thread_id, NULL);

    while (!list_empty(&qlist))   //清空qlist，将所有的data_buf转移到dqlist
    {
            struct data_buf *pbuffer;

            pbuffer = list_entry(qlist.next,
                           struct data_buf, list);
            list_del_init(&(pbuffer->list));
            list_add_tail(&(pbuffer->list), &dqlist);
    }
    return ioctl(fd_camera, VIDIOC_STREAMOFF, &type);
}








 
 
 
