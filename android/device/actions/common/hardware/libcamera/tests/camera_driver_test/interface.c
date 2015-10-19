


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include "interface.h"
#include "device_op.h"
#include "video_dev.h"
#include "display.h"

void camera_open(void)
{

    if(open_device_camera())
    errno_exit("camera_open");
}


void camera_close(void)
{
    if(close_device_camera())
    errno_exit("camera_close");
}

void camera_get_ctrls(void)
{
  int i;
  int ret;

  for(i = 0; i < query_array_size; i++)
  {
      ret = queryctrl_camera(&query_array[i]);
      if(ret)
      {
          errno_exit("camera_get_ctrls");
      }
  }
}

void camera_set_ctrls_all_fast(void)
{
  int i;
  int ret;
  for(i = 0; i< query_array_size; i++)
  {
      printf("%d %d\n",i,query_array_size);
      ret = setctrl_camera(&query_array[i], MODE_ALL_FAST);
      if(ret)
      {
          errno_exit("camera_set_ctrls_all_fast");
      }
      sleep(2);
  }
}

void camera_set_ctrls_all_slow(void)
{
  int i;
  int ret;
  for(i = 0; i< query_array_size; i++)
  {
      ret = setctrl_camera(&query_array[i], MODE_ALL_SLOW);
      if(ret)
      {
          errno_exit("camera_set_ctrls_all_slow");
      }
      sleep(2);
  }
}



void camera_set_ctrls_default_fast(void)
{
  int i;
  int ret;
  for(i = 0; i< query_array_size; i++)
  {
      ret = setctrl_camera(&query_array[i], MODE_DEFAULT_FAST);
      if(ret)
      {
          errno_exit("camera_set_ctrls_default_fast");
      }
      sleep(2);
  }
}

void camera_set_ctrls_default_slow(void)
{
  int i;
  int ret;
  for(i = 0; i< query_array_size; i++)
  {
      ret = setctrl_camera(&query_array[i], MODE_DEFAULT_SLOW);
      if(ret)
      {
          errno_exit("camera_set_ctrls_default_slow");
      }
      sleep(2);
  }
}



void camera_get_framesize(void)
{
    int ret;
    init_framesize_array();
    ret = enum_framesize_camera(framesize_array);
    if(ret)
    {
        errno_exit("camera_get_framesize");
    }
}


void camera_get_framerate(void)
{
    int ret;
    init_frame_interval_array();
    ret = enum_frameinterval_camera(frame_interval_array);
    if(ret)
    {
        errno_exit("camera_get_framerate");
    }
}

void camera_set_frame_rate(void)
{
    int ret;
    struct v4l2_streamparm parms;
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parms.parm.capture.timeperframe.numerator = frame_interval_array[FRAME_RATE_ID].discrete.numerator;
    parms.parm.capture.timeperframe.denominator = frame_interval_array[FRAME_RATE_ID].discrete.denominator;
    parms.parm.capture.extendedmode = 0;

  printf("set frame rate: %dfps\n",(parms.parm.capture.timeperframe.denominator)/(parms.parm.capture.timeperframe.numerator));
    ret = set_frame_rate_camera(&parms);
    if(ret)
    {
        errno_exit("camera_set_param");
    }
}





void camera_set_format(void)
{
    int ret;
    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = framesize_array[FRAME_SIZE_ID].discrete.width;
    format.fmt.pix.height = framesize_array[FRAME_SIZE_ID].discrete.height;
    format.fmt.pix.pixelformat = FRAME_FORMAT;
    printf("width%d height%d  pixelformat%x\n",format.fmt.pix.width,format.fmt.pix.height, format.fmt.pix.pixelformat);
    ret = set_format_camera(&format);
    if(ret)
    {
        errno_exit("camera_set_format");
    }
}





void camera_start(void)
{
    printf("enter %s\n",__FUNCTION__);
    int ret;
    if (PROCESS_MODE == MODE_DISP)
    {
        ret = init_device_video();
        if (ret)
        {
            errno_exit("init_device_video");
        }
    }
    ret = stream_on_camera();
    if(ret)
    {
        errno_exit("stream_on_camera");
    }
}

void camera_stop(void)
{
    int ret;
  printf("enter %s\n",__FUNCTION__);
    ret = stream_off_camera();
    if(ret)
    {
        errno_exit("stream_off_camera");
    }
    if (PROCESS_MODE == MODE_DISP)
    {
        ret = uninit_device_video();
        if (ret)
        {
            errno_exit("uninit_device_video");
        }
    }
}

void report_capture_time(void)
{

}

void report_frame_rate(void)
{

}

void camera_front_in_use(void)
{
    DEV_CAMERA_NAME = DEV_CAMERA_FRONT_NAME;
    CAMERA_TYPE = "front";

}

void camera_back_in_use(void)
{
    DEV_CAMERA_NAME = DEV_CAMERA_BACK_NAME;
    CAMERA_TYPE = "back";
}







