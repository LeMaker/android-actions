





#ifndef _VIDEO_DEV_H
#define _VIDEO_DEV_H

#include "buf_op.h"

extern int fd_video;
extern char *DEV_VIDEO_NAME;
extern unsigned int SCREEN_WITDH;
extern unsigned int SCREEN_HEIGHT;


extern int init_device_video(void);
extern int uninit_device_video(void);
extern int disp_frame(struct data_buf *pbuf);



#endif //_VIDEO_DEV_H




















