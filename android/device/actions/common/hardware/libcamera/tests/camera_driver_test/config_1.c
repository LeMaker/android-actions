

#ifndef _CONFIG_1_H
#define _CONFIG_1_H

#include <linux/videodev2.h>
#include "buf_op.h"
#include "display.h"
#include "device_op.h"
#include "video_dev.h"



void load_config(void)
{
//camera设备对应的设备名
DEV_CAMERA_NAME = DEV_CAMERA_BACK_NAME;


DEV_CAMERA_FRONT_NAME = "/dev/video3";
DEV_CAMERA_BACK_NAME = "/dev/video0";


//vout设备对应的设备名
DEV_VIDEO_NAME = "/dev/video1";

//用来采集数据的buffer数目
BUF_COUNT  = 5;


 
/**采用的io方式
*  IO_METHOD_MMAP
*  IO_METHOD_USERPTR
*/
io = IO_METHOD_USERPTR;


 
//需要从camera获取的总帧数
 CAPTURE_NUM = 30;
 
// //需要预览的帧数
// TOTAL_PREVIEW_NUM = 300; 
 
///*测试每种功能时的模式
//*  MODE_ALL：测试驱动所支持的每种功能的所有不同模式
//*  MODE_DEFAULT:仅测试驱动内部配置的每种功能的默认模式
//*/
//OPTION_TESTMODE = MODE_ALL;




/*camera输出的帧数据格式
*   V4L2_PIX_FMT_YUV420:
*   V4L2_PIX_FMT_YUV422P:
*   V4L2_PIX_FMT_NV12: YUV420 semi planar
*   V4L2_PIX_FMT_YUYV:
*   V4L2_PIX_FMT_SBGGR8:
*/
FRAME_FORMAT = V4L2_PIX_FMT_YUV420;



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
PROCESS_MODE = MODE_SAVEFILE;



//帧数据保存路径
SAVEFILE_PATH = "/data";



SCREEN_WITDH = 800;
SCREEN_HEIGHT = 480;



}







#endif //_CONFIG_1_H
