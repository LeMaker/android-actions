#ifndef __ASM_ARM_ARCH_FLASHLIGHT_H
#define __ASM_ARM_ARCH_FLASHLIGHT_H

#include <linux/gpio.h>
#include <mach/gpio.h>

#define FLASHLIGHT_NAME "flashlight"

#define FLASHLIGHT_OFF   0
#define FLASHLIGHT_TORCH 1
#define FLASHLIGHT_FLASH 2
#define FLASHLIGHT_NUM   3

struct flashlight_platform_data {
    struct gpio_pre_cfg *gpio_cfg;
	int gpio_pin;
	int flash_duration_ms;
};

int flashlight_control(int level);

#endif /*__ASM_ARM_ARCH_FLASHLIGHT_H*/
