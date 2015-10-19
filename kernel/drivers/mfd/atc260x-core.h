/*
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

/* This header is used by core only, other modules has no need to include this. */


#ifndef __MFD_ATC260X_CORE_H__
#define __MFD_ATC260X_CORE_H__

#include <linux/kernel.h>
#include <linux/mfd/atc260x/atc260x.h>


#define ATC260X_MAX_SAVE_REGS       256

enum {
	ATC260X_ACCESS_MODE_NORMAL = 0,
	ATC260X_ACCESS_MODE_DIRECT,
	ATC260X_ACCESS_MODE_NONE
};


struct atc260x_auxadc_chn_hwinfo;

/* ATC260X device */
struct atc260x_dev {
	/* all fields internal used only, not for sub devices! */

	struct device               *dev;
	int                         irq;       /* 260x_core的IRQ号, 对应SOC的某个外部中断. */
	struct spi_device           *spi;
	struct i2c_client           *i2c_client;
	struct regmap               *regmap;
	struct regmap_irq_chip_data *regmap_irqc_data;

	u8                          ic_type;       /* see ATC260X_ICTYPE_2603A ... */
	u8                          ic_ver;        /* see ATC260X_ICVER_A ... */
	u8                          bus_num;       /* SPI / I2C bus number, used by reg direct access. */
	u8                          bus_addr;      /* device's bus address, only for I2C, 7bit, r/w bit excluded */
	u8                          reg_access_mode; /* see ATC260X_ACCESS_MODE_NORMAL ... */
	u8                          direct_access_ioremap_flag;

	spinlock_t                  dacc_spinlock; /* for direct access */
	void __iomem *              dacc_iobase;     /* for bus, I2C SPI ... */
	void __iomem *              dacc_cmu_iobase; /* for CMU */
	int (*direct_read_reg)(struct atc260x_dev *atc260x, uint reg);
	int (*direct_write_reg)(struct atc260x_dev *atc260x, uint reg, u16 val);
	void (*direct_acc_init)(struct atc260x_dev *atc260x);
	void (*direct_acc_exit)(struct atc260x_dev *atc260x);

	struct mutex                auxadc_read_mutex;
	const struct atc260x_auxadc_hwinfo *auxadc_hwinfo;
	void                        *auxadc_udata;

	struct notifier_block       pm_notif_blk;

	u16                         *reg_save_buf;

	u32                         _obj_type_id;
};

#define ATC260x_PARENT_OBJ_TYPE_ID 0x72f80927U

/* for debug */
#define ATC260X_ASSERT_VALID_DEV(ADEV) \
	BUG_ON((ADEV == NULL) || IS_ERR(ADEV) || \
	 (ADEV)->_obj_type_id != ATC260x_PARENT_OBJ_TYPE_ID)
#define ATC260X_CHK_VALID_DEV(ADEV) \
	(! ((ADEV == NULL) || IS_ERR(ADEV) || \
	 (ADEV)->_obj_type_id != ATC260x_PARENT_OBJ_TYPE_ID))


/* only for core internal. */
extern int atc260x_core_dev_init(struct atc260x_dev *atc260x);
extern void atc260x_core_dev_exit(struct atc260x_dev *atc260x);
extern int atc260x_core_dev_suspend(struct atc260x_dev *atc260x);
extern int atc260x_core_dev_suspend_late(struct atc260x_dev *atc260x);
extern int atc260x_core_dev_resume_early(struct atc260x_dev *atc260x);
extern int atc260x_core_dev_resume(struct atc260x_dev *atc260x);

extern int atc260x_auxadc_dev_init(struct atc260x_dev *atc260x);
extern void atc260x_auxadc_dev_exit(struct atc260x_dev *atc260x);

extern void atc260x_extapi_dev_init(struct atc260x_dev *atc260x);
extern void atc260x_extapi_dev_exit(struct atc260x_dev *atc260x);


extern int atc260x_pstore_dbg_dump(struct atc260x_dev *atc260x, char *buf, uint bufsize);


#endif /* __MFD_ATC260X_CORE_H__ */
