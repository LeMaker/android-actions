/*
 * Actions USB HOST XHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
 *	Vikas Sajjan <vikas.sajjan@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * This file is a conglomeration for DWC3-init sequence and further
 * Actions specific PHY-init sequence.
 */

#include <common.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <usb.h>
#include <watchdog.h>
#if 0
#include <asm/arch/cpu.h>
#include <asm/arch/power.h>
#include <asm/arch/clocks.h>
#endif
#include <asm/arch/xhci-owl.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/gpio.h>
#include <asm/arch/pmu.h>
#include <asm-generic/errno.h>
#include <linux/compat.h>
#include <linux/usb/dwc3.h>

#include "xhci.h"

#undef debug
//#define XHCI_OWL_DEBUG
#ifdef XHCI_OWL_DEBUG
#define debug(fmt, args...)	printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif

/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

/**
 * Contains pointers to register base addresses
 * for the usb controller.
 */
struct owl_xhci {
	struct xhci_hccr *hcd;
	struct dwc3 *dwc3_reg;
#if 0
	struct gpio_desc vbus_gpio;
#else
	struct owl_fdt_gpio_state vbus_gpio;
#endif
};

static struct owl_xhci owl;

static int owl_usb3_parse_dt(const void *blob, struct owl_xhci *owl)
{
	unsigned int node;

	node = fdtdec_next_compatible(blob, 0, COMPAT_ACTIONS_OWL_USB3_MONITOR);
	if (node <= 0) {
		debug("XHCI: Can't get device node for xhci\n");
		return -ENODEV;
	}

	/* Vbus gpio */
	owl_fdtdec_decode_gpio(blob, node, "vbus_otg_en_gpios", &owl->vbus_gpio);
	return 0;
}

static void dwc3_set_mode(struct dwc3 *dwc3_reg, u32 mode)
{
	clrsetbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG),
			DWC3_GCTL_PRTCAPDIR(mode));
}

static void dwc3_core_soft_reset(struct dwc3 *dwc3_reg)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_CORESOFTRESET);

	/* Assert USB3 PHY reset */
	setbits_le32(&dwc3_reg->g_usb3pipectl[0],
			DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	setbits_le32(&dwc3_reg->g_usb2phycfg,
			DWC3_GUSB2PHYCFG_PHYSOFTRST);

	mdelay(100);

	/* Clear USB3 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb3pipectl[0],
			DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb2phycfg,
			DWC3_GUSB2PHYCFG_PHYSOFTRST);

	/* After PHYs are stable we can take Core out of reset state */
	clrbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_CORESOFTRESET);
}


void disable_bias(void)
{
	u32 reg;
    
	reg = readl(DWC3_BASE + BACKDOOR);
	reg |=(1<<27);
	writel(reg, DWC3_BASE + BACKDOOR);
	reg = readl(DWC3_BASE + ANA0F);
	reg &=~(1<<14);
	writel(reg, DWC3_BASE + ANA0F);
}

void enable_bias(void)
{
	u32 reg;
   
	reg = readl(DWC3_BASE + ANA0F);
	reg |=(1<<14);
	writel(reg, DWC3_BASE + ANA0F);
}

static int dwc3_core_init(struct dwc3 *dwc3_reg)
{
	u32 reg;
	u32 revision;
	unsigned int dwc3_hwparams1;

	revision = readl(&dwc3_reg->g_snpsid);
	/* This should read as U3 followed by revision number */
	if ((revision & DWC3_GSNPSID_MASK) != 0x55330000) {
		puts("this is not a DesignWare USB3 DRD Core\n");
		return -EINVAL;
	}

	writel(0x6046, DWC3_BASE + ANA02);
	writel(0x2010, DWC3_BASE + ANA0E);
	writel(0x8000, DWC3_BASE + ANA0F);
	writel(0x0, DWC3_BASE + REV1);
	writel(0x0013, DWC3_BASE + PAGE1_REG02);
	writel(0x0004, DWC3_BASE + PAGE1_REG06);
	writel(0x22ed, DWC3_BASE + PAGE1_REG07);
	writel(0xf802, DWC3_BASE + PAGE1_REG08);
	writel(0x3080, DWC3_BASE + PAGE1_REG09);
	writel(0x2030, DWC3_BASE + PAGE1_REG0B);
	writel((1<<14), DWC3_BASE + ANA0F);

	dwc3_core_soft_reset(dwc3_reg);

		//====force to high speed====
	reg = readl(DWC3_BASE + DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);
	reg |= DWC3_DCFG_HIGHSPEED;
	writel(reg, DWC3_BASE + DWC3_DCFG);
	dwc3_hwparams1 = readl(&dwc3_reg->g_hwparams1);

	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
	reg &= ~DWC3_GCTL_DISSCRAMBLE;
	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc3_hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	default:
		debug("No power optimization available\n");
	}

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if ((revision & DWC3_REVISION_MASK) < 0x190a)
		reg |= DWC3_GCTL_U2RSTECN;

	writel(reg, &dwc3_reg->g_ctl);

	return 0;
}

static int owl_xhci_core_init(struct owl_xhci *owl)
{
	int ret;

	ret = dwc3_core_init(owl->dwc3_reg);
	if (ret) {
		debug("failed to initialize core\n");
		return -EINVAL;
	}
	dwc3_phy_init(1, 0);
	dwc3_phy_init(1, 1);
	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(owl->dwc3_reg, DWC3_GCTL_PRTCAP_HOST);

	return 0;
}

static void owl_xhci_core_exit(struct owl_xhci *owl)
{

}

int xhci_hcd_init(int index, struct xhci_hccr **hccr, struct xhci_hcor **hcor)
{
	struct owl_xhci *ctx = &owl;
	int ret;

	owl_usb3_parse_dt(gd->fdt_blob, ctx);

	ctx->hcd = (struct xhci_hccr *)DWC3_BASE;
	ctx->dwc3_reg = (struct dwc3 *)((char *)(ctx->hcd) + DWC3_REG_OFFSET);

	/* setup the Vbus gpio here */
	atc260x_reg_write(0x3d, atc260x_reg_read(0x3d) | (0x1<<9));
	debug("vbus_gpio, gpio:%d, flags:%d\n", ctx->vbus_gpio.gpio, ctx->vbus_gpio.flags);
	owl_gpio_generic_direction_output(ctx->vbus_gpio.chip, ctx->vbus_gpio.gpio, ctx->vbus_gpio.flags);
	
	dwc3_clk_init();
	owl_usb3_open_powergate();
	
	ret = owl_xhci_core_init(ctx);
	if (ret) {
		puts("XHCI: failed to initialize controller\n");
		return -EINVAL;
	}

	*hccr = (ctx->hcd);
	*hcor = (struct xhci_hcor *)((uint32_t) *hccr
				+ HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	debug("owl-xhci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(xhci_readl(&(*hccr)->cr_capbase)));

	return 0;
}

void xhci_hcd_stop(int index)
{
	struct owl_xhci *ctx = &owl;

	owl_xhci_core_exit(ctx);
	dwc3_clk_exit();
	debug("vbus_gpio, gpio:%d, flags:%d\n", ctx->vbus_gpio.gpio, !ctx->vbus_gpio.flags);
	owl_gpio_generic_direction_output(ctx->vbus_gpio.chip, ctx->vbus_gpio.gpio, !ctx->vbus_gpio.flags);
	atc260x_reg_write(0x3d, atc260x_reg_read(0x3d) & ~(0x1<<9));
}
