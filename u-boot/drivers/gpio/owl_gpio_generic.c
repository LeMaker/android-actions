#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <common.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/gpio.h>

#define MAX_CHIP_NUM	2


enum {
	FDT_GPIO_NONE = -1U,	/* an invalid GPIO used to end our list */

	FDT_GPIO_ACTIVE_LOW = 1 << 0,	/* input is active low (else high) */
};

struct gpio_chip chip_list[MAX_CHIP_NUM];

struct gpio_chip *master_ic_chip;

static struct gpio_chip *find_gpio_chip_by_node(int dev_node)
{
	int i;

	for (i = 0; i < MAX_CHIP_NUM; i++) {
		if (chip_list[i].dev_node == dev_node)
			return &chip_list[i];
	}

	return NULL;
}

int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	if (master_ic_chip)
		return master_ic_chip->ops->direction_input(gpio);

	return -1;
}

int gpio_direction_output(unsigned gpio, int value)
{
	if (master_ic_chip)
		return master_ic_chip->ops->direction_output(gpio, value);

	return -1;
}

int gpio_get_value(unsigned gpio)
{
	if (master_ic_chip)
		return master_ic_chip->ops->get_value(gpio);

	return -1;
}

int gpio_set_value(unsigned gpio, int value)
{
	if (master_ic_chip)
		return master_ic_chip->ops->set_value(gpio, value);

	return -1;
}

int owl_gpio_generic_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	if (chip)
		return chip->ops->direction_input(gpio);

	return -1;
}

int owl_gpio_generic_direction_output(struct gpio_chip *chip, unsigned gpio, int value)
{
	if (chip)
		return chip->ops->direction_output(gpio, value);

	return -1;
}

int owl_gpio_generic_get_value(struct gpio_chip *chip, unsigned gpio)
{
	if (chip)
		return chip->ops->get_value(gpio);

	return -1;
}

int owl_gpio_generic_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	if (chip)
		return chip->ops->set_value(gpio, value);

	return -1;
}

/**
 * Decode a list of GPIOs from an FDT. This creates a list of GPIOs with no
 * terminating item.
 *
 * @param blob		FDT blob to use
 * @param node		Node to look at
 * @param prop_name	Node property name
 * @param gpio		Array of gpio elements to fill from FDT. This will be
 *			untouched if either 0 or an error is returned
 * @param max_count	Maximum number of elements allowed
 * @return number of GPIOs read if ok, -FDT_ERR_BADLAYOUT if max_count would
 * be exceeded, or -FDT_ERR_NOTFOUND if the property is missing.
 */
int owl_fdtdec_decode_gpios(const void *blob, int node,
		const char *prop_name,
		struct owl_fdt_gpio_state *gpio, int max_count)
{
	const struct fdt_property *prop;
	const u32 *cell;
	const char *name;
	int len, i;
	int chip_node = 0;

	assert(max_count > 0);
	prop = fdt_get_property(blob, node, prop_name, &len);
	if (!prop) {
		debug("%s: property '%s' missing\n", __func__, prop_name);
		return -FDT_ERR_NOTFOUND;
	}

	/* We will use the name to tag the GPIO */
	name = fdt_string(blob, fdt32_to_cpu(prop->nameoff));
	cell = (u32 *)prop->data;
	len /= sizeof(u32) * 3;		/* 3 cells per GPIO record */
	if (len > max_count) {
		debug(
			" %s: too many GPIOs / cells for property '%s'\n",
			__func__, prop_name);
		return -FDT_ERR_BADLAYOUT;
	}

	/* Read out the GPIO data from the cells */
	for (i = 0; i < len; i++, cell += 3) {
		chip_node =
			fdt_node_offset_by_phandle(blob, fdt32_to_cpu(cell[0]));
		if (chip_node <= 0)
			return -1;

		gpio[i].chip = find_gpio_chip_by_node(chip_node);
		gpio[i].gpio = fdt32_to_cpu(cell[1]);
		gpio[i].flags = fdt32_to_cpu(cell[2]);
		gpio[i].name = name;

		debug("chip_node = 0x%x\n", chip_node);
		debug("gpio num = %d\n", gpio[i].gpio);
		debug("gpio flag = %d\n", gpio[i].flags);
		debug("gpio name = %s\n", gpio[i].name);
	}

	return len;
}

int owl_fdtdec_decode_gpio(const void *blob, int node,
		const char *prop_name,
		struct owl_fdt_gpio_state *gpio)
{
	int err;

	debug("%s: %s\n", __func__, prop_name);
	gpio->chip = NULL;
	gpio->gpio = FDT_GPIO_NONE;
	gpio->name = NULL;
	err = owl_fdtdec_decode_gpios(blob, node, prop_name, gpio, 1);
	return err == 1 ? 0 : err;
}

int gpiochip_add(int id, int dev_node, struct gpiochip_ops *ops)
{
	int i;
	int find_slot = 0;
	struct gpio_chip *chip;

	for (i = 0; i < MAX_CHIP_NUM; i++) {
		if (chip_list[i].id == 0) {
			find_slot = 1;
			break;
		}
	}

	if (find_slot) {
		chip = &chip_list[i];
		chip->id = id;
		chip->dev_node = dev_node;
		chip->ops = ops;
	} else {
		return -1;
	}

	if (id == OWL_GPIOID_MASTER_IC)
		master_ic_chip = chip;

	return 0;
}
