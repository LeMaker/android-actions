#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <common.h>

#include <asm/arch/gpio.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

#define GPIO_REG_BASE               (GPIO_MFP_PWM_BASE)

#define GPIO_BANK(gpio)             ((gpio) / 32)
#define GPIO_IN_BANK(gpio)          ((gpio) % 32)
#define GPIO_BIT(gpio)              (1 << GPIO_IN_BANK(gpio))

#define GPIO_REG_OUTEN(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x0)
#define GPIO_REG_INEN(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x4)
#define GPIO_REG_DAT(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x8)

#define DEVCLKEN_GPIO			(0x1 << 18)

int owl_gpio_get_value(unsigned gpio)
{
	return readl(GPIO_REG_DAT(gpio)) & GPIO_BIT(gpio);
}

int owl_gpio_set_value(unsigned gpio, int value)
{
	if (value)
		setbits_le32(GPIO_REG_DAT(gpio), GPIO_BIT(gpio));
	else
		clrbits_le32(GPIO_REG_DAT(gpio), GPIO_BIT(gpio));

	return 0;
}

int owl_gpio_direction_input(unsigned gpio)
{
	clrbits_le32(GPIO_REG_OUTEN(gpio), GPIO_BIT(gpio));
	setbits_le32(GPIO_REG_INEN(gpio), GPIO_BIT(gpio));
	return 0;
}
int owl_gpio_clr(unsigned gpio)
{
	clrbits_le32(GPIO_REG_OUTEN(gpio), GPIO_BIT(gpio));
	clrbits_le32(GPIO_REG_INEN(gpio), GPIO_BIT(gpio));
	return 0;
}


int owl_gpio_direction_output(unsigned gpio, int value)
{
	clrbits_le32(GPIO_REG_INEN(gpio), GPIO_BIT(gpio));
	setbits_le32(GPIO_REG_OUTEN(gpio), GPIO_BIT(gpio));

	owl_gpio_set_value(gpio, value);
	return 0;
}

struct gpiochip_ops owl_gpiochip_ops = {
	.direction_input = owl_gpio_direction_input,
	.direction_output = owl_gpio_direction_output,
	.get_value = owl_gpio_get_value,
	.set_value = owl_gpio_set_value,
};

static void gpio_cfg_init(void)
{
	int node, i, j, k;
	char sname[32];
	unsigned int tmp[3];
	const uint32_t *cell;
	int len, iolen, item_num, gpio;

	node = fdt_path_offset(gd->fdt_blob, "/gpio_int");
	if ( node < 0 ) {
		printf("get node gpio_init fail\n");
		return ;
	}
	iolen = sizeof(uint32_t)*3;
	for ( i = 0 ; i < 7; i++ ) { //GPIOA-GPIOG
		sprintf(sname, "initgpio_%c", 'A'+i);
		len = 0;
		cell = fdt_getprop(gd->fdt_blob, node, sname, &len);
		if (!cell || len < iolen ) {
			//printf("get %s, len=%d fail\n", sname, len);
			continue;
		}
		item_num = len/iolen;
		for ( j= 0; j < item_num; j++) {
			for ( k = 0 ; k < 3; k++ ) {
				tmp[k] = fdt32_to_cpu(cell[j*3+k]);
			}
			if ( tmp[0] >=32 ) {
				printf("%s, gpio num=%d >= 32 fail\n", sname, tmp[0]);
				continue;
			}
			gpio = tmp[0]+ i*32;
			printf("gpio=%d,mode=%d,val=%d\n",gpio, tmp[1], tmp[2]);
			if ( tmp[1] == 0 )
				owl_gpio_clr(gpio);
			else if ( tmp[1] == 1 )
				owl_gpio_direction_output(gpio, tmp[2]);
			else
				owl_gpio_direction_input(gpio);

		}		

	}
	
}

int owl_gpio_init(void)
{
	int dev_node = 0;
	int ret;

	dev_node = fdtdec_next_compatible(gd->fdt_blob,
			0, COMPAT_ACTIONS_OWL_GPIO);
	if (dev_node <= 0) {
		debug("Can't get owl gpio device node\n");
		return -1;
	}

/***enable gpio module*************/
	setbits_le32(CMU_DEVCLKEN0, DEVCLKEN_GPIO);
/*****************************/

	ret = gpiochip_add(OWL_GPIOID_MASTER_IC,
			dev_node, &owl_gpiochip_ops);
	if (ret)
		return -1;
	gpio_cfg_init();
	return 0;
}

