#include <stdlib.h>
#include <utils/Log.h>
#include "tvout_display_test.h"
#include "hdmi_tx_drv.h"
using namespace android;	

#define TEST_DISPLAY_MODE
//#define TEST_DISPLAY_PLUG
//#define TEST_TV_SCALE
//#define TEST_READ_EDID

#define FLAG_TVOUT_SIMULTANEOUS 0x1
#define FLAG_LCD_HAS_VIDEO 0x2
#define FLAG_MIN_BANDWIDTH 0x4
/*this macro should keep the same with DisplayManager.java*/
#define DISPLAY_MODE_LOW_BANDWIDTH (FLAG_MIN_BANDWIDTH)
#define DISPLAY_MODE_MEDIUM_BANDWIDTH (FLAG_MIN_BANDWIDTH | FLAG_LCD_HAS_VIDEO)
#define DISPLAY_MODE_HIGH_BANDWIDTH \
	(FLAG_TVOUT_SIMULTANEOUS | FLAG_LCD_HAS_VIDEO | FLAG_MIN_BANDWIDTH)

/*display mode
 MODE_DISP_SYNC_DEFAULT_TV_GV_LCD_GV:0
 
 MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_GV_LCD_GV:1
 MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_GV_LCD_G:17
MODE_DISP_DOUBLE_DEFAULT_NO_SYNC_TV_V_LCD_G:33

MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_GV_LCD_GV:3
MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_GV_LCD_G:19
MODE_DISP_DOUBLE_DEFAULT_SYNC_EXT_TV_V_LCD_G:35*/

int main(int argc, char **argv)
{
	ALOGD("enter test_tvout_display\n");

	actions_server_DisplayService_init();
/*1.test tvout display*/
#ifdef TEST_DISPLAY_MODE
	if (argc == 5) {
		//set cvbs format
		if (strcmp("!", argv[1])) {
			ALOGD("argv[1]:%s\n", argv[1]);
			actions_server_DisplayService_setCvbsParam(argv[1]);
		}
		
		//set hdmi vid
		if (strcmp("!", argv[2])) {
			ALOGD("argv[2]:%s\n", argv[2]);
			if (!actions_server_DisplayService_setHdmiParam(atoi(argv[2]))) {
					ALOGD("actions_server_DisplayService_setHdmiParam error\n");
					return -1;
			}
		}
		
		//set display mode
		if (strcmp("!", argv[3])) {
			ALOGD("argv[3]:%s\n", argv[3]);
				actions_server_DisplayService_setDisplaySingleMode(atoi(argv[3]));
		}

		//set displayer
		if (strcmp("!", argv[4])) {
			ALOGD("argv[4]:%s\n", argv[4]);
			actions_server_DisplayService_setOutputDisplayer1(argv[4]);
		}
	}
#endif

/*2.test tv scale*/
#ifdef TEST_TV_SCALE
		int xScale = atoi(argv[1]);
		int yScale = atoi(argv[2]);
		actions_server_DisplayService_setTvScale(xScale, yScale);
#endif

/*3.test read edid*/
#ifdef TEST_READ_EDID
	actions_server_DisplayService_getHdmiCap();
#endif

/*4.test display plug*/
#ifdef TEST_DISPLAY_PLUG
	actions_server_DisplayService_cable_monitor();
#endif
	return 0;
}
