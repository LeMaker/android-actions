#include <common.h>
#include <asm/arch/owl_afi.h>
#include <asm/arch/owl_timer.h>
#include <asm/arch/pmu.h>
#include <power/owl_power.h>

DECLARE_GLOBAL_DATA_PTR;

static const u16 sc_pmu_regtbl_rtc_ms[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_RTC_MS,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_RTC_MS,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_RTC_MS,
};
static const u16 sc_pmu_regtbl_rtc_h[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_RTC_H,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_RTC_H,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_RTC_H,
};
static const u16 sc_pmu_regtbl_rtc_ymd[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_RTC_YMD,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_RTC_YMD,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_RTC_YMD,
};
static const u16 sc_pmu_regtbl_rtc_msalm[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_RTC_MSALM,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_RTC_MSALM,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_RTC_MSALM,
};
static const u16 sc_pmu_regtbl_rtc_halm[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_RTC_HALM,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_RTC_HALM,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_RTC_HALM,
};
static const u16 sc_pmu_regtbl_rtc_ymdalm[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_RTC_YMDALM,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_RTC_YMDALM,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_RTC_YMDALM,
};
static const u16 sc_pmu_regtbl_sysctl0[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL0,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL0,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL0,
};
static const u16 sc_pmu_regtbl_sysctl1[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL1,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL1,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL1,
};
static const u16 sc_pmu_regtbl_sysctl3[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_SYS_CTL3,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_SYS_CTL3,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_SYS_CTL3,
};
static const u16 sc_pmu_regtbl_uv_status[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_UV_STATUS,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_UV_STATUS,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_UV_STATUS,
};
static const u16 sc_pmu_regtbl_uv_int_en[OWL_PMU_ID_CNT] = {
	[OWL_PMU_ID_ATC2603A] = ATC2603A_PMU_UV_INT_EN,
	[OWL_PMU_ID_ATC2603C] = ATC2603C_PMU_UV_INT_EN,
	[OWL_PMU_ID_ATC2603B] = ATC2609A_PMU_UV_INT_EN,
};

static int leap(int year)
{
	if(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
		return 1;
	else
		return 0;
}

static int month_day(int year, int month)
{
	int day_tab[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if(leap(year) && (month == 2))
		return 28;

	return day_tab[month - 1];
}

static int adjust_time(unsigned short *rtc_ms_value,
	unsigned short *rtc_h_value,
	unsigned short *rtc_ymd_value,
	unsigned short timevalue)
{
	unsigned short min = 0, sec = 0, hour = 0 , day = 0, mon = 0, year = 0;
	unsigned short rtc_ms = 0, rtc_h = 0, rtc_ymd = 0;

	if(!rtc_ms_value || !rtc_h_value || !rtc_ymd_value)
		return -1;

	rtc_ms = *rtc_ms_value;
	rtc_h = *rtc_h_value;
	rtc_ymd = *rtc_ymd_value;

	hour = (rtc_h & 0x1f);
	min = ((rtc_ms & (0x3f << 6)) >> 6);
	sec = (rtc_ms & 0x3f);
	day = (rtc_ymd & 0x1f);
	mon = ((rtc_ymd & (0xf << 5)) >> 5);
	year = ((rtc_ymd & (0x7f << 9)) >> 9);

	sec = sec + timevalue;
	if(sec >= 60) {
		sec = sec - 60;
		min = min + 1;
		if(min >= 60) {
			min = min - 60;
			hour = hour + 1;
			if(hour >= 24) {
				hour = hour - 24;
				day = day + 1;
				if(day >= month_day(year, mon)) {
					day = day - month_day(year, mon);
					mon= mon + 1;
					if(mon >= 12) {
						mon = mon - 12;
						year = year + 1;
					}
				}
			}
		}
	}

	*rtc_ymd_value = (((year & 0x7f) << 9) |
		((mon & 0xf) << 5) | (day & 0x1f));
	*rtc_h_value = (hour & 0x1f);
	*rtc_ms_value = (((min & 0x3f) << 6) | (sec));
	return 0;
}

void reset_cpu(ulong addr)
{
	u16 reg_rtc_ms, reg_rtc_h, reg_rtc_ymd;
	u16 reg_rtc_msalm, reg_rtc_halm, reg_rtc_ymdalm;
	u16 reg_pmu_sysctl0, reg_pmu_sysctl1, reg_pmu_sysctl3;
	int pmu_type, ret =0;
	unsigned short rtc_ms=0, rtc_h=0, rtc_ymd=0;

	int timevalue = 3;

	debug("%s\n", __func__);
	/* get register address */
	pmu_type = OWL_PMU_ID;
	reg_rtc_ms      = sc_pmu_regtbl_rtc_ms[pmu_type];
	reg_rtc_h       = sc_pmu_regtbl_rtc_h[pmu_type];
	reg_rtc_ymd     = sc_pmu_regtbl_rtc_ymd[pmu_type];
	reg_rtc_msalm   = sc_pmu_regtbl_rtc_msalm[pmu_type];
	reg_rtc_halm    = sc_pmu_regtbl_rtc_halm[pmu_type];
	reg_rtc_ymdalm  = sc_pmu_regtbl_rtc_ymdalm[pmu_type];


	rtc_ms = atc260x_reg_read(reg_rtc_ms);
	rtc_h  = atc260x_reg_read(reg_rtc_h);
	rtc_ymd = atc260x_reg_read(reg_rtc_ymd);

	ret = adjust_time(&rtc_ms, &rtc_h, &rtc_ymd, timevalue);

	if(ret != 0) {
		printf("\n########### ERROR at %s %d#########", __FUNCTION__, __LINE__);
		return ;
	}

	atc260x_reg_write(reg_rtc_ymdalm, rtc_ymd);
	atc260x_reg_write(reg_rtc_halm, rtc_h);
	atc260x_reg_write(reg_rtc_msalm, rtc_ms);

	/* SRC_ALARM, S1, S2, S3, are same definition in 2603a/2603c/2609a */
	reg_pmu_sysctl0 = sc_pmu_regtbl_sysctl0[pmu_type];
	reg_pmu_sysctl1 = sc_pmu_regtbl_sysctl1[pmu_type];
	reg_pmu_sysctl3 = sc_pmu_regtbl_sysctl3[pmu_type];
	atc260x_set_bits(reg_pmu_sysctl0,
		(WAKEUP_SRC_ALARM << 5), (WAKEUP_SRC_ALARM << 5));
	atc260x_set_bits(reg_pmu_sysctl3,
		PMU_SYS_CTL3_EN_S2|PMU_SYS_CTL3_EN_S3, PMU_SYS_CTL3_EN_S3);
	atc260x_set_bits(reg_pmu_sysctl1,
		PMU_SYS_CTL1_EN_S1, 0);
	return ;
}

void s_init(void)
{
	timer_init();	
}

