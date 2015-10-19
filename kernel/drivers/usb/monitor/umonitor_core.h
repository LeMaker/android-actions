/*! \cond USBMONITOR*/
/*********************************************************************************
*                            Module: usb monitor driver
*                (c) Copyright 2003 - 2008, Actions Co,Ld. 
*                        All Right Reserved 
*
* History:        
*      <author>      <time>       <version >    <desc>
*       houjingkun   2011/07/08   1.0         build this file 
********************************************************************************/ 
/*!
 * \file   umonitor_core.h
 * \brief  
 *      usb monitor detect opration headfile.
 * \author houjingkun
 * \par GENERAL DESCRIPTION:
 * \par EXTERNALIZED FUNCTIONS:
 *       null
 *
 *  Copyright(c) 2008-2012 Actions Semiconductor, All Rights Reserved.
 *
 * \version 1.0
 * \date  2011/07/08
 *******************************************************************************/
#ifndef _UMONITOR_CORE_H_
#define _UMONITOR_CORE_H_

#include  "umonitor_hal.h"

#define USB_ERR_PLACE               printk("err:%s,%d\n", __FILE__, __LINE__)

typedef struct umonitor_api_ops {
	/* wakeup_func -- wakeup usb monitor detect thread. */
	void (* wakeup_func) (void);

	#define MON_MSG_USB_B_IN		1
	#define MON_MSG_USB_B_OUT		2
	#define MON_MSG_USB_A_IN		3
	#define MON_MSG_USB_A_OUT		4
	#define MON_MSG_USB_CHARGER_IN		5
	#define MON_MSG_USB_CHARGER_OUT		6
	void (* putt_msg)(unsigned int msg); 
} umonitor_api_ops_t;



typedef struct umonitor_dev_status {
	usb_hal_monitor_t umonitor_hal;

	/* detect valid or not. 1--valid, 0--invalid. */
	volatile unsigned char detect_valid;
	/* 1--usb monitor is detecting, 0--not in detecting state. */
	volatile unsigned char detect_running;

	volatile unsigned char vbus_status;	/* detect vbus status. 1-- valid. */
	volatile unsigned char dc5v_status;	/* detect dc5v status. 1-- valid. */
	volatile unsigned char vbus_enable_power;	/* 1-- valid. */
	volatile unsigned char det_phase;	/* 0--device detecting, 1--host detecting. */
	/*
	 * confirm device state. 0--not detect, 1--vbus on detected, enable 500k, 
	 * 2--dp dm detect, 3-- dp dm debounce.
	 */
	volatile unsigned char device_confirm;

#define MAX_DETECT_SOF_CNT  50
	volatile unsigned char sof_check_times;

	/*
	 * confirm host state. 0--not detect, 1--dp,dm detected, 
	 * 2--dp dm debounce.
	 */
	volatile unsigned char host_confirm;
	volatile unsigned char usb_pll_on;	/* 1--on, 0--off. */
	/*
	 * 0 ~ 3 bit - detect device or host. refer to DET_USB_MODE_T. 
	 * 4 ~ 7 bit - vbus detect mode. refer to DET_VBUS_MODE_T.
	 * 8 ~ 11 bit - id detect mode. refer to DET_ID_MODE_T.
	 * 12 ~ 15 bit - dc5v detect mode. refer to DET_DC5V_MODE_T.
	 */
	volatile unsigned int detect_mode;
	volatile unsigned char reserv[2];
	volatile unsigned int dp_dm_status;	/* 00 or 11, connet to pc or charger; 01 or 10, connet with udisk. */
	/* 
	 * detect steps. device steps: USB_DEVICE_DETECT_STEPS 
	 * host steps: USB_HOST_DETECT_STEPS
	 */
	volatile int timer_steps;
	volatile unsigned int timer_interval;	/* msecond. */
	volatile int check_cnt;	/* total detect count. */

	/* message status that indicate what message has been send. */
#define MONITOR_B_IN            0
#define MONITOR_CHARGER_IN      1
#define MONITOR_A_IN            2
	volatile unsigned int message_status;
	//volatile unsigned int fix_bug_record; 

	umonitor_api_ops_t *core_ops;
	umon_port_config_t *port_config;
} umonitor_dev_status_t;

int umonitor_core_init(umonitor_api_ops_t *core_ops, umon_port_config_t *port_config, unsigned int base);

int umonitor_core_exit(void);

/* status: 1--enable detecting, 0--disable detecting. */
int umonitor_detection(unsigned int status);

/* this function is a timer func. */
void umonitor_timer_func(void);

/* 获取运行到下一次检测的时间间隔，返回值以毫秒为单位。 */
unsigned int umonitor_get_timer_step_interval(void);

unsigned int umonitor_get_run_status(void);

unsigned int umonitor_get_message_status(void);

void umonitor_printf_debuginfo(void);

int umonitor_vbus_power_onoff(int value);

int umonitor_core_suspend(void);

int umonitor_core_resume(void);

int umonitor_dwc_otg_init(void);
#endif  /* _UMONITOR_CORE_H_ */
/*! \endcond*/

