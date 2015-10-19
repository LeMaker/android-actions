#include <common.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_io.h>
#include <asm/arch/owl_clk.h>

static void owl_reset_set(int rst_id, int iassert)
{
	unsigned long reg;
	unsigned int bit;

	reg  = CMU_DEVRST0 + (rst_id / 32) * 4;
	bit = rst_id % 32;

	if (iassert)
		owl_clrsetbits(reg, 1 << bit, 0);
	else
		owl_clrsetbits(reg, 1 << bit, 1 << bit);
}

void owl_reset_assert(int rst_id)
{
	owl_reset_set(rst_id, 1);
}

void owl_reset_deassert(int rst_id)
{
	owl_reset_set(rst_id, 0);
}

void owl_reset(int rst_id)
{
	owl_reset_assert(rst_id);
	udelay(1);
	owl_reset_deassert(rst_id);
}

void owl_reset_and_enable_clk(int rst_id, int clk_id)
{
	owl_reset_assert(rst_id);
	owl_clk_enable(clk_id);
	udelay(1);
	owl_reset_deassert(rst_id);
}

