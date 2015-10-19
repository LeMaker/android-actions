/*
 * GPIO definitions
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#define OWL_GPIO_BANKS         5
#define OWL_GPIO_PER_BANK      32

enum gpio_group
{
    GPIO_GROUP_INVALID = -1,
    GPIO_GROUP_A = 0,
    GPIO_GROUP_B,
    GPIO_GROUP_C,
    GPIO_GROUP_D,
    GPIO_GROUP_E,
};

/* GPIOA/B/C/D/E, GPIOE0~4 */
#define NR_OWL_GPIO            (4 * 32 + 4)

#define OWL_GPIO_PORTA(x)      ((x) + OWL_GPIO_PER_BANK * 0)
#define OWL_GPIO_PORTB(x)      ((x) + OWL_GPIO_PER_BANK * 1)
#define OWL_GPIO_PORTC(x)      ((x) + OWL_GPIO_PER_BANK * 2)
#define OWL_GPIO_PORTD(x)      ((x) + OWL_GPIO_PER_BANK * 3)
#define OWL_GPIO_PORTE(x)      ((x) + OWL_GPIO_PER_BANK * 4)

#define OWL_GPIO_PORT(iogroup, pin_num) ((pin_num) + OWL_GPIO_PER_BANK * (iogroup))

#endif /* __ASM_ARCH_GPIO_H */

