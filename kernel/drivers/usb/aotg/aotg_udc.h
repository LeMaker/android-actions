
#ifndef  __LINUX_USB_GADGET_AOTG_H
#define  __LINUX_USB_GADGET_AOTG_H

#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include "aotg.h"

struct udc_ring {
	unsigned is_running:1;
	int num_trbs;
	u8 mask;
	atomic_t num_trbs_free;
	struct aotg_trb *first_trb;
	struct aotg_trb *last_trb;
	struct aotg_trb *enqueue_trb;
	struct aotg_trb *cur_trb;
	u32 trb_dma;
};

/* about dma_no: if (dma_no & 0x10) == 0x10, it's hcout, otherwise it's hcin. */
#define AOTG_DMA_OUT_PREFIX		0x10
#define AOTG_DMA_NUM_MASK		0xf
#define AOTG_IS_DMA_DEVICE_IN_HOUT(x)	((x) & AOTG_DMA_OUT_PREFIX)
#define AOTG_GET_DMA_NUM(x)		((x) & AOTG_DMA_NUM_MASK)

#define   CONFIG_USB_GADGET_DEBUG_FS

#define   EP0_PACKET_SIZE       64

#define   BULK_HS_PACKET_SIZE		512
#define   BULK_FS_PACKET_SIZE		64

#define   INT_HS_PACKET_SIZE   	1024
#define   INT_FS_PACKET_SIZE     	64

#define   ISO_HS_PACKET_SIZE   	1024
#define   ISO_FS_PACKET_SIZE    1023

#define 	IRQ_AOTG_DMA    			0

#define  	USB_UDC_IN_MASK       0x10
#define   	EP_ADDR_MASK          0x0F

#define		EP1_BULK_OUT_STARTADD   		0x00000080
#define 	EP1_BULK_IN_STARTADD  			0x00000480
#define		EP2_BULK_IN_STARTADD   			0x00000880
#define 	EP2_BULK_OUT_STARTADD  			0x00000A80

#define 	EP_ISO_IN_STARTADD  			0x00000E80
#define 	EP_INT_IN_STARTADD  			0x00001280

#define AOTG_UDC_NUM_ENDPOINTS      11
#define	OUT_EP_MAX_NUM			5
#define	IN_EP_MAX_NUM			5


#define UDC_BULK_EP(index,name,addr,buftype) \
		aotg_ep_setup(udc, index, name "-bulk", addr, \
				USB_ENDPOINT_XFER_BULK, buftype);
#define UDC_INT_EP(index,name,addr, buftype) \
		aotg_ep_setup(udc, index, name "-int", addr, \
				USB_ENDPOINT_XFER_INT, buftype);
#define UDC_ISO_EP(index, name, addr, buftype) \
		aotg_ep_setup(udc , index, name "-iso", addr, \
				USB_ENDPOINT_XFER_ISOC, buftype);

struct epstats {
	u32 ops;
	u32 bytes;
};

//struct aotg_udc;

struct aotg_ep {
	struct usb_ep ep;
	struct aotg_udc *dev;
	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;
	char name[14];
	u32 udc_irqs;
	struct epstats read;
	struct epstats write;
	unsigned short maxpacket;
	u8 bEndpointAddress;
	u8 bmAttributes;
	unsigned stopped:1;
	u8 mask;
	u8 buftype;
	u32 reg_udccs;
	u32 reg_udccon;
	u32 reg_udcbc;
	u32 reg_udcfifo;
	u32 reg_maxckl;
	u32 reg_fifostaddr;
	int dma_no;

	u32 reg_dmalinkaddr;
	u32 reg_curaddr;
	u32 reg_dmactrl;
	u32 reg_dmacomplete_cnt;

	struct udc_ring *ring;
};

struct aotg_request {
	struct usb_request req;
	struct list_head queue;
	struct aotg_trb *queue_trb;
	struct aotg_trb *start_trb;
	struct aotg_trb *end_trb;
	unsigned num_trbs:16;
	unsigned cross_req:1;
};

enum ep0_state {
	EP0_WAIT_FOR_SETUP,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_END_XFER,
	EP0_STALL,
};

enum udc_state {
	UDC_UNKNOWN,
	UDC_IDLE,
	UDC_ACTIVE,
	UDC_SUSPEND,
	UDC_DISABLE,
};

struct udc_stats {
	struct epstats read, write;
	u32 irqs;
};

struct aotg_udc {
	void __iomem *base;
	int irq;
	int	id;
	int reset_cnt;
	spinlock_t lock;

	struct aotg_plat_data  *port_specific;
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct device *dev;
	struct usb_phy *transceiver;

	enum ep0_state ep0state;
	enum udc_state state;
	struct udc_stats stats;
	resource_size_t		rsrc_start;
	resource_size_t		rsrc_len;

	struct aotg_ep ep[AOTG_UDC_NUM_ENDPOINTS];

	unsigned enabled:1;
	unsigned pullup_on:1;
	unsigned pullup_resume:1;
	unsigned softconnect:1;
	unsigned config:2;
	unsigned highspeed:1;
	unsigned suspend:1;
	unsigned disconnect:1;
	unsigned req_pending:1;
	unsigned req_std:1;
	unsigned req_config:1;
	unsigned rwk:1;
	unsigned inited:1;

#ifdef CONFIG_PM
	unsigned udccsr0;
#endif
#ifdef CONFIG_USB_GADGET_DEBUG_FS
	struct dentry *debugfs_udc;
#endif
};

#define		dplus_up(udc)  	usb_clearbitsb(USBCS_DISCONN, udc->base + USBCS)
#define		dplus_down(udc)  	usb_setbitsb(USBCS_DISCONN, udc->base + USBCS)

#define		extern_irq_enable(udc) usb_setbitsb(USBEIRQ_USBIEN, udc->base + USBEIEN)
#define		extern_irq_disable(udc) usb_setbitsb(USBEIRQ_USBIEN, udc->base + USBEIEN)

/*#define DEBUG_UDC
#define ERR_UDC
#define ERR_UDC_DMA*/
#define UDC_DBG_INFO		printk(KERN_ERR "udch--<%s:%d>", __func__, __LINE__)
/*#define UDC_DBG_INFO		do {} while (0)*/
#define UDC_DBG_ERR		pr_err("udch err!--<%s:%d>", __func__, __LINE__)
#define UDC_DBG_TRACE		do {} while (0)
/*#define UDC_DBG_TRACE		printk(KERN_ERR "udch trace:--<%s:%d>", __func__, __LINE__)*/

#ifdef DEBUG_UDC
#define UDC_DEBUG(fmt, args...)    printk(KERN_ERR "<UDC %s:%d>"fmt,__func__,__LINE__, ## args)
#else
#define UDC_DEBUG(fmt, args...)	/*not printk */
#endif

#define UDC_NOTICE(fmt, args...)    printk(KERN_ERR "<UDC %s:%d>"fmt,__func__,__LINE__, ## args)

#define UDC_PRINTK(fmt, args...)

#ifdef ERR_UDC
#define UDC_ERR(fmt, args...)    printk(KERN_ERR "<UDC %s:%d>"fmt,__func__,__LINE__, ## args)
#else
#define UDC_ERR(fmt, args...)	/*not printk */
#endif

#ifdef ERR_UDC_DMA
#define UDC_DMA_DEBUG(fmt, args...)    printk(KERN_ERR fmt, ## args)
#else
#define UDC_DMA_DEBUG(fmt, args...)	/*not printk */
#endif

#define UDC_WARNING(fmt, args...) printk(KERN_WARNING fmt, ## args)

extern struct aotg_udc memory;
extern struct aotg_udc *acts_udc_controller;
extern const char udc_driver_name[];
 int aotg_udc_register(int dev_id);
void aotg_udc_unregister(int dev_id);
extern struct platform_driver aotg_udc_driver;
irqreturn_t aotg_udc_irq(int irq, void *data);
void udc_reinit(struct aotg_udc *dev);
int pullup(struct aotg_udc *udc, int is_active);
void aotg_udc_endpoint_config(struct aotg_udc *udc);

#endif				/* __LINUX_USB_GADGET_AOTG_H */
