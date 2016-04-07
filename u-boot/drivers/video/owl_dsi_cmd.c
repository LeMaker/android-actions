/*
 * owl_dsi.c - OWL display driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_lcd.h>
#include <asm/io.h>
#include <asm/arch/pwm_backlight.h>
#include <asm/gpio.h>

#include <common.h>
#include <malloc.h>

#include <video_fb.h>
#include <owl_dss.h>
#include <owl_dsi.h>
#include <linux/list.h>
#include <linux/fb.h>

DECLARE_GLOBAL_DATA_PTR;

#define PICOS2KHZ(a) (1000000000UL/(a))
#define KHZ2PICOS(a) (1000000000UL/(a))


#define LONG_CMD_MODE 0x39
#define SHORT_CMD_MODE 0x05

#define POWER_MODE  0x01

void send_long_cmd(char *pcmd, int cnt)
{
	dsihw_send_long_packet(LONG_CMD_MODE,cnt,(unsigned int *)pcmd,POWER_MODE);	
}

void send_short_cmd(int cmd)
{
	dsihw_send_short_packet(SHORT_CMD_MODE,cmd,POWER_MODE);  
}

void send_short_cmd_1(int cmd, int singnal)
{
	dsihw_send_short_packet(singnal,cmd,POWER_MODE);  
}

/*


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
void send_cmd(void)
{
	char pakg[100];
/*	
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
	send_short_cmd(0x11);
	mdelay(200);
	send_short_cmd(0x29);
	send_short_cmd_1(0, 0x32);
	mdelay(200);
}

