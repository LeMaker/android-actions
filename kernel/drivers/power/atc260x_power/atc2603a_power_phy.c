/*
 * atc260x_power_2603A_phy.c  --  Power driver for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This file include reg write and read opts mainly.
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <linux/power_supply.h>
#include "atc260x_power.h"

/* ATC2603A_PMU_CHARGER_CTL0 */
#define     PMU_CHARGER_CTL0_CHGAUTO_DETECT_EN      (1 << 0)
#define     PMU_CHARGER_CTL0_CHGPWR_SET_SHIFT       (1)
#define     PMU_CHARGER_CTL0_CHGPWR_SET_MASK        (0x3 << PMU_CHARGER_CTL0_CHGPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHGPWR_SET_100MV       (0 << PMU_CHARGER_CTL0_CHGPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHGPWR_SET_200MV       (1 << PMU_CHARGER_CTL0_CHGPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHGPWR_SET_300MV       (2 << PMU_CHARGER_CTL0_CHGPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHGPWR_SET_400MV       (3 << PMU_CHARGER_CTL0_CHGPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_CURRENT_TEMP       (1 << 3)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR_SET_SHIFT   (4)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR_SET         (0x3 << PMU_CHARGER_CTL0_CHG_SYSPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR_SET_3810MV      (0 << PMU_CHARGER_CTL0_CHG_SYSPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR_SET_3960MV      (1 << PMU_CHARGER_CTL0_CHG_SYSPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR_SET_4250MV      (2 << PMU_CHARGER_CTL0_CHG_SYSPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR_SET_4400MV      (3 << PMU_CHARGER_CTL0_CHG_SYSPWR_SET_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_SYSPWR             (1 << 6)
#define     PMU_CHARGER_CTL0_DTSEL_SHIFT            (7)
#define     PMU_CHARGER_CTL0_DTSEL_MASK             (0x1 << PMU_CHARGER_CTL0_DTSEL_SHIFT)
#define     PMU_CHARGER_CTL0_DTSEL_12MIN            (0 << PMU_CHARGER_CTL0_DTSEL_SHIFT)
#define     PMU_CHARGER_CTL0_DTSEL_20S              (1 << PMU_CHARGER_CTL0_DTSEL_SHIFT)
#define     PMU_CHARGER_CTL0_CHG_FORCE_OFF          (1 << 8)
#define     PMU_CHARGER_CTL0_TRICKLEEN              (1 << 9)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER2_SHIFT    (10)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER2_MASK     (0x3 << PMU_CHARGER_CTL0_CHARGE_TIMER2_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER2_30MIN    (0 << PMU_CHARGER_CTL0_CHARGE_TIMER2_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER2_40MIN    (1 << PMU_CHARGER_CTL0_CHARGE_TIMER2_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER2_50MIN    (2 << PMU_CHARGER_CTL0_CHARGE_TIMER2_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER2_60MIN    (3 << PMU_CHARGER_CTL0_CHARGE_TIMER2_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER1_SHIFT    (12)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER1_MASK     (0x3 << PMU_CHARGER_CTL0_CHARGE_TIMER1_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER1_4H       (0 << PMU_CHARGER_CTL0_CHARGE_TIMER1_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER1_6H       (1 << PMU_CHARGER_CTL0_CHARGE_TIMER1_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER1_8H       (2 << PMU_CHARGER_CTL0_CHARGE_TIMER1_SHIFT)
#define     PMU_CHARGER_CTL0_CHARGE_TIMER1_12H      (3 << PMU_CHARGER_CTL0_CHARGE_TIMER1_SHIFT)
#define     PMU_CHARGER_CTL0_CHGTIME                (1 << 14)
#define     PMU_CHARGER_CTL0_ENCH                   (1 << 15)

/* ATC2603A_PMU_CHARGER_CTL1 */
#define     PMU_CHARGER_CTL1_ICHG_REG_CC_SHIFT      (0)
#define     PMU_CHARGER_CTL1_ICHG_REG_CC_MASK       (0xf << PMU_CHARGER_CTL1_ICHG_REG_CC_SHIFT)
#define     PMU_CHARGER_CTL1_ICHG_REG_CC(i)         \
                    ((((i) / 100) << PMU_CHARGER_CTL1_ICHG_REG_CC_SHIFT) & PMU_CHARGER_CTL1_ICHG_REG_CC_MASK)

#define     PMU_CHARGER_CTL1_BAT_EXIST_EN           (1 << 5)
#define     PMU_CHARGER_CTL1_CURRENT_SOFT_START     (1 << 6)
#define     PMU_CHARGER_CTL1_STOPV_SHIFT            (7)
#define     PMU_CHARGER_CTL1_STOPV_MASK             (0x1 << PMU_CHARGER_CTL1_STOPV_SHIFT)
#define     PMU_CHARGER_CTL1_STOPV_4180MV           (0 << PMU_CHARGER_CTL1_STOPV_SHIFT)
#define     PMU_CHARGER_CTL1_STOPV_4160MV           (1 << PMU_CHARGER_CTL1_STOPV_SHIFT)
#define     PMU_CHARGER_CTL1_CHARGER_TIMER_END      (1 << 8)
#define     PMU_CHARGER_CTL1_BAT_DT_OVER            (1 << 9)
#define     PMU_CHARGER_CTL1_BAT_EXIST              (1 << 10)
#define     PMU_CHARGER_CTL1_CUR_ZERO               (1 << 11)
#define     PMU_CHARGER_CTL1_CHGPWROK               (1 << 12)
#define     PMU_CHARGER_CTL1_PHASE_SHIFT            (13)
#define     PMU_CHARGER_CTL1_PHASE_MASK             (0x3 << PMU_CHARGER_CTL1_PHASE_SHIFT)
#define     PMU_CHARGER_CTL1_PHASE_PRECHARGE        (1 << PMU_CHARGER_CTL1_PHASE_SHIFT)
#define     PMU_CHARGER_CTL1_PHASE_CONSTANT_CURRENT (2 << PMU_CHARGER_CTL1_PHASE_SHIFT)
#define     PMU_CHARGER_CTL1_PHASE_CONSTANT_VOLTAGE (3 << PMU_CHARGER_CTL1_PHASE_SHIFT)
#define     PMU_CHARGER_CTL1_CHGEND                 (1 << 15)

/* ATC2603A_PMU_CHARGER_CTL2 */
#define     PMU_CHARGER_CTL2_ICHG_REG_T_SHIFT       (4) /*jlingzhang*/
#define     PMU_CHARGER_CTL2_ICHG_REG_T_MASK        (0x3 << PMU_CHARGER_CTL2_ICHG_REG_T_SHIFT)
#define     PMU_CHARGER_CTL2_ICHG_REG_T(i)          \
                    ((((i) / 100) << PMU_CHARGER_CTL2_ICHG_REG_T_SHIFT) & PMU_CHARGER_CTL2_ICHG_REG_T_MASK)

#define     PMU_CHARGER_CTL2_CV_SET                 (1 << 6)

#define		PMU_CHARGER_CTL2_TEMPTH1_SHIFT             (13) 
#define		PMU_CHARGER_CTL2_TEMPTH1_MASK 		(0x3 << PMU_CHARGER_CTL2_TEMPTH1_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH1_75			(0 << PMU_CHARGER_CTL2_TEMPTH1_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH1_90			(1 << PMU_CHARGER_CTL2_TEMPTH1_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH1_105			(2 << PMU_CHARGER_CTL2_TEMPTH1_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH1_115			(3 << PMU_CHARGER_CTL2_TEMPTH1_SHIFT)
#define		PMU_CHARGER_CTL2_TEMPTH2_SHIFT             (11) 
#define		PMU_CHARGER_CTL2_TEMPTH2_MASK             (0x3 << PMU_CHARGER_CTL2_TEMPTH2_SHIFT) 
#define 		PMU_CHARGER_CTL2_TEMPTH2_90			(0 << PMU_CHARGER_CTL2_TEMPTH2_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH2_105			(1 << PMU_CHARGER_CTL2_TEMPTH2_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH2_120			(2 << PMU_CHARGER_CTL2_TEMPTH2_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH2_135			(3 << PMU_CHARGER_CTL2_TEMPTH2_SHIFT)
#define		PMU_CHARGER_CTL2_TEMPTH3_SHIFT             (9)
#define		PMU_CHARGER_CTL2_TEMPTH3_MASK		 (0x3 << PMU_CHARGER_CTL2_TEMPTH3_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH3_100			(0 << PMU_CHARGER_CTL2_TEMPTH3_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH3_120			(1 << PMU_CHARGER_CTL2_TEMPTH3_SHIFT)
#define 		PMU_CHARGER_CTL2_TEMPTH3_130			(2 << PMU_CHARGER_CTL2_TEMPTH3_SHIFT)
/* ATC2603A_PMU_SYS_Pending */
#define     PMU_SYS_PENDING_BAT_OV                  (1 << 15)

/* ATC2603A_PMU_OT_CTL */
#define     PMU_OT_CTL_OT                           (1 << 15)
#define 	PMU_OT_CTL_OT_INT_EN 			(1 << 12)
#define 	PMU_OT_CTL_OT_SHUTOFF_EN		(1 << 11)
#define	PMU_OT_CTL_OT_SHUTOFF_SET_SHIFT        (9) 
#define	PMU_OT_CTL_OT_SHUTOFF_SET_MASK        (0x3 << PMU_OT_CTL_OT_SHUTOFF_SET_SHIFT) 
#define	PMU_OT_CTL_OT_SHUTOFF_SET_100        (0 << PMU_OT_CTL_OT_SHUTOFF_SET_SHIFT)
#define	PMU_OT_CTL_OT_SHUTOFF_SET_120        (1 << PMU_OT_CTL_OT_SHUTOFF_SET_SHIFT)
#define	PMU_OT_CTL_OT_SHUTOFF_SET_130        (2 << PMU_OT_CTL_OT_SHUTOFF_SET_SHIFT)
#define	PMU_OT_CTL_OT_SHUTOFF_SET_140        (3 << PMU_OT_CTL_OT_SHUTOFF_SET_SHIFT)
#define 	PMU_OT_CTL_OT_EN					(1 << 8)
 
 /* ATC2603A_APDS_CTL */
#define	VBUS_CONTROL_EN 	(1 << 15)
#define     VBUS_CONTROL_SEL_SHIFT                (14)
#define     VBUS_CONTROL_SEL_MASK                (1 << VBUS_CONTROL_SEL_SHIFT)
#define     VBUS_CONTROL_SEL_VOL                (0x0 << VBUS_CONTROL_SEL_SHIFT)
#define     VBUS_CONTROL_SEL_CUR                (0x1 << VBUS_CONTROL_SEL_SHIFT)
#define	VBUS_CUR_LIMITED_SHIFT		(12)
#define	VBUS_CUR_LIMITED_MASK		(0x3 << VBUS_CUR_LIMITED_SHIFT)
#define	VBUS_CUR_LIMITED_100MA	(0x0 << VBUS_CUR_LIMITED_SHIFT)
#define	VBUS_CUR_LIMITED_300MA	(0x1 << VBUS_CUR_LIMITED_SHIFT)
#define	VBUS_CUR_LIMITED_500MA	(0x2 << VBUS_CUR_LIMITED_SHIFT)
#define	VBUS_CUR_LIMITED_800MA	(0x3 << VBUS_CUR_LIMITED_SHIFT)
#define     APDS_CTL_VBUS_VOL_LIMITED_SHIFT (10)
#define     APDS_CTL_VBUS_VOL_LIMITED_MASK (0x3 << APDS_CTL_VBUS_VOL_LIMITED_SHIFT)
#define     APDS_CTL_VBUSOTG                (1 << 9)
#define     APDS_CTL_VBUS_PD                 (1 << 2)
#define     APDS_CTL_WALL_PD			(1 << 1)
#define     ATC260X_SUPPLY_WALL          (0)
#define     ATC260X_SUPPLY_VBUS          (1)
#define     ATC260X_SUPPLY_BAT             (2)

#define    CV_SET_VAL                             4300
 int atc2603a_read_adc(struct atc260x_dev *atc260x, const char *channel_name)
{
	int ret;
	int translate_data;
	unsigned int channel_num;
	
	channel_num = atc260x_auxadc_find_chan(atc260x,channel_name);
	if(channel_num < 0 )
	{
		printk("not support this channel or the channel name is error!\n");
		return channel_num;
	}
	ret = atc260x_auxadc_get_translated(atc260x,channel_num, &translate_data);
	if(ret < 0)
		printk("cannot get the correct translation data!\n");
	
	return translate_data;
}
 
 int atc2603a_read_adc_supply(struct atc260x_dev *atc260x,
                     const char *channel_name,
                     union power_supply_propval *val)
{
	int translate_data;

	translate_data = atc2603a_read_adc(atc260x,channel_name);
	val->intval = translate_data;
	
	return translate_data;
}

 int atc2603a_check_bat_online(struct atc260x_dev *atc260x)
{
	int ret;
	int exist;

	/* dectect bit 0 > 1 to start dectecting */
	ret = atc260x_set_bits(atc260x, ATC2603A_PMU_CHARGER_CTL1, 
	PMU_CHARGER_CTL1_BAT_EXIST_EN, PMU_CHARGER_CTL1_BAT_EXIST_EN);
	if (ret < 0)
		return ret;

	/* wait bat detect over */
	msleep(ATC260X_BAT_DETECT_DELAY_MS);

	ret = atc260x_reg_read(atc260x, ATC2603A_PMU_CHARGER_CTL1);
	if (ret < 0)
		return ret;

	exist = ret & PMU_CHARGER_CTL1_BAT_EXIST;

	/* cleare battery detect bit, otherwise cannot changer */
	ret = atc260x_set_bits(atc260x, ATC2603A_PMU_CHARGER_CTL1, 
	PMU_CHARGER_CTL1_BAT_EXIST_EN, 0);
	if (ret < 0)
		return ret;

	if (exist) {
		int bat_v = atc2603a_read_adc(atc260x, "BATV");
		printk("%s bat_v:%d\n",__func__, bat_v);
		return 1;
	}

	return 0;
}


int atc2603a_check_wall_online(struct atc260x_charger *charger,  
	union power_supply_propval *val)
{
	int ret;
	
		ret = atc2603a_read_adc_supply(charger->atc260x, "WALLV", val);
		if (ret < 0)
			return ret;


        if ((ret > charger->wall_v_thresh) &&
        	(ret > charger->bat_mv))
		val->intval = 1;
	else
		val->intval = 0;

	return ret;
}

int atc2603a_check_vbus_online(struct atc260x_charger *charger, 
	union power_supply_propval *val)
{
	int ret = 0;
	
	if (charger->g_vbus_is_otg) {
		/* no usb-charger while otg is enabled */
		val->intval = 0;
	} else {
		ret = atc2603a_read_adc_supply(charger->atc260x, "VBUSV", val);
		if (ret < 0)
			return ret;

		if (ret > ATC260X_VBUS_VOLTAGE_THRESHOLD)
			val->intval = 1;
		else
			val->intval = 0;
	}

	return ret;
}

void atc2603a_charger_update_phrase_type(struct atc260x_charger *charger)
{
	int ret = atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1);
	charger->chg_type = ret & PMU_CHARGER_CTL1_PHASE_MASK;
}

void atc2603a_set_charge(struct atc260x_dev *atc260x, int on)
{
	int val;

	if (on) {
		val = PMU_CHARGER_CTL0_ENCH;
	} else {    
		val = 0;
	}
	
	atc260x_set_bits(atc260x, ATC2603A_PMU_CHARGER_CTL0, 
		PMU_CHARGER_CTL0_ENCH, val);
}

/**
 * @brief atc260x_check_bat_status
 * check if battery is healthy
 * @return BATTERY_HEALTHY_STATE
 */
static int atc2603a_bat_check_health_pre(struct atc260x_dev *atc260x)
{
	int batv = atc2603a_read_adc(atc260x, "BATV");
	if (batv <= 200)
	{
		int pmu_charger_ctl1 =  atc260x_reg_read(atc260x, ATC2603A_PMU_CHARGER_CTL1);
		pmu_charger_ctl1 &= ~0xf;
		atc260x_reg_write(atc260x, ATC2603A_PMU_CHARGER_CTL1, pmu_charger_ctl1); 
		atc2603a_set_charge(atc260x, 1);
		msleep(64);
		batv = atc2603a_read_adc(atc260x, "BATV");
		atc2603a_set_charge(atc260x, 0);
		if (batv <= 200)
		{
			return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		} 
	}

	return POWER_SUPPLY_HEALTH_GOOD;
}

/*detect if the bat is OVERVOLTAGE(or too heat)*/
int atc2603a_bat_check_health(struct atc260x_charger *charger, int *health)
{
	struct atc260x_dev *atc260x = charger->atc260x;
	int ret;

	ret = atc260x_reg_read(atc260x, ATC2603A_PMU_SYS_PENDING);
	if (ret < 0)
		return ret;

	if (ret & PMU_SYS_PENDING_BAT_OV) 
	{
		*health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		return 0;
	}

	*health = POWER_SUPPLY_HEALTH_GOOD;

	return 0;
}

void atc2603a_set_trick_current(struct atc260x_charger *charger)
{
	atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2, 
                    atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2) |
                    PMU_CHARGER_CTL2_ICHG_REG_T(charger->thresholds.trickle_charge_current));
	
	 atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
		atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) |
		PMU_CHARGER_CTL0_TRICKLEEN |
		PMU_CHARGER_CTL0_CHARGE_TIMER2_30MIN); 
}

int atc2603a_get_constant_current(struct atc260x_charger *charger)
{
	 return atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1) & 0xf;
}


void atc2603a_set_constant_current(struct atc260x_charger *charger, int set_current)
{
	/* 
	* constant charge current: 1000ma 
	* charge stop voltage (OCV): 4.16v
	*/
	unsigned int current_now = atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1);
	
	if ((current_now & 0xf) != set_current)
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1, 
			(current_now & (~0xf)) | set_current);
	}
	
	 atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
		atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) |
		PMU_CHARGER_CTL0_CHARGE_TIMER1_12H);
}

static void set_vbus_control_mode(struct atc260x_charger *charger, int vbus_control_mode)
{
	if (vbus_control_mode == VOLTAGE_LIMITED)
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
						(atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL) &
						(~VBUS_CONTROL_SEL_MASK )) |
						VBUS_CONTROL_EN |
						VBUS_CONTROL_SEL_VOL);
		power_dbg("vubs control mode:***VOLTAGE_LIMITED***, ATC2603A_PMU_APDS_CTL:%x\n",
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL));
	}
	else if (vbus_control_mode == CURRENT_LIMITED)
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
						(atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL) &
						(~VBUS_CUR_LIMITED_MASK ) &
						(~VBUS_CONTROL_SEL_MASK)) |
						VBUS_CONTROL_EN |
						VBUS_CUR_LIMITED_500MA |
						VBUS_CONTROL_SEL_CUR);
		power_dbg("vubs control mode:***CURRENT_LIMITED***, ATC2603A_PMU_APDS_CTL:%x\n",
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL));
	}
	else
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
						(atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL) &
						(~VBUS_CONTROL_EN)));
	}
}


void atc2603a_set_vbus_path(struct atc260x_charger * charger, bool enable)
{
	if (enable)
	{
		/* connect the path */
		atc260x_set_bits(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
		APDS_CTL_VBUSOTG, 0);
	}
	else 
	{
		/* cut off the path */
		atc260x_set_bits(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
		APDS_CTL_VBUSOTG, APDS_CTL_VBUSOTG);
	}
}

int atc2603a_get_vbus_path(struct atc260x_charger * charger)
{
	/*if vbus is cut,return 0,else return 1 */
	return !(atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL) & APDS_CTL_VBUSOTG);
}

/*5k resistor of wall enable*/
void atc2603a_set_apds_wall_pd(struct atc260x_charger *charger, bool enable)
{
	if (enable)
	{
		atc260x_set_bits(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
			APDS_CTL_WALL_PD, APDS_CTL_WALL_PD);
	}
	else
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL) & ~APDS_CTL_WALL_PD);
	}
}

void atc2603a_set_apds_vbus_pd(struct atc260x_charger *charger, bool enable)
{
	if (enable)
	{
		atc260x_set_bits(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
			APDS_CTL_VBUS_PD, APDS_CTL_VBUS_PD);
	}
	else
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_APDS_CTL, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_APDS_CTL)  & ~APDS_CTL_VBUS_PD);
	}
}


void atc2603a_set_syspwr(struct atc260x_charger *charger)
{
	atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
		atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) |
		PMU_CHARGER_CTL0_CHGPWR_SET_100MV | 
		PMU_CHARGER_CTL0_CHG_SYSPWR_SET_4250MV);

	if (charger->cfg_items.ext_dcdc_exist == 1) 
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) & ~PMU_CHARGER_CTL0_CHG_SYSPWR);  
	}
	else 
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) | PMU_CHARGER_CTL0_CHG_SYSPWR);  
	}
}

void atc2603a_set_current_soft_start(struct atc260x_charger *charger, bool enable)
{
	if (enable)
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1, 
				atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1) |
				PMU_CHARGER_CTL1_CURRENT_SOFT_START );
	}
	else
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1, 
				atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1) &
				(~PMU_CHARGER_CTL1_CURRENT_SOFT_START) );
	}
} 

void atc2603a_cv_set(struct atc260x_charger *charger, int cv_val_mv)
{
	if (cv_val_mv == 4300)
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2, 
	                    atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2) |
	                    PMU_CHARGER_CTL2_CV_SET);
	}
	else 
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2, 
	                    atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2) &
	                    ~PMU_CHARGER_CTL2_CV_SET);
	}
}

void atc2603a_set_ot_shutoff(struct atc260x_charger *charger, int ot_shutoff_enable)
{
	if (!ot_shutoff_enable) //disable over temperature shutoff bit
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_OT_CTL, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_OT_CTL) & ~PMU_OT_CTL_OT_SHUTOFF_EN);
	} 
	else 
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_OT_CTL, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_OT_CTL) | PMU_OT_CTL_OT_SHUTOFF_EN);
	}
}

void atc2603a_set_change_current_temp(struct atc260x_charger *charger, int  change_current_temp)
{
	 if (change_current_temp) {
	      atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
	          atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) | PMU_CHARGER_CTL0_CHG_CURRENT_TEMP);
	 } else {
	     atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
	          atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) & (~PMU_CHARGER_CTL0_CHG_CURRENT_TEMP));
	 }
}

void atc2603a_set_charger_stop_voltage(struct atc260x_charger *charger, int stopv)
{
	atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1, 
			(atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL1) &
			(~ PMU_CHARGER_CTL1_STOPV_MASK)) | 
			stopv);
}



void atc2603a_set_temp_threshold(struct atc260x_charger *charger)
{
	atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2, 
                    (atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL2)  & 
                    (~PMU_CHARGER_CTL2_TEMPTH1_MASK) &
                    (~PMU_CHARGER_CTL2_TEMPTH2_MASK) &
                    (~PMU_CHARGER_CTL2_TEMPTH3_MASK)) |
                    PMU_CHARGER_CTL2_TEMPTH1_90 | PMU_CHARGER_CTL2_TEMPTH2_105 | PMU_CHARGER_CTL2_TEMPTH3_120);
}

void atc2603a_set_auto_detect_charging(struct atc260x_charger *charger, bool enable)
{
	if (!enable)
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0,
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) & 
			~PMU_CHARGER_CTL0_CHGAUTO_DETECT_EN &
			~PMU_CHARGER_CTL0_CHG_FORCE_OFF);
	}
	else 
	{
		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0,
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) | 
			PMU_CHARGER_CTL0_CHGAUTO_DETECT_EN |
			PMU_CHARGER_CTL0_CHG_FORCE_OFF);

		atc260x_reg_write(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0, 
			atc260x_reg_read(charger->atc260x, ATC2603A_PMU_CHARGER_CTL0) |
			PMU_CHARGER_CTL0_DTSEL_12MIN |
			PMU_CHARGER_CTL0_CHGTIME);

	}
}

void atc2603a_charger_phy_init(struct atc260x_charger *charger)
{
	/*
	  * charger init
	  */
	atc2603a_set_trick_current(charger);// trickle charge current: 100mA
	atc2603a_set_constant_current(charger, PMU_CHARGER_CTL1_ICHG_REG_CC(charger->thresholds.constant_charge_current));//set constant current value
	atc2603a_set_current_soft_start(charger, true);
	atc2603a_set_charger_stop_voltage(charger, PMU_CHARGER_CTL1_STOPV_4160MV);
	atc2603a_cv_set(charger, CV_SET_VAL);//enable fast charge
	atc2603a_set_ot_shutoff(charger, charger->cfg_items.ot_shutoff_enable);//shutoff charger when ov according to cfg xml
	 atc2603a_set_temp_threshold(charger);// set temp protect threshold
	atc2603a_set_change_current_temp(charger, charger->cfg_items.change_current_temp);
	atc2603a_set_auto_detect_charging(charger, false);

	/*
	  * syspwr init
	  */
	atc2603a_set_syspwr(charger);

	/*
	  * wall init
	  */
	atc2603a_set_apds_wall_pd(charger, true);
}

void atc2603a_dump_regs(struct atc260x_charger *charger)
{
	int i;
	for (i = 0x00; i <= 0x64; i++ )
	{
		//printk("0x%x:0x%x\n", i, atc260x_reg_read(charger->atc260x, ATC2603A_PMU_BASE + i));
	}
}


void atc2603a_get_info(struct atc260x_charger *charger)
{
	charger->name = "power-2603A";
	//charger->read_adc = atc2603a_read_adc_supply;
	charger->read_adc = atc2603a_read_adc;
	charger->check_bat_online = atc2603a_check_bat_online;
	charger->check_wall_online = atc2603a_check_wall_online;
	charger->check_vbus_online = atc2603a_check_vbus_online;
	charger->update_phrase_type = atc2603a_charger_update_phrase_type;
	charger->update_charger_mode = NULL;
	charger->set_charge = atc2603a_set_charge;
	charger->set_vbus_ctl_mode = set_vbus_control_mode;
	charger->check_health_pre = atc2603a_bat_check_health_pre;
	charger->check_health = atc2603a_bat_check_health;
	charger->set_constant_current = atc2603a_set_constant_current;
	charger->get_constant_current = atc2603a_get_constant_current;
	charger->cv_set = atc2603a_cv_set;
	charger->set_apds_wall_pd = atc2603a_set_apds_wall_pd;
	charger->set_vbus_path = atc2603a_set_vbus_path;
	charger->get_vbus_path = atc2603a_get_vbus_path;
	charger->set_apds_vbus_pd = atc2603a_set_apds_vbus_pd;
	charger->charger_phy_init = atc2603a_charger_phy_init;
	charger->dump_regs = atc2603a_dump_regs;
}
