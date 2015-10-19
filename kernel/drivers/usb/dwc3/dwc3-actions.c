/**
 * dwc3-actions.c - actions-semi DWC3 Specific Glue layer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <mach/irqs.h>
#include <mach/hardware.h>

#include "core.h"
#include "usb3_regs.h"

#define  ADFUS_PROC
static struct resource dwc3_resources[3];
static u64 dwc3_dma_mask = DMA_BIT_MASK(32);
static struct platform_device *pdev_dwc_actions;
extern  void dwc3_set_usb_clk_ops(void (*clk_open)(void ),void (*clk_close)(void ));

#ifdef ADFUS_PROC

#define ADFUS_PROC_FILE_LEN 64

enum probatch_status {
	PROBATCH_START = 0,
	PROBATCH_INSTAL_FLASH,
	PROBATCH_FINISH_INSTALL_FLASH,
	PROBATCH_WRITE_PHY,
	PROBATCH_FINISH_WRITE_PHY,
	PROBATCH_FORMAT,
	PROBATCH_FINISH_FORMAT,
	PROBATCH_FINISH,	
	PROBATCH_FINISH_OK,
};	
#endif

enum{
   IC_ATM7039C = 0,
   IC_ATM7059A
};

struct dwc3_port_info {
	void __iomem *usbecs;
	void __iomem *devrst;
	void __iomem *usbpll;
};

struct dwc3_actions {
	struct platform_device	*dwc3;
	struct device		*dev;

	struct dwc3_port_info port_info;	
	void __iomem        *base;
       int 				ic_type;
};
int dwc3_get_device_id(void);
void dwc3_put_device_id(int id);

//static void dwc3_clk_init(struct dwc3_actions *dwc3_owl)
static void dwc3_clk_init(void)
{
	u32		reg;
    	struct dwc3_actions	*dwc3_owl;
	struct dwc3_port_info *port_info ;

	if(pdev_dwc_actions ==NULL){
		printk("------can't get dwc3-actions platform device structure!!---\n");
		return ;
	}  
	dwc3_owl = platform_get_drvdata(pdev_dwc_actions);
	port_info = &dwc3_owl->port_info;
	printk("\n------------dwc3_clk_init-----ic=%d-----\n",dwc3_owl->ic_type);
	/*USB3 PLL enable*/
	reg = readl(port_info->usbpll);
	if(dwc3_owl->ic_type == IC_ATM7059A){
	    reg |= (0x7f);
	}
	else if(dwc3_owl->ic_type == IC_ATM7039C){
	    reg |= (0x1f);
	}
	writel(reg, port_info->usbpll);

	udelay(1000);

	/*USB3 Cmu Reset */
	reg = readl(port_info->devrst);
	reg &= ~(USB3_MOD_RST);
	writel(reg, port_info->devrst);

	udelay(100);
	
	reg = readl(port_info->devrst);
	reg |= (USB3_MOD_RST);
	writel(reg, port_info->devrst);

	udelay(100);

 	if(dwc3_owl->ic_type == IC_ATM7059A){     
		reg = readl(dwc3_owl->base + DWC3_CMU_DEBUG_LDO);
		reg |= CMU_BIAS_EN;
		writel(reg, dwc3_owl->base + DWC3_CMU_DEBUG_LDO);
		reg = readl(port_info->usbecs );
		reg |= (1 << USB3_P0_CTL_PLLLDOEN_IC1 )|(/*2*/3 << USB3_P0_CTL_LDOVREFSEL_SHIFT_IC1);
		writel(reg, port_info->usbecs );
	}
	 else if(dwc3_owl->ic_type == IC_ATM7039C){
		/*PLL1 enable*/
		reg = readl(dwc3_owl->base + DWC3_CMU_DEBUG_LDO);
		reg |= CMU_BIAS_EN;
		writel(reg, dwc3_owl->base + DWC3_CMU_DEBUG_LDO);

		/*PLL2 enable*/
		reg = (BIST_QINIT(0x3) | EYE_HEIGHT(0x4) | PLL2_LOCK | PLL2_RS(0x2) | 
				PLL2_ICP(0x1) | CMU_SEL_PREDIV | CMU_DIVX2 | PLL2_DIV(0x17) | 
				PLL2_POSTDIV(0x3) | PLL2_PU);
		writel(reg, dwc3_owl->base + DWC3_CMU_PLL2_BISTDEBUG);
	        reg = readl(port_info->usbecs );
		reg |= (1 << USB3_P0_CTL_PLLLDOEN )|(2 << USB3_P0_CTL_LDOVREFSEL_SHIFT);
		writel(reg, port_info->usbecs );
	}
     
	udelay(100);
	reg = readl(port_info->usbecs);
	reg &=  ~((0x1 << USB3_P0_CTL_DPPUEN_P0)|(0x1 << USB3_P0_CTL_DMPUEN_P0)); 
	writel(reg, port_info->usbecs );
    
       udelay(1000);
	return;
}

//static void dwc3_clk_exit(struct dwc3_actions *dwc3_owl)
static void dwc3_clk_exit(void)
{
	u32		reg;
    	struct dwc3_actions	*dwc3_owl;
	struct dwc3_port_info *port_info ;

	if(pdev_dwc_actions ==NULL){
		printk("------can't get dwc3-actions platform device structure!!---\n");
		return ;
	}  
    	dwc3_owl = platform_get_drvdata(pdev_dwc_actions);
     	port_info = &dwc3_owl->port_info;
    
	printk("\n------------dwc3_clk_exit----------\n");
	/*USB3 PLL disable*/
	reg = readl(port_info->usbpll);
	
       if(dwc3_owl->ic_type == IC_ATM7059A){
	    reg &= ~(0x7f);
       }
       else if(dwc3_owl->ic_type == IC_ATM7039C){
	    reg &= ~(0x1f);
       }
	writel(reg, port_info->usbpll);
}

/*---------------------------------------------------------------------------
 *	proc file entry  for debug
 *---------------------------------------------------------------------------*/
#ifdef ADFUS_PROC
static struct proc_dir_entry *adfus_proc_entry;
char adfus_proc_path[] = "adfus_proc";

char probatch_phase[ADFUS_PROC_FILE_LEN];

char all_probatch_phase[][ADFUS_PROC_FILE_LEN]=
{
	"null",
	"install_flash",
	"finish_install_flash",
	"write_phy",
	"finish_write_phy",
	"format",
	"finish_format",
	"finish",
	"finish_ok",
};

int set_probatch_phase(int id)
{

	strcpy(probatch_phase, all_probatch_phase[id]);
	return 0;
}	
EXPORT_SYMBOL_GPL(set_probatch_phase);

int is_probatch_phase(int id)
{
	int ret;
	
	ret = memcmp(probatch_phase, all_probatch_phase[id], strlen(all_probatch_phase[id]));
	return ret;
}
EXPORT_SYMBOL_GPL(is_probatch_phase);

static ssize_t adfus_proc_read(struct file *file, char __user *buffer,
                   size_t count, loff_t * offset)
{
	int len;
	
	if(*offset > 0)
	    return 0;
	    
	len = strlen(probatch_phase);
	if(len > count)
		len = count;
		
    if(copy_to_user(buffer, probatch_phase, len))
        return -EFAULT;
    
    *offset += len;
	return len;
}

static ssize_t adfus_proc_write(struct file *file, const char __user *buffer,
                   size_t count, loff_t * offset)
{
	int len;
	
	if(*offset > 0)
	    return 0;
	if(count > ADFUS_PROC_FILE_LEN)
		len = ADFUS_PROC_FILE_LEN;
	else
		len = count;
	
	if (copy_from_user(probatch_phase, buffer, len ))
		return -EFAULT;
		
	probatch_phase[len]=0;
	
    *offset += len;
	return len;
}

static const struct file_operations __adfus_proc_file_operations =
{
    .owner =        THIS_MODULE,
    .read =         adfus_proc_read,
    .write =        adfus_proc_write,
};
#endif



static  struct dwc3_actions  atm7039c_data = {
	.ic_type = IC_ATM7039C,

};

static  struct dwc3_actions  atm7059a_data = {
	.ic_type = IC_ATM7059A,
};

static const struct of_device_id  owl_dwc3_actions_of_match[]   = {
	{.compatible = "actions,atm7039c-usb", .data = &atm7039c_data},
	{.compatible = "actions,atm7059tc-usb", .data = &atm7059a_data },
	{.compatible = "actions,atm7059a-usb", .data = &atm7059a_data },
	{}

};
MODULE_DEVICE_TABLE(of, owl_dwc3_actions_of_match);

static int  dwc3_actions_probe(struct platform_device *pdev)
{
	struct platform_device	*dwc3;
	struct resource		*res;
	struct device       *dev = &pdev->dev;
	struct dwc3_actions	*dwc3_owl;
    	const struct of_device_id *id;
	void __iomem        *base;
	int			devid = 0;
	int			ret = -ENOMEM;
       
	pdev_dwc_actions = pdev;//record this platform_device for later use
	id = of_match_device(owl_dwc3_actions_of_match, &pdev->dev);
	if(id ==NULL)
	{
		printk("<dwc3_actions>err: no config !!!\n");
		return -EINVAL;
	}
	dwc3_owl =(struct dwc3_actions	*) id->data;
      
	(pdev->dev).dma_mask = &dwc3_dma_mask;
	(pdev->dev).coherent_dma_mask	= DMA_BIT_MASK(32);
	//dwc3_5202 = devm_kzalloc(dev,sizeof(*dwc3_5202), GFP_KERNEL);
	//if (!dwc3_5202) {
	//	dev_err(&pdev->dev, "not enough memory\n");
	//	goto err1;
	//}
    platform_set_drvdata(pdev, dwc3_owl);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory base resource\n");
		return -EINVAL;
	}
	memcpy(&dwc3_resources[0], res, sizeof(*res));

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "missing IRQ base resource\n");
		return -EINVAL;
	}
	memcpy(&dwc3_resources[1], res, sizeof(*res));

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(dev, "missing memory base resource\n");
		return -EINVAL;
	}

	res = devm_request_mem_region(dev, res->start,resource_size(res),
			dev_name(dev));
	if (!res) {
		dev_err(dev, "can't request mem region\n");
		return -ENOMEM;
	}

	base = devm_ioremap(dev, res->start, resource_size(res));
	if (!base) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}
	
	dwc3_owl->port_info.devrst = (void __iomem *)IO_ADDRESS(CMU_DEVRST1);
	dwc3_owl->port_info.usbecs = (void __iomem *)IO_ADDRESS(USB3_P0_CTL);
	dwc3_owl->port_info.usbpll = (void __iomem *)IO_ADDRESS(CMU_USBPLL);

	devid = dwc3_get_device_id();
	if (devid < 0)	
		return -ENODEV;
		

	dwc3 = platform_device_alloc("dwc3", devid);
	if (!dwc3) {
		dev_err(&pdev->dev, "couldn't allocate dwc3 device\n");
		goto err1;
	}

	dma_set_coherent_mask(&dwc3->dev, pdev->dev.coherent_dma_mask);

	dwc3->dev.parent = &pdev->dev;
	dwc3->dev.dma_mask = pdev->dev.dma_mask;
	dwc3->dev.dma_parms = pdev->dev.dma_parms;
	dwc3_owl->dwc3	= dwc3;
	dwc3_owl->dev	= &pdev->dev;
	dwc3_owl->base	= base;

    	dwc3_set_usb_clk_ops(dwc3_clk_init,dwc3_clk_exit);
	dwc3_clk_init();
       //to transfor ic type to dwc3.ko,use resource method
       dwc3_resources[2] = dwc3_resources[1];
       dwc3_resources[2].flags = (IORESOURCE_REG|(dwc3_owl->ic_type));
	ret = platform_device_add_resources(dwc3, dwc3_resources, 3);
	if (ret) {
		dev_err(&pdev->dev, "couldn't add resources to dwc3 device\n");
		goto err2;
	}

	ret = platform_device_add(dwc3);
	if (ret) {
		dev_err(&pdev->dev, "failed to register dwc3 device\n");
		goto err2;
	}
#ifdef ADFUS_PROC
	adfus_proc_entry = proc_create(adfus_proc_path, 0, NULL, &__adfus_proc_file_operations);
	if (adfus_proc_entry) {
		/* proc op */
		strcpy(probatch_phase, all_probatch_phase[0]);		
	} else {
		/* proc op */
		printk("adfus :can not create proc file\n");
	}
#endif
	return 0;
	
err2:
	platform_device_put(dwc3);
	dwc3_clk_exit();
err1:
	dwc3_put_device_id(devid);
	return ret;
}
#if 0
#ifdef CONFIG_PM
static int dwc3_actions_suspend(struct platform_device *pdev, pm_message_t state)
{
	int	irq;
	struct dwc3_actions	*dwc3_owl = platform_get_drvdata(pdev);
	irq = platform_get_irq(dwc3_owl->dwc3, 0);
	disable_irq(irq);
	//dwc3_clk_exit(dwc3_5202);
    
	return 0;
}

static int dwc3_actions_resume(struct platform_device *pdev)
{
	int	irq;
	struct dwc3_actions	*dwc3_owl = platform_get_drvdata(pdev);
	irq = platform_get_irq(dwc3_owl->dwc3, 0);
	//dwc3_clk_init(dwc3_owl);
	enable_irq(irq);

	return 0;
}
#else
#define	dwc3_actions_suspend	NULL
#define	dwc3_actions_resume		NULL
#endif
#endif
static int  dwc3_actions_remove(struct platform_device *pdev)
{
	struct dwc3_actions	*dwc3_owl = platform_get_drvdata(pdev);
#ifdef ADFUS_PROC
	if (adfus_proc_entry)
		remove_proc_entry(adfus_proc_path, NULL);
#endif
	dwc3_clk_exit();
	platform_device_unregister(dwc3_owl->dwc3);
	dwc3_put_device_id(dwc3_owl->dwc3->id);

	return 0;
}
#if 0
static void dwc3_actions_release(struct device * dev)
{
	return ;
}


static struct resource dwc3_resources[] = {
	[0] = {
		.start	= USB3_REGISTER_BASE,
		.end	= USB3_REGISTER_BASE + USB3_ACTIONS_START - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= OWL_IRQ_USB3,
		.end	= OWL_IRQ_USB3,
		.flags	= IORESOURCE_IRQ,
	},	
};

static struct resource actions_resources[] = {
	[0] = {
		.start	= USB3_REGISTER_BASE + USB3_ACTIONS_START,
		.end	= USB3_REGISTER_BASE + USB3_ACTIONS_END,
		.flags	= IORESOURCE_MEM,
	},
};

static u64 dwc3_dma_mask = DMA_BIT_MASK(32);

static struct platform_device dwc3_actions_device = {
	.name       = "actions-dwc3",
	.id     = 1,
	.dev	= {
		.dma_mask		= &dwc3_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),	
		.release = dwc3_actions_release,
	},
	.resource	= actions_resources,
	.num_resources	= ARRAY_SIZE(actions_resources),    
};

#endif
static struct platform_driver dwc3_actions_driver = {
	.probe		= dwc3_actions_probe,
	.remove		= dwc3_actions_remove,
	//.suspend	= dwc3_actions_suspend,
	//.resume		= dwc3_actions_resume,
	.driver		= {
		.name	= "actions-dwc3",
					.of_match_table = owl_dwc3_actions_of_match,
	},
};

static int __init  dwc3_actions_init(void)
{
	//platform_device_register(&dwc3_actions_device);
	return platform_driver_register(&dwc3_actions_driver);
}
module_init(dwc3_actions_init);

static void __exit dwc3_actions_exit(void)
{
	platform_driver_unregister(&dwc3_actions_driver);
	//platform_device_unregister(&dwc3_actions_device);
}
module_exit(dwc3_actions_exit);

//module_platform_driver(dwc3_actions_driver);

MODULE_ALIAS("platform:actions-dwc3");
MODULE_AUTHOR("wanlong <wanlong@actions-semi.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DesignWare USB3 actions Glue Layer");
