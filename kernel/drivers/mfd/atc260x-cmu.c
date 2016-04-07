#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/atc260x/atc260x.h>

#include "atc260x-core.h"



struct atc260x_cmu_reg_cfg {
	u16         clk_ctl_reg;
	u16         rst_ctl_reg;
	u8          rst_bit_tbl[ATC260X_CMU_MODULE_NUM]; /* index by ATC260X_CMU_MODULE_xxx */
	u8          clk_bit_tbl[ATC260X_CMU_MODULE_NUM]; /* index by ATC260X_CMU_MODULE_xxx */
};

static const struct atc260x_cmu_reg_cfg sc_atc2603a_cmu_reg_cfg = {
	.clk_ctl_reg = ATC2603A_CMU_DEVRST,
	.rst_ctl_reg = ATC2603A_CMU_DEVRST,
				 /*   TP,  MFP, INTS, ETHPHY, AUDIO,  PWSI  */
	.rst_bit_tbl = {   0,    1,    2,      3,     4,  0xff },
	.clk_bit_tbl = {   8, 0xff, 0xff,      9,    10,  0xff },
};
static const struct atc260x_cmu_reg_cfg sc_atc2603c_cmu_reg_cfg = {
	.clk_ctl_reg = ATC2603C_CMU_DEVRST,
	.rst_ctl_reg = ATC2603C_CMU_DEVRST,
				 /*   TP,  MFP, INTS, ETHPHY, AUDIO,  PWSI  */
	.rst_bit_tbl = {0xff,    1,    2,   0xff,     4,  0xff },
	.clk_bit_tbl = {0xff, 0xff, 0xff,   0xff,    10,  0xff },
};
static const struct atc260x_cmu_reg_cfg sc_atc2609a_cmu_reg_cfg = {
	.clk_ctl_reg = ATC2609A_CMU_DEVRST,
	.rst_ctl_reg = ATC2609A_CMU_DEVRST,
				 /*   TP,  MFP, INTS, ETHPHY, AUDIO,  PWSI  */
	.rst_bit_tbl = {0xff,    1,    2,   0xff,     0,     3 },
	.clk_bit_tbl = {0xff, 0xff, 0xff,   0xff,     8,  0xff },
};
static const struct atc260x_cmu_reg_cfg * const sc_atc260x_cmu_reg_cfg_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = &sc_atc2603a_cmu_reg_cfg,
	[ATC260X_ICTYPE_2603C] = &sc_atc2603c_cmu_reg_cfg,
	[ATC260X_ICTYPE_2609A] = &sc_atc2609a_cmu_reg_cfg,
};

int atc260x_cmu_reset(struct atc260x_dev *atc260x, uint cmu_module)
{
	struct atc260x_cmu_reg_cfg const *regcfg;
	uint reg_addr, reg_val, rst_bit;
	int ret;

	if (cmu_module >= ATC260X_CMU_MODULE_NUM) {
		dev_err(atc260x->dev, "Invalid ATC260X cmu module %u\n", cmu_module);
		return -EINVAL;
	}
	ATC260X_ASSERT_VALID_DEV(atc260x);

	BUG_ON(atc260x->ic_type >= ATC260X_ICTYPE_CNT);
	regcfg = sc_atc260x_cmu_reg_cfg_tbl[atc260x->ic_type];
	rst_bit = regcfg->rst_bit_tbl[cmu_module];
	if (rst_bit >= 32) {
		dev_err(atc260x->dev, "No reset func for cmu module %u\n", cmu_module);
		return -ENXIO; /* no such function */
	}
	reg_addr = regcfg->rst_ctl_reg;

	ret = atc260x_reg_read(atc260x, reg_addr);
	if (ret < 0) {
		goto label_err;
	}
	reg_val = ret;
	ret = atc260x_reg_write(atc260x, reg_addr, reg_val & ~(1U << rst_bit));
	if (ret < 0) {
		goto label_err;
	}
	udelay(50);
	ret = atc260x_reg_write(atc260x, reg_addr, reg_val);
	if (ret < 0) {
		goto label_err;
	}
	udelay(50);

	return 0;

	label_err:
	dev_err(atc260x->dev, "%s() faile to read/write cmu reset reg 0x%x\n",
		__func__, reg_addr);
	return -EIO;
}
EXPORT_SYMBOL_GPL(atc260x_cmu_reset);

int atc260x_cmu_clk_ctrl(struct atc260x_dev *atc260x, uint cmu_module, uint clk_en)
{
	struct atc260x_cmu_reg_cfg const *regcfg;
	uint reg_addr, clk_bit;
	int ret;

	if (cmu_module >= ATC260X_CMU_MODULE_NUM) {
		dev_err(atc260x->dev, "Invalid ATC260X cmu module %u\n", cmu_module);
		return -EINVAL;
	}
	ATC260X_ASSERT_VALID_DEV(atc260x);

	clk_en = !!clk_en;

	regcfg = sc_atc260x_cmu_reg_cfg_tbl[atc260x->ic_type];
	clk_bit = regcfg->clk_bit_tbl[cmu_module];
	if (clk_bit >= 32) {
		dev_err(atc260x->dev, "No clk_ctl func for cmu module %u\n", cmu_module);
		return -ENXIO; /* no such function */
	}
	reg_addr = regcfg->clk_ctl_reg;

	ret = atc260x_reg_setbits(atc260x, reg_addr, (1U << clk_bit), (clk_en << clk_bit));
	if (ret < 0) {
		dev_err(atc260x->dev, "%s() faile to read/write cmu clock reg 0x%x\n",
			__func__, reg_addr);
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(atc260x_cmu_clk_ctrl);


