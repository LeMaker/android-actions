/*
 * Asoc  irkeypad driver
 *
 * Copyright (C) 2011 Actions Semiconductor, Inc
 * Author:	chenbo <chenbo@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#define IR_PROTOCOL_9012    0x0
#define IR_PROTOCOL_NEC8    0x1
#define IR_PROTOCOL_RC5     0x2

/*debug related*/
#define IRKEYPAD_DEBUG			1
#define IRKEYPAD_REG_PRINT		1
#define IRKEYPAD_DEBUG_INFO		1
#define DEBUG_IRQ_HANDLER 		1
#define IRKEYPAD_SUPPORT_MOUSE  1
/*IR Power Key is Suspend Func */
#if (IRKEYPAD_DEBUG_INFO == 1)
	#define GL5201_IRKEYPAD_INFO(fmt, args...)	\
	printk(KERN_INFO "gl5201_irkeypad_drv: " fmt, ##args)
#else
	#define GL5201_IRKEYPAD_INFO(fmt, args...)
#endif

#define res_size(res)	((res)->end - (res)->start + 1)

#define IRC_STAT_UCMP 		(0x1 << 6)
#define IRC_STAT_KDCM 		(0x1 << 5)
#define IRC_STAT_RCD 		(0x1 << 4)
#define IRC_STAT_IIP 		(0x1 << 2)
#define IRC_STAT_IREP 		(0x1 << 0)

#define GPIO_NAME_POWER_LED "power_led"
//static struct gpio_pre_cfg power_led_cfg;
static int gpio_power_led_pin;
static bool power_led_exist;
static bool led_blink_enable = 0;


struct atc260x_dev *atc260x_dev_global = NULL;
static unsigned long power_timeout;
static bool suspending = false;

struct asoc_irkeypad {
	unsigned int size;
        unsigned int ir_ch;
	unsigned int protocol;
	unsigned int user_code;
	unsigned int wk_code;
	unsigned int period;

	unsigned int *ir_code;
	unsigned int *key_code;

	struct atc260x_dev *atc260x_dev;

	struct input_dev *input_dev;

	struct workqueue_struct *wq;
  	struct delayed_work work;
  	struct tasklet_struct tasklet_pressed;

	int irq;
	unsigned int ir_val;
	unsigned int old_key_val;
	unsigned int new_key_val;
	unsigned int old_mouse_move;
	unsigned int new_mouse_move;
	unsigned int speed_up;
	unsigned int mouse_speed;
	unsigned int is_mouse_mode;
};

#if 0
static  unsigned int asoc_irkeypad_keycode[]=
{
	KEY_POWER,KEY_MUTE,KEY_VOLUMEUP,KEY_VOLUMEDOWN,KEY_1,
	KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,
	KEY_7,KEY_8,KEY_9,KEY_0,/*KEY_INPUT,*/
	KEY_BACKSPACE,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,
	KEY_SELECT,KEY_HOMEPAGE,KEY_MENU,KEY_BACK,KEY_MOVE,
	KEY_MENU,KEY_MOVE,KEY_MENU
/*	KEY_PAGEUP,KEY_PAGEDOWN,KEY_MOVE,KEY_F2_VLC,KEY_F3_VLC,
	KEY_SETTING,KEY_LOCAL,KEY_SEARCH,KEY_CYCLE_PLAY,KEY_F4_VLC,
	KEY_HELP,KEY_HDMI,KEY_AV,KEY_YPBPR,KEY_COLLECTION,
	KEY_PLAYPAUSE*/
};

static  unsigned int asoc_irkeypad_ircode[]=
{
	0x51,0x4D,0xBB,0xBD,0x31,
	0x32,0x33,0x34,0x35,0x36,
	0x37,0x38,0x39,0x30,/*0x58,*/
	0x44,0x26,0x28,0x25,0x27,
	0x0D,0x49,0xBA,0x1B,0x4A,
	0x52,0x46,0x53
	/*0x56,0x4E,0x48,0x41,0x59,
	0x53,0x42,0x46,0x4C,0x52,
	0x54,0x09,0x11,0x12,0x4F,
	0x50*/
};
#endif

void irkeypad_reg_print(struct asoc_irkeypad *irkeypad)
{
#if (IRKEYPAD_REG_PRINT == 1)
	GL5201_IRKEYPAD_INFO("\n\nfollowing list all irkeypad register's value!\n");
	GL5201_IRKEYPAD_INFO("register atc2603_INTS_PD(0x%08x) value is 0x%08x\n",
		ATC2603C_INTS_PD, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_INTS_PD));
	GL5201_IRKEYPAD_INFO("register atc2603_INTS_MSK(0x%08x) value is 0x%08x\n",
		ATC2603C_INTS_MSK, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_INTS_MSK));
	GL5201_IRKEYPAD_INFO("register atc2603_PMU_SYS_CTL3(0x%08x) value is 0x%08x\n",
		ATC2603C_PMU_SYS_CTL3, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_PMU_SYS_CTL3));
	GL5201_IRKEYPAD_INFO("register atc2603_PMU_SYS_CTL0(0x%08x) value is 0x%08x\n",
		ATC2603C_PMU_SYS_CTL0, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_PMU_SYS_CTL0));
	GL5201_IRKEYPAD_INFO("register atc2603_IRC_CTL(0x%08x) value is 0x%08x\n",
		ATC2603C_IRC_CTL, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_IRC_CTL));
	GL5201_IRKEYPAD_INFO("register atc2603_IRC_STAT(0x%08x) value is 0x%08x\n",
		ATC2603C_IRC_STAT, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_IRC_STAT));
	GL5201_IRKEYPAD_INFO("register atc2603_IRC_CC(0x%08x) value is 0x%08x\n",
		ATC2603C_IRC_CC, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_IRC_CC));
	GL5201_IRKEYPAD_INFO("register atc2603_IRC_KDC(0x%08x) value is 0x%08x\n",
		ATC2603C_IRC_KDC, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_IRC_KDC));
	GL5201_IRKEYPAD_INFO("register atc2603_IRC_WK(0x%08x) value is 0x%08x\n",
		ATC2603C_IRC_WK, atc260x_reg_read(irkeypad->atc260x_dev,ATC2603C_IRC_WK));
#endif
}

static void asoc_irkeypad_enable_irq(struct asoc_irkeypad *irkeypad)
{
	struct atc260x_dev *atc260x_dev = irkeypad->atc260x_dev;
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_CTL,
		0x0004, 0x0004);
}

static void asoc_irkeypad_disable_irq(struct asoc_irkeypad *irkeypad)
{
	struct atc260x_dev *atc260x_dev = irkeypad->atc260x_dev;
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_CTL,
		0x0004, 0x0000);
}

static void asoc_irkeypad_config(struct asoc_irkeypad * irkeypad)
{
    struct atc260x_dev *atc260x_dev = irkeypad->atc260x_dev;

	GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);

	/*IRC enable*/
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_CTL,
		0x0008, 0x0008);

	/*irq*/
	atc260x_set_bits(atc260x_dev, ATC2603C_INTS_MSK,
		0x0100, 0x0100);    //INTS_MSK bit[8]TR

	/*mfp*/
        if(irkeypad->ir_ch == 1){
            atc260x_set_bits(atc260x_dev, ATC2603C_PMU_MUX_CTL0,
                  0x0003, 0x0002);
        }else {
	    atc260x_set_bits(atc260x_dev, ATC2603C_PMU_MUX_CTL0,
	          0x3000, 0x1000);
        }

	/*wake up*/
	atc260x_set_bits(atc260x_dev, ATC2603C_PMU_SYS_CTL0,
		0x0020, 0x0020);

	/*set IRC code mode,NEC default*/
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_CTL,
		0x0003, 0x0001);
	/*set IRC cc,NEC default*/
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_CC,
		0xffff, irkeypad->user_code);

	/*set IRC wakeup*/
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_WK,
		0xffff, ((~irkeypad->wk_code) << 8) | (irkeypad->wk_code));

    /*set IRC filter*/
    atc260x_set_bits(atc260x_dev, ATC2603C_IRC_FILTER,
        0xffff,0x000b);

#if 0
	unsigned int user_code;
	unsigned int wk_code;
	user_code = irkeypad->user_code;
	wk_code = irkeypad->wk_code;

	switch(irkeypad->protocol) {

	case IR_PROTOCOL_9012:
	 	atc260x_set_bits(atc260x_dev, atc2603_IRC_CTL,
			0x0003, 0x0000);
		atc260x_set_bits(atc260x_dev, atc2603_IRC_CC,
			0xffff, (user_code << 8) | user_code);
		atc260x_set_bits(atc260x_dev, atc2603_IRC_WK,
			0xffff, (~wk_code << 8) | user_code);
		break;

	case IR_PROTOCOL_NEC8:
		atc260x_set_bits(atc260x_dev, atc2603_IRC_CTL,
			0x0003, 0x0001);
		atc260x_set_bits(atc260x_dev, atc2603_IRC_CC,
			0xffff, (~user_code << 8) | user_code);
		atc260x_set_bits(atc260x_dev, atc2603_IRC_WK,
			0xffff, (~wk_code << 8) | user_code);
		break;

	case IR_PROTOCOL_RC5:
		atc260x_set_bits(atc260x_dev, atc2603_IRC_CTL,
			0x0003, 0x0002);
		atc260x_set_bits(atc260x_dev, atc2603_IRC_CC,
			0xffff, user_code & 0x001f);
		atc260x_set_bits(atc260x_dev, atc2603_IRC_WK,
			0xffff, wk_code & 0x003f);
		break;

	default:
		break;
	}
#endif
	GL5201_IRKEYPAD_INFO("[%s finished]\n",__func__);
}

static void asoc_irkeypad_convert(unsigned int protocol,
		unsigned int *val)
{
	switch (protocol) {
	case IR_PROTOCOL_9012:
		*val &= 0x00ff;
		break;

	case IR_PROTOCOL_NEC8:
		*val &= 0x00ff;
		break;

	case IR_PROTOCOL_RC5:
		*val &= 0x003f;
		break;

	default:
		break;
	}

	return;
}

static void asoc_irkeypad_scan(struct asoc_irkeypad *irkeypad)
{

	struct atc260x_dev *atc260x_dev = irkeypad->atc260x_dev;

	//GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);

	irkeypad->ir_val = atc260x_reg_read(atc260x_dev, ATC2603C_IRC_KDC);

	asoc_irkeypad_convert(irkeypad->protocol, &(irkeypad->ir_val));

	//GL5201_IRKEYPAD_INFO("[%s finished]\n",__func__);

	return;
}

static void asoc_irkeypad_map(struct asoc_irkeypad *irkeypad)
{
	int i;
	unsigned int *ir_val;

	for (i = 0; i < irkeypad->size; i++) {
		ir_val = irkeypad->ir_code + i;
		asoc_irkeypad_convert(irkeypad->protocol, ir_val);
		if (*ir_val == irkeypad->ir_val) {

			irkeypad->new_key_val = *(irkeypad->key_code + i);
			return;
		}

	}

	GL5201_IRKEYPAD_INFO("[%s]irkeypad map failed, ir_val = 0x%x\n",__func__, irkeypad->ir_val);

	return;
}

static void asoc_irkeypad_report_released(struct asoc_irkeypad *irkeypad)
{
	struct input_dev *input_dev;

#if 0
    input_dev = irkeypad->input_dev;

     if (irkeypad->old_key_val  != KEY_RESERVED)
     {
	    input_report_key(input_dev, irkeypad->old_key_val, 0);
	    GL5201_IRKEYPAD_INFO("key: %d %s\n",
		    irkeypad->old_key_val,"released");
     }
	 input_sync(input_dev);
     irkeypad->new_key_val = KEY_RESERVED;
     irkeypad->old_key_val = irkeypad->new_key_val;

#else

    asoc_irkeypad_disable_irq(irkeypad);

#if DEBUG_IRQ_HANDLER
	GL5201_IRKEYPAD_INFO("%s start : old_key_val = %d \n",__func__, irkeypad->old_key_val);
#endif

    input_dev = irkeypad->input_dev;
    if (irkeypad->old_key_val  != KEY_RESERVED) {
		input_report_key(input_dev, irkeypad->old_key_val, 0);
		input_sync(input_dev);
			GL5201_IRKEYPAD_INFO("key: %d %s\n",
				irkeypad->old_key_val,"released");
 			if(irkeypad->old_key_val == KEY_POWER){
				power_timeout = jiffies + HZ*5;
			}   	
			irkeypad->new_key_val = KEY_RESERVED;
    		irkeypad->old_key_val = irkeypad->new_key_val;
	}

	if(power_led_exist && led_blink_enable){
		__gpio_set_value(gpio_power_led_pin, 0);
	}

	asoc_irkeypad_enable_irq(irkeypad);

#if DEBUG_IRQ_HANDLER
	GL5201_IRKEYPAD_INFO("%s end\n",__func__);
#endif
#endif

}

static void  asoc_irkeypad_report_pressed(struct asoc_irkeypad *irkeypad)
{
	unsigned int changed;
	struct input_dev *input_dev = irkeypad->input_dev;

#if DEBUG_IRQ_HANDLER
	GL5201_IRKEYPAD_INFO("%s start\n",__func__);
#endif

	asoc_irkeypad_map(irkeypad);

#if (IRKEYPAD_SUPPORT_MOUSE == 1)
    if (irkeypad->is_mouse_mode == 1)
    {
        if (irkeypad->speed_up == 1)
        {
            irkeypad->new_mouse_move = irkeypad->new_key_val;
            if (irkeypad->old_mouse_move == irkeypad->new_mouse_move)
            {
                irkeypad->mouse_speed += 2;
                if (irkeypad->mouse_speed >= 50)
                {
                    irkeypad->mouse_speed = 50;
                }
            }
            else
            {
                irkeypad->mouse_speed = 10;
            }
        }
        switch(irkeypad->new_key_val)
        {
            case KEY_UP:
                    input_report_rel(input_dev, REL_X,0);
                    input_report_rel(input_dev, REL_Y, -irkeypad->mouse_speed);
                    input_sync(input_dev);
                    irkeypad->speed_up      = 1;
                    break;
            case KEY_DOWN:
                    input_report_rel(input_dev,REL_X,0);
                    input_report_rel(input_dev,REL_Y, irkeypad->mouse_speed);
                    input_sync(input_dev);
                    irkeypad->speed_up      = 1;
                    break;
            case KEY_LEFT:
                    input_report_rel(input_dev,REL_X,-irkeypad->mouse_speed);
                    input_report_rel(input_dev,REL_Y,0);
                    input_sync(input_dev);
                    irkeypad->speed_up      = 1;
                    break;
            case KEY_RIGHT:
                    input_report_rel(input_dev,REL_X,irkeypad->mouse_speed);
                    input_report_rel(input_dev,REL_Y,0);
                    input_sync(input_dev);
                    irkeypad->speed_up       = 1;
                    break;
            default:
                    irkeypad->speed_up       = 0;
                    irkeypad->mouse_speed    = 10;
                    break;
        }

        if (irkeypad->speed_up == 1)
        {
            irkeypad->old_mouse_move = irkeypad->new_key_val;
            irkeypad->new_key_val = KEY_RESERVED;
            irkeypad->old_key_val = irkeypad->new_key_val ;
        }
    }
#endif

    changed = irkeypad->old_key_val ^ irkeypad->new_key_val;
    if (changed)
    {
#if (IRKEYPAD_SUPPORT_MOUSE == 1)
        if (irkeypad->new_key_val == KEY_MOVE)
        {
              irkeypad->speed_up     = 0;
              irkeypad->mouse_speed  = 10;
              if (irkeypad->is_mouse_mode == 0)
              {
                   GL5201_IRKEYPAD_INFO("irkeypad mouse mode enable \n");
                   irkeypad->is_mouse_mode = 1;
                   input_report_rel(input_dev, REL_X,  1);
                   input_report_rel(input_dev, REL_Y,  1);
                   input_report_key(input_dev, BTN_LEFT,  0);
                   input_report_key(input_dev, BTN_RIGHT,  0);
                   irkeypad->old_key_val = irkeypad->new_key_val ;
                   input_sync(input_dev);
                   return;
               }
               else
               {
                   irkeypad->is_mouse_mode = 0;
                   GL5201_IRKEYPAD_INFO("irkeypad key mode enable \n");
               }
        }

        if (irkeypad->is_mouse_mode == 1)
        {
            switch(irkeypad->new_key_val)
            {
                case KEY_SELECT:
                    irkeypad->new_key_val = BTN_LEFT;
                    break;

                //case KEY_BACK:
                //    irkeypad->new_key_val = BTN_RIGHT;
                //    break;

                default:
                    break;
            }
        }

		if(!(irkeypad->old_key_val ^ irkeypad->new_key_val))
		  return;
#endif
#if 0

        if (irkeypad->old_key_val != KEY_RESERVED)
        {
              input_report_key(input_dev,irkeypad->old_key_val, 0);
        }

        if (irkeypad->new_key_val != KEY_RESERVED)
        {
             input_report_key(input_dev,irkeypad->new_key_val, 1);
        }

        irkeypad->old_key_val = irkeypad->new_key_val;
        input_sync(input_dev);
#endif		
		
		/*report old val released*/
		if (irkeypad->old_key_val != KEY_RESERVED) {

			//asoc_irkeypad_report_released(irkeypad);
		}

		if(irkeypad->new_key_val != KEY_RESERVED) {
			input_report_key(input_dev, irkeypad->new_key_val, 1);
			input_sync(input_dev);
			GL5201_IRKEYPAD_INFO("key: %d %s\n",
					irkeypad->new_key_val,"pressed");

			if(power_led_exist && led_blink_enable){
				__gpio_set_value(gpio_power_led_pin, 1);
			}
		}

		irkeypad->old_key_val = irkeypad->new_key_val;
	}
	return;
}

static void asoc_irkeypad_tasklet_pressed(unsigned long data)
{
	struct asoc_irkeypad *irkeypad = (struct asoc_irkeypad *)data;

	asoc_irkeypad_report_pressed(irkeypad);
}

static void asoc_irkeypad_work_released(struct work_struct *work)
{
	struct asoc_irkeypad *irkeypad =
		container_of(work, struct asoc_irkeypad, work.work);

	asoc_irkeypad_report_released(irkeypad);
}

static inline unsigned int asoc_irkeypad_get_pend(struct asoc_irkeypad *irkeypad)
{
	return (atc260x_reg_read(irkeypad->atc260x_dev,
			ATC2603C_IRC_STAT) & IRC_STAT_IIP ? 1 : 0);
}

static inline unsigned int asoc_irkeypad_get_irep(struct asoc_irkeypad *irkeypad, unsigned int stat)
{
	/*return (atc260x_reg_read(irkeypad->atc260x_dev,
			atc2603_IRC_STAT) & IRC_STAT_IREP ? 1 : 0);*/
	return ( (stat & IRC_STAT_IREP) ? 1 : 0);
}

bool is_invalid_power(struct asoc_irkeypad *irkeypad){
	int i;
	unsigned int *ir_val;
	unsigned int keycode = 0;

	for (i = 0; i < irkeypad->size; i++) {
		ir_val = irkeypad->ir_code + i;
		asoc_irkeypad_convert(irkeypad->protocol, ir_val);
		if (*ir_val == irkeypad->ir_val) {
			keycode = *(irkeypad->key_code + i);
			break;
		}

	}

	if(suspending){
            if(keycode == KEY_POWER){
                return false;
            }else {
	        return true;
            }
	}

	if(time_before(jiffies, power_timeout) && keycode == KEY_POWER){
	  return true;
	} else {
	  return false;
	}
}

static int wakeup_powerkey_check(struct asoc_irkeypad *irkeypad)
{
	struct atc260x_dev *atc260x_dev = irkeypad->atc260x_dev;
        unsigned int wk_src;
        struct input_dev *input_dev;

        input_dev = irkeypad->input_dev;
 
        wk_src = atc260x_reg_read(atc260x_dev, ATC2603C_PMU_SYS_CTL1);
        GL5201_IRKEYPAD_INFO("Suspending of resuming, wk_src = 0x%x \n",wk_src);
        wk_src &= 0x20;
        if(wk_src != 0x20){
                GL5201_IRKEYPAD_INFO("Suspending of resuming, no key.");
                return -1;
        }

        asoc_irkeypad_scan(irkeypad);   

        if(is_invalid_power(irkeypad)){
                GL5201_IRKEYPAD_INFO("Suspending of resuming, ignore this power key.");
                return -1;
        } 

        input_report_key(input_dev, KEY_POWER, 1);
        input_report_key(input_dev, KEY_POWER, 0);
        input_sync(input_dev);

        return 0;

}

static irqreturn_t asoc_irkeypad_irq_handler(int irq, void *dev_id)
{
	struct asoc_irkeypad *irkeypad = dev_id;
	struct atc260x_dev *atc260x_dev = irkeypad->atc260x_dev;
	unsigned int stat;
    unsigned int old_ir_val;

#if DEBUG_IRQ_HANDLER
	GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);
#endif

	stat = atc260x_reg_read(irkeypad->atc260x_dev, ATC2603C_IRC_STAT);
	atc260x_set_bits(atc260x_dev, ATC2603C_IRC_STAT, 0xffff, 0x0114);

	old_ir_val = irkeypad->ir_val;
	asoc_irkeypad_scan(irkeypad);

	if(is_invalid_power(irkeypad)){
		GL5201_IRKEYPAD_INFO("Suspending of resuming, ignore this power key.");
		return IRQ_HANDLED;
	}

#if DEBUG_IRQ_HANDLER
        stat = atc260x_reg_read(irkeypad->atc260x_dev, ATC2603C_IRC_STAT);
	GL5201_IRKEYPAD_INFO("IRC_STAT : 0x%x\n", stat);
#endif

	if(power_led_exist && led_blink_enable){
		__gpio_set_value(gpio_power_led_pin, 0);
	}


	
	if (asoc_irkeypad_get_irep(irkeypad, stat)) {
		if ((irkeypad->ir_val == old_ir_val) || (old_ir_val == 0)) {
			GL5201_IRKEYPAD_INFO("invalide code, old_ir_val = 0x%x, new_ir_val = 0x%x, IRC_STAT = 0x%x\n",
					old_ir_val, irkeypad->ir_val, stat);
#if DEBUG_IRQ_HANDLER
			irkeypad_reg_print(irkeypad);
#endif
			return IRQ_HANDLED;
		}
	}

#if DEBUG_IRQ_HANDLER
	GL5201_IRKEYPAD_INFO("[%s] : old_key_val = %d, new_key_val = %d \n",__func__, 
				irkeypad->old_key_val, irkeypad->new_key_val);
#endif

	if( stat & IRC_STAT_RCD ){
		if ((irkeypad->new_key_val == KEY_RESERVED) && (irkeypad->speed_up == 0) ){
			GL5201_IRKEYPAD_INFO("invalide repeat, IRC_STAT = 0x%x\n", stat);
		} else {
			cancel_delayed_work(&irkeypad->work);
#if (IRKEYPAD_SUPPORT_MOUSE == 1)
			tasklet_schedule(&irkeypad->tasklet_pressed);
#endif
			queue_delayed_work(irkeypad->wq, &irkeypad->work,
					msecs_to_jiffies(irkeypad->period));
		}

	} else {
		cancel_delayed_work(&irkeypad->work);
		//if ( irkeypad->new_key_val != KEY_RESERVED ){
		//	asoc_irkeypad_report_released(irkeypad);
		//}
		//asoc_irkeypad_scan(irkeypad);
		tasklet_schedule(&irkeypad->tasklet_pressed);
		queue_delayed_work(irkeypad->wq, &irkeypad->work,
				msecs_to_jiffies(irkeypad->period));
	}

	return IRQ_HANDLED;
}


static void asoc_irkeypad_start(struct asoc_irkeypad * irkeypad)
{
	GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);

	asoc_irkeypad_config(irkeypad);
	asoc_irkeypad_enable_irq(irkeypad);

	GL5201_IRKEYPAD_INFO("[%s finished]\n",__func__);
}

static void asoc_irkeypad_stop(struct asoc_irkeypad *irkeypad)
{

	GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);

	cancel_delayed_work(&irkeypad->work);
	asoc_irkeypad_disable_irq(irkeypad);

	GL5201_IRKEYPAD_INFO("[%s finished]\n",__func__);
}

static int asoc_irkeypad_open(struct input_dev *dev)
{
	struct asoc_irkeypad *irkeypad = input_get_drvdata(dev);

	GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);

	asoc_irkeypad_start(irkeypad);

	GL5201_IRKEYPAD_INFO("[%s finished]\n",__func__);
	return 0;
}

static void asoc_irkeypad_close(struct input_dev *dev)
{

	struct asoc_irkeypad *irkeypad = input_get_drvdata(dev);

	GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);

	asoc_irkeypad_stop(irkeypad);

	GL5201_IRKEYPAD_INFO("[%s finished]\n",__func__);
	return;
}

static ssize_t atv5201_irkeypad_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,size_t count)
{
	char *endp;
	struct asoc_irkeypad *irkeypad = dev_get_drvdata(dev);
	int cmd = simple_strtoul(buf,&endp,0);
	size_t size = endp - buf;

	if (*endp && isspace(*endp))
		size++;
	if (size != count)
		return -EINVAL;

	switch (cmd) {
	case 0:
		irkeypad_reg_print(irkeypad);
		break;

	default:
		break;
	}

	return count;


}

static ssize_t atv5201_irkeypad_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	ssize_t ret_size = 0;

	return ret_size;
}

static ssize_t atv5201_irkeypad_mousespeed_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,size_t count)
{
	char *endp;
    int __attribute__((unused)) cmd;
    size_t size;
	//struct asoc_irkeypad *irkeypad = dev_get_drvdata(dev);
    
	cmd = simple_strtoul(buf, &endp, 0);
	size = endp - buf;

	if (*endp && isspace(*endp))
		size++;
	if (size != count)
		return -EINVAL;

	return count;
}

static ssize_t atv5201_irkeypad_mousespeed_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
    struct asoc_irkeypad *irkeypad = dev_get_drvdata(dev);
    unsigned int offset = 0;

	offset += snprintf(&buf[offset], PAGE_SIZE - offset, "%d\n",
					irkeypad->mouse_speed);
	return offset;
}



static struct device_attribute irkeypad_attrs[] = {
	__ATTR(mouseSpeed, S_IRUGO | S_IWUSR,  atv5201_irkeypad_mousespeed_show, atv5201_irkeypad_mousespeed_store),
	__ATTR(queueSize,  S_IRUGO | S_IWUSR,	atv5201_irkeypad_show, atv5201_irkeypad_store),
};


static int atc260x_irkeypad_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x_dev = dev_get_drvdata(pdev->dev.parent);
	struct asoc_irkeypad *irkeypad;
	struct device_node *np;
	struct input_dev *input_dev;
	int ret = 0;
	int i;

    atc260x_dev_global = atc260x_dev;

	printk("[%s start]\n",__func__);
	np = pdev->dev.of_node;

	if(led_blink_enable){/*
		memset((void *)&power_led_cfg, 0, sizeof(struct gpio_pre_cfg));
		if (gpio_get_pre_cfg(GPIO_NAME_POWER_LED, &power_led_cfg)) {
			printk("\n[power] get  power led gpio failed!");
			power_led_exist = false;
		} else {
			power_led_exist = true;
			gpio_power_led_pin =  ASOC_GPIO_PORT(power_led_cfg.iogroup, power_led_cfg.pin_num);
			gpio_request(gpio_power_led_pin, GPIO_NAME_POWER_LED);
			gpio_direction_output(gpio_power_led_pin, 0);
		}*/
	}

	irkeypad = kzalloc(sizeof(struct asoc_irkeypad), GFP_KERNEL);
	if (irkeypad == NULL) {

		dev_err(&pdev->dev, "failed to allocate irkeypad driver data\n");
		ret = -ENOMEM;
		return ret;

	}
	platform_set_drvdata(pdev, irkeypad);

	irkeypad->atc260x_dev = atc260x_dev;
	irkeypad->old_key_val = KEY_RESERVED;
	irkeypad->old_mouse_move = KEY_RESERVED;
	irkeypad->mouse_speed  = 10;
#if 0 /*(IRKEYPAD_DEBUG == 1)*/
	irkeypad->size = ARRAY_SIZE(asoc_irkeypad_keycode);
	irkeypad->user_code = 0x80;
	irkeypad->wk_code = 0x51;
	irkeypad->ir_code = asoc_irkeypad_ircode;
	irkeypad->key_code = asoc_irkeypad_keycode;
	irkeypad->protocol = 0x01;
	irkeypad->period   = 130;
#endif
	/*get size*/
	ret = of_property_read_u32(np, "size", &(irkeypad->size));
	if ((ret) || (!irkeypad->size)) {
		dev_err(&pdev->dev, "Get size failed ret = %d \r\n", ret);
		goto of_property_read_err;
	}
	dev_info(&pdev->dev, "size = %d\n", irkeypad->size);

        ret = of_property_read_u32(np, "ir_ch", &(irkeypad->ir_ch));
        if ((ret) || (!irkeypad->ir_ch)) {
                dev_err(&pdev->dev, "Get ir_ch failed ret = %d \r\n", ret);
                irkeypad->ir_ch = 0;
                //goto of_property_read_err;
        }
        printk("atc260x_irkeypad: ir_ch = %d\n", irkeypad->ir_ch);

	/*get user_code*/
	ret = of_property_read_u32(np, "user_code", &(irkeypad->user_code));
	if ((ret) || (!irkeypad->user_code)) {
		dev_err(&pdev->dev, "Get user_code failed ret = %d \r\n", ret);
		goto of_property_read_err;
	}
	dev_info(&pdev->dev, "user_code = %d\n", irkeypad->user_code);

	/*get protocol*/
	ret = of_property_read_u32(np, "protocol", &(irkeypad->protocol));
	if ((ret) || (!irkeypad->protocol)) {
		dev_err(&pdev->dev, "Get protocol failed ret = %d \r\n", ret);
		goto of_property_read_err;
	}
	dev_info(&pdev->dev, "protocol = %d\n", irkeypad->protocol);
	
	/*get wk_code*/
	ret = of_property_read_u32(np, "wk_code", &(irkeypad->wk_code));
	if ((ret) || (!irkeypad->wk_code)) {
		dev_err(&pdev->dev, "Get wk_code failed ret = %d \r\n", ret);
		goto of_property_read_err;
	}
	dev_info(&pdev->dev, "wk_code = %d\n", irkeypad->wk_code);
	
	/*get period*/
	ret = of_property_read_u32(np, "period", &(irkeypad->period));
	if ((ret) || (!irkeypad->period)) {
		dev_err(&pdev->dev, "Get period failed ret = %d \r\n", ret);
		goto of_property_read_err;
	}
	dev_info(&pdev->dev, "period = %d\n", irkeypad->period);


	/*get ir_code*/
	irkeypad->ir_code = devm_kzalloc(&pdev->dev,
		sizeof(unsigned int) * (irkeypad->size), GFP_KERNEL);
	if (!irkeypad->ir_code)
		goto free;

	ret = of_property_read_u32_array(np, "ir_code",
		(u32 *)irkeypad->ir_code,
		irkeypad->size);
	if (ret) {
		dev_err(&pdev->dev, "Get ir_code failed ret = %d\r\n", ret);
		goto free_ir_code;
	}


	/*get key_code*/
	irkeypad->key_code = devm_kzalloc(&pdev->dev,
		sizeof(unsigned int) * (irkeypad->size), GFP_KERNEL);
	if (!irkeypad->key_code)
		goto free;

	ret = of_property_read_u32_array(np, "key_code",
		(u32 *)irkeypad->key_code,
		irkeypad->size);
	if (ret) {
		dev_err(&pdev->dev, "Get key_code failed ret = %d\r\n", ret);
		goto free_key_code;
	}


	/*
	 * the WORK work is in charge of reporting key release and
	 * the WORK work_pressed reports key pressed.
	 */
	tasklet_init(&irkeypad->tasklet_pressed, asoc_irkeypad_tasklet_pressed, (unsigned long)irkeypad);
	INIT_DELAYED_WORK(&irkeypad->work, asoc_irkeypad_work_released);
	irkeypad->wq = create_workqueue("atv5201-IRKEYPAD");

	/*
	 * irq related
	 */
	irkeypad->irq = platform_get_irq(pdev, 0);
	GL5201_IRKEYPAD_INFO("[%s] get irq: %d\n", __func__, irkeypad->irq);
	if (irkeypad->irq < 0) {
		dev_err(&pdev->dev, "failed to get irkeypad irq\n");
		ret = -ENXIO;
		goto free_key_code;
	}
	ret = request_threaded_irq(irkeypad->irq, NULL, asoc_irkeypad_irq_handler,
		IRQF_TRIGGER_HIGH, "atc260x-irkeypad", irkeypad);
	if (ret) {
		dev_err(&pdev->dev, "failed to request irkeypad IRQ\n");
		goto free_key_code;
	}
        asoc_irkeypad_disable_irq(irkeypad);
	/*
	 * input_dev related
	 */
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "failed to allocate  irkeypad input device\n");
		ret = -ENOMEM;
		goto free_irq;
	}
	input_dev->name = pdev->name;
	input_dev->phys = "asoc-irkeypad/input5";
	input_dev->open = asoc_irkeypad_open;
	input_dev->close = asoc_irkeypad_close;
	input_dev->dev.parent = &pdev->dev;
	input_dev->keycodemax = irkeypad->size;
	input_dev->id.bustype = BUS_HOST;

	irkeypad->input_dev = input_dev;
	input_set_drvdata(input_dev, irkeypad);
#if (IRKEYPAD_SUPPORT_MOUSE == 1)
	input_dev->evbit[0] 	= BIT(EV_KEY) | BIT(EV_REL);
    input_dev->keybit[BIT_WORD(BTN_MOUSE)]  = BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT);
    input_dev->relbit[0]    = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
    irkeypad->is_mouse_mode    = 0;
#else
	input_dev->evbit[0] = BIT(EV_KEY);
#endif

	for (i = 0; i < input_dev->keycodemax; i++){

		if(irkeypad->key_code[i] != KEY_RESERVED)
			__set_bit(irkeypad->key_code[i],
				input_dev->keybit);

	}
	ret = input_register_device(input_dev);
	if (ret) {

		dev_err(&pdev->dev, "failed to register input device\n");
		goto free_input;

	}

	for (i = 0; i < ARRAY_SIZE(irkeypad_attrs); i++) {
		ret = device_create_file(&pdev->dev, &irkeypad_attrs[i]);
		if (ret) {
			printk("failed to create sysfs file\n");
			break;
		}
	}
	
	power_timeout = jiffies;
	//asoc_irkeypad_open(input_dev);
	printk("[%s finished]\n", __func__);
	return 0;

free_input:
	input_free_device(input_dev);
        input_set_drvdata(input_dev, NULL);
free_irq:
	free_irq(irkeypad->irq, pdev);

free_key_code:
free_ir_code:
        platform_set_drvdata(pdev, NULL);
of_property_read_err:
free:
	kfree(irkeypad);

	return ret;
}

static int atc260x_irkeypad_remove(struct platform_device *pdev)
{
        int i;
	struct asoc_irkeypad *irkeypad = platform_get_drvdata(pdev);

        GL5201_IRKEYPAD_INFO("[%s start]\n",__func__);
	//asoc_irkeypad_close(irkeypad->input_dev);
	free_irq(irkeypad->irq, irkeypad);
	input_unregister_device(irkeypad->input_dev);
	input_free_device(irkeypad->input_dev);
	input_set_drvdata(irkeypad->input_dev, NULL);

	for (i = 0; i < ARRAY_SIZE(irkeypad_attrs); i++) {
        device_remove_file(&pdev->dev, &irkeypad_attrs[i]);
	}
	destroy_workqueue(irkeypad->wq);
	platform_set_drvdata(pdev, NULL);
 
        kfree(irkeypad);
    
	GL5201_IRKEYPAD_INFO("[%s finished]\n", __func__);
	return 0;
}


void atc260x_irkeypad_reset_irc(void)
{
        GL5201_IRKEYPAD_INFO("%s ,reset IRC protocol!\n", __func__);
	atc260x_set_bits(atc260x_dev_global, ATC2603C_IRC_CTL,
		0x0003, 0x0002);
	msleep(100);
	atc260x_set_bits(atc260x_dev_global, ATC2603C_IRC_CTL,
		0x0003, 0x0001);
}

static void atc260x_irkeypad_shutdown(struct platform_device *pdev){
	atc260x_irkeypad_reset_irc();
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void irkeypad_early_suspend(struct early_suspend *h)
{
//	atc260x_set_bits(atc260x_dev_global, atc2603_IRC_CTL,
//		0x0004, 0x0000);
	suspending = true;
        GL5201_IRKEYPAD_INFO("[%s finished]\n", __func__);
}

static void irkeypad_late_resume(struct early_suspend *h)
{
//	atc260x_set_bits(atc260x_dev_global, atc2603_IRC_CTL,
//		0x0004, 0x0004);
	suspending = false;
	power_timeout = jiffies + 5*HZ;
        GL5201_IRKEYPAD_INFO("[%s finished]\n", __func__);
}

static struct early_suspend irkeypad_early_suspend_desc = {
		.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
		.suspend = irkeypad_early_suspend,
		.resume = irkeypad_late_resume,
};
#endif
static int atc260x_irkeypad_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct asoc_irkeypad *irkeypad = platform_get_drvdata(pdev);
	asoc_irkeypad_stop(irkeypad);

	atc260x_irkeypad_reset_irc();
        GL5201_IRKEYPAD_INFO("[%s finished]\n", __func__);
	return 0;
}

static int atc260x_irkeypad_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct asoc_irkeypad *irkeypad = platform_get_drvdata(pdev);
        wakeup_powerkey_check(irkeypad);
	asoc_irkeypad_start(irkeypad);
        GL5201_IRKEYPAD_INFO("[%s finished]\n", __func__);
	return 0;
}

static const struct dev_pm_ops s_atc260x_irkeypad_pm_ops = {
	.suspend        = atc260x_irkeypad_suspend,
	.resume	        = atc260x_irkeypad_resume,
};

static const struct of_device_id atc260x_irkey_of_match[] = {
        {.compatible = "actions,atc2603a-irkeypad",},
        {.compatible = "actions,atc2603c-irkeypad",},
        {.compatible = "actions,atc2609a-irkeypad",},
        {}
};
MODULE_DEVICE_TABLE(of, atc260x_irkey_of_match);

static struct platform_driver atc260x_irkeypad_driver = {
        .driver         = {
                .name   = "atc260x-irkeypad",
                .owner  = THIS_MODULE,
                .pm     = &s_atc260x_irkeypad_pm_ops,
                .of_match_table = of_match_ptr(atc260x_irkey_of_match),
        },
	.probe		= atc260x_irkeypad_probe,
	.remove		= atc260x_irkeypad_remove,
        .shutdown       = atc260x_irkeypad_shutdown,
};

static int __init atc260x_irkeypad_init(void)
{
	int ret;
	ret=platform_driver_register(&atc260x_irkeypad_driver);
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&irkeypad_early_suspend_desc);
#endif
	return ret;
}

static void __exit atc260x_irkeypad_exit(void)
{
	platform_driver_unregister(&atc260x_irkeypad_driver);

#ifdef CONFIG_HAS_EARLYSUSPEND
	/*if config for early suspen , unregister it */	
        unregister_early_suspend(&irkeypad_early_suspend_desc);
#endif
}

module_init(atc260x_irkeypad_init);
module_exit(atc260x_irkeypad_exit);

MODULE_DESCRIPTION("Asoc irkeypad controller drvier");
MODULE_AUTHOR("ZhigaoNi <zhigaoni@actions-semi.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:asoc-irkeypad");
