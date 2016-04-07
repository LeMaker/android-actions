#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/mfd/atc260x/atc260x.h>

#include <mach/regs_map-atm7039.h>

#include "atc260x-core.h"



/*----------------------------------------------------------------------------*/
/* code for direct access */

#define REG_I2C_CTL(_iob)                   ((_iob)+(I2C0_CTL-I2C0_BASE))
#define REG_I2C_CLKDIV(_iob)                ((_iob)+(I2C0_CLKDIV-I2C0_BASE))
#define REG_I2C_STAT(_iob)                  ((_iob)+(I2C0_STAT-I2C0_BASE))
#define REG_I2C_ADDR(_iob)                  ((_iob)+(I2C0_ADDR-I2C0_BASE))
#define REG_I2C_TXDAT(_iob)                 ((_iob)+(I2C0_TXDAT-I2C0_BASE))
#define REG_I2C_RXDAT(_iob)                 ((_iob)+(I2C0_RXDAT-I2C0_BASE))
#define REG_I2C_CMD(_iob)                   ((_iob)+(I2C0_CMD-I2C0_BASE))
#define REG_I2C_FIFOCTL(_iob)               ((_iob)+(I2C0_FIFOCTL-I2C0_BASE))
#define REG_I2C_FIFOSTAT(_iob)              ((_iob)+(I2C0_FIFOSTAT-I2C0_BASE))
#define REG_I2C_DATCNT(_iob)                ((_iob)+(I2C0_DATCNT-I2C0_BASE))
#define REG_I2C_RCNT(_iob)                  ((_iob)+(I2C0_RCNT-I2C0_BASE))

#define REG_CMU_ETHERNETPLL(_iob)           ((_iob)+(CMU_ETHERNETPLL-CMU_BASE))
#define REG_CMU_DEVCLKEN0(_iob)             ((_iob)+(CMU_DEVCLKEN0-CMU_BASE))
#define REG_CMU_DEVCLKEN1(_iob)             ((_iob)+(CMU_DEVCLKEN1-CMU_BASE))
#define REG_CMU_DEVRST0(_iob)               ((_iob)+(CMU_DEVRST0-CMU_BASE))
#define REG_CMU_DEVRST1(_iob)               ((_iob)+(CMU_DEVRST1-CMU_BASE))

#define _da_i2c_writel(_iob, _val, _reg_name) writel( (_val), REG_ ##_reg_name ((_iob)) )
#define _da_i2c_readl(_iob, _reg_name)        readl( REG_ ##_reg_name ((_iob)) )




/* I2Cx_CTL */
#define I2C_CTL_GRAS                (0x1 << 0)      /* Generate ACK or NACK Signal */
#define I2C_CTL_GRAS_ACK           0            /* generate the ACK signal at 9th clock of SCL */
#define I2C_CTL_GRAS_NACK          I2C_CTL_GRAS /* generate the NACK signal at 9th clock of SCL */
#define I2C_CTL_RB                  (0x1 << 1)     /* Release Bus */
#define I2C_CTL_GBCC_MASK           (0x3 << 2)     /* Loop Back Enable */
#define I2C_CTL_GBCC(x)             (((x) & 0x3) << 2)
#define I2C_CTL_GBCC_NONE           I2C_CTL_GBCC(0)
#define I2C_CTL_GBCC_START          I2C_CTL_GBCC(1)
#define I2C_CTL_GBCC_STOP           I2C_CTL_GBCC(2)
#define I2C_CTL_GBCC_RESTART        I2C_CTL_GBCC(3)
#define I2C_CTL_IRQE                (0x1 << 5)     /* IRQ Enable */
#define I2C_CTL_PUEN                (0x1 << 6)     /* Internal Pull-Up resistor (1.5k) enable. */
#define I2C_CTL_EN                  (0x1 << 7)     /* Enable. When enable, reset the status machine to IDLE */
#define I2C_CTL_AE                  (0x1 << 8)     /* Arbitor enable */

/* I2Cx_CLKDIV */
#define I2C_CLKDIV_DIV_MASK         (0xff << 0)     /* Clock Divider Factor (only for master mode). */
#define I2C_CLKDIV_DIV(x)           (((x) & 0xff) << 0)

/* I2Cx_STAT */
#define I2C_STAT_RACK               (0x1 << 0)      /* Receive ACK or NACK when transmit data or address */
#define I2C_STAT_BEB                (0x1 << 1)      /* IRQ Pending Bit, Write “1” to clear this bit */
#define I2C_STAT_IRQP               (0x1 << 2)      /* IRQ Pending Bit, Write “1” to clear this bit */
#define I2C_STAT_LAB                (0x1 << 3)      /* Lose arbitration bit, Write “1” to clear this bit */
#define I2C_STAT_STPD               (0x1 << 4)      /* Stop detect bit, Write “1” to clear this bit */
#define I2C_STAT_STAD               (0x1 << 5)      /* Start detect bit, Write “1” to clear this bit */
#define I2C_STAT_BBB                (0x1 << 6)      /* Bus busy bit */
#define I2C_STAT_TCB                (0x1 << 7)      /* Transfer complete bit */
#define I2C_STAT_LBST               (0x1 << 8)      /* Last Byte Status Bit, 0: address, 1: data */
#define I2C_STAT_SAMB               (0x1 << 9)      /* Slave address match bit */
#define I2C_STAT_SRGC               (0x1 << 10)     /* Slave receive general call */

#define I2C_BUS_ERR_MSK             ( I2C_STAT_LAB | I2C_STAT_BEB)

/* I2Cx_CMD */
#define I2C_CMD_SBE                 (0x1 << 0)      /* Start bit enable */
#define I2C_CMD_AS_MASK             (0x7 << 1)      /* Address select */
#define I2C_CMD_AS(x)               (((x) & 0x7) << 1)
#define I2C_CMD_RBE                 (0x1 << 4)      /* Restart bit enable */
#define I2C_CMD_SAS_MASK            (0x7 << 5)      /* Second Address select */
#define I2C_CMD_SAS(x)              (((x) & 0x7) << 5)
#define I2C_CMD_DE                  (0x1 << 8)      /* Data enable */
#define I2C_CMD_NS                  (0x1 << 9)      /* NACK select */
#define I2C_CMD_SE                  (0x1 << 10)     /* Stop enable */
#define I2C_CMD_MSS                 (0x1 << 11)     /* MSS Master or slave mode select */
#define I2C_CMD_WRS                 (0x1 << 12)     /* Write or Read select */
#define I2C_CMD_EXEC                (0x1 << 15)     /* Start to execute the command list */

/*FIFO mode write cmd 0x8d01 */
#define I2C_CMD_X   (\
	I2C_CMD_EXEC | I2C_CMD_MSS | \
	I2C_CMD_SE | I2C_CMD_DE | I2C_CMD_SBE)

/* I2Cx_FIFOCTL */
#define I2C_FIFOCTL_NIB             (0x1 << 0)      /* NACK Ignore Bit */
#define I2C_FIFOCTL_RFR             (0x1 << 1)      /* RX FIFO reset bit, Write 1 to reset RX FIFO */
#define I2C_FIFOCTL_TFR             (0x1 << 2)      /* TX FIFO reset bit, Write 1 to reset TX FIFO */

/* I2Cx_FIFOSTAT */
#define I2C_FIFOSTAT_CECB           (0x1 << 0)      /* command Execute Complete bit */
#define I2C_FIFOSTAT_RNB            (0x1 << 1)      /* Receive NACK Error bit */
#define I2C_FIFOSTAT_RFE            (0x1 << 2)      /* RX FIFO empty bit */
#define I2C_FIFOSTAT_RFF            (0x1 << 3)      /* RX FIFO full bit */
#define I2C_FIFOSTAT_TFE            (0x1 << 4)      /* TX FIFO empty bit */
#define I2C_FIFOSTAT_TFF            (0x1 << 5)      /* TX FIFO full bit */
#define I2C_FIFOSTAT_RFD_MASK       (0xff << 8)     /* Rx FIFO level display */
#define I2C_FIFOSTAT_RFD_SHIFT      (8)
#define I2C_FIFOSTAT_TFD_MASK       (0xff << 16)    /* Tx FIFO level display */
#define I2C_FIFOSTAT_TFD_SHIFT      (16)

#define I2C_CTL_START_CMD               (  \
		  I2C_CTL_IRQE |   \
		  I2C_CTL_EN |     \
		  I2C_CTL_GBCC_START |    \
		  I2C_CTL_PUEN |     \
		  I2C_CTL_RB   \
		 )

#define I2C_CTL_STOP_CMD                (  \
			I2C_CTL_EN |  \
			I2C_CTL_PUEN | \
			I2C_CTL_GBCC_STOP | \
			I2C_CTL_RB | \
			I2C_CTL_IRQE \
		)

#define I2C_CTL_RESTART_CMD             ( \
			I2C_CTL_IRQE | \
			I2C_CTL_EN | \
			I2C_CTL_GBCC_RESTART | \
			I2C_CTL_PUEN | \
			I2C_CTL_RB \
		)



#define I2C_WRITE   (0)
#define I2C_READ    (1)

#define I2C_OK          0
#define I2C_NOK         1
#define I2C_NACK        2
#define I2C_NOK_TOUT    3
#define I2C_TIMEOUT     1

/*
 * direct msg.
 */
struct _da_i2c_dmsg {
	u8  type;       /*read or write */
	u8  s_addr;     /*slave address */
	u8  r_addr;     /*reg address */
	u8  len;        /*data length */
	u8 * data;      /*data buffer */
};

/*
 * cmd_type is 0 for write, 1 for read.
 *
 * addr_len can take any value from 0-255, it is only limited
 * by the char, we could make it larger if needed. If it is
 * 0 we skip the address write cycle.
 */
static int _do_i2c_transfer(void __iomem *iobase, struct _da_i2c_dmsg * msg)
{
	u32 val = 0, i = 0, result = I2C_OK;
	u32 i2c_cmd;
	u8 chip, addr_len, data_len;
	u8 *addr;
	u8 *data;

	chip = msg->s_addr;
	addr = &(msg->r_addr);
	addr_len = 1; /*fix */
	data = msg->data;
	data_len = msg->len;


	switch (msg->type) {
	case I2C_WRITE:
		/*1, enable i2c ,not enable interrupt*/
		_da_i2c_writel(iobase, 0x80, I2C_CTL);
		/*2, write data count*/
		_da_i2c_writel(iobase, data_len, I2C_DATCNT);
		/*3, write slave addr*/
		_da_i2c_writel(iobase, (chip << 1), I2C_TXDAT);
		/*4, write register addr*/
		for (i = 0; i < addr_len; i++)
			_da_i2c_writel(iobase, addr[i], I2C_TXDAT);
		/*5, write data*/
		for (i = 0; i < data_len; i++)
			_da_i2c_writel(iobase, data[i], I2C_TXDAT);
		/*6, write fifo command */
		i2c_cmd = I2C_CMD_X | I2C_CMD_AS((addr_len) + sizeof(chip));
		_da_i2c_writel(iobase, i2c_cmd, I2C_CMD);

		/* wait command complete */
		while (1) {
			val = _da_i2c_readl(iobase, I2C_FIFOSTAT);
			if (val & I2C_FIFOSTAT_RNB) {
				result = I2C_NACK;
				goto label_err;
			}
			if (val & I2C_FIFOSTAT_CECB)
				break;
			/* 因为可能跑在关中断&关调度的场景, 不做超时处理了. */
		}
		result = I2C_OK;
		break;

	case I2C_READ:
		/*1, enable i2c ,not enable interrupt*/
		_da_i2c_writel(iobase, 0x80, I2C_CTL);
		/*2, write data count*/
		_da_i2c_writel(iobase, data_len, I2C_DATCNT);
		/*3, write slave addr*/
		_da_i2c_writel(iobase, (chip << 1), I2C_TXDAT);
		/*4, write register addr*/
		for (i = 0; i < addr_len; i++)
			_da_i2c_writel(iobase, addr[i], I2C_TXDAT);
		/*5, write slave addr | read_flag*/
		_da_i2c_writel(iobase, (chip << 1) | I2C_READ, I2C_TXDAT);
		/*6, write fifo command */
		i2c_cmd = I2C_CMD_X | I2C_CMD_RBE | I2C_CMD_NS \
			| I2C_CMD_SAS(sizeof(chip)) |\
			I2C_CMD_AS(sizeof(chip) + addr_len);
		_da_i2c_writel(iobase, i2c_cmd, I2C_CMD);

		/* wait command complete */
		while (1) {
			val = _da_i2c_readl(iobase, I2C_FIFOSTAT);
			if (val & I2C_FIFOSTAT_RNB) {
				result = I2C_NACK;
				goto label_err;
			}
			if (val & I2C_FIFOSTAT_CECB)
				break;
			/* 因为可能跑在关中断&关调度的场景, 不做超时处理了. */
		}
		result = I2C_OK;

		/*8, Read data from rxdata*/
		for (i = 0; i < data_len; i++) {
			data[i] = _da_i2c_readl(iobase, I2C_RXDAT);
			/*i2c_dbg("-->>Read data[%d] = 0x%02x\r\n", i, data[i]); */
		}
		break;

	default:
		/*i2c_dbg("i2c_transfer: bad call\n"); */
		result = I2C_NOK;
		break;
	}
	return result;

	label_err:
	/* clear err bit */
	_da_i2c_writel(iobase, I2C_FIFOSTAT_RNB, I2C_FIFOSTAT);
	/* reset fifo */
	_da_i2c_writel(iobase, 0x06, I2C_FIFOCTL);
	_da_i2c_readl(iobase, I2C_FIFOCTL);
	return result;
}

static int _i2c_transfer(void __iomem *iobase, struct _da_i2c_dmsg * msg)
{
	int ret;
	_da_i2c_writel(iobase, 0xff, I2C_STAT);/*reset all the stats */
	_da_i2c_writel(iobase, I2C_CTL_EN | I2C_CTL_PUEN, I2C_CTL); /*disable inttrupt. */
	ret = _do_i2c_transfer(iobase, msg);
	/*disable adapter. */
	_da_i2c_writel(iobase, 0, I2C_CTL);
	return ret;
}

static int _i2c_read(void __iomem *iobase, u8 addr, u8 reg, u8 *data, u32 len)
{
	int ret = 0;
	struct _da_i2c_dmsg msg;

	msg.type = I2C_READ;
	msg.s_addr = addr;
	msg.r_addr = reg;
	msg.len = len;
	msg.data = data;
	ret = _i2c_transfer(iobase, &msg);
	if (ret != 0) {
		pr_err("direct I2c read failed, ret=%d\n", ret);
		return -EIO;
	}
	return 0;
}

static int _i2c_write(void __iomem *iobase, u8 addr, u8 reg, u8 *data, u32 len)
{
	struct _da_i2c_dmsg msg;
	int ret = 0;

	msg.type = I2C_WRITE;
	msg.s_addr = addr;
	msg.r_addr = reg;
	msg.len = len;
	msg.data = data;

	ret = _i2c_transfer(iobase, &msg);
	if (ret != 0) {
		pr_err("driect I2c write failed, ret=%d\n", ret);
		return -EIO;
	}
	return 0;
}

static void _atc260x_i2c_direct_access_init(struct atc260x_dev *atc260x)
{
	static const ulong sc_hw_iobase_tbl[] = {
		I2C0_BASE, I2C1_BASE, I2C2_BASE, I2C3_BASE
	};
	static const u8 sc_i2c_clk_ctl_bit_tbl[] = {
		14, 15, 30, 31,  /* for gl5206 only !!! */
	};
	static const u8 sc_i2c_reset_ctl_bit_tbl[] = {
		12, 13, 18, 19,  /* for gl5206 only !!! */
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

	/* init I2C clock  (gl5206 only !!!) */
	/* ETH PLL */
	writel(readl(REG_CMU_ETHERNETPLL(hw_cmu_iobase)) | (1U << 0),
			REG_CMU_ETHERNETPLL(hw_cmu_iobase));
	udelay(600);
	mask = 1U << sc_i2c_clk_ctl_bit_tbl[atc260x->bus_num];
	writel(readl(REG_CMU_DEVCLKEN1(hw_cmu_iobase)) | mask,
			REG_CMU_DEVCLKEN1(hw_cmu_iobase));
	readl(REG_CMU_DEVCLKEN1(hw_cmu_iobase));

	/* reset I2C */
	mask = 1U << sc_i2c_reset_ctl_bit_tbl[atc260x->bus_num];
	writel(readl(REG_CMU_DEVRST1(hw_cmu_iobase)) & ~mask,
			REG_CMU_DEVRST1(hw_cmu_iobase));
	readl(REG_CMU_DEVRST1(hw_cmu_iobase));
	udelay(20);
	writel(readl(REG_CMU_DEVRST1(hw_cmu_iobase)) | mask,
			REG_CMU_DEVRST1(hw_cmu_iobase));
	readl(REG_CMU_DEVRST1(hw_cmu_iobase));
	udelay(50);

	/* I2C clock divider (400kHz) */
	writel((5U<<8)|(16U<<0), REG_I2C_CLKDIV(atc260x->dacc_iobase));

	dev_info(atc260x->dev, "%s() direct_access activated\n", __func__);
}

static void _atc260x_i2c_direct_access_exit(struct atc260x_dev *atc260x)
{
	if (atc260x->direct_access_ioremap_flag != 0) {
		atc260x->direct_access_ioremap_flag = 0;
		devm_iounmap(atc260x->dev, atc260x->dacc_iobase);
		devm_iounmap(atc260x->dev, atc260x->dacc_cmu_iobase);
		dev_info(atc260x->dev, "%s() direct_access IO unmapped\n", __func__);
	}
}

static int _atc260x_i2c_direct_read_reg(struct atc260x_dev *atc260x, uint reg)
{
	u8 buffer[4];
	int ret;

	ret = _i2c_read(atc260x->dacc_iobase, atc260x->bus_addr, reg, buffer, 2);
	if (ret < 0) {
		return ret;
	}
	ret = buffer[1] | ((uint)(buffer[0]) << 8);
	return ret;
}

static int _atc260x_i2c_direct_write_reg(struct atc260x_dev *atc260x, uint reg, u16 val)
{
	u8 buffer[4];
	int ret;

	buffer[0] = (val >> 8) & 0xffU;
	buffer[1] = val & 0xffU;
	ret = _i2c_write(atc260x->dacc_iobase, atc260x->bus_addr, reg, buffer, 2);
	return ret;
}


/*----------------------------------------------------------------------------*/

static struct regmap_config atc2603c_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,

	/* TODO : add wr_table rd_table volatile_table precious_table */

	.cache_type = REGCACHE_NONE,   /* TODO : i2c reg_rw need a fast cache, REGCACHE_FLAT  */
	.max_register = ATC2603C_CHIP_VER,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
};

static struct regmap_config atc2609a_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,

	/* TODO : add wr_table rd_table volatile_table precious_table */

	.cache_type = REGCACHE_NONE,   /* TODO : i2c reg_rw need a fast cache, REGCACHE_FLAT  */
	.max_register = ATC2609A_CHIP_VER,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
};

static const struct of_device_id atc260x_i2c_of_match[] = {
	{ .compatible = "actions,atc2603c", .data = (void *)ATC260X_ICTYPE_2603C },
	{ .compatible = "actions,atc2609a", .data = (void *)ATC260X_ICTYPE_2609A },
	{ },
};
MODULE_DEVICE_TABLE(of, atc260x_i2c_of_match);

static int atc260x_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id_unused)
{
	struct atc260x_dev *atc260x;
	struct regmap_config *p_regmap_cfg;
	const struct of_device_id *of_id;
	int ret;

	dev_info(&i2c->dev, "Probing...\n");

	ret = i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C);
	if (!ret) {
		dev_err(&i2c->dev, "I2C bus not functional\n");
		return -EFAULT;
	}

	atc260x = devm_kzalloc(&i2c->dev, sizeof(*atc260x), GFP_KERNEL);
	if (atc260x == NULL)
		return -ENOMEM;

	of_id = of_match_device(atc260x_i2c_of_match, &i2c->dev); /* match again, get of_id */
	if (!of_id) {
		dev_err(&i2c->dev, "of_match failed, unable to get device data\n");
		return -ENODEV;
	}
	atc260x->ic_type = (ulong) of_id->data;

	i2c_set_clientdata(i2c, atc260x);
	atc260x->dev = &i2c->dev; /* 不要创建新设备, 复用i2c的从设备即可. */
	atc260x->irq =i2c->irq;

	/* init direct-access functions */
	atc260x->bus_num = (typeof(atc260x->bus_num))(i2c->adapter->nr);
	atc260x->bus_addr = i2c->addr;
	atc260x->direct_read_reg = _atc260x_i2c_direct_read_reg;
	atc260x->direct_write_reg = _atc260x_i2c_direct_write_reg;
	atc260x->direct_acc_init = _atc260x_i2c_direct_access_init;
	atc260x->direct_acc_exit = _atc260x_i2c_direct_access_exit;

	/* register regmap */
	switch (atc260x->ic_type) {
	case ATC260X_ICTYPE_2603C:
		p_regmap_cfg = &atc2603c_i2c_regmap_config;
		break;
	case ATC260X_ICTYPE_2609A:
		p_regmap_cfg = &atc2609a_i2c_regmap_config;
		break;
	default:
		BUG();
	}
	atc260x->regmap = devm_regmap_init_i2c(i2c, p_regmap_cfg);
	if (IS_ERR(atc260x->regmap)) {
		ret = PTR_ERR(atc260x->regmap);
		dev_err(atc260x->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	ret = atc260x_core_dev_init(atc260x);

	return ret;
}

static int atc260x_i2c_remove(struct i2c_client *i2c)
{
	struct atc260x_dev *atc260x = i2c_get_clientdata(i2c);
	atc260x_core_dev_exit(atc260x);
	return 0;
}

static int atc260x_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct atc260x_dev *atc260x = i2c_get_clientdata(client);
	return atc260x_core_dev_suspend(atc260x);
}
static int atc260x_i2c_suspend_late(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct atc260x_dev *atc260x = i2c_get_clientdata(client);
	return atc260x_core_dev_suspend_late(atc260x);
}
static int atc260x_i2c_resume_early(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct atc260x_dev *atc260x = i2c_get_clientdata(client);
	return atc260x_core_dev_resume_early(atc260x);
}
static int atc260x_i2c_resume(struct device *dev)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct atc260x_dev *atc260x = i2c_get_clientdata(client);
	return atc260x_core_dev_resume(atc260x);
}
static const struct dev_pm_ops s_atc260x_i2c_pm_ops = {
	.suspend       = atc260x_i2c_suspend,
	.suspend_late  = atc260x_i2c_suspend_late,
	.resume_early  = atc260x_i2c_resume_early,
	.resume        = atc260x_i2c_resume,
	.freeze        = atc260x_i2c_suspend,
	.freeze_late   = atc260x_i2c_suspend_late,
	.thaw_early    = atc260x_i2c_resume_early,
	.thaw          = atc260x_i2c_resume,
	.poweroff      = atc260x_i2c_suspend,
	.poweroff_late = atc260x_i2c_suspend_late,
	.restore_early = atc260x_i2c_resume_early,
	.restore       = atc260x_i2c_resume,
};

static const struct i2c_device_id atc260x_i2c_id_tbl[] = {
	{ "atc2603a", 0 },
	{ "atc2603c", 0 },
	{ "atc2609a", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, atc260x_i2c_id_tbl);

static struct i2c_driver atc260x_i2c_driver = {
	.probe      = atc260x_i2c_probe,
	.remove     = atc260x_i2c_remove,
	.driver = {
		.name   = "atc260x_i2c",
		.owner  = THIS_MODULE,
		.pm     = &s_atc260x_i2c_pm_ops,
		.of_match_table = of_match_ptr(atc260x_i2c_of_match),
	},
	.id_table   = atc260x_i2c_id_tbl,
};

static int __init atc260x_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&atc260x_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register atc260x I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(atc260x_i2c_init);
/*module_init(atc260x_i2c_init); */ /* for debug */

static void __exit atc260x_i2c_exit(void)
{
	i2c_del_driver(&atc260x_i2c_driver);
}
module_exit(atc260x_i2c_exit);

MODULE_DESCRIPTION("I2C support for atc260x PMICs");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions Semi.");
