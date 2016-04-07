#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/mfd/atc260x/atc260x.h>

#include <mach/regs_map-atm7039.h>

#include "atc260x-core.h"



/*----------------------------------------------------------------------------*/
/* code for direct access */

#define REG_SPI_CTL(_iob)                   ((_iob)+(SPI0_CTL-SPI0_BASE))
#define REG_SPI_CLKDIV(_iob)                ((_iob)+(SPI0_CLKDIV-SPI0_BASE))
#define REG_SPI_STAT(_iob)                  ((_iob)+(SPI0_STAT-SPI0_BASE))
#define REG_SPI_RXDAT(_iob)                 ((_iob)+(SPI0_RXDAT-SPI0_BASE))
#define REG_SPI_TXDAT(_iob)                 ((_iob)+(SPI0_TXDAT-SPI0_BASE))
#define REG_SPI_TCNT(_iob)                  ((_iob)+(SPI0_TCNT-SPI0_BASE))
#define REG_SPI_SEED(_iob)                  ((_iob)+(SPI0_SEED-SPI0_BASE))
#define REG_SPI_TXCR(_iob)                  ((_iob)+(SPI0_TXCR-SPI0_BASE))
#define REG_SPI_RXCR(_iob)                  ((_iob)+(SPI0_TXCR-SPI0_BASE))

#define REG_CMU_ETHERNETPLL(_iob)           ((_iob)+(CMU_ETHERNETPLL-CMU_BASE))
#define REG_CMU_DEVCLKEN0(_iob)             ((_iob)+(CMU_DEVCLKEN0-CMU_BASE))
#define REG_CMU_DEVCLKEN1(_iob)             ((_iob)+(CMU_DEVCLKEN1-CMU_BASE))
#define REG_CMU_DEVRST0(_iob)               ((_iob)+(CMU_DEVRST0-CMU_BASE))
#define REG_CMU_DEVRST1(_iob)               ((_iob)+(CMU_DEVRST1-CMU_BASE))


static int spi_shift_out(void __iomem *iobase, unsigned int var)
{
	int temp;
	int wait_times = 0x500000;

	writel(var, REG_SPI_TXDAT(iobase));
	do
	{
		temp =readl(REG_SPI_STAT(iobase));
		temp =temp & 0x04;
	}
	while ((temp == 0)&&(--wait_times > 0));
	writel(temp, REG_SPI_STAT(iobase));

	return(temp);
}
static void spi_direct_write(void __iomem *iobase, u16 addr, u16 data)
{
	u32 delay_cell = 0x00000000; /*0x1000000:delay 1 hclk cycle;0x2000000:delay 2 hclk cycle */
	unsigned short temp;

	writel(0x401c0+delay_cell, REG_SPI_CTL(iobase));    /*16bit mode,pull down spi_ss line */
	temp=(addr<<3)|0x8000;          /*bit15=1 is write flag,low 15 bit is the register address */
	spi_shift_out(iobase, temp);            /*send out the write flag and address */
	spi_shift_out(iobase, data);            /*send out the write data */
	writel(0x401d0+delay_cell, REG_SPI_CTL(iobase));  /*16bit mode,pull up spi_ss line */
}
static u16 spi_direct_read(void __iomem *iobase, u16 addr)
{
	u32 delay_cell = 0x00000000; /*0x1000000:delay 1 hclk cycle;0x2000000:delay 2 hclk cycle */
	unsigned short temp;

	writel(0x401c0+delay_cell, REG_SPI_CTL(iobase));    /*16bit mode,pull down spi_ss line */

	temp=(addr<<3)&0x7fff;          /*bit15=0 is read flag,low 15 bit is the register address */
	spi_shift_out(iobase, temp);            /*send out the read flag and address */

	writel(0x30, REG_SPI_STAT(iobase));         /*reset the fifo */

	spi_shift_out(iobase, 0x0);         /*send out read clock */
	temp = readl(REG_SPI_RXDAT(iobase));          /*read the register value */
	writel(0x401d0+delay_cell, REG_SPI_CTL(iobase));    /*16bit mode,pull up spi_ss line */

	return temp;
}

static void _atc260x_spi_direct_access_init(struct atc260x_dev *atc260x)
{
	static const ulong sc_hw_iobase_tbl[] = {
		SPI0_BASE, SPI1_BASE, SPI2_BASE, SPI3_BASE
	};
	static const u8 sc_spi_clk_ctl_bit_tbl[] = {
		10, 11, 12, 13,  /* for gl5206 only !!! */
	};
	static const u8 sc_spi_reset_ctl_bit_tbl[] = {
		8, 9, 10, 11,  /* for gl5206 only !!! */
	};
	ulong hw_iobase, hw_iosize;
	void __iomem *hw_cmu_iobase;
	uint mask;

	/* ioremap ... */
	if (atc260x->direct_access_ioremap_flag == 0) {
		atc260x->direct_access_ioremap_flag = 1;

		BUG_ON(atc260x->bus_num >= ARRAY_SIZE(sc_hw_iobase_tbl));
		hw_iobase = sc_hw_iobase_tbl[atc260x->bus_num];
		hw_iosize = SPI0_RXCR - SPI0_BASE +4U;
		/* no need to request the IO region, it always failed because we
		 * are now trying to steal others' stuff ;)
		if (!request_mem_region(hw_iobase, hw_iosize, "atc260x-spi-dacc")) {
			dev_err(atc260x->dev, "failed to request SPI region\n");
			BUG();
		} */
		atc260x->dacc_iobase = devm_ioremap(atc260x->dev, hw_iobase, hw_iosize);
		if (atc260x->dacc_iobase == NULL) {
			dev_err(atc260x->dev, "failed to ioremap SPI region\n");
			BUG();
		}
		atc260x->dacc_cmu_iobase = devm_ioremap(atc260x->dev,
					CMU_BASE, (CMU_DIGITALDEBUG-CMU_BASE));
		if (atc260x->dacc_cmu_iobase == NULL) {
			dev_err(atc260x->dev, "failed to ioremap CMU region\n");
			BUG();
		}
	}
	hw_cmu_iobase = atc260x->dacc_cmu_iobase;

	/* init SPI clock */
	mask = 1U << sc_spi_clk_ctl_bit_tbl[atc260x->bus_num];
	writel(readl(REG_CMU_DEVCLKEN1(hw_cmu_iobase)) | mask,
			REG_CMU_DEVCLKEN1(hw_cmu_iobase));
	readl(REG_CMU_DEVCLKEN1(hw_cmu_iobase));

	/* reset SPI */
	mask = 1U << sc_spi_reset_ctl_bit_tbl[atc260x->bus_num];
	writel(readl(REG_CMU_DEVRST1(hw_cmu_iobase)) & ~mask,
			REG_CMU_DEVRST1(hw_cmu_iobase));
	readl(REG_CMU_DEVRST1(hw_cmu_iobase));
	udelay(20);
	writel(readl(REG_CMU_DEVRST1(hw_cmu_iobase)) | mask,
			REG_CMU_DEVRST1(hw_cmu_iobase));
	readl(REG_CMU_DEVRST1(hw_cmu_iobase));
	udelay(50);

	/* SPI clock divider */
	/* assume HCLK <= 1GHz */
	writel((250U<<0), REG_SPI_CLKDIV(atc260x->dacc_iobase));

	dev_info(atc260x->dev, "%s() direct_access activated\n", __func__);
}

static void _atc260x_spi_direct_access_exit(struct atc260x_dev *atc260x)
{
	if (atc260x->direct_access_ioremap_flag != 0) {
		atc260x->direct_access_ioremap_flag = 0;
		devm_iounmap(atc260x->dev, atc260x->dacc_iobase);
		devm_iounmap(atc260x->dev, atc260x->dacc_cmu_iobase);
		dev_info(atc260x->dev, "%s() direct_access IO unmapped\n", __func__);
	}
}

static int _atc260x_spi_direct_read_reg(struct atc260x_dev *atc260x, uint reg)
{
	return spi_direct_read(atc260x->dacc_iobase, reg);
}

static int _atc260x_spi_direct_write_reg(struct atc260x_dev *atc260x, uint reg, u16 val)
{
	spi_direct_write(atc260x->dacc_iobase, reg, val);
	return 0;
}

/*----------------------------------------------------------------------------*/

static struct regmap_config atc2603a_regmap_config = {
	.reg_bits = 13,
	.pad_bits = 3,
	.val_bits = 16,

	.read_flag_mask = 0x0,
	.write_flag_mask = 0x80,

	.cache_type = REGCACHE_NONE,  /* SPI的速率比较高, 不用cache都没有问题了. */
	.max_register = ATC2603A_CVER,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
};

static const struct of_device_id atc2603_spi_of_match[] = {
	{ .compatible = "actions,atc2603a", .data = (void *)ATC260X_ICTYPE_2603A },
	{ },
};
MODULE_DEVICE_TABLE(of, atc260x_of_match);

static int atc260x_spi_probe(struct spi_device *spi)
{
	struct atc260x_dev *atc260x;
	const struct of_device_id *of_id;
	int ret;

	dev_info(&spi->dev, "Probing...\n");

	atc260x = devm_kzalloc(&spi->dev, sizeof(*atc260x), GFP_KERNEL);
	if (atc260x == NULL)
		return -ENOMEM;

	of_id = of_match_device(atc2603_spi_of_match, &spi->dev); /* match again, get of_id */
	if (!of_id) {
		dev_err(&spi->dev, "of_match failed, unable to get device data\n");
		return -ENODEV;
	}
	atc260x->ic_type = (ulong) of_id->data;

	/* 当bits_per_word>8时, word内使用native_endian(little)的字节序,
	 * 这是 OWL spi master驱动那边的实现方式.
	 * 而这边regmap内部已经有endian的处理了,
	 * 提交到spi的都是按big_endian处理好的字节流, 故这里bits_per_word不能配置为16,
	 * 否则寄存器的地址/数据的字节序又反过来了. */
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_3;

	spi_set_drvdata(spi, atc260x);
	atc260x->dev = &spi->dev;
	atc260x->irq = spi->irq;

	/* init direct-access functions */
	atc260x->bus_num = (typeof(atc260x->bus_num))(spi->master->bus_num);
	atc260x->bus_addr = 0;
	atc260x->direct_read_reg = _atc260x_spi_direct_read_reg;
	atc260x->direct_write_reg = _atc260x_spi_direct_write_reg;
	atc260x->direct_acc_init = _atc260x_spi_direct_access_init;
	atc260x->direct_acc_exit = _atc260x_spi_direct_access_exit;

	/* register regmap */
	atc260x->regmap = devm_regmap_init_spi(spi, &atc2603a_regmap_config);
	if (IS_ERR(atc260x->regmap)) {
		ret = PTR_ERR(atc260x->regmap);
		dev_err(atc260x->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	ret = atc260x_core_dev_init(atc260x);
	if (ret) {
		dev_err(&spi->dev, "Unable to init atc260x device");
	}

	return ret;
}

static int atc260x_spi_remove(struct spi_device *spi)
{
	struct atc260x_dev *atc260x = spi_get_drvdata(spi);
	atc260x_core_dev_exit(atc260x);
	return 0;
}

static int atc260x_spi_suspend(struct device *dev)
{
	struct atc260x_dev *atc260x = dev_get_drvdata(dev);
	return atc260x_core_dev_suspend(atc260x);
}
static int atc260x_spi_suspend_late(struct device *dev)
{
	struct atc260x_dev *atc260x = dev_get_drvdata(dev);
	return atc260x_core_dev_suspend_late(atc260x);
}
static int atc260x_spi_resume_early(struct device *dev)
{
	struct atc260x_dev *atc260x = dev_get_drvdata(dev);
	return atc260x_core_dev_resume_early(atc260x);
}
static int atc260x_spi_resume(struct device *dev)
{
	struct atc260x_dev *atc260x = dev_get_drvdata(dev);
	return atc260x_core_dev_resume(atc260x);
}
static const struct dev_pm_ops s_atc260x_spi_pm_ops = {
	.suspend       = atc260x_spi_suspend,
	.suspend_late  = atc260x_spi_suspend_late,
	.resume_early  = atc260x_spi_resume_early,
	.resume        = atc260x_spi_resume,
	.freeze        = atc260x_spi_suspend,
	.freeze_late   = atc260x_spi_suspend_late,
	.thaw_early    = atc260x_spi_resume_early,
	.thaw          = atc260x_spi_resume,
	.poweroff      = atc260x_spi_suspend,
	.poweroff_late = atc260x_spi_suspend_late,
	.restore_early = atc260x_spi_resume_early,
	.restore       = atc260x_spi_resume,
};

static struct spi_driver atc260x_spi_driver = {
	.probe      = atc260x_spi_probe,
	.remove     = atc260x_spi_remove,
	.driver = {
		.name   = "atc260x_spi",
		.owner  = THIS_MODULE,
		.pm     = &s_atc260x_spi_pm_ops,
		.of_match_table = of_match_ptr(atc2603_spi_of_match),
	},
};

static int __init atc260x_spi_init(void)
{
	int ret;

	ret = spi_register_driver(&atc260x_spi_driver);
	if (ret != 0)
		pr_err("Failed to register atc260x SPI driver: %d\n", ret);
	return ret;
}
subsys_initcall(atc260x_spi_init);
/*module_init(atc260x_spi_init); */ /* for debug */

static void __exit atc260x_spi_exit(void)
{
	spi_unregister_driver(&atc260x_spi_driver);
}
module_exit(atc260x_spi_exit);

MODULE_DESCRIPTION("SPI support for atc260x PMICs");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions Semi.");
