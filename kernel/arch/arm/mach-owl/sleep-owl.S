/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013 Actions Semi Inc.
*/
/******************************************************************************/
#include <mach/hardware.h>
#include <asm/memory.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/asm-offsets.h>
#include <asm/assembler.h>
#include <linux/linkage.h>
#include <linux/threads.h>

/* note : DO NOT print before MMU_OFF */
#define DBG_STAGE_PRINT_MMU_OFF         52  /* '4' */
#define DBG_STAGE_PRINT_DDR_CH0_DWN     51  /* '3' */
#define DBG_STAGE_PRINT_DDR_CH1_DWN     50  /* '2' */
#define DBG_STAGE_PRINT_SWITCH_HOSC     49  /* '1' */
#define DBG_STAGE_PRINT_PMU_CLR_S1      48  /* '0' */

	.pushsection .sram.text, "ax"

ENTRY(owl_finish_suspend)
	ldr sp, =0xffff8FF0
	stmfd sp!, {lr}
	b _temp_arg_pool_end

	/* tmp stack */ 
_temp_arg_pool:
    .word   0
    .word   0
    .word   0
    .word   0
_temp_arg_pool_end:
	/* save arguments */
	adrl    r12, _temp_arg_pool
	stmia   r12, {r0-r3}

	bl  leopard_clean_dcache_all

	/*
	 * Clean & invalid L2 cache, and sync L2 cache.
	 */
	ldr  r0, =IO_ADDRESS(OWL_PA_L2CC)
	ldr  r1, =0x0000ffff
	str  r1, [r0, #L2X0_CLEAN_INV_WAY]
wait:
	ldr  r2, [r0, #L2X0_CLEAN_INV_WAY]
	cmp  r2, #0
	bne  wait

	ldr  r1, =0x0
	str  r1, [r0, #L2X0_CACHE_SYNC]
sync:
	ldr  r1, [r0, #L2X0_CACHE_SYNC]
	ands r1, r1, #0x1
	bne  sync

	bl leopard_flush_dcache_all

	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x0006			@ .............ca.
	bic	r0, r0, #0x1
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	nop
	nop

	mov r0, #0x4000
delay_mmu_off:
	subs r0, r0, #1
	bne delay_mmu_off
	mov r0, #DBG_STAGE_PRINT_MMU_OFF
	bl print_char

	ldr r1, =0xB01C0004
	mov r0, #3
	str r0, [r1] /* SLEEP */

	mov r0, #0x4000
delay_ch0_lp:
	subs r0, r0, #1
	bne delay_ch0_lp

	/* wait ch0 to LP, maximum retry 0x1000 times */
	mov r3, #0x1000
	ldr r1, =0xB01C0008
	mov r2, #0x45
wait_ch0:
	ldr r0, [r1]
	subs r3, r3, #1
	beq wait_ch0_end
	cmp r0, r2
	nop
	nop
	bne wait_ch0
wait_ch0_end:
	mov r0, #0
	cmp r0, r3
	bne ch0_ok
	mov r0, #DBG_STAGE_PRINT_DDR_CH0_DWN
	bl print_char
ch0_ok:

//	ldr r1, =0xb01b00e0
//	ldr r0, [r1]
//	and r0, r0, #0xf
//	mov r1, #0xf
//	cmp r0, r1
//	beq vdd_is_disable
//
//	ldr r1, =0xB01D0004
//	mov r0, #3
//	str r0, [r1] /* SLEEP */
//
//	mov r0, #0x4000
//delay_ch1_lp:
//	subs r0, r0, #1
//	bne delay_ch1_lp
//
//	/* wait ch1 to LP, maximum retry 0x1000 times */
//	mov r3, #0x1000
//	ldr r1, =0xB01D0008
//	mov r2, #0x45
//wait_ch1:
//	ldr r0, [r1]
//	subs r3, r3, #1
//	beq wait_ch1_end
//	cmp r0, r2
//	nop
//	nop
//	bne wait_ch1
//wait_ch1_end:
//	mov r0, #0
//	cmp r0, r3
//	bne ch1_ok
//	mov r0, #DBG_STAGE_PRINT_DDR_CH1_DWN
//	bl print_char
//ch1_ok:



vdd_is_disable:

operate_ddr_pad_zhong:		//add by zhongxu for vddq down
	ldr r0,=0xb01c8024
	//ldr r1,=0x33fc3c1a
	ldr r1,=0x1bfc3c18
	str r1,[r0]
  
	ldr r0,=0xb01c8028
	ldr r1,=0x0000080c
	str r1,[r0] 
  
	ldr r0,=0xb01c802c
	//ldr r1,=0xc4fc001f
	//ldr r1,=0xc5fc001f 
	//ldr r1,=0xc5fc1817 
	//ldr r1,=0xc5f01817 
	ldr r1,=0xc5fc0000
	//ldr r1,=0xf2f0001f 
	str r1,[r0]  

	mov r0, #0x4000
delay_ch0_lp_zhong:
	subs r0, r0, #1
	bne delay_ch0_lp_zhong



	ldr r1, =0xB0290000
	ldr r0, [r1]
	orr r0, r0, #(1 << 1)
	str r0, [r1]

	mov r3, #0x30 @timeout
	mov r4, #0
wait_delay:
	sub r3, r3, #0x1
	cmp r3, r4
	bne wait_delay

	ldr r0, [r1]
	ldr r3,=0xefffffff
	and r0, r0, r3
	str r0, [r1]
	
	mov r3, #0x30 @timeout
	mov r4, #0
wait_delay1:
	sub r3, r3, #0x1
	cmp r3, r4
	bne wait_delay1

	ldr r0, [r1]
	orr r0, r0, #(1 << 28)
	str r0, [r1]

	mov r3, #0x30 @timeout
	mov r4, #0
wait_delay2:
	sub r3, r3, #0x1
	cmp r3, r4
	bne wait_delay2


	/* switch corepll to hosc! */
	ldr r0, =CMU_BASE
	ldr r1, [r0, #0x1c] @ load CMU_BUSCLK
	bic r1, r1, #0x3    @ clear bit[1:0]
	orr r1, r1, #0x1
	str r1, [r0, #0x1c]
	nop
	nop
	nop
	nop

	mov r0, #0x4000
delay_switch_hosc:
	subs r0, r0, #1
	bne delay_switch_hosc
	mov r0, #DBG_STAGE_PRINT_SWITCH_HOSC
	bl print_char

	/*---------------------------------------------*/
	/* restore arguments */
	adrl    r0, _temp_arg_pool
	ldmia   r0, {r4-r7}
	/* r4 :  cpu_type
	 * r5 :  bus_iobase
	 * r6 :  pmic_bus_addr
	 * r7 :  pmic_type      */

	ldr r0, =SPI0_BASE
	cmp r5, r0
	blo i2c_pmic_enter_s2
	ldr r1, =SPI3_BASE
	cmp r5, r1
	bhi i2c_pmic_enter_s2

	/*---------------------------------------------*/
spi_pmic_enter_s2:
	/* reset spi module */
reset_spi:
	mov r0, r5    @Load SPI1 Control base address
	ldr r4, =0x0
	ldr r3, =0x30 @timeout 2us at 24M clk

	ldr r2, =0xd0
	str r2, [r0]

	/* delay 2us */
wait_for_reset:
	sub r3, r3, #0x1
	cmp r3, r4
	bne wait_for_reset

	ldr  r2, =0x401d0
	str  r2, [r0]

	/* start transfer */
	ldr r3, =0x1d4c0      @timeout 5ms at 24M clk
	/* enter to S2 state */
	mov r0, r5           @Load SPI1 Control base address
	ldr r1, =0x30
	str r1, [r0, #0x8]   @clear SPI FIFO

	ldr r1, =0x401c0
	str r1, [r0]         @16bit mode 3,SPI CS- low

	ldr r1, =0x8008      @(1 << 3) | 0x8000
	str r1, [r0, #0x10]  @write atc2603_PMU_SYS_CTL1 register number

wait_for_spi:
	subs  r3, r3, #0x1
	beq  reset_spi

	ldr r1, [r0, #0x8]
	ands r1, r1, #0x4
	beq  wait_for_spi
	nop

	ldr r1, [r0, #0x8]  @clear Transfer Complete Bit
	str r1, [r0, #0x8]

	ldr r1, =0x30
	str r1, [r0, #0x8] @clear SPI FIFO

	ldr r3, =0x1d4c0      @timeout 5ms at 24M clk
	ldr r1, =0x16         @write reg value
	str r1, [r0, #0x10]

wait_for_spi_1:
	subs  r3, r3, #0x1
	beq  reset_spi

	ldr r1, [r0, #0x8]
	ands r1, r1, #0x4
	beq  wait_for_spi_1
	nop

	ldr r1, [r0, #0x8]  @clear Transfer Complete Bit
	str r1, [r0, #0x8]
	ldr r1, =0x30
	str r1, [r0, #0x8] @clear SPI FIFO

	ldr r1, =0x401d0
	str r1, [r0]         @16bit mode 3,SPI CS- high
	nop
	nop
	nop
	nop

wait_for_spi_2:
	mov r0, #DBG_STAGE_PRINT_PMU_CLR_S1
	bl print_char
	b   wait_for_spi_2

	/* r4 :  cpu_type
	 * r5 :  bus_iobase
	 * r6 :  pmic_bus_addr
	 * r7 :  pmic_type      
	 * r8 :  i2c address
	 * r9 : i2c_data send or receive(16bits)
	 */
i2c_pmic_enter_s2:
	/* reset i2c0 module */
	/*check clock source & pll*/

	/*set reg address, PMU_DC2_CTL0 (same for 5307 & 5303), get and set bit5~6 to 0*/
	mov r8, #0x14
   bl i2c_rx
   
   ldr r0, =0xFF9F  //set dcdc2 to pfm mode
   and r9, r0
   bl i2c_tx

	/*set reg address, PMU_SYS_CTL1 (same for 5307 & 5303), send data 0x0016*/
	mov r8, #1
	mov r9, #0x16
   bl i2c_tx

wait_for_enter_s2:
	mov r0, #DBG_STAGE_PRINT_PMU_CLR_S1
	bl print_char
	b   wait_for_enter_s2
ENDPROC(owl_finish_suspend)


ENTRY(reset_i2c_and_set_address)
	stmfd	sp!, {lr}

reset_i2c:
	mov r0, r5
	ldr r3, =0x30 @timeout 2us at 24M clk

	ldr r2, =0x00
	str r2, [r0]

	/* delay 2us */
wait_for_i2c_reset:
	sub r3, r3, #0x1
	cmp r3, #0
	bne wait_for_i2c_reset

	/*enable i2c and pull-up resistor.*/
	ldr  r2, =0xc0
	str  r2, [r0]
	
	/*set data cnt*/
	ldr r2, =0x02
	str r2, [r0, #0x24]
	
	/*set slave address*/
	mov r2, r6, LSL #1
	str r2, [r0, #0x10]
	
   /*set reg address*/
	str r8, [r0, #0x10]
	
	ldmfd	sp!, {pc}
ENDPROC(reset_i2c_and_set_address)

ENTRY(wait_i2c_complete)
	stmfd	sp!, {lr}
	ldr r3, =0x1d4c0      @timeout 5ms at 24M clk
wait_for_i2c_1:
	subs  r3, r3, #0x1
	beq  reset_i2c
	/*check fifo status, CECB, wait i2c transfer finish.*/
	ldr r1, [r0, #0x20]
	ands r1, r1, #0x1
	beq  wait_for_i2c_1
	nop
	ldmfd	sp!, {pc}
ENDPROC(wait_i2c_complete)

	/* r4 :  cpu_type
	 * r5 :  bus_iobase
	 * r6 :  pmic_bus_addr
	 * r7 :  pmic_type      
	 * r8 :  i2c address
	 * r9 : i2c_data send(16bits)
	 */
ENTRY(i2c_tx)
	stmfd	sp!, {lr}
   bl  reset_i2c_and_set_address

	/*set data, high bytes*/
	mov r2, r9, LSR #8
	str r2, [r0, #0x10]
	
	/*set data, low bytes*/
	str r9, [r0, #0x10]
	
	/*write send cmd*/
	ldr r2, =0x8d05
	str r2, [r0, #0x18]

   bl wait_i2c_complete
	
	ldmfd	sp!, {pc}
ENDPROC(i2c_tx)

	/* r4 :  cpu_type
	 * r5 :  bus_iobase
	 * r6 :  pmic_bus_addr
	 * r7 :  pmic_type      
	 * r8 :  i2c address
	 * r9 : i2c_data return(16bits)
	 */
ENTRY(i2c_rx)
	stmfd	sp!, {lr}

   bl  reset_i2c_and_set_address

   ldr r2, =0xcb
	str r2, [r0, #0x10]

	/*read send cmd*/
	ldr r2, =0x8f35
	str r2, [r0, #0x18]

   bl wait_i2c_complete

   ldr r2, [r0, #0x14]
   ldr r9, [r0, #0x14]
   orr r9, r2, LSL #8

	ldmfd	sp!, {pc}
ENDPROC(i2c_rx)

ENTRY(print_char)
	/* check clock before doing any thing! */
	ldr r1, =0xb0160000
	ldr r2, [r1, #0xa4]
//	tst r2, #0x100             /* uart2, bit 21 */
	tst r2, #0x200000          /* uart5, bit 21 */
	beq 24f
//	ldr r1, =0xb0124000        /* uart 2 */
	ldr r1, =0xb012a000        /* uart 5 */
	/* only test fifo_full bit, it will be 0 in most state,
	 * and will not cause hang up here.
	 * DO NOT test fifo_empty bit! */
14:	ldr r2, [r1, #0xc]
	tst r2, #0x200             @ TXFIFO Full?
	bne 14b
	str r0, [r1, #8]
24:	mov	pc, lr
ENDPROC(print_char)



ENTRY(owl_cpu_resume)
	/* goto cpu_resume */
	b       cpu_resume
ENDPROC(owl_cpu_resume)

ENTRY(leopard_flush_dcache_all)
	dmb
	mrc p15, 1, r0, c0, c0, 1   @ read clidr
	ands r3, r0, #0x7000000     @ extract loc from clidr
	mov r3, r3, lsr #23         @ left align loc bit field
	beq finished                @ if loc is 0, then no need to clean
	mov r10, #0                 @ start clean at cache level 0
loop1:
	add r2, r10, r10, lsr #1    @ work out 3x current cache level
	mov r1, r0, lsr r2          @ extract cache type bits from clidr
	and r1, r1, #7              @ mask of the bits for current cache only
	cmp r1, #2                  @ see what cache we have at this level
	blt skip                    @ skip if no cache, or just i-cache
#ifdef CONFIG_PREEMPT
	save_and_disable_irqs_notrace r9  @ make cssr&csidr read atomic
#endif
	mcr p15, 2, r10, c0, c0, 0        @ select current cache level in cssr
	isb                               @ isb to sych the new cssr&csidr
	mrc p15, 1, r1, c0, c0, 0         @ read the new csidr
#ifdef CONFIG_PREEMPT
	restore_irqs_notrace r9
#endif
	and r2, r1, #7                @ extract the length of the cache lines
	add r2, r2, #4                @ add 4 (line length offset)
	ldr r4, =0x3ff
	ands r4, r4, r1, lsr #3       @ find maximum number on the way size
	clz r5, r4                    @ find bit position of way size increment
	ldr	r7, =0x7fff
	ands r7, r7, r1, lsr #13      @ extract max number of the index size
loop2:
	mov r9, r4                    @ create working copy of max way size
loop3:
 ARM(   orr r11, r10, r9, lsl r5   )  @ factor way and cache number into r11
 THUMB( lsl r6, r9, r5  )
 THUMB( orr r11, r10, r6  )           @ factor way and cache number into r11
 ARM(   orr r11, r11, r7, lsl r2   )  @ factor index number into r11
 THUMB( lsl r6, r7, r2    )
 THUMB( orr r11, r11, r6    )         @ factor index number into r11
	mcr p15, 0, r11, c7, c14, 2       @ clean & invalidate by set/way
	subs r9, r9, #1                   @ decrement the way
	bge	loop3
	subs r7, r7, #1                   @ decrement the index
	bge	loop2
skip:
	add r10, r10, #2                  @ increment cache number
	cmp r3, r10
	bgt loop1
finished:
	mov r10, #0                       @ swith back to cache level 0
	mcr p15, 2, r10, c0, c0, 0        @ select current cache level in cssr
	dsb
	isb
	mov	pc, lr
ENDPROC(leopard_flush_dcache_all)


ENTRY(leopard_clean_dcache_all)
	dmb
	mrc p15, 1, r0, c0, c0, 1     @ read clidr
	ands r3, r0, #0x7000000       @ extract loc from clidr
	mov r3, r3, lsr #23           @ left align loc bit field
	beq finished_                 @ if loc is 0, then no need to clean
	mov r10, #0                   @ start clean at cache level 0
loop1_:
	add r2, r10, r10, lsr #1      @ work out 3x current cache level
	mov r1, r0, lsr r2            @ extract cache type bits from clidr
	and r1, r1, #7                @ mask of the bits for current cache only
	cmp r1, #2                    @ see what cache we have at this level
	blt skip_                     @ skip if no cache, or just i-cache
#ifdef CONFIG_PREEMPT
	save_and_disable_irqs_notrace r9 @ make cssr&csidr read atomic
#endif
	mcr p15, 2, r10, c0, c0, 0       @ select current cache level in cssr
	isb                              @ isb to sych the new cssr&csidr
	mrc p15, 1, r1, c0, c0, 0        @ read the new csidr
#ifdef CONFIG_PREEMPT
	restore_irqs_notrace r9
#endif
	and r2, r1, #7                @ extract the length of the cache lines
	add r2, r2, #4                @ add 4 (line length offset)
	ldr r4, =0x3ff
	ands r4, r4, r1, lsr #3       @ find maximum number on the way size
	clz r5, r4                    @ find bit position of way size increment
	ldr r7, =0x7fff
	ands r7, r7, r1, lsr #13      @ extract max number of the index size
loop2_:
	mov r9, r4                    @ create working copy of max way size
loop3_:
 ARM(   orr r11, r10, r9, lsl r5    )   @ factor way and cache number into r11
 THUMB( lsl r6, r9, r5    )
 THUMB( orr r11, r10, r6    )           @ factor way and cache number into r11
 ARM(   orr r11, r11, r7, lsl r2    )   @ factor index number into r11
 THUMB( lsl	r6, r7, r2    )
 THUMB( orr	r11, r11, r6    )           @ factor index number into r11
	mcr p15, 0, r11, c7, c10, 2         @ clean & invalidate by set/way
	subs r9, r9, #1                     @ decrement the way
	bge loop3_
	subs r7, r7, #1                     @ decrement the index
	bge	loop2_
skip_:
	add r10, r10, #2                    @ increment cache number
	cmp r3, r10
	bgt loop1_
finished_:
	mov r10, #0                         @ swith back to cache level 0
	mcr p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
	dsb
	isb
	mov	pc, lr
ENDPROC(leopard_clean_dcache_all)

	.popsection
