/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/******************************************************************************/

/******************************************************************************/
#ifndef __OWL_CPU_PWM_VOLT_H__
#define __OWL_CPU_PWM_VOLT_H__

#ifdef __cplusplus
extern "C" {
#endif

/********gamma info********************/
#define OWL_DSS_GAMMA_SIZE	(256 * 3)

struct owl_gamma_info {
	u32 gamma_table[OWL_DSS_GAMMA_SIZE / 4];
	int is_valid;
};
/****************************/

/*********cpu pwm volt table**********************/
#define CPU_PWM_VOLT_TABLE_LEN 16

struct cpu_pwm_volt_table {
	unsigned int pwm_val;
	unsigned int voltage_mv;
};

struct cpu_pwm_volt_info {
	struct cpu_pwm_volt_table cpu_pwm_volt_tb[CPU_PWM_VOLT_TABLE_LEN];
	int cpu_pwm_volt_tb_len;
};
/******************************/

struct kernel_reserve_info {
	struct owl_gamma_info gamma;
	struct cpu_pwm_volt_info cpu_pwm_volt;
} __attribute__ ((packed));

extern struct kernel_reserve_info *kinfo;

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
