#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/suspend.h>
#include <linux/mfd/core.h>
#include <linux/mfd/atc260x/atc260x.h>

#include "atc260x-core.h"

static const u16 sc_suspend_save_regs_atx2603a[] = {
	ATC2603A_PMU_SYS_CTL1,    /* u-boot may change. */
	ATC2603A_CMU_DEVRST,
	/*ATC2603A_INTS_PD, */
	ATC2603A_INTS_MSK,        /* regmap_irq require this */
	ATC2603A_MFP_CTL0,
	ATC2603A_MFP_CTL1,
	ATC2603A_PAD_DRV0,
	ATC2603A_PAD_DRV1,
	ATC2603A_PAD_EN,
	ATC2603A_PMU_AUXADC_CTL1, /* CTL1 goes first. */
	ATC2603A_PMU_AUXADC_CTL0,
	/*ATC2603A_TP_CTL0, */
	/*ATC2603A_TP_CTL1, */
	0xffff,  /* end */
};
static const u16 sc_suspend_save_regs_atx2603c[] = {
	ATC2603C_PMU_SYS_CTL1,    /* u-boot may change. */
	ATC2603C_PMU_MUX_CTL0,    /* MFP for SGPIOs */
	ATC2603C_CMU_DEVRST,
	ATC2603C_INTS_MSK,        /* regmap_irq require this */
	ATC2603C_MFP_CTL,
	ATC2603C_PAD_VSEL,
	ATC2603C_PAD_DRV,
	ATC2603C_PAD_EN,
	ATC2603C_PMU_AUXADC_CTL1, /* CTL1 goes first. */
	ATC2603C_PMU_AUXADC_CTL0,
	0xffff,  /* end */
};
static const u16 sc_suspend_save_regs_atx2609a[] = {
	ATC2609A_PMU_SYS_CTL1,    /* u-boot may change. */
	ATC2609A_CMU_DEVRST,
	ATC2609A_INTS_MSK,        /* regmap_irq require this */
	ATC2609A_MFP_CTL,
	ATC2609A_PAD_VSEL,
	ATC2609A_PAD_DRV,
	ATC2609A_PAD_EN,
	ATC2609A_PMU_AUXADC_CTL1, /* CTL1 goes first. */
	ATC2609A_PMU_AUXADC_CTL0,
	0xffff,  /* end */
};
static const u16 * const sc_suspend_save_regs_tbl[ATC260X_ICTYPE_CNT] = {
	sc_suspend_save_regs_atx2603a,
	sc_suspend_save_regs_atx2603c,
	sc_suspend_save_regs_atx2609a,
};

static int _atc260x_save_critical_regs(struct atc260x_dev *atc260x)
{
	uint i, reg;
	int ret;
	u16 const *p;
	p = sc_suspend_save_regs_tbl[atc260x->ic_type];
	for (i = 0; i < ATC260X_MAX_SAVE_REGS; i++,p++) {
		reg = *p;
		if (reg == 0xffffU)
			break;

		ret = atc260x_reg_read(atc260x, reg);
		if (ret < 0)
			return ret;
		atc260x->reg_save_buf[i] = ret;

		dev_dbg(atc260x->dev, "%s() save reg 0x%x value 0x%x\n",
			__func__, reg, ret);
	}
	return 0;
}

static int _atc260x_restore_critical_regs(struct atc260x_dev *atc260x)
{
	uint i, reg, value;
	int ret;
	u16 const *p;
	p = sc_suspend_save_regs_tbl[atc260x->ic_type];
	for (i = 0; i < ATC260X_MAX_SAVE_REGS; i++,p++) {
		reg = *p;
		if (reg == 0xffffU)
			break;

		value = atc260x->reg_save_buf[i];
		ret = atc260x_reg_write(atc260x, reg, value);
		if (ret) {
			return ret;
		}
		dev_dbg(atc260x->dev, "%s() restore reg 0x%x value 0x%x\n",
			__func__, reg, value);
#if defined(DEBUG)
		ret = atc260x_reg_read(atc260x, reg); /* read back check */
		if (ret < 0) {
			return ret;
		}
		if (ret != value) {
			/* 读回值不对有可能是正常的, 因为会有pending位和只读状态位. print下即可. */
			dev_warn(atc260x->dev, "%s() readback cmp diff, "
				"reg=0x%x org=0x%x readback=0x%x\n",
				__func__, reg, value, (uint)ret);
		}
#endif
	}
	return 0;
}

/* Get atc260x parent device structure.  For sub-device used. */
struct atc260x_dev *atc260x_get_parent_dev(struct device *sub_dev)
{
	struct atc260x_dev *atc260x;
	atc260x = (struct atc260x_dev*)(dev_get_drvdata(sub_dev->parent));
	if (ATC260X_CHK_VALID_DEV(atc260x)) {
		return atc260x;
	}
	dev_err(sub_dev, "%s() not sub device of atc260x\n", __func__);
	dump_stack();
	return NULL;
}
EXPORT_SYMBOL_GPL(atc260x_get_parent_dev);

void atc260x_get_bus_info(struct atc260x_dev *atc260x, uint *bus_num, uint *bus_addr)
{
	*bus_num = atc260x->bus_num;
	*bus_addr = atc260x->bus_addr;
}
EXPORT_SYMBOL_GPL(atc260x_get_bus_info);

uint atc260x_get_ic_type(struct atc260x_dev *atc260x)
{
	return atc260x->ic_type;
}
EXPORT_SYMBOL_GPL(atc260x_get_ic_type);

uint atc260x_get_ic_ver(struct atc260x_dev *atc260x)
{
	return atc260x->ic_ver;
}
EXPORT_SYMBOL_GPL(atc260x_get_ic_ver);

/*
 * Safe read, modify, write methods
 */
int atc260x_reg_setbits(struct atc260x_dev *atc260x, uint reg,
			u16 mask, u16 val)
{
	unsigned long irq_state_save;
	int ret;
	uint reg_tmp_val, reg_old_val;

	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_dbg(atc260x->dev, "%s() reg=0x%x mask=0x%x val=0x%x\n",
		__func__, reg, mask, val);

	switch (atc260x->reg_access_mode) {
	case ATC260X_ACCESS_MODE_NORMAL:
		ret = regmap_update_bits(atc260x->regmap, reg, mask, val);
		break;
	case ATC260X_ACCESS_MODE_DIRECT:
		spin_lock_irqsave(&atc260x->dacc_spinlock, irq_state_save);
		ret = atc260x->direct_read_reg(atc260x, reg);
		if (ret < 0) {
			spin_unlock_irqrestore(&atc260x->dacc_spinlock, irq_state_save);
			break;
		}
		reg_tmp_val = reg_old_val = ret;
		ret = 0;
		reg_tmp_val &= ~((uint)mask);
		reg_tmp_val |= (uint)val & mask;
		if (reg_tmp_val != reg_old_val) {
			ret = atc260x->direct_write_reg(atc260x, reg, reg_tmp_val);
		}
		spin_unlock_irqrestore(&atc260x->dacc_spinlock, irq_state_save);
		break;
	default:
		dev_err(atc260x->dev, "%s() unknown reg access mode %u. "
			"it is likely that somebody still calling this API "
			"while we are in suspended state.\n",
			__func__, atc260x->reg_access_mode);
		/* Keep this dump and we can see whom call us. */
		dump_stack();
		ret = -EIO;
	}

	if (ret) {
		dev_err(atc260x->dev, "reg 0x%x set bits failed\n", reg);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(atc260x_reg_setbits);

int atc260x_reg_read(struct atc260x_dev *atc260x, uint reg)
{
	unsigned long irq_state_save;
	int ret;
	uint data;

	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_dbg(atc260x->dev, "%s() reg=0x%x\n", __func__, reg);

	data = -1;
	switch (atc260x->reg_access_mode) {
	case ATC260X_ACCESS_MODE_NORMAL:
		ret = regmap_read(atc260x->regmap, reg, &data);
		break;
	case ATC260X_ACCESS_MODE_DIRECT:
		spin_lock_irqsave(&atc260x->dacc_spinlock, irq_state_save);
		ret = atc260x->direct_read_reg(atc260x, reg);
		if (ret >= 0) {
			data = ret;
		}
		spin_unlock_irqrestore(&atc260x->dacc_spinlock, irq_state_save);
		ret = 0;
		break;
	default:
		dev_err(atc260x->dev, "%s() unknown reg access mode %u. "
			"it is likely that somebody still calling this API "
			"while we are in suspended state.\n",
			__func__, atc260x->reg_access_mode);
		/* Keep this dump and we can see whom call us. */
		dump_stack();
		ret = -EIO;
	}

	if (ret) {
		dev_err(atc260x->dev, "read from reg 0x%x failed, ret=%d\n", reg, ret);
	}
	return data;
}
EXPORT_SYMBOL_GPL(atc260x_reg_read);

int atc260x_reg_write(struct atc260x_dev *atc260x, uint reg, u16 val)
{
	unsigned long irq_state_save;
	int ret;

	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_dbg(atc260x->dev, "%s() reg=0x%x val=0x%x\n", __func__, reg, val);

	switch (atc260x->reg_access_mode) {
	case ATC260X_ACCESS_MODE_NORMAL:
		ret = regmap_write(atc260x->regmap, reg, val);
		break;
	case ATC260X_ACCESS_MODE_DIRECT:
		spin_lock_irqsave(&atc260x->dacc_spinlock, irq_state_save);
		ret = atc260x->direct_write_reg(atc260x, reg, val);
		spin_unlock_irqrestore(&atc260x->dacc_spinlock, irq_state_save);
		break;
	default:
		dev_err(atc260x->dev, "%s() unknown reg access mode %u. "
			"it is likely that somebody still calling this API "
			"while we are in suspended state.\n",
			__func__, atc260x->reg_access_mode);
		/* Keep this dump and we can see whom call us. */
		dump_stack();
		ret = -EIO;
	}

	if (ret) {
		dev_err(atc260x->dev, "write to reg 0x%x failed\n", reg);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(atc260x_reg_write);

void atc260x_exit_reg_direct_access(struct atc260x_dev *atc260x)
{
	unsigned long irq_state_save;
	uint mode;

	spin_lock_irqsave(&atc260x->dacc_spinlock, irq_state_save);
	(atc260x->direct_acc_exit)(atc260x); /* call the exit callback. */
	mode = atc260x->reg_access_mode = ATC260X_ACCESS_MODE_NORMAL;
	spin_unlock_irqrestore(&atc260x->dacc_spinlock, irq_state_save);

	dev_info(atc260x->dev, "reg access mode set to %u\n", mode);
}

void atc260x_set_reg_direct_access(struct atc260x_dev *atc260x, bool enable)
{
	unsigned long irq_state_save;
	uint mode;

	ATC260X_ASSERT_VALID_DEV(atc260x);

	spin_lock_irqsave(&atc260x->dacc_spinlock, irq_state_save);
	mode = atc260x->reg_access_mode;
	if (enable) {
		if (mode != ATC260X_ACCESS_MODE_DIRECT) {
			(atc260x->direct_acc_init)(atc260x); /* call the init callback. */
			mode = ATC260X_ACCESS_MODE_DIRECT;
		}
	} else {
		if (mode == ATC260X_ACCESS_MODE_DIRECT) {
			(atc260x->direct_acc_exit)(atc260x); /* call the exit callback. */
			mode = ATC260X_ACCESS_MODE_NONE;
		}
	}
	atc260x->reg_access_mode = mode;
	spin_unlock_irqrestore(&atc260x->dacc_spinlock, irq_state_save);

	dev_info(atc260x->dev, "reg access mode set to %u\n", mode);
}
EXPORT_SYMBOL_GPL(atc260x_set_reg_direct_access);


static int atc260x_pm_notifier_func(struct notifier_block *nb, unsigned long event, void *dummy)
{
	struct atc260x_dev *atc260x;

	atc260x = container_of(nb, struct atc260x_dev, pm_notif_blk);

	/* TODO */
	/*switch(event) { */
	/*case PM_HIBERNATION_PREPARE: ** Going to hibernate ** */
	/*  break; */
	/*case PM_POST_HIBERNATION:    ** Hibernation finished ** */
	/*  break; */
	/*case PM_SUSPEND_PREPARE:     ** Going to suspend the system ** */
	/*  break; */
	/*case PM_POST_SUSPEND:        ** Suspend finished ** */
	/*  break; */
	/*case PM_RESTORE_PREPARE:     ** Going to restore a saved image ** */
	/*  break; */
	/*case PM_POST_RESTORE:        ** Restore failed ** */
	/*  break; */
	/*} */
	return NOTIFY_OK;
}

#if 0  /* for io debug */
static void _atc260x_init_test_reg_access(struct atc260x_dev *atc260x)
{
	static const u16 sc_test_reg_tbl[ATC260X_ICTYPE_CNT][2] = {
		[ATC260X_ICTYPE_2603A] = {ATC2603A_PMU_SYS_CTL8, ATC2603A_PMU_SYS_CTL9},
		[ATC260X_ICTYPE_2603C] = {ATC2603C_PMU_FW_USE3, ATC2603C_PMU_FW_USE4},
		[ATC260X_ICTYPE_2609A] = {ATC2609A_PMU_FW_USE0, ATC2609A_PMU_FW_USE1},
	};
	uint i, seed, tmp1, tmp2;
	u16 reg1_addr, reg2_addr;
	int ret;

	dev_err(atc260x->dev, "start reg io test ............................\n");
	dump_stack();

	reg1_addr = sc_test_reg_tbl[atc260x->ic_type][0];
	reg2_addr = sc_test_reg_tbl[atc260x->ic_type][1];

	seed = 0x123456;
	for (i = 0; i < 10000; i++) {
		seed = seed * 1103515245U + 12345U; /* randomize */
		tmp1 = ((seed & 0xffffU) ^ (seed >> 16)) & 0xffffU;
		tmp2 = ((seed & 0xffffU) ^ ~(seed >> 16)) & 0xffffU;
		ret = atc260x_reg_write(atc260x, reg1_addr, tmp1);
		if (ret) {
			dev_err(atc260x->dev, "test write err, i=%u reg=0x%x value=0x%x\n",
				i, ATC2603C_PMU_FW_USE4, tmp1);
			break;
		}
		ret = atc260x_reg_write(atc260x, reg2_addr, tmp2);
		if (ret) {
			dev_err(atc260x->dev, "test write err, i=%u reg=0x%x value=0x%x\n",
				i, ATC2603C_PMU_FW_USE4, tmp2);
			break;
		}
		if (i == 5000) {
			dev_err(atc260x->dev, "test switch to direct_access mode\n");
			atc260x_set_reg_direct_access(atc260x, true);
		}
		ret = atc260x_reg_read(atc260x, reg1_addr);
		if (ret < 0 || (uint)ret != tmp1) {
			dev_err(atc260x->dev, "test write err, i=%u reg=0x%x value=0x%x org_value=0x%x\n",
				i, ATC2603C_PMU_FW_USE3, ret, tmp1);
			break;
		}
		ret = atc260x_reg_read(atc260x, reg2_addr);
		if (ret < 0 || (uint)ret != tmp2) {
			dev_err(atc260x->dev, "test write err, i=%u reg=0x%x value=0x%x org_value=0x%x\n",
				i, ATC2603C_PMU_FW_USE4, ret, tmp2);
			break;
		}
		if ((i % 512) == 0) {
			dev_err(atc260x->dev, "test cycle = %u\n", i);
		}
		ret = 0;
	}
	dev_err(atc260x->dev, "end reg io test. ret=%d\n", ret);
	BUG();
}
#endif

static int _atc2603a_init_hardware(struct atc260x_dev *atc260x)
{
	uint tmp;
	int ret;

	ret = atc260x_reg_read(atc260x, ATC2603A_CMU_HOSCCTL);
	if (ret < 0) {
		dev_err(atc260x->dev, "Failed to read magicnum, ret=%d\n", ret);
		goto err;
	}
	if ((ret & ~(1U<<15)) != 0x2aab) {
		dev_err(atc260x->dev, "Device is not a atc2603a: Register 0x%x value 0x%x!=0x%x\n",
			ATC2603A_PMU_OC_INT_EN, ret, 0x2aab);
		ret = -EINVAL;
		goto err;
	}
	/*_atc260x_init_test_reg_access(atc260x); */

	ret = atc260x_reg_read(atc260x, ATC2603A_PMU_BDG_CTL);
	tmp = ret;
	tmp |= (0x1 << 7);
	tmp |= (0x1 << 6);
	tmp &= ~(0x1 << 5);
	tmp &= ~(0x1 << 11);
	atc260x_reg_write(atc260x, ATC2603A_PMU_BDG_CTL, tmp);
	dev_info(atc260x->dev, "set bdg ctl, ret=%d value=0x%x\n", ret, tmp);
	/*atc260x_reg_setbits(atc260x, ATC2603A_PMU_BDG_CTL, (0x1 << 7),(0x1 << 7));
	atc260x_reg_setbits(atc260x, ATC2603A_PMU_BDG_CTL, (0x1 << 11),(~(0x1 << 11)));
	atc260x_reg_setbits(atc260x, ATC2603A_PMU_BDG_CTL, (0x1 << 6),(0x1 << 6));
	atc260x_reg_setbits(atc260x, ATC2603A_PMU_BDG_CTL, (0x1 << 5),(~(0x1 << 5)));*/

	/* disable TP pad (X1, Y1, X2, Y2) for external TP chip */
	ret = atc260x_reg_setbits(atc260x, ATC2603A_MFP_CTL1, 0x00e0, 0x0);

	/* init interrupt */
	atc260x_cmu_reset(atc260x, ATC260X_CMU_MODULE_INTS); /* reset ATC260X INTC */
	atc260x_reg_write(atc260x, ATC2603A_INTS_MSK, 0);    /* disable all sources */
	atc260x_reg_setbits(atc260x, ATC2603A_PAD_EN, 0x1, 0x1); /* Enable P_EXTIRQ pad */

	err:
	return ret;
}
static int _atc2603c_init_hardware(struct atc260x_dev *atc260x)
{
	uint tmp;
	int ret;

	ret = atc260x_reg_read(atc260x, ATC2603C_PMU_OC_INT_EN);
	if (ret < 0) {
		dev_err(atc260x->dev, "Failed to read magicnum, ret=%d\n", ret);
		goto err;
	}
	if (ret != 0x1bc0) {
		dev_err(atc260x->dev, "Device is not an atc2603c: Register 0x%x value 0x%x!=0x%x\n",
			ATC2603C_PMU_OC_INT_EN, ret, 0x1bc0);
		ret = -EINVAL;
		goto err;
	}
	/*_atc260x_init_test_reg_access(atc260x); */

	ret = atc260x_reg_read(atc260x, ATC2603C_PMU_BDG_CTL);
	tmp = ret;
	tmp |= (0x1 << 7);      /*dbg enable */
	tmp |= (0x1 << 6);      /*dbg filter. */
	tmp &= ~(0x1 << 5);     /*disabel pulldown resistor. */
	tmp &= ~(0x1 << 11);    /*efuse. */
	ret = atc260x_reg_write(atc260x, ATC2603C_PMU_BDG_CTL, tmp);
	dev_info(atc260x->dev, "set bdg ctl, ret=%d value=0x%x\n", ret, tmp);
	/*atc260x_reg_setbits(atc260x, ATC2603C_PMU_BDG_CTL, (0x1 << 7),(0x1 << 7));
	atc260x_reg_setbits(atc260x, ATC2603C_PMU_BDG_CTL, (0x1 << 11),(~(0x1 << 11)));
	atc260x_reg_setbits(atc260x, ATC2603C_PMU_BDG_CTL, (0x1 << 6),(0x1 << 6));
	atc260x_reg_setbits(atc260x, ATC2603C_PMU_BDG_CTL, (0x1 << 5),(~(0x1 << 5)));*/

	/* init interrupt */
	atc260x_cmu_reset(atc260x, ATC260X_CMU_MODULE_INTS); /* reset ATC260X INTC */
	atc260x_reg_write(atc260x, ATC2603C_INTS_MSK, 0);    /* disable all sources */
	atc260x_reg_setbits(atc260x, ATC2603C_PAD_EN, 0x1, 0x1); /* Enable P_EXTIRQ pad */

	/* Output 32K clock */
	{
		u32 of_cfg_val;
		int ret, vsel_31;

		ret = of_property_read_u32(atc260x->dev->of_node, "losc_32k_output_enable", &of_cfg_val);
		if (ret == 0 && of_cfg_val != 0) {
			vsel_31 = 0;
			ret = of_property_read_u32(atc260x->dev->of_node, "losc_32k_output_voltage", &of_cfg_val);
			if (ret == 0)
				vsel_31 = (of_cfg_val == 31) ? 1 : 0;

			atc260x_reg_setbits(atc260x, ATC2603C_PAD_VSEL, (1U << 2), (vsel_31 << 2));
			atc260x_reg_setbits(atc260x, ATC2603C_MFP_CTL, (3U << 7), (2U << 7));
			atc260x_reg_setbits(atc260x, ATC2603C_PAD_EN, (1U << 2), (1U << 2));
		}
	}

	ret = 0;

	err:
	return ret;
}
static int _atc2609a_init_hardware(struct atc260x_dev *atc260x)
{
	int ret;

	ret = atc260x_reg_read(atc260x, ATC2609A_PMU_OC_INT_EN);
	if (ret < 0) {
		dev_err(atc260x->dev, "Failed to read magicnum, ret=%d\n", ret);
		goto err;
	}
	if (ret != 0x0ff8) {
		dev_err(atc260x->dev, "Device is not an atc2609a: Register 0x%x value 0x%x!=0x%x\n",
			ATC2603C_PMU_OC_INT_EN, ret, 0x0ff8);
		ret = -EINVAL;
		goto err;
	}
	/*_atc260x_init_test_reg_access(atc260x); */

	/* init interrupt */
	atc260x_cmu_reset(atc260x, ATC260X_CMU_MODULE_INTS); /* reset ATC260X INTC */
	atc260x_reg_write(atc260x, ATC2609A_INTS_MSK, 0);    /* disable all sources */
	atc260x_reg_setbits(atc260x, ATC2609A_PAD_EN, 0x1, 0x1); /* Enable P_EXTIRQ pad */

	/* Output 32K clock */
	{
		u32 of_cfg_val;
		int ret;
		ret = of_property_read_u32(atc260x->dev->of_node, "losc_32k_output_enable", &of_cfg_val);
		if (ret == 0 && of_cfg_val != 0) {
			atc260x_reg_setbits(atc260x, ATC2609A_PMU_SYS_CTL4, (7U << 5), (4U << 5));
		}
	}

	ret = 0;

	err:
	return ret;
}
static int atc260x_init_hardware(struct atc260x_dev *atc260x)
{
	int ret = -ENXIO;
	switch (atc260x->ic_type) {
	case ATC260X_ICTYPE_2603A:
		ret = _atc2603a_init_hardware(atc260x);
		break;
	case ATC260X_ICTYPE_2603C:
		ret = _atc2603c_init_hardware(atc260x);
		break;
	case ATC260X_ICTYPE_2609A:
		ret = _atc2609a_init_hardware(atc260x);
		break;
	}
	return ret;
}



/* ATC2603A IRQs */
#define ATC2603A_IRQ_AUDIO              (0)
#define ATC2603A_IRQ_TP                 (1)
#define ATC2603A_IRQ_ETHERNET           (2)
#define ATC2603A_IRQ_OV                 (3)
#define ATC2603A_IRQ_OC                 (4)
#define ATC2603A_IRQ_OT                 (5)
#define ATC2603A_IRQ_UV                 (6)
#define ATC2603A_IRQ_ALARM              (7)
#define ATC2603A_IRQ_ONOFF              (8)
#define ATC2603A_IRQ_WKUP               (9)
#define ATC2603A_IRQ_IR                 (10)

/* ATC2603C IRQs */
#define ATC2603C_IRQ_AUDIO              (0)
#define ATC2603C_IRQ_OV                 (1)
#define ATC2603C_IRQ_OC                 (2)
#define ATC2603C_IRQ_OT                 (3)
#define ATC2603C_IRQ_UV                 (4)
#define ATC2603C_IRQ_ALARM              (5)
#define ATC2603C_IRQ_ONOFF              (6)
#define ATC2603C_IRQ_SGPIO              (7)
#define ATC2603C_IRQ_IR                 (8)
#define ATC2603C_IRQ_REMCON             (9)
#define ATC2603C_IRQ_POWER_IN           (10)

/* ATC2609A IRQs */
#define ATC2609A_IRQ_AUDIO              (0)
#define ATC2609A_IRQ_OV                 (1)
#define ATC2609A_IRQ_OC                 (2)
#define ATC2609A_IRQ_OT                 (3)
#define ATC2609A_IRQ_UV                 (4)
#define ATC2609A_IRQ_ALARM              (5)
#define ATC2609A_IRQ_ONOFF              (6)
#define ATC2609A_IRQ_WKUP               (7)
#define ATC2609A_IRQ_IR                 (8)
#define ATC2609A_IRQ_REMCON             (9)
#define ATC2609A_IRQ_POWER_IN           (10)

static const struct regmap_irq atc2603a_irqs[] = {
	[ATC2603A_IRQ_AUDIO]    = { .reg_offset = 0, .mask = BIT(0), },
	[ATC2603A_IRQ_TP]       = { .reg_offset = 0, .mask = BIT(1), },
	[ATC2603A_IRQ_ETHERNET] = { .reg_offset = 0, .mask = BIT(2), },
	[ATC2603A_IRQ_OV]       = { .reg_offset = 0, .mask = BIT(3), },
	[ATC2603A_IRQ_OC]       = { .reg_offset = 0, .mask = BIT(4), },
	[ATC2603A_IRQ_OT]       = { .reg_offset = 0, .mask = BIT(5), },
	[ATC2603A_IRQ_UV]       = { .reg_offset = 0, .mask = BIT(6), },
	[ATC2603A_IRQ_ALARM]    = { .reg_offset = 0, .mask = BIT(7), },
	[ATC2603A_IRQ_ONOFF]    = { .reg_offset = 0, .mask = BIT(8), },
	[ATC2603A_IRQ_WKUP]     = { .reg_offset = 0, .mask = BIT(9), },
	[ATC2603A_IRQ_IR]       = { .reg_offset = 0, .mask = BIT(10), },
};
static const struct regmap_irq_chip atc2603a_irq_chip = {
	.name = "atc2603a",
	.irqs = atc2603a_irqs,
	.num_irqs = ARRAY_SIZE(atc2603a_irqs),

	.num_regs = 1,
	.status_base = ATC2603A_INTS_PD,
	.mask_base = ATC2603A_INTS_MSK,
	.mask_invert = true,
};

static const struct regmap_irq atc2603c_irqs[] = {
	[ATC2603C_IRQ_AUDIO]    = { .reg_offset = 0, .mask = BIT(0), },
	[ATC2603C_IRQ_OV]       = { .reg_offset = 0, .mask = BIT(1), },
	[ATC2603C_IRQ_OC]       = { .reg_offset = 0, .mask = BIT(2), },
	[ATC2603C_IRQ_OT]       = { .reg_offset = 0, .mask = BIT(3), },
	[ATC2603C_IRQ_UV]       = { .reg_offset = 0, .mask = BIT(4), },
	[ATC2603C_IRQ_ALARM]    = { .reg_offset = 0, .mask = BIT(5), },
	[ATC2603C_IRQ_ONOFF]    = { .reg_offset = 0, .mask = BIT(6), },
	[ATC2603C_IRQ_SGPIO]    = { .reg_offset = 0, .mask = BIT(7), },
	[ATC2603C_IRQ_IR]       = { .reg_offset = 0, .mask = BIT(8), },
	[ATC2603C_IRQ_REMCON]   = { .reg_offset = 0, .mask = BIT(9), },
	[ATC2603C_IRQ_POWER_IN] = { .reg_offset = 0, .mask = BIT(10), },
};
static const struct regmap_irq_chip atc2603c_irq_chip = {
	.name = "atc2603c",
	.irqs = atc2603c_irqs,
	.num_irqs = ARRAY_SIZE(atc2603c_irqs),

	.num_regs = 1,
	.status_base = ATC2603C_INTS_PD,
    .ack_base = ATC2603C_INTS_PD,
	.mask_base = ATC2603C_INTS_MSK,
	.mask_invert = true,
};

static const struct regmap_irq atc2609a_irqs[] = {
	[ATC2609A_IRQ_AUDIO]    = { .reg_offset = 0, .mask = BIT(0), },
	[ATC2609A_IRQ_OV]       = { .reg_offset = 0, .mask = BIT(1), },
	[ATC2609A_IRQ_OC]       = { .reg_offset = 0, .mask = BIT(2), },
	[ATC2609A_IRQ_OT]       = { .reg_offset = 0, .mask = BIT(3), },
	[ATC2609A_IRQ_UV]       = { .reg_offset = 0, .mask = BIT(4), },
	[ATC2609A_IRQ_ALARM]    = { .reg_offset = 0, .mask = BIT(5), },
	[ATC2609A_IRQ_ONOFF]    = { .reg_offset = 0, .mask = BIT(6), },
	[ATC2609A_IRQ_WKUP]     = { .reg_offset = 0, .mask = BIT(7), },
	[ATC2609A_IRQ_IR]       = { .reg_offset = 0, .mask = BIT(8), },
	[ATC2609A_IRQ_REMCON]   = { .reg_offset = 0, .mask = BIT(9), },
	[ATC2609A_IRQ_POWER_IN] = { .reg_offset = 0, .mask = BIT(10), },
};
static const struct regmap_irq_chip atc2609a_irq_chip = {
	.name = "atc2609a",
	.irqs = atc2609a_irqs,
	.num_irqs = ARRAY_SIZE(atc2609a_irqs),

	.num_regs = 1,
	.status_base = ATC2609A_INTS_PD,
	.mask_base = ATC2609A_INTS_MSK,
	.mask_invert = true,
};

static const struct regmap_irq_chip * const sc_atc260x_irq_chip_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = &atc2603a_irq_chip,
	[ATC260X_ICTYPE_2603C] = &atc2603c_irq_chip,
	[ATC260X_ICTYPE_2609A] = &atc2609a_irq_chip
};


/*#ifdef CONFIG_ATC260X_SYSFS_REG */
#if 1
static ssize_t store_atc260x_reg(struct device *dev, struct device_attribute *attr,
								 const char *buf, size_t count)
{
	struct atc260x_dev *atc260x;
	unsigned int reg, reg2, mid_token, reg_val;
	char *end_ptr;
	int ret, write;

	atc260x = dev_get_drvdata(dev);
	ATC260X_ASSERT_VALID_DEV(atc260x);

	reg = simple_strtoul(buf, &end_ptr, 16);
	if ((buf == end_ptr) || (reg > 0xffff))
		goto out;
	mid_token = *end_ptr++;

	reg2 = reg;
	write = reg_val = 0;
	switch (mid_token) {
	case '-' :
		reg2 = simple_strtoul(end_ptr, NULL, 16);
		if (reg2 > 0xffff || reg2 < reg)
			goto out;
		break;
	case '=' :
		reg_val = simple_strtoul(end_ptr, NULL, 16);
		if (reg_val > 0xffff)
			goto out;
		write = 1;
		break;
	}

	if (write) {
		ret = atc260x_reg_write(atc260x, reg, reg_val);
		if (ret < 0)
			goto out;
		pr_err("[ATC260x] reg [0x%04x] <- 0x%04x\n", reg, reg_val);
	}
	/* "read" or "read back after written" */
	for (; reg <= reg2; reg++) {
		ret = atc260x_reg_read(atc260x, reg);
		if (ret < 0)
			goto out;
		pr_err("[ATC260x] reg [0x%04x] :  0x%04x\n", reg, ret);
	}
out:
	return count;
}
static ssize_t show_atc260x_reg(struct device *dev,
								struct device_attribute *attr, char *buf)
{
	return sprintf(buf,
		"echo reg_addr > reg_dbg  : dump register @reg_addr\n"
		"echo reg_addr1-reg_addr1 > reg_dbg  : dump register @range [reg_addr1,reg_addr2]\n"
		"echo reg_addr=value > reg_dbg  : write value to register @reg_addr\n");
}
static ssize_t show_atc260x_auxadc_dbg_values(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct atc260x_dev *atc260x;
	const char *ch_name, *ch_unit_name;
	uint channel, size_accu;
	s32 tr_value;
	int ret, ret2;

	atc260x = dev_get_drvdata(dev);
	ATC260X_ASSERT_VALID_DEV(atc260x);

	size_accu = 0; ;
	for (channel = 0; ; channel++){
		ch_name = atc260x_auxadc_channel_name(atc260x, channel);
		ch_unit_name = atc260x_auxadc_channel_unit_name(atc260x, channel);
		if (ch_name == NULL || ch_unit_name == NULL) {
			break;
		}
		ret = atc260x_auxadc_get_translated(atc260x, channel, &tr_value);
		if (ret == 0) {
			ret2 = scnprintf(buf, PAGE_SIZE-size_accu, "%-2u %-10s  %d %s\n",
				channel, ch_name, tr_value, ch_unit_name);
		} else {
			ret2 = scnprintf(buf, PAGE_SIZE-size_accu, "%-2u %-10s <error, ret=%d>\n",
				channel, ch_name, ret);
		}
		if (ret2 < 0) {
			break;
		}
		size_accu += ret2;
		buf += ret2;
	}
	return size_accu;
}
static ssize_t show_atc260x_pstore_dbg_values(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct atc260x_dev *atc260x;
	int ret;

	atc260x = dev_get_drvdata(dev);
	ATC260X_ASSERT_VALID_DEV(atc260x);

	ret = atc260x_pstore_dbg_dump(atc260x, buf, PAGE_SIZE);
	if (ret < 0){
		dev_err(atc260x->dev, "%s() err, ret=%d\n", __func__, ret);
		return 0;
	}
	return ret;
}

static struct device_attribute atc260x_attrs[] = {
	__ATTR(reg_dbg, 0644, show_atc260x_reg, store_atc260x_reg),
	__ATTR(auxadc_dbg, 0444, show_atc260x_auxadc_dbg_values, NULL),
	__ATTR(pstore_dbg, 0444, show_atc260x_pstore_dbg_values, NULL),
};

/* create sysfs register operation interface */
static int atc260x_create_attr(struct device *dev)
{
	int i, ret;
	for (i = 0; i < ARRAY_SIZE(atc260x_attrs); i++) {
		ret = device_create_file(dev, &atc260x_attrs[i]);
		if (ret)
			return ret;
	}
	return 0;
}
static void atc260x_remove_attr(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(atc260x_attrs); i++) {
		device_remove_file(dev, &atc260x_attrs[i]);
	}
}
#else /* CONFIG_ATC260X_SYSFS_REG */
static int atc260x_create_attr(struct device *dev)
{
	return 0;
}
static void atc260x_remove_attr(struct device *dev)
{
}
#endif /* CONFIG_ATC260X_SYSFS_REG */



static uint atc260x_get_revised_version(struct atc260x_dev *atc260x)
{
	static const u16 sc_ver_reg_addr_tbl[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = ATC2603A_CVER,
		[ATC260X_ICTYPE_2603C] = ATC2603C_CHIP_VER,
		[ATC260X_ICTYPE_2609A] = ATC2609A_CHIP_VER
	};
	uint val, reg;

	/* 目前支持的3款IC的rev定义都是一致的, 后续有不一致的情况就要走分支. */

	BUG_ON(atc260x->ic_type >= ARRAY_SIZE(sc_ver_reg_addr_tbl));
	reg = sc_ver_reg_addr_tbl[atc260x->ic_type];
	BUG_ON(reg == 0);
	val = atc260x_reg_read(atc260x, reg);
	BUG_ON((int)val < 0 || val > 31);
	return __ffs(val + 1U);
}

/* import the sub-devices define. */
#define __MFD_ATC260X__NEED_SUB_DEV_DEFINE__ 1
#include "atc260x-subdev.h"
#undef __MFD_ATC260X__NEED_SUB_DEV_DEFINE__

int atc260x_core_dev_init(struct atc260x_dev *atc260x)
{
	struct irq_domain *p_regmap_irq_domain;
	int ret;

	dev_info(atc260x->dev, "%s() enter\n", __func__);

	/* prepare atc260x structure. */
	atc260x->reg_save_buf = devm_kzalloc(
		atc260x->dev,
		sizeof(*(atc260x->reg_save_buf)) * ATC260X_MAX_SAVE_REGS,
		GFP_KERNEL);
	if (atc260x->reg_save_buf == NULL) {
		dev_err(atc260x->dev, "no mem\n");
		ret = -ENOMEM;
		goto label_err_lv0;
	}
	dev_set_drvdata(atc260x->dev, atc260x);

	/* set type ID */
	atc260x->_obj_type_id = ATC260x_PARENT_OBJ_TYPE_ID;

	/* hardware init. */
	ret = atc260x_init_hardware(atc260x);
	if (ret) {
		dev_err(atc260x->dev, "failed to init hardware, ret=%d\n", ret);
		goto label_err_lv1;
	}

	atc260x->ic_ver = atc260x_get_revised_version(atc260x);
	dev_info(atc260x->dev, "detect PMU chip type %u ver %c\n",
			atc260x->ic_type, 'A' + atc260x->ic_ver);

	/* init direct access */
	spin_lock_init(&atc260x->dacc_spinlock);
	atc260x->reg_access_mode = ATC260X_ACCESS_MODE_NORMAL;

	/* irq chip */
	dev_info(atc260x->dev, "PMU root IRQ %u\n", atc260x->irq);
	BUG_ON(atc260x->ic_type >= ARRAY_SIZE(sc_atc260x_irq_chip_tbl));
	/* PMU subdev IRQ use dynamic (linear) mapping (set argument irq_base=0) */
	ret = regmap_add_irq_chip(atc260x->regmap, atc260x->irq,
				  IRQF_ONESHOT | IRQF_SHARED, 0,
				  sc_atc260x_irq_chip_tbl[atc260x->ic_type],
				  &atc260x->regmap_irqc_data);
	if (ret) {
		dev_err(atc260x->dev, "failed to add irq chip: %d\n", ret);
		goto label_err_lv1;
	}
	/* get the irq_domain, used later */
	p_regmap_irq_domain = regmap_irq_get_domain(atc260x->regmap_irqc_data);
	BUG_ON(p_regmap_irq_domain == NULL);

	/* init auxadc sub-function */
	ret = atc260x_auxadc_dev_init(atc260x);
	if (ret) {
		dev_err(atc260x->dev, "failed to init auxadc: %d\n", ret);
		goto label_err_lv2;
	}

	/* register PM notify */
	atc260x->pm_notif_blk.notifier_call = atc260x_pm_notifier_func;
	ret = register_pm_notifier(&(atc260x->pm_notif_blk));
	if (ret) {
		dev_err(atc260x->dev, "failed to register pm_notifier, ret=%d\n", ret);
		goto label_err_lv3;
	}

	/* sysfs */
	ret = atc260x_create_attr(atc260x->dev);
	if (ret) {
		dev_err(atc260x->dev, "failed to create sysfs nodes, ret=%d\n", ret);
		goto label_err_lv4;
	}

	/* init Global API */
	atc260x_extapi_dev_init(atc260x);

	/* The core device is up, instantiate the subdevices.
	 * We proveide the irq_domain,
	 * so IRQ will get mapped after mfd_add_devices. */
	BUG_ON(atc260x->ic_type >= ARRAY_SIZE(sc_atc260x_mfd_cell_def_tbl) ||
		atc260x->ic_type >= ARRAY_SIZE(sc_atc260x_mfd_cell_cnt_tbl) ||
		sc_atc260x_mfd_cell_def_tbl[atc260x->ic_type] == NULL ||
		sc_atc260x_mfd_cell_cnt_tbl[atc260x->ic_type] == 0);
	ret = mfd_add_devices(atc260x->dev, 0,
		sc_atc260x_mfd_cell_def_tbl[atc260x->ic_type],
		sc_atc260x_mfd_cell_cnt_tbl[atc260x->ic_type],
		NULL, 0, p_regmap_irq_domain);
	if (ret) {
		dev_err(atc260x->dev, "failed to add children devices: %d\n", ret);
		goto label_err_lv5;
	}

	dev_info(atc260x->dev, "%s() exit\n", __func__);
	return 0;

	label_err_lv5:
	atc260x_extapi_dev_exit(atc260x);
	atc260x_remove_attr(atc260x->dev);
	label_err_lv4:
	unregister_pm_notifier(&(atc260x->pm_notif_blk));
	label_err_lv3:
	atc260x_auxadc_dev_exit(atc260x);
	label_err_lv2:
	regmap_del_irq_chip(atc260x->irq, atc260x->regmap_irqc_data);
	label_err_lv1:
	dev_set_drvdata(atc260x->dev, NULL);
	label_err_lv0:
	atc260x->_obj_type_id = 0;
	return ret;
}

void atc260x_core_dev_exit(struct atc260x_dev *atc260x)
{
	ATC260X_ASSERT_VALID_DEV(atc260x);

	mfd_remove_devices(atc260x->dev); /* remove all sub devices */

	atc260x_extapi_dev_exit(atc260x);
	atc260x_remove_attr(atc260x->dev);
	unregister_pm_notifier(&(atc260x->pm_notif_blk));
	atc260x_auxadc_dev_exit(atc260x);
	regmap_del_irq_chip(atc260x->irq, atc260x->regmap_irqc_data);
	dev_set_drvdata(atc260x->dev, NULL);
	atc260x->_obj_type_id = 0;
}

int atc260x_core_dev_suspend(struct atc260x_dev *atc260x)
{
	int ret;

	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_info(atc260x->dev, "%s() enter\n", __func__);

	disable_irq(atc260x->irq);

	/* save critical registers. */
	ret = _atc260x_save_critical_regs(atc260x);
	if (ret) {
		dev_err(atc260x->dev, "%s() save reg err, ret=%d\n", __func__, ret);
		return ret;
	}

	dev_dbg(atc260x->dev, "%s() exit\n", __func__);
	return 0;
}

int atc260x_core_dev_suspend_late(struct atc260x_dev *atc260x)
{
	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_info(atc260x->dev, "%s() enter\n", __func__);

	/* disable all reg access until atc260x_set_reg_direct_access is called */
	atc260x->reg_access_mode = ATC260X_ACCESS_MODE_NONE;
	/* 这里不立即启用direct_access模式.
	 * 因为i2c驱动现在还没有suspend, 仍有可能有未suspend的其它设备找它传输数据, 这里立即切到
	 * direct_access模式就会影响i2c功能(产生竟争状态). */

	dev_dbg(atc260x->dev, "%s() exit\n", __func__);
	return 0;
}

int atc260x_core_dev_resume_early(struct atc260x_dev *atc260x)
{
	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_info(atc260x->dev, "%s() enter\n", __func__);
	atc260x_exit_reg_direct_access(atc260x);
	dev_dbg(atc260x->dev, "%s() exit\n", __func__);
	return 0;
}

int atc260x_core_dev_resume(struct atc260x_dev *atc260x)
{
	int ret;

	ATC260X_ASSERT_VALID_DEV(atc260x);
	dev_info(atc260x->dev, "%s() enter\n", __func__);

	/* restore critical registers. */
	ret = _atc260x_restore_critical_regs(atc260x);
	if (ret) {
		dev_err(atc260x->dev, "%s() restore reg err, ret=%d\n", __func__, ret);
		return ret;
	}

	/*enable irq MUST after restore registers */
	enable_irq(atc260x->irq);

	dev_dbg(atc260x->dev, "%s() exit\n", __func__);
	return 0;
}

/* About the suspend order
 * 关于 suspend 顺序的说明:
 *
 * kernel的suspend顺序是依赖于设备的注册顺序的(单纯的device_add()的顺序, 与驱动probe顺序几乎无关),
 * 而i2c/spi总线上的260x设备的device_add()一定是晚于mmc_host/usb_host等由DTS注册的platform_dev的.
 * 故260x设备的suspend一定是比mmc_host/usb_host等platform_dev调用得要早.
 *
 * 这里有个问题: mmc_host/usb_host等设备会用到regulator, 这就要求260x设备以及它的regulator子设备
 * 在suspend后要仍然能工作. 这里260x旧将部分影响功能的操作放到suspend_late执行.
 *
 * 由于i2c驱动的suspend是放在suspend_noirq阶段, 为了防止其suspend后260x仍通过regmap操作它,
 * 在260x的suspend_late时将reg读写锁死, 直到direct_access模式启用后才可继续访问reg.
 * 因为这锁死之前, 所有依赖260x的设备均已经suspend, 不会造成问题.
 *
 * direct_access模式启用必须在i2c/spi的suspend_noirq之后, 否则两者会争抢操作硬件.
 *
 * 安全的suspend顺序是:
 *   0. 非platform设备suspend (期间会调用regulator_disable关闭dcdc/ldo)
 *   1. regulator suspend (不做任何事情)
 *   2. 260x suspend
 *   3. mmc_host / camera 等platform设备 suspend (期间会调用regulator_disable关闭dcdc/ldo)
 *   4. regulator suspend_late (打印regulator状态)
 *   5. 260x suspend_late (锁寄存器访问)
 *   6. i2c/spi host suspend_noirq (关闭i2c/spi硬件)
 *   7. atc260x_set_reg_direct_access() 被 platform_suspend_ops.prepare_late 调用
 *   8. 系统进S2
 *
 * 安全的resume顺序则相反. */
