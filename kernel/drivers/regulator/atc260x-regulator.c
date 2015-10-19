/*
 * atc260x_regulator.c  --  regulator driver for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mfd/atc260x/atc260x.h>

#define CONFIG_ATC260x_REGU_FULL_DBG 0
#if CONFIG_ATC260x_REGU_FULL_DBG
#define dev_dbgl(DEV, FMT, ARGS...) dev_info(DEV, FMT, ## ARGS)
#else
#define dev_dbgl(DEV, FMT, ARGS...) do{}while(0)
#endif

#define REGULATOR_MEM_STATE_MODE_NO_RESTORE     BIT(0)

/* UTF-8 encoded! */

/* voltage band 划分规则:
 * 如: 0 4 6 8 13 16 19 77
 * 划为
 *     [0,   4]  间距 4   点数 2
 *     [4,   8]  间距 2   点数 3
 *     [8,  13]  间距 5   点数 2
 *     [13, 19]  间距 3   点数 3
 *     [19, 77]  间距 58  点数 2
 * 除非是固定电压的regulator, 否则不存在只有一个点的band, 相邻band之间总有一个点的交集.
 * 这样划分能确保给定任意电压, 容易找到最接近的点.
 * */
struct atc260x_regu_volt_band {
	u32 volt_max_uv;
	u32 volt_min_uv;
	u32 volt_step_uv;
	u16 vsel_start;     /* 这个range内起始(最低)的那个电压点的配置序号(寄存器配置值) */
	u16 vsel_end;       /* 结束的序号, 与上面相似. */
};

struct atc260x_regulator_dev;
struct atc260x_regu_hwinfo;
typedef int (*atc260x_regu_volt_lookup_func_t)(struct atc260x_regulator_dev *atc260x_regu,
		const struct atc260x_regu_hwinfo *hwinfo, uint band, uint index);

struct atc260x_regu_hwinfo {
	/* 电压查找函数, 当电压与配置值不成线性关系时使用(通常是些有BUG的硬件). */
	/* 若为NULL, 则使用volt_bands进行(分段)线性查找. */
	/* 即使是用了这个函数, 下面的bands还是要填的, 至少要提供各个band的电压范围. */
	atc260x_regu_volt_lookup_func_t nonlinear_lookup;

	struct atc260x_regu_volt_band volt_bands[2]; /* 目前最麻烦的那个dcdc是分两段的, 其它都是一段全线性的. */
	uint    volt_band_exclusive;    /* 各个band是互斥的, 只能初始化时N选1. 少数LDO会用到. */

	u16     ctrl_reg_addr;
	u16     ctrl_en_bm;
	u16     ctrl_bypass_bm;

	u16     mode_reg_addr;
	u16     mode_bm;
	u16     mode_bv_auto;
	u16     mode_bv_pwm;
	u16     mode_bv_pfm;

	u16     vsel_reg_addr;
	u16     vsel_volt_bm;

	u16     vsel_band_reg_addr;     /* 互斥band的配置寄存器. 少数LDO会用到. */
	u16     vsel_band_bm;

	u16     state_ov_reg_addr;      /* 过压 */
	u16     state_uv_reg_addr;      /* 欠压 */
	u16     state_oc_reg_addr;      /* 过流 */
	u16     state_ov_bm;
	u16     state_uv_bm;
	u16     state_oc_bm;

	u32     stable_time_on;         /* off->on 所需要的稳定时间. (uS) */
	u32     stable_time_chng_volt;  /* 调节电压所需的最长稳定时间. (uS) */

	u32     valid_modes_mask;       /* REGULATOR_MODE_NORMAL ... */
	u32     valid_ops_mask;         /* REGULATOR_CHANGE_VOLTAGE ... */
};

struct atc260x_regulator_dev {
	struct device *dev;
	struct atc260x_dev *atc260x;

	const struct atc260x_regu_hwinfo *regu_hwinfo;
	struct regulator_desc regulator_desc;
	struct regulator_dev *regulator;
	const char *name;
	uint regu_id;
	uint selected_volt_band;
	uint suspend_save_state;    /* state before suspend */
	uint suspend_save_selector; /* selector before suspend */
};


/* bring in the hwinfo structures */
#define __MFD_ATC260X_REGULATOR__NEED_HWINFO_DEFINE__ 1
#include "atc260x-regulator.h"
#undef __MFD_ATC260X_REGULATOR__NEED_HWINFO_DEFINE__



static int _atc260x_regulator_set_volt_band(struct atc260x_regulator_dev *atc260x_regu,
		uint sel_band)
{
	struct atc260x_dev *atc260x = atc260x_regu->atc260x;
	const struct atc260x_regu_hwinfo *hwinfo = atc260x_regu->regu_hwinfo;
	int ret;

	BUG_ON(hwinfo->vsel_band_reg_addr == 0 || hwinfo->vsel_band_bm == 0);
	dev_info(atc260x_regu->dev, "attemp to change volt band to #%u\n", sel_band);

	ret = atc260x_reg_setbits(atc260x,
		hwinfo->vsel_band_reg_addr, hwinfo->vsel_band_bm,
		(sel_band << __ffs(hwinfo->vsel_band_bm)) & hwinfo->vsel_band_bm);
	if (ret) {
		dev_err(atc260x_regu->dev, "%s() failed, reg=0x%x bm=0x%x ret=%d\n",
			__func__, hwinfo->vsel_band_reg_addr, hwinfo->vsel_band_bm, ret);
	}
	return ret;
}

static int _atc260x_regulator_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	const struct atc260x_regu_volt_band *volt_band;
	uint i, result;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;

	/* check hwinfo valid */
	if (hwinfo->ctrl_reg_addr == 0) {
		dev_err(atc260x_regu->dev,
			"You are going to use a regulator (%s) w/o valid hwinfo structure, "
			"this is a BUG in the dts file!\n",
			atc260x_regu->name);
		BUG();
	}

	/* try lookup func first */
	if (hwinfo->nonlinear_lookup != NULL) {
		ret = (hwinfo->nonlinear_lookup)(
			atc260x_regu, hwinfo,
			atc260x_regu->selected_volt_band, selector);
		if (ret < 0) {
			dev_err(atc260x_regu->dev, "%s() call nonlinear_lookup err, ret=%d\n",
				__func__, ret);
			return ret;
		}
		result = ret;
	}
	else {
		/* try linear band. */
		if (hwinfo->volt_band_exclusive) {
			BUG_ON(hwinfo->volt_band_exclusive >= ARRAY_SIZE(hwinfo->volt_bands));
			volt_band = &(hwinfo->volt_bands[atc260x_regu->selected_volt_band]);
		} else {
			volt_band = NULL;
			for (i = 0; i < ARRAY_SIZE(hwinfo->volt_bands); i++) {
				if (hwinfo->volt_bands[i].volt_max_uv == 0) /* not used? */
					break;
				if (selector >= hwinfo->volt_bands[i].vsel_start &&
					selector <= hwinfo->volt_bands[i].vsel_end) {
					volt_band = &(hwinfo->volt_bands[i]);
					break;
				}
			}
		}
		if (volt_band == NULL) {
			dev_err(atc260x_regu->dev, "%s() selector %u not match\n",
				__func__, selector);
			return -EINVAL;
		}

		result = volt_band->volt_min_uv +
				(selector - volt_band->vsel_start) * volt_band->volt_step_uv;
	}

	dev_dbgl(atc260x_regu->dev, "%s() selector %u --> voltage %uuV\n",
		__func__, selector, result);
	return result;
}

static int _atc260x_regulator_get_vchng_stable_time(struct regulator_dev *rdev,
					 unsigned int old_selector, unsigned int new_selector)
{
	struct atc260x_regulator_dev *atc260x_regu = rdev_get_drvdata(rdev);
	int old_volt, new_volt;

	/* 升压时需要等待, 降压时不需要.
	 * DCDC/LDO的selector与输出电压不一定正相关的(有些DCDC/LDO有硬件BUG的缘故),
	 * 所以不能简单地比较selector. */
	old_volt = (atc260x_regu->regulator_desc.ops->list_voltage)(rdev, old_selector);
	if (old_volt < 0)
		return old_volt; /* error code */

	new_volt = (atc260x_regu->regulator_desc.ops->list_voltage)(rdev, new_selector);
	if (new_volt < 0)
		return new_volt; /* error code */

	return (new_volt > old_volt) ?
		atc260x_regu->regu_hwinfo->stable_time_chng_volt : 0;
}

static int _atc260x_regulator_get_hw_status(struct atc260x_regulator_dev *atc260x_regu,
	u8 *p_on, u8 *p_bypass, u8 *p_oc, u8 *p_ov, u8 *p_uv)
{
	const struct atc260x_regu_hwinfo *hwinfo;
	u8  *state_result_list[3];
	uint state_reg_list[3];
	u16  state_reg_bm_list[3];
	uint i, ctrl_reg_val;
	int ret;

	hwinfo = atc260x_regu->regu_hwinfo;

	if (p_on || p_bypass) {
		ret = atc260x_reg_read(atc260x_regu->atc260x, hwinfo->ctrl_reg_addr);
		if (ret < 0) {
			dev_err(atc260x_regu->dev, "%s() read act260x reg %u err, ret=%u\n",
					__func__, hwinfo->ctrl_reg_addr, ret);
			return -EIO;
		}
		ctrl_reg_val = ret;

		if (p_on) {
			if (hwinfo->ctrl_en_bm == 0) {
				*p_on = -1; /* always_on */
			} else if (ctrl_reg_val & hwinfo->ctrl_en_bm) {
				*p_on = 1; /* on */
			} else {
				*p_on = 0;
			}
		}
		if (p_bypass) {
			if (ctrl_reg_val & hwinfo->ctrl_bypass_bm) {
				*p_bypass = 1;
			} else {
				*p_bypass = 0;
			}
		}
	}

	state_result_list[0]     = p_ov;
	state_reg_list[0]        = hwinfo->state_ov_reg_addr;
	state_reg_bm_list[0]     = hwinfo->state_ov_bm;
	state_result_list[1]     = p_uv;
	state_reg_list[1]        = hwinfo->state_uv_reg_addr;
	state_reg_bm_list[1]     = hwinfo->state_uv_bm;
	state_result_list[2]     = p_oc;
	state_reg_list[2]        = hwinfo->state_oc_reg_addr;
	state_reg_bm_list[2]     = hwinfo->state_oc_bm;
	for (i = 0; i < ARRAY_SIZE(state_reg_bm_list); i++) {
		if (state_result_list[i] == NULL)
			continue;
		if (state_reg_bm_list[i] != 0) {
			ret = atc260x_reg_read(atc260x_regu->atc260x, state_reg_list[i]);
			if (ret < 0) {
				dev_err(atc260x_regu->dev, "%s() read act260x reg %u err, ret=%u\n",
						__func__, state_reg_list[i], ret);
				return -EIO;
			}
			*(state_result_list[i]) = ((uint)ret & state_reg_bm_list[i]) ? 1 : 0;
		} else {
			*(state_result_list[i]) = 0;
		}
	}

	return 0;
}

static int _atc260x_regulator_get_actual_status(struct regulator_dev *rdev)
{
	struct atc260x_regulator_dev *atc260x_regu;
	u8 state_on, state_bypass;
	u8 state_oc, state_ov, state_uv;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);

	ret = _atc260x_regulator_get_hw_status(atc260x_regu,
		&state_on, &state_bypass, &state_oc, &state_ov, &state_uv);
	if (ret) {
		dev_err(atc260x_regu->dev, "%s() get HW state err, ret=%u\n",
				__func__,ret);
		return -EIO;
	}
	if (state_oc | state_ov | state_uv) {
		return REGULATOR_STATUS_ERROR;
	}
	if (state_bypass) {
		return REGULATOR_STATUS_BYPASS;
	}
	if (state_on) {
		return REGULATOR_STATUS_ON;
	}
	return REGULATOR_STATUS_OFF;
}

static int _atc260x_regulator_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	uint reg_mask;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;
	reg_mask = hwinfo->vsel_volt_bm;
	if (reg_mask) {
		ret = atc260x_reg_setbits(atc260x_regu->atc260x,
			hwinfo->vsel_reg_addr,
			reg_mask, (selector << __ffs(reg_mask)) & reg_mask);
	} else {
		/* fixed voltage, selector should be 0 */
		ret = (selector == 0) ? 0 : -ENXIO;
	}
	dev_dbgl(atc260x_regu->dev, "set voltage_sel=%u, ret=%d\n", selector, ret);
	return ret;
}

static int _atc260x_regulator_get_voltage_sel(struct regulator_dev *rdev)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	uint reg_mask;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;
	reg_mask = hwinfo->vsel_volt_bm;
	if (reg_mask) {
		ret = atc260x_reg_read(atc260x_regu->atc260x, hwinfo->vsel_reg_addr);
		if (ret >= 0) {
			ret = ((uint)ret & reg_mask) >> __ffs(reg_mask);
		}
	} else {
		/* fixed voltage, selector=0 */
		ret = 0;
	}
	return ret;
}

static int _atc260x_regulator_enable_inner(struct regulator_dev *rdev, uint enable)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	uint reg_mask;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;
	reg_mask = hwinfo->ctrl_en_bm;
	if (reg_mask) {
		ret = atc260x_reg_setbits(atc260x_regu->atc260x,
			hwinfo->ctrl_reg_addr, reg_mask, (enable ? reg_mask : 0));
	} else {
		/* always on. */
		ret = enable ? 0 : -ENXIO;
	}
	dev_dbgl(atc260x_regu->dev, "set enable=%u, ret=%d\n", enable, ret);
	return ret;
}
static int _atc260x_regulator_enable(struct regulator_dev *rdev)
{
	return _atc260x_regulator_enable_inner(rdev, 1);
}
static int _atc260x_regulator_disable(struct regulator_dev *rdev)
{
	return _atc260x_regulator_enable_inner(rdev, 0);
}

static int _atc260x_regulator_is_enabled(struct regulator_dev *rdev)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	uint reg_mask;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;
	reg_mask = hwinfo->ctrl_en_bm;
	if (reg_mask) {
		ret = atc260x_reg_read(atc260x_regu->atc260x, hwinfo->ctrl_reg_addr);
		if (ret >= 0) {
			ret = (((uint)ret & reg_mask) != 0);
		}
	} else {
		/* always on. */
		ret = 1;
	}
	return ret;
}

static int _atc260x_regulator_set_bypass(struct regulator_dev *rdev, bool enable)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	uint reg_mask;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;
	reg_mask = hwinfo->ctrl_bypass_bm;
	if (reg_mask) {
		ret = atc260x_reg_setbits(atc260x_regu->atc260x,
			hwinfo->ctrl_reg_addr, reg_mask, (enable ? reg_mask : 0));
	} else {
		/* no such function. */
		ret = enable ? -ENXIO : 0;
	}
	dev_dbgl(atc260x_regu->dev, "set bypass=%u, ret=%d\n", enable, ret);
	return ret;
}

static int _atc260x_regulator_get_bypass(struct regulator_dev *rdev, bool *enable)
{
	struct atc260x_regulator_dev *atc260x_regu;
	const struct atc260x_regu_hwinfo *hwinfo;
	uint reg_mask;
	int ret;

	atc260x_regu = rdev_get_drvdata(rdev);
	hwinfo = atc260x_regu->regu_hwinfo;
	reg_mask = hwinfo->ctrl_bypass_bm;
	if (reg_mask) {
		ret = atc260x_reg_read(atc260x_regu->atc260x, hwinfo->ctrl_reg_addr);
		if (ret >= 0) {
			*enable = (((uint)ret & reg_mask) != 0);
		}
	} else {
		/* no such function. */
		*enable = 0;
		ret = -ENXIO;
	}
	return ret;
}

static struct regulator_ops atc260x_regulator_linear_ops = {
	/* use ourselfs' function */
	.list_voltage       = regulator_list_voltage_linear,
	/*.enable_time        = NULL, ** no need ** */
	.set_voltage_time_sel = _atc260x_regulator_get_vchng_stable_time,

	.set_mode           = NULL,
	.get_mode           = NULL,
	.get_optimum_mode   = NULL,
	.get_status         = _atc260x_regulator_get_actual_status,

	/* basic ops */
	.set_voltage_sel    = _atc260x_regulator_set_voltage_sel,
	.get_voltage_sel    = _atc260x_regulator_get_voltage_sel,
	.enable             = _atc260x_regulator_enable,
	.disable            = _atc260x_regulator_disable,
	.is_enabled         = _atc260x_regulator_is_enabled,
	.set_bypass         = _atc260x_regulator_set_bypass,
	.get_bypass         = _atc260x_regulator_get_bypass,

	/* 原本是有打算直接使用 regulator_set_voltage_sel_regmap 那一套API的,
	 * 但是那样就相当于绕过atc260x_core直接调regmap框架了, 在suspend最后阶段时
	 * arch的代码可能会调regulator_suspend_prepare, 这时会出问题,
	 * 因为regmap依赖的I2C已经down了.
	 * 最后还是改回使用core的API, core那边会处理suspend的情况. */
};

static struct regulator_ops atc260x_regulator_banded_ops = {
	/* use ourselfs' function */
	.list_voltage       = _atc260x_regulator_list_voltage,
	/*.enable_time        = NULL, ** no need ** */
	.set_voltage_time_sel = _atc260x_regulator_get_vchng_stable_time,

	.set_mode           = NULL,
	.get_mode           = NULL,
	.get_optimum_mode   = NULL,
	.get_status         = _atc260x_regulator_get_actual_status,

	/* basic ops */
	.set_voltage_sel    = _atc260x_regulator_set_voltage_sel,
	.get_voltage_sel    = _atc260x_regulator_get_voltage_sel,
	.enable             = _atc260x_regulator_enable,
	.disable            = _atc260x_regulator_disable,
	.is_enabled         = _atc260x_regulator_is_enabled,
	.set_bypass         = _atc260x_regulator_set_bypass,
	.get_bypass         = _atc260x_regulator_get_bypass,
};

/* return : true / false */
static int _atc260x_chk_constraints_vrang(struct regulation_constraints *cs,
		uint v_min, uint v_max, uint correct)
{
	uint ret = 1;
	if (correct) {
		if (cs->min_uV < v_min) {
			cs->min_uV = v_min;
			ret = 0;
		}
		if (cs->min_uV > v_max) {
			cs->min_uV = v_max;
			ret = 0;
		}
		if (cs->max_uV < v_min) {
			cs->max_uV = v_min;
			ret = 0;
		}
		if (cs->max_uV > v_max) {
			cs->max_uV = v_max;
			ret = 0;
		}
	} else {
		ret = (cs->min_uV < v_min) ? 0 : ret;
		ret = (cs->min_uV > v_max) ? 0 : ret;
		ret = (cs->max_uV < v_min) ? 0 : ret;
		ret = (cs->max_uV > v_max) ? 0 : ret;
	}
	return ret;
}

static int _atc260x_init_regulator_desc(struct atc260x_regulator_dev *atc260x_regu,
		struct regulator_desc *r_desc, struct regulator_config *r_cfg,
		struct regulator_init_data  *init_data)
{
	static const struct atc260x_regu_hwinfo *  sc_atc260x_dcdc_hwinfo_tbl_pool[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = sc_atc2603a_dcdc_hwinfo_tbl,
		[ATC260X_ICTYPE_2603C] = sc_atc2603c_ver_a_dcdc_hwinfo_tbl,
		[ATC260X_ICTYPE_2609A] = sc_atc2609a_dcdc_hwinfo_tbl,
	};
	static  uint sc_atc260x_dcdc_hwinfo_tbl_len[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = ARRAY_SIZE(sc_atc2603a_dcdc_hwinfo_tbl),
		[ATC260X_ICTYPE_2603C] = ARRAY_SIZE(sc_atc2603c_ver_a_dcdc_hwinfo_tbl),
		[ATC260X_ICTYPE_2609A] = ARRAY_SIZE(sc_atc2609a_dcdc_hwinfo_tbl),
	};
	static const struct atc260x_regu_hwinfo * const sc_atc260x_ldo_hwinfo_tbl_pool[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = sc_atc2603a_ldo_hwinfo_tbl,
		[ATC260X_ICTYPE_2603C] = sc_atc2603c_ldo_hwinfo_tbl,
		[ATC260X_ICTYPE_2609A] = sc_atc2609a_ldo_hwinfo_tbl,
	};
	static const uint sc_atc260x_ldo_hwinfo_tbl_len[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = ARRAY_SIZE(sc_atc2603a_ldo_hwinfo_tbl),
		[ATC260X_ICTYPE_2603C] = ARRAY_SIZE(sc_atc2603c_ldo_hwinfo_tbl),
		[ATC260X_ICTYPE_2609A] = ARRAY_SIZE(sc_atc2609a_ldo_hwinfo_tbl),
	};
	struct atc260x_dev *atc260x;
	const struct atc260x_regu_hwinfo *hwinfo_tbl, *hwinfo;
	const struct atc260x_regu_volt_band *linear_volt_band;
	uint hwinfo_tbl_len, selected_volt_band, total_n_volt;
	uint ic_type;
	int ret;

	atc260x = atc260x_regu->atc260x;
	ic_type = atc260x_get_ic_type(atc260x);
	if((ic_type == ATC260X_ICTYPE_2603C) && (atc260x_get_ic_ver(atc260x) > 0) &&(atc260x_regu->regu_id == 2)) {	
		init_data->constraints.min_uV = 1000000;
		sc_atc260x_dcdc_hwinfo_tbl_pool[ATC260X_ICTYPE_2603C] = sc_atc2603c_ver_b_dcdc_hwinfo_tbl;
		sc_atc260x_dcdc_hwinfo_tbl_len[ATC260X_ICTYPE_2603C] = ARRAY_SIZE(sc_atc2603c_ver_b_dcdc_hwinfo_tbl);
	}
	BUG_ON(ic_type >= ATC260X_ICTYPE_CNT);

	if (strstr(atc260x_regu->name, "dcdc") != NULL) {
		hwinfo_tbl = sc_atc260x_dcdc_hwinfo_tbl_pool[ic_type];
		hwinfo_tbl_len = sc_atc260x_dcdc_hwinfo_tbl_len[ic_type];
	}
	else if (strstr(atc260x_regu->name, "ldo") != NULL) {
		hwinfo_tbl = sc_atc260x_ldo_hwinfo_tbl_pool[ic_type];
		hwinfo_tbl_len = sc_atc260x_ldo_hwinfo_tbl_len[ic_type];
	}
	else {
		dev_err(atc260x_regu->dev, "%s() unknown type of regulator dev %s\n",
			__func__, atc260x_regu->name);
		return -ENXIO;
	}
	if (atc260x_regu->regu_id >= hwinfo_tbl_len) {
		dev_err(atc260x_regu->dev, "%s() unknown regulator (%s) id %d\n",
			__func__, atc260x_regu->name, atc260x_regu->regu_id);
		return -ENXIO;
	}
	hwinfo = &(hwinfo_tbl[atc260x_regu->regu_id]);

	atc260x_regu->regu_hwinfo = hwinfo;

	/* handle voltage band */
	if (hwinfo->volt_bands[1].volt_max_uv == 0) {
		/* only 1 band */
		/* apply a smaller range */
		_atc260x_chk_constraints_vrang(&init_data->constraints,
			hwinfo->volt_bands[0].volt_min_uv,
			hwinfo->volt_bands[0].volt_max_uv, true);
		linear_volt_band = &(hwinfo->volt_bands[0]);
		selected_volt_band = 0;
		total_n_volt = hwinfo->volt_bands[0].vsel_end -
				hwinfo->volt_bands[0].vsel_start + 1U;
	} else {
		if (hwinfo->volt_band_exclusive == 0) {
			/* 电压配置分了两段(每段有不同的间距), 非互斥, 寄存器不用切band操作. */
			_atc260x_chk_constraints_vrang(&init_data->constraints,
				hwinfo->volt_bands[0].volt_min_uv,
				hwinfo->volt_bands[1].volt_max_uv, true);
			linear_volt_band = NULL;
			selected_volt_band = 0;
			total_n_volt = hwinfo->volt_bands[1].vsel_end -
					hwinfo->volt_bands[0].vsel_start + 1U;
		} else {
			/* 电压配置分了两段, 是互斥的, 只能2选1. */
			if (_atc260x_chk_constraints_vrang(&init_data->constraints,
				hwinfo->volt_bands[0].volt_min_uv,
				hwinfo->volt_bands[0].volt_max_uv, false))
			{
				linear_volt_band = &(hwinfo->volt_bands[0]);
				selected_volt_band = 0;
				total_n_volt = hwinfo->volt_bands[0].vsel_end -
						hwinfo->volt_bands[0].vsel_start + 1U;
			}
			else if (_atc260x_chk_constraints_vrang(&init_data->constraints,
				hwinfo->volt_bands[1].volt_min_uv,
				hwinfo->volt_bands[1].volt_max_uv, false))
			{
				linear_volt_band = &(hwinfo->volt_bands[1]);
				selected_volt_band = 1;
				total_n_volt = hwinfo->volt_bands[1].vsel_end -
						hwinfo->volt_bands[1].vsel_start + 1U;
			}
			else {
				dev_err(atc260x_regu->dev, "%s() unmatched voltage rang [%u,%u]\n",
					__func__,
					init_data->constraints.min_uV, init_data->constraints.max_uV);
				return -ENXIO;
			}
			ret = _atc260x_regulator_set_volt_band(atc260x_regu, selected_volt_band);
			if (ret) {
				return ret;
			}
		}
	}

	atc260x_regu->selected_volt_band = selected_volt_band;

	init_data->constraints.valid_modes_mask |= hwinfo->valid_modes_mask;
	init_data->constraints.valid_ops_mask |= hwinfo->valid_ops_mask;
	if (hwinfo->ctrl_en_bm == 0) {
		init_data->constraints.always_on = 1;
		init_data->constraints.valid_ops_mask &= ~REGULATOR_CHANGE_STATUS;
	}
	if (init_data->constraints.min_uV == init_data->constraints.max_uV) {
		init_data->constraints.apply_uV = 1;
	}

	r_cfg->dev = atc260x_regu->dev;
	r_cfg->init_data = init_data;
	r_cfg->driver_data = atc260x_regu;
	r_cfg->of_node = atc260x_regu->dev->of_node;
	/* r_cfg->regmap = atc260x_get_internal_regmap(atc260x); not need currently.*/

	r_desc->name = init_data->constraints.name;
	r_desc->id = atc260x_regu->regu_id;
	r_desc->type = REGULATOR_VOLTAGE;
	r_desc->n_voltages = total_n_volt;
	r_desc->ops = &atc260x_regulator_banded_ops;
	r_desc->owner = THIS_MODULE;

	if (linear_volt_band != NULL && hwinfo->nonlinear_lookup == NULL) {
		r_desc->min_uV = linear_volt_band->volt_min_uv;
		r_desc->uV_step = linear_volt_band->volt_step_uv;
		r_desc->linear_min_sel = linear_volt_band->vsel_start;
		r_desc->ops = &atc260x_regulator_linear_ops;
	}

	r_desc->enable_time = hwinfo->stable_time_on;

#if 0
	/* for regmap, not need currently */
	r_desc->vsel_reg           = hwinfo->vsel_reg_addr;
	r_desc->vsel_mask          = hwinfo->vsel_volt_bm;
	r_desc->apply_reg          = 0;
	r_desc->apply_bit          = 0;
	r_desc->enable_reg         = hwinfo->ctrl_reg_addr;
	r_desc->enable_mask        = hwinfo->ctrl_en_bm;
	r_desc->enable_is_inverted = (hwinfo->ctrl_en_bm == 0) ? 1:0; /* make it seems enabled, if no such ctrl bit */
	r_desc->bypass_reg         = hwinfo->ctrl_reg_addr;
	r_desc->bypass_mask        = hwinfo->ctrl_bypass_bm;
#endif
	return 0;
}

static void _atc260x_dump_regulator_info(struct atc260x_regulator_dev *atc260x_regu,
		struct regulator_init_data  *init_data)
{
#if CONFIG_ATC260x_REGU_FULL_DBG
	uint i;
	const struct atc260x_regu_hwinfo *hwinfo;

	dev_info(atc260x_regu->dev, "dump regulator ----------------------------\n");
	pr_info("  id: %u  name: %s\n", atc260x_regu->regu_id, atc260x_regu->name);
	pr_info("  selected_volt_band: %u\n", atc260x_regu->selected_volt_band);

	hwinfo = atc260x_regu->regu_hwinfo;
	pr_info("  hwinfo :\n");
	pr_info("    nonlinear_lookup : %lx\n", (ulong)(hwinfo->nonlinear_lookup));
	for (i = 0 ;i < ARRAY_SIZE(hwinfo->volt_bands); i++) {
		pr_info("    volt_bands_%u     : min %uuV  max %uuV  step %uuV  vsel_start %u  vsel_end %u\n",
			i,
			hwinfo->volt_bands[i].volt_min_uv, hwinfo->volt_bands[i].volt_max_uv,
			hwinfo->volt_bands[i].volt_step_uv,
			hwinfo->volt_bands[i].vsel_start, hwinfo->volt_bands[i].vsel_end);
	}
	pr_info("    volt_band_exclusive : %u\n", hwinfo->volt_band_exclusive);
	pr_info("    ctrl_reg_addr    : 0x%x\n", hwinfo->ctrl_reg_addr);
	pr_info("    ctrl_en_bm       : 0x%x\n", hwinfo->ctrl_en_bm);
	pr_info("    ctrl_bypass_bm   : 0x%x\n", hwinfo->ctrl_bypass_bm);

	pr_info("    mode_reg_addr    : 0x%x\n", hwinfo->mode_reg_addr);
	pr_info("    mode_bm          : 0x%x\n", hwinfo->mode_bm);
	pr_info("    mode_bv_auto     : 0x%x\n", hwinfo->mode_bv_auto);
	pr_info("    mode_bv_pwm      : 0x%x\n", hwinfo->mode_bv_pwm);
	pr_info("    mode_bv_pfm      : 0x%x\n", hwinfo->mode_bv_pfm);

	pr_info("    vsel_reg_addr    : 0x%x\n", hwinfo->vsel_reg_addr);
	pr_info("    vsel_volt_bm     : 0x%x\n", hwinfo->vsel_volt_bm);

	pr_info("    vsel_band_reg_addr : 0x%x\n", hwinfo->vsel_band_reg_addr);
	pr_info("    vsel_band_bm     : 0x%x\n", hwinfo->vsel_band_bm);

	pr_info("    state_ov_reg_addr  : 0x%x\n", hwinfo->state_ov_reg_addr);
	pr_info("    state_uv_reg_addr  : 0x%x\n", hwinfo->state_uv_reg_addr);
	pr_info("    state_oc_reg_addr  : 0x%x\n", hwinfo->state_oc_reg_addr);
	pr_info("    state_ov_bm        : 0x%x\n", hwinfo->state_ov_bm);
	pr_info("    state_uv_bm        : 0x%x\n", hwinfo->state_uv_bm);
	pr_info("    state_oc_bm        : 0x%x\n", hwinfo->state_oc_bm);

	pr_info("    stable_time_on        : %uuS\n", hwinfo->stable_time_on);
	pr_info("    stable_time_chng_volt : %uuS\n", hwinfo->stable_time_chng_volt);

	pr_info("    valid_modes_mask : 0x%x\n", hwinfo->valid_modes_mask);
	pr_info("    valid_ops_mask   : 0x%x\n", hwinfo->valid_ops_mask);

	pr_info("  constraints :\n");
	pr_info("    min_uV           : 0x%x\n", init_data->constraints.min_uV);
	pr_info("    max_uV           : 0x%x\n", init_data->constraints.max_uV);
	pr_info("    initial_mode     : 0x%x\n", init_data->constraints.initial_mode);
	pr_info("    ramp_delay       : %u\n", init_data->constraints.ramp_delay);
	pr_info("    always_on        : %u\n", init_data->constraints.always_on);
	pr_info("    boot_on          : %u\n", init_data->constraints.boot_on);
	pr_info("    apply_uV         : %u\n", init_data->constraints.apply_uV);
	pr_info("    valid_modes_mask : 0x%x\n", init_data->constraints.valid_modes_mask);
	pr_info("    valid_ops_mask   : 0x%x\n", init_data->constraints.valid_ops_mask);
#endif
}

static int atc260x_regulator_probe(struct platform_device *pdev)
{
	struct regulator_config config = { };
	struct regulator_init_data  *init_data;
	struct atc260x_dev *atc260x;
	struct atc260x_regulator_dev *atc260x_regu;
	uint regu_id;
	int ret;
	u32 val;

	atc260x = atc260x_get_parent_dev(&pdev->dev);
	regu_id = pdev->id;

	dev_info(&pdev->dev, "Probing %s, id=%u\n", pdev->name, regu_id);

	init_data = of_get_regulator_init_data(&pdev->dev, pdev->dev.of_node);
	if (!init_data)
		return -ENOMEM;
	/* get other stuff from DTS */
	if (of_find_property(pdev->dev.of_node, "regulator-suspend-off", NULL)) {
		init_data->constraints.state_standby.disabled = 1;
		init_data->constraints.state_mem.disabled = 1;
		init_data->constraints.state_disk.disabled = 1;
	}

	if (!of_property_read_u32(pdev->dev.of_node, "regulator-state-mode", &val)) {
		init_data->constraints.state_standby.mode  = val;
		init_data->constraints.state_mem.mode  = val;
		init_data->constraints.state_disk.mode  = val;
	}

	atc260x_regu = devm_kzalloc(
			&pdev->dev, sizeof(struct atc260x_regulator_dev), GFP_KERNEL);
	if (atc260x_regu == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	}
	atc260x_regu->dev = &pdev->dev;
	atc260x_regu->atc260x = atc260x;
	atc260x_regu->regu_id = regu_id;
	atc260x_regu->name = init_data->constraints.name;

	/* init desc */
	ret = _atc260x_init_regulator_desc(atc260x_regu,
			&(atc260x_regu->regulator_desc), &config, init_data);
	if (ret) {
		dev_err(&pdev->dev, "%s() failed to init regulator_desc, ret=%d\n",
			__func__, ret);
		return ret;
	}
	_atc260x_dump_regulator_info(atc260x_regu, init_data);

	atc260x_regu->regulator = regulator_register(
			&(atc260x_regu->regulator_desc), &config);
	if (IS_ERR(atc260x_regu->regulator)) {
		ret = PTR_ERR(atc260x_regu->regulator);
		dev_err(&pdev->dev, "%s() Failed to register regulator, ret=%d\n",
			__func__, ret);
		return ret;
	}

	platform_set_drvdata(pdev, atc260x_regu);

#if CONFIG_ATC260x_REGU_FULL_DBG && defined(CONFIG_REGULATOR_VIRTUAL_CONSUMER)
	/* for debug */
	platform_device_register_resndata(NULL, "reg-virt-consumer",
		(strstr(atc260x_regu->name, "dcdc") ? 0 : 10) + atc260x_regu->regu_id,
		NULL, 0,
		atc260x_regu->name, strlen(atc260x_regu->name) + 1);
#endif
	return 0;
}

static int atc260x_regulator_remove(struct platform_device *pdev)
{
	struct atc260x_regulator_dev *atc260x_regu = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
	regulator_unregister(atc260x_regu->regulator);
	return 0;
}

static int atc260x_regulator_print_suspend_state(
		struct atc260x_regulator_dev *atc260x_regu, const char *prompt)
{
	u8 state_on, state_bypass;
	u8 state_oc, state_ov, state_uv;
	int ret;

	ret = _atc260x_regulator_get_hw_status(atc260x_regu,
		&state_on, &state_bypass, &state_oc, &state_ov, &state_uv);
	if (ret) {
		dev_err(atc260x_regu->dev, "state %s : <sw failed, ret=%d>\n",
			prompt, ret);
		return ret;
	}
	if (((state_on == 1) && (atc260x_regu->regulator->constraints->always_on == 0)) ||
		(state_bypass|state_oc|state_ov|state_uv)) {
		/* 有异常状态时提高打印等级. */
		dev_warn(atc260x_regu->dev, "state %s : on=%u bypass=%u oc=%u ov=%u uv=%u\n",
			prompt,
			state_on, state_bypass, state_oc, state_ov, state_uv);
	} else {
		dev_info(atc260x_regu->dev, "state %s : on=%u bypass=%u oc=%u ov=%u uv=%u\n",
			prompt,
			state_on, state_bypass, state_oc, state_ov, state_uv);
	}
	return 0;
}

static int atc260x_regulator_suspend_late(struct device *dev)
{
	struct atc260x_regulator_dev *atc260x_regu = dev_get_drvdata(dev);
	int ret;

	ret = _atc260x_regulator_get_voltage_sel(atc260x_regu->regulator);
	if (ret < 0) {
		dev_err(dev, "%s() failed to save selector, ret=%d\n", __func__, ret);
		return ret;
	}
	atc260x_regu->suspend_save_selector = ret;
	dev_dbgl(dev, "save selector: %u\n", ret);

	atc260x_regu->suspend_save_state = 0;
	if (atc260x_regu->regulator->constraints->state_mem.disabled) {
		if (_atc260x_regulator_is_enabled(atc260x_regu->regulator)) {
			atc260x_regu->suspend_save_state = 1;
			ret = _atc260x_regulator_disable(atc260x_regu->regulator);
			if (ret) {
				dev_err(dev, "%s() failed to disable, ret=%d\n", __func__, ret);
				/* do not return error, keep going. */
			}
		}
	}

	/* 打印状态, 方便休眠漏电的调试. */
	return atc260x_regulator_print_suspend_state(atc260x_regu, "@suspend_late");
}

static int atc260x_regulator_resume_early(struct device *dev)
{
	struct atc260x_regulator_dev *atc260x_regu = dev_get_drvdata(dev);
	int ret;

	/* 为加快resume, 这里就不打印regulator状态了, 打印也无甚意义. */

	if ((atc260x_regu->regulator->constraints->state_mem.mode & REGULATOR_MEM_STATE_MODE_NO_RESTORE) == 0) {
		/*对于cpu_vdd此类的regulator，由于启动代码中会设置电压且cpu跑在高频
		，因此除非由cpufreq调整频率电压，其他模块不应调整电压*/
		ret = _atc260x_regulator_set_voltage_sel(
			atc260x_regu->regulator, atc260x_regu->suspend_save_selector);
		if (ret) {
			dev_err(dev, "%s() failed to restore selector, ret=%d\n", __func__, ret);
			return ret;
		}
		dev_dbgl(dev, "restore selector: %u\n", atc260x_regu->suspend_save_selector);
	}

	if (atc260x_regu->regulator->constraints->state_mem.disabled) {
		if (atc260x_regu->suspend_save_state != 0) {
			ret = _atc260x_regulator_enable(atc260x_regu->regulator);
			if (ret) {
				dev_err(dev, "%s() failed to enable, ret=%d\n", __func__, ret);
				return ret;
			}
		}
	}
	return 0;
}

static void atc260x_regulator_shutdown(struct platform_device *pdev)
{
	atc260x_regulator_print_suspend_state(
		platform_get_drvdata(pdev), "before shutdown");
}

static const struct dev_pm_ops s_atc260x_regulator_pm_ops = {
	.suspend_late  = atc260x_regulator_suspend_late,
	.resume_early  = atc260x_regulator_resume_early,
	.freeze_late   = atc260x_regulator_suspend_late,
	.thaw_early    = atc260x_regulator_resume_early,
	.poweroff_late = atc260x_regulator_suspend_late,
	.restore_early = atc260x_regulator_resume_early,
};

static const struct of_device_id atc260x_regulator_match[] = {
	/* only for voltage ragulators. (DC-DC & LDO) */
	{ .compatible = "actions,atc2603a-dcdc", },
	{ .compatible = "actions,atc2603a-ldo", },
	{ .compatible = "actions,atc2603c-dcdc", },
	{ .compatible = "actions,atc2603c-ldo", },
	{ .compatible = "actions,atc2609a-dcdc", },
	{ .compatible = "actions,atc2609a-ldo", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_regulator_match);

static struct platform_driver atc260x_regulator_driver = {
	.driver     = {
		.name   = "atc260x-regulator",
		.owner  = THIS_MODULE,
		.pm     = &s_atc260x_regulator_pm_ops,
		.of_match_table = of_match_ptr(atc260x_regulator_match),
	},
	.probe = atc260x_regulator_probe,
	.remove = atc260x_regulator_remove,
	.shutdown = atc260x_regulator_shutdown,
};

static int __init atc260x_regulator_init(void)
{
	int ret;
	ret = platform_driver_register(&atc260x_regulator_driver);
	if (ret != 0) {
		pr_err("%s(): failed to register ATC260X regulator driver, ret=%d\n",
			__func__, ret);
	}
	return ret;
}
subsys_initcall(atc260x_regulator_init);

static void __exit atc260x_regulator_exit(void)
{
	platform_driver_unregister(&atc260x_regulator_driver);
}
module_exit(atc260x_regulator_exit);

/* Module information */
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("ATC260X regulator driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-regulator");
