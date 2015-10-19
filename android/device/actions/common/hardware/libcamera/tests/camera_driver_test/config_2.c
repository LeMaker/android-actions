

#ifndef _CONFIG_1_H
#define _CONFIG_1_H

#include "buf_op.h"
#include "display.h"
#include "device_op.h"


void load_config(void)
{
//camera设备对应的设备名
DEV_CAMERA_NAME = "/dev/video0";

//用来采集数据的buffer数目
 BUF_COUNT  = 6;
 //采用的io方式
 io = IO_METHOD_USERPTR;
 //将camera驱动dq出的第几帧数据作为拍照数据
 CAPTURE_NUM = 300;
 
// //需要预览的帧数
// TOTAL_PREVIEW_NUM = 300; 
 
/*测试每种功能时的模式
*  MODE_ALL：测试驱动所支持的每种功能的所有不同模式
*  MODE_DEFAULT:仅测试驱动内部配置的每种功能的默认模式
*/
OPTION_TESTMODE = MODE_ALL_FAST;

/*camera输出的帧数据格式
*   V4L2_PIX_FMT_YUV420:
*   V4L2_PIX_FMT_YUV422P:
*   V4L2_PIX_FMT_NV12:
*   V4L2_PIX_FMT_YUYV:
*   V4L2_PIX_FMT_SBGGR8:
*/
FRAME_FORMAT = V4L2_PIX_FMT_YVU420;

//用于确定所获取的帧大小，为framesize_array[]数组的下标
FRAME_SIZE_ID = 0;


//设置帧率，为frame_interval_array[]数组的下标
FRAME_RATE_ID = 0;

/*处理帧数据的方式
*
*   MODE_DISP：送到lcd显示
*   MODE_SAVEFILE：保存文件
*
*/
PROCESS_MODE = MODE_DISP;
//帧数据保存路径
DEV_CAMERA_NAME = DEV_CAMERA_NAME;

}







#endif //_CONFIG_1_H
