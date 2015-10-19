

/****
*         本测试用例用于预览流程的测试。
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
  camera_open();
  camera_get_ctrls();
  camera_set_params();
  camera_get_framesize();
  camera_set_format();
  camera_start();
  camera_stop();
  camera_close();
  report_frame_rate();
  return 0;
}
