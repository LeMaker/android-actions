#include "clocktree-owl.h"
#include <mach/module-owl.h>

#if 1
 #define COREPLL_CHANGE_MIDDLE_CLK CLOCK__HOSC
 #define COREPLL_CHANGE_DELAY_DIV 25
#else
 #define COREPLL_CHANGE_MIDDLE_CLK CLOCK__VCE_CLK
 #define COREPLL_CHANGE_DELAY_DIV 1
#endif



static unsigned long *rvregs;
static struct owl_clocknode *clocks;
static struct owl_pll *pllnode;
static struct clk *owl_clks;
static struct owl_cmumod *modnode;
static struct owl_clk_foo *clk_foo_clocks;
static struct clk_lookup *lookup_table;


static DEFINE_SPINLOCK(cmureg_update_lock);
unsigned long regs[R_CMUMAX];
static spinlock_t cpu_lock;

int read_clkreg_val(struct owl_clkreq *reg)
{
	regs[reg->reg_no] = act_readl((u32)reg->reg_hw);
	return ((regs[reg->reg_no])&reg->mask)>>reg->offset;
}
EXPORT_SYMBOL(read_clkreg_val);

void write_clkreg_val(struct owl_clkreq *reg, int val)
{
	unsigned long read_back;
	unsigned long flags;

	spin_lock_irqsave(&cmureg_update_lock, flags);

	regs[reg->reg_no] = act_readl((u32)reg->reg_hw);
	regs[reg->reg_no] &= ~reg->mask;
	regs[reg->reg_no] |= (val << reg->offset) & reg->mask;
	act_writel(regs[reg->reg_no], (u32)reg->reg_hw);

	read_back = act_readl((u32)reg->reg_hw);
	if (read_back != regs[reg->reg_no]) {
		printk(KERN_WARNING "%s/%d: CMUREG read back error\n", __FILE__, __LINE__);
		act_writel(regs[reg->reg_no], (u32)reg->reg_hw);
	}

	spin_unlock_irqrestore(&cmureg_update_lock, flags);
}
EXPORT_SYMBOL(write_clkreg_val);

int getdivider_resetval(struct owl_div *div)
{
	int reg_no;

	reg_no = div->reg->reg_no;
	return (rvregs[reg_no] & div->reg->mask) >> div->reg->offset;
}

int getdivider(struct owl_div *div, int n)
{
	int d;
	struct owl_section  *owl_section;

	d = -1;
	if (n >= div->range_from && n <= div->range_to) {
		if (div->type == DIV_T_COMP) {
			if (n >= div->ext.comp->sections[0].range_from
				&& n <= div->ext.comp->sections[0].range_to) {
				owl_section = &div->ext.comp->sections[0];
			} else {
				owl_section = &div->ext.comp->sections[1];
			}
		} else {
			owl_section = (struct owl_section *)div;
		}

		switch (owl_section->type) {
		case DIV_T_NATURE:
			d = n + 1;
			break;
		case DIV_T_EXP:
			d = (1 << n);
			break;
		case DIV_T_EXP_D2:
			d = (2 << n);
			break;
		case DIV_T_SEQ:
			d = owl_section->ext.seq->seq[n];
			break;
		case DIV_T_SEQ_D2:
			d = (owl_section->ext.seq->seq[n] << 1);
			break;
		case DIV_T_TABLE:
			d = owl_section->ext.tab->div[n-owl_section->range_from];
			break;
		}
	}

	return d;
}

static int getdivider_ge(struct owl_div *div, int divexp)
{
	int i;
	int d;
	int r, m;
	struct owl_section  *owl_section;

	d = -1;

	m = divexp >> 16;
    divexp &= 0xffff;
	if (div->type == DIV_T_COMP) {
		owl_section = &div->ext.comp->sections[1];
		d = getdivider(div, owl_section->range_from);
		if ( (m == 0 && divexp < d) || (m != 0 && divexp < d * m) ) {
			owl_section = &div->ext.comp->sections[0];
		} else {
			d = -1;
		}
	} else {
		owl_section = (struct owl_section *)div;
	}

    if(owl_section->type != DIV_T_TABLE && m != 0)
        divexp = (divexp / m) + 1;
	switch (owl_section->type) {
	case DIV_T_NATURE:
		if (divexp < owl_section->range_from + 1) {
			d = owl_section->range_from + 1;
		} else if (divexp > owl_section->range_to + 1) {
			break;
		} else
			d = divexp;
		break;
	case DIV_T_EXP_D2:
		if (divexp < (2 << owl_section->range_from)) {
			d = (2 << owl_section->range_from);
		} else if (divexp > (2 << owl_section->range_to)) {
			break;
		} else {
			d = 1;
			r = divexp;
			while (d < divexp) {
				d <<= 1;
			}
		}
		break;
	case DIV_T_EXP:
		if (divexp < (1 << owl_section->range_from)) {
			d = (1 << owl_section->range_from);
		} else if (divexp > (1 << owl_section->range_to)) {
			break;
		} else {
			d = 1;
			r = divexp;
			while (d < divexp) {
				d <<= 1;
			}
		}
		break;
	case DIV_T_SEQ_D2:
		for (i = owl_section->range_to; i >= owl_section->range_from; i--) {
			if (divexp > (owl_section->ext.seq->seq[i] << 1))
				break;

			d = (owl_section->ext.seq->seq[i] << 1);
		}
		break;
	case DIV_T_SEQ:
		for (i = owl_section->range_to; i >= owl_section->range_from; i--) {
			if (divexp > owl_section->ext.seq->seq[i])
				break;

			d = owl_section->ext.seq->seq[i];
		}
		break;
	case DIV_T_TABLE:
		for (i = owl_section->range_to-owl_section->range_from; i >= 0; i--) {
			r = (owl_section->ext.tab->div[i] & 0xffff0000) >> 16;
			if(r != 0 && m != 0) {
				if (divexp * r > (owl_section->ext.tab->div[i] & 0xffff) * m)
					break;
			} else if(r != 0 && m == 0) {
				if (divexp * r > (owl_section->ext.tab->div[i] & 0xffff))
					break;
		    } else if(r == 0 && m != 0) {
				if (divexp > (owl_section->ext.tab->div[i] & 0xffff) * m)
					break;
			} else {
				if (divexp > owl_section->ext.tab->div[i])
					break;
			}
			
			d = owl_section->ext.tab->div[i];
		}
		break;
	}
	return d;
}

static int getdivider_index(struct owl_div *div, int divexp)
{
	int i;
	int idx;
	struct owl_section  *owl_section;

	idx = -1;

	if (div->type == DIV_T_COMP) {
		int j = 0;
		owl_section = &div->ext.comp->sections[1];
loop:
		i = getdivider(div, owl_section->range_from+j);
		if (divexp == i) {
			return owl_section->range_from + j;
		}

		if (i > 0 && (i & 0xffff0000)) {
			j++;
			if (owl_section->range_from + j <= owl_section->range_to) {
				goto loop;
			}
		}
		if (divexp < i) {
			owl_section = &div->ext.comp->sections[0];
		} else {
			idx = -1;
		}
	} else {
		owl_section = (struct owl_section *)div;
	}

	switch (owl_section->type) {
	case DIV_T_NATURE:
		if (divexp > owl_section->range_from && divexp <= owl_section->range_to + 1) {
			idx = divexp - 1;
		}
		break;
	case DIV_T_EXP_D2:
		if (divexp & 1) {
			break;
		}
		divexp >>= 1;
	case DIV_T_EXP:
		if (divexp >= (1 << owl_section->range_from) &&
			divexp <= (1 << owl_section->range_to)) {
			idx = owl_section->range_from;
			i = (1 << idx);
			while (i < divexp) {
				i <<= 1;
				idx++;
			}
			if (i != divexp) {
				idx = -1;
			}
		}
		break;
	case DIV_T_SEQ_D2:
		if (divexp & 1) {
			break;
		}
		divexp >>= 1;
	case DIV_T_SEQ:
		for (i = owl_section->range_to; i >= owl_section->range_from; i--) {
			if (divexp == owl_section->ext.seq->seq[i]) {
				idx = i;
				break;
			}
		}
		break;
	case DIV_T_TABLE:
		for (i = owl_section->range_to - owl_section->range_from; i >= 0; i--) {
			if (divexp == owl_section->ext.tab->div[i]) {
				idx = owl_section->range_from + i;
				break;
			}
		}
		break;
	}
	return idx;
}

int addclock(int clock)
{
	struct owl_clocknode *node;
	struct owl_clocknode *parent;

	node = &clocks[clock];
	if (node->id != clock) {
		/* node index error */
		return -1;
	}

	if (node->parent != NULL) {
		return -1;
	}

	if (node->type != TYPE_STATIC
		&& node->source_sel >= 0
		&& node->source_sel < node->source_lim) {
		parent = &clocks[node->source_av[node->source_sel]];
		node->parent = parent;

		if (parent->sub == NULL) {
			node->prev = node;
			node->next = NULL;
			parent->sub = node;
		} else {
			node->prev = parent->sub->prev;
			node->next = NULL;
			parent->sub->prev->next = node;
			parent->sub->prev = node;
		}

		return 0;
	}

	return -1;
}

static int removeclock(int clock)
{
	struct owl_clocknode *node;
	struct owl_clocknode *parent;
	struct owl_clocknode *p;

	node = &clocks[clock];
	if (node->parent == NULL) {
		return -1;
	}

	parent = &clocks[node->source_av[node->source_sel]];
	if (parent != node->parent) {
		/* parent error */
		return -1;
	}

	node->parent = NULL;

	p = parent->sub;
	while (p && p->id != clock) {
		p = p->next;
	}

	if (p) {
		if (parent->sub->prev != parent->sub) {
			if (p->next) {
				p->next->prev = p->prev;
			} else {
				parent->sub->prev = p->prev;
			}
			if (p->prev->next == p) {
				p->prev->next = p->next;
			}
			if (p == parent->sub) {
				parent->sub = p->next;
			}
			p->prev = p->next = NULL;
		} else {
			parent->sub = NULL;
			p->prev = p->next = NULL;
		}

		return 0;
	}

	return -1;
}

static void changeclock(int clock)
{
	struct owl_clocknode *node;
	struct owl_clocknode *p;

	node = &clocks[clock];
	node->changed = 1;
	p = node->sub;
	while (p) {
		changeclock(p->id);
		p = p->next;
	}
}


static int  sourcesel(int clock, int source)
{
	struct owl_clocknode *node;
	int i;
	int clocksync;
	struct owl_clkreq *reg;

	node = &clocks[clock];
	for (i = 0; i < node->source_lim; i++) {
		if (node->source_av[i] == source) {
			removeclock(clock);
			reg = node->reg_srcsel;
			write_clkreg_val(reg, i);

			node->source_sel = i;
			addclock(clock);
			changeclock(clock);

			switch (clock) {
			case CLOCK__DE1_CLK:
				clocksync = CLOCK__DE2_CLK;
				goto share_selbit_sync;
			case CLOCK__DE2_CLK:
				clocksync = CLOCK__DE1_CLK;
				goto share_selbit_sync;
			case CLOCK__SENSOR_CLKOUT0:
				clocksync = CLOCK__SENSOR_CLKOUT1;
				goto share_selbit_sync;
			case CLOCK__SENSOR_CLKOUT1:
				clocksync = CLOCK__SENSOR_CLKOUT0;
				goto share_selbit_sync;
			case CLOCK__NANDC_CLK:
				clocksync = CLOCK__ECC_CLK;
				goto share_selbit_sync;
			case CLOCK__ECC_CLK:
				clocksync = CLOCK__NANDC_CLK;
				goto share_selbit_sync;
			default:
				break;
			share_selbit_sync:
				/* share_selbit_sync to clocks[clocksync] */
				node = &clocks[clocksync];
				removeclock(clocksync);
				node->source_sel = i;
				addclock(clocksync);
				changeclock(clocksync);
				break;
			}

			return 0;
		}
	}
	return -1;
}


static void calcfrequency(int clock)
{
	struct owl_clocknode *node;
	struct owl_clocknode *parent;
	unsigned long parentfreq;

	node = &clocks[clock];
	if (node->changed) {
		if (node->type == TYPE_PLL || node->type == TYPE_STATIC) {
			node->changed = 0;
			return;
		}
		parent = node->parent;
		if (parent) {
			if (parent->changed && parent->type == TYPE_DYNAMIC) {
				calcfrequency(parent->id);
			}
			parentfreq = parent->frequency;
			if (node->multipler) {
				parentfreq *= node->multipler;
			}
			if (node->divider) {
				parentfreq /= node->divider;
			}

			node->frequency = parentfreq;
			node->changed = 0;
		}
	}
}


static void  pllsub_putaway(int pllclock)
{
	struct owl_clocknode *pll;
	struct owl_clocknode *subnode;

	if (pllclock >= CLOCK__COREPLL && pllclock <= CLOCK__ETHERNETPLL) {
		pll = &clocks[pllclock];
		if (pll->type != TYPE_PLL) {
			return;
		}

		if (pllclock == CLOCK__DEVPLL) {
			if (pll == clocks[CLOCK__DEV_CLK].parent) {
				subnode = clocks[CLOCK__DEV_CLK].sub;
				while (subnode != NULL) {
					if (subnode->putaway_enabled && subnode->putaway_sel != subnode->source_sel) {
						if (subnode->putaway_sel == -1)
							subnode->putaway_sel = subnode->divsel;

						if (subnode->putaway_divsel > subnode->divsel)
							write_clkreg_val(subnode->actdiv->reg, subnode->putaway_sel);

						write_clkreg_val(subnode->reg_srcsel, subnode->putaway_sel);

						if (subnode->putaway_divsel < subnode->divsel)
							write_clkreg_val(subnode->actdiv->reg, subnode->putaway_sel);

						if (subnode->putback_divsel >= 0) {
							subnode->divsel = subnode->putback_divsel;
							subnode->putback_divsel = -1;
							subnode->changed = 1;
						}

                        printk("putaway clock %s from %s to %s\n", subnode->name,
                        	clocks[subnode->source_av[subnode->source_sel]].name,
                        	clocks[subnode->source_av[subnode->putaway_sel]].name);
					}

					subnode = subnode->next;
				}
			}
		}

		switch (pllclock) {
		case CLOCK__COREPLL:
		case CLOCK__DEVPLL:
		case CLOCK__DDRPLL:
		case CLOCK__NANDPLL:
		case CLOCK__DISPLAYPLL:
			subnode = pll->sub;
			while (subnode != NULL) {
				if (subnode->putaway_enabled && subnode->putaway_sel != subnode->source_sel) {
					if (subnode->putaway_sel == -1)
						subnode->putaway_sel = subnode->divsel;

					if (subnode->putaway_divsel > subnode->divsel)
						write_clkreg_val(subnode->actdiv->reg, subnode->putaway_sel);
							
					write_clkreg_val(subnode->reg_srcsel, subnode->putaway_sel);

					if (subnode->putaway_divsel < subnode->divsel)
						write_clkreg_val(subnode->actdiv->reg, subnode->putaway_sel);

					if (subnode->putback_divsel >= 0) {
						subnode->divsel = subnode->putback_divsel;
						subnode->putback_divsel = -1;
						subnode->changed = 1;
					}

                    printk("putaway clock %s from %s to %s\n", subnode->name,
                    	clocks[subnode->source_av[subnode->source_sel]].name,
                    	clocks[subnode->source_av[subnode->putaway_sel]].name);
				}

				subnode = subnode->next;
			}
			break;

		default:
			break;
		}
	}
}

static void  pllsub_resume(int pllclock)
{
	struct owl_clocknode *pll;
	struct owl_clocknode *subnode;

	if (pllclock >= CLOCK__COREPLL && pllclock <= CLOCK__ETHERNETPLL) {
		pll = &clocks[pllclock];
		if (pll->type != TYPE_PLL) {
			return;
		}
		switch (pllclock) {
		case CLOCK__COREPLL:
		case CLOCK__DEVPLL:
		case CLOCK__DDRPLL:
		case CLOCK__NANDPLL:
		case CLOCK__DISPLAYPLL:
			subnode = pll->sub;
			while (subnode != NULL) {
				if (subnode->putaway_enabled && subnode->putaway_sel != subnode->source_sel) {
					if (subnode->divsel > subnode->putaway_divsel)
						write_clkreg_val(subnode->actdiv->reg, subnode->divsel);
							
					write_clkreg_val(subnode->reg_srcsel, subnode->source_sel);

					if (subnode->divsel < subnode->putaway_divsel)
						write_clkreg_val(subnode->actdiv->reg, subnode->divsel);

					subnode->putaway_divsel = -1;

                    printk("restored clock %s from %s to %s\n", subnode->name,
                    	clocks[subnode->source_av[subnode->putaway_sel]].name,
                    	clocks[subnode->source_av[subnode->source_sel]].name);
				}

				subnode = subnode->next;
			}
			break;

		default:
			break;
		}

		if (pllclock == CLOCK__DEVPLL) {
			if (pll == clocks[CLOCK__DEV_CLK].parent) {
				subnode = clocks[CLOCK__DEV_CLK].sub;
				while (subnode != NULL) {
					if (subnode->putaway_enabled && subnode->putaway_sel != subnode->source_sel) {
						if (subnode->divsel > subnode->putaway_divsel)
							write_clkreg_val(subnode->actdiv->reg, subnode->divsel);
							
						write_clkreg_val(subnode->reg_srcsel, subnode->source_sel);

						if (subnode->divsel < subnode->putaway_divsel)
							write_clkreg_val(subnode->actdiv->reg, subnode->divsel);

						subnode->putaway_divsel = -1;

                        printk("restored clock %s from %s to %s\n", subnode->name,
                        	clocks[subnode->source_av[subnode->putaway_sel]].name,
                        	clocks[subnode->source_av[subnode->source_sel]].name);
					}

					subnode = subnode->next;
				}
			}
		}
	}
}

static int setpll(int clock, int freq)
{
	int pll;
	int index;
	struct owl_pll *node;

	if (clock >= 0 && clock < CLOCK__MAX) {
		if (clocks[clock].type == TYPE_PLL) {
			pll = clock - CLOCK__COREPLL;
			if (pll < 0 || pll >= PLL__MAX) {
				goto fail;
			}
			node = &pllnode[pll];
			switch (node->type) {
			case PLL_T_STEP:
			case PLL_T_D4DYN:
				index = (freq / node->freq.step.step) - node->freq.step.offset;
				if ((node->freq.step.step + node->freq.step.offset) * index != freq) {
					goto fail;
				}
				if (index < node->range_from) {
					goto fail;
				}
				if (index > node->range_to) {
					goto fail;
				}
				node->sel = index;
				break;
			case PLL_T_FREQ:
				for (index = node->range_from; index <= node->range_to; index++) {
					if (node->freq.freqtab[index] == freq) {
						goto found;
					}
				}
				goto fail;

				found:
				node->sel = index;
				break;
			default:
				break;
			}
			if (clocks[clock].frequency == freq) {
				return 0;
			}

			if (node->reg_pllfreq) {
				pllsub_putaway(clock);
				write_clkreg_val(node->reg_pllfreq, node->sel);

				if (node->delay == 0)
					udelay(PLLDELAY);
				else
					udelay(node->delay);
				pllsub_resume(clock);
			}

			clocks[clock].frequency = freq;
			changeclock(clock);
			if (pll == PLL__TVOUTPLL) {
				pllnode[PLL__DEEPCOLORPLL].freq.step.step =  freq / 4;
				clocks[CLOCK__DEEPCOLORPLL].frequency =
					pllnode[PLL__DEEPCOLORPLL].freq.step.step
					* pllnode[PLL__DEEPCOLORPLL].sel;
				clocks[CLOCK__DEEPCOLORPLL].changed = 0;
			}
			return 0;
		}
	}
fail:
	return -1;
}

int setcorepll(int clock, int freq)
{
	int pll;
	int index;
	struct owl_pll *node;
	struct owl_clocknode *core_clk;
	int swap;
	unsigned long flags;

	if (clock == CLOCK__COREPLL) {
		if (clocks[clock].frequency == freq) {
			return 0;
		}

		pll = clock - CLOCK__COREPLL;
		if (pll != 0) {
			goto fail;
		}
		node = &pllnode[pll];

		index = freq / node->freq.step.step;
		if (node->freq.step.step * index != freq) {
			goto fail;
		}
		if (index < node->range_from) {
			goto fail;
		}
		if (index > node->range_to) {
			index = node->range_to;
			goto fail;
		}
		node->sel = index;

		spin_lock_irqsave(&cpu_lock, flags);

		core_clk = &clocks[CLOCK__CORE_CLK];
		swap = 0;
		if (core_clk->parent == &clocks[clock]) {
			sourcesel(CLOCK__CORE_CLK, COREPLL_CHANGE_MIDDLE_CLK);
			swap = 1;
		}

		if (node->reg_pllfreq) {
			write_clkreg_val(node->reg_pllfreq, node->sel);
			if (node->delay == 0)
				udelay((PLLDELAY+COREPLL_CHANGE_DELAY_DIV-1)/COREPLL_CHANGE_DELAY_DIV);
			else
				udelay((node->delay+COREPLL_CHANGE_DELAY_DIV-1)/COREPLL_CHANGE_DELAY_DIV);
		}

		clocks[clock].frequency = freq;
		changeclock(clock);

		if (swap) {
			sourcesel(CLOCK__CORE_CLK, CLOCK__COREPLL);
		}

		spin_unlock_irqrestore(&cpu_lock, flags);
		return 0;
	}
fail:
	return -1;
}

static int setdivider(int clock, int divider, int multipler)
{
	int sel;
	int divexp;
	int clocksync;
	struct owl_clkreq *reg;

	if (clock >= 0 && clock < CLOCK__MAX) {
		if (clocks[clock].type == TYPE_DYNAMIC) {
			if (clocks[clock].actdiv) {
				if (multipler) {
					divexp = multipler << 16 | divider;
				} else {
					divexp = divider;
				}

				sel = getdivider_index(clocks[clock].actdiv, divexp);
				if (sel >= 0) {
					clocks[clock].divider = divider;
					clocks[clock].multipler = multipler;
					clocks[clock].divsel = sel;

					reg = clocks[clock].actdiv->reg;
					write_clkreg_val(reg, sel);

					changeclock(clock);

					switch (clock) {
					case CLOCK__CLK_TMDS:
						clocksync = CLOCK__CLK_PIXEL;
						goto share_divider_sync;
					case CLOCK__CLK_PIXEL:
						clocksync = CLOCK__CLK_TMDS;
						goto share_divider_sync;
					default:
						break;
					share_divider_sync:
						clocks[clocksync].divider = divider;
						clocks[clocksync].multipler = multipler;
						clocks[clocksync].divsel = sel;
						changeclock(clocksync);
					}
				} else {
					return -1;
				}
			}
		}
		return 0;
	}
	return -1;
}

static int clocksel(int clock, int src)
{
	int ret;

	if (clock >= 0 && clock < CLOCK__MAX && src >= 0 && src < CLOCK__MAX) {
		ret = sourcesel(clock, src);
		return ret;
	}
	return -1;
}


/*
 * clk_ops function imp
 */
static unsigned long clkops_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	unsigned long rate;
	int divider;
	int multipler;
	struct owl_div;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;
	if (clock >= 0 && clock < CLOCK__MAX) {
		if (clocks[clock].changed) {
			calcfrequency(clock);
		}

		divider = clocks[clock].divider;
		multipler = clocks[clock].multipler;
		rate = parent_rate / divider;
		if (multipler) {
			rate *= multipler;
		}
		return rate;
	}

	/* fail */
	return 0;
}

static long clkops_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *parent_rate)
{
	static int pro_clk_divisor_3x[4] = {3, 4, 6, 12};
	int index;
	struct owl_div *div;
	int divider;
	int divexp, restnum, divnum, mulnum;
	int multipler = 0;
	int ret = -1;
	int parent;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_DYNAMIC) {
		div = clocks[clock].actdiv;
		if (!div)
			goto error;

		if (clocks[clock].changed) {
			calcfrequency(clock);
		}

		if (!clocks[clock].parent)
			goto error;

		parent =  clocks[clock].parent->frequency;

		if (clock == CLOCK__PRO_CLK)
			goto pro_clk;

		if (rate == 0) {
			pr_err("ERROR: requesting clock rate 0Hz");
			goto error;
		}
		divexp = parent / rate;
		restnum = parent - (rate * divexp);
		if(restnum != 0) {
			divnum = parent / restnum;
			if(restnum * divnum != parent)
				goto other;
			mulnum = rate / restnum;
			if(restnum * mulnum != rate)
				goto other;
			divexp = mulnum << 16 | divnum;
		} 
other:
		if ((divexp & 0xffff0000) == 0 && parent > divexp * (rate + 1))
			divexp++;

		divider = getdivider_ge(div, divexp);
		if (divider > 0) {
			if (divider & 0xffff0000) {
				multipler = divider >> 16;
				divider &= 0xffff;
				return parent / divider * multipler;
			}
			return parent / divider;
		} else {
			ret = -1;
			goto error;
		}
	}
	parent = 0;
pro_clk:
	divexp = parent * 3 / rate;
	if (parent * 3 > divexp * rate)
		divexp++;

	for (index = 0; index < 4; index++) {
		if (pro_clk_divisor_3x[index] >= divexp)
			break;
	}
	if (index < 4) {
		divider = pro_clk_divisor_3x[index];
		multipler = 3;
		return parent * multipler / divider;
	} else {
		ret = -1;
	}
error:
	if (ret < 0) {
		ret = 0;
	}
	return ret;
}

static int  clkops_set_parent(struct clk_hw *hw, unsigned char index)
{
	int ret;
	int clock;
	int src;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (index < clocks[clock].source_lim) {
		src = clocks[clock].source_av[index];
		ret = clocksel(clock, src);
		if (ret < 0) {
			ret = -ENOENT;
		}
		return ret;
	}
	ret = -ENOENT;
	return ret;
}

static unsigned char clkops_get_parent(struct clk_hw *hw)
{
	int ret = -1;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX) {
		ret = clocks[clock].source_sel;
	}
	if (ret < 0) {
		ret = -ENOENT;
	}
	return ret;
}

static int  clkops_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	int index;
	struct owl_div *div;
	int divider;
	int multipler = 0;
	int ret = -1;
	int divexp, restnum, divnum, mulnum;
	int parent;
	int clock;
	struct owl_clk_foo *foo;


	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_DYNAMIC) {
		div = clocks[clock].actdiv;
		if (!div)
			goto error;

		if (clocks[clock].changed) {
			calcfrequency(clock);
		}
		if (!clocks[clock].parent)
			goto error;
		parent =  clocks[clock].parent->frequency;
		if (rate == 0) {
			pr_err("ERROR: requesting clock rate 0Hz");
			goto error;
		}
		divexp = parent / rate;
		restnum = parent - (rate * divexp);
		if(restnum != 0) {
			divnum = parent / restnum;
			if(restnum * divnum != parent)
				goto divint;
			mulnum = rate / restnum;
			if(restnum * mulnum != rate)
				goto divint;
			divexp = mulnum << 16 | divnum;
		}
divint:

		index = getdivider_index(div, divexp);
		if (index >= 0) {
			divider = getdivider(div, index);
			if (divider <= 0)
				goto error;

			if (divider > 0 && (divider & 0xffff0000)) {
				multipler = divider >> 16;
				divider &= 0xffff;
			}
			ret = setdivider(clock, divider, multipler);
			if (ret == 0) {
				calcfrequency(clock);
			}
		}
	}
error:
	if (ret < 0) {
		ret = -ENOENT;
	}
	return ret;
}

static long parent_round_rate(int clock, unsigned long rate, unsigned long *best_parent)
{
	struct clk *parent;
	int index;
	long l;

	index = clocks[clock].source_av[clocks[clock].source_sel];
	parent = &owl_clks[index];

	if (parent->ops->round_rate) {
		l = parent->ops->round_rate(parent->hw, rate, best_parent);
		return l;
	}
	if (clocks[index].changed) {
		calcfrequency(index);
	}
	l =  clocks[index].frequency;
	return l;
}

static long clkops_b_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *parent_rate)
{
	struct owl_div *div;
	struct owl_div tmpdiv;
	struct owl_refertab tab;
	int divider;
	int multipler;
	int dividerlim;
	int i;
	int ret = -1;
	int clock;
	struct owl_clk_foo *foo;
	unsigned long bp;
	unsigned long best = 0;
	unsigned long best_parent;
	unsigned long tmp;

	if (parent_rate == NULL) {
		return clkops_round_rate(hw, rate, NULL);
	}

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_DYNAMIC) {

		div = clocks[clock].actdiv;
		if (!div) {
			tmpdiv.type = DIV_T_TABLE;
			tmpdiv.range_from = 0;
			tmpdiv.range_to = 0;
			tmpdiv.ext.tab = &tab;
			tmpdiv.ext.tab->div[0] = clocks[clock].divider;
			div = &tmpdiv;
		}

		if (rate == 0) {
			pr_err("ERROR: requesting clock rate 0Hz");
			goto error;
		}
		i = 0;
		dividerlim = 0xffffffff / rate;
		divider = getdivider(div, i);
		while (divider > 0) {
			unsigned long pr;
			unsigned long pr_want;

			multipler = divider >> 16;
			if (multipler == 0 &&  divider > dividerlim)
				break;
			if (multipler > 0) {
				divider &=  0xffff;
				if (divider > dividerlim * multipler)
					break;
			}

			if (multipler == 0) {
				pr_want = rate * divider + divider - 1;
				pr = parent_round_rate(clock, pr_want, &bp);
				tmp = pr / divider;
			} else {
				pr_want = rate / multipler * divider + divider / multipler;
				pr_want += rate % multipler * divider / multipler;
				pr = parent_round_rate(clock, pr_want, &bp);
				tmp = pr / divider * multipler;
			}

			if (tmp >= best && tmp <= rate) {
				best = tmp;
				best_parent = pr;
			}

			i++;
			divider = getdivider(div, i);
		}
		if (best > 0) {
			ret = best;
			*parent_rate = best_parent;
		}
	}

error:
	if (ret < 0) {
		ret = 0;
	}
	return ret;
}

static unsigned long best_newrate(struct owl_clocknode *node, unsigned long parent_rate)
{
	int nextdiv;
	int lookup;
	int divider;
	int multipler;
	unsigned long rate;
	unsigned long bestrate;

	bestrate = parent_rate/node->divider;
	if (node->actdiv == NULL) {
		return bestrate;
	}

	if (node->id == CLOCK__PRO_CLK) {
		for (lookup = 0; ; lookup++) {
			divider = getdivider(node->actdiv, lookup);
			if (divider < 0) {
				goto over;
			}
			multipler = divider >> 16;
			if (multipler > 0) {
				divider &= 0xffff;
			} else {
				multipler = 1;
			}

			rate = parent_rate / divider * multipler;
			if (node->frequency <= bestrate && rate < bestrate) {
				bestrate = rate;
			} else if (bestrate <= rate && rate < node->frequency) {
				bestrate = rate;
			}
		}
		goto over;
	}

	nextdiv = parent_rate/node->frequency;

	if (nextdiv > node->divider) {
		for (lookup = node->divsel + 1; ; lookup++) {
			divider = getdivider(node->actdiv, lookup);
			if (divider < 0) {
				goto over;
			}
			multipler = divider >> 16;
			if (multipler > 0) {
				divider &= 0xffff;
			} else {
				multipler = 1;
			}

			rate = parent_rate / divider * multipler;
			if (rate >= node->frequency && rate < bestrate) {
				bestrate = rate;
			} else {
				bestrate = rate;
				goto over;
			}
		}
	} else if (nextdiv < node->divider) {
		for (lookup = node->divsel - 1; lookup >= 0; lookup--) {
			divider = getdivider(node->actdiv, lookup);
			if (divider < 0) {
				goto over;
			}
			multipler = divider >> 16;
			if (multipler > 0) {
				divider &= 0xffff;
			} else {
				multipler = 1;
			}

			rate = parent_rate / divider * multipler;
			if (rate <= node->frequency && rate > bestrate) {
				bestrate = rate;
			} else {
				goto over;
			}
		}
	}

over:
	return bestrate;
}

static unsigned long clkops_b_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	unsigned long rate;
	int divider;
	int multipler;
	struct owl_div;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;
	if (clock >= 0 && clock < CLOCK__MAX) {
		if (clocks[clock].changed) {
			calcfrequency(clock);
		}

		if (clocks[clock].parent->frequency != parent_rate) {
			/* shall never be used on float divisor */
			return best_newrate(&clocks[clock], parent_rate);
		}

		divider = clocks[clock].divider;
		multipler = clocks[clock].multipler;

		rate = parent_rate / divider;
		if (multipler) {
			rate *= multipler;
		}
		return rate;
	}

	/* fail */
	return 0;
}


static int  pllops_enable(struct clk_hw *hw)
{
	int pll;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_PLL) {
		pll = clock - CLOCK__COREPLL;
		if (pll >= 0 && pll < PLL__MAX) {
			write_clkreg_val(pllnode[pll].reg_pllen, 1);
			if (pllnode[pll].delay == 0)
				udelay(PLLDELAY);
			else
				udelay(pllnode[pll].delay);
			return 0;
		}
	}
	return -ENOENT;
}

static void pllops_disable(struct clk_hw *hw)
{
	int pll;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_PLL) {
		pll = clock - CLOCK__COREPLL;
		if (pll >= 0 && pll < PLL__MAX) {
			write_clkreg_val(pllnode[pll].reg_pllen, 0);
		}
	}
}

static int  pllops_is_enabled(struct clk_hw *hw)
{
	int pll;
	int ret;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_PLL) {
		pll = clock - CLOCK__COREPLL;
		if (pll >= 0 && pll < PLL__MAX) {
			ret = read_clkreg_val(pllnode[pll].reg_pllen);
			return ret;
		}
	}
	return 0;
}

static unsigned long pllops_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	unsigned long ret;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	ret = clocks[clock].frequency;
	return ret;
}

static long pllops_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *parent_rate)
{
	int pll;
	int index;
	long best;
	struct owl_pll *node;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	pll = clock - CLOCK__COREPLL;
	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_PLL) {
		pll = clock - CLOCK__COREPLL;
		if (pll >= 0 && pll < PLL__MAX) {
			node = &pllnode[pll];
		} else
			goto error;
	} else
		goto error;

	switch (node->type) {
	case PLL_T_STEP:
	case PLL_T_D4DYN:
		index = (rate / node->freq.step.step) - node->freq.step.offset;
		if (index >= node->range_from && index <= node->range_to) {
			return (index + node->freq.step.offset) * node->freq.step.step;
		}
		if (index > node->range_to) {
			return (node->range_to + node->freq.step.offset) * node->freq.step.step;
		}
		return 0;
	case PLL_T_FREQ:
		best = 0;
		for (index = node->range_from; index <= node->range_to; index++) {
			if (node->freq.freqtab[index] > best
				&& node->freq.freqtab[index] < rate) {
				best = node->freq.freqtab[index];
			}
			if (node->freq.freqtab[index] == rate) {
				return rate;
			}
		}
		if (best > 0) {
			return best;
		}
		break;

	default:
		break;
	}

error:
	return -ENONET;
}


static int  pllops_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	int ret = -1;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_PLL) {
		ret = setpll(clock, rate);
		if (ret < 0) {
			ret = -ENOENT;
		}
		return ret;
	}
	if (ret < 0) {
		ret = -ENOENT;
	}
	return ret;
}

static int  pllops_corepll_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	int ret = -1;
	int clock;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	clock = foo->clock;

	if (clock >= 0 && clock < CLOCK__MAX && clocks[clock].type == TYPE_PLL) {
		ret = setcorepll(clock, rate);
		if (ret < 0) {
			ret = -ENOENT;
		}
		return ret;
	}
	if (ret < 0) {
		ret = -ENOENT;
	}
	return ret;
}

static int  modops_enable(struct clk_hw *hw)
{
	int ret = -1;
	int module;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	module  = foo->clock;

	if (module >= 0 && module < MOD__MAX_IN_CLK) {
		write_clkreg_val(modnode[module].reg_devclken, 1);
		ret = 0;
	}
	if (ret < 0) {
		ret = -ENOENT;
	}
	return ret;
}

static void modops_disable(struct clk_hw *hw)
{
	int module;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	module  = foo->clock;

	if (module >= 0 && module < MOD__MAX_IN_CLK) {
		write_clkreg_val(modnode[module].reg_devclken, 0);
	}
}

static int  modops_is_enabled(struct clk_hw *hw)
{
	int ret = -1;
	int module;
	struct owl_clk_foo *foo;

	foo = to_clk_foo(hw);
	module  = foo->clock;

	if (module >= 0 && module < MOD__MAX_IN_CLK) {
		ret = read_clkreg_val(modnode[module].reg_devclken);
	}
	if (ret < 0) {
		ret = 0;
	}
	return ret;
}


struct clk_ops clk_ops_corepll = {
	.prepare = pllops_enable,
	.unprepare = pllops_disable,
	.is_enabled = pllops_is_enabled,
	.recalc_rate = pllops_recalc_rate,
	.round_rate = pllops_round_rate,
	.set_rate = pllops_corepll_set_rate,
};

struct clk_ops clk_ops_pll = {
	.prepare = pllops_enable,
	.unprepare = pllops_disable,
	.is_enabled = pllops_is_enabled,
	.recalc_rate = pllops_recalc_rate,
	.round_rate = pllops_round_rate,
	.set_rate = pllops_set_rate,
};

struct clk_ops clk_ops_gate_module = {
	.enable = modops_enable,
	.disable = modops_disable,
	.is_enabled = modops_is_enabled,
};

struct clk_ops clk_ops_direct_s_parent = {
};

struct clk_ops clk_ops_direct_m_parent = {
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};

struct clk_ops clk_ops_s_divider_s_parent = {
	.recalc_rate = clkops_recalc_rate,
};

struct clk_ops clk_ops_s_divider_m_parent = {
	.recalc_rate = clkops_recalc_rate,
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};

struct clk_ops clk_ops_m_divider_s_parent = {
	.recalc_rate = clkops_recalc_rate,
	.round_rate = clkops_round_rate,
	.set_rate = clkops_set_rate,
};

struct clk_ops clk_ops_m_divider_m_parent = {
	.recalc_rate = clkops_recalc_rate,
	.round_rate = clkops_round_rate,
	.set_rate = clkops_set_rate,
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};

struct clk_ops clk_ops_foo = {
};


struct clk_ops clk_ops_b_s_divider_s_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_b_round_rate,
};

struct clk_ops clk_ops_b_s_divider_m_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_b_round_rate,
	.set_rate = clkops_set_rate,
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};

struct clk_ops clk_ops_b_m_divider_s_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_b_round_rate,
	.set_rate = clkops_set_rate,
};

struct clk_ops clk_ops_b_m_divider_m_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_b_round_rate,
	.set_rate = clkops_set_rate,
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};


struct clk_ops clk_ops_h_s_divider_s_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_round_rate,
};

struct clk_ops clk_ops_h_s_divider_m_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_round_rate,
	.set_rate = clkops_set_rate,
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};

struct clk_ops clk_ops_h_m_divider_s_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_round_rate,
	.set_rate = clkops_set_rate,
};

struct clk_ops clk_ops_h_m_divider_m_parent = {
	.recalc_rate = clkops_b_recalc_rate,
	.round_rate = clkops_round_rate,
	.set_rate = clkops_set_rate,
	.set_parent = clkops_set_parent,
	.get_parent = clkops_get_parent,
};

static struct clk_lookup cl_smp_twd = {
	.dev_id = "smp_twd",
};

int owl_pllsub_set_putaway(int clock, int source)
{
	struct owl_clocknode *node;
	int i;

	if (clock < 0 || clock > CLOCK__MAX)
		return -1;

	node = &clocks[clock];
	if (source == -1) {
		node->putaway_enabled = 0;
	} else {
		for (i = 0; i < node->source_lim; i++) {
			if (node->source_av[i] == source) {
				node->putaway_sel = i;
				node->putaway_enabled = 1;
				node->putaway_divsel = -1;
				node->putback_divsel = -1;
				return 0;
			}
		}
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(owl_pllsub_set_putaway);


unsigned long owl_get_putaway_parent_rate(struct clk *clk)
{
	struct owl_clk_foo *foo;
	struct owl_clocknode *node;
	int away_parent_clock;

	foo = to_clk_foo(clk->hw);
	if (foo >= &clk_foo_clocks[CLOCK__MAX])
		return 0;

	node = &clocks[foo->clock];

	if (node->putaway_enabled) {
		away_parent_clock = node->source_av[node->putaway_sel];

		if (clocks[away_parent_clock].changed)
			calcfrequency(away_parent_clock);

		return clocks[away_parent_clock].frequency;
	}

	return 0;
}
EXPORT_SYMBOL(owl_get_putaway_parent_rate);


unsigned long owl_getparent_newrate(struct clk *clk)
{
	if (clk->parent)
		return clk->parent->new_rate;

	return 0;
}
EXPORT_SYMBOL(owl_getparent_newrate);


int owl_getdivider_index(struct clk *clk, int divexp)
{
	struct owl_clk_foo *foo;
	struct owl_clocknode *node;
	int index;

	foo = to_clk_foo(clk->hw);
	if (foo >= &clk_foo_clocks[CLOCK__MAX])
		return -1;

	node = &clocks[foo->clock];
	index = getdivider_index(node->actdiv, divexp);
	return index;
}
EXPORT_SYMBOL(owl_getdivider_index);


int owl_set_putaway_divsel(struct clk *clk, int tmp_divsel, int new_divsel)
{
	struct owl_clk_foo *foo;
	struct owl_clocknode *node;
	
	foo = to_clk_foo(clk->hw);
	if (foo >= &clk_foo_clocks[CLOCK__MAX])
		return -1;

	node = &clocks[foo->clock];
	node->putaway_divsel = tmp_divsel;
	node->putback_divsel = new_divsel;
	
	if (node->putaway_enabled)
		return 0;
	
	return -1;
}
EXPORT_SYMBOL(owl_set_putaway_divsel);


extern void clk_calc_subtree(struct clk *clk, unsigned long newrate);
void owl_update_notify_newrate(struct clk *clk, unsigned long newrate)
{
	clk_calc_subtree(clk, newrate);
}
EXPORT_SYMBOL(owl_update_notify_newrate);


int module_clk_enable(int mod_id)
{
	if (mod_id >= 0 && mod_id < MOD__MAX) {
		if (modnode[mod_id].reg_devclken) {
			write_clkreg_val(modnode[mod_id].reg_devclken, 1);
		}
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(module_clk_enable);

int module_clk_disable(int mod_id)
{
	if (mod_id >= 0 && mod_id < MOD__MAX) {
		if (modnode[mod_id].reg_devclken) {
			write_clkreg_val(modnode[mod_id].reg_devclken, 0);
		}
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(module_clk_disable);

static int __module_reset(int mod_id, int holding, int holding_assert)
{
	if (mod_id >= 0 && mod_id < MOD__MAX) {
		if (modnode[mod_id].reg_devrst) {

			if (!holding || holding_assert) {
				write_clkreg_val(modnode[mod_id].reg_devrst, 0);
				read_clkreg_val(modnode[mod_id].reg_devrst);
//				mdelay(MODULE_RESET_TIME_MS);
			}

			if (!holding || !holding_assert) {
				write_clkreg_val(modnode[mod_id].reg_devrst, 1);
				read_clkreg_val(modnode[mod_id].reg_devrst);
//				mdelay(MODULE_RESET_TIME_MS);
			}

		}
		return 0;
	}
	return -1;
}

int module_reset(int modid)
{
	return __module_reset(modid, 0, 0);
}
EXPORT_SYMBOL(module_reset);

int owl_module_reset_assert(int modid)
{
	return __module_reset(modid, 1, 1);
}
EXPORT_SYMBOL(owl_module_reset_assert);

int owl_module_reset_deassert(int modid)
{
	return __module_reset(modid, 1, 0);
}
EXPORT_SYMBOL(owl_module_reset_deassert);

static struct clk_ops *clkops_select_version(const struct clk_ops *ops, int version)
{
	static struct clk_ops *tab[4][3] = {
		{&clk_ops_s_divider_s_parent, &clk_ops_b_s_divider_s_parent, &clk_ops_h_s_divider_s_parent},
		{&clk_ops_s_divider_m_parent, &clk_ops_b_s_divider_m_parent, &clk_ops_h_s_divider_m_parent},
		{&clk_ops_m_divider_s_parent, &clk_ops_b_m_divider_s_parent, &clk_ops_h_m_divider_s_parent},
		{&clk_ops_m_divider_m_parent, &clk_ops_b_m_divider_m_parent, &clk_ops_h_m_divider_m_parent},
	};

	int i;
	for (i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
		if (ops == tab[i][0] || ops == tab[i][1] || ops == tab[i][2]) {
			if (version == 0) {
				return tab[i][0];
			} else if (version == 1) {
				return tab[i][1];
			} else if (version == 2) {
				return tab[i][2];
			}
			return NULL;
		}
	}
	return NULL;
}


int owl_clk_config_recursion(int clock, int recursion)
{
	const struct clk_ops *ops;
	if (clock >= 0 && clock <= CLOCK__MAX) {
		ops = owl_clks[clock].ops;
		if (ops == &clk_ops_direct_s_parent || ops == &clk_ops_direct_m_parent) {
			if (recursion) {
				owl_clks[clock].flags |= CLK_SET_RATE_PARENT;
			} else {
				owl_clks[clock].flags &=  ~CLK_SET_RATE_PARENT;
			}
			return 0;
		}

		ops = clkops_select_version(owl_clks[clock].ops, recursion);
		if (ops == NULL) {
			return -1;
		}
		if (ops == owl_clks[clock].ops) {
			return 0;
		}
		if (recursion == 1) {
			owl_clks[clock].flags |= CLK_SET_RATE_PARENT;
		} else {
			owl_clks[clock].flags &=  ~CLK_SET_RATE_PARENT;
		}
		owl_clks[clock].ops = ops;
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(owl_clk_config_recursion);


static void __init init_clocktree(void)
{
    struct clocks_table *clks_table;

	if (of_machine_is_compatible("actions,atm7059a")) {
        clks_table = atm7059_get_clocktree();
        clocks = clks_table->clocks;
        rvregs = clks_table->rvregs;
        pllnode = clks_table->pllnode;
        owl_clks = clks_table->owl_clks;
        modnode = clks_table->modnode;
        clk_foo_clocks = clks_table->clk_foo_clocks;
        lookup_table = clks_table->lookup_table;

        atm7059_init_clocktree();
    }
}

static int owl_limit_notify(struct notifier_block *nb, unsigned long action, void *data)
{
	struct clk_notifier_data *cnd;
	struct owl_clk_foo *foo;
	int clock;

	if (action == PRE_RATE_CHANGE) {
		cnd = data;
		foo = to_clk_foo(cnd->clk->hw);
		clock = foo->clock;

		if (clock == CLOCK__NIC_CLK) {
			if (cnd->new_rate > 300 * MEGA) {
				printk("NIC_CLK is too fast to set to %lu!\n", cnd->new_rate);
				return NOTIFY_BAD;
			} else if (cnd->new_rate > 250 * MEGA) {
				printk("warning: NIC_CLK is set to %lu!\n", cnd->new_rate);
			} else if (cnd->new_rate < 180 * MEGA) {
				printk("warning: NIC_CLK is set to %lu!\n", cnd->new_rate);
			}
		} else if (clock == CLOCK__NIC_DIV_CLK) {
			if (cnd->new_rate > 150 * MEGA) {
				printk("NIC_DIV_CLK cannot be set to %lu!\n", cnd->new_rate);
				return NOTIFY_BAD;
			}
		} else if (clock == CLOCK__H_CLK) {
			if (cnd->new_rate > 125 * MEGA) {
				printk("H_CLK cannot be set to %lu!\n", cnd->new_rate);
				return NOTIFY_BAD;
			} else if (cnd->new_rate > 100 * MEGA) {
				printk("warning: H_CLK is set to %lu\n", cnd->new_rate);
			}
		}
	} else if (action == POST_RATE_CHANGE) {
		cnd = data;
		foo = to_clk_foo(cnd->clk->hw);
		clock = foo->clock;

		if (clock == CLOCK__APBDBG_CLK
			|| clock == CLOCK__ACP_CLK
			|| clock == CLOCK__PERIPH_CLK) {
			if (clocks[CLOCK__APBDBG_CLK].divider != 8
				|| clocks[CLOCK__ACP_CLK].divider != 4
				|| clocks[CLOCK__PERIPH_CLK].divider != 8) {
				printk("clocktree: WARNING! ...................\n");
				printk("APBDBG_CLK: ACP_CLK: PERIPH_CLK\n");
				printk("should be /8   /4   /8\n");
				printk("but is set to /%d  /%d  /%d\n",
					clocks[CLOCK__APBDBG_CLK].divider,
					clocks[CLOCK__ACP_CLK].divider,
					clocks[CLOCK__PERIPH_CLK].divider);
			}
		}
	}

	return NOTIFY_OK;
}

static int pll_in_change;

static int owl_pll_notify(struct notifier_block *nb, unsigned long action, void *data)
{

	if (action == PRE_RATE_CHANGE)
		pll_in_change = 1;
	else
		pll_in_change = 0;

	return NOTIFY_OK;
}

int owl_pll_in_change(void)
{
	return pll_in_change;
}

EXPORT_SYMBOL(owl_pll_in_change);


static struct {
	struct notifier_block nic_clk;
	struct notifier_block apbdbg_clk;
	struct notifier_block acp_clk;
	struct notifier_block periph_clk;
	struct notifier_block h_clk;

	struct notifier_block devpll;
	struct notifier_block displaypll;
	struct notifier_block tvoutpll;
	struct notifier_block deepcolorpll;
	struct notifier_block audiopll;
} owl_clk_nb  = {
	{ .notifier_call = owl_limit_notify, },
	{ .notifier_call = owl_limit_notify, },
	{ .notifier_call = owl_limit_notify, },
	{ .notifier_call = owl_limit_notify, },
	{ .notifier_call = owl_limit_notify, },

	{ .notifier_call = owl_pll_notify, },
	{ .notifier_call = owl_pll_notify, },
	{ .notifier_call = owl_pll_notify, },
	{ .notifier_call = owl_pll_notify, },
	{ .notifier_call = owl_pll_notify, },
};

static void __init prepare_clocktree(void)
{
	if (of_machine_is_compatible("actions,atm7059a")) {
		atm7059_prepare_clocktree();
	}
}

int __init owl_init_clocks(void)
{
	int i;
    
	spin_lock_init(&cpu_lock);

	init_clocktree();

	for (i = 0; i < CLOCK__MAX + MOD__MAX_IN_CLK; i++) {
	    if(owl_clks[i].ops)
    		__clk_init(NULL, &owl_clks[i]);
	}

	prepare_clocktree();

	for (i = 0; i < CLOCK__MAX; i++) {
		calcfrequency(i);
		owl_clks[i].rate = clocks[i].frequency;
	}

	clkdev_add_table(lookup_table, CLOCK__MAX + MOD__MAX_IN_CLK);
	
	cl_smp_twd.clk = &owl_clks[CLOCK__PERIPH_CLK],
	clkdev_add(&cl_smp_twd);

	return 0;
}

/* arch_initcall(owl_init_clocks); moved init code to machine .init_early */

static int __init init_notifier(void)
{
	clk_notifier_register(&owl_clks[CLOCK__NIC_CLK], &owl_clk_nb.nic_clk);
	clk_notifier_register(&owl_clks[CLOCK__APBDBG_CLK], &owl_clk_nb.apbdbg_clk);
	clk_notifier_register(&owl_clks[CLOCK__ACP_CLK], &owl_clk_nb.acp_clk);
	clk_notifier_register(&owl_clks[CLOCK__PERIPH_CLK], &owl_clk_nb.periph_clk);
	clk_notifier_register(&owl_clks[CLOCK__H_CLK], &owl_clk_nb.h_clk);

	clk_notifier_register(&owl_clks[CLOCK__DEVPLL], &owl_clk_nb.devpll);
	clk_notifier_register(&owl_clks[CLOCK__DISPLAYPLL], &owl_clk_nb.displaypll);
	clk_notifier_register(&owl_clks[CLOCK__TVOUTPLL], &owl_clk_nb.tvoutpll);
	clk_notifier_register(&owl_clks[CLOCK__DEEPCOLORPLL], &owl_clk_nb.deepcolorpll);
	clk_notifier_register(&owl_clks[CLOCK__AUDIOPLL], &owl_clk_nb.audiopll);
	return 0;
}

arch_initcall(init_notifier);

/*******************/
#include <linux/debugfs.h>

static ssize_t corepll_delay_write(struct file *filp, const char __user *buffer,
        size_t count, loff_t *ppos)
{
	unsigned int corepll_delay;

	corepll_delay = simple_strtoul(buffer, NULL, 10);

	pllnode[PLL__COREPLL].delay = corepll_delay;

	return count;
}

static ssize_t corepll_delay_read(struct file *filp, char __user *buffer,
        size_t count, loff_t *ppos)
{
	printk(KERN_WARNING "core pll delay = %d\n", pllnode[PLL__COREPLL].delay);
	return 0;
}

static int corepll_delay_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

static struct file_operations corepll_delay_fops = {
	.open = corepll_delay_open,
	.read = corepll_delay_read,
	.write = corepll_delay_write,
};

int __init clocktree_debug_init(void)
{
	struct dentry *dir;
	struct dentry *d;

	dir = debugfs_create_dir("clocktree", NULL);
	if (!dir)
		return -ENOMEM;

	d = debugfs_create_file("corepll_delay", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dir, NULL,
		&corepll_delay_fops);
	if (!d)
		return -ENOMEM;

	return 0;
}
module_init(clocktree_debug_init);
