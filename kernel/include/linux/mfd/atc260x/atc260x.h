/*
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

/*  UTF-8 encoded.  */

/* 这里给出的大部分API都是给子设备使用的, 名字均以 atc260x_ 开头, 均需要提供父设备device指针,
 * 另外有少量API供外部其它模块使用, 名字以 atc260x_ex_ 开头,
 * 这样做主要是为了避免原本内部使用的API被外部滥用, 多IC兼容时造成麻烦.
 * 设计上要做到: 子设备之外的其它模块很难拿到atc260x_dev的指针. */


/* atc260x 寄存器归属说明:
 * 1. 各个子模块的寄存器归各自所有
 * 2. CMU模块的寄存器归core所有, 各个子设备不能直接操作, core有接口
 * 3. INT模块的寄存器归core/regmap所有, 各个子设备不能直接操作, 注册ISR(request_irq)时会自动配置
 * 4. PMU-AUXADC的寄存器归core所有, ..., core有接口
 * 5. PMU中不掉电不reset的FW用寄存器归core所有, ..., core有接口
 * 6. MFP 原本是公有的, 但是由于配置甚少, 各个子设备自行维护. */



#ifndef __MFD_ATC260X_H__
#define __MFD_ATC260X_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>

#include <linux/mfd/atc260x/regs_map_atc2603a.h>
#include <linux/mfd/atc260x/regs_map_atc2603c.h>
#include <linux/mfd/atc260x/regs_map_atc2609a.h>

enum {
	/* DO NOT change the order!
	 * 进suspend的最后一段汇编代码依赖于这里的顺序来区分IC. */
	ATC260X_ICTYPE_2603A = 0,
	ATC260X_ICTYPE_2603C,
	ATC260X_ICTYPE_2609A,
	ATC260X_ICTYPE_CNT
};

enum {
	ATC260X_ICVER_A = 0,
	ATC260X_ICVER_B,
	ATC260X_ICVER_C,
	ATC260X_ICVER_D,
	ATC260X_ICVER_E,
	ATC260X_ICVER_F,
	ATC260X_ICVER_G,
	ATC260X_ICVER_H,
};

struct atc260x_dev;

/* Get atc260x parent device structure. For sub-device used only ! */
extern struct atc260x_dev *atc260x_get_parent_dev(struct device *sub_dev);

extern void atc260x_get_bus_info(struct atc260x_dev *atc260x, uint *bus_num, uint *bus_addr); /* for atc260x-pm only! */
extern uint atc260x_get_ic_type(struct atc260x_dev *atc260x); /* see ATC260X_ICTYPE_2603A ... */
extern uint atc260x_get_ic_ver(struct atc260x_dev *atc260x);  /* see ATC260X_ICVER_A ... */



/* register read/ write ----------------------------------------------------- */

/* ATC260X register interface.  For sub-device used only ! */

/* 注意:
 * 1. 尽量使用atc260x_reg_setbits接口, 这样可以保证寄存器的读-改-写操作是原子的.
 *    单独使用atc260x_reg_read和atc260x_reg_write来实现读-改-写操作, 若寄存器是共用的,
 *    就会有竞争问题.
 * 2. atc260x_reg_setbits 只有内容有更改时才写入, 所以不能用来清pending位(写1清0的位),
 *    再者, 用setbits接口来清pending也会意外将mask之外的别的已经为1的pending位清掉.
 * 3. IC有时会有正常的bit与写1清0的bit混放到一个寄存器里边的情况(设计失误?),
 *    操作这类寄存器需要特别注意, 因为改正常bits的read-modify-write操作会意外地将其它的
 *    已经置1的pending位清掉,
 *    操作这样的寄存器请使用 atc260x_reg_wp_xxx 的接口. */

extern int atc260x_reg_read(struct atc260x_dev *atc260x, uint reg);
extern int atc260x_reg_write(struct atc260x_dev *atc260x, uint reg, u16 val);
extern int atc260x_reg_setbits(struct atc260x_dev *atc260x, uint reg, u16 mask, u16 val);

/* 兼容旧版本的接口. */
static inline int atc260x_set_bits(struct atc260x_dev *atc260x, uint reg, u16 mask, u16 val)
{
	return atc260x_reg_setbits(atc260x, reg, mask, val);
}

/* 读写 "正常的bit与写1清0的bit混放" 的寄存器的接口. */
/* reg_wp_setbits 用于修改其中的"正常"的bits */
static inline int atc260x_reg_wp_setbits(struct atc260x_dev *atc260x,
	uint reg, u16 all_pnd_mask, u16 mask, u16 val)
{
	/* 保证所有pending都写0 */
	return atc260x_reg_setbits(atc260x, reg, (all_pnd_mask | mask), (val & mask & ~all_pnd_mask));
}
/* reg_wp_clrpnd 用于清pending */
static inline int atc260x_reg_wp_clrpnd(struct atc260x_dev *atc260x,
	uint reg, u16 all_pnd_mask, u16 clr_mask)
{
	uint val;
	int ret = atc260x_reg_read(atc260x, reg);
	if (ret < 0)
		return ret;
	val = ((uint)ret & ~all_pnd_mask) | clr_mask;
	return atc260x_reg_write(atc260x, reg, val);
}

/* 切换到direct access模式的接口, for atc260x-pm only! */
extern void atc260x_set_reg_direct_access(struct atc260x_dev *atc260x, bool enable);



/* for module clock & reset ------------------------------------------------- */

/* 模块clock开关 & reset API */

/* DO NOT change the order!!! */
#define ATC260X_CMU_MODULE_NUM           (6) /* CMU module count */
#define ATC260X_CMU_MODULE_TP            (0)
#define ATC260X_CMU_MODULE_MFP           (1)
#define ATC260X_CMU_MODULE_INTS          (2)
#define ATC260X_CMU_MODULE_ETHPHY        (3)
#define ATC260X_CMU_MODULE_AUDIO         (4)
#define ATC260X_CMU_MODULE_PWSI          (5)

/* module reset controll */
extern int atc260x_cmu_reset(struct atc260x_dev *atc260x, uint cmu_module);
/* module clock enable/disable controll */
extern int atc260x_cmu_clk_ctrl(struct atc260x_dev *atc260x, uint cmu_module, uint clk_en);

static inline int atc260x_cmu_enable(struct atc260x_dev *atc260x, uint cmu_module)
{
	return atc260x_cmu_clk_ctrl(atc260x, cmu_module, 1);
}
static inline int atc260x_cmu_disable(struct atc260x_dev *atc260x, uint cmu_module)
{
	return atc260x_cmu_clk_ctrl(atc260x, cmu_module, 0);
}



/* for auxadc --------------------------------------------------------------- */

/* 从名字获得通道号. N个IC兼容的缘故,通道号不是固定的.  返回 <0 表示错误. */
extern int atc260x_auxadc_find_chan(struct atc260x_dev *atc260x, const char *channel_name);
/* 返回通道的名字, 错误时返回NULL */
extern const char *atc260x_auxadc_channel_name(struct atc260x_dev *atc260x, uint channel);
/* 返回通道的计量单位(如"mV"), 错误时返回NULL */
extern const char *atc260x_auxadc_channel_unit_name(struct atc260x_dev *atc260x, uint channel);
/* 获取ADC原始值(从ADC数据寄存器上读到的值), 返回 <0 表示错误. */
extern int atc260x_auxadc_get_raw(struct atc260x_dev *atc260x, uint channel);
/* 获取ADC转换后的值, 返回 0表示OK, 返回 <0 表示错误.
 * 结果放在 *p_tr_value, 是带符号的. 度量一般是标准单位的千分之一, 如BATV的单位是mV. */
extern int atc260x_auxadc_get_translated(struct atc260x_dev *atc260x, uint channel, s32 *p_tr_value);

/* N个IC兼容的缘故, 通道数目&编号不是固定的, 这里的通道号与IC spec上的AuxADC通道编号一致.
 * 各个子设备应该自己判断不同IC对应的通道号,
 * 或在驱动probe时使用atc260x_auxadc_find_chan查找所需通道号, 而不应该继续使用宏定义.
 *
 * 各个IC的可用 channel_name :
 * atc2603a :
 *    "BATV", "BATI", "VBUSV", "VBUSI", "SYSPWRV", "WALLV", "WALLI", "CHGI",
 *    "IREF", "REMCON", "ICTEMP", "BAKBATV", "AUX0", "AUX1", "AUX2", "AUX3"
 * atc2603c :
 *    "BATV", "BATI", "VBUSV", "VBUSI", "SYSPWRV", "WALLV", "WALLI", "CHGI",
 *    "IREF", "REMCON", "ICTEMP", "BAKBATV", "AUX0", "AUX1", "AUX2", "ICM",
 *    "SVCC"
 * atc2609a :
 *    "IREF", "CHGI", "VBUSI", "WALLI", "BATI", "REMCON", "ICTEMP", "SVCC",
 *    "BAKBATV", "SYSPWRV", "WALLV", "VBUSV", "AUX3", "AUX2", "AUX1", "AUX0"
 * */



/* for persistent storage --------------------------------------------------- */

/* 统一管理260x中几个不掉电/不复位的FW用寄存器, 避免过度自由分配造成问题. */
/* 不同的PMIC中各个域的分配位置是不同的, 建议统一使用这套接口, 各自处理会带来不必要的麻烦. */
/* pstore保证在量产后, 其管理的bits初值为0. */
/* 这里的API是给子设备用的, 外部的ko请用 atc260x_ex_ 的版本. */

enum {
	ATC260X_PSTORE_TAG_REBOOT_ADFU = 0,     /* 重启进ADFU标志. */
	ATC260X_PSTORE_TAG_REBOOT_RECOVERY,     /* 重启进recovery标志. */
	ATC260X_PSTORE_TAG_FW_S2,               /* 软件S2标志 */
	ATC260X_PSTORE_TAG_DIS_MCHRG,           /* 重启不进mini_charger */
	ATC260X_PSTORE_TAG_RTC_MSALM,           /* RTC Alarm 备份区, reboot/suspend会用到. */
	ATC260X_PSTORE_TAG_RTC_HALM,            /* RTC Alarm 备份区 */
	ATC260X_PSTORE_TAG_RTC_YMDALM,          /* RTC Alarm 备份区 */
	ATC260X_PSTORE_TAG_GAUGE_CAP,           /* 保存软电量计电量 (8bits) */
	ATC260X_PSTORE_TAG_GAUGE_BAT_RES,       /* 保存软电量计推算的电池内阻 (16bits) */
	ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST,     /* 电量计是否使用ICM功能 (1bit) */
	ATC260X_PSTORE_TAG_GAUGE_SHDWN_TIME,    /* 保存关机时间,电量计内部使用 (31bits) */
	ATC260X_PSTORE_TAG_GAUGE_S2_CONSUMP,    /* 电量计用, 记录S2期间的功耗. (6bits) */
	ATC260X_PSTORE_TAG_GAUGE_CLMT_RESET,    /* 电量计用, coulomb_meter复位标记 (1bit) */
	ATC260X_PSTORE_TAG_RESUME_ADDR,         /* S2 resume address (32bit) */
	ATC260X_PSTORE_TAG_NUM
};

/* write pstore TAG */
extern int atc260x_pstore_set(struct atc260x_dev *atc260x, uint tag, u32 value);
/* read pstore TAG */
extern int atc260x_pstore_get(struct atc260x_dev *atc260x, uint tag, u32 *p_value);
/* clear all pstore TAG (set to zero), call after upgrade process */
extern int atc260x_pstore_reset_all(struct atc260x_dev *atc260x);



/* External API ------------------------------------------------------------- */

/* 提供外部使用的API */

extern int atc260x_ex_auxadc_find_chan(const char *channel_name);
extern int atc260x_ex_auxadc_read(uint channel, s32 *p_tr_value); /* return error code! */
static inline int atc260x_ex_auxadc_read_by_name(const char *channel_name, s32 *p_tr_value) /* return error code! */
{
	int channel = atc260x_ex_auxadc_find_chan(channel_name);
	if (channel < 0)
		return channel;
	return atc260x_ex_auxadc_read(channel, p_tr_value);
}

extern int atc260x_ex_pstore_set(uint tag, u32 value);
extern int atc260x_ex_pstore_get(uint tag, u32 *p_value);



/* misc --------------------------------------------------------------------- */

/* 主要是power那边导出的接口, 临时放置于此. */

int atc260x_enable_vbusotg(int on);

enum {
	DEV_CHARGER_CURRENT_LCD = 0,
	DEV_CHARGER_CURRENT_CAMERA,
	DEV_CHARGER_CURRENT_WIFI,
	DEV_CHARGER_CURRENT_MAX,
};

enum {
	DEV_CHARGER_PRE_CONFIG = 0,
	DEV_CHARGER_POST_CONFIG,
};

extern void config_inner_charger_current(int pre_post, int dev_type, int param);
extern void pmic_charger_set_fun(void (*funp)(int, int, int));


#endif /* __MFD_ATC260X_H__ */
