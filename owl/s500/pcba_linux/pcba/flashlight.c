#include "case.h"

/*
typedef struct {
    unsigned int type;			//自动or手动
    unsigned int index;       	//号码
	unsigned int position;		//位置
	int enable;					//是否可用
    char name[20];         		//测试名字
	char dev_name[60];			//驱动名字			
	char nod_path[60];			//参数
    char init_string[50];       
	char pass_string[100];
	char fail_string[50];
	IDirectFBSurface *surface;
}case_t;
*/

void test_flashlight(case_t *flashlight_case)
{
	int interval_time = 1000000;
	int light_time = 1000000;
	char i;
	char command[100];
	if(flashlight_case->enable)
	{
		while(true)
		{
			strcpy(command, "/sys/bus/platform/devices/flashlight.0/status");
			if(!is_nod_exists(command))
			{
				printf("can't operate flashlight nod\n");
				return;
			}
			strcpy(command, "echo 0 > /sys/bus/platform/devices/flashlight.0/status");
			printf("command = %s\n", command);
			pcba_system(command);
			
			direct_thread_sleep(light_time);
			
			strcpy(command, "echo 1 > /sys/bus/platform/devices/flashlight.0/status");
			printf("command = %s\n", command);
			pcba_system(command);
			
			direct_thread_sleep(interval_time);
		}
	}
}
	
