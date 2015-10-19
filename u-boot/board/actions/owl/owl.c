
/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 */
#include <common.h>
#include <asm/arch/owl_afi.h>
#include <asm/arch/owl_clk.h>
#include <asm/arch/owl_mmc.h>
#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>
#include <asm/arch/actions_reg_owl.h>
#include <i2c.h>
#include <asm/arch/sys_proto.h>
#include <owl_dss.h>
#include <asm/arch/pwm.h>
#include <netdev.h>
#include <asm/arch/pmu.h>


 
DECLARE_GLOBAL_DATA_PTR;

int get_boot_dev_num(void)
{
	char *s;
	if(owl_get_boot_dev() == OWL_BOOTDEV_NAND) {
		s = getenv("nanddev");	
	} else if(owl_get_boot_dev() == OWL_BOOTDEV_SD2){
		s = getenv("emmcdev");	
	} else {		
	    s = getenv("mmcdev");	
	}

	debug("boot dev num: %s\n", s);
	return (int)((char)s[0] - '0');	
}
int dram_init(void)
{
	unsigned long msize;
	msize = owl_get_ddr_size();	
	if ( msize < (8*1024*1024) )
		gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	else
		gd->ram_size = msize;
	printf("mem-size=%ldMB\n", gd->ram_size>>20);
	return 0;
}

int board_init(void)
{
	owl_afi_init();
	owl_clk_init();
	kinfo_init();
#if defined(CONFIG_OWL_PINCTRL)
	pinctrl_init_r();
#endif

#if defined(CONFIG_OWL_GPIO)
	owl_gpio_init();
#endif

#if defined(CONFIG_SYS_I2C_OWL)
    board_i2c_init(gd->fdt_blob);
#endif
	
#if defined(CONFIG_OWL_PWM)
	if (pwm_init(gd->fdt_blob))
		debug("%s: Failed to init pwm\n", __func__);
#endif
	pmu_init(gd->fdt_blob);

	return 0;
}

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	int rc = 0;
#if defined(CONFIG_ACTS_OWL_MAC) 
	rc = owl_mac_initialize(bis);
	if (rc)
		printf("Error %d registering ETHERNET\n", rc);
#endif
#if defined(CONFIG_USB_ETHER) && !defined(CONFIG_SPL_BUILD)
	rc = usb_eth_initialize(bis);
	if (rc < 0)
		printf("Error %d registering USB_ETHER\n", rc);
#endif
	return rc;
}
#endif

#ifdef CONFIG_OWL_MMC
int board_mmc_init(bd_t *bis)
{
	int ret = 0;

	if(owl_get_boot_mode() == (int)BOOT_MODE_PRODUCE)
		return 0;

#ifdef CONFIG_OF_CONTROL
    int node = 0, dev_index;
	fdt_addr_t addr;

	do {
		node = fdtdec_next_compatible(gd->fdt_blob, node,
			COMPAT_ACTIONS_OWL_SDMMC);
		if (node < 0) {
			return -1;
		}

    	if(fdtdec_get_is_enabled(gd->fdt_blob, node)){
			addr = fdtdec_get_addr(gd->fdt_blob, node, "reg");
			if (addr == FDT_ADDR_T_NONE) {
				debug("Can't get the mmc register address\n");
				return -1;
			}
			debug("reg: 0x%08x\n", addr);

			dev_index = (int)((addr - SD0_BASE) >> 14);
			if(dev_index > 3 || dev_index < 0){
				debug("mmc register address error\n");
				return -1;
			}	
			ret = owl_mmc_init(dev_index);
			if(ret){
				debug("error :%s ,ret :%d\n",__FUNCTION__,ret);
			}
    	}
	}while(1);
	
#else
	ret = owl_mmc_init((int)SDC2_SLOT);
	if(ret){
		debug("error :%s ,ret :%d\n",__FUNCTION__,ret);
	}
#endif

	return ret;
}
#endif
