/*
* pmu.h - OWL PMIC driver
*
* Copyright (C) 2012, Actions Semiconductor Co. LTD.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published
* by the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/
#ifndef __ASM_ARM_ARCH_PMU_H__
#define __ASM_ARM_ARCH_PMU_H__

//#include <afinfo.h>
#include <asm/arch/regs_map_atc2603a.h>
#include <asm/arch/regs_map_atc2603c.h>
#include <asm/arch/regs_map_atc2609a.h>
#include <i2c.h>
extern struct spi_slave g_atc260x_spi_slave;


typedef struct
{
    /* offs: 0x0 */
	unsigned char shift;
    /* offs: 0x1 */
	unsigned char mask;
    /* offs: 0x2 */
	unsigned char val;
    /* offs: 0x3 */
	unsigned char no;
} __attribute__ ((packed)) mfp_t;

typedef struct
{
    /* offs: 0x0 */
	unsigned long pwm_val;
    /* offs: 0x4 */
	mfp_t	mfp;
} __attribute__ ((packed)) pwm_config_t;

struct owl_pmu_config {
		unsigned int	bus_id;
		unsigned int	dcdc_en_bm;
		unsigned int dcdc_cfgs[7];
		unsigned int	ldo_en_bm;
		unsigned int ldo_cfgs[12];
		unsigned int sgpio_out_en;
		unsigned int sgpio_in_en;
		unsigned int sgpio_out;
		pwm_config_t  pwm_config[3];
		int i2c_config;
		//cpu_pwm_volt_table  cpu_pwm_volt_tb[];
		//int ic_type;
};

/* PMU 类型 */
#define OWL_PMU_ID_ATC2603A    0
#define OWL_PMU_ID_ATC2603B    1
#define OWL_PMU_ID_ATC2603C    2
#define OWL_PMU_ID_CNT		3

/* 根据板型配置得出PMU类型的定义, 代码中请统一使用宏OWL_PMU_ID */
#if defined(CONFIG_ATC2603A)
#define OWL_PMU_ID   OWL_PMU_ID_ATC2603A
#elif defined(CONFIG_ATC2603C)
#define OWL_PMU_ID   OWL_PMU_ID_ATC2603C
#elif defined(CONFIG_ATC2603B)
#define OWL_PMU_ID   OWL_PMU_ID_ATC2603B
#else
/* 原本想从afinfo那边拿的, 但是那样会使SPL变大超限, 故还是先定死. */
#define OWL_PMU_ID   OWL_PMU_ID_ATC2603C /*(afinfo->pmu_id)*/
#endif


extern int pmu_init(const void *blob);
extern void pmu_prepare_for_s2(void);
extern void vdd_cpu_voltage_scan(void);
extern void vdd_cpu_voltage_store(void);
extern void set_ddr_voltage(int add_flag);

extern int atc260x_reg_read(unsigned short reg);
extern int atc260x_reg_write(unsigned short reg , unsigned short value);
extern int atc260x_set_bits(unsigned int reg, unsigned short mask, unsigned short val);
extern int atc260x_get_version(void);

/* for persistent storage */
/* 统一管理260x中几个不掉电/不复位的FW用寄存器, 避免过度自由分配造成问题. */
/* 不同的PMIC中各个域的分配位置是不同的, 建议统一使用这套接口, 各自处理会带来不必要的麻烦. */
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
	ATC260X_PSTORE_TAG_RESUME_ADDR,         /* S2 resume address (low 32bit) */
	ATC260X_PSTORE_TAG_NUM
};
extern int atc260x_pstore_set(uint tag, u32 value);
extern int atc260x_pstore_get(uint tag, u32 *p_value);
extern ulong atc260x_pstore_get_noerr(uint tag);

extern int s2_resume;
extern void (*cpu_resume_fn)(void);
extern int alarm_wakeup;

#endif
