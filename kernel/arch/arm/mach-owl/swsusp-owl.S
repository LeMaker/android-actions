/*
 * swsusp.S - This file is based on arch/i386/power/swsusp.S;
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/linkage.h>
#include <asm/segment.h>
#include <asm/page.h>

	.text

#define LOCAL_WORD(x) \
	.data					; \
	.p2align 2				; \
	.type   x, #object		; \
	.size   x, 4			; \
x:							; \
	.long 1

#define WORD_ADDR(x) \
	.align 2				; \
.L##x:						; \
	.word x

#define FUNC(x) \
	.text					; \
	.p2align 2				; \
	.globl x				; \
	.type   x, #function	; \
x:

#define FUNC_END(x) \
	.size   x, .-x

#define CHANGE_MODE(x) \
	mov     r1, r0				; \
	bic     r1, r1, #0x1f		; \
	orr     r1, r1, #0x##x		; \
	msr     cpsr_c, r1

#define SAVE_VFP_REG(x) \
	fmrrd   r0, r1, d##x		; \
	str     r0, [r2]			; \
	str     r1, [r3]			; \
	add     r2, r2, #4			; \
	add     r3, r3, #4


#define RESTORE_VFP_REG(x) \
	ldr     r0, [r2]			; \
	ldr     r1, [r3]			; \
	fmdrr   d##x, r0, r1


/* nonvolatile int registers */
	.globl  saved_context_r0

	LOCAL_WORD(saved_context_r0)
	LOCAL_WORD(saved_context_r1)
	LOCAL_WORD(saved_context_r2)
	LOCAL_WORD(saved_context_r3)
	LOCAL_WORD(saved_context_r4)
	LOCAL_WORD(saved_context_r5)
	LOCAL_WORD(saved_context_r6)
	LOCAL_WORD(saved_context_r7)
	LOCAL_WORD(saved_context_r8)
	LOCAL_WORD(saved_context_r9)
	LOCAL_WORD(saved_context_r10)
	LOCAL_WORD(saved_context_r11)
	LOCAL_WORD(saved_context_r12)
	LOCAL_WORD(saved_context_r13)
	LOCAL_WORD(saved_context_r14)
	LOCAL_WORD(saved_cpsr)

	LOCAL_WORD(saved_context_r8_fiq)
	LOCAL_WORD(saved_context_r9_fiq)
	LOCAL_WORD(saved_context_r10_fiq)
	LOCAL_WORD(saved_context_r11_fiq)
	LOCAL_WORD(saved_context_r12_fiq)
	LOCAL_WORD(saved_context_r13_fiq)
	LOCAL_WORD(saved_context_r14_fiq)
	LOCAL_WORD(saved_spsr_fiq)

	LOCAL_WORD(saved_context_r13_irq)
	LOCAL_WORD(saved_context_r14_irq)
	LOCAL_WORD(saved_spsr_irq)

	LOCAL_WORD(saved_context_r13_svc)
	LOCAL_WORD(saved_context_r14_svc)
	LOCAL_WORD(saved_spsr_svc)

	LOCAL_WORD(saved_context_r13_abt)
	LOCAL_WORD(saved_context_r14_abt)
	LOCAL_WORD(saved_spsr_abt)

	LOCAL_WORD(saved_context_r13_und)
	LOCAL_WORD(saved_context_r14_und)
	LOCAL_WORD(saved_spsr_und)

	LOCAL_WORD(saved_fpscr)
	LOCAL_WORD(saved_fpexc)


FUNC(swsusp_arch_suspend)
	/* save current program status register */
	ldr     r3, .Lsaved_cpsr
	mrs     r1, cpsr
	str     r1, [r3]

	/* hold current mode */
	mrs     r0, cpsr

	CHANGE_MODE(1f) /* change to system(user) mode */
	/* save nonvolatile int register */
	ldr     r3, .Lsaved_context_r0
	stmia   r3, {r0-r14}

.Lnext:
	CHANGE_MODE(11) /* change to fiq mode */
	/* save nonvolatile int register */
	ldr     r3, .Lsaved_context_r8_fiq
	stmia   r3, {r8-r14}
	/* save spsr_fiq register */
	ldr     r3, .Lsaved_spsr_fiq
	mrs     r1, spsr
	str     r1, [r3]

	CHANGE_MODE(12) /* change to irq mode */
	/* save nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_irq
	stmia   r3, {r13-r14}
	/* save spsr_irq register */
	ldr     r3, .Lsaved_spsr_irq
	mrs     r1, spsr
	str     r1, [r3]

	CHANGE_MODE(13) /* change to svc mode */
	/* save nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_svc
	stmia   r3, {r13-r14}
	/* save spsr_svc register */
	ldr     r3, .Lsaved_spsr_svc
	mrs     r1, spsr
	str     r1, [r3]

	CHANGE_MODE(17) /* change to abt mode */
	/* save nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_abt
	stmia   r3, {r13-r14}
	/* save spsr_abt register */
	ldr     r3, .Lsaved_spsr_abt
	mrs     r1, spsr
	str     r1, [r3]

	CHANGE_MODE(1b) /* change to und mode */
	/* save nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_und
	stmia   r3, {r13-r14}
	/* save spsr_und register */
	ldr     r3, .Lsaved_spsr_und
	mrs     r1, spsr
	str     r1, [r3]

	/* go back to original mode */
	msr     cpsr_c, r0

	/* call swsusp_save */
	bl      swsusp_save

	/* restore return address */
	ldr     r3, .Lsaved_context_r14_svc
	ldr     lr, [r3]
	mov     pc, lr

	WORD_ADDR(saved_context_r0)
	WORD_ADDR(saved_cpsr)
	WORD_ADDR(saved_context_r8_fiq)
	WORD_ADDR(saved_spsr_fiq)
	WORD_ADDR(saved_context_r13_irq)
	WORD_ADDR(saved_spsr_irq)
	WORD_ADDR(saved_context_r13_svc)
	WORD_ADDR(saved_context_r14_svc)
	WORD_ADDR(saved_spsr_svc)
	WORD_ADDR(saved_context_r13_abt)
	WORD_ADDR(saved_spsr_abt)
	WORD_ADDR(saved_context_r13_und)
	WORD_ADDR(saved_spsr_und)

	WORD_ADDR(saved_fpscr)
	WORD_ADDR(saved_fpexc)

FUNC_END(swsusp_arch_suspend)

	.pushsection .idmap.text, "ax"
ENTRY(swsusp_arch_resume)

	mov r0, #0

	/* Flush TLB (Invalidate unified TLB unlocked entries) */
	mcr     p15, 0, r0, c8, c7, 0
	mcr     p15, 0, r0, c7, c5, 0 @ invalidate I cache

	ldr r1, idmap
	mcr p15, 0, r1, c2, c0, 0		@ set TTB

	mcr p15, 0, r0, c7, c5, 4		@ ISB
	mcr p15, 0, r0, c7, c10, 4		@ DSB

	ldr r3, =swsusp_arch_resume_with_mmu
	ldr r1, saved_cr
	mcr p15, 0, r1, c1, c0, 0		@ set CR

	mcr p15, 0, r0, c7, c5, 4		@ CP15ISB

	mov pc, r3 /* swsusp_arch_resume_with_mmu根据新页表译址了 */

	.globl idmap
	.globl saved_cr
idmap:
	.long 1
saved_cr:
	.long 1
ENDPROC(swsusp_arch_resume)
	.popsection

	.globl swsusp_arch_resume_with_mmu
ENTRY(swsusp_arch_resume_with_mmu)
	mov r0, #0
	ldr r1, .Lsaved_ttb
	ldr r1, [r1]

	/* Flush TLB (Invalidate unified TLB unlocked entries) */
	mcr     p15, 0, r0, c8, c7, 0

	mcr   p15, 0, r1, c2, c0, 0     @ set TTB 0
	mcr   p15, 0, r0, c7, c5, 4     @ CP15ISB

	mrs     r0, cpsr

	CHANGE_MODE(1f) /* change to system(user) mode */

.Lnext_restore:
	/* restore nonvolatile int register */
	ldr     r3, .Lsaved_context_r0
	ldmia   r3, {r0-r14}
	/* restore current program status register */
	ldr     r3, .Lsaved_cpsr
	ldr     r1, [r3]
	msr     cpsr_cxsf, r1

	CHANGE_MODE(11) /* change to fiq mode */
	/* restore nonvolatile int register */
	ldr     r3, .Lsaved_context_r8_fiq
	ldmia   r3, {r8-r14}
	/* restore spsr_fiq register */
	ldr     r3, .Lsaved_spsr_fiq
	ldr     r1, [r3]
	msr     spsr_cxsf, r1

	CHANGE_MODE(12) /* change to irq mode */
	/* restore nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_irq
	ldmia   r3, {r13-r14}
	/* restore spsr_irq register */
	ldr     r3, .Lsaved_spsr_irq
	ldr     r1, [r3]
	msr     spsr_cxsf, r1

	CHANGE_MODE(13) /* change to svc mode */
	/* restore nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_svc
	ldmia   r3, {r13-r14}
	/*  ldr     r13, [r3] */
	/* restore spsr_svc register */
	ldr     r3, .Lsaved_spsr_svc
	ldr     r1, [r3]
	msr     spsr_cxsf, r1

	CHANGE_MODE(17) /* change to abt mode */
	/* restore nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_abt
	ldmia   r3, {r13-r14}
	/* restore spsr_abt register */
	ldr     r3, .Lsaved_spsr_abt
	ldr     r1, [r3]
	msr     spsr_cxsf, r1

	CHANGE_MODE(1b) /* change to und mode */
	/* restore nonvolatile int register */
	ldr     r3, .Lsaved_context_r13_und
	ldmia   r3, {r13-r14}
	/* restore spsr_und register */
	ldr     r3, .Lsaved_spsr_und
	ldr     r1, [r3]
	msr     spsr_cxsf, r1

	/* go back to original mode */
	msr     cpsr_c, r0

	/* restore return address */
	ldr     r3, .Lsaved_context_r14_svc
	ldr     lr, [r3]

	mov r0, #0
	mov pc, lr

	.globl saved_ttb
saved_ttb:
	.long 1
.Lsaved_ttb:
	.long saved_ttb

ENDPROC(swsusp_arch_resume_with_mmu)
