/* UTF-8 encoded. */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/atc260x/atc260x.h>

#include "atc260x-core.h"

/* external API
 * 这里提供一些供子设备之外的模块调用的接口.
 * 这样做主要是为了避免内部使用的API被外部滥用, 多IC兼容时造成麻烦. */


static struct atc260x_dev *s_current_atc260x = NULL;

/* auxadc ------------------------------------------------------------------- */

int atc260x_ex_auxadc_find_chan(const char *channel_name)
{
	ATC260X_ASSERT_VALID_DEV(s_current_atc260x); /* 设备枚举前调用则报BUG, 下同. */
	return atc260x_auxadc_find_chan(s_current_atc260x, channel_name);
}
EXPORT_SYMBOL_GPL(atc260x_ex_auxadc_find_chan);

int atc260x_ex_auxadc_read(uint channel, s32 *p_tr_value)
{
	ATC260X_ASSERT_VALID_DEV(s_current_atc260x);
	return atc260x_auxadc_get_translated(s_current_atc260x, channel, p_tr_value);
}
EXPORT_SYMBOL_GPL(atc260x_ex_auxadc_read);


/* pstore ------------------------------------------------------------------- */

int atc260x_ex_pstore_set(uint tag, u32 value)
{
	ATC260X_ASSERT_VALID_DEV(s_current_atc260x);
	return atc260x_pstore_set(s_current_atc260x, tag, value);
}
EXPORT_SYMBOL_GPL(atc260x_ex_pstore_set);

int atc260x_ex_pstore_get(uint tag, u32 *p_value)
{
	ATC260X_ASSERT_VALID_DEV(s_current_atc260x);
	return atc260x_pstore_get(s_current_atc260x, tag, p_value);
}
EXPORT_SYMBOL_GPL(atc260x_ex_pstore_get);


/* init --------------------------------------------------------------------- */

void atc260x_extapi_dev_init(struct atc260x_dev *atc260x)
{
	if (s_current_atc260x == NULL)
		s_current_atc260x = atc260x;
}

void atc260x_extapi_dev_exit(struct atc260x_dev *atc260x)
{
	if (atc260x == s_current_atc260x)
		s_current_atc260x = NULL;
}
