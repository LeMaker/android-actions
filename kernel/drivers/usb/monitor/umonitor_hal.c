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
 * \file   umonitor_hal.c
 * \brief  
 *      usb monitor hardware opration api code.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <mach/hardware.h>

#include "aotg_regs.h"
#include "umonitor_hal.h"

#define USB_PLUGED_PC	    1
#define USB_PLUGED_ADP		2


static void dwc3_controllor_exit(usb_hal_monitor_t * pdev);
void __dwc3_controllor_mode_cfg(usb_hal_monitor_t *pdev);

extern int atc260x_enable_vbusotg(int on);


void mon_dwc3_set_mode(usb_hal_monitor_t * pdev, int mode)
{
	u32 reg;

	reg = readl(pdev->io_base + DWC3_GCTL);
	reg &= ~(DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG));
	reg |= DWC3_GCTL_PRTCAPDIR(mode);
	writel(reg, pdev->io_base + DWC3_GCTL);
}


static int usb_monitor_vbus_power(usb_hal_monitor_t * pdev, int is_on)
{
 	 MONITOR_PRINTK("usb_monitor_vbus_power %04x\n", is_on);
	if(is_on){
		atc260x_enable_vbusotg(is_on);
        	if(pdev->config->power_switch_gpio_no !=0xffff)
			gpio_set_value_cansleep(pdev->config->power_switch_gpio_no, pdev->config->power_switch_active_level);
	}else{
		if(pdev->config->power_switch_gpio_no !=0xffff)
			gpio_set_value_cansleep(pdev->config->power_switch_gpio_no, !pdev->config->power_switch_active_level);
		atc260x_enable_vbusotg(is_on);
	}	
	
	return 0;
}

static int usb_get_dc5v_state(usb_hal_monitor_t * pdev)
{
	/* no use. */
	return USB_DC5V_INVALID;
}

static int get_usb_plug_type(struct usb_hal_monitor *pdev)
{
	int ret = 0;
	int reg_bk;
	int reg;

	reg_bk = readl(pdev->usbecs);
	if ((reg_bk & (1 << 11))) {
		reg = reg_bk;
		writel(reg | 0x0000f000, pdev->usbecs);	 /* dp up enable, dp down disable */
		udelay(200);
		reg = readl(pdev->usbecs);
		if((reg & 0x00000018) != 0) {			/* bit 3, bit 4 not 0, this is usb_adapter */
			//printf("----- current connect is USB_ADPT! \n");
			ret = USB_PLUGED_ADP;
		} else {
			//printf("----- current connect is USB_PC! \n");
			ret = USB_PLUGED_PC;
		}
		writel(reg_bk, pdev->usbecs);			/*restore the USB3_P0_CTL */
	}
	
	return ret;
}

static int usb_get_vbus_state(usb_hal_monitor_t * pdev)
{
	int ret = USB_VBUS_INVALID;
	s32 adc_val;

	switch (pdev->config->vbus_type) {
   	case UMONITOR_USB_VBUS:
		/* vbus valid. */
		  ret = readl(pdev->usbecs) & (1 << USB3_P0_CTL_VBUS_P0);
		  ret = ret ? USB_VBUS_HIGH: USB_VBUS_LOW;
		  break;
   	case UMONITOR_GPIO_VBUS:
   	  break;
   	case UMONITOR_DC5V_VBUS:
    	  ret = atc260x_ex_auxadc_read_by_name("VBUSV", &adc_val);
    	  if(ret < 0)
			break;
		  ret = (adc_val > VBUS_THRESHOLD) ? USB_VBUS_HIGH: USB_VBUS_LOW;
		  break;
	  default:
		  break;
	}
	/*for debug only:let user can detect host/device as wish*/
	if((pdev->config->vbus_debug ==0)||(pdev->config->vbus_debug ==1))
		return pdev->config->vbus_debug;
    	/* printk vbus state for debug*/
	if(pdev->config->vbus_debug == 2)
		printk("\n  usb_get_vbus_state: %d \n",ret);
    
	MONITOR_PRINTK("%s state: %u\n", __func__, ret);
	return ret;	
}

static int usb_hardware_init(usb_hal_monitor_t * pdev);
static int usb_get_idpin_state(usb_hal_monitor_t * pdev)
{
	int value,ret = USB_ID_STATE_INVALID;  
    
	value = readl(pdev->usbpll) & 0x1f;
	if(value == 0){
	       //need to open usbpll before read idpin status
		__dwc3_controllor_mode_cfg(pdev);
       }
    
	switch (pdev->config->idpin_type) {
	case UMONITOR_USB_IDPIN:
    ret = (readl(pdev->usbecs) &
         (1 << USB3_P0_CTL_ID_P0)) ? USB_ID_STATE_DEVICE:USB_ID_STATE_HOST;
		break;
	case UMONITOR_SOFT_IDPIN:
        	break;
	case UMONITOR_GPIO_IDPIN:
		if(0==gpio_get_value_cansleep(pdev->config->idpin_gpio_no))
			ret=USB_ID_STATE_HOST;
		else
			ret = USB_ID_STATE_DEVICE;
         break;
	default:
		break;
	}
	if(value == 0){
	    	//need to close usbpll after read idpin status
	      	dwc3_controllor_exit( pdev);
	}
	/*for debug only:let user can detect host/device as wish*/
	if((pdev->config->idpin_debug ==0)||(pdev->config->idpin_debug ==1))
		return (pdev->config->idpin_debug?USB_ID_STATE_DEVICE: USB_ID_STATE_HOST);
	/* printk idpin state for debug*/    
    	if(pdev->config->idpin_debug ==2)
		printk("\n  usb_get_idpin_state: %s \n",(ret==1)?"device":"host");            
	return ret;
}

static unsigned int usb_get_linestates(usb_hal_monitor_t * pdev)
{
	unsigned int ls;

	ls = ((readl(pdev->usbecs) & USB3_P0_CTL_LS_P0_MASK) >> USB3_P0_CTL_LS_P0_SHIFT);
	return ls;
}

static int usb_set_dp_500k_15k(usb_hal_monitor_t * pdev, int enable_500k_up,
			       int enable_15k_down)
{
	unsigned int val;
	
	val = readl(pdev->usbecs) & (~((1 << USB3_P0_CTL_DPPUEN_P0) |
			(1 << USB3_P0_CTL_DMPUEN_P0) |
			(1 << USB3_P0_CTL_DPPDDIS_P0) |
			(1 << USB3_P0_CTL_DMPDDIS_P0)));
			
	if (enable_500k_up != 0) {
		val |= (1 << USB3_P0_CTL_DPPUEN_P0)|(1 << USB3_P0_CTL_DMPUEN_P0);
	}
	if (enable_15k_down == 0) {
		val |= (1 << USB3_P0_CTL_DPPDDIS_P0)|(1 << USB3_P0_CTL_DMPDDIS_P0);
	}
	
	MONITOR_PRINTK("dpdm is %08x\n", val);
	writel(val, pdev->usbecs);	/* 500k up enable, 15k down disable; */
	return 0;	
	
}

static int usb_set_soft_id(usb_hal_monitor_t * pdev, int en_softid,
			   int id_state)
{
#if 0 //no soft idpin in ATM7059
	unsigned int val;

	if (pdev->config->idpin_type == UMONITOR_USB_IDPIN) {
		/* ignore soft idpin setting. */
		en_softid = 0;
	}
	val = readl(pdev->usbecs);
	if (en_softid != 0) {
		val |= 0x1 << USB3_P0_CTL_SOFTIDEN_P0;	/* soft id enable. */
	} else {
		val &= ~(0x1 << USB3_P0_CTL_SOFTIDEN_P0);	/* soft id disable. */
	}

	if (id_state != 0) {
		val |= (0x1 << USB3_P0_CTL_SOFTID_P0);
	} else {
		val &= ~(0x1 << USB3_P0_CTL_SOFTID_P0);
	}
	writel(val, pdev->usbecs);
#endif    
	return 0;	
}

static int usb_set_soft_vbus(usb_hal_monitor_t * pdev, int en_softvbus, int vbus_state)
{
	unsigned int val;

	if (pdev->config->vbus_type == UMONITOR_USB_VBUS) {
		/* ignore soft vbus setting. */
		en_softvbus = 0;
	}

	val = readl(pdev->usbecs);
	if (en_softvbus != 0) {
		val |= 0x1 << USB3_P0_CTL_SOFTVBUSEN_P0;	/* soft id enable. */
	} else {
		val &= ~(0x1 << USB3_P0_CTL_SOFTVBUSEN_P0);	/* soft id disable. */
	}

	if (vbus_state != 0) {
		val |= (0x1 << USB3_P0_CTL_SOFTVBUS_P0);
	}else {
		val &= ~(0x1 << USB3_P0_CTL_SOFTVBUS_P0);
	}
	writel(val, pdev->usbecs);
	
	return 0;
}

static void usb_hal_set_cmu_usbpll(usb_hal_monitor_t *pdev, int enable)
{
	if (enable != 0) {
		writel(readl(pdev->usbpll) | (0x1f), pdev->usbpll);
	} else {
		writel(readl(pdev->usbpll) & ~(0x1f),pdev->usbpll);
	}
	mdelay(2);
}

/******************************************************************************/
/*!
 * \par  Description:
 *       对usb模块进行初始化（包括开启时钟，复位整个usb硬件模块，并确保复位在返回前完成
 * \param[in] void
 * \param[in] void
 * \return    int
 * \retval    0：usb模块初始化完成    负值：usb模块初始化失败
 * \ingroup   hal_usb
 * \par
 * \note
 * 
 *******************************************************************************/
static int usb_hardware_init(usb_hal_monitor_t * pdev)
{  
	//writel((readl(pdev->usbecs) | 0x0000f000), pdev->usbecs);	
	//set VBUS detection threshold,VBUS_DET_THRESHOLD_LEVEL3
	writel((readl(pdev->usbecs) | VBUS_DET_THRESHOLD_LEVEL3), pdev->usbecs);
  
  MONITOR_PRINTK("usbecs value is %08x------>/n", readl(pdev->usbecs));
		
	return 0;
}

/******************************************************************************/
/*!                    
* \par  Description:
*     enable or disable the usb controller
* \param[in]    aotg  contains the controller info
* \param[in]    enable  enable(1) or disable(0) the controller  
* \return       the result  
* \retval           0 sucess 
* \retval           1 failed
* \ingroup      USB_UOC
*******************************************************************************/
static int usb_hal_aotg_enable(usb_hal_monitor_t * pdev, int enable)
{
	if (enable != 0) {
		MONITOR_PRINTK("aotg mon enable\n");
		if (usb_hardware_init(pdev) != 0) {
			return -1;
		}
	} else {
		MONITOR_PRINTK("aotg mon disable\n");		  
	}
	return 0;
}

static int usb_hal_set_mode(usb_hal_monitor_t * pdev, int mode)
{
	if (mode == USB_IN_DEVICE_MOD) {
		//writel(readl(pdev->usbecs) | (0xf << 12), pdev->usbecs);
	}
	if (mode == USB_IN_HOST_MOD) {
		writel(readl(pdev->usbecs) & ((~0xf) << 12), pdev->usbecs);
	}
	return 0;
}

static void usb_hal_dp_up(usb_hal_monitor_t * pdev)
{
	return;
}

static void usb_hal_dp_down(usb_hal_monitor_t * pdev)
{
	return;
}

static int usb_hal_is_sof(usb_hal_monitor_t * pdev)
{
  return 0;
}

static int usb_hal_enable_irq(struct usb_hal_monitor *pdev, int enable)
{
	return 0;
}

static void usb_hal_debug(usb_hal_monitor_t * pdev)
{
	printk("%s:%d\n", __FILE__, __LINE__);
	printk("pdev->usbecs addr: 0x%x\n\n", (unsigned int)pdev->usbecs);
	return;
}

int usb_suspend_or_resume(usb_hal_monitor_t *pdev, int is_suspend)
{
	/* save/reload usbecs when suspend/resume */
	if (is_suspend) {
		pdev->usbecs_val = readl(pdev->usbecs);
	}else{
		usb_hardware_init(pdev);
		//writel(pdev->usbecs_val, pdev->usbecs);
	}	
	return 0;
}

static void dwc3_controllor_init(usb_hal_monitor_t * pdev)
{
		
	u32		reg;
	int	ic_type =pdev->ic_type;

	/*USB3 PLL enable*/
	reg = readl(pdev->usbpll);
       if(ic_type == IC_ATM7059A){
	    reg |= (0x7f);
       }
       else if(ic_type == IC_ATM7039C){
	    reg |= (0x1f);
       }
	writel(reg, pdev->usbpll);

   	 udelay(1000);

	/*USB3 Cmu Reset */
	reg = readl(pdev->devrst);
	reg &= ~(USB3_MOD_RST);
	writel(reg, pdev->devrst);

	udelay(500);

	reg = readl(pdev->devrst);
	reg |= (USB3_MOD_RST);
	writel(reg, pdev->devrst);

	udelay(500);

 	if(ic_type == IC_ATM7059A){

		writel(0x6046, pdev->io_base+ANA02 );
		writel(0x2010,pdev->io_base+ANA0E );
		writel(0x8000,pdev->io_base+ANA0F );
		writel(0x0, pdev->io_base+ REV1 );
		writel(0x0013,pdev->io_base+PAGE1_REG02 );
		writel(0x0004,pdev->io_base+PAGE1_REG06 );
		writel(0x22ed,pdev->io_base+PAGE1_REG07 );
		writel(0xf802, pdev->io_base+PAGE1_REG08 );
		writel(0x3080,pdev->io_base+PAGE1_REG09 );
		writel(0x2030, pdev->io_base+PAGE1_REG0B );
		writel((1<<14), pdev->io_base+ANA0F );  
             //enable bias
            reg = readl(pdev->io_base + DWC3_CMU_DEBUG_LDO);
		reg |= CMU_BIAS_EN;
		writel(reg, pdev->io_base+ DWC3_CMU_DEBUG_LDO);
        		/*USB2 LDO enable*/
		reg = readl(pdev->usbecs );
		reg |= (1 << USB3_P0_CTL_PLLLDOEN_IC1 )|(/*2*/3  << USB3_P0_CTL_LDOVREFSEL_SHIFT_IC1);
		writel(reg, pdev->usbecs );	

        
	}
	 else if(ic_type == IC_ATM7039C){
		/*PLL1 enable*/
		reg = readl(pdev->io_base + DWC3_CMU_DEBUG_LDO);
		reg |= CMU_BIAS_EN;
		writel(reg, pdev->io_base+ DWC3_CMU_DEBUG_LDO);

		/*PLL2 enable*/
		reg = (BIST_QINIT(0x3) | EYE_HEIGHT(0x4) | PLL2_LOCK | PLL2_RS(0x2) | 
				PLL2_ICP(0x1) | CMU_SEL_PREDIV | CMU_DIVX2 | PLL2_DIV(0x17) | 
				PLL2_POSTDIV(0x3) | PLL2_PU);
		writel(reg, pdev->io_base + DWC3_CMU_PLL2_BISTDEBUG);
		/*USB2 LDO enable*/
		reg = readl(pdev->usbecs );
		reg |= (1 << USB3_P0_CTL_PLLLDOEN )|(/*2*/3  << USB3_P0_CTL_LDOVREFSEL_SHIFT);
		writel(reg, pdev->usbecs );	
	}


    
	udelay(1000);

}

static void dwc3_controllor_exit(usb_hal_monitor_t * pdev)
{
	u32		reg;
    
	int	ic_type =pdev->ic_type;
    
	/*USB3 PLL disable*/
	reg = readl(pdev->usbpll);
	
       if(ic_type == IC_ATM7059A){
	    reg &= ~(0x7f);
       }
       else if(ic_type == IC_ATM7039C){
	    reg &= ~(0x1f);
       }
	writel(reg, pdev->usbpll);
}

/*dwc3_controllor_otg_cfg函数调用有以下3种情况;主要目的是为了设置主控器处于otg模式;
以防止id pin的状态在其它模式下检测不准.调用的情况如下,resume函数,检测到usb拔出,检测
到u盘拔出.*/
/*dwc3_controllor_exit函数会将usbpll关掉,目前发现有风险,不能在这里调用.因拔线情况下,
dwc3(usb)驱动可能还未卸载,而仍然可能访问usb主控制器,而这时已经关掉了pll,会导致问题.*/
void __dwc3_controllor_mode_cfg(usb_hal_monitor_t *pdev)
{
//  u32 value;
  /*不为0,表示usb3之前的频率打开,驱动存在;故这边不需要打开频率;退出时亦不需要关闭频率*/

	dwc3_controllor_init(pdev);
	mon_dwc3_set_mode(pdev, DWC3_GCTL_PRTCAP_OTG); 
  
#if 0     
  if(value == 0)
    dwc3_controllor_exit(pdev);
#endif    

}
void dwc3_controllor_mode_cfg(usb_hal_monitor_t *pdev)
{
     return ;
}
int usb_hal_init_monitor_hw_ops(usb_hal_monitor_t * pdev, 
	umon_port_config_t * config, unsigned int base)
{
	int ret = 0;
	
	pdev->name = "usb controller";
	pdev->io_base =(void __iomem *) base;
	pdev->usbecs = (void __iomem *)IO_ADDRESS(USB3_P0_CTL);
	pdev->usbpll = (void __iomem *)IO_ADDRESS(CMU_USBPLL);
	pdev->devrst =(void __iomem *)IO_ADDRESS( CMU_DEVRST1);

	pdev->vbus_power_onoff = usb_monitor_vbus_power;
	pdev->get_dc5v_state = usb_get_dc5v_state;
	pdev->get_vbus_state = usb_get_vbus_state;
	pdev->get_linestates = usb_get_linestates;
	pdev->get_idpin_state = usb_get_idpin_state;
	pdev->set_dp_500k_15k = usb_set_dp_500k_15k;
	pdev->set_soft_id = usb_set_soft_id;
	pdev->set_soft_vbus = usb_set_soft_vbus;
	pdev->aotg_enable = usb_hal_aotg_enable;
	pdev->set_mode = usb_hal_set_mode;
	pdev->set_cmu_usbpll = usb_hal_set_cmu_usbpll;
	pdev->dp_up = usb_hal_dp_up;
	pdev->dp_down = usb_hal_dp_down;
	pdev->is_sof = usb_hal_is_sof;
	pdev->enable_irq = usb_hal_enable_irq;
	pdev->debug = usb_hal_debug;
	pdev->monitor_get_usb_plug_type = get_usb_plug_type;
	pdev->suspend_or_resume = usb_suspend_or_resume;
	pdev->dwc3_otg_mode_cfg = dwc3_controllor_mode_cfg;
	pdev->dwc3_otg_exit = dwc3_controllor_exit;
    pdev->dwc3_otg_init = dwc3_controllor_init;


	pdev->config = config;
	return ret;
}


