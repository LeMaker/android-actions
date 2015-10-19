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
 * \file   umonitor_config.h
 * \brief  
 *      usb monitor configure headfile.
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
#ifndef _UMONITOR_CONFIG_H_
#define _UMONITOR_CONFIG_H_
#define  SUPPORT_NOT_RMMOD_USBDRV 1
//#ifdef ATM7029_EVB
//#define GPIO_VBUS_SUPPLY    0x28      //KS_OUT1 GPIOB8   001  01000
//#endif
//
//#ifdef ATM7029_DEMO
//#define GPIO_VBUS_SUPPLY    0x11      //RMII_RXER GPIPA17 010 10001
//#endif

#define CHECK_TIMER_INTERVAL  1000


#define __GPIO_GROUP(x)     ((x) >> 5)
#define __GPIO_NUM(x)       ((x) & 0x1f)

typedef struct umon_port_config {
    #define UMONITOR_DISABLE   		   0
    #define UMONITOR_DEVICE_ONLY       	   1
    #define UMONITOR_HOST_ONLY	           2
    #define UMONITOR_HOST_AND_DEVICE       3
    unsigned char detect_type;	/* usb port detect request. */
    /* if detect_type == UMONITOR_DISABLE, below is no use. */
    
    unsigned char port_type;
	#define PORT_DWC3   		   0
	#define PORT_USB2       	   1    
    

    #define UMONITOR_USB_IDPIN   	   0
    #define UMONITOR_SOFT_IDPIN       	   1
    #define UMONITOR_GPIO_IDPIN	           2  /* gpio detect idpin. */
    unsigned char idpin_type;
    /*
     * if idpin_type set to UMONITOR_USB_IDPIN or UMONITOR_SOFT_IDPIN, 
     * below is no use. 
     */
    unsigned char idpin_gpio_group;
    unsigned int idpin_gpio_no;

    #define UMONITOR_USB_VBUS   	   0
    #define UMONITOR_GPIO_VBUS	           1  /* gpio detect vbus. */
    #define UMONITOR_DC5V_VBUS             2  /* use dc5v to detect vbus. */
    unsigned char vbus_type;
    /*
     * if vbus_type set to UMONITOR_USB_VBUS, below is no use. 
     */
    unsigned char vbus_gpio_group;
    unsigned int vbus_gpio_no;

    /* in host state, if vbus power switch onoff use gpio, set it. */
    unsigned char power_switch_gpio_group;
    unsigned int power_switch_gpio_no;
    unsigned char power_switch_active_level;
#ifdef SUPPORT_NOT_RMMOD_USBDRV      
    /* add a node to receive vold msg ,to open close controlers */
    char usb_con_msg[32];
#endif
    /*add a node to fix idpin&vbus state, let user can detect host/device as wish*/	
    unsigned char idpin_debug;
    unsigned char vbus_debug;	
} umon_port_config_t;


	
//#define 	DEBUG_MONITOR
//#define   ERR_MONITOR
	
#ifdef DEBUG_MONITOR
#define MONITOR_PRINTK(fmt, args...)    printk(KERN_INFO fmt, ## args)
#else
#define MONITOR_PRINTK(fmt, args...)    /*not printk*/
#endif

#ifdef ERR_MONITOR
#define MONITOR_ERR(fmt, args...)    printk(KERN_ERR fmt, ## args)
#else
#define MONITOR_ERR(fmt, args...)    /*not printk*/
#endif	
		
#endif  /* _UMONITOR_CONFIG_H_ */
/*! \endcond*/

