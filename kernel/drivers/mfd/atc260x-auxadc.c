/* UTF-8 encoded. */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/mfd/core.h>
#include <linux/mfd/atc260x/atc260x.h>

#include "atc260x-core.h"


/* 为了兼容N颗IC, 这里使用了比较繁琐的数据结构. */

#define ATC260x_AUXADC_MAX_CHNL 20


struct atc260x_auxadc_chn_hwinfo;
typedef int (*atc260x_auxadc_getraw_func_t)(uint channel, const struct atc260x_auxadc_chn_hwinfo *ch_info, struct atc260x_dev *atc260x);
typedef int (*atc260x_auxadc_translate_func_t)(uint raw_val, const struct atc260x_auxadc_chn_hwinfo *ch_info, struct atc260x_dev *atc260x);

struct atc260x_auxadc_chn_hwinfo {
	u16     dat_reg_addr;
	u8      dat_bit_offs;
	u8      dat_bit_cnt;

	/* get raw value function, if the above setting is not useful. */
	atc260x_auxadc_getraw_func_t getraw_func;

	/* translate function, for complex transformation */
	atc260x_auxadc_translate_func_t tran_func;

	/* simple linear transformation, fall-back method. */
	/* y = ax + b,  a=tr_multiplier/(1<<tr_r_shift), b=tr_offset */
	s32     tr_offset;
	s32     tr_multiplier;
	u8      tr_r_shift;
};

struct atc260x_auxadc_hwinfo {
	uint            channels;
	const char      *chn_names[ATC260x_AUXADC_MAX_CHNL];
	const char      *chn_unit_names[ATC260x_AUXADC_MAX_CHNL]; /* 度量单位. */
	const struct atc260x_auxadc_chn_hwinfo *chn_hwinfos[ATC260x_AUXADC_MAX_CHNL];
};


/* for atc2603a ------------------------------------------------------------- */
/* no postfix means rev.D */
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_batv = {
	.dat_reg_addr    = ATC2603A_PMU_BATVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv * 2 */
	.tr_r_shift      = 6,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_bati = {
	.dat_reg_addr    = ATC2603A_PMU_BATIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500, /* raw * 1500 / 1024 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_vbusv = {
	.dat_reg_addr    = ATC2603A_PMU_VBUSVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1875, /* raw * 2.9296875mv * 2.5 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_vbusi = {
	.dat_reg_addr    = ATC2603A_PMU_VBUSIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500, /* raw * 1500 / 1024 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_syspwrv = {
	.dat_reg_addr    = ATC2603A_PMU_SYSPWRADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1875, /* raw * 2.9296875mv * 2.5 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_wallv = {
	.dat_reg_addr    = ATC2603A_PMU_WALLVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1875, /* raw * 2.9296875mv * 2.5 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_walli = {
	.dat_reg_addr    = ATC2603A_PMU_WALLIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500, /* raw * 1500 / 1024 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_chgi = {
	.dat_reg_addr    = ATC2603A_PMU_CHGIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500, /* raw * 1500 / 1024 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_iref = {
	.dat_reg_addr    = ATC2603A_PMU_IREFADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv */
	.tr_r_shift      = 7,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_remcon = {
	.dat_reg_addr    = ATC2603A_PMU_REMCONADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_ictemp = {
	.dat_reg_addr    = ATC2603A_PMU_ICTEMPADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 399155,   /* (raw_val * 194.9) - 14899 - 30000 (mC) */
	.tr_r_shift      = 11,
	.tr_offset       = -14899 -30000,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_bakbatv = {
	.dat_reg_addr    = ATC2603A_PMU_BAKBATADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv */
	.tr_r_shift      = 7,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_aux0 = {
	.dat_reg_addr    = ATC2603A_PMU_AUXADC0,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_aux1 = {
	.dat_reg_addr    = ATC2603A_PMU_AUXADC1,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_aux2 = {
	.dat_reg_addr    = ATC2603A_PMU_AUXADC2,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603a_chn_hwinfos_aux3 = {
	.dat_reg_addr    = ATC2603A_PMU_AUXADC3,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_hwinfo sc_atc2603a_auxadc_hwinfo = {
	.channels = 16,
	.chn_names = {
		"BATV", "BATI", "VBUSV", "VBUSI", "SYSPWRV", "WALLV", "WALLI", "CHGI",
		"IREF", "REMCON", "ICTEMP", "BAKBATV", "AUX0", "AUX1", "AUX2", "AUX3"
	},
	.chn_unit_names = {
		"mV", "mA", "mV", "mA", "mV", "mV", "mA", "mA",
		"mV", "/1024", "mCel", "mV", "/1024", "/1024", "/1024", "/1024"
	},
	.chn_hwinfos = {
		&sc_atc2603a_chn_hwinfos_batv,
		&sc_atc2603a_chn_hwinfos_bati,
		&sc_atc2603a_chn_hwinfos_vbusv,
		&sc_atc2603a_chn_hwinfos_vbusi,
		&sc_atc2603a_chn_hwinfos_syspwrv,
		&sc_atc2603a_chn_hwinfos_wallv,
		&sc_atc2603a_chn_hwinfos_walli,
		&sc_atc2603a_chn_hwinfos_chgi,
		&sc_atc2603a_chn_hwinfos_iref,
		&sc_atc2603a_chn_hwinfos_remcon,
		&sc_atc2603a_chn_hwinfos_ictemp,
		&sc_atc2603a_chn_hwinfos_bakbatv,
		&sc_atc2603a_chn_hwinfos_aux0,
		&sc_atc2603a_chn_hwinfos_aux1,
		&sc_atc2603a_chn_hwinfos_aux2,
		&sc_atc2603a_chn_hwinfos_aux3
	}
};


/* for atc2603c ------------------------------------------------------------- */
/* no postfix means rev.A */

/* software fix for gl5307.
 *
 0.  CHGI = ADC_DBG0 * 1546 / I_ref(max)
 1.  VBUSI = ADC_DBG1 *1509 / I_ref(max)
 2.  WALLI = ADC_DBG2 * 1527 / I_ref(max)
 3.  BATI = ADC_DBG3 * 1546 / I_ref(max)
 4.  REMCON = ADC_DBG4 * 1024 / 2*SVCC(max)
 */

struct atc260x_auxadc_udata_3c {
	uint    dbg_ref_valid;
	uint    adc_iref_max;       /* for DBG channels */
	uint    adc_svcc_min;       /* for DBG channels */
	uint    icm_res_10ohm;      /* ICM sample resistor is 10mOhm */
};

static int _atc2603c_auxadc_getraw_icm_chn(
		uint channel, const struct atc260x_auxadc_chn_hwinfo *ch_info,
		struct atc260x_dev *atc260x)
{
	uint reg_val;
	int ret;

	/* the channel is already enabled. */
	while (1) {
		ret = atc260x_reg_read(atc260x, ATC2603C_PMU_ICMADC);
		if (ret < 0) {
			return ret;
		}
		reg_val = ret;
		if (reg_val & (1U << 11)) { /* CM_DATAOK ? */
			break;
		}
		udelay(200);
	}
	return reg_val & ((1U << 11) -1U);
}
static void _atc2603c_auxadc_usertran_prepare_dbg_refs(struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_3c *pud;
	uint i, tmp, iref_max, svcc_min;
	int ret;
	pud = (struct atc260x_auxadc_udata_3c *)(atc260x->auxadc_udata);
	if (! pud->dbg_ref_valid) {
		/*get max current reference value for later current value adjusting.*/
		iref_max = 0;
		svcc_min = (typeof(svcc_min)) -1;
		for (i = 0; i < 10; i++){
			ret = atc260x_auxadc_get_raw(atc260x, 8); /* channel #8, IREF */
			if (ret < 0)
				goto label_err;
			tmp = 0xFFFFU & ret;
			if (tmp > iref_max)
				iref_max = tmp;
			ret = atc260x_auxadc_get_raw(atc260x, 16); /* channel #16, SVCC */
			if (ret < 0)
				goto label_err;
			tmp = 0xFFFFU & ret;
			if (tmp < svcc_min)
				svcc_min = tmp;
			msleep(1);
		}
		pud->adc_iref_max = iref_max;
		pud->adc_svcc_min = svcc_min;
		pud->dbg_ref_valid = 1;
	}
	return;
	label_err:
	dev_err(atc260x->dev,
		"%s() failed to get raw value of ch-8 and ch-17\n", __func__);
}
static int _atc2603c_auxadc_usertran_dbg_i_mode(
		uint raw_val, const struct atc260x_auxadc_chn_hwinfo *ch_info, struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_3c *pud;
	uint adc_iref_max;

	_atc2603c_auxadc_usertran_prepare_dbg_refs(atc260x);

	pud = (struct atc260x_auxadc_udata_3c *)(atc260x->auxadc_udata);
	adc_iref_max = pud->adc_iref_max;
	if (adc_iref_max == 0)
		adc_iref_max = 1;
	return raw_val * ch_info->tr_multiplier / adc_iref_max;
}
static int _atc2603c_auxadc_usertran_dbg_rem_mode(
		uint raw_val, const struct atc260x_auxadc_chn_hwinfo *ch_info, struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_3c *pud;
	uint adc_svcc_min;

	_atc2603c_auxadc_usertran_prepare_dbg_refs(atc260x);

	/* remcon_uv = ADC_DBG4*(1024/2)*SVCCADC(小) , 单位是uV */
	/* remcon_ratio = (remcon_uv * 1024) / svcc_uv
	 * remcon_ratio = (remcon_uv *1024) / (SVCCADC * 2929.6875 * 2 )
	 * remcon_ratio = ADC_DBG4*512*512 / 2929.6875
	 * remcon_ratio ~= (ADC_DBG4 * 46912496) >> 19   (SVCCADC??)
	 */

	pud = (struct atc260x_auxadc_udata_3c *)(atc260x->auxadc_udata);
	adc_svcc_min = pud->adc_svcc_min;
	if (adc_svcc_min == 0)
		adc_svcc_min = 1;
	return raw_val * 512U / adc_svcc_min;
}
static int _atc2603c_auxadc_usertran_icm(
		uint raw_val, const struct atc260x_auxadc_chn_hwinfo *ch_info, struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_3c *pud;
	int tmp;
	pud = (struct atc260x_auxadc_udata_3c *)(atc260x->auxadc_udata);
	tmp = raw_val & 0x3ffU;
	if (pud->icm_res_10ohm == 0) {
		tmp = (tmp * 9375U) >> 12;   /*tmp = tmp * 2343.75U / 1024U; */
	} else {
		tmp = (tmp * 9375U) >> 11;  /*tmp = tmp * 4687.5U / 1024U; */
	}
	if (raw_val & 0x400U) {
		tmp = -tmp;
		/*TODO: "+1step effect" in minus direction. */
	}
	return tmp;
}

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_batv = {
	.dat_reg_addr    = ATC2603C_PMU_BATVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv * 2 */
	.tr_r_shift      = 6,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_a_chn_hwinfos_bati = {
	.dat_reg_addr    = ATC2603C_PMU_ADC_DBG3,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1546,
	.tran_func       = _atc2603c_auxadc_usertran_dbg_i_mode,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_b_chn_hwinfos_bati = {
	.dat_reg_addr    = ATC2603C_PMU_BATIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500,
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_vbusv = {
	.dat_reg_addr    = ATC2603C_PMU_VBUSVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1875, /* raw * 2.9296875mv * 2.5 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_a_chn_hwinfos_vbusi = {
	.dat_reg_addr    = ATC2603C_PMU_ADC_DBG1,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1509,
	.tran_func       = _atc2603c_auxadc_usertran_dbg_i_mode,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_b_chn_hwinfos_vbusi = {
	.dat_reg_addr    = ATC2603C_PMU_VBUSIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500,
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_syspwrv = {
	.dat_reg_addr    = ATC2603C_PMU_SYSPWRADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1875, /* raw * 2.9296875mv * 2.5 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_wallv = {
	.dat_reg_addr    = ATC2603C_PMU_WALLVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1875, /* raw * 2.9296875mv * 2.5 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_a_chn_hwinfos_walli = {
	.dat_reg_addr    = ATC2603C_PMU_WALLIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500,
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_b_chn_hwinfos_walli = {
	.dat_reg_addr    = ATC2603C_PMU_ADC_DBG2,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1500,
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_a_chn_hwinfos_chgi = {
	.dat_reg_addr    = ATC2603C_PMU_ADC_DBG0,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1546,
	.tran_func       = _atc2603c_auxadc_usertran_dbg_i_mode,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_ver_b_chn_hwinfos_chgi = {
	.dat_reg_addr    = ATC2603C_PMU_CHGIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 2000,
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};

static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_iref = {
	.dat_reg_addr    = ATC2603C_PMU_IREFADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv */
	.tr_r_shift      = 7,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_remcon = {
	.dat_reg_addr    = ATC2603C_PMU_ADC_DBG4,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tran_func       = _atc2603c_auxadc_usertran_dbg_rem_mode, /* output raw value without BUG */
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_ictemp = {
	.dat_reg_addr    = ATC2603C_PMU_ICTEMPADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 399155,   /* (raw_val * 194.9) - 14899 - 30000 (mC) */
	.tr_r_shift      = 11,
	.tr_offset       = -14899 -30000,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_bakbatv = {
	.dat_reg_addr    = ATC2603C_PMU_BAKBATADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv */
	.tr_r_shift      = 7,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_aux0 = {
	.dat_reg_addr    = ATC2603C_PMU_AUXADC0,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_aux1 = {
	.dat_reg_addr    = ATC2603C_PMU_AUXADC1,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_aux2 = {
	.dat_reg_addr    = ATC2603C_PMU_AUXADC2,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 1,   /* raw value (0~1023) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_icm = {
	.dat_reg_addr    = ATC2603C_PMU_ICMADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 11,
	.getraw_func     = _atc2603c_auxadc_getraw_icm_chn,
	.tran_func       = _atc2603c_auxadc_usertran_icm,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2603c_chn_hwinfos_svccv = {
	.dat_reg_addr    = ATC2603C_PMU_SVCCADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 10,
	.tr_multiplier   = 375, /* raw * 2.9296875mv * 2 */
	.tr_r_shift      = 6,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_hwinfo sc_atc2603c_ver_a_auxadc_hwinfo = {
	.channels = 17,
	.chn_names = {
		"BATV", "BATI", "VBUSV", "VBUSI", "SYSPWRV", "WALLV", "WALLI", "CHGI",
		"IREF", "REMCON", "ICTEMP", "BAKBATV", "AUX0", "AUX1", "AUX2", "ICM",
		"SVCC"
	},
	.chn_unit_names = {
		"mV", "mA", "mV", "mA", "mV", "mV", "mA", "mA",
		"mV", "/1024", "mCel", "/1024", "/1024", "/1024", "mV", "mA",
		"mV"
	},
	.chn_hwinfos = {
		&sc_atc2603c_chn_hwinfos_batv,
		&sc_atc2603c_ver_a_chn_hwinfos_bati,
		&sc_atc2603c_chn_hwinfos_vbusv,
		&sc_atc2603c_ver_a_chn_hwinfos_vbusi,
		&sc_atc2603c_chn_hwinfos_syspwrv,
		&sc_atc2603c_chn_hwinfos_wallv,
		&sc_atc2603c_ver_a_chn_hwinfos_walli,
		&sc_atc2603c_ver_a_chn_hwinfos_chgi,
		&sc_atc2603c_chn_hwinfos_iref,
		&sc_atc2603c_chn_hwinfos_remcon,
		&sc_atc2603c_chn_hwinfos_ictemp,
		&sc_atc2603c_chn_hwinfos_bakbatv,
		&sc_atc2603c_chn_hwinfos_aux0,
		&sc_atc2603c_chn_hwinfos_aux1,
		&sc_atc2603c_chn_hwinfos_aux2,
		&sc_atc2603c_chn_hwinfos_icm,
		&sc_atc2603c_chn_hwinfos_svccv
	}
};

static const struct atc260x_auxadc_hwinfo sc_atc2603c_ver_b_auxadc_hwinfo = {
	.channels = 17,
	.chn_names = {
		"BATV", "BATI", "VBUSV", "VBUSI", "SYSPWRV", "WALLV", "WALLI", "CHGI",
		"IREF", "REMCON", "ICTEMP", "BAKBATV", "AUX0", "AUX1", "AUX2", "ICM",
		"SVCC"
	},
	.chn_unit_names = {
		"mV", "mA", "mV", "mA", "mV", "mV", "mA", "mA",
		"mV", "/1024", "mCel", "/1024", "/1024", "/1024", "mV", "mA",
		"mV"
	},
	.chn_hwinfos = {
		&sc_atc2603c_chn_hwinfos_batv,
		&sc_atc2603c_ver_b_chn_hwinfos_bati,
		&sc_atc2603c_chn_hwinfos_vbusv,
		&sc_atc2603c_ver_b_chn_hwinfos_vbusi,
		&sc_atc2603c_chn_hwinfos_syspwrv,
		&sc_atc2603c_chn_hwinfos_wallv,
		&sc_atc2603c_ver_b_chn_hwinfos_walli,
		&sc_atc2603c_ver_b_chn_hwinfos_chgi,
		&sc_atc2603c_chn_hwinfos_iref,
		&sc_atc2603c_chn_hwinfos_remcon,
		&sc_atc2603c_chn_hwinfos_ictemp,
		&sc_atc2603c_chn_hwinfos_bakbatv,
		&sc_atc2603c_chn_hwinfos_aux0,
		&sc_atc2603c_chn_hwinfos_aux1,
		&sc_atc2603c_chn_hwinfos_aux2,
		&sc_atc2603c_chn_hwinfos_icm,
		&sc_atc2603c_chn_hwinfos_svccv
	}
};


/* for atc2609a ------------------------------------------------------------- */
/* no postfix means rev.D */

struct atc260x_auxadc_udata_9a {
	int    adc_ictemp_adj;
};

static int _atc2609a_auxadc_usertran_ictemp_adj(
		uint raw_val, const struct atc260x_auxadc_chn_hwinfo *ch_info, struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_9a *pud;
	pud = (struct atc260x_auxadc_udata_9a *)(atc260x->auxadc_udata);
	return  51 * ((int)raw_val + pud->adc_ictemp_adj - 1333);
}

static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_iref = {
	.dat_reg_addr    = ATC2609A_PMU_IREFADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 375, /* raw * 0.732421875mv */
	.tr_r_shift      = 9,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_chgi = {
	.dat_reg_addr    = ATC2609A_PMU_CHGIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 4000, /* raw * 4000 / 4096 */
	.tr_r_shift      = 12,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_vbusi = {
	.dat_reg_addr    = ATC2609A_PMU_VBUSIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 4000, /* raw * 4000 / 4096 */
	.tr_r_shift      = 12,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_walli = {
	.dat_reg_addr    = ATC2609A_PMU_WALLIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 4000, /* raw * 4000 / 4096 */
	.tr_r_shift      = 12,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_bati = {
	.dat_reg_addr    = ATC2609A_PMU_BATIADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 4000, /* raw * 4000 / 4096 */
	.tr_r_shift      = 12,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_remcon = {
	.dat_reg_addr    = ATC2609A_PMU_REMCONADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1,   /* raw value (0~4095) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_ictemp = {
	.dat_reg_addr    = ATC2609A_PMU_ICTEMPADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tran_func       = _atc2609a_auxadc_usertran_ictemp_adj,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_svccv = {
	.dat_reg_addr    = ATC2609A_PMU_SVCCADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 375, /* raw * 0.732421875mv * 2 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_bakbatv = {
	.dat_reg_addr    = ATC2609A_PMU_BAKBATADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 375, /* raw * 0.732421875mv * 2 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_syspwrv = {
	.dat_reg_addr    = ATC2609A_PMU_SYSPWRADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1875, /* raw * 0.732421875mv * 2.5 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_wallv = {
	.dat_reg_addr    = ATC2609A_PMU_WALLVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1875, /* raw * 0.732421875mv * 2.5 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_vbusv = {
	.dat_reg_addr    = ATC2609A_PMU_VBUSVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1875, /* raw * 0.732421875mv * 2.5 */
	.tr_r_shift      = 10,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_aux3 = {
	.dat_reg_addr    = ATC2609A_PMU_AUXADC3,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1,   /* raw value (0~4095) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_aux2 = {
	.dat_reg_addr    = ATC2609A_PMU_AUXADC2,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1,   /* raw value (0~4095) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_aux1 = {
	.dat_reg_addr    = ATC2609A_PMU_AUXADC1,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1,   /* raw value (0~4095) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_aux0 = {
	.dat_reg_addr    = ATC2609A_PMU_AUXADC0,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 1,   /* raw value (0~4095) without BUG */
	.tr_r_shift      = 0,
	.tr_offset       = 0,
};
static const struct atc260x_auxadc_chn_hwinfo sc_atc2609a_chn_hwinfos_batv = {
	.dat_reg_addr    = ATC2609A_PMU_BATVADC,
	.dat_bit_offs    = 0,
	.dat_bit_cnt     = 12,
	.tr_multiplier   = 375, /* raw * 0.732421875mv * 2 */
	.tr_r_shift      = 8,
	.tr_offset       = 0,
};

static const struct atc260x_auxadc_hwinfo sc_atc2609a_auxadc_hwinfo = {
	.channels = 17,
	.chn_names = {
		"IREF", "CHGI", "VBUSI", "WALLI", "BATI", "REMCON", "ICTEMP", "SVCC",
		"BAKBATV", "SYSPWRV", "WALLV", "VBUSV", "AUX3", "AUX2", "AUX1", "AUX0",
		"BATV"
	},
	.chn_unit_names = {
		"mV", "mA", "mA", "mA", "mA", "/4096", "mCel", "mV",
		"mV", "mV", "mV", "mv", "/4096", "/4096", "/4096", "/4096",
		"mV"
	},
	.chn_hwinfos = {
		&sc_atc2609a_chn_hwinfos_iref,
		&sc_atc2609a_chn_hwinfos_chgi,
		&sc_atc2609a_chn_hwinfos_vbusi,
		&sc_atc2609a_chn_hwinfos_walli,
		&sc_atc2609a_chn_hwinfos_bati,
		&sc_atc2609a_chn_hwinfos_remcon,
		&sc_atc2609a_chn_hwinfos_ictemp,
		&sc_atc2609a_chn_hwinfos_svccv,
		&sc_atc2609a_chn_hwinfos_bakbatv,
		&sc_atc2609a_chn_hwinfos_syspwrv,
		&sc_atc2609a_chn_hwinfos_wallv,
		&sc_atc2609a_chn_hwinfos_vbusv,
		&sc_atc2609a_chn_hwinfos_aux3,
		&sc_atc2609a_chn_hwinfos_aux2,
		&sc_atc2609a_chn_hwinfos_aux1,
		&sc_atc2609a_chn_hwinfos_aux0,
		&sc_atc2609a_chn_hwinfos_batv,
	},
};




int atc260x_auxadc_find_chan(struct atc260x_dev *atc260x, const char *channel_name)
{
	uint i;
	ATC260X_ASSERT_VALID_DEV(atc260x);
	if (!channel_name) {
		return -EINVAL;
	}
	for (i = 0; i < atc260x->auxadc_hwinfo->channels; i++) {
		if (0 == strcmp(channel_name, atc260x->auxadc_hwinfo->chn_names[i])) {
			return i;
		}
	}
	return -ENXIO;
}
EXPORT_SYMBOL_GPL(atc260x_auxadc_find_chan);

const char *atc260x_auxadc_channel_name(struct atc260x_dev *atc260x, uint channel)
{
	ATC260X_ASSERT_VALID_DEV(atc260x);
	if (channel >= atc260x->auxadc_hwinfo->channels)
		return NULL;
	return atc260x->auxadc_hwinfo->chn_names[channel];
}
EXPORT_SYMBOL_GPL(atc260x_auxadc_channel_name);

const char *atc260x_auxadc_channel_unit_name(struct atc260x_dev *atc260x, uint channel)
{
	ATC260X_ASSERT_VALID_DEV(atc260x);
	if (channel >= atc260x->auxadc_hwinfo->channels)
		return NULL;
	return atc260x->auxadc_hwinfo->chn_unit_names[channel];
}
EXPORT_SYMBOL_GPL(atc260x_auxadc_channel_unit_name);

static int _atc260x_auxadc_get_raw_inner(struct atc260x_dev *atc260x, uint channel,
		const struct atc260x_auxadc_chn_hwinfo *ch_info)
{
	uint raw_value;
	int ret;

	/* get raw value */
	if (unlikely(ch_info->getraw_func)) {
		/* only this branch needs lock. */
		mutex_lock(&atc260x->auxadc_read_mutex);
		ret = (ch_info->getraw_func)(channel, ch_info, atc260x);
		mutex_unlock(&atc260x->auxadc_read_mutex);
		if (ret < 0) {
			dev_err(atc260x->dev,
				"%s() getraw_func failed, auxadc #%u\n", __func__, channel);
			return -EIO;
		}
		raw_value = ret;
	} else {
		ret = atc260x_reg_read(atc260x, ch_info->dat_reg_addr);
		if (ret < 0) {
			dev_err(atc260x->dev,
				"%s() can not read auxadc reg 0x%x\n",
				__func__, ch_info->dat_reg_addr);
			return -EIO;
		}
		raw_value = ((uint)ret >> ch_info->dat_bit_offs) & ((1U << ch_info->dat_bit_cnt) -1U);
	}
	return raw_value;
}

int atc260x_auxadc_get_raw(struct atc260x_dev *atc260x, uint channel)
{
	const struct atc260x_auxadc_chn_hwinfo *ch_info;

	ATC260X_ASSERT_VALID_DEV(atc260x);
	if (channel >= atc260x->auxadc_hwinfo->channels) {
		dev_err(atc260x->dev,
				"%s() no such channel: %u\n", __func__, channel);
		return -ENXIO;
	}
	ch_info = atc260x->auxadc_hwinfo->chn_hwinfos[channel];
	return _atc260x_auxadc_get_raw_inner(atc260x, channel, ch_info);
}
EXPORT_SYMBOL_GPL(atc260x_auxadc_get_raw);

int atc260x_auxadc_get_translated(struct atc260x_dev *atc260x, uint channel, s32 *p_tr_value)
{
	const struct atc260x_auxadc_chn_hwinfo *ch_info;
	int ret, tr_value;

	ATC260X_ASSERT_VALID_DEV(atc260x);

	if (channel >= atc260x->auxadc_hwinfo->channels) {
		dev_err(atc260x->dev,
				"%s() no such channel: %u\n", __func__, channel);
		return -EINVAL;
	}
	ch_info = atc260x->auxadc_hwinfo->chn_hwinfos[channel];

	ret = _atc260x_auxadc_get_raw_inner(atc260x, channel, ch_info);
	if (ret < 0) {
		dev_err(atc260x->dev,
			"%s() can not get raw value from auxadc #%u, ret=%d\n",
			__func__, channel, ret);
		return ret;
	}
	dev_dbg(atc260x->dev, "%s() channel #%u raw = %d\n", __func__, channel, ret);

	if (ch_info->tran_func) {
		tr_value = (ch_info->tran_func)(ret, ch_info, atc260x);
		dev_dbg(atc260x->dev, "%s() channel #%u call 0x%lx for translate, result=%d\n",
				__func__, channel, (ulong)(ch_info->tran_func), tr_value);
	} else {
		tr_value = (s32)(((s64)ret * ch_info->tr_multiplier) >> ch_info->tr_r_shift) +
			ch_info->tr_offset;
		dev_dbg(atc260x->dev, "%s() channel #%u linear translate, "
				"multiplier=%d r_shift=%u offset=%d result=%d\n",
				__func__, channel, ch_info->tr_multiplier, ch_info->tr_r_shift,
				ch_info->tr_offset, tr_value);
	}

	if (p_tr_value) {
		*p_tr_value = tr_value;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(atc260x_auxadc_get_translated);


/* init --------------------------------------------------------------------- */

static int _atc2603a_auxadc_dev_init(struct atc260x_dev *atc260x)
{
	atc260x->auxadc_hwinfo = &sc_atc2603a_auxadc_hwinfo;
	atc260x->auxadc_udata = NULL;

	/* open all channels (+ ~1uA) */
	atc260x_reg_write(atc260x, ATC2603A_PMU_AUXADC_CTL0, 0xffffU);

	return 0;
}
static void _atc2603a_auxadc_dev_exit(struct atc260x_dev *atc260x)
{
}

static int _atc2603c_auxadc_dev_init(struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_3c *usrdata;
	int icm_ohm, ret = 0;
	uint dat;
	int ic_ver;

	ic_ver = atc260x_get_ic_ver(atc260x);
	if(ic_ver == 0) {
		atc260x->auxadc_hwinfo = &sc_atc2603c_ver_a_auxadc_hwinfo;
	}
	else{
		atc260x->auxadc_hwinfo = &sc_atc2603c_ver_b_auxadc_hwinfo;
	}
	usrdata = devm_kzalloc(atc260x->dev, sizeof(*usrdata), GFP_KERNEL);
	if (!usrdata) {
		return -ENOMEM;
	}
	atc260x->auxadc_udata = usrdata;

	/*enable adc data output.*/
	ret = atc260x_reg_read(atc260x, ATC2603C_PMU_AUXADC_CTL1);
	if (ret < 0){
		goto label_io_err;
	}
	dat = ret & 0xFFFF;
	/*version B don't set bit3 0 */
	if(ic_ver == 0){
		if (dat & (1U << 3)) {
			/* disable COMP OFFSET TRIMMING */
			atc260x_reg_write(atc260x, ATC2603C_PMU_AUXADC_CTL1, dat & ~(1<<3));
		}
	}
	/* open all channels (+ ~1uA) */
	atc260x_reg_write(atc260x, ATC2603C_PMU_AUXADC_CTL0, 0xffffU);

	icm_ohm = 20;
	ret = of_property_read_u32(atc260x->dev->of_node, "icm_ohm", &icm_ohm);
	if (ret){
		dev_err(atc260x->dev, "icm_ohm not config, use default value\n");
	}
	dev_info(atc260x->dev, "auxadc icm_ohm = %u\n", icm_ohm);
	usrdata->icm_res_10ohm = (icm_ohm == 10);

	/*setup value between CMN and CMP according to the resistor on board.*/
	ret = atc260x_reg_setbits(atc260x, ATC2603C_PMU_AUXADC_CTL1,
		(1U << 4), (((icm_ohm == 10) ? 1 : 0) << 4));
	if (ret) {
		goto label_io_err;
	}

	return 0;

	label_io_err:
	dev_err(atc260x->dev, "%s() fialed to read/write register\n", __func__);
	return -EIO;
}
static void _atc2603c_auxadc_dev_exit(struct atc260x_dev *atc260x)
{
	/* no need to do anything. */
}

static int _atc2609a_auxadc_dev_init(struct atc260x_dev *atc260x)
{
	struct atc260x_auxadc_udata_9a *usrdata;
	uint ictemp_adj;
	int ret;

	atc260x->auxadc_hwinfo = &sc_atc2609a_auxadc_hwinfo;

	usrdata = devm_kzalloc(atc260x->dev, sizeof(*usrdata), GFP_KERNEL);
	if (!usrdata) {
		return -ENOMEM;
	}
	atc260x->auxadc_udata = usrdata;

	/* open all channels (+ ~1uA) */
	atc260x_reg_write(atc260x, ATC2609A_PMU_AUXADC_CTL0, 0xffffU);

	/* IC_TEMP ADJ */
	/*TODO */
	ictemp_adj = 0;
	ret = atc260x_reg_write(atc260x, ATC2609A_PMU_ICTEMPADC_ADJ, ictemp_adj);
	if (ret) {
		dev_err(atc260x->dev, "%s() failed to set ic_temp adjuest value\n", __func__);
	}
	usrdata->adc_ictemp_adj = ictemp_adj;

	return 0;
}
static void _atc2609a_auxadc_dev_exit(struct atc260x_dev *atc260x)
{
}

typedef int (*atc260x_auxadc_init_func_t)(struct atc260x_dev *atc260x);
typedef void (*atc260x_auxadc_exit_func_t)(struct atc260x_dev *atc260x);
static const atc260x_auxadc_init_func_t sc_init_func_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = _atc2603a_auxadc_dev_init,
	[ATC260X_ICTYPE_2603C] = _atc2603c_auxadc_dev_init,
	[ATC260X_ICTYPE_2609A] = _atc2609a_auxadc_dev_init,
};
static const atc260x_auxadc_exit_func_t sc_exit_func_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = _atc2603a_auxadc_dev_exit,
	[ATC260X_ICTYPE_2603C] = _atc2603c_auxadc_dev_exit,
	[ATC260X_ICTYPE_2609A] = _atc2609a_auxadc_dev_exit,
};

int atc260x_auxadc_dev_init(struct atc260x_dev *atc260x)
{
	int ret;

	mutex_init(&atc260x->auxadc_read_mutex);

	BUG_ON(atc260x->ic_type >= ATC260X_ICTYPE_CNT);
	ret = (sc_init_func_tbl[atc260x->ic_type])(atc260x);
	if (ret) {
		dev_err(atc260x->dev, "%s() failed, ret=%d\n", __func__, ret);
	}
	return ret;
}
void atc260x_auxadc_dev_exit(struct atc260x_dev *atc260x)
{
	(sc_exit_func_tbl[atc260x->ic_type])(atc260x);
	mutex_destroy(&atc260x->auxadc_read_mutex);
}
