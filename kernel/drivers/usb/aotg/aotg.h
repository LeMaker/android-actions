#ifndef  __LINUX_AOTG_H 
#define  __LINUX_AOTG_H 

#include <mach/hardware.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/hcd.h>
#include <linux/earlysuspend.h>

#include "aotg_regs.h"

#define TRB_ITE	(1 << 11)
#define TRB_CHN	(1 << 10)
#define TRB_CSP	(1 << 9)
#define TRB_COF	(1 << 8)
#define TRB_ICE	(1 << 7)
#define TRB_IZE	(1 << 6)
#define TRB_ISE (1 << 5)
#define TRB_LT	(1 << 4)
#define AOTG_TRB_IOC	(1 << 3)
#define AOTG_TRB_IOZ	(1 << 2)
#define AOTG_TRB_IOS	(1 << 1)
#define TRB_OF	(1 << 0)

#define USE_SG
#ifdef USE_SG
#define NUM_TRBS (256)
#define RING_SIZE (NUM_TRBS * 16)
#else
#define NUM_TRBS (64)
#define RING_SIZE (NUM_TRBS * 16)
#endif

struct aotg_trb {
	u32 hw_buf_ptr;
	u32 hw_buf_len;
	u32 hw_buf_remain;
	u32 hw_token;
};

extern int vbus_otg_en_gpio[2][2];
extern int aotg_udc_enable[2];
void aotg_hardware_init(int id);
void aotg_udc_exit(int id);
extern struct aotg_hcd *act_hcd_ptr[2];
extern struct aotg_udc *acts_udc_controller;
extern unsigned int port_device_enable[2];
extern struct of_device_id aotg_of_match[];
extern int aotg_probe(struct platform_device *pdev);
extern int aotg_remove(struct platform_device *pdev);
void aotg_clk_enable(int id, int is_enable);
int aotg_udc_register(int id);


#endif