/*
 * atc260x_pm.c  --  ATC260X power management (suspend to ram) support.
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/regulator/driver.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <linux/of.h>

#include <asm/suspend.h>
#include <mach/hardware.h>
#include <mach/power.h>
#include <linux/mfd/atc260x/atc260x.h>


/* ATC2603A_PMU_SYS_CTL1 register bits */
#define PMU_SYS_CTL1_EN_S1              (1 << 0)
#define PMU_SYS_CTL1_LB_S4_SHIFT        (3)
#define PMU_SYS_CTL1_LB_S4_MASK                 (0x3 << PMU_SYS_CTL1_LB_S4_SHIFT)
#define PMU_SYS_CTL1_LB_S4_3_1V         (0x2 << PMU_SYS_CTL1_LB_S4_SHIFT)/*3.1v 低电进S4电压 */
#define PMU_SYS_CTL1_LB_S4_3_3V         (0x3 << PMU_SYS_CTL1_LB_S4_SHIFT)/*3.3v 低电进S4电压 */
#define PMU_SYS_CTL1_WAKE_FLAG_SHIFT    (5)

/* ATC2603A_PMU_SYS_CTL2 register bits */
#define PMU_SYS_CTL2_S2_TIMER_SHIFT     (3)
#define PMU_SYS_CTL2_S2_TIMER_MASK      (0x7 << PMU_SYS_CTL2_S2_TIMER_SHIFT)
#define PMU_SYS_CTL2_S2_TIMER_EN        (1 << 6)

/* ATC2603A_PMU_SYS_CTL3 register bits */
#define PMU_SYS_CTL3_S3_TIMER_SHIFT     (10)
#define PMU_SYS_CTL3_S3_TIMER_MASK      (0x7 << PMU_SYS_CTL3_S3_TIMER_SHIFT)
#define PMU_SYS_CTL3_S3_TIMER_EN        (1 << 13)
#define PMU_SYS_CTL3_EN_S3              (1 << 14)
#define PMU_SYS_CTL3_EN_S2              (1 << 15)

/* ATC2603C_PMU_SYS_CTL3 register bits */
#define ATC2603C_S2S3TOS1_TIMER_EN      (1 << 9)

/* ATC2603A_PMU_SYS_CTL5 register bits */
#define PMU_SYS_CTL5_DETECT_MASK        (0xf << 7)

#define PMU_SYS_CTL5_WALLWKDTEN         (1 << 7)
#define PMU_SYS_CTL5_VBUSWKDTEN         (1 << 8)
#define PMU_SYS_CTL5_REMCON_DECT_EN     (1 << 9)
#define PMU_SYS_CTL5_TP_DECT_EN         (1 << 10)


#define to_atc260x_pm_attr(_attr) \
	container_of(_attr, struct atc260x_pm_attribute, attr)

/*s2 mode,=0 normal mode, =1 low temp mode; */
static u32 s2_mode;

/*in minutes. */
static const unsigned long s2tos3_timeout_table[8] = {
	6, 16, 31, 61, 91, 121, 151, 181
};
/*in minutes. */
static const unsigned long s3tos4_timeout_table[8] = {
	6, 16, 31, 61, 91, 121, 151, 181
};

struct atc260x_pm_alarm_save {
	u16 msalm;
	u16 halm;
	u16 ymdalm;
};

struct atc260x_pm_dev {
	struct device *dev;
	struct atc260x_dev  *atc260x;
	struct notifier_block pm_nb;
	uint pmic_type;
	uint pmic_ver;
	uint active_wakeup_srcs;
};

struct atc260x_pm_attribute{
	struct kobj_attribute attr;
	int index;
};

static struct atc260x_pm_dev *s_current_atc260x_pm_obj = NULL;

static struct atc260x_pm_dev * _get_curr_atc260x_pm_obj(void)
{
	struct atc260x_pm_dev *atc260x_pm = s_current_atc260x_pm_obj;
	if (atc260x_pm == NULL) {
		pr_err("%s() atc260x_pm not registered!\n", __func__);
		BUG();
	}
	return atc260x_pm;
}

/* ---------- sys pm interface ---------------------------------------------- */

struct atc260x_pm_wakeup_src_reg_desc {
	u16 regs[4];
	u8  bit_tbl[OWL_PMIC_WAKEUP_SRC_CNT][2];
};

static const struct atc260x_pm_wakeup_src_reg_desc sc_atc2603a_pm_wakeup_src_desc = {
	.regs = {ATC2603A_PMU_SYS_CTL0, ATC2603A_PMU_SYS_CTL1, 0xffff, 0xffff},
	.bit_tbl = {
		/*  w,   r    */
		{   5,   16+5,  }, /* OWL_PMIC_WAKEUP_SRC_IR */
		{   6,   16+6,  }, /* OWL_PMIC_WAKEUP_SRC_RESET */
		{   7,   16+7,  }, /* OWL_PMIC_WAKEUP_SRC_HDSW */
		{   8,   16+8,  }, /* OWL_PMIC_WAKEUP_SRC_ALARM */
		{   9,   16+9,  }, /* OWL_PMIC_WAKEUP_SRC_REMCON */
		{   10,  16+10, }, /* OWL_PMIC_WAKEUP_SRC_TP */
		{   11,  16+11, }, /* OWL_PMIC_WAKEUP_SRC_WKIRQ */
		{   12,  16+12, }, /* OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT */
		{   13,  16+13, }, /* OWL_PMIC_WAKEUP_SRC_ONOFF_LONG */
		{   14,  16+14, }, /* OWL_PMIC_WAKEUP_SRC_WALL_IN */
		{   15,  16+15, }, /* OWL_PMIC_WAKEUP_SRC_VBUS_IN */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_RESTART */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_SGPIOIRQ */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_WALL_OUT */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_VBUS_OUT */
	}
};
static const struct atc260x_pm_wakeup_src_reg_desc sc_atc2603c_pm_wakeup_src_desc = {
	.regs = {ATC2603C_PMU_SYS_CTL0, ATC2603C_PMU_SYS_CTL1, ATC2603C_PMU_SYS_CTL3, 0xffff},
	.bit_tbl = {
		/*  w,   r    */
		{   5,   16+5,  }, /* OWL_PMIC_WAKEUP_SRC_IR */
		{   6,   16+6,  }, /* OWL_PMIC_WAKEUP_SRC_RESET */
		{   7,   16+7,  }, /* OWL_PMIC_WAKEUP_SRC_HDSW */
		{   8,   16+8,  }, /* OWL_PMIC_WAKEUP_SRC_ALARM */
		{   9,   16+9,  }, /* OWL_PMIC_WAKEUP_SRC_REMCON */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_TP */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_WKIRQ */
		{   12,  16+12, }, /* OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT */
		{   13,  16+13, }, /* OWL_PMIC_WAKEUP_SRC_ONOFF_LONG */
		{   14,  16+14, }, /* OWL_PMIC_WAKEUP_SRC_WALL_IN */
		{   15,  16+15, }, /* OWL_PMIC_WAKEUP_SRC_VBUS_IN */
		{   10,  16+10, }, /* OWL_PMIC_WAKEUP_SRC_RESTART */
		{   11,  16+11, }, /* OWL_PMIC_WAKEUP_SRC_SGPIOIRQ */
		{ 32+2,  32+0,  }, /* OWL_PMIC_WAKEUP_SRC_WALL_OUT */ /* same as WALL_IN */
		{ 32+3,  32+1,  }, /* OWL_PMIC_WAKEUP_SRC_VBUS_OUT */ /* same as VBUS_IN */
	}
};
static const struct atc260x_pm_wakeup_src_reg_desc sc_atc2609a_pm_wakeup_src_desc = {
	.regs = {ATC2609A_PMU_SYS_CTL0, ATC2609A_PMU_SYS_CTL1, 0xffff, 0xffff},
	.bit_tbl = {
		/*  w,   r    */
		{   5,   16+5,  }, /* OWL_PMIC_WAKEUP_SRC_IR */
		{   6,   16+6,  }, /* OWL_PMIC_WAKEUP_SRC_RESET */
		{   7,   16+7,  }, /* OWL_PMIC_WAKEUP_SRC_HDSW */
		{   8,   16+8,  }, /* OWL_PMIC_WAKEUP_SRC_ALARM */
		{   9,   16+9,  }, /* OWL_PMIC_WAKEUP_SRC_REMCON */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_TP */
		{   11,  16+11, }, /* OWL_PMIC_WAKEUP_SRC_WKIRQ */
		{   12,  16+12, }, /* OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT */
		{   13,  16+13, }, /* OWL_PMIC_WAKEUP_SRC_ONOFF_LONG */
		{   14,  16+14, }, /* OWL_PMIC_WAKEUP_SRC_WALL_IN */
		{   15,  16+15, }, /* OWL_PMIC_WAKEUP_SRC_VBUS_IN */
		{   10,  16+10, }, /* OWL_PMIC_WAKEUP_SRC_RESTART */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_SGPIOIRQ */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_WALL_OUT */
		{   255, 255,   }, /* OWL_PMIC_WAKEUP_SRC_VBUS_OUT */
	}
};
static const struct atc260x_pm_wakeup_src_reg_desc * const sc_atc260x_pm_wakeup_src_desc_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = &sc_atc2603a_pm_wakeup_src_desc,
	[ATC260X_ICTYPE_2603C] = &sc_atc2603c_pm_wakeup_src_desc,
	[ATC260X_ICTYPE_2609A] = &sc_atc2609a_pm_wakeup_src_desc,
};

static const u16 sc_atc260x_pm_regtbl_sysctl0[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_PMU_SYS_CTL0,
	[ATC260X_ICTYPE_2603C] = ATC2603C_PMU_SYS_CTL0,
	[ATC260X_ICTYPE_2609A] = ATC2609A_PMU_SYS_CTL0,
};
static const u16 sc_atc260x_pm_regtbl_sysctl1[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_PMU_SYS_CTL1,
	[ATC260X_ICTYPE_2603C] = ATC2603C_PMU_SYS_CTL1,
	[ATC260X_ICTYPE_2609A] = ATC2609A_PMU_SYS_CTL1,
};
static const u16 sc_atc260x_pm_regtbl_sysctl2[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_PMU_SYS_CTL2,
	[ATC260X_ICTYPE_2603C] = ATC2603C_PMU_SYS_CTL2,
	[ATC260X_ICTYPE_2609A] = ATC2609A_PMU_SYS_CTL2,
};
static const u16 sc_atc260x_pm_regtbl_sysctl3[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_PMU_SYS_CTL3,
	[ATC260X_ICTYPE_2603C] = ATC2603C_PMU_SYS_CTL3,
	[ATC260X_ICTYPE_2609A] = ATC2609A_PMU_SYS_CTL3,
};
static const u16 sc_atc260x_pm_regtbl_sysctl5[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_PMU_SYS_CTL5,
	[ATC260X_ICTYPE_2603C] = ATC2603C_PMU_SYS_CTL5,
	[ATC260X_ICTYPE_2609A] = ATC2609A_PMU_SYS_CTL5,
};
static const u16 sc_atc260x_pm_regtbl_rtc_ms[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_MS,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_MS,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_MS,
};
static const u16 sc_atc260x_pm_regtbl_rtc_h[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_H,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_H,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_H,
};
static const u16 sc_atc260x_pm_regtbl_rtc_ymd[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_YMD,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_YMD,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_YMD,
};
static const u16 sc_atc260x_pm_regtbl_rtc_dc[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_DC,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_DC,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_DC,
};
static const u16 sc_atc260x_pm_regtbl_rtc_msalm[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_MSALM,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_MSALM,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_MSALM,
};
static const u16 sc_atc260x_pm_regtbl_rtc_halm[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_HALM,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_HALM,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_HALM,
};
static const u16 sc_atc260x_pm_regtbl_rtc_ymdalm[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = ATC2603A_RTC_YMDALM,
	[ATC260X_ICTYPE_2603C] = ATC2603C_RTC_YMDALM,
	[ATC260X_ICTYPE_2609A] = ATC2609A_RTC_YMDALM,
};


static int _atc260x_pm_set_wakeup_src_inner(struct atc260x_pm_dev *atc260x_pm,
		uint wakeup_mask, uint wakeup_src)
{
	const struct atc260x_pm_wakeup_src_reg_desc *wakeup_src_desc;
	uint i, reg_addr, reg_val, reg_mask, tmp_mask;
	u16 reg_vals[4], reg_masks[4];
	int ret;

	dev_info(atc260x_pm->dev, "%s() enter, mask=0x%x src=0x%x\n",
		__func__, wakeup_mask, wakeup_src);

	if ((wakeup_mask & ~OWL_PMIC_WAKEUP_SRC_ALL) ||
		(wakeup_src & ~OWL_PMIC_WAKEUP_SRC_ALL) ||
		(wakeup_src & ~wakeup_mask)) {
		dev_err(atc260x_pm->dev, "%s() invalid wakeup source\n", __func__);
		return -EINVAL;
	}
	if (wakeup_mask == 0)
		return 0;

	/* update wakeup source EN bit(s) */
	memset(reg_vals, 0, sizeof(reg_vals));
	memset(reg_masks, 0, sizeof(reg_masks));
	wakeup_src_desc = sc_atc260x_pm_wakeup_src_desc_tbl[atc260x_pm->pmic_type];
	tmp_mask = wakeup_mask;
	while (tmp_mask) {
		uint src_index, comb_bit, reg_index, reg_bit;
		uint tmp_mask1 = tmp_mask & ~(tmp_mask -1U); /* 最低为1的位. */
		tmp_mask &= ~tmp_mask1;
		src_index = __ffs(tmp_mask1);
		BUG_ON(src_index >= OWL_PMIC_WAKEUP_SRC_CNT);

		comb_bit = wakeup_src_desc->bit_tbl[src_index][0];
		if (comb_bit == 255) {
			if (wakeup_src & (1U<<src_index)) {
				dev_warn(atc260x_pm->dev, "%s() src %u (mask 0x%x) not support\n",
					__func__, src_index, (1U<<src_index));
			}
			continue;
		}
		reg_index = comb_bit / 16U;
		reg_bit = comb_bit % 16U;
		BUG_ON(reg_index >= ARRAY_SIZE(reg_masks));

		reg_masks[reg_index] |= 1U << reg_bit;
		if (wakeup_src & (1U<<src_index)) {
			reg_vals[reg_index] |= 1U << reg_bit;
		}
	}
	for (i = 0; i < ARRAY_SIZE(reg_masks); i++) {
		reg_addr = wakeup_src_desc->regs[i];
		if (reg_masks[i] != 0 && reg_addr != 0xffff) {
			ret = atc260x_reg_setbits(atc260x_pm->atc260x,
					reg_addr, reg_masks[i], reg_vals[i]);
			if (ret) {
				dev_err(atc260x_pm->dev, "%s() io err, ret=%d\n", __func__, ret);
				return -EIO;
			}
			dev_info(atc260x_pm->dev, "%s() setup reg=0x%x msk=0x%x val=0x%x readback=0x%x\n",
					__func__, reg_addr, reg_masks[i], reg_vals[i],
					atc260x_reg_read(atc260x_pm->atc260x, reg_addr));
		}
	}

	/* set detect enable bit(s) */
	reg_val = reg_mask = 0;
	switch (atc260x_pm->pmic_type) {
	case ATC260X_ICTYPE_2603A:
		if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_TP) {
			reg_mask |= PMU_SYS_CTL5_TP_DECT_EN;
			if (wakeup_src & OWL_PMIC_WAKEUP_SRC_TP){
				reg_val |= PMU_SYS_CTL5_TP_DECT_EN;
				/* must eable TEN if TP as wakeup source */
			}
		}

		if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_REMCON) {
			reg_mask |= PMU_SYS_CTL5_REMCON_DECT_EN;
			if (wakeup_src & OWL_PMIC_WAKEUP_SRC_REMCON)
				reg_val |= PMU_SYS_CTL5_REMCON_DECT_EN;
		}

		/* for re-plugin wakeup, not disable wakeup detect for wall/usb cable */
		#if 0
		if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_WALL_IN) {
			reg_mask |= PMU_SYS_CTL5_WALLWKDTEN;
			if (wakeup_src & OWL_PMIC_WAKEUP_SRC_WALL_IN)
				reg_val |= PMU_SYS_CTL5_WALLWKDTEN;
		}

		if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_VBUS_IN) {
			reg_mask |= PMU_SYS_CTL5_VBUSWKDTEN;
			if (wakeup_src & OWL_PMIC_WAKEUP_SRC_VBUS_IN)
				reg_val |= PMU_SYS_CTL5_VBUSWKDTEN;
		}
		#endif
		atc260x_reg_setbits(atc260x_pm->atc260x,
			ATC2603A_PMU_SYS_CTL5, reg_mask, reg_val);
		break;

	case ATC260X_ICTYPE_2603C:
		if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_REMCON) {
			reg_mask |= PMU_SYS_CTL5_REMCON_DECT_EN;
			if (wakeup_src & OWL_PMIC_WAKEUP_SRC_REMCON)
				reg_val |= PMU_SYS_CTL5_REMCON_DECT_EN;
		}
		atc260x_reg_setbits(atc260x_pm->atc260x,
			ATC2603C_PMU_SYS_CTL5, reg_mask, reg_val);

	case ATC260X_ICTYPE_2609A:
		if (wakeup_mask & OWL_PMIC_WAKEUP_SRC_REMCON) {
			reg_mask |= PMU_SYS_CTL5_REMCON_DECT_EN;
			if (wakeup_src & OWL_PMIC_WAKEUP_SRC_REMCON)
				reg_val |= PMU_SYS_CTL5_REMCON_DECT_EN;
		}
		atc260x_reg_setbits(atc260x_pm->atc260x,
			ATC2609A_PMU_SYS_CTL5, reg_mask, reg_val);
	}

	return 0;
}

static int _atc260x_pm_get_wakeup_src_inner(struct atc260x_pm_dev *atc260x_pm, bool get_flag)
{
	const struct atc260x_pm_wakeup_src_reg_desc *wakeup_src_desc;
	uint i, comb_bit, reg_index, reg_bit, reg_addr, wakeup_src_bm;
	u16 reg_vals[4];
	int ret;

	get_flag = !!get_flag;
	wakeup_src_desc = sc_atc260x_pm_wakeup_src_desc_tbl[atc260x_pm->pmic_type];

	for (i = 0; i < ARRAY_SIZE(reg_vals); i++) {
		reg_addr = wakeup_src_desc->regs[i];
		if (reg_addr != 0xffff) {
			ret = atc260x_reg_read(atc260x_pm->atc260x, wakeup_src_desc->regs[i]);
			if (ret < 0) {
				dev_err(atc260x_pm->dev, "%s() io err, ret=%d\n", __func__, ret);
				return ret;
			}
			reg_vals[i] = ret;
		} else {
			reg_vals[i] = 0;
		}
	}

	wakeup_src_bm = 0;
	for (i = 0; i < OWL_PMIC_WAKEUP_SRC_CNT; i++) {
		comb_bit = wakeup_src_desc->bit_tbl[i][get_flag];
		if (comb_bit == 255) /* not support ? */
			continue;
		reg_index = comb_bit / 16U;
		reg_bit = comb_bit % 16U;
		BUG_ON(reg_index >= ARRAY_SIZE(reg_vals));
		if (reg_vals[reg_index] & (1U << reg_bit)) {
			wakeup_src_bm |= 1U << i;
		}
	}

	return wakeup_src_bm;
}

static int _atc260x_pm_set_wakeup_src(uint wakeup_mask, uint wakeup_src)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	return _atc260x_pm_set_wakeup_src_inner(atc260x_pm, wakeup_mask, wakeup_src);
}

static int _atc260x_pm_get_wakeup_src(void)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	return _atc260x_pm_get_wakeup_src_inner(atc260x_pm, 0);
}

static int _atc260x_pm_get_wakeup_flag_inner(struct atc260x_pm_dev *atc260x_pm)
{
	int ret;

	ret = _atc260x_pm_get_wakeup_src_inner(atc260x_pm, 1);
	if (ret < 0)
		return ret;
	atc260x_pm->active_wakeup_srcs = ret;
	dev_info(atc260x_pm->dev, "translated wakeup falgs: 0x%x\n", ret);
	return 0;
}

static int _atc260x_pm_get_wakeup_flag(void)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	return atc260x_pm->active_wakeup_srcs;
}

static int _atc260x_pm_setup_rtc_alarm(struct atc260x_pm_dev *atc260x_pm,
		uint seconds, uint safe_margin,
		struct atc260x_pm_alarm_save *p_old_alarm)
{
	struct rtc_time rtc_tmp_tm;
	uint reg_rtc_ms, reg_rtc_h, reg_rtc_ymd, reg_rtc_dc;
	uint reg_rtc_msalm, reg_rtc_halm, reg_rtc_ymdalm;
	uint rtc_ms, rtc_h, rtc_ymd, rtc_cen, rtc_msalm, rtc_halm, rtc_ymdalm;
	ulong rtc_times, rtc_alm_times;
	int ret1, ret2, ret3, ret4, ret5, ret6, ret7;

	dev_info(atc260x_pm->dev, "%s() enter, seconds=%u safe_margin=%u\n",
		__func__, seconds, safe_margin);

	reg_rtc_ms      = sc_atc260x_pm_regtbl_rtc_ms[atc260x_pm->pmic_type];
	reg_rtc_h       = sc_atc260x_pm_regtbl_rtc_h[atc260x_pm->pmic_type];
	reg_rtc_ymd     = sc_atc260x_pm_regtbl_rtc_ymd[atc260x_pm->pmic_type];
	reg_rtc_dc      = sc_atc260x_pm_regtbl_rtc_dc[atc260x_pm->pmic_type];
	reg_rtc_msalm   = sc_atc260x_pm_regtbl_rtc_msalm[atc260x_pm->pmic_type];
	reg_rtc_halm    = sc_atc260x_pm_regtbl_rtc_halm[atc260x_pm->pmic_type];
	reg_rtc_ymdalm  = sc_atc260x_pm_regtbl_rtc_ymdalm[atc260x_pm->pmic_type];

	ret1 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_ms);
	ret2 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_h);
	ret3 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_ymd);
	ret4 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_dc);
	ret5 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_msalm);
	ret6 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_halm);
	ret7 = atc260x_reg_read(atc260x_pm->atc260x, reg_rtc_ymdalm);
	if (ret1 < 0 || ret2 < 0 || ret3 < 0 || ret4 < 0 || ret5 < 0 || ret6 < 0 || ret7 < 0) {
		dev_err(atc260x_pm->dev, "%s() read IO err\n", __func__);
		return -EIO;
	}
	rtc_ms  = (uint)ret1 & 0xfffU;
	rtc_h   = (uint)ret2 & 0x1fU;
	rtc_ymd = (uint)ret3;
	rtc_cen = (uint)ret4 & 0x7fU;
	rtc_msalm  = (uint)ret5 & 0xfffU;
	rtc_halm   = (uint)ret6 & 0x1fU;
	rtc_ymdalm = (uint)ret7;

	rtc_times = mktime(
			rtc_cen * 100 + ((rtc_ymd >> 9) & 0x7fU),
			(rtc_ymd >> 5) & 0xfU,
			rtc_ymd & 0x1fU,
			rtc_h,
			(rtc_ms >> 6) & 0x3fU,
			rtc_ms & 0x3fU);
	rtc_alm_times = mktime(
			rtc_cen * 100 + ((rtc_ymdalm >> 9) & 0x7fU),
			(rtc_ymdalm >> 5) & 0xfU,
			rtc_ymdalm & 0x1fU,
			rtc_halm,
			(rtc_msalm >> 6) & 0x3fU,
			rtc_msalm & 0x3fU);

	/* save old alarm if needed. */
	if (rtc_times < rtc_alm_times) {
		/* Alarm already activated. */
		if ((rtc_alm_times - rtc_times) <= safe_margin) {
			dev_warn(atc260x_pm->dev, "%s() old alarm will be postponed (+ ~%us)\n",
				__func__, safe_margin);
			rtc_alm_times = rtc_times + safe_margin;
		}
		/* save old alarm */
		rtc_time_to_tm(rtc_alm_times, &rtc_tmp_tm);
		p_old_alarm->msalm =
			((rtc_tmp_tm.tm_min) << 6) |
			rtc_tmp_tm.tm_sec;
		p_old_alarm->halm =
			rtc_tmp_tm.tm_hour;
		p_old_alarm->ymdalm =
			((rtc_tmp_tm.tm_year+1900 - rtc_cen*100) << 9) |
			((rtc_tmp_tm.tm_mon +1) << 5) |
			rtc_tmp_tm.tm_mday;
		dev_info(atc260x_pm->dev, "%s() saved old alarm %u-%u-%u %u:%u:%u, "
			"msalm=0x%x halm=0x%x ymdalm=0x%x\n",
			__func__,
			rtc_tmp_tm.tm_year+1900, rtc_tmp_tm.tm_mon+1, rtc_tmp_tm.tm_mday,
			rtc_tmp_tm.tm_hour, rtc_tmp_tm.tm_min, rtc_tmp_tm.tm_sec,
			p_old_alarm->msalm, p_old_alarm->halm, p_old_alarm->ymdalm);
	} else {
		dev_info(atc260x_pm->dev, "%s() no need to save old alarm\n", __func__);
		memset(p_old_alarm, 0, sizeof(*p_old_alarm));
	}

	/* set new alarm */
	rtc_alm_times = rtc_times + seconds;
	rtc_time_to_tm(rtc_alm_times, &rtc_tmp_tm);
	rtc_msalm =
		((rtc_tmp_tm.tm_min) << 6) |
		rtc_tmp_tm.tm_sec;
	rtc_halm =
		rtc_tmp_tm.tm_hour;
	rtc_ymdalm =
		((rtc_tmp_tm.tm_year+1900 - rtc_cen*100) << 9) |
		((rtc_tmp_tm.tm_mon+1) << 5) |
		rtc_tmp_tm.tm_mday;
	ret5 = atc260x_reg_write(atc260x_pm->atc260x, reg_rtc_msalm, rtc_msalm);
	ret6 = atc260x_reg_write(atc260x_pm->atc260x, reg_rtc_halm, rtc_halm);
	ret7 = atc260x_reg_write(atc260x_pm->atc260x, reg_rtc_ymdalm, rtc_ymdalm);
	if (ret5 || ret6 || ret7) {
		dev_err(atc260x_pm->dev, "%s() write IO err\n", __func__);
		return -EIO;
	}
	dev_info(atc260x_pm->dev, "%s() new alarm set to %u-%u-%u %u:%u:%u, "
		"msalm=0x%x halm=0x%x ymdalm=0x%x\n",
		__func__,
		rtc_tmp_tm.tm_year+1900, rtc_tmp_tm.tm_mon+1, rtc_tmp_tm.tm_mday,
		rtc_tmp_tm.tm_hour, rtc_tmp_tm.tm_min, rtc_tmp_tm.tm_sec,
		rtc_msalm, rtc_halm, rtc_ymdalm);

	dev_info(atc260x_pm->dev, "%s() exit\n", __func__);
	return 0;
}

static int _atc260x_pm_prepare_suspend(void)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();

	dev_info(atc260x_pm->dev, "%s() enter\n", __func__);

	/* avoid using standard SPI/I2C interface that maybe halt in suspend process */
	atc260x_set_reg_direct_access(atc260x_pm->atc260x, true);

	dev_info(atc260x_pm->dev, "%s() exit\n", __func__);
	return 0;
}

static int _set_s2_mode(struct atc260x_pm_dev *atc260x_pm)
{
	struct device_node *of_parent_node;
	int ret;

	of_parent_node = of_get_parent(atc260x_pm->dev->of_node);
	if (of_parent_node == NULL) {
		dev_err(atc260x_pm->dev, "%s() can not get parent of_nade\n", __func__);
		return -ENODEV;
	}
	
	ret = of_property_read_u32(of_parent_node, "s2_mode", &s2_mode);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() can not get of_property s2_mode \n", __func__);
		return -ENODEV;
	}
	dev_info(atc260x_pm->dev, "got s2_mode: %u\n", s2_mode);

	return 0;	
}

/* only prepare the env, not disable S1 immediately */
static int _atc260x_pm_enter_suspend(void)
{
	struct atc260x_pm_dev *atc260x_pm;
	struct atc260x_dev *atc260x;
	void (*p_cpu_resume)(void);
	uint reg_sysctl2, reg_sysctl3;
	uint reg_sysctl3_val;
	phys_addr_t resume_phy_address;
	int i, ret;

	atc260x_pm = _get_curr_atc260x_pm_obj();
	atc260x = atc260x_pm->atc260x;

	dev_info(atc260x_pm->dev, "%s() enter\n", __func__);

	reg_sysctl2 = sc_atc260x_pm_regtbl_sysctl2[atc260x_pm->pmic_type];
	reg_sysctl3 = sc_atc260x_pm_regtbl_sysctl3[atc260x_pm->pmic_type];

	reg_sysctl3_val = PMU_SYS_CTL3_EN_S2;

	/* check S2->S3 timer */
	ret = atc260x_reg_read(atc260x, reg_sysctl2);
	if (ret < 0) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		return ret;
	}
	if ((uint)ret & PMU_SYS_CTL2_S2_TIMER_EN) {
		reg_sysctl3_val |= PMU_SYS_CTL3_EN_S3;
	}

	/* save resume address to pstore */
	p_cpu_resume = symbol_get(owl_cpu_resume);
	if (!p_cpu_resume || IS_ERR(p_cpu_resume))
	{
		dev_err(atc260x_pm->dev, "%s() symbol owl_cpu_resume not found", __func__);
		return -EINVAL;
	}
	resume_phy_address = (phys_addr_t)virt_to_phys(p_cpu_resume);
	dev_info(atc260x_pm->dev, "%s() owl_cpu_resume @ 0x%lx (phy 0x%lx)\n",
		__func__, (ulong)p_cpu_resume, (ulong)resume_phy_address);
		
	if(s2_mode)
		resume_phy_address |= (1UL << (sizeof(resume_phy_address)*8-1)); /* set highest bit */
	
	ret = atc260x_pstore_set(atc260x,
		ATC260X_PSTORE_TAG_RESUME_ADDR, resume_phy_address);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		return ret;
	}
	#ifdef CONFIG_64BIT
	ret = atc260x_pstore_set(atc260x,
		ATC260X_PSTORE_TAG_RESUME_ADDR_H, resume_phy_address >> 32);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		return ret;
	}
	#endif

	/* set FW suspend flag */
	ret = atc260x_pstore_set(atc260x_pm->atc260x, ATC260X_PSTORE_TAG_FW_S2, 1);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		return ret;
	}

	if(s2_mode) {
		/* suspend 分两个阶段进行:
		 * 1.是kernel这边初进S2,
		 * 2.是alarm唤醒进u-boot, u-boot做一些省电配置, 再进S2. */

		/* set phase #1 wakeup alarm */
		
		ret = _atc260x_pm_setup_rtc_alarm(atc260x_pm, 5, 15,
				(struct atc260x_pm_alarm_save *)0xc0000400);
		if (ret) {
			dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
			return ret;
		}
	}

	/* set S2 S3  */
	for (i = 0; i < 10; i++) {
		ret = atc260x_reg_setbits(atc260x, reg_sysctl3,
				PMU_SYS_CTL3_EN_S2 | PMU_SYS_CTL3_EN_S3, reg_sysctl3_val);
		if (ret == 0) {
			break;
		}
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		mdelay(1);
	}
	if (ret) {
		return ret;
	}

	/* clean old wakeup_srcs record. */
	atc260x_pm->active_wakeup_srcs = 0;

	dev_info(atc260x_pm->dev, "%s() exit\n", __func__);
	return 0;
}

static int _atc260x_pm_wakeup(void)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	int ret;

	dev_info(atc260x_pm->dev, "%s() enter\n", __func__);

	atc260x_set_reg_direct_access(atc260x_pm->atc260x, true);

	/* get get_wakeup_flag */
	ret = _atc260x_pm_get_wakeup_flag_inner(atc260x_pm);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() get_wakeup_flag failed\n", __func__);
	}

	/* clear FW suspend flag */
	ret = atc260x_pstore_set(atc260x_pm->atc260x, ATC260X_PSTORE_TAG_FW_S2, 0);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() pstore access failed\n", __func__);
	}

	dev_info(atc260x_pm->dev, "%s() exit\n", __func__);
	return 0;
}

static int _atc260x_pm_finsh_wakeup(void)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	dev_info(atc260x_pm->dev, "%s() enter\n", __func__);
	atc260x_set_reg_direct_access(atc260x_pm->atc260x, false);
	return 0;
}

static int _atc260x_pm_prepare_powerdown(void)
{
	/* switch to direct access,
	 * avoid using standard SPI/I2C interface that maybe halt in pwrdwn process */
	atc260x_set_reg_direct_access(_get_curr_atc260x_pm_obj()->atc260x, true);
	return 0;
}

static int _atc260x_pm_powerdown(uint deep_pwrdn, uint for_upgrade)
{
	struct atc260x_pm_dev *atc260x_pm;
	struct atc260x_dev *atc260x;
	uint reg_sysctl1, reg_sysctl3, reg_mask, reg_val;
	int ret;

	atc260x_pm = _get_curr_atc260x_pm_obj();
	atc260x = atc260x_pm->atc260x;

	dev_info(atc260x_pm->dev, "%s() enter, deep_pwrdn=%u for_upgrade=%u\n",
		__func__, deep_pwrdn, for_upgrade);

	if (for_upgrade) {
		/* clear pstore */
		ret = atc260x_pstore_reset_all(atc260x);
		if (ret) {
			dev_err(atc260x_pm->dev, "%s() failed to reset pstore\n", __func__);
		}
	}

	reg_sysctl1 = sc_atc260x_pm_regtbl_sysctl1[atc260x_pm->pmic_type];
	reg_sysctl3 = sc_atc260x_pm_regtbl_sysctl3[atc260x_pm->pmic_type];

	reg_mask = PMU_SYS_CTL3_EN_S2 | PMU_SYS_CTL3_EN_S3;
	reg_val = deep_pwrdn ? 0 : PMU_SYS_CTL3_EN_S3;
	ret = atc260x_reg_setbits(atc260x, reg_sysctl3, reg_mask, reg_val);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		return ret;
	}

	pr_alert("%s() : powerdown.......................\n", __func__);

	ret = atc260x_reg_setbits(atc260x, reg_sysctl1, PMU_SYS_CTL1_EN_S1, 0);
	while (ret == 0) { /* wait for powerdown if success. */
		mdelay(200); /* DO NOT use msleep() here! non-schedulable contex! */
		dev_warn(atc260x_pm->dev, "%s() wait for powerdown...\n", __func__);
	}
	return ret;
}

static int _atc2603a_pm_do_reboot(struct atc260x_pm_dev *atc260x_pm)
{
	struct atc260x_pm_alarm_save old_alarm;
	int ret, ret1, ret2;

	/* 旧的5302没有软件触发的reset功能, 只能用PowerDown+Alarm来模拟. */

	/* disable WALL/VBUS wakeup, enable alarm wakeup. */
	ret = _atc260x_pm_set_wakeup_src_inner(atc260x_pm,
		(OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_VBUS_IN |
		OWL_PMIC_WAKEUP_SRC_ALARM),
		OWL_PMIC_WAKEUP_SRC_ALARM);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() clear VBUS&WALL wakeup err, ret=%d\n",
			__func__, ret);
		return ret;
	}

	/* set alarm */
	ret = _atc260x_pm_setup_rtc_alarm(atc260x_pm, 3, 8, &old_alarm);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() setup alarm err, ret=%d\n", __func__, ret);
		return ret;
	}

	/* backup old alarm data to pstore */
	ret = atc260x_pstore_set(atc260x_pm->atc260x,
		ATC260X_PSTORE_TAG_RTC_MSALM, old_alarm.msalm);
	ret1 = atc260x_pstore_set(atc260x_pm->atc260x,
		ATC260X_PSTORE_TAG_RTC_HALM, old_alarm.halm);
	ret2 = atc260x_pstore_set(atc260x_pm->atc260x,
		ATC260X_PSTORE_TAG_RTC_YMDALM, old_alarm.ymdalm);
	if (ret || ret1 || ret2) {
		dev_err(atc260x_pm->dev, "%s() faile to save old alarm\n", __func__);
		return -EIO;
	}

	/* enter S3 */
	ret = _atc260x_pm_powerdown(false, false);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() faile to powerdown, ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

extern void owl_switch_jtag(void);

static int _atc2603c_9a_pm_do_reboot(struct atc260x_pm_dev *atc260x_pm)
{
	/* 新的IC带软件触发的reset功能, 直接使用即可. */
	struct atc260x_dev *atc260x;
	uint reg_sysctl0, reg_sysctl3;
	int ret;

	atc260x = atc260x_pm->atc260x;

	/* clear all wakeup source except on/off and p_reset */
	ret = _atc260x_pm_set_wakeup_src_inner(atc260x_pm,
		OWL_PMIC_WAKEUP_SRC_ALL,
		OWL_PMIC_WAKEUP_SRC_ONOFF_LONG | OWL_PMIC_WAKEUP_SRC_RESET);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() clear setup wakeup err, ret=%d\n",
			__func__, ret);
		return ret;
	}

	/* set S3, clear S2 */
	reg_sysctl3 = sc_atc260x_pm_regtbl_sysctl3[atc260x_pm->pmic_type];
	ret = atc260x_reg_setbits(atc260x, reg_sysctl3,
			PMU_SYS_CTL3_EN_S2 | PMU_SYS_CTL3_EN_S3, PMU_SYS_CTL3_EN_S3);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		return ret;
	}

	reg_sysctl0 = sc_atc260x_pm_regtbl_sysctl0[atc260x_pm->pmic_type];

	owl_switch_jtag();
	pr_alert("%s() : reboot.......................\n", __func__);

	ret = atc260x_reg_setbits(atc260x, reg_sysctl0, (1U << 10), (1U << 10));
	while (ret == 0) { /* wait for reboot if success. */
		mdelay(200); /* DO NOT use msleep() here! non-schedulable contex! */
		dev_warn(atc260x_pm->dev, "%s() wait for reboot...\n", __func__);
	}
	return ret;
}

static int _atc260x_pm_reboot(uint tgt)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	struct atc260x_dev *atc260x = atc260x_pm->atc260x;
	uint flag_adfu, flag_recovery, flag_dis_mchrg;
	int ret;

	dev_info(atc260x_pm->dev, "%s() enter, tgt=%u\n", __func__, tgt);

	flag_adfu = flag_recovery = flag_dis_mchrg = 0;
	switch (tgt) {
	case OWL_PMIC_REBOOT_TGT_NORMAL:
		flag_adfu = flag_recovery = flag_dis_mchrg = 0;
		break;
	case OWL_PMIC_REBOOT_TGT_SYS:
		flag_adfu = flag_recovery = 0;
		flag_dis_mchrg = 1;
		break;
	case OWL_PMIC_REBOOT_TGT_ADFU:
		flag_adfu = flag_dis_mchrg = 1;
		flag_recovery = 0;
		break;
	case OWL_PMIC_REBOOT_TGT_RECOVERY:
		flag_recovery = flag_dis_mchrg = 1;
		flag_adfu = 0;
		break;
	case OWL_PMIC_REBOOT_TGT_BOOTLOADER:
		flag_recovery = 2;
		flag_dis_mchrg = 0;
		flag_adfu = 0;
		break;	
	case OWL_PMIC_REBOOT_TGT_FASTBOOT:
		flag_recovery = 3;
		flag_dis_mchrg = 0;
		flag_adfu = 0;
		break;		
		
	}
	ret = atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_REBOOT_ADFU, flag_adfu);
	ret |= atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_REBOOT_RECOVERY, flag_recovery);
	ret |= atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_DIS_MCHRG, flag_dis_mchrg);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() failed to update pstore\n", __func__);
		return -EIO;
	}

	ret = -ENODEV;
	switch (atc260x_pm->pmic_type) {
	case ATC260X_ICTYPE_2603A:
		ret = _atc2603a_pm_do_reboot(atc260x_pm);
		break;
	case ATC260X_ICTYPE_2603C:
	case ATC260X_ICTYPE_2609A:
		ret = _atc2603c_9a_pm_do_reboot(atc260x_pm);
		break;
	}
	dev_err(atc260x_pm->dev, "%s() failed, ret=%d\n", __func__, ret);
	return ret;
}

static int _atc260x_pm_get_bus_info(uint *bus_num, uint *addr, uint *ic_type)
{
	/* sys pm 那边进suspend的汇编代码要用到这些信息. */
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	struct atc260x_dev *atc260x = atc260x_pm->atc260x;
	atc260x_get_bus_info(atc260x, bus_num, addr);
	*ic_type = atc260x_pm->pmic_type;
	return 0;
}

static struct owl_pmic_pm_ops atc260x_pm_pmic_ops = {
	.set_wakeup_src = _atc260x_pm_set_wakeup_src,
	.get_wakeup_src = _atc260x_pm_get_wakeup_src,
	.get_wakeup_flag = _atc260x_pm_get_wakeup_flag,

	.shutdown_prepare = _atc260x_pm_prepare_powerdown,
	.powerdown = _atc260x_pm_powerdown,
	.reboot = _atc260x_pm_reboot,

	.suspend_prepare = _atc260x_pm_prepare_suspend,
	.suspend_enter = _atc260x_pm_enter_suspend,
	.suspend_wake = _atc260x_pm_wakeup,
	.suspend_finish = _atc260x_pm_finsh_wakeup,

	.get_bus_info = _atc260x_pm_get_bus_info,
};

/* ---------- sysfs interface ----------------------------------------------- */

static ssize_t atc260x_wakeup_attr_shown(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int index = to_atc260x_pm_attr(attr)->index;
	int wakeup_src;

	pr_debug("%s %d: index %d\n", __FUNCTION__, __LINE__, index);

	wakeup_src = _atc260x_pm_get_wakeup_src();

	wakeup_src = (wakeup_src & (1 << index)) ? 1 : 0;

	return sprintf(buf, "%d\n", wakeup_src);
}

static ssize_t atc260x_wakeup_attr_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int index = to_atc260x_pm_attr(attr)->index;
	unsigned long val = 0;
	int ret;

	pr_debug("%s %d: index %d\n", __FUNCTION__, __LINE__, index);

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	_atc260x_pm_set_wakeup_src(1 << index, (!!val) << index);

	return count;
}


static ssize_t atc260x_wakeup_srcs_attr_shown(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int wakeup_src;

	pr_debug("%s %d:\n", __FUNCTION__, __LINE__);

	wakeup_src = _atc260x_pm_get_wakeup_src();

	return sprintf(buf, "0x%x\n", wakeup_src);
}

static ssize_t atc260x_wakeup_srcs_attr_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long wakeup_src = 0;
	int ret;

	pr_debug("%s %d:\n", __FUNCTION__, __LINE__);

	ret = strict_strtoul(buf, 0, &wakeup_src);
	if (ret < 0)
		return ret;

	if (wakeup_src & ~OWL_PMIC_WAKEUP_SRC_ALL)
		return -EINVAL;

	_atc260x_pm_set_wakeup_src(OWL_PMIC_WAKEUP_SRC_ALL, wakeup_src);

	return count;
}

static ssize_t atc260x_s2tos3_timeout_shown(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct atc260x_pm_dev *atc260x_pm;
	int ret, i;
	int len = 0;

	atc260x_pm = _get_curr_atc260x_pm_obj();

	pr_debug("%s %d:\n", __FUNCTION__, __LINE__);

	ret = atc260x_reg_read(atc260x_pm->atc260x,
			sc_atc260x_pm_regtbl_sysctl2[atc260x_pm->pmic_type]);

	if (ret & PMU_SYS_CTL2_S2_TIMER_EN) {
		i = (ret & PMU_SYS_CTL2_S2_TIMER_MASK) >> PMU_SYS_CTL2_S2_TIMER_SHIFT;
		len = sprintf(buf, "[%lu] ", s2tos3_timeout_table[i]);
	} else {
		len = sprintf(buf, "[%d] ", 0);
	}

	for (i = 0; i < ARRAY_SIZE(s2tos3_timeout_table); i++) {
		len += sprintf(buf + len, "%lu ", s2tos3_timeout_table[i]);
	}

	len += sprintf(buf + len, " (min)\n");

	return len;
}

static ssize_t atc260x_s2tos3_timeout_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *instr, size_t bytes)
{
	struct atc260x_pm_dev *atc260x_pm;
	unsigned long timeout = 0;
	int ret, i, tbl_size;

	atc260x_pm = _get_curr_atc260x_pm_obj();

	pr_debug("%s %d:\n", __FUNCTION__, __LINE__);

	tbl_size = ARRAY_SIZE(s2tos3_timeout_table);

	ret = strict_strtoul(instr, 0, &timeout);
	if (ret)
		return ret;

	if (timeout == 0) {
		/* disable S2->S3 timer */
		ret = atc260x_reg_setbits(atc260x_pm->atc260x,
			ATC2603A_PMU_SYS_CTL2,
			PMU_SYS_CTL2_S2_TIMER_EN,
			0);

		return ret;
	}

	for (i = 0; i < tbl_size; i++) {
		if (timeout == s2tos3_timeout_table[i])
			break;
	}

	if (i == tbl_size)
		return -EINVAL;

	/* enable S2->S3 timer */
	ret = atc260x_reg_setbits(atc260x_pm->atc260x,
		sc_atc260x_pm_regtbl_sysctl2[atc260x_pm->pmic_type],
		PMU_SYS_CTL2_S2_TIMER_MASK | PMU_SYS_CTL2_S2_TIMER_EN,
		(i << PMU_SYS_CTL2_S2_TIMER_SHIFT) | PMU_SYS_CTL2_S2_TIMER_EN);

	atc260x_reg_read(atc260x_pm->atc260x,
		sc_atc260x_pm_regtbl_sysctl2[atc260x_pm->pmic_type]);

	return bytes;
}


static ssize_t atc260x_s3tos4_timeout_shown(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct atc260x_pm_dev *atc260x_pm;
	int ret, i;
	int len = 0;

	atc260x_pm = _get_curr_atc260x_pm_obj();

	pr_debug("%s %d:\n", __FUNCTION__, __LINE__);

	ret = atc260x_reg_read(atc260x_pm->atc260x,
			sc_atc260x_pm_regtbl_sysctl3[atc260x_pm->pmic_type]);

	if (ret & PMU_SYS_CTL3_S3_TIMER_EN) {
		i = (ret & PMU_SYS_CTL3_S3_TIMER_MASK) >> PMU_SYS_CTL3_S3_TIMER_SHIFT;
		len = sprintf(buf, "[%lu] ", s2tos3_timeout_table[i]);
	} else {
		len = sprintf(buf, "[%d] ", 0);
	}

	for (i = 0; i < ARRAY_SIZE(s3tos4_timeout_table); i++) {
		len += sprintf(buf + len, "%lu ", s3tos4_timeout_table[i]);
	}

	len += sprintf(buf + len, " (min)\n");

	return len;
}

static ssize_t atc260x_s3tos4_timeout_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *instr, size_t bytes)
{
	struct atc260x_pm_dev *atc260x_pm;
	unsigned long timeout = 0;
	int ret, i, tbl_size;

	atc260x_pm = _get_curr_atc260x_pm_obj();

	pr_debug("%s %d:\n", __FUNCTION__, __LINE__);

	tbl_size = ARRAY_SIZE(s3tos4_timeout_table);

	ret = strict_strtoul(instr, 0, &timeout);
	if (ret)
		return ret;

	if (timeout == 0) {
		/* disable S3->S4 timer */
		ret = atc260x_reg_setbits(atc260x_pm->atc260x,
			ATC2603A_PMU_SYS_CTL3,
			PMU_SYS_CTL3_S3_TIMER_EN,
			0);

		return ret;
	}

	for (i = 0; i < tbl_size; i++) {
		if (timeout == s3tos4_timeout_table[i])
			break;
	}

	if (i == tbl_size)
		return -EINVAL;

	ret = atc260x_reg_setbits(atc260x_pm->atc260x,
		sc_atc260x_pm_regtbl_sysctl3[atc260x_pm->pmic_type],
		PMU_SYS_CTL3_S3_TIMER_MASK | PMU_SYS_CTL3_S3_TIMER_EN,
		(i << PMU_SYS_CTL3_S3_TIMER_SHIFT) | PMU_SYS_CTL3_S3_TIMER_EN);

	return bytes;
}

#define ATC260X_PM_ATTR(_name, _mode, _show, _store, _index)    \
	{ .attr = __ATTR(_name, _mode, _show, _store),  \
	  .index = _index }

#define WAKEUP_ATTR(_name, _index)  \
static struct atc260x_pm_attribute atc260x_wakeup_attr_##_name      \
	= ATC260X_PM_ATTR(_name, S_IRUGO | S_IWUSR, atc260x_wakeup_attr_shown, atc260x_wakeup_attr_store, _index)

#define WAKEUP_ATTR_PTR(_name) \
	&atc260x_wakeup_attr_##_name.attr.attr

#define PM_ATTR(_name, _mode, _show, _store)    \
static struct atc260x_pm_attribute atc260x_pm_attr_##_name      \
	= ATC260X_PM_ATTR(_name, _mode, _show, _store, 0)

#define PM_ATTR_PTR(_name)  \
	&atc260x_pm_attr_##_name.attr.attr

WAKEUP_ATTR(wken_ir, 0);
WAKEUP_ATTR(wken_reset, 1);
WAKEUP_ATTR(wken_hdsw, 2);
WAKEUP_ATTR(wken_alarm, 3);
WAKEUP_ATTR(wken_remcon, 4);
WAKEUP_ATTR(wken_tp, 5);
WAKEUP_ATTR(wken_wkirq, 6);
WAKEUP_ATTR(wken_onoff_short, 7);
WAKEUP_ATTR(wken_onoff_long, 8);
WAKEUP_ATTR(wken_wall, 9);
WAKEUP_ATTR(wken_vbus, 10);
WAKEUP_ATTR(wken_restart, 11);
WAKEUP_ATTR(wken_sgpio, 12);

PM_ATTR(wakeup_srcs, S_IRUGO | S_IWUSR, atc260x_wakeup_srcs_attr_shown, \
	atc260x_wakeup_srcs_attr_store);

PM_ATTR(s2tos3_timeout, S_IRUGO | S_IWUSR, atc260x_s2tos3_timeout_shown, \
	atc260x_s2tos3_timeout_store);
PM_ATTR(s3tos4_timeout, S_IRUGO | S_IWUSR, atc260x_s3tos4_timeout_shown, \
	atc260x_s3tos4_timeout_store);


static struct attribute *atc260x_pm_attrs[] = {
	WAKEUP_ATTR_PTR(wken_ir),
	WAKEUP_ATTR_PTR(wken_reset),
	WAKEUP_ATTR_PTR(wken_hdsw),
	WAKEUP_ATTR_PTR(wken_alarm),
	WAKEUP_ATTR_PTR(wken_remcon),
	WAKEUP_ATTR_PTR(wken_tp),
	WAKEUP_ATTR_PTR(wken_wkirq),
	WAKEUP_ATTR_PTR(wken_onoff_short),
	WAKEUP_ATTR_PTR(wken_onoff_long),
	WAKEUP_ATTR_PTR(wken_wall),
	WAKEUP_ATTR_PTR(wken_vbus),
	WAKEUP_ATTR_PTR(wken_restart),
	WAKEUP_ATTR_PTR(wken_sgpio),

	PM_ATTR_PTR(wakeup_srcs),

	PM_ATTR_PTR(s2tos3_timeout),
	PM_ATTR_PTR(s3tos4_timeout),

	NULL,
};

static struct attribute_group asoc_pmattr_group = {
	.name = "wakeups",
	.attrs = atc260x_pm_attrs,
};

static void _clear_status(struct atc260x_pm_dev *atc260x_pm)
{
	struct atc260x_dev *atc260x;
	u32 rtc_msalm, rtc_halm, rtc_ymdalm;
	int ret;

	atc260x = atc260x_pm->atc260x;

	/* disable alarm wake */
	ret = _atc260x_pm_set_wakeup_src_inner(
		atc260x_pm, OWL_PMIC_WAKEUP_SRC_ALARM, 0);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
	}

	/* restore old alarm */
	ret = atc260x_pstore_get(atc260x, ATC260X_PSTORE_TAG_RTC_MSALM, &rtc_msalm);
	ret |= atc260x_pstore_get(atc260x, ATC260X_PSTORE_TAG_RTC_HALM, &rtc_halm);
	ret |= atc260x_pstore_get(atc260x, ATC260X_PSTORE_TAG_RTC_YMDALM, &rtc_ymdalm);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
	}
	if (rtc_msalm != 0 && rtc_halm != 0 && rtc_ymdalm != 0) {
		ret = atc260x_reg_write(atc260x,
			sc_atc260x_pm_regtbl_rtc_msalm[atc260x_pm->pmic_type], rtc_msalm);
		ret |= atc260x_reg_write(atc260x,
			sc_atc260x_pm_regtbl_rtc_halm[atc260x_pm->pmic_type], rtc_halm);
		ret |= atc260x_reg_write(atc260x,
			sc_atc260x_pm_regtbl_rtc_ymdalm[atc260x_pm->pmic_type], rtc_ymdalm);
		if (ret) {
			dev_err(atc260x_pm->dev, "%s() restore old alarm err\n", __func__);
		}
		dev_info(atc260x_pm->dev, "%s() restore old alarm, "
				"msalm=0x%x halm=0x%x ymdalm=0x%x\n",
				__func__, rtc_msalm, rtc_halm, rtc_ymdalm);
	} else {
		dev_info(atc260x_pm->dev, "%s() no need to restore old alarm\n", __func__);
	}

	/* clear pstore */
	ret = atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_REBOOT_ADFU, 0);
	ret |= atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_REBOOT_RECOVERY, 0);
	ret |= atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_DIS_MCHRG, 0);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
	}
	ret = atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_RTC_MSALM, 0);
	ret |= atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_RTC_HALM, 0);
	ret |= atc260x_pstore_set(atc260x, ATC260X_PSTORE_TAG_RTC_YMDALM, 0);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
	}
}

static int atc260x_pm_setup_wall_vbus_wakup_function(struct atc260x_pm_dev *atc260x_pm)
{
	struct device_node *of_parent_node, *of_power_node;
	u32 support_adaptor_type;
	uint reg_mask, reg_val;
	int ret;

	of_parent_node = of_get_parent(atc260x_pm->dev->of_node);
	if (of_parent_node == NULL) {
		dev_err(atc260x_pm->dev, "%s() can not get parent of_nade\n", __func__);
		return -ENODEV;
	}
	of_power_node = of_get_child_by_name(of_parent_node, "atc260x-power");
	if (of_parent_node == NULL) {
		dev_err(atc260x_pm->dev, "%s() can not find power of_nade\n", __func__);
		return -ENODEV;
	}
	ret = of_property_read_u32(of_power_node, "support_adaptor_type", &support_adaptor_type);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() can not get of_property support_adaptor_type \n", __func__);
		return -ENODEV;
	}
	dev_info(atc260x_pm->dev, "got support_adaptor_type: %u\n", support_adaptor_type);

	reg_mask = (3U << 7);
	switch (support_adaptor_type == 1) {
	case 1:
		/* only support WALL */
		reg_val = (1U << 7);
		break;
	case 2:
		/* only support VBUS */
		reg_val = (1U << 8);
		break;
	default :
		reg_val = (3U << 7);
	}
	ret = atc260x_reg_setbits(atc260x_pm->atc260x,
			sc_atc260x_pm_regtbl_sysctl5[atc260x_pm->pmic_type],
			reg_mask, reg_val);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u IO err\n", __func__, __LINE__);
		return ret;
	}

	return 0;
}

static int atc260x_pm_notifier_callback(struct notifier_block *nb, unsigned long event, void *dummy)
{
	struct atc260x_pm_dev *atc260x_pm = _get_curr_atc260x_pm_obj();
	switch (event) {
	case PM_POST_HIBERNATION:    /* Hibernation finished */
		_clear_status(atc260x_pm);
		break;
	}
	return NOTIFY_OK;
}

static int atc260x_pm_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x;
	struct atc260x_pm_dev *atc260x_pm;
	int ret;

	dev_info(&pdev->dev, "Probing...\n");

	atc260x = atc260x_get_parent_dev(&pdev->dev);

	atc260x_pm = devm_kzalloc(&pdev->dev, sizeof(struct atc260x_pm_dev),
				GFP_KERNEL);
	if (!atc260x_pm) {
		dev_err(&pdev->dev, "%s() no mem\n", __func__);
		return -ENOMEM;
	}
	atc260x_pm->dev = &pdev->dev;
	atc260x_pm->atc260x = atc260x;
	atc260x_pm->pmic_type = atc260x_get_ic_type(atc260x);
	atc260x_pm->pmic_ver = atc260x_get_ic_ver(atc260x);

	platform_set_drvdata(pdev, atc260x_pm);

	if (s_current_atc260x_pm_obj == NULL) {
		s_current_atc260x_pm_obj = atc260x_pm;
	}

	ret = atc260x_reg_setbits(atc260x,
		sc_atc260x_pm_regtbl_sysctl1[atc260x_pm->pmic_type],
		PMU_SYS_CTL1_LB_S4_MASK, PMU_SYS_CTL1_LB_S4_3_1V);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
	}
	
	_set_s2_mode(atc260x_pm);
	
	_atc260x_pm_set_wakeup_src_inner(atc260x_pm, OWL_PMIC_WAKEUP_SRC_ALL,
			OWL_PMIC_WAKEUP_SRC_RESET | OWL_PMIC_WAKEUP_SRC_HDSW |
			OWL_PMIC_WAKEUP_SRC_ONOFF_LONG | OWL_PMIC_WAKEUP_SRC_WALL_IN |
			OWL_PMIC_WAKEUP_SRC_VBUS_IN);
	_clear_status(atc260x_pm);

	/* delay some time from s2/s3 to s1 to avoid ddr init fail in bootloader */
	if (atc260x_pm->pmic_type == ATC260X_ICTYPE_2603C) {
		ret = atc260x_reg_setbits(atc260x_pm->atc260x,
			sc_atc260x_pm_regtbl_sysctl3[atc260x_pm->pmic_type],
			ATC2603C_S2S3TOS1_TIMER_EN, ATC2603C_S2S3TOS1_TIMER_EN);
		if (ret) {
			dev_err(atc260x_pm->dev, "%s() %u err\n", __func__, __LINE__);
		}
	}

	ret = atc260x_pm_setup_wall_vbus_wakup_function(atc260x_pm);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() setup_wall_vbus_wakup_function failed\n", __func__);
		/* not serious error, keep going. */
	}

	/* get get_wakeup_flag */
	ret = _atc260x_pm_get_wakeup_flag_inner(atc260x_pm);
	if (ret) {
		dev_err(atc260x_pm->dev, "%s() get_wakeup_flag failed\n", __func__);
		/* not serious error, keep going. */
	}

	/* register PM notify */
	atc260x_pm->pm_nb.notifier_call = atc260x_pm_notifier_callback;
	ret = register_pm_notifier(&(atc260x_pm->pm_nb));
	if (ret) {
		dev_err(atc260x_pm->dev, "failed to register pm_notifier, ret=%d\n", ret);
		goto label_err_lv2;
	}

	ret = sysfs_create_group(power_kobj, &asoc_pmattr_group);
	if (ret) {
		dev_err(atc260x_pm->dev, "failed to create sysfs nodes for atc260x_pm, ret=%d\n", ret);
		goto label_err_lv3;
	}

	owl_pmic_set_pm_ops(&atc260x_pm_pmic_ops);

	dev_info(atc260x_pm->dev, "#####%s PMU_SYS_CTL0:0x%x ####\n", __func__,
		atc260x_reg_read(atc260x, sc_atc260x_pm_regtbl_sysctl0[atc260x_pm->pmic_type]));

	return 0;

	label_err_lv3:
	unregister_pm_notifier(&(atc260x_pm->pm_nb));
	label_err_lv2:
	owl_pmic_set_pm_ops(NULL);
	/*label_err_lv1: */
	platform_set_drvdata(pdev, NULL);
	if (s_current_atc260x_pm_obj == atc260x_pm) {
		s_current_atc260x_pm_obj = NULL;
	}
	return ret;
}

static int atc260x_pm_remove(struct platform_device *pdev)
{
	struct atc260x_pm_dev *atc260x_pm = platform_get_drvdata(pdev);
	sysfs_remove_group(power_kobj, &asoc_pmattr_group);
	unregister_pm_notifier(&(atc260x_pm->pm_nb));
	owl_pmic_set_pm_ops(NULL);
	platform_set_drvdata(pdev, NULL);
	if (s_current_atc260x_pm_obj == atc260x_pm) {
		s_current_atc260x_pm_obj = NULL;
	}
	return 0;
}


static const struct of_device_id atc260x_pm_match[] = {
	{ .compatible = "actions,atc2603a-pm", },
	{ .compatible = "actions,atc2603c-pm", },
	{ .compatible = "actions,atc2609a-pm", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_pm_match);

static struct platform_driver atc260x_pm_driver = {
	.probe = atc260x_pm_probe,
	.remove = atc260x_pm_remove,
	.driver = {
		.name   = "atc260x-pm",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(atc260x_pm_match),
	},
};

static int __init atc260x_pm_init(void)
{
	int ret;
	ret = platform_driver_register(&atc260x_pm_driver);
	if (ret != 0)
		pr_err("Failed to register ATC260X PM driver: %d\n", ret);
	return 0;
}
subsys_initcall(atc260x_pm_init);
/*module_init(atc260x_pm_init); */

static void __exit atc260x_pm_exit(void)
{
	platform_driver_unregister(&atc260x_pm_driver);
}
module_exit(atc260x_pm_exit);

/* Module information */
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("ATC260X PM driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-pm");


