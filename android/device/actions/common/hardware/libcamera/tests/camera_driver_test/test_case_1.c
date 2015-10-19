

/****
*         本测试用例用于拍照流程的测试。
*主要测试点：
*  1.拍照模式下各功能的设定是否正常。
*    各功能包括：曝光模式、白平衡、亮度、特殊效果
*  2.拍照后屏显所拍照片，查看是否正常。
*  3.测试拍照的耗时，检查是否在合理范围。
*
*
*/
#include "interface.h"


int main()
{
  load_config();
  
  
  
//  camera_back_in_use();
//  camera_open();
//  camera_get_ctrls();
//  camera_set_ctrls_all_fast();
//  camera_set_ctrls_default_fast();
//  camera_get_framesize();
//  camera_set_format();
//  //camera_get_framerate();
// // camera_set_frame_rate();
//  camera_start();
//
////     camera_set_ctrls_all_slow();
////  camera_set_ctrls_default_slow();
//     camera_stop();
//     camera_close();
//  report_capture_time();
  
  
  
  camera_front_in_use();
  camera_open();
  camera_get_ctrls();
  camera_set_ctrls_all_fast();
  camera_set_ctrls_default_fast();
  camera_get_framesize();
  camera_set_format();
  camera_get_framerate();
     camera_set_frame_rate();
  camera_start();

//     camera_set_ctrls_all_slow();
//  camera_set_ctrls_default_slow();
     camera_stop();
     camera_close();
  report_capture_time();
  return 0;
  
  
}
