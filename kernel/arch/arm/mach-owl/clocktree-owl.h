#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk-private.h>
#include <linux/clkdev.h>

#include <linux/of.h>

#include <mach/hardware.h>
#include <mach/clkname.h>
#include <mach/clkname_priv.h>
#include "clock-owl.h"

#define CLOCKNODE_UNUSED(clock)	\
	/*[CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_STATIC,\
		.source_lim = 0,\
		.source_sel = -1,\
		.frequency = 0,\
		.clock_en = 0,\
	}

#define PLLNODE(clock, src)	\
	/*[CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_PLL,\
		.source_av = {CLOCK__##src,},\
		.source_lim = 1,\
		.source_sel = 0,\
		.clock_en = 1,\
	}

#define CLOCKNODE_ROOT(clock, freq)	\
	/*[CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_STATIC,\
		.source_lim = 0,\
		.source_sel = -1,\
		.frequency = freq,\
		.clock_en = 1,\
	}

#define CLOCKNODE_S1(clock, src)	\
	/*[CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_DYNAMIC,\
		.source_av = {CLOCK__##src,},\
		.source_lim = 1,\
		.source_sel = 0,\
		.clock_en = 1,\
		.changed = 1,\
	}

#define CLOCKNODE_S2(clock, src, src2, sel)	\
	/* [CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_DYNAMIC,\
		.source_av = {CLOCK__##src, CLOCK__##src2},\
		.source_lim = 2,\
		.source_sel = sel,\
		.reg_srcsel = &selbit_##clock,\
		.clock_en = 1,\
		.changed = 1,\
	}

#define CLOCKNODE_S3(clock, src, src2, src3, sel)	\
	/* [CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_DYNAMIC,\
		.source_av = {CLOCK__##src, CLOCK__##src2, CLOCK__##src3},\
		.source_lim = 3,\
		.source_sel = sel,\
		.reg_srcsel = &selbit_##clock,\
		.clock_en = 1,\
		.changed = 1,\
	}

#define CLOCKNODE_S4(clock, src, src2, src3, src4, sel)	\
	/* [CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_DYNAMIC,\
		.source_av = {CLOCK__##src, CLOCK__##src2, CLOCK__##src3, CLOCK__##src4},\
		.source_lim = 4,\
		.source_sel = sel,\
		.reg_srcsel = &selbit_##clock,\
		.clock_en = 1,\
		.changed = 1,\
	}

#define CLOCKNODE_S5(clock, src, src2, src3, src4, src5, sel)	\
	/* [CLOCK__##clock] = */ {\
		.name = CLKNAME_##clock,\
		.id = CLOCK__##clock,\
		.type = TYPE_DYNAMIC,\
		.source_av = {CLOCK__##src, CLOCK__##src2, CLOCK__##src3, CLOCK__##src4, CLOCK__##src5},\
		.source_lim = 5,\
		.source_sel = sel,\
		.reg_srcsel = &selbit_##clock,\
		.clock_en = 1,\
		.changed = 1,\
	}

#define BITMAP(reg, _mask, _offset) \
	{\
		.reg_no = R_##reg, \
		.reg_hw = (unsigned long *)reg, \
		.mask = _mask, \
		.offset = _offset,\
	}

#define FREQUENCY_24M	(24 * 1 * MEGA)
#define FREQUENCY_32K	(32 * 1 * KILO)

#define MODULE_RESET_TIME_MS  1

extern struct clk_ops clk_ops_corepll;
extern struct clk_ops clk_ops_pll;
extern struct clk_ops clk_ops_gate_module;
extern struct clk_ops clk_ops_direct_s_parent;
extern struct clk_ops clk_ops_direct_m_parent;
extern struct clk_ops clk_ops_s_divider_s_parent;
extern struct clk_ops clk_ops_s_divider_m_parent;
extern struct clk_ops clk_ops_m_divider_s_parent;
extern struct clk_ops clk_ops_m_divider_m_parent;
extern struct clk_ops clk_ops_foo;
extern struct clk_ops clk_ops_b_s_divider_s_parent;
extern struct clk_ops clk_ops_b_s_divider_m_parent;
extern struct clk_ops clk_ops_b_m_divider_s_parent;
extern struct clk_ops clk_ops_b_m_divider_m_parent;
extern struct clk_ops clk_ops_h_s_divider_s_parent;
extern struct clk_ops clk_ops_h_s_divider_m_parent;
extern struct clk_ops clk_ops_h_m_divider_s_parent;
extern struct clk_ops clk_ops_h_m_divider_m_parent;

struct clocks_table {
    struct owl_clocknode *clocks;
    unsigned long *rvregs;
    struct owl_pll *pllnode;
    struct clk *owl_clks;
    struct owl_cmumod *modnode;
    struct owl_clk_foo *clk_foo_clocks;
    struct clk_lookup *lookup_table;
};

int owl_clk_config_recursion(int clock, int recursion);

int getdivider(struct owl_div *div, int n);
int getdivider_resetval(struct owl_div *div);
int addclock(int clock);

struct clocks_table * atm7059_get_clocktree(void);
void atm7059_init_clocktree(void);
void atm7059_prepare_clocktree(void);

