#include "case.h"

void print_case(case_t *c, int case_num)
{
	int i;
	for(i = 0; i < case_num; i++)
	{
		printf("case %d  type = %d index = %d position = %d enable = %d\n", i, c[i].type, c[i].index, c[i].position, c[i].enable);
		printf("case %d name = %s\n", i, c[i].name);
		printf("case %d dev_name = %s\n", i, c[i].dev_name);
		printf("case %d nod_path = %s\n", i, c[i].nod_path);
		printf("case %d init_string = %s\n", i, c[i].init_string);
		printf("case %d pass_string = %s\n", i, c[i].pass_string);
		printf("case %d fail_string = %s\n", i, c[i].fail_string);
	}
}

int init_case(case_t *c, int case_num, int auto_num)
{
	int i;
	
	for(i = 0; i < case_num; i++)
	{
		if(i < auto_num)
			c[i].type = 0;
		else
			c[i].type = 1;
		c[i].index = i - c[i].type * auto_num;
		c[i].position = 0;
		c[i].enable = 0;
		c[i].doubleline = 0;
		strcpy(c[i].nod_path, "");
		strcpy(c[i].dev_name, "");
		switch(i)
		{
			case wifi:
			case bt:
				strcpy(c[i].init_string, "搜索AP中");
				break;
			case sdcard:
			case usb:
			case usbpc:
			case hdmi:
			case headphone:
				strcpy(c[i].init_string, "等待插入");
				break;
			case mtv:
				strcpy(c[i].init_string, "Searching");
				break;
			case key:
			case charge:
				strcpy(c[i].init_string, "");
				break;
			default:
				strcpy(c[i].init_string, "测试中");
		}
		strcpy(c[i].pass_string, "PASS");
		strcpy(c[i].fail_string, "FAIL");
		c[i].surface = NULL;
	}
	
	//fill name
	strcpy(c[mem].name, "内存测试");
	strcpy(c[ddrsize].name, "内存容量");
	strcpy(c[flash].name, "内置存储测试");
	strcpy(c[wifi].name, "WIFI测试");
	strcpy(c[bt].name, "蓝牙测试");
	strcpy(c[gsensor].name, "重力传感器");
	strcpy(c[gyro].name, "陀螺仪");
	strcpy(c[comp].name, "地磁仪");
	strcpy(c[lightsensor].name, "光感测试");
	strcpy(c[rtc].name, "rtc测试");
	strcpy(c[gps].name, "gps测试");
	strcpy(c[sdcard].name, "插卡测试");
	strcpy(c[usb].name, "USB连接");
	strcpy(c[usbpc].name, "PC连接");
	strcpy(c[hdmi].name, "HDMI连接");
	strcpy(c[headphone].name, "耳机连接");
	strcpy(c[key].name, "按键测试");
	strcpy(c[onoff].name, "开关键测试");
	strcpy(c[charge].name, "充电测试");
	strcpy(c[mtv].name, "数字电视");
	strcpy(c[uart].name, "串口测试");
	strcpy(c[ethernet].name, "以太网测试");
	strcpy(c[tp].name, "tp");
	strcpy(c[global].name, "global");
	
	strcpy(c[hdmi].pass_string, "PASS(check TV)");
	
	//strcpy nod_path
	strcpy(c[mem].nod_path, "8M");
	strcpy(c[flash].nod_path, "/proc/nand/phy_cap");
	strcpy(c[rtc].nod_path, "/dev/video0");
	strcpy(c[gps].nod_path, "/dev/stpgps");
	strcpy(c[sdcard].nod_path, "/sys/block/mmcblk0");
	strcpy(c[usb].nod_path, "/sys/monitor/usb_port/status/udisk_connected");
	strcpy(c[usbpc].nod_path, "/sys/monitor/usb_port/status/pc_connected");
	strcpy(c[hdmi].nod_path, "/sys/class/switch/hdmi/state");
	strcpy(c[headphone].nod_path, "/sys/class/switch/h2w/state");
	
	//strcpy devname
	strcpy(c[key].nod_path, "atc260x-adckeypad");
	strcpy(c[onoff].nod_path, "atc260x_onoff");
}

// int main()
// {
	// case_t ccc[18];
	
	// init_case(ccc, 18, 8);
	// print_case(ccc, 18);
// }
