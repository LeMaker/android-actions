
#include <linux/delay.h>
#include <linux/platform_device.h>


#include "dsihw.h"
#include "dsi.h"

#define LONG_CMD_MODE 0x39
#define SHORT_CMD_MODE 0x05

#define POWER_MODE  0x01

void send_long_cmd(struct platform_device *pdev, char *pcmd, int cnt)
{
	dsihw_send_long_packet(pdev, LONG_CMD_MODE,cnt,(unsigned int *)pcmd,POWER_MODE);	
}

void send_short_cmd(struct platform_device *pdev, int cmd)
{
	dsihw_send_short_packet(pdev, SHORT_CMD_MODE,cmd,POWER_MODE);  
}

void send_short_cmd_1(struct platform_device *pdev, int cmd, int singnal)
{
	dsihw_send_short_packet(pdev, singnal,cmd,POWER_MODE);  
}


/*
1. 如需发长包命令
	首先调用start_long_cmd
	在发完最后一个长命令后调用end_long_cmd

2. 长包格式
	pakg[0] cmd
	pakg[1] data1
	pakg[2] data2
	.       .
	.       .
	send_long_cmd(pakg, 4); 第一个参数是指针，第二个参数是传输个数
	
3. 短包格式
	send_short_cmd(cmd);
	exp： send_short_cmd(0x11);
	
4. 需要用到毫秒延时
	mdelay(100); //100毫秒的延时
*/
void send_cmd(struct platform_device *pdev)
{
/*	char pakg[100];
	
	start_long_cmd();
	pakg[0] = 0xbf; 
	pakg[1] = 0x93; 
	pakg[2] = 0x61; 
	pakg[3] = 0xf4; 
	send_long_cmd(pakg, 4);
	pakg[0] = 0xbf; 
	pakg[1] = 0x93; 
	send_long_cmd(pakg, 2);	
	end_long_cmd();
	
*/	
	send_short_cmd(pdev, 0x11);
	mdelay(200);
	send_short_cmd(pdev, 0x29);
	mdelay(200);
	send_short_cmd_1(pdev, 0, 0x32);
	mdelay(200);
}


void send_cmd_test(struct platform_device *pdev)
{
	char pakg[100];
	
	pakg[0] = 0xbf; 
	pakg[1] = 0x93; 
	pakg[2] = 0x61; 
	pakg[3] = 0xf4; 
	send_long_cmd(pdev, pakg, 4);
	pakg[0] = 0xbf; 
	pakg[1] = 0x93; 
	send_long_cmd(pdev, pakg, 2);	
}
