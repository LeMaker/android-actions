#include <common.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_io.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>
#include <asm/arch/owl_clk.h>
#include <asm/arch/owl_afi.h>




#define BUSCLK_NICDIVDIV_MASK	(0x3 << 18)
#define BUSCLK_NICDIVDIV_D1	(0x0 << 18)
#define BUSCLK_NICDIVDIV_D2	(0x1 << 18)
#define BUSCLK_NICDIVDIV_D3	(0x2 << 18)
#define BUSCLK_NICDIVDIV_D4	(0x3 << 18)


#define BUSCLK_PDBGDIV_MASK	(0x7 << 26)
#define BUSCLK_PERDIV_MASK	(0x7 << 20)
#define BUSCLK_NICSRC_MASK	(0x7 << 4)
#define BUSCLK_DIVEN		(0x1 << 31)
#define BUSCLK_USE_HOSC		0x1
#define BUSCLK_USE_COREPLL	0x2
#define BUSCLK_USE_MASK		0x3

#define BUSCLK_NICDIV_SHIFT	16
#define BUSCLK_NICDIV_MASK	(0x3 << BUSCLK_NICDIV_SHIFT)
#define BUSCLK_NICDIV_D1	(0x0 << BUSCLK_NICDIV_SHIFT)
#define BUSCLK_NICDIV_D2	(0x1 << BUSCLK_NICDIV_SHIFT)
#define BUSCLK_NICDIV_D3	(0x2 << BUSCLK_NICDIV_SHIFT)
#define BUSCLK_NICDIV_D4	(0x3 << BUSCLK_NICDIV_SHIFT)

#define DEVCLKSS_MASK		(0x1 << 12)
#define DEVCLKSS_HOSC		(0x0 << 12)
#define DEVCLKSS_DEVPLL		(0x1 << 12)
#define DEVPLLEN		(0x1 << 8)
#define DEVPLLCLK_MASK		(0x7F << 0)



#define DISPLAYPLLEN		(0x1 << 8)
#define DISPLAYPLL_FRQ_MHZ		(612)
#define HOSC_CLKRATE_HZ	24000000

DECLARE_GLOBAL_DATA_PTR;

static void owl_clk_set(int clk_id, int enable)
{
	unsigned long reg;
	unsigned int bit;

	reg  = CMU_DEVCLKEN0 + (clk_id / 32) * 4;
	bit = clk_id % 32;

	if (enable)
		owl_clrsetbits(reg, 1 << bit, 1 << bit);
	else
		owl_clrsetbits(reg, 1 << bit, 0);
}

void owl_clk_enable(int clk_id)
{
	owl_clk_set(clk_id, 1);
}

void owl_clk_disable(int clk_id)
{
	owl_clk_set(clk_id, 0);
}

int owl_get_displaypll_rate(void)
{
	u32 tmp;
	tmp = readl(CMU_DISPLAYPLL);

	tmp &= 0x7f;
	return tmp * 6000000;
}

int owl_get_devpll_rate(void)
{
	u32 tmp;
	tmp = readl(CMU_DEVPLL);
	tmp &= 0x7f;

	return tmp * 6000000;
}

int owl_get_devclk_rate(void)
{
	u32 tmp;
	u32 source_clk_hz;
	tmp = readl(CMU_DEVPLL);
	if ((tmp & DEVCLKSS_MASK) == DEVCLKSS_HOSC)
		source_clk_hz = HOSC_CLKRATE_HZ;
	else
		source_clk_hz = owl_get_devpll_rate();

	return source_clk_hz;

}

int owl_get_nic_clk_rate(void)
{
	u32 tmp;
	u32 source_clk_hz;
	u32 nic_div;

	tmp = readl(CMU_BUSCLK);
	tmp = (tmp & BUSCLK_NICDIV_MASK) >> BUSCLK_NICDIV_SHIFT;
	nic_div = tmp + 1;
	source_clk_hz = owl_get_devclk_rate();
	return source_clk_hz / nic_div;
}


int owl_clk_init(void)
{
	int node, mask;
	int core_pll,dev_pll,bus_pll,bus1_pll,display_pll, clk_spead;
	node = fdt_node_offset_by_compatible(gd->fdt_blob, 0, "actions,owl-clk");
	if ( node < 0 ) {
		printf("clk:get clk dts fail\n");
		return -1;
	}
	core_pll = fdtdec_get_int(gd->fdt_blob,node, "core_pll", 0x330);
	
	if (owl_get_boot_mode() == BOOT_MODE_PRODUCE) {		
		core_pll = 0x311;
		printf("product process  core pll is 0x%x!\n",core_pll);		
	}
	
	dev_pll = fdtdec_get_int(gd->fdt_blob,node, "dev_pll", 0x1164);	
	bus_pll = fdtdec_get_int(gd->fdt_blob,node, "bus_pll", 0x1c710000);
	bus1_pll = fdtdec_get_int(gd->fdt_blob,node, "bus1_pll", 0x2e004);
	display_pll = fdtdec_get_int(gd->fdt_blob,node, "display_pll", 0x164);
	clk_spead = fdtdec_get_int(gd->fdt_blob,node, "clk_spead", 0);
	printf("PLL:core=0x%x,dev=0x%x,bus=0x%x,bus1=0x%x,display=0x%x, spead=0x%x\n", 
			core_pll,dev_pll,bus_pll,bus1_pll,display_pll, clk_spead);


	owl_clk_enable(DEVCLKEN_DE);
	owl_clk_enable(DEVCLKEN_NOC1);


	writel(display_pll, CMU_DISPLAYPLL);
	udelay(200);

		/*
		 * 1. use HOSC
		 * 2. set divisors
		 * 4. set devpll
		 */
	mask = BUSCLK_PDBGDIV_MASK | BUSCLK_PERDIV_MASK 
			| BUSCLK_NICDIV_MASK | BUSCLK_NICSRC_MASK;
	owl_clrsetbits(CMU_BUSCLK, mask, bus_pll & mask);
	udelay(5);		
	writel(bus1_pll, CMU_BUSCLK1);
	
	owl_clrsetbits(CMU_BUSCLK, BUSCLK_DIVEN, BUSCLK_DIVEN);
	udelay(5);
	
	/* 1 select HOSC
	 * 2 set DEVPEN=1, and DEVPLLCLK
	 * 3 select  DEVPLL
	 * */
	owl_clrsetbits(CMU_DEVPLL, DEVCLKSS_MASK, DEVCLKSS_HOSC);
	udelay(5);
	owl_clrsetbits(CMU_DEVPLL, DEVPLLCLK_MASK,
		(dev_pll & DEVPLLCLK_MASK) | DEVPLLEN);
	udelay(70);
	owl_clrsetbits(CMU_DEVPLL, DEVCLKSS_MASK, DEVCLKSS_DEVPLL);


	if(clk_spead != 0)	{
		writel(clk_spead, CMU_DDRPLLDEBUG);
		writel(clk_spead, CMU_NANDPLLDEBUG);
		writel(clk_spead, CMU_DISPLAYPLLDEBUG);
	}
	udelay(100);
	writel(0x2a, CMU_NANDCCLK);
	udelay(100);
	writel(0x118, CMU_NANDPLL);
	udelay(100);
	writel(0x2, CMU_NANDCCLK);
	udelay(100);


	/*
	 * 1. use HOSC
	 * 2. set divisors
	 * 3. enable the divisors
	 * 4. set corepll
	 * 5. switch to the corepll
	 */

	owl_clrsetbits(CMU_BUSCLK, BUSCLK_USE_MASK | BUSCLK_DIVEN,
		BUSCLK_USE_HOSC);
	udelay(5);

	/* HOSC EN, CorePLL EN, CORECLK=120MHz */

	writel(core_pll, CMU_COREPLL);
	udelay(70);

	owl_clrsetbits(CMU_BUSCLK, BUSCLK_USE_MASK, BUSCLK_USE_COREPLL);
	udelay(5);

	return 0;

}
