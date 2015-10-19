/* This is ugly,
 * but we need to move these long list away to make code clean and readable. */

#ifdef __MFD_ATC260X_REGULATOR__NEED_HWINFO_DEFINE__

#ifndef __MFD_ATC260X_REGULATOR_HWINFO_DEF_H__
#define __MFD_ATC260X_REGULATOR_HWINFO_DEF_H__


/* for atc2603a ------------------------------------------------------------- */

static const struct atc260x_regu_hwinfo sc_atc2603a_dcdc_hwinfo_tbl[] = {
	/* keep space for DCDC0 (there's no DCDC0 here) */
	{ },

	/* DCDC 1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1400000,
				.volt_min_uv =  700000,
				.volt_step_uv =  25000,
				.vsel_start = 0,
				.vsel_end   = 28,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_DC1_CTL0,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_DC1_CTL0,
		.vsel_volt_bm = 0x1fU << 7,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 15,
		.state_uv_bm = 1 << 15,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* DCDC 2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1950000,
				.volt_min_uv = 1300000,
				.volt_step_uv =  50000,
				.vsel_start = 0,
				.vsel_end   = 13,
			},
			{
				.volt_max_uv = 2150000,
				.volt_min_uv = 1950000,
				.volt_step_uv = 100000,
				.vsel_start = 13,
				.vsel_end   = 15,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_DC2_CTL0,
		.ctrl_en_bm = 1 << 15,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_DC2_CTL0,
		.vsel_volt_bm = 0xfU << 8,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 14,
		.state_uv_bm = 1 << 14,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_DC3_CTL0,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_DC3_CTL0,
		.vsel_volt_bm = 0x7U << 9,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 13,
		.state_uv_bm = 1 << 13,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* DCDC 4 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 5000000,
				.volt_min_uv = 5000000,
				.volt_step_uv = 0,
				.vsel_start = 0,
				.vsel_end   = 0,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_DC4_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_DC4_CTL0,
		.vsel_volt_bm = 0,

		.stable_time_on = 3000,

		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
};

static const struct atc260x_regu_hwinfo sc_atc2603a_ldo_hwinfo_tbl[] = {
	/* keep space for LDO0 (there's no LDO0 here) */
	{ },

	/* LDO1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO1_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO1_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 11,
		.state_uv_bm = 1 << 11,
		.state_oc_bm = 1 << 15,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO2_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO2_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 10,
		.state_uv_bm = 1 << 10,
		.state_oc_bm = 1 << 14,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2000000,
				.volt_min_uv = 1500000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 5,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO3_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO3_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 9,
		.state_uv_bm = 1 << 9,
		.state_oc_bm = 1 << 13,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO4 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3500000,
				.volt_min_uv = 2800000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO4_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO4_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 8,
		.state_uv_bm = 1 << 8,
		.state_oc_bm = 1 << 12,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO5 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO5_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO5_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 7,
		.state_uv_bm = 1 << 7,
		.state_oc_bm = 1 << 11,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO6 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1400000,
				.volt_min_uv =  700000,
				.volt_step_uv =  25000,
				.vsel_start = 0,
				.vsel_end   = 28,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO6_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO6_CTL,
		.vsel_volt_bm = 0x1fU << 11,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 6,
		.state_uv_bm = 1 << 6,
		.state_oc_bm = 1 << 10,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO7 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2000000,
				.volt_min_uv = 1500000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 5,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO7_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO7_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 5,
		.state_uv_bm = 1 << 5,
		.state_oc_bm = 1 << 9,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO8 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2300000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 10,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO8_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO8_CTL,
		.vsel_volt_bm = 0xfU << 12,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 4,
		.state_uv_bm = 1 << 4,
		.state_oc_bm = 1 << 8,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO9 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1500000,
				.volt_min_uv = 1000000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 5,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO9_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO9_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 3,
		.state_uv_bm = 1 << 3,
		.state_oc_bm = 1 << 7,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO10 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2300000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 10,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO10_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO10_CTL,
		.vsel_volt_bm = 0xfU << 12,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 2,
		.state_uv_bm = 1 << 2,
		.state_oc_bm = 1 << 6,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO11 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603A_PMU_LDO11_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603A_PMU_LDO11_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603A_PMU_OC_STATUS,
		.state_ov_bm = 0,
		.state_uv_bm = 0,
		.state_oc_bm = 0,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
};

/* for atc2603c ------------------------------------------------------------- */

static const struct atc260x_regu_hwinfo sc_atc2603c_ver_a_dcdc_hwinfo_tbl[] = {
	/* keep space for DCDC0 (there's no DCDC0 here) */
	{ },

	/* DCDC 1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1400000,
				.volt_min_uv =  700000,
				.volt_step_uv =  25000,
				.vsel_start = 0,
				.vsel_end   = 28,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_DC1_CTL0,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_DC1_CTL0,
		.vsel_volt_bm = 0x1fU << 7,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 15,
		.state_uv_bm = 1 << 15,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* DCDC 2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1950000,
				.volt_min_uv = 1300000,
				.volt_step_uv =  50000,
				.vsel_start = 0,
				.vsel_end   = 13,
			},
			{
				.volt_max_uv = 2150000,
				.volt_min_uv = 1950000,
				.volt_step_uv = 100000,
				.vsel_start = 13,
				.vsel_end   = 15,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_DC2_CTL0,
		.ctrl_en_bm = 1 << 15,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_DC2_CTL0,
		.vsel_volt_bm = 0xfU << 8,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 14,
		.state_uv_bm = 1 << 14,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_DC3_CTL0,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_DC3_CTL0,
		.vsel_volt_bm = 0x7U << 9,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 13,
		.state_uv_bm = 1 << 13,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
};

static const struct atc260x_regu_hwinfo sc_atc2603c_ver_b_dcdc_hwinfo_tbl[] = {
	/* keep space for DCDC0 (there's no DCDC0 here) */
	{ },

	/* DCDC 1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1400000,
				.volt_min_uv =  700000,
				.volt_step_uv =  25000,
				.vsel_start = 0,
				.vsel_end   = 28,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_DC1_CTL0,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_DC1_CTL0,
		.vsel_volt_bm = 0x1fU << 7,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 15,
		.state_uv_bm = 1 << 15,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* DCDC 2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1850000,
				.volt_min_uv = 1000000,
				.volt_step_uv =  50000,
				.vsel_start = 0,
				.vsel_end   = 17,
			},
			#if 0
			{
				.volt_max_uv = 2150000,
				.volt_min_uv = 1950000,
				.volt_step_uv = 100000,
				.vsel_start = 13,
				.vsel_end   = 15,
			},
			#endif
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_DC2_CTL0,
		.ctrl_en_bm = 1 << 15,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_DC2_CTL0,
		.vsel_volt_bm = 0x1fU << 8,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 14,
		.state_uv_bm = 1 << 14,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_DC3_CTL0,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_DC3_CTL0,
		.vsel_volt_bm = 0x7U << 9,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 13,
		.state_uv_bm = 1 << 13,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 350,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
};
static const struct atc260x_regu_hwinfo sc_atc2603c_ldo_hwinfo_tbl[] = {
	/* keep space for LDO0 (there's no LDO0 here) */
	{ },

	/* LDO1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO1_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO1_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 11,
		.state_uv_bm = 1 << 11,
		.state_oc_bm = 1 << 15,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO2_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO2_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 10,
		.state_uv_bm = 1 << 10,
		.state_oc_bm = 1 << 14,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2000000,
				.volt_min_uv = 1500000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 5,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO3_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO3_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 9,
		.state_uv_bm = 1 << 9,
		.state_oc_bm = 1 << 13,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* space for LDO4 */
	{ },

	/* LDO5 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO5_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO5_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 7,
		.state_uv_bm = 1 << 7,
		.state_oc_bm = 1 << 11,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO6 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1400000,
				.volt_min_uv =  700000,
				.volt_step_uv =  25000,
				.vsel_start = 0,
				.vsel_end   = 28,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO6_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO6_CTL,
		.vsel_volt_bm = 0x1fU << 11,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 6,
		.state_uv_bm = 1 << 6,
		.state_oc_bm = 1 << 10,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},

	/* LDO7 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2000000,
				.volt_min_uv = 1500000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 5,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO7_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO7_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 5,
		.state_uv_bm = 1 << 5,
		.state_oc_bm = 1 << 9,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO8 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2300000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 10,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO8_CTL,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO8_CTL,
		.vsel_volt_bm = 0xfU << 12,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 1 << 4,
		.state_uv_bm = 1 << 4,
		.state_oc_bm = 1 << 8,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* space for LDO9 */
	{ },
	/* space for LDO10 */
	{ },

	/* LDO11 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2603C_PMU_LDO11_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2603C_PMU_LDO11_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2603C_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2603C_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2603C_PMU_OC_STATUS,
		.state_ov_bm = 0,
		.state_uv_bm = 0,
		.state_oc_bm = 0,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
};


/* for atc2609a ------------------------------------------------------------- */

static const struct atc260x_regu_hwinfo sc_atc2609a_dcdc_hwinfo_tbl[] = {
	/* DCDC 0 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2193750,
				.volt_min_uv =  600000,
				.volt_step_uv =   6250,
				.vsel_start = 0,
				.vsel_end   = 255,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_DC_OSC,
		.ctrl_en_bm = 1 << 4,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_DC0_CTL0,
		.vsel_volt_bm = 0xffU << 8,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 15,
		.state_uv_bm = 1 << 15,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 250,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2193750,
				.volt_min_uv =  600000,
				.volt_step_uv =   6250,
				.vsel_start = 0,
				.vsel_end   = 255,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_DC_OSC,
		.ctrl_en_bm = 1 << 5,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_DC1_CTL0,
		.vsel_volt_bm = 0xffU << 8,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 14,
		.state_uv_bm = 1 << 14,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 250,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2193750,
				.volt_min_uv =  600000,
				.volt_step_uv =   6250,
				.vsel_start = 0,
				.vsel_end   = 255,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_DC_OSC,
		.ctrl_en_bm = 1 << 6,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_DC2_CTL0,
		.vsel_volt_bm = 0xffU << 8,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 13,
		.state_uv_bm = 1 << 13,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 250,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 1400000,
				.volt_min_uv =  600000,
				.volt_step_uv =   6250,
				.vsel_start = 0,
				.vsel_end   = 128,
			},
			{
				.volt_max_uv = 4000000,
				.volt_min_uv = 1400000,
				.volt_step_uv =  25000,
				.vsel_start = 128,
				.vsel_end   = 232,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_DC_OSC,
		.ctrl_en_bm = 1 << 7,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_DC3_CTL0,
		.vsel_volt_bm = 0xffU << 8,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 12,
		.state_uv_bm = 1 << 12,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 250,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* DCDC 4 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2193750,
				.volt_min_uv =  600000,
				.volt_step_uv =   6250,
				.vsel_start = 0,
				.vsel_end   = 255,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_DC_OSC,
		.ctrl_en_bm = 1 << 8,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_DC4_CTL0,
		.vsel_volt_bm = 0xffU << 8,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 2,
		.state_uv_bm = 1 << 2,
		.state_oc_bm = 0,

		.stable_time_on = 800,
		.stable_time_chng_volt = 250,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},
};

/* for LDO-6 bugfix */
static int _atc2609a_ldo6_volt_lookup(struct atc260x_regulator_dev *atc260x_regu,
		const struct atc260x_regu_hwinfo *hwinfo, uint band, uint index)
{
	static const u16 sc_revd_buggy_volt_tbl[16] = {
		/* in mV */
		850, 900, 950, 1000, 1050, 1050, 1050, 1050, 1050, 1050, 1050,
		1100, 1150, 1200, 1250, 2200
	};
	BUG_ON(index >= 16);
	if (band == 0) {
		/* check rev */
		if (atc260x_get_ic_ver(atc260x_regu->atc260x) > ATC260X_ICVER_D) {
			/* > revD, no bug */
			BUG(); /* TODO : fill this when revE ready. */
			return 0;
		} else {
			return sc_revd_buggy_volt_tbl[index] * 1000U; /* in uV */
		}
	} else {
		/* band 1 has no bug */
		return 2100000U + 100000U * index;
	}
}

static const struct atc260x_regu_hwinfo sc_atc2609a_ldo_hwinfo_tbl[] = {
	/* LDO 0 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3400000,
				.volt_min_uv = 2300000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 11,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_LDO0_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 1 << 1,

		.vsel_reg_addr = ATC2609A_PMU_LDO0_CTL0,
		.vsel_volt_bm = 0xfU << 2,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 11,
		.state_uv_bm = 1 << 11,
		.state_oc_bm = 1 << 11,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_BYPASS,
	},

	/* LDO 1 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3400000,
				.volt_min_uv = 2300000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 11,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_LDO1_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 1 << 1,

		.vsel_reg_addr = ATC2609A_PMU_LDO1_CTL0,
		.vsel_volt_bm = 0xfU << 2,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 10,
		.state_uv_bm = 1 << 10,
		.state_oc_bm = 1 << 10,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_BYPASS,
	},

	/* LDO 2 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3400000,
				.volt_min_uv = 2300000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 11,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_LDO2_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 1 << 1,

		.vsel_reg_addr = ATC2609A_PMU_LDO2_CTL0,
		.vsel_volt_bm = 0xfU << 2,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 9,
		.state_uv_bm = 1 << 9,
		.state_oc_bm = 1 << 9,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_BYPASS,
	},

	/* LDO 3 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2200000,
				.volt_min_uv =  700000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 15,
			},
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2100000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 12,
			},
		},
		.volt_band_exclusive = 1,

		.ctrl_reg_addr = ATC2609A_PMU_LDO3_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO3_CTL0,
		.vsel_volt_bm = 0xfU << 1,
		.vsel_band_reg_addr = ATC2609A_PMU_LDO3_CTL0,
		.vsel_band_bm = 1 << 5,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 8,
		.state_uv_bm = 1 << 8,
		.state_oc_bm = 1 << 8,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO 4 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2200000,
				.volt_min_uv =  700000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 15,
			},
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2100000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 12,
			},
		},
		.volt_band_exclusive = 1,

		.ctrl_reg_addr = ATC2609A_PMU_LDO4_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO4_CTL0,
		.vsel_volt_bm = 0xfU << 1,
		.vsel_band_reg_addr = ATC2609A_PMU_LDO4_CTL0,
		.vsel_band_bm = 1 << 5,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 7,
		.state_uv_bm = 1 << 7,
		.state_oc_bm = 1 << 7,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO 5 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2200000,
				.volt_min_uv =  700000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 15,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_LDO5_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO5_CTL0,
		.vsel_volt_bm = 0xfU << 1,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 6,
		.state_uv_bm = 1 << 6,
		.state_oc_bm = 1 << 6,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO 6 */
	{
		.nonlinear_lookup = _atc2609a_ldo6_volt_lookup, /* for bugfix */
		.volt_bands = {
			{
				.volt_max_uv = 2200000,
				.volt_min_uv =  850000,
				.volt_step_uv =  50000,
				.vsel_start = 0,
				.vsel_end   = 15,
			},
			{
				.volt_max_uv = 3200000,
				.volt_min_uv = 2100000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 11,
			},
		},
		.volt_band_exclusive = 1,

		.ctrl_reg_addr = ATC2609A_PMU_LDO6_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO6_CTL0,
		.vsel_volt_bm = 0xfU << 1,
		.vsel_band_reg_addr = ATC2609A_PMU_LDO6_CTL0,
		.vsel_band_bm = 1 << 5,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 5,
		.state_uv_bm = 1 << 5,
		.state_oc_bm = 1 << 5,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO 7 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2200000,
				.volt_min_uv =  700000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 15,
			},
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2100000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 12,
			},
		},
		.volt_band_exclusive = 1,

		.ctrl_reg_addr = ATC2609A_PMU_LDO7_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO7_CTL0,
		.vsel_volt_bm = 0xfU << 1,
		.vsel_band_reg_addr = ATC2609A_PMU_LDO7_CTL0,
		.vsel_band_bm = 1 << 5,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 4,
		.state_uv_bm = 1 << 4,
		.state_oc_bm = 1 << 4,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO 8 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 2200000,
				.volt_min_uv =  700000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 15,
			},
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2100000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 12,
			},
		},
		.volt_band_exclusive = 1,

		.ctrl_reg_addr = ATC2609A_PMU_LDO8_CTL0,
		.ctrl_en_bm = 1 << 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO8_CTL0,
		.vsel_volt_bm = 0xfU << 1,
		.vsel_band_reg_addr = ATC2609A_PMU_LDO8_CTL0,
		.vsel_band_bm = 1 << 5,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 1 << 3,
		.state_uv_bm = 1 << 3,
		.state_oc_bm = 1 << 3,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},

	/* LDO 9 */
	{
		.volt_bands = {
			{
				.volt_max_uv = 3300000,
				.volt_min_uv = 2600000,
				.volt_step_uv = 100000,
				.vsel_start = 0,
				.vsel_end   = 7,
			},
		},
		.volt_band_exclusive = 0,

		.ctrl_reg_addr = ATC2609A_PMU_LDO9_CTL,
		.ctrl_en_bm = 0,
		.ctrl_bypass_bm = 0,

		.vsel_reg_addr = ATC2609A_PMU_LDO9_CTL,
		.vsel_volt_bm = 0x7U << 13,

		.state_ov_reg_addr = ATC2609A_PMU_OV_STATUS,
		.state_uv_reg_addr = ATC2609A_PMU_UV_STATUS,
		.state_oc_reg_addr = ATC2609A_PMU_OC_STATUS,
		.state_ov_bm = 0,
		.state_uv_bm = 0,
		.state_oc_bm = 0,

		.stable_time_on = 2000,
		.stable_time_chng_volt = 800,

		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
};




#endif /* __MFD_ATC260X_REGULATOR_HWINFO_DEF_H__ */

#endif
