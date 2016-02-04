#include <common.h>
#include <power/boot_power.h>
#include <asm/arch/pmu.h>

#include <libfdt.h>
#include <fdt_support.h>
#include <asm/arch/gmp.h>
#include <asm/io.h>

#include <asm/arch/pwm.h>

#define DEFAULT_CAP 15

struct boot_power_cfg_items {
	int boot_cap_threshold;
	int gauge_bus_id;
	int gauge_id;
};
struct boot_power_cfg_items power_cfg_items;

extern int hw_gauge_get_capacity(int gauge_bus_id);
DECLARE_GLOBAL_DATA_PTR;
/* PMU_CHARGER_CTL1 */
static const u16 pmu_charger_ctl1[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_CHARGER_CTL1,
	[OWL_PMU_ID_ATC2603B] = -1,//ATC2609A_PMU_CHARGER_CTL1,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_CHARGER_CTL1
};
/* PMU_BATVADC */
static const u16 pmu_batvadc[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_BATVADC,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_BATVADC,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_BATVADC
};

/* PMU_WALLVADC */
static const u16 pmu_wallvadc[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_WALLVADC,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_WALLVADC,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_WALLVADC
};
/* PMU_VBUSVADC */
static const u16 pmu_vbusvadc[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_VBUSVADC,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_VBUSVADC,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_VBUSVADC
};
/* PMU_SYS_CTL3 */
static const u16 pmu_sys_ctl3[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL3,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL3,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL3
};
/* PMU_SYS_CTL1 */
static const u16 pmu_sys_ctl1[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL1,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL1,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL1
};
/* PMU_SYS_CTL5 */
static const u16 pmu_sys_ctl5[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL5,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL5,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL5
};
/* PMU_SYS_CTL0 */
static const u16 pmu_sys_ctl0[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL0,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL0,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL0
};
/* PMU_SYS_CTL9 */
static const u16 pmu_sys_ctl9[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL9,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL9,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL9
};

static unsigned int table_bat_percentage[11][2] =
{
	{3420,0},
	{3477,1},
	{3534,2},
	{3591,3},
	{3624,4},
	{3637,5},
	{3649,6},
	{3661,7},
	{3667,8},
	{3673,9},
	{3677,10},
};

/*check whether the batter exits */
int is_battery_exist(void)
{
	int ret =  0;
	int exist;

	/* detect bit 0 -> 1 to start detecting */
	ret = atc260x_set_bits(pmu_charger_ctl1[OWL_PMU_ID],
			PMU_CHARGER_CTL1_BAT_EXIST_EN,
			PMU_CHARGER_CTL1_BAT_EXIST_EN);
	if (ret < 0)
		return ret;

	/* wait bat detect over */
	mdelay(300);

	ret = atc260x_reg_read(pmu_charger_ctl1[OWL_PMU_ID]);//depend on pmu.c
	if (ret < 0)
		return ret;

	exist = ret & PMU_CHARGER_CTL1_BAT_EXIST;

	/* clear battery detect bit, otherwise cannot changer */
	ret = atc260x_set_bits(pmu_charger_ctl1[OWL_PMU_ID],
			PMU_CHARGER_CTL1_BAT_EXIST_EN, 0);
	if (ret < 0)
		return ret;

	if (exist)
		return 1;

	return 0;
}

int get_bat_voltage(int *volt)
{
	int ret =  0;

	/*BATVADC have no enable bit, just read the register*/
	ret = atc260x_reg_read(pmu_batvadc[OWL_PMU_ID]);
	if (ret < 0)
		return ret;

	*volt = (ret & 0x3ff) * 2930 * 2 / 1000;
	debug("----------bat_voltage = %d\n",*volt);
	return 1;

}

/*need to know the capacity to judge whether it is permitted to turn on */
#define SOC_CALC_FACTOR (250*250/1000)
void get_bat_capacity(int *cap,int bat_mv)
{
	int bat_cap = 0;
	int save_cap;
	int i;
	unsigned int pmu_sys_ctl9_data;
	int ocv;
	ocv = bat_mv + SOC_CALC_FACTOR;
	debug("batv=%d,ocv=%d\n",bat_mv,ocv);
	for (i=0; i < 11; i++) 
	{
		if (ocv < table_bat_percentage[i][0]) 
		{
			bat_cap = table_bat_percentage[i][1];
			break;
		}
	}
	if(i == 11)
		bat_cap = DEFAULT_CAP;
		
	/* compare with the one stored in PMU_SYS_CTL9 if it's not the first time turning on*/
	pmu_sys_ctl9_data = atc260x_reg_read(pmu_sys_ctl9[OWL_PMU_ID]);
	if (pmu_sys_ctl9_data & 0x8000) 
	{
		save_cap = (pmu_sys_ctl9_data >> 8) & 0x7f; 
		debug("store cap: %d\n", save_cap);
		if (bat_cap > save_cap)
			bat_cap = save_cap;
	}
	*cap = bat_cap;
	debug("the test battery cap is: %d\n",bat_cap);
	
	return;	
}

int get_charge_plugin_status(int* wall_mv, int* vbus_mv)
{
	int ret;
	int plug_stat = 0; /*default:no plug*/
	ret = atc260x_reg_read(pmu_wallvadc[OWL_PMU_ID]);
	if(ret >= 0)
	{
		ret *= 2930;		/* 2.93mv * 2.5 */
		ret = ret * 2 + ret / 2;
		ret /= 1000;
		*wall_mv = ret;
		if (*wall_mv > 3000)
			plug_stat = WALL_PLUG;
	}
	debug("wall_mv=%d\n",*wall_mv);
	
	ret = atc260x_reg_read(pmu_vbusvadc[OWL_PMU_ID]);
	if(ret >= 0)
	{
		ret *= 2930;            /* 2.93mv * 2.5 */
		ret = ret * 2 + ret / 2;
		ret /= 1000;
		*vbus_mv = ret;
		if (*vbus_mv  > 3000) 
			plug_stat |= USB_PLUG; 
	}
	debug("----------the plug_stat is 0-NO_PLUG,1-WALL_PLUG,2-USB_PLUG:%d\n",plug_stat);
	return plug_stat;
}

int get_adaptor_type(int *type)
{
	char *node_path[OWL_PMU_ID_CNT] = {"/spi@b0204000/atc2603a@00/atc260x-power"," ","/i2c@b0170000/atc2603c@65/atc260x-power"};
	const void *blob = gd->fdt_blob;	
	int node;
	
	debug("node_path[%d]=%s\n",OWL_PMU_ID,node_path[OWL_PMU_ID]);	
	node = fdt_path_offset(blob, node_path[OWL_PMU_ID]);
	*type = fdtdec_get_int(blob, node, "support_adaptor_type", UNKNOWN);
	switch(*type)
	{
		case     DCIN:    debug("support type:only DCIN\n");return 1;break;
		case      USB:    debug("support type:only USB\n");return 1;break;
		case DCIN_USB:    debug("support type: both DCIN and USB\n");return 1;break;
		default:		  debug("cannot find support type!!\n");return 0;
	}
}

/* modifed @2015-10-13
 *in uboot .dts file, add gauge_id¡¢gauge_bus_id¡¢boot_cap_threshold three configuration items
 */		 
void get_cfg_items(void)
{
	char *node_path[OWL_PMU_ID_CNT] = {"/spi@b0204000/atc2603a@00/atc260x-power"," ","/i2c@b0170000/atc2603c@65/atc260x-power"};
	const void *blob = gd->fdt_blob;
	int node;
	
	node = fdt_path_offset(blob, node_path[OWL_PMU_ID]);
	power_cfg_items.gauge_id = fdtdec_get_int(blob, node, "gauge_id", 0);	
	power_cfg_items.gauge_bus_id  = fdtdec_get_int(blob, node, "gauge_bus_id", 2);	
	power_cfg_items.boot_cap_threshold  = fdtdec_get_int(blob, node, "boot_cap_threshold", 7);	
}

bool support_minicharger()
{
	char *node_path[OWL_PMU_ID_CNT] = {"/spi@b0204000/atc2603a@00/atc260x-power"," ","/i2c@b0170000/atc2603c@65/atc260x-power"};
	const void *blob = gd->fdt_blob;
	int node = fdt_path_offset(blob, node_path[OWL_PMU_ID]);
	int cfg = fdtdec_get_int(blob, node, "support_minicharger", 1);
	debug("support_minicharger:%d\n",cfg);
	return cfg;
}
 void atc260x_shutoff(void)
 {
	atc260x_set_bits(
		pmu_sys_ctl3[OWL_PMU_ID],
		PMU_SYS_CTL3_EN_S2 | PMU_SYS_CTL3_EN_S3,
		0);
	atc260x_set_bits(pmu_sys_ctl1[OWL_PMU_ID], PMU_SYS_CTL1_EN_S1, 0);
	return;
}

void check_power(void)
{
	int support_adaptor_type;
	int charge_plugin_status;
	int wall_mv,vbus_mv,bat_mv;
	int low_power_boot_choice = 0;
	int cap;
	
	gd->flags &= 0x3ffff;
	
	get_cfg_items();
	charge_plugin_status = get_charge_plugin_status(&wall_mv, &vbus_mv);
	
	if(get_bat_voltage(&bat_mv))
		debug("bat_mv:%d\n",bat_mv);

	if(get_adaptor_type(&support_adaptor_type))
	{
		if(support_adaptor_type == DCIN)
		{
			atc260x_set_bits(pmu_sys_ctl5[OWL_PMU_ID], (1<<8), 0); /* disable VBUS wake */	
			mdelay(10);
			debug("only support DCIN type,disable USB awake\n");
			atc260x_set_bits(pmu_sys_ctl0[OWL_PMU_ID], (1<<15), 0); 
			mdelay(10);			
		}
	}
	
	if(charge_plugin_status != NO_PLUG)
	{
		low_power_boot_choice = atc260x_pstore_get_noerr(ATC260X_PSTORE_TAG_DIS_MCHRG);
		debug("low_power_boot_choice is %d \n",low_power_boot_choice);
		if(low_power_boot_choice != 0) {
			/* someone request NOT TO enter mini-charger JUST THIS TIME, */
			/* so, clear the flag, and exit. */
			atc260x_pstore_set(ATC260X_PSTORE_TAG_DIS_MCHRG, 0);
			return;
		}
		
		if(!support_minicharger())
			return;
		
		if(is_battery_exist() <= 0)/*if there is plug,but battery don't exist,boot normally*/
			return;
		/* modifed @2014-12-15
		 * if battery exists && batv<3.3,we do not turn on backlight with usb plugging
		 */
		 if (charge_plugin_status == USB_PLUG)
		 {
			if (support_adaptor_type == DCIN)
			{
				debug("only support DCIN,USB do not support charging\n");
				return;
			}

			if(bat_mv < 3300)
			{
				printf("low power,do not open backlight\n");
				gd->flags |= GD_FLG_BL_LOWPOWER;
			}	
		 } 
					
		gd->flags |= GD_FLG_CHARGER ;    /*go to minicharger */
		if(gd->flags & 0x40000)
			debug("========NOW go to the minicharger=========\n");
		return;
	}
	else   /* if there is no plug,then detect whether it is low power/capacity */
	{
		if(bat_mv < 3700) /* it may be low power */
		{
			if(is_battery_exist() <= 0)
			{
				printf("battery may not exist,please check the battery!");
				return;
			}
			/*added by cxj @2014-11-08 */
			if(bat_mv < 3300)
			{
				atc260x_shutoff();
				return;
			}
			
			/*
				change by tuhm @2015-03-03 
				gauge_id: 0 default gague;1 hw_gauge
			*/
			if(power_cfg_items.gauge_id == 1){
				cap = hw_gauge_get_capacity(power_cfg_items.gauge_bus_id);
				printf("[HW_GUAGE] low power capacity :%d \n",cap);
			}else{
				get_bat_capacity(&cap,bat_mv);
				printf("low power cap :%d\n",cap);
			}

			if(cap < power_cfg_items.boot_cap_threshold)
			{
				/* someone may request NOT TO enter mini-charger THIS TIME. */
				/* although this can not be fulfilled, the flag should be clear. */
				atc260x_pstore_set(ATC260X_PSTORE_TAG_DIS_MCHRG, 0);

				/*show the picture of low power ,then shutdown*/
				gd->flags |= GD_FLG_LOWPOWER;
				if(gd->flags & 0x80000 == 0x80000)
					printf("low power,ready to shut down power...\n");
			}
		}
		/* if the battery capacity isn't too low,boot normally with battery */
		return;
	}

}
