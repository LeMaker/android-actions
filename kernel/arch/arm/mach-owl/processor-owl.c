#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <mach/power.h>

struct saved_context saved_context;

void __save_processor_state(struct saved_context *ctxt)
{
	preempt_disable();

	/* save coprocessor 15 registers*/
	asm volatile ("mrc p15, 2, %0, c0, c0, 0" : "=r"(ctxt->CSSR));
	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctxt->CR));
	asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r"(ctxt->AUXCR));
	asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r"(ctxt->CACR));
	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r"(ctxt->TTB_0R));
	asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r"(ctxt->TTB_1R));
	asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r"(ctxt->TTBCR));
	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r"(ctxt->DACR));
	asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r"(ctxt->D_FSR));
	asm volatile ("mrc p15, 0, %0, c5, c0, 1" : "=r"(ctxt->I_FSR));
	asm volatile ("mrc p15, 0, %0, c5, c1, 0" : "=r"(ctxt->D_AFSR));
	asm volatile ("mrc p15, 0, %0, c5, c1, 1" : "=r"(ctxt->I_AFSR));
	asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r"(ctxt->D_FAR));
	asm volatile ("mrc p15, 0, %0, c6, c0, 2" : "=r"(ctxt->I_FAR));
	asm volatile ("mrc p15, 0, %0, c7, c4, 0" : "=r"(ctxt->PAR));
	asm volatile ("mrc p15, 0, %0, c9, c12, 0" : "=r"(ctxt->PMControlR));
	asm volatile ("mrc p15, 0, %0, c9, c12, 1" : "=r"(ctxt->CESR));
	asm volatile ("mrc p15, 0, %0, c9, c12, 2" : "=r"(ctxt->CECR));
	asm volatile ("mrc p15, 0, %0, c9, c12, 3" : "=r"(ctxt->OFSR));
	asm volatile ("mrc p15, 0, %0, c9, c12, 5" : "=r"(ctxt->PCSR));
	asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r"(ctxt->CCR));
	asm volatile ("mrc p15, 0, %0, c9, c13, 1" : "=r"(ctxt->ESR));
	asm volatile ("mrc p15, 0, %0, c9, c13, 2" : "=r"(ctxt->PMCountR));
	asm volatile ("mrc p15, 0, %0, c9, c14, 0" : "=r"(ctxt->UER));
	asm volatile ("mrc p15, 0, %0, c9, c14, 1" : "=r"(ctxt->IESR));
	asm volatile ("mrc p15, 0, %0, c9, c14, 2" : "=r"(ctxt->IECR));
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(ctxt->PRRR));
	asm volatile ("mrc p15, 0, %0, c10, c2, 1" : "=r"(ctxt->NRRR));
	asm volatile ("mrc p15, 0, %0, c12, c0, 0" : "=r"(ctxt->SNSVBAR));
	asm volatile ("mrc p15, 0, %0, c13, c0, 0" : "=r"(ctxt->FCSE));
	asm volatile ("mrc p15, 0, %0, c13, c0, 1" : "=r"(ctxt->CID));
	asm volatile ("mrc p15, 0, %0, c13, c0, 2" : "=r"(ctxt->URWTPID));
	asm volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r"(ctxt->UROTPID));
	asm volatile ("mrc p15, 0, %0, c13, c0, 4" : "=r"(ctxt->POTPID));

	saved_cr = ctxt->CR;
	saved_ttb = ctxt->TTB_0R;
	idmap = virt_to_phys(idmap_pgd);
}

void save_processor_state(void)
{
	__save_processor_state(&saved_context);
}
EXPORT_SYMBOL(save_processor_state);

void __restore_processor_state(struct saved_context *ctxt)
{
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r"(ctxt->CSSR));
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ctxt->CR));
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(ctxt->AUXCR));
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(ctxt->CACR));
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(ctxt->TTB_0R));
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(ctxt->TTB_1R));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(ctxt->TTBCR));
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(ctxt->DACR));
	asm volatile ("mcr p15, 0, %0, c5, c0, 0" : : "r"(ctxt->D_FSR));
	asm volatile ("mcr p15, 0, %0, c5, c0, 1" : : "r"(ctxt->I_FSR));
	asm volatile ("mcr p15, 0, %0, c5, c1, 0" : : "r"(ctxt->D_AFSR));
	asm volatile ("mcr p15, 0, %0, c5, c1, 1" : : "r"(ctxt->I_AFSR));
	asm volatile ("mcr p15, 0, %0, c6, c0, 0" : : "r"(ctxt->D_FAR));
	asm volatile ("mcr p15, 0, %0, c6, c0, 2" : : "r"(ctxt->I_FAR));
	asm volatile ("mcr p15, 0, %0, c7, c4, 0" : : "r"(ctxt->PAR));
	asm volatile ("mcr p15, 0, %0, c9, c12, 0" : : "r"(ctxt->PMControlR));
	asm volatile ("mcr p15, 0, %0, c9, c12, 1" : : "r"(ctxt->CESR));
	asm volatile ("mcr p15, 0, %0, c9, c12, 2" : : "r"(ctxt->CECR));
	asm volatile ("mcr p15, 0, %0, c9, c12, 3" : : "r"(ctxt->OFSR));
	asm volatile ("mcr p15, 0, %0, c9, c12, 5" : : "r"(ctxt->PCSR));
	asm volatile ("mcr p15, 0, %0, c9, c13, 0" : : "r"(ctxt->CCR));
	asm volatile ("mcr p15, 0, %0, c9, c13, 1" : : "r"(ctxt->ESR));
	asm volatile ("mcr p15, 0, %0, c9, c13, 2" : : "r"(ctxt->PMCountR));
	asm volatile ("mcr p15, 0, %0, c9, c14, 0" : : "r"(ctxt->UER));
	asm volatile ("mcr p15, 0, %0, c9, c14, 1" : : "r"(ctxt->IESR));
	asm volatile ("mcr p15, 0, %0, c9, c14, 2" : : "r"(ctxt->IECR));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(ctxt->PRRR));
	asm volatile ("mcr p15, 0, %0, c10, c2, 1" : : "r"(ctxt->NRRR));
	asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(ctxt->SNSVBAR));
	asm volatile ("mcr p15, 0, %0, c13, c0, 0" : : "r"(ctxt->FCSE));
	asm volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r"(ctxt->CID));
	asm volatile ("mcr p15, 0, %0, c13, c0, 2" : : "r"(ctxt->URWTPID));
	asm volatile ("mcr p15, 0, %0, c13, c0, 3" : : "r"(ctxt->UROTPID));
	asm volatile ("mcr p15, 0, %0, c13, c0, 4" : : "r"(ctxt->POTPID));

	preempt_enable();
}

void restore_processor_state(void)
{
	__restore_processor_state(&saved_context);
}
EXPORT_SYMBOL(restore_processor_state);
