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
 * \file   umonitor_core.c
 * \brief  
 *      usb monitor detect opration code.
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <mach/hardware.h>
#include <linux/io.h>

#include "aotg_regs.h"
#include "umonitor_config.h"
#include "umonitor_core.h"


enum {
	USB_DET_NONE = 0,	/* nothing detected, maybe B plus is out. */
	USB_DET_DEVICE_DEBUOUNCING,	/* detected device, debouncing and confirming. */
	USB_DET_DEVICE_PC,	/* detected device confirmed. pc connected. */
	USB_DET_DEVICE_CHARGER	/* detected device confirmed. charger connected. */
};

enum {
	USB_DET_HOST_NONE = 0,	/* nothing detected. maybe udisk is plug out. */
	USB_DET_HOST_DEBOUNCING,	/* detecting host, debouncing and confirming. */
	USB_DET_HOST_UDISK	/* detected udisk confirmed. udisk connected. */
};

#define USB_DEVICE_DETECT_STEPS    4
#define USB_HOST_DETECT_STEPS      4
#define USB_MONITOR_DEF_INTERVAL   500	/* interval to check usb port state, unit: ms. */

umonitor_dev_status_t *umonitor_status;
static int usb_monitor_debug_status_inf( void )
{
#if 0
	umonitor_dev_status_t *pStatus = umonitor_status;

	printk(KERN_INFO ".det_phase %d %d %d %d %d \n",
	       (unsigned int) pStatus->det_phase,
	       (unsigned int) pStatus->vbus_status,
	       (unsigned int) pStatus->timer_steps,
	       (unsigned int) pStatus->host_confirm,
	       (unsigned int) pStatus->message_status);
	printk(KERN_INFO "-----------------------------\n");
	printk(KERN_INFO ".vbus_status %d %x \n", (unsigned int) pStatus->vbus_status,
	       (unsigned int) pStatus->vbus_status);
	printk(KERN_INFO ".vbus_enable_power %d\n", (unsigned int) pStatus->vbus_enable_power);
	printk(KERN_INFO ".det_phase %d \n", (unsigned int) pStatus->det_phase);
	printk(KERN_INFO ".device_confirm %d\n", (unsigned int) pStatus->device_confirm);
	printk(KERN_INFO ".host_confirm %d \n", (unsigned int) pStatus->host_confirm);
	printk(KERN_INFO ".usb_pll_on %d \n", (unsigned int) pStatus->usb_pll_on);
	printk(KERN_INFO ".dp_dm_status %d 0x%x \n", (unsigned int) pStatus->dp_dm_status,
	       (unsigned int) pStatus->dp_dm_status);
	printk(KERN_INFO ".timer_steps %d \n", (unsigned int) pStatus->timer_steps);
	printk(KERN_INFO ".timer_interval %d \n", (unsigned int) pStatus->timer_interval);
	printk(KERN_INFO ".check_cnt %d \n", (unsigned int) pStatus->check_cnt);
	printk(KERN_INFO ".sof_check_times %d\n", (unsigned int) pStatus->sof_check_times);
	printk(KERN_INFO "\n \n ");
#endif
	return 0;
}

static int usb_init_monitor_status(umonitor_dev_status_t * pStatus)
{
  
	pStatus->detect_valid = 0;
	pStatus->detect_running = 0;
	pStatus->vbus_status = 0;
	pStatus->dc5v_status = 0;
	pStatus->det_phase = 0;
	pStatus->device_confirm = 0;
	pStatus->sof_check_times = 0;
	pStatus->host_confirm = 0;
	pStatus->usb_pll_on = 0;
	pStatus->dp_dm_status = 0;
	pStatus->timer_steps = 0;
	pStatus->timer_interval = USB_MONITOR_DEF_INTERVAL;
	pStatus->check_cnt = 0;
	pStatus->message_status = 0;
	pStatus->core_ops = NULL;
		
	pStatus->vbus_enable_power = 0;
	
	return 0;
}

/* 获取运行到下一次检测的时间间隔，返回值以毫秒为单位。 */
unsigned int umonitor_get_timer_step_interval(void)
{
	umonitor_dev_status_t *pStatus;

	pStatus = umonitor_status;

	if ((pStatus->port_config->detect_type == UMONITOR_DISABLE)
		|| (pStatus->detect_valid == 0)) {
		return 0x70000000;	/* be longer enough that it would not run again. */
	}

	if (pStatus->timer_steps == 0) {
		//pStatus->timer_interval = USB_MONITOR_DEF_INTERVAL;
		pStatus->timer_interval = 30;	/*重新进入step 0 检查 */
		goto out;
	}

	if (pStatus->det_phase == 0) {
		switch (pStatus->timer_steps) {
			/* 
			 * 1－3步，检测时间间隔500 ms（可调整），一旦端口有变化马上调到第四步，
			 * 进入debounce和confirm状态。
			 */
		case 1:
		case 2:
		case 3:
			pStatus->timer_interval = USB_MONITOR_DEF_INTERVAL;
			break;

		case 4:
			switch (pStatus->device_confirm) {
			case 0:	/* 进入第4步，下一步是检测端口vbus有无变化。 */
				pStatus->timer_interval =
				    USB_MONITOR_DEF_INTERVAL;
				break;

			case 1:	/* 已经检测到端口vbus有电，需要下一步再确认一次。 */
				pStatus->timer_interval = 10;	/* 10 ms, 1 tick. */
				break;

				/*
				 * device_confirm == 2, 已经确认vbus有电，并enable上拉500k，
				 * disable下拉15k，下一步检测dp、dm状态。 
				 */
			case 2:
				pStatus->timer_interval = 300;
				break;

				/* 第一次获取dp、dm状态，需要下一次再确认一次。 */
			case 3:
				pStatus->timer_interval = 30;
				break;

				/* 
				 * 进入这一步，需要多次判断pc是否有sof或reset信号给小机，这里每20毫秒查询sof中断一次，
				 * 累积会查询 MAX_DETECT_SOF_CNT 次，等待时间可能会有8秒以上。 
				 * 因为从小机dp上拉到pc发sof，中间可能延时长达8秒以上。
				 */
				/* wait sof again time interval, the whole detect sof time is (20 * sof_check_times) msecond. */
			case 4:
				pStatus->timer_interval = 20;
				break;

			default:
				USB_ERR_PLACE;
				pStatus->timer_interval =
				    USB_MONITOR_DEF_INTERVAL;
				break;
			}
			break;

		default:
			USB_ERR_PLACE;
			pStatus->timer_interval = USB_MONITOR_DEF_INTERVAL;
			break;
		}
	} else {
		switch (pStatus->timer_steps) {
		case 1:	/* 从step 0的idle状态切换到vbus对外供电的时间间隔。 */
			pStatus->timer_interval = 30;
			break;

		case 2:	/* vbus对外供电后，到开始第2次检测，判断dp是否上拉的时间。 */
			pStatus->timer_interval = 600;
			break;

		case 3:	/* 第2次检测到第3次检测之间，判断dp是否上拉的时间。 */
			pStatus->timer_interval = 600;
			break;

		case 4:
			switch (pStatus->host_confirm) {
			case 0:
				pStatus->timer_interval =
				    USB_MONITOR_DEF_INTERVAL;
				break;

			case 1:	/* debounce time. */
				pStatus->timer_interval = 10;	/* 10 ms, 1 tick. */
				break;

			default:
				USB_ERR_PLACE;
				pStatus->timer_interval =
				    USB_MONITOR_DEF_INTERVAL;
				break;
			}
			break;

		default:
			USB_ERR_PLACE;
			pStatus->timer_interval = USB_MONITOR_DEF_INTERVAL;
			break;
		}
	}

out:
	return pStatus->timer_interval;
}

/* 
 * retval:
 * refer to below macro:
 *    USB_DET_NONE,
 *    USB_DET_DEVICE_DEBUOUNCING,
 *    USB_DET_DEVICE_PC,
 *    USB_DET_DEVICE_CHARGER,  
 */
static int usb_timer_det_pc_charger(umonitor_dev_status_t * pStatus)
{
	int ret = 0;
	unsigned int val = 0;
	usb_hal_monitor_t *p_hal = &pStatus->umonitor_hal;
	
	MONITOR_PRINTK("entring usb_timer_det_pc_charger\n");

	if (pStatus->device_confirm == 0) {
		/* make sure power off. */
		if (pStatus->vbus_enable_power != 0) {
			p_hal->vbus_power_onoff(p_hal, 0);
			pStatus->vbus_enable_power = 0;
			p_hal->set_soft_id(p_hal, 1, 1);
		}
	}

	pStatus->vbus_status = (unsigned char) p_hal->get_vbus_state(p_hal);

	if (pStatus->vbus_status == USB_VBUS_HIGH) {
      MONITOR_PRINTK("vbus is high!!!!!!!\n");
		/* 
		 * if B_IN is send out, needn't check device at all. 
		 * 我们只处理先插入充电器的情况下，检测pc的连接和断开。如果先连接pc，则对于充电器的插拔检测不了。
		 */
		if ((pStatus->message_status & (0x1 << MONITOR_B_IN)) != 0) {
#if 0
			/*
			 * 此段代码本为检测圆口充电器和pc同时插入的情况下检测pc的拔线，但实际上存在误判断pc拔线的情况，
			 * 因为pc的sof可能在多次发送未果后一段时间内不再发送。
			 */
			/* if pc is connected, and charger is new plug in, we ignore it. */
			if ((pStatus->message_status &
			     (0x1 << MONITOR_CHARGER_IN)) == 0)
#endif
			pStatus->device_confirm = 0;
			pStatus->timer_steps = 0;
			ret = USB_DET_DEVICE_PC;
			goto out2;
		}
		if ((pStatus->message_status & (0x1 << MONITOR_CHARGER_IN)) != 0) {
			pStatus->device_confirm = 0;
			pStatus->timer_steps = 0;
			ret = USB_DET_DEVICE_CHARGER;
			goto out2;
		}

		switch (pStatus->device_confirm) {
			/* 进入第4步，检测到端口vbus有电。至少deboundce一次，确保检测正确。 */
		case 0:
			pStatus->timer_steps = USB_DEVICE_DETECT_STEPS;	/* the last timer_steps is to confirm. */
			pStatus->device_confirm = 1;
			ret = USB_DET_DEVICE_DEBUOUNCING;
			goto out2;

			/* 已经确认vbus有电，并enable上拉500k，disable下拉15k，下一步检测dp、dm状态。 */
		case 1:
			p_hal->set_dp_500k_15k(p_hal, 1, 0);	/* 500k up enable, 15k down disable; */
			pStatus->device_confirm = 2;
			ret = USB_DET_DEVICE_DEBUOUNCING;
			goto out2;

			/* 第一次获取dp、dm状态，需要再确认一次。 */
		case 2:
			pStatus->dp_dm_status = p_hal->get_linestates(p_hal);	// get dp dm status.
			pStatus->device_confirm = 3;
			//pStatus->device_confirm = 2;  /* always in get dp dm states, just for test. */
			ret = USB_DET_DEVICE_DEBUOUNCING;
			goto out2;

			/* 
			 * 第二次获取dp、dm状态，如果两次不变，则确认ok，否则进一步debounce。
			 * dp和dm非0状态为充电器，反之进一步判断sof中断位看看是否pc。
			 */
		case 3:
			val = p_hal->get_linestates(p_hal);	// get dp dm status.
			pStatus->sof_check_times = 0;
			if (val == pStatus->dp_dm_status) {
				if (val == 0x00) {
					pStatus->timer_steps = 0;
					pStatus->device_confirm = 0;
					ret = USB_DET_DEVICE_PC;
					
					goto out2;
				} else {
					pStatus->device_confirm = 0;
					/* if enable monitor again, it should begin from step 0.  */
					pStatus->timer_steps = 0;
					ret = USB_DET_DEVICE_PC;
					goto out2;
				}
			} else {
				pStatus->device_confirm = 1;
				ret = USB_DET_DEVICE_DEBUOUNCING;
				goto out2;
			}

			/* 
			 * 进入这一步，需要多次判断pc是否有sof或reset信号给小机，
			 * 等待时间可能会有8秒以上。 因为从小机dp上拉到pc发sof，中间可能延时长达8秒以上。
			 */
			/* for detect sof or reset irq. */
		case 4:
			val = p_hal->is_sof(p_hal);
			if (val != 0) {
				/* if enable monitor again, it should begin from step 0. */
				pStatus->timer_steps = 0;
				pStatus->device_confirm = 0;
				pStatus->sof_check_times = 0;
				p_hal->dp_down(p_hal);
				ret = USB_DET_DEVICE_PC;
				goto out2;
			}
			if (pStatus->sof_check_times < MAX_DETECT_SOF_CNT) {	/* 10 * MAX_DETECT_SOF_CNT ms. */
				pStatus->device_confirm = 4;	/* next step still check again. */
				pStatus->sof_check_times++;
				ret = USB_DET_DEVICE_DEBUOUNCING;
				goto out2;
			}

			/* if enable monitor again, it should begin from step 0. */
			pStatus->timer_steps = 0;
			pStatus->device_confirm = 0;
			pStatus->sof_check_times = 0;
			p_hal->dp_down(p_hal);
			/* treated as charger in. */
			ret = USB_DET_DEVICE_CHARGER;
			goto out2;

		default:
			MONITOR_ERR("into device confirm default, err!\n");
			pStatus->device_confirm = 0;
			ret = USB_DET_NONE;
			goto out;
		}
	} else {	  
		pStatus->device_confirm = 0;
		pStatus->timer_steps =USB_DEVICE_DETECT_STEPS;
		ret = USB_DET_NONE;
		goto out;
	}

      out:
	pStatus->timer_steps++;
	if (pStatus->timer_steps > USB_DEVICE_DETECT_STEPS) {
		pStatus->timer_steps = 0;
	}
      out2:
	return ret;
}

/* 
 * retval:
 * refer to below macro:
 *    USB_DET_HOST_NONE,
 *    USB_DET_HOST_DEBOUNCING,
 *    USB_DET_HOST_UDISK,
 */
static int usb_timer_det_udisk(umonitor_dev_status_t * pStatus)
{
	unsigned int val;
	usb_hal_monitor_t *p_hal = &pStatus->umonitor_hal;

	if (pStatus->timer_steps == 1) {

		p_hal->set_dp_500k_15k(p_hal, 0, 1);	/* disable 500k, enable 15k. */

		if (pStatus->vbus_enable_power == 0) {
			p_hal->vbus_power_onoff(p_hal, 1);
			pStatus->vbus_enable_power = 1;
			p_hal->set_soft_id(p_hal, 1, 0);
		}
		goto out;
	} else {
		if (pStatus->vbus_enable_power != 1) {
			USB_ERR_PLACE;
		}
		   
		val = p_hal->get_linestates(p_hal);	// get dp dm status.
		MONITOR_PRINTK("host debounce!!!, linestate %04x\n", val);
		
    pStatus->timer_steps = 0;
    pStatus->host_confirm = 0;
    return USB_DET_HOST_UDISK;
		    
		if ((val == 0x1) || (val == 0x2)) {
			switch (pStatus->host_confirm) {
			case 0:
				pStatus->host_confirm = 1;
				/* the last step is always debounce and confirm step. */
				pStatus->timer_steps = USB_HOST_DETECT_STEPS;
				pStatus->dp_dm_status = val;
				return USB_DET_HOST_DEBOUNCING;

			case 1:
				if (val == pStatus->dp_dm_status) {
					/* if enable monitor again, it should begin from step 0.  */
					pStatus->timer_steps = 0;
					pStatus->host_confirm = 0;
					return USB_DET_HOST_UDISK;
				} else {
					pStatus->dp_dm_status = val;
					pStatus->host_confirm = 0;
					return USB_DET_HOST_DEBOUNCING;
				}

			default:
				break;
			}
		} else {
			pStatus->host_confirm = 0;
			goto out;
		}
	}

out:
	pStatus->timer_steps++;
	if (pStatus->timer_steps > USB_HOST_DETECT_STEPS) {
		pStatus->timer_steps = 0;
		return USB_DET_HOST_NONE;	/* nothing detect, maybe udisk is plug out. */
	}
	return USB_DET_HOST_DEBOUNCING;
}

/* 
 * 处理恢复到step 0（即第0步）的情况。step 0阶段不区分device检测还是host检测，
 * 它只是恢复到一个默认状态，为下一次device或host检测做准备。 
 */
static int usb_timer_process_step0(umonitor_dev_status_t * pStatus)
{
	int ret = 0;
	unsigned int status = 0;
	usb_hal_monitor_t *p_hal = &pStatus->umonitor_hal;
	
	MONITOR_PRINTK("entring usb_timer_process_step0\n");

	if ((pStatus->message_status & (0x1 << MONITOR_B_IN)) != 0) {
		MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
		printk("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
		pStatus->core_ops->putt_msg(MON_MSG_USB_B_OUT);
		pStatus->message_status =pStatus->message_status & (~(0x1 << MONITOR_B_IN));
	}

	if ((pStatus->message_status & (0x1 << MONITOR_A_IN)) != 0) {
		MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_A_OUT\n", __FILE__, __LINE__);
		printk("\n%s--%d, SYS_MSG_USB_A_OUT\n", __FILE__, __LINE__);
		pStatus->core_ops->putt_msg(MON_MSG_USB_A_OUT);
		pStatus->message_status = pStatus->message_status & (~(0x1 << MONITOR_A_IN));
	}

	/*
	 * 对于有id pin, 或用gpio检测idpin的情况, 当idpin为0, 则让它一直处于host检测状态,
	 * 不要让vbus掉电. 一直vbus供电,可以兼容插入mp3,mp4的情况. (因为这些设备有可能在供电
	 * 几十秒后dp才上拉。
	 */
	if (p_hal->config->detect_type == UMONITOR_DEVICE_ONLY) {
		ret = USB_ID_STATE_DEVICE;
	} else if (p_hal->config->detect_type == UMONITOR_HOST_ONLY) {
		ret = USB_ID_STATE_HOST;
	} else {
		ret = p_hal->get_idpin_state(p_hal);
	}
  MONITOR_PRINTK("idpin is %d\n", ret);
  

	if (ret != USB_ID_STATE_INVALID) {
		if (ret == USB_ID_STATE_HOST) {
host_detect:		  
		  MONITOR_PRINTK("host detecting!!!!\n");
			if ((pStatus->message_status & (0x1 << MONITOR_B_IN)) != 0) {
				MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
				printk("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
				pStatus->core_ops->putt_msg(MON_MSG_USB_B_OUT);
				pStatus->message_status =pStatus->message_status & (~(0x1 << MONITOR_B_IN));
			}
			if ((pStatus->message_status & (0x1 << MONITOR_CHARGER_IN)) != 0) {
				MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_CHARGER_OUT\n", __FILE__, __LINE__);
				pStatus->core_ops->putt_msg(MON_MSG_USB_CHARGER_OUT);
				pStatus->message_status =pStatus->message_status & (~(0x1 << MONITOR_CHARGER_IN));
			}
      
			p_hal->set_dp_500k_15k(p_hal, 0, 1);	/* disable 500k, enable 15k. */

			if (pStatus->vbus_enable_power == 0) {
				p_hal->vbus_power_onoff(p_hal, 1);
				pStatus->vbus_enable_power = 1;
				p_hal->set_soft_id(p_hal, 1, 0);
			}
			pStatus->det_phase = 1;
		} else {
		  MONITOR_PRINTK("device detect prepare!!!!\n");
			if ((pStatus->message_status & (0x1 << MONITOR_A_IN)) != 0) {
				MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_A_OUT\n", __FILE__, __LINE__);
				printk("\n%s--%d, SYS_MSG_USB_A_OUT\n", __FILE__, __LINE__);
				pStatus->core_ops->putt_msg(MON_MSG_USB_A_OUT);
				pStatus->message_status = pStatus->message_status & (~(0x1 << MONITOR_A_IN));
			}			
			if (pStatus->vbus_enable_power) {
			    p_hal->vbus_power_onoff(p_hal, 0);
			    pStatus->vbus_enable_power = 0;
			}
			p_hal->set_dp_500k_15k(p_hal, 0, 0);	/* disable 500k, disable 15k. */
			p_hal->set_soft_id(p_hal, 1, 1);

			pStatus->det_phase = 0;
		}
		pStatus->device_confirm = 0;
		pStatus->host_confirm = 0;
		pStatus->timer_steps = 1;
		goto out;
	}

	/* the last time check host state before change to device detect phase. */
	if ((pStatus->vbus_enable_power != 0) && (pStatus->det_phase != 0)) {
		pStatus->dp_dm_status = p_hal->get_linestates(p_hal);	// get dp dm status.
		if ((pStatus->dp_dm_status == 0x1) || (pStatus->dp_dm_status == 0x2)) {
			pStatus->timer_steps = USB_HOST_DETECT_STEPS;
			pStatus->host_confirm = 0;
			goto out;
		}
	}

	p_hal->vbus_power_onoff(p_hal, 0);
	pStatus->vbus_enable_power = 0;
	p_hal->set_dp_500k_15k(p_hal, 0, 0);	/* disable 500k, disable 15k. */
	p_hal->set_soft_id(p_hal, 1, 1);
	
	pStatus->check_cnt++;

	/* if it's the first time to check, must in checking device phase. */
	if ((pStatus->check_cnt == 1) ||
	    (pStatus->port_config->detect_type == UMONITOR_DEVICE_ONLY)) {
		pStatus->det_phase = 0;
	} else {
		/* reverse detect phase. */
		pStatus->det_phase = !pStatus->det_phase;

		/* if it's B_IN status, it needn't to check host in, because there is just one usb port. 
		   同时，如果只连接usb充电器，此时再使用GPIO对外供电来检查host是否插入，则会导致机器掉电。
		   一样也是需要此时禁止检测host */
		status = pStatus->message_status & ((0x1 << MONITOR_B_IN) | (0x1 << MONITOR_CHARGER_IN));
		if ((pStatus->det_phase == 1) && (status != 0)) {
			pStatus->det_phase = 0;
			goto out1;
		}
		pStatus->check_cnt = 0;
		goto host_detect;
		
	}
out1:
	pStatus->device_confirm = 0;
	pStatus->host_confirm = 0;
	pStatus->timer_steps = 1;

out:
	return 0;
}

/******************************************************************************/
/*!
* \brief  check whether usb plug in/out
*
* \par    Description
*         this function is a timer func, interval is 500ms.
*
* \param[in]  null
* \return     null
* \ingroup   usbmonitor
*
* \par
******************************************************************************/
void umonitor_timer_func(void)
{
	int ret = 0;
	unsigned int status = 0;
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t * p_hal;
	u32 reg;
    
	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;
	
	MONITOR_PRINTK("entring umonitor_timer_func\n");

	if ((pStatus->port_config->detect_type == UMONITOR_DISABLE)
		|| (pStatus->detect_valid == 0)) {
		goto out;
	}
	pStatus->detect_running = 1;

	/* err check! */
	if ((pStatus->timer_steps > USB_DEVICE_DETECT_STEPS)
	    && (pStatus->timer_steps > USB_HOST_DETECT_STEPS)) {
		MONITOR_ERR("timer_steps err:%d \n", pStatus->timer_steps);
		pStatus->timer_steps = 0;
		goto out;
	}
	//usb_monitor_debug_status_inf(usb_ctrl_no);

	if (pStatus->timer_steps == 0) {	/* power on/off phase. */
		usb_timer_process_step0(pStatus);
		goto out;
	}

	if (pStatus->det_phase == 0) {	/* power off, device detect phase. */
		ret = usb_timer_det_pc_charger(pStatus);
		switch (ret) {
		case USB_DET_NONE:
			if ((pStatus->message_status & (0x1 << MONITOR_B_IN)) != 0) {
				//MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
				printk("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
				pStatus->core_ops->putt_msg(MON_MSG_USB_B_OUT);
				pStatus->message_status =pStatus->message_status & (~(0x1 << MONITOR_B_IN));
			}
			if ((pStatus->message_status & (0x1 << MONITOR_CHARGER_IN)) != 0) {
				printk("\n%s--%d, SYS_MSG_USB_CHARGER_OUT\n", __FILE__, __LINE__);
				//MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_CHARGER_OUT\n", __FILE__, __LINE__);
				pStatus->core_ops->putt_msg(MON_MSG_USB_CHARGER_OUT);
				pStatus->message_status = pStatus->message_status & (~(0x1 << MONITOR_CHARGER_IN));
			}
			break;

		case USB_DET_DEVICE_DEBUOUNCING:	/* debounce. */
			break;

		case USB_DET_DEVICE_PC:
			if(p_hal->get_idpin_state(p_hal) != USB_ID_STATE_DEVICE){
				pStatus->device_confirm = 0;
				pStatus->timer_steps =0;
                		goto out;
			}
			status = pStatus->message_status & (0x1 << MONITOR_B_IN);
			if (status != 0) {
				goto out;
			}
			p_hal->set_mode(p_hal, USB_IN_DEVICE_MOD);
			//need to reset dp/dm before dwc3 loading
			reg = readl(p_hal->usbecs);
			reg &=  ~((0x1 << USB3_P0_CTL_DPPUEN_P0)|(0x1 << USB3_P0_CTL_DMPUEN_P0)); 
			writel(reg, p_hal->usbecs );
			//MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_B_IN\n", __FILE__, __LINE__);
			printk("\n%s--%d, SYS_MSG_USB_B_IN\n", __FILE__, __LINE__);
			pStatus->core_ops->putt_msg(MON_MSG_USB_B_IN);
			pStatus->message_status |= 0x1 << MONITOR_B_IN;
			pStatus->detect_valid = 0;	//disable detection
			goto out;	/* todo stop timer. */

		case USB_DET_DEVICE_CHARGER:
			/* if B_IN message not clear, clear it. B_OUT when adaptor is in. */
			status = pStatus->message_status & (0x1 << MONITOR_B_IN);
			if (status != 0) {
			  printk("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
				//MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_B_OUT\n", __FILE__, __LINE__);
				pStatus->core_ops->putt_msg(MON_MSG_USB_B_OUT);
				pStatus->message_status =pStatus->message_status & (~(0x1 << MONITOR_B_IN));
			}
			/* if adaptor in is send, it needn't sent again. */
			status = pStatus->message_status & (0x1 << MONITOR_CHARGER_IN);
			if (status != 0) {
				goto out;
			}
			p_hal->set_mode(p_hal, USB_IN_DEVICE_MOD);
			MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_CHARGER_IN\n", __FILE__, __LINE__);
			pStatus->core_ops->putt_msg(MON_MSG_USB_CHARGER_IN);
			pStatus->message_status |= 0x1 << MONITOR_CHARGER_IN;
			pStatus->detect_valid = 0;	//disable detection
			goto out;	/* todo stop timer. */

		default:
			USB_ERR_PLACE;
			break;
		}
		goto out;
	} else {		/* power on, host detect phase. */

		ret = usb_timer_det_udisk(pStatus);
		status = pStatus->message_status & (0x1 << MONITOR_A_IN);
		if ((status != 0) && (ret == USB_DET_HOST_NONE)) {
			//MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_A_OUT\n", __FILE__, __LINE__);
			printk("\n%s--%d, SYS_MSG_USB_A_OUT\n", __FILE__, __LINE__);
			pStatus->core_ops->putt_msg(MON_MSG_USB_A_OUT);
			pStatus->message_status = pStatus->message_status & (~(0x1 << MONITOR_A_IN));
			goto out;
		}
		if (ret == USB_DET_HOST_UDISK) {
			p_hal->set_mode(p_hal, USB_IN_HOST_MOD);
			//MONITOR_PRINTK("\n%s--%d, SYS_MSG_USB_A_IN\n", __FILE__, __LINE__);
			printk("\n%s--%d, SYS_MSG_USB_A_IN\n", __FILE__, __LINE__);
			pStatus->core_ops->putt_msg(MON_MSG_USB_A_IN);
			pStatus->message_status |= 0x1 << MONITOR_A_IN;

			/*如果是A线插入，则关闭定时器；B线插入，定时器不用关闭 */
			pStatus->detect_valid = 0;	//disable detection
			goto out;	/* todo stop timer. */
		}
		goto out;
	}
	
out:
	pStatus->detect_running = 0;
	return;
}

/******************************************************************************/
/*!
* \brief  set monitor detect flag
*
* \par    Description
*         set monitor detect flag
*
* \param[in]  status
*             1---   set detection flag to detect
*             0---   reverse
* \return     0------设置成功
* \ingroup   usbmonitor
*
* \par
******************************************************************************/
static int set_monitor_detect_flag(umonitor_dev_status_t *pStatus, unsigned int status)
{
	int i;
	unsigned int ms_status = 0;	/* record is a in ? */
	usb_hal_monitor_t *p_hal = &pStatus->umonitor_hal;

	pStatus->check_cnt = 0;
	pStatus->det_phase = 0;
	pStatus->timer_steps = 0;

	if (status != 0) {	/*enable detect flag */
		p_hal->vbus_power_onoff(p_hal, 0);
		pStatus->vbus_enable_power = 0;

		if (pStatus->detect_valid == 0) {
			MONITOR_PRINTK("%s,%d\n", __FUNCTION__, __LINE__);
			pStatus->detect_valid = 1;
			goto out;
		} else {
			MONITOR_PRINTK("usb detection flag is already setted, %s,%d\n", __FUNCTION__, __LINE__);
		}
	} 
	else {		/*disable detection flag */
		i = 0;
		do {
			if (pStatus->detect_running == 0) {
				pStatus->detect_valid = 0;
				break;
			}
			msleep(1);
			++i;
		} while (i < 1000);
		MONITOR_PRINTK("enable detection flag\n");
		
		if (ms_status == 0) {
			/* make sure power is off. */
			p_hal->vbus_power_onoff(p_hal, 0);
			pStatus->vbus_enable_power = 0;
			p_hal->set_soft_id(p_hal, 1, 1);
		}
	}

out:
	if (pStatus->core_ops->wakeup_func != NULL) {
		pStatus->core_ops->wakeup_func();
	}
	return 0;
}

/*! \cond USBMONITOR_INTERNAL_API*/
/******************************************************************************/
/*!
* \brief  enable or disable usb plug_in/out check
*
* \par    Description
*         enable or disable the func of checking usb plug_in/out
*
*
* \param[in]  status
*             1---   enable check func;
*             0---   disable check func;
* \return     0------使能/禁止成功
                负值---驱动当前忙，请稍候重新进行此操作
* \ingroup   usbmonitor
*
* \par
******************************************************************************/
int umonitor_detection(unsigned int status)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t * p_hal;


	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;
	MONITOR_PRINTK("umonitor_detection:%d\n", status);

	if (status != 0) {
		p_hal->dwc3_otg_mode_cfg(p_hal);
		p_hal->aotg_enable(p_hal, 1);
		p_hal->set_mode(p_hal, USB_IN_DEVICE_MOD);
		set_monitor_detect_flag(pStatus, 1);
	} else {
		//这部分现在已无效,由在已经检测到B或者A插入时,发送了消息后,将定时器port_timer停止;
		//checktimer仍旧运行监测A(id是否变化)或者B(vbus是否改变)插入的状态是否发生改变.
		//此处为保留原始代码,/*disable detection,加载UDC驱动时，先关闭定时器检测 */
		p_hal->aotg_enable(p_hal, 0);
		set_monitor_detect_flag(pStatus, 0);
	}
	return 0;
}

/*! \cond NOT_INCLUDE*/
/******************************************************************************/
/*!
* \brief  parse the runtime args of monitor driver.
* \par    Description
*         初始化，开始准备进行检测。
*
* \retval      0---args parse successed
* \ingroup     UsbMonitor
******************************************************************************/
int umonitor_core_init(umonitor_api_ops_t * core_ops,
		   umon_port_config_t * port_config , unsigned int base)
{
	umonitor_dev_status_t *pStatus;

	pStatus = kmalloc(sizeof (umonitor_dev_status_t), GFP_KERNEL);
	if (pStatus == NULL) {
		return -1;
	}
	umonitor_status = pStatus;

	usb_hal_init_monitor_hw_ops(&pStatus->umonitor_hal, port_config, base);
	usb_init_monitor_status(pStatus);
	pStatus->core_ops = core_ops;
	pStatus->port_config = port_config;

	return 0;
}

int umonitor_core_exit(void)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t *p_hal;

	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;

	p_hal->enable_irq(p_hal, 0);
	if (pStatus != NULL)
		kfree(pStatus);
		
	umonitor_status = NULL;
	return 0;
}

unsigned int umonitor_get_run_status(void)
{
	umonitor_dev_status_t *pStatus;

	pStatus = umonitor_status;
	
	return (unsigned int)pStatus->detect_valid;
}

unsigned int umonitor_get_message_status(void)
{
	umonitor_dev_status_t *pStatus;

	pStatus = umonitor_status;
	
	return (unsigned int)pStatus->message_status;
}

void umonitor_printf_debuginfo(void)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t *p_hal;

	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;

	usb_monitor_debug_status_inf();
	printk("in printf_debuginfo\n");
	p_hal->debug(p_hal);

	return;
}

int umonitor_vbus_power_onoff(int value)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t *p_hal;

	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;
	
	return p_hal->vbus_power_onoff(p_hal, value);
}

int umonitor_core_suspend(void)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t *p_hal;

	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;

  pStatus->detect_valid = 0;
  
  printk("SUSPEND pStatus->message_status is %d!!!!!!!!!!!!!!\n", pStatus->message_status);
  
	if(pStatus->vbus_enable_power && p_hal->vbus_power_onoff)
		p_hal->vbus_power_onoff(p_hal,  0);
	
  p_hal->suspend_or_resume(p_hal, 1);
	
	return 0;
}

int umonitor_core_resume(void)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t *p_hal;
	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;	
	
	printk(KERN_DEBUG"RESUME pStatus->message_status is %d!!!!!!!!!!!!!!\n", pStatus->message_status);

	if((pStatus->message_status &(0x1 << MONITOR_B_IN)) != 0){
		printk(KERN_DEBUG"RESUME SNED B_OUT\n");
		pStatus->core_ops->putt_msg(MON_MSG_USB_B_OUT);
			pStatus->message_status &= ~(0x1 << MONITOR_B_IN);
	}
	if((pStatus->message_status &(0x1 << MONITOR_A_IN)) != 0){
		printk(KERN_DEBUG"RESUME SNED A_OUT\n");
		//p_hal->vbus_power_onoff(p_hal,  1);
		//pStatus->vbus_enable_power = 1;
		pStatus->core_ops->putt_msg(MON_MSG_USB_A_OUT);
		pStatus->message_status &= ~(0x1 << MONITOR_A_IN);
	}
	p_hal->suspend_or_resume(p_hal, 0);
	umonitor_detection(1);
	return 0;
}

/*保证关机流程中才会调用,其它地方不会调用*/
int umonitor_dwc_otg_init(void)
{
	umonitor_dev_status_t *pStatus;
	usb_hal_monitor_t *p_hal;

	pStatus = umonitor_status;
	p_hal = &pStatus->umonitor_hal;

  p_hal->dwc3_otg_init(p_hal);
	
	return 0;
}
