/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/******************************************************************************/

/******************************************************************************/
#ifndef __OWL_CPU_MAP_H__
#define __OWL_CPU_MAP_H__

#ifdef __cplusplus
extern "C" {
#endif

struct cpu0_opp_table {
	unsigned long clk; /*khz*/
	unsigned long volt; /*uv*/
};

//extern int cpu0_add_opp_table(struct device *cpu_dev, struct cpu0_opp_table *table, int table_size);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
