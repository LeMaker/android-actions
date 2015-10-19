/*
 * arch/arm/mach-gl5202/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <mach/hardware.h>

    .macro  addruart, rp, rv, tmp
    ldr     \rp, =UART0_CTL                     @ physical base address
    mrc     p15, 0, \rv, c1, c0
    tst     \rv, #1                             @ MMU enabled?
    beq     100f
    ldr     \rp, =IO_ADDRESS(UART0_CTL)         @ virtual address

100:
    ldr     \rv, [\rp]
    and     \rv, \rv, #0x00008000
    cmp     \rv, #0                             @ is port configured?
    bne     101f
    
    add     \rp, \rp, #0x00002000
    b       100b

101:
    and     \rp, \rp, #0x03ffffff
    orr     \rv, \rp, #0xf8000000               @ virtual
    orr     \rp, \rp, #0xb0000000               @ physical

    .endm

    .macro  senduart,rd,rx
    str     \rd, [\rx, #(UART0_TXDAT - UART0_BASE)] @ Write to Transmitter Holding Register
    .endm

    .macro  busyuart, rd, rx
1001:   
    ldr     \rd, [\rx, #(UART0_STAT - UART0_BASE)]  @ Read Status Register
    tst     \rd, #0x40                          @ TXFIFO Full?
    bne     1001b
    .endm

    .macro  waituart,rd,rx
1001:   
    ldr     \rd, [\rx, #(UART0_STAT - UART0_BASE)]  @ Read Status Register
    tst     \rd, #0x400                         @ TXFIFO Empty?
    beq     1001b
    .endm