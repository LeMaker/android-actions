
#include <common.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_timer.h>
#include <asm/arch/owl_io.h>
#include <asm/arch/owl_clk.h>


DECLARE_GLOBAL_DATA_PTR;


#define TIMER_CLK_24M		24
#define TIMER_CLOCK			(TIMER_CLK_24M * 1000 * 1000)
#define COUNT_TO_USEC(x)	((x) / TIMER_CLK_24M)
#define USEC_TO_COUNT(x)	((x) * TIMER_CLK_24M)
#define TICKS_PER_HZ		(TIMER_CLOCK / CONFIG_SYS_HZ)
#define TICKS_TO_HZ(x)		((x) / TICKS_PER_HZ)
#define TIMER_LOAD_VAL		0xffffffff


	
int timer_init(void)
{    
	owl_clk_enable(DEVCLKEN_TIMER);
	writel(0x0, T0_CTL);
	/* reset timer */
	writel(0x1, T0_CTL);
	writel(0x0, T0_VAL);
	writel(0x0, T0_CMP);
	/* enable timer */
	writel(0x5, T0_CTL);
	return 0;
}

ulong get_timer_masked(void)
{
	/* current tick value */
	ulong now = TICKS_TO_HZ(readl(T0_VAL));

	if (now >= gd->arch.lastinc)	/* normal (non rollover) */
		gd->arch.tbl += (now - gd->arch.lastinc);
	else			/* rollover */
		gd->arch.tbl += (TICKS_TO_HZ(TIMER_LOAD_VAL) - gd->arch.lastinc) + now;
	gd->arch.lastinc = now;
	return gd->arch.tbl;
}

void __udelay(ulong usec)
{
	long tmo = usec * (TIMER_CLOCK / 1000) / 1000;
	ulong now, last = readl(T0_VAL);

	while (tmo > 0) {
		now = readl(T0_VAL);
		if (now > last)	/* normal (non rollover) */
			tmo -= now - last;
		else		/* rollover */
			tmo -= TIMER_LOAD_VAL - last + now;
		last = now;
	}
}

ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

unsigned long long get_ticks(void)
{
	return get_timer(0);
}


ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
