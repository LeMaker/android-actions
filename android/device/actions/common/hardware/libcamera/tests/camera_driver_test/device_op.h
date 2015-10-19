/*
 *         camera device operate head file
 *  mainly including device open close.
 *
 *
 *
 */
 
#ifndef _DEVICE_OP_H
#define _DEVICE_OP_H

#include <linux/videodev2.h>

//extern int fd_camera;

typedef struct camera_info{
struct v4l2_format f;
}camera_info_t;

typedef enum{

/*设置默认属性，设置完成后会停留一段时间*/
MODE_DEFAULT_SLOW = 0,
/*设置驱动支持范围内的所有有效值，设置完成后会停留一段时间*/
MODE_ALL_SLOW,
/*设置默认属性，设置成功后不停留，直接进行一下功能设置*/
MODE_DEFAULT_FAST,
/*设置驱动支持范围内的所有有效值，设置成功后不停留，直接进行一下功能设置*/
MODE_ALL_FAST,
}option_mode;

extern struct v4l2_queryctrl query_array[];
extern struct v4l2_frmsizeenum framesize_array[];
extern struct v4l2_frmivalenum frame_interval_array[];
extern  int query_array_size;
extern char *DEV_CAMERA_NAME;
extern char *DEV_CAMERA_FRONT_NAME;
extern char *DEV_CAMERA_BACK_NAME;
extern int fd_camera;
extern int FRAME_FORMAT;
extern int FRAME_SIZE_ID;
extern int FRAME_RATE_ID;
extern option_mode OPTION_TESTMODE;


extern int open_device_camera(void);
extern int close_device_camera(void);
extern int queryctrl_camera(struct v4l2_queryctrl* query);
extern int setctrl_camera(struct v4l2_queryctrl *query_value, option_mode mode);
extern int stream_on_camera(void);
extern int stream_off_camera(void);
extern int enum_framesize_camera(struct v4l2_frmsizeenum* framesize);
extern int enum_frameinterval_camera(struct v4l2_frmivalenum* frame_interval);
extern int set_format_camera(struct v4l2_format *format);
extern int set_frame_rate_camera(struct v4l2_streamparm *parms);
extern void errno_exit(const char *s);

extern void init_framesize_array(void);

extern void init_frame_interval_array(void);





#endif  //_DEVICE_OP_H
