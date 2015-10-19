/*
 * arch/arm/mach-owl/i2c-owl.c
 *
 * I2C master mode controller driver for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_i2c.h>
#include <linux/pinctrl/consumer.h>
#include <mach/clkname.h>
#include <mach/module-owl.h>

static int info_switch;
static int err_switch;

//#define I2C_DEBUG_INFO
#define I2C_DEBUG_WARNING
#define owl_dump_mem(a, b, c, d)           do {} while (0);

/* support FIFO read/write */
#define OWL_I2C_SUPPORT_FIFO
#define OWL_FIFO_MAX_DATA_LENGTH           120
#define OWL_FIFO_MAX_INTER_ADDR_LENGTH     7

/* don't use FIFO mode for HDMI HDCP because of incompatible timing */
#define OWL_I2C_SUPPORT_HDMI_NOFIFO
//#define I2C_DEBUG_INFO
//#define I2C_DEBUG_WARNING
#ifdef I2C_DEBUG_INFO
#define i2c_dbg(fmt, args...)   \
    printk(KERN_INFO fmt, ##args)
#else
#define i2c_dbg(fmt, args...)   \
    do {} while(0)
#endif

#ifdef I2C_DEBUG_WARNING
#define i2c_warn(fmt, args...)  \
    printk(KERN_WARNING "owl_i2c: "fmt"\n", ##args)
#else
#define i2c_warn(fmt, args...)   \
    do {} while(0)
#endif

#ifdef I2C_DEBUG_SWITCH
#define i2c_info(fmt, args...) do { \
	if (info_switch) \
		pr_info(fmt"\n", ##args); \
	} while (0)

#define i2c_err(fmt, args...)  do { \
	if (err_switch) \
		pr_err(fmt"\n", ##args); \
	} while (0)
#else
	#define i2c_info(fmt, args...)
	#define i2c_err(fmt, args...)
#endif


#define I2C_CTL                     0x0000          /* I2C Control Register */
#define I2C_CLKDIV                  0x0004          /* I2C Clock Divide Register */
#define I2C_STAT                    0x0008          /* I2C Status Register */
#define I2C_ADDR                    0x000C          /* I2C Address Register */
#define I2C_TXDAT                   0x0010          /* I2C TX Data Register */
#define I2C_RXDAT                   0x0014          /* I2C RX Data Register */
#define I2C_CMD                     0x0018          /* I2C Command Register */
#define I2C_FIFOCTL                 0x001C          /* I2C FIFO control Register */
#define I2C_FIFOSTAT                0x0020          /* I2C FIFO status Register */
#define I2C_DATCNT                  0x0024          /* I2C Data transmit counter */
#define I2C_RCNT                    0x0028          /* I2C Data transmit remain counter */

/* I2Cx_CTL */
#define I2C_CTL_GRAS                (0x1 << 0)      /* Generate ACK or NACK Signal */
#define I2C_CTL_GRAS_ACK            0            /* generate the ACK signal at 9th clock of SCL */
#define I2C_CTL_GRAS_NACK           I2C_CTL_GRAS /* generate the NACK signal at 9th clock of SCL */
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
#define I2C_STAT_BEB                (0x1 << 1)      /* IRQ Pending Bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_IRQP               (0x1 << 2)      /* IRQ Pending Bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_LAB                (0x1 << 3)      /* Lose arbitration bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_STPD               (0x1 << 4)      /* Stop detect bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_STAD               (0x1 << 5)      /* Start detect bit, Write ¡°1¡± to clear this bit */
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
#define I2C_MODULE_CLK		(100*1000*1000)
#define I2C_CLK_HDMI                    (87*1000)
#define I2C_TRANS_TIMEOUT               (5*HZ)
#define I2C_STATE_DEFAULT "default"

enum i2c_state {
    INVALIDSTATE,
    RESTARTSTATE,
    STARTSTATE,
    WRITESTATE,
    READSTATE,
    STOPSTATE,

    FIFO_READSTATE,
    FIFO_WRITESTATE,
};

struct pin_state{
	struct pinctrl *p;
	struct pinctrl_state *s;
};

struct owl_i2c_dev {
    struct device       *dev;
    struct resource     *ioarea;
    struct i2c_adapter  adapter;
    struct completion   cmd_complete;
    struct i2c_msg      *msgs;
    struct mutex        mutex;
    wait_queue_head_t   waitqueue;
    unsigned int        msg_num;
    unsigned int        msg_idx;
    unsigned int        msg_ptr;
	int i2c_addr_type;	/*1: HDMI,0 else */

    spinlock_t          lock;
    enum i2c_state           state;
    void __iomem        *base;      /* virtual */
    unsigned long       phys;
    u32                 speed;      /* Speed of bus in Khz */
    u32                 i2c_freq;
    u32                 i2c_freq_cfg;
    struct clk          *clk;
	struct clk *i2c_clk;
    int                 irq;
    u8                  fifo_size;
    u8                  rev;
#ifdef OWL_I2C_SUPPORT_HDMI_NOFIFO
    /* use FIFO mode for HDMI HDCP? */
    int                 hdmi_nofifo;
    uint                in_suspend_state;
#endif
    struct pin_state    i2c_pin_state;
};

static void owl_i2cdev_reinit(struct owl_i2c_dev *dev);
static void owl_i2c_put_pin_mux(struct pin_state *i2c_pin_state);

static int is_hdmi_use_i2c_flag;
void set_hdmi_i2c_flag(int flag)
{
	if (0 != flag)
		is_hdmi_use_i2c_flag = 1;
	else
		is_hdmi_use_i2c_flag = 0;
}
EXPORT_SYMBOL(set_hdmi_i2c_flag);
static int is_hdmi_use_i2c(void)
{
	return !!is_hdmi_use_i2c_flag;
}

static ssize_t info_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	int cnt;

	cnt = sprintf(buf, "%d\n(Note: 1: open, 0:close)\n", info_switch);
	return cnt;
}

static ssize_t info_store(struct device *dev,
			  struct device_attribute *attr, const char *buf,
			  size_t count)
{
	int cnt, tmp;
	cnt = sscanf(buf, "%d", &tmp);
	switch (tmp) {
	case 0:
	case 1:
		info_switch = tmp;
		break;
	default:
		pr_err("invalid input\n");
		break;
	}
	return count;
}

static ssize_t err_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int cnt;

	cnt = sprintf(buf, "%d\n(Note: 1: open, 0:close)\n", err_switch);
	return cnt;
}

static ssize_t err_store(struct device *dev,
			 struct device_attribute *attr, const char *buf,
			 size_t count)
{
	int cnt, tmp;
	cnt = sscanf(buf, "%d", &tmp);
	switch (tmp) {
	case 0:
	case 1:
		err_switch = tmp;
		break;
	default:
		pr_err("invalid input\n");
		break;
	}
	return count;
}

static struct device_attribute owl_i2c_attr[] = {
	__ATTR(msg_info, S_IRUSR | S_IWUSR, info_show, info_store),
	__ATTR(msg_error, S_IRUSR | S_IWUSR, err_show, err_store),
};

static inline void owl_i2c_writel(struct owl_i2c_dev *i2c_dev, u32 val, int reg)
{
    i2c_dbg("-->>write 0x%x to 0x%x",val, (u32)(i2c_dev->base +reg));
    __raw_writel(val, i2c_dev->base + reg);
}

static inline u32 owl_i2c_readl(struct owl_i2c_dev *i2c_dev, int reg)
{
    return __raw_readl(i2c_dev->base + reg);
}


static void owl_i2c_printifo(struct owl_i2c_dev * dev)
{
#if 0
	i2c_info("====================================\n");
	i2c_info("MFP_CTL2:0x%x\n", act_readl(MFP_CTL2));
	i2c_info("MFP_CTL3:0x%x\n", act_readl(MFP_CTL3));
	i2c_info("CMU_DEVCLKEN0:0x%x\n", act_readl(CMU_DEVCLKEN0));
	i2c_info("CMU_DEVCLKEN1:0x%x\n", act_readl(CMU_DEVCLKEN1));
	i2c_info("I2C_CTL:0x%x\n", owl_i2c_readl(dev, I2C_CTL));
	i2c_info("I2C_CLKDIV:0x%x\n", owl_i2c_readl(dev, I2C_CLKDIV));
	i2c_info("I2C_STAT:0x%x\n", owl_i2c_readl(dev, I2C_STAT));
	i2c_info("I2C_ADDR:0x%x\n", owl_i2c_readl(dev, I2C_ADDR));
	i2c_info("I2C_CMD:0x%x\n", owl_i2c_readl(dev, I2C_CMD));
	i2c_info("I2C_FIFOCTL:0x%x\n", owl_i2c_readl(dev, I2C_FIFOCTL));
	i2c_info("I2C_FIFOSTAT:0x%x\n", owl_i2c_readl(dev, I2C_FIFOSTAT));
	i2c_info(":0x%x\n", owl_i2c_readl(dev, I2C_DATCNT));
	i2c_info("I2C_RCNT:0x%x\n", owl_i2c_readl(dev, I2C_RCNT));
	i2c_info("====================================\n");
#endif

}

static int owl_i2c_set_speed(struct owl_i2c_dev *dev)
{
	u32 ret = 0;
	u32 freq = 0;
	u32 divide = 0;
	ret = of_property_read_u32(dev->dev->of_node, "clock-frequency", &freq);
	if (ret) {
		ret = -ENODATA;
		goto of_property_read_err;
	}
	if (freq == 0) {
		ret = -EINVAL;
		goto freq_err;
	}
	dev->i2c_addr_type = 0;
	dev->i2c_freq_cfg = dev->i2c_freq = freq;
	divide = I2C_MODULE_CLK / (freq * 16);
	owl_i2c_writel(dev, divide, I2C_CLKDIV);
	return 0;

of_property_read_err:
	i2c_err("of_property_read_u32() failed\n");
	return ret;
freq_err:
	i2c_err("freq error\n");
	return ret;
}
/*
 * Disable i2C controler <Disable its clock..??>
 */
static void owl_i2c_disable(struct owl_i2c_dev *dev)
{
    owl_i2c_writel(dev,
        owl_i2c_readl(dev, I2C_CTL) & ~I2C_CTL_EN, I2C_CTL);

    clk_disable(dev->clk);

    return;
}

//static void i2c_set_freq(struct owl_i2c_dev *dev, u32 pclk, u32 i2c_freq)
//{
//    u32 div_factor;

//    div_factor = (pclk + i2c_freq * 16 - 1) / (i2c_freq * 16);
//    owl_i2c_writel(dev, I2C_CLKDIV_DIV(div_factor), I2C_CLKDIV);

//    i2c_dbg("iic clock divisor is %d!\n", div_factor);
//    i2c_dbg("iic clock is %dHz!\n", pclk / (div_factor * 16));

//    return;
//}

/*
 * Initialize hardware registers.
 */

static void owl_i2cdev_init(struct owl_i2c_dev *dev)
{
//    u32 iiccon = I2C_CTL_EN | I2C_CTL_IRQE | I2C_CTL_PUEN;
    u32 iiccon = I2C_CTL_EN | I2C_CTL_IRQE | I2C_CTL_PUEN;

    i2c_dbg("owl_i2cdev_init");
    owl_i2c_writel(dev, 0xff, I2C_STAT);
    owl_i2c_writel(dev, iiccon, I2C_CTL);
}

static void owl_i2c_enable(struct owl_i2c_dev *dev)
{
	/* 1 enable clk */
	clk_enable(dev->i2c_clk);
	/* 2 init the dev */
	owl_i2cdev_reinit(dev);
}

static inline void owl_i2c_disable_irq(struct owl_i2c_dev *dev)
{
    u32 tmp = owl_i2c_readl(dev, I2C_CTL) & (~I2C_CTL_IRQE);
    owl_i2c_writel(dev, tmp, I2C_CTL);
}

static inline void owl_i2c_enable_irq(struct owl_i2c_dev *dev)
{
    u32 tmp = owl_i2c_readl(dev, I2C_CTL) | I2C_CTL_IRQE;
    owl_i2c_writel(dev, tmp, I2C_CTL);
}

static inline void owl_i2c_clear_tcb(struct owl_i2c_dev *dev)
{
    volatile unsigned int tmp;
    int retry_times = 10;

    i2c_dbg("clear tcb");

    do
    {
        tmp = owl_i2c_readl(dev, I2C_STAT) | I2C_STAT_IRQP;
        owl_i2c_writel(dev, tmp, I2C_STAT);

        /* ensure write finished */
        tmp = owl_i2c_readl(dev, I2C_STAT);
        if (!(tmp & I2C_STAT_IRQP))
            return;

        retry_times--;
    } while (retry_times > 0);

    /* clear IRQ pending timeout, we have to reset the I2C controller */

    i2c_warn("%s(): [i2c%d] clear IRQ pending error! reset controller\n",
        __FUNCTION__,
        dev->adapter.nr);

    owl_i2c_printifo(dev);

    /* reset I2C controller */
    owl_i2c_writel(dev, owl_i2c_readl(dev, I2C_CTL) & ~I2C_CTL_EN, I2C_CTL);
    udelay(1);
    owl_i2c_writel(dev, owl_i2c_readl(dev, I2C_CTL) | I2C_CTL_EN, I2C_CTL);
    udelay(1);
}

static inline void owl_i2c_reset_fifo(struct owl_i2c_dev *dev)
{
    owl_i2c_writel(dev, I2C_FIFOCTL_RFR | I2C_FIFOCTL_TFR, I2C_FIFOCTL);
    //wait for FIFO Reset ok	
    while(owl_i2c_readl(dev, I2C_FIFOCTL) & (I2C_FIFOCTL_RFR | I2C_FIFOCTL_TFR));
}

static inline int isMsgEnd(struct owl_i2c_dev *dev)
{
    return (dev->msg_ptr == (dev->msgs->len - 1));
}

static inline int isLastMsg(struct owl_i2c_dev *dev)
{
    return (dev->msg_idx == (dev->msg_num - 1));
}

static inline int isMsgFinish(struct owl_i2c_dev *dev)
{
    return (dev->msg_ptr >= dev->msgs->len);
}

static inline int owl_i2c_reset(struct owl_i2c_dev *dev)
{
    owl_i2c_reset_fifo(dev);

    /* reset i2c controller */
    owl_i2c_writel(dev,
        owl_i2c_readl(dev, I2C_CTL) & ~I2C_CTL_EN, I2C_CTL);

    owl_i2c_writel(dev,
        owl_i2c_readl(dev, I2C_CTL) | I2C_CTL_EN, I2C_CTL);
	return 0;
}

static void owl_master_trans_completion(struct owl_i2c_dev *dev, int retval)
{
    i2c_dbg("I2C Trans complete %s", retval? "failed" : "successfully");

//    spin_lock_irq(&dev->lock);
    dev->msgs = NULL;
    dev->msg_num = 0;
    dev->msg_ptr = 0;
    dev->msg_idx++;
    if ( retval )
        dev->msg_idx = retval;
//    spin_unlock_irq(&dev->lock);

    wake_up(&dev->waitqueue);
}

static void owl_i2c_message_start(struct owl_i2c_dev *dev,
                                   struct i2c_msg *msg)
{
    u16 addr = (msg->addr & 0x7f) << 1;
    u32 ctl;

    i2c_dbg("%s()\n", __FUNCTION__);

    if ( msg->flags & I2C_M_RD ) {
        addr |= 1;
    }

#if 1
    dev->state = STARTSTATE;
    ctl = I2C_CTL_IRQE | I2C_CTL_EN | I2C_CTL_GBCC_START
        | I2C_CTL_PUEN | I2C_CTL_RB;
    owl_i2c_writel(dev, addr, I2C_TXDAT);
    owl_i2c_writel(dev, ctl, I2C_CTL);
#endif
}

static void owl_i2c_message_restart(struct owl_i2c_dev *dev,
                                     struct i2c_msg *msg)
{
    u16 addr = (msg->addr & 0x7f) << 1;
    u32 ctl;

    i2c_dbg("%s()\n", __FUNCTION__);

    if (msg->flags & I2C_M_RD){
        addr |= 1;
    }

    ctl = (owl_i2c_readl(dev, I2C_CTL) & ~I2C_CTL_GBCC_MASK)
        | I2C_CTL_GBCC_RESTART | I2C_CTL_RB;
    owl_i2c_writel(dev, addr, I2C_TXDAT);
    owl_i2c_writel(dev, ctl, I2C_CTL);
}

//Only to send stop command, but Not stop at all.
static void owl_i2c_stop(struct owl_i2c_dev *dev, int retval)
{
    u32 ctl;

    i2c_dbg("%s(): retval %d\n", __FUNCTION__, retval);

    dev->state = STOPSTATE;
    ctl = I2C_CTL_EN | I2C_CTL_GBCC_STOP | I2C_CTL_RB ; //stop cmd: 0xaa
    owl_i2c_writel(dev, ctl, I2C_CTL);
    udelay(10);
    owl_master_trans_completion(dev, retval);
}

#ifdef OWL_I2C_SUPPORT_FIFO

static void owl_i2c_message_fifo_start(struct owl_i2c_dev *dev,
                                   struct i2c_msg *msgs, int num)
{
    u16 addr = (msgs[0].addr & 0x7f) << 1;
    struct i2c_msg *msg;
    int i, read = 1, addr_len;
    int fifo_cmd;

    i2c_dbg("%s() %d\n", __FUNCTION__, __LINE__);
    owl_i2c_writel(dev, I2C_CTL_IRQE | I2C_CTL_EN, I2C_CTL);

#ifdef I2C_DEBUG_INFO
    owl_i2c_printifo(dev);
#endif

    if (num == 1) {
        /* 1 message */
        if (!(msgs[0].flags & I2C_M_RD)) {
            /* for write to device */
            owl_i2c_writel(dev, addr, I2C_TXDAT);
            read = 0;
        }

        addr_len = 1;
        msg = &msgs[0];
        dev->msg_idx = 0;
    } else {
        /* 2 messages for read from device */

        /* write address */
        owl_i2c_writel(dev, addr, I2C_TXDAT);

        /* write internal register address */
        for (i = 0; i < msgs[0].len; i++)
            owl_i2c_writel(dev, msgs[0].buf[i], I2C_TXDAT);

        /* internal register address length +  1 byte device address */
        addr_len = msgs[0].len + 1;
        msg = &msgs[1];
        dev->msg_idx = 1;
    }

    i2c_dbg("%s() %d: msg->len %d\n", __FUNCTION__, __LINE__, msg->len);
    owl_i2c_writel(dev, msg->len, I2C_DATCNT);

    if (read) {
        /* write restart with WR address */
        owl_i2c_writel(dev, addr | 1, I2C_TXDAT);

        /* read from device */
        fifo_cmd = I2C_CMD_EXEC | I2C_CMD_MSS | I2C_CMD_SE | I2C_CMD_NS | I2C_CMD_DE | \
                   I2C_CMD_AS(addr_len) | I2C_CMD_SBE;
        if (num == 2)
            fifo_cmd |= I2C_CMD_SAS(1) | I2C_CMD_RBE;       /* 0x8f35 */

        dev->state = FIFO_READSTATE;
    } else {
        /* write to device */
        while (!(I2C_FIFOSTAT_TFF & owl_i2c_readl(dev, I2C_FIFOSTAT))) {
            if (dev->msg_ptr >= msg->len)
                break;

            i2c_dbg("%s(): [i2c%d] fifostat %x, write dev->msg_ptr %d: %x\n", __FUNCTION__, 
                dev->adapter.nr,
                owl_i2c_readl(dev, I2C_FIFOSTAT),
                dev->msg_ptr,
                msg->buf[dev->msg_ptr]);

            owl_i2c_writel(dev, msg->buf[dev->msg_ptr++], I2C_TXDAT);
        }

        fifo_cmd = I2C_CMD_EXEC | I2C_CMD_MSS | I2C_CMD_SE | I2C_CMD_NS | I2C_CMD_DE | \
                   I2C_CMD_AS(1) | I2C_CMD_SBE;             /* 0x8f03 */

        dev->state = FIFO_WRITESTATE;
    }

    /* don't care NACK for hdmi device */
    if (msg->flags & I2C_M_IGNORE_NAK)
        owl_i2c_writel(dev,
            owl_i2c_readl(dev, I2C_FIFOCTL) | I2C_FIFOCTL_NIB, I2C_FIFOCTL);
    else
        owl_i2c_writel(dev,
            owl_i2c_readl(dev, I2C_FIFOCTL) & ~I2C_FIFOCTL_NIB, I2C_FIFOCTL);

    /* write fifo command to start transfer */
    owl_i2c_writel(dev, fifo_cmd, I2C_CMD);

    i2c_dbg("%s() %d\n", __FUNCTION__, __LINE__);

#ifdef I2C_DEBUG_INFO
    owl_i2c_printifo(dev);
#endif

    i2c_dbg("%s() end\n", __FUNCTION__);
}

/*
 * check if we can use the fifo mode to transfer data
 *
 * For simplicity, only support the following data pattern:
 *  1) 1 message,  msg[0] write (MAX 120bytes)
 *  2) 1 message,  msg[0] read  (MAX 120bytes)
 *  3) 2 messages, msg[0] write (MAX 7bytes), msg[1] read (MAX 120bytes)
 */
static int can_use_fifo_trans(struct owl_i2c_dev *dev,
                              struct i2c_msg *msgs, int num)
{
    int pass = 0;


    i2c_dbg("%s(): msgs[0].flags 0x%x, msgs[0].len %d\n", 
        __FUNCTION__, msgs[0].flags, msgs[0].len);

    switch (num) {
    case 1:
        /* 1 message must be write to device */
        pass = 1;
        break;
    case 2:
        /* 2 message must be read from device */
        if ((msgs[0].flags & I2C_M_RD) || !(msgs[1].flags & I2C_M_RD)) {
            i2c_warn("%s(): cannot use fifo mode, msgs[0].flags 0x%x, msgs[0].flags 0x%x\n", 
                __FUNCTION__, msgs[0].flags, msgs[1].flags);
            break;
        }
        pass = 1;
        break;
    default:
        i2c_dbg("%s(): skip msg (num %d)\n", __FUNCTION__, num);
    }
	if(dev->adapter.nr ==1) {
	i2c_dbg("can use i2c freq is %d\n", dev->i2c_freq);
	i2c_dbg("can use hdmi_nofifo is %d\n", dev->hdmi_nofifo);
	}
#ifdef OWL_I2C_SUPPORT_HDMI_NOFIFO
    /* don't use FIFO mode for hdmi */
    if (dev->i2c_freq == I2C_CLK_HDMI || dev->hdmi_nofifo ==1) {
        pass = 0;
    }
#endif
    i2c_dbg("%s() pass %d\n", __FUNCTION__, pass);

    return pass;
}
#else
#define owl_i2c_message_fifo_start(dev, msgs, num) do { } while (0)
#define can_use_fifo_trans(dev, msgs, num)          (0)
#endif


static int owl_i2c_doxfer(struct owl_i2c_dev *dev, struct i2c_msg *msgs, int num)
{
    unsigned long timeout;
    int ret = 0;
    int i;

    spin_lock_irq(&dev->lock);
    dev->state = STARTSTATE;
    dev->msgs = msgs;
    dev->msg_num = num;
    dev->msg_idx = dev->msg_ptr = 0;
    spin_unlock_irq(&dev->lock);

    i2c_dbg("%s(): msg num %d\n", __FUNCTION__, num);

    for (i = 0; i < num; i++) {
        i2c_dbg("  msg[%d]: addr 0x%x, len %d, flags 0x%x\n", 
            i, msgs[i].addr, msgs[i].len, msgs[i].flags);
        owl_dump_mem(msgs[i].buf, msgs[i].len, 0, 1);
    }

    if (can_use_fifo_trans(dev, msgs, num))
        owl_i2c_message_fifo_start(dev, msgs, num);
    else
        owl_i2c_message_start(dev, msgs);

    timeout = wait_event_timeout(dev->waitqueue,
                dev->msg_num == 0, I2C_TRANS_TIMEOUT);
    if ( !timeout ) {
        ret = -EAGAIN;
        i2c_warn("Timedout..");
        goto out;
    }

    if ( dev->msg_idx < 0) {
        ret = -EAGAIN;
        owl_i2cdev_init(dev);
        i2c_warn("Transition failed");
    } else {
        ret = dev->msg_idx;
    }

out:
    /* disable i2c after transfer */
    owl_i2c_writel(dev, 0, I2C_CTL);

    return ret;
}

static int owl_i2c_irq_nextbyte(struct owl_i2c_dev *dev, unsigned int status)
{
    u8 byte, ctl_reg, spec = 0;
    int ret = 0;
    unsigned long flags;

    i2c_dbg("%s(): status 0x%x, dev->state 0x%x\n", __FUNCTION__, 
        status, dev->state);
#ifdef I2C_DEBUG_INFO
    owl_i2c_printifo(dev);
#endif

    spin_lock_irqsave(&dev->lock, flags);
    switch (dev->state) {
        case STOPSTATE:
            i2c_dbg("%s(): STOPSTATE\n", __FUNCTION__);

//            owl_i2c_disable_irq(dev);
            goto out;
        case STARTSTATE:
            i2c_dbg("%s(): STARTSTATE\n", __FUNCTION__);

            if ( dev->msgs->flags & I2C_M_RD ) {
                dev->state = READSTATE;
            } else {
                dev->state = WRITESTATE;
            }

            if ( dev->msgs->len == 0 ) {
                owl_i2c_stop(dev, 0);
                goto out;
            }

        i2c_dbg("%s(): -> dev->state 0x%x\n", __FUNCTION__, 
            dev->state);

            if ( dev->state == READSTATE ) {
                goto pre_read;
            } else if ( dev->state == WRITESTATE ) {
                goto retry_write;
            }

        case READSTATE:

            i2c_dbg("%s(): READSTATE\n", __FUNCTION__);

            byte = owl_i2c_readl(dev, I2C_RXDAT);
            dev->msgs->buf[dev->msg_ptr++] = byte;

pre_read:
            i2c_dbg("%s(): READSTATE - %d, %d, %d\n", __FUNCTION__,
                isMsgEnd(dev), isMsgFinish(dev), isLastMsg(dev));

            if ( isMsgEnd(dev) ) {
                spec = I2C_CTL_GRAS;
            } else if (isMsgFinish(dev)) {
                if ( !isLastMsg(dev) ) {
                    dev->msgs++;
                    dev->msg_idx++;
                    dev->msg_ptr = 0;
                    dev->state = STARTSTATE;
                    owl_i2c_message_restart(dev, dev->msgs);
                    goto out;
                } else {
                    owl_i2c_stop(dev, 0);
                    goto out;
                }
            }
            break;

        case WRITESTATE:
            i2c_dbg("%s(): WRITESTATE\n", __FUNCTION__);

retry_write:
            i2c_dbg("%s(): WRITESTATE - %d, %d, %d\n", __FUNCTION__,
                isMsgEnd(dev), isMsgFinish(dev), isLastMsg(dev));

            if (!isMsgFinish(dev)) {

#ifdef I2C_DEBUG_INFO
    owl_i2c_printifo(dev);
#endif
                byte = dev->msgs->buf[dev->msg_ptr++];
                owl_i2c_writel(dev, byte, I2C_TXDAT);

#ifdef I2C_DEBUG_INFO
    owl_i2c_printifo(dev);
#endif
            } else if (!isLastMsg(dev)) {
                dev->msgs++;
                dev->msg_idx++;
                dev->msg_ptr = 0;
                dev->state = STARTSTATE;
                owl_i2c_message_restart(dev, dev->msgs);
                goto out;
            } else {
                owl_i2c_stop(dev, 0);
                goto out;
            }

            break;
        default:
            i2c_warn("Invalid State..");
            ret = -EINVAL;
            break;
    }

    ctl_reg = (owl_i2c_readl(dev, I2C_CTL) & ~I2C_CTL_GBCC_MASK)
                | I2C_CTL_GBCC_NONE | I2C_CTL_RB | spec;
    owl_i2c_writel(dev, ctl_reg, I2C_CTL);

#ifdef I2C_DEBUG_INFO
    owl_i2c_printifo(dev);
#endif
out:
    spin_unlock_irqrestore(&dev->lock, flags);
    return ret;
}

#ifdef OWL_I2C_SUPPORT_FIFO

static int fifo_write_irq(struct owl_i2c_dev *dev)
{
    struct i2c_msg *msg = &dev->msgs[dev->msg_idx];

    i2c_dbg("%s(): [i2c%d] fifo write, msg->len %d, dev->msg_ptr %d, fifostat %x, rcnt %d\n", __FUNCTION__, 
        dev->adapter.nr,
        msg->len, 
        dev->msg_ptr, 
        owl_i2c_readl(dev, I2C_FIFOSTAT),
        owl_i2c_readl(dev, I2C_RCNT));

    BUG_ON(msg->len < dev->msg_ptr);

    while (!(I2C_FIFOSTAT_TFF & owl_i2c_readl(dev, I2C_FIFOSTAT))) {
        if (dev->msg_ptr >= msg->len)
            break;

        i2c_dbg("%s(): [i2c%d] fifostat %x, write dev->msg_ptr %d: %x\n", __FUNCTION__, 
            dev->adapter.nr,
            owl_i2c_readl(dev, I2C_FIFOSTAT),
            dev->msg_ptr,
            msg->buf[dev->msg_ptr]);

        owl_i2c_writel(dev, msg->buf[dev->msg_ptr++], I2C_TXDAT);
    }

    if (msg->len == dev->msg_ptr && (I2C_FIFOSTAT_CECB & owl_i2c_readl(dev, I2C_FIFOSTAT))) {
        i2c_dbg("%s(): [i2c%d] end fifo write, msg->len %d, fifostat %x, %d/%d\n", __FUNCTION__, 
            dev->adapter.nr,
            msg->len, 
            owl_i2c_readl(dev, I2C_FIFOSTAT),
            owl_i2c_readl(dev, I2C_DATCNT) - owl_i2c_readl(dev, I2C_RCNT),
            owl_i2c_readl(dev, I2C_DATCNT));
        owl_i2c_reset(dev);
        owl_master_trans_completion(dev, 0);
    }

    return 0;
}


static int fifo_read_irq(struct owl_i2c_dev *dev, int stop_detected)
{
    struct i2c_msg *msg = &dev->msgs[dev->msg_idx];
    int byte;

    i2c_dbg("%s(): [i2c%d] fifo read, msg->len %d, dev->msg_ptr %d, fifostat %x\n", __FUNCTION__, 
        dev->adapter.nr,
        msg->len, 
        dev->msg_ptr, 
        owl_i2c_readl(dev, I2C_FIFOSTAT));

    BUG_ON(msg->len < dev->msg_ptr);

    if (msg->len > dev->msg_ptr) {
        while (I2C_FIFOSTAT_RFE & owl_i2c_readl(dev, I2C_FIFOSTAT)) {
            i2c_dbg("%s(): [i2c%d] fifostat %x, stat %x\n", __FUNCTION__, 
                dev->adapter.nr,
                owl_i2c_readl(dev, I2C_FIFOSTAT),
                owl_i2c_readl(dev, I2C_STAT));

            byte = owl_i2c_readl(dev, I2C_RXDAT);
            msg->buf[dev->msg_ptr++] = byte;

            i2c_dbg("%s(): [i2c%d] read to dev->msg_ptr %d: 0x%02x\n", __FUNCTION__, 
                dev->adapter.nr,
                dev->msg_ptr - 1,
                byte);
        }
    }

    if (msg->len == dev->msg_ptr && stop_detected) {
        i2c_dbg("%s(): [i2c%d] finish fifo read, msg->len %d, fifostat %x, stat %x, %d/%d\n", __FUNCTION__, 
            dev->adapter.nr,
            msg->len, 
            owl_i2c_readl(dev, I2C_FIFOSTAT),
            owl_i2c_readl(dev, I2C_STAT),
            owl_i2c_readl(dev, I2C_DATCNT) - owl_i2c_readl(dev, I2C_RCNT),
            owl_i2c_readl(dev, I2C_DATCNT));

        owl_i2c_reset(dev);
        owl_master_trans_completion(dev, 0);
    }

    return 0;
}

static int owl_i2c_fifo_irq(struct owl_i2c_dev *dev, int stop_detected)
{
#ifdef I2C_DEBUG_INFO
    struct i2c_msg *msg = &dev->msgs[dev->msg_idx];
#endif
    unsigned int fifostat;

    i2c_dbg("%s(): fifo mode, state %d, msg_idx %d, len %d, stop_detected %d\n", 
        __FUNCTION__, dev->state, dev->msg_idx, msg->len, stop_detected);

    if (dev->msg_idx >= 2) {
        i2c_warn("%s(): [i2c%d] i2c bus error! I2C_CTL 0x%x, I2C_STAT 0x%x, fifostat 0x%x\n", 
            __FUNCTION__,
            dev->adapter.nr,
            owl_i2c_readl(dev, I2C_CTL),
            owl_i2c_readl(dev, I2C_STAT),
            owl_i2c_readl(dev, I2C_FIFOSTAT));

        owl_i2c_reset(dev);
        owl_master_trans_completion(dev, -ENXIO);
        return -1;
    }

    fifostat = owl_i2c_readl(dev, I2C_FIFOSTAT);
    if (fifostat & I2C_FIFOSTAT_RNB) {
        i2c_warn("%s(): [i2c%d] no ACK, fifostat 0x%x\n", __FUNCTION__, 
            dev->adapter.nr,
            fifostat);
        owl_i2c_reset(dev);
        owl_master_trans_completion(dev, -ENXIO);
        return -1;
    }
    
    if (dev->state == FIFO_WRITESTATE)
        fifo_write_irq(dev);
    else if (dev->state == FIFO_READSTATE)
        fifo_read_irq(dev, stop_detected);
    else 
        BUG_ON(1);

    i2c_dbg("%s() %d:\n", __FUNCTION__, __LINE__);

    return 0;
}
#else
#define owl_i2c_fifo_irq(dev)          do { } while (0)
#endif

static irqreturn_t owl_i2c_interrupt(int irq, void *dev_id)
{
    struct owl_i2c_dev *dev = dev_id;
    unsigned int status = 0, ctl_reg = 0;
    int flags, stop_detected;
    
    i2c_dbg("%s(): irq %d, I2C_STAT 0x%x, I2C_FIFOSTAT 0x%x, dev->state %d\n", 
        __FUNCTION__, 
        irq, owl_i2c_readl(dev, I2C_STAT),
        owl_i2c_readl(dev, I2C_FIFOSTAT), dev->state);

    stop_detected = (owl_i2c_readl(dev, I2C_STAT) & I2C_STAT_STPD) ? 1 : 0;

    /* clear STPD/IRQP */
    owl_i2c_clear_tcb(dev);

    if (dev->state == FIFO_READSTATE || dev->state == FIFO_WRITESTATE) {
        owl_i2c_fifo_irq(dev, stop_detected);
        goto out;
    }

    status = owl_i2c_readl(dev, I2C_STAT);
    if (status & I2C_BUS_ERR_MSK) {
        i2c_warn("I2C trans failed <stat: 0x%x>", status);
        goto out;
    }

    if (dev->state == STOPSTATE)
        goto out;

    if (dev->msgs == NULL) {
        i2c_warn("I2C: skip spurious interrupt, status 0x%x\n", status);
        goto out;
    }

    flags = dev->msgs->flags;

    ctl_reg = owl_i2c_readl(dev, I2C_CTL);
    if (!(flags & I2C_M_RD) && !(flags & I2C_M_IGNORE_NAK)) {
        if ( status & I2C_STAT_RACK) {
            i2c_dbg("ACK\n");                   
        } else {                    
            i2c_warn("No Ack\n");
            goto no_ack;
        }
    }

    owl_i2c_irq_nextbyte(dev, status);

out:
    return IRQ_HANDLED;
no_ack:
    owl_i2c_stop(dev, -ENXIO);
    goto out;
}

static int owl_i2c_xfer(struct i2c_adapter *adap,
    struct i2c_msg *pmsg, int num)
{
    struct owl_i2c_dev *dev = i2c_get_adapdata(adap);
    int ret = 0;
	u32 divide;
	u32 freq = 0;
    if (!adap || !pmsg)
        return -EINVAL;

    if (mutex_lock_interruptible(&dev->mutex) < 0)
        return -EAGAIN;

	if (dev->in_suspend_state != 0) {
		dev_err(&adap->dev, "%s() someone call this API while we are in "
			"suspend state, drop it.\n", __func__);
		dump_stack();
		return -EBUSY;
	}

    /* Sometimes the TP i2c address is 0x30 at I2C1 */
	if ((0 != is_hdmi_use_i2c()) &&
		(((0x60 >> 1) == pmsg->addr) ||
		((0xA0 >> 1) == pmsg->addr) ||
		((0x74 >> 1) == pmsg->addr))) {
        	if (I2C_CLK_HDMI != dev->i2c_freq) {
			dev->i2c_freq = I2C_CLK_HDMI;
			dev->hdmi_nofifo = 1;
		}
	} else {
		dev->hdmi_nofifo = 0;
		dev->i2c_freq = dev->i2c_freq_cfg;
	}
	i2c_dbg("owl_i2c_xfer i2c freq is %d\n", dev->i2c_freq);
	i2c_dbg("owl_i2c_xfer hdmi_nofifo is %d\n", dev->hdmi_nofifo);
	freq = dev->i2c_freq;
	divide = I2C_MODULE_CLK / (freq * 16);
	owl_i2c_writel(dev, divide, I2C_CLKDIV);
    owl_i2cdev_init(dev);
    ret = owl_i2c_doxfer(dev, pmsg, num);

    mutex_unlock(&dev->mutex);

    return ret;
}

static u32 owl_i2c_func(struct i2c_adapter *adapter)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm owl_i2c_algo = {
    .master_xfer    = owl_i2c_xfer,
    .functionality  = owl_i2c_func,
};

static int owl_i2c_remove(struct platform_device *pdev)
{
	struct owl_i2c_dev *dev = platform_get_drvdata(pdev);
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);
	free_irq(dev->irq, dev);
	i2c_del_adapter(&dev->adapter);
	owl_i2c_disable(dev);
	owl_i2c_put_pin_mux(&dev->i2c_pin_state);
	kfree(dev);
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

	return 0;
}

static int owl_i2c_suspend(struct device *dev)
{
	struct owl_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	dev_info(dev, "suspend.\n");
	if (mutex_lock_interruptible(&i2c_dev->mutex) < 0)
		return -EAGAIN;
	disable_irq(i2c_dev->irq);
	owl_i2c_disable(i2c_dev);
	i2c_dev->in_suspend_state = 1;
	mutex_unlock(&i2c_dev->mutex);
	return 0;
}

static int owl_i2c_resume(struct device *dev)
{
	struct owl_i2c_dev *i2c_dev = dev_get_drvdata(dev);

	dev_info(dev, "resume.\n");
	if (mutex_lock_interruptible(&i2c_dev->mutex) < 0)
		return -EAGAIN;

	pr_debug("%s %d\n", __FUNCTION__, __LINE__);

	owl_i2c_enable(i2c_dev);
	enable_irq(i2c_dev->irq);
	i2c_dev->in_suspend_state = 0;
	mutex_unlock(&i2c_dev->mutex);
	return 0;
}


static int owl_i2c_set_pin_mux(struct platform_device *pdev)
{
	int ret;
	struct owl_i2c_dev *dev = platform_get_drvdata(pdev);

	dev->i2c_pin_state.p = pinctrl_get(&pdev->dev);
	if (IS_ERR(dev->i2c_pin_state.p)) {
		i2c_err("owl i2c get pinctrl handle failed");
		return PTR_ERR(dev->i2c_pin_state.p);
	}
	dev->i2c_pin_state.s =
	    pinctrl_lookup_state(dev->i2c_pin_state.p, I2C_STATE_DEFAULT);
	if (IS_ERR(dev->i2c_pin_state.s)) {
		i2c_err("alloc find pinctrl state failed");
		pinctrl_put(dev->i2c_pin_state.p);
		return PTR_ERR(dev->i2c_pin_state.s);
	}

	ret = pinctrl_select_state(dev->i2c_pin_state.p, dev->i2c_pin_state.s);
	if (ret < 0) {
		i2c_err("alloc set pinctrl state failed");
		pinctrl_put(dev->i2c_pin_state.p);
		return ret;
	}

	i2c_info("i2c pinctrl select state successfully\n");
	return 0;

}

static void owl_i2c_put_pin_mux(struct pin_state *i2c_pin_state)
{
	if (i2c_pin_state->p)
		pinctrl_put(i2c_pin_state->p);
}

/*Enable the i2c_clock tree. it's parent is eth_clk
   i2c speed need be set additional*/
static int owl_i2c_set_clk_tree(struct platform_device *pdev)
{
	int ret = 0;
	u32 freq = 0;
	struct owl_i2c_dev *dev = platform_get_drvdata(pdev);

	switch (pdev->id) {
	case 0:
		dev->i2c_clk = clk_get(NULL, CLKNAME_I2C0_CLK);
		freq = clk_round_rate(dev->i2c_clk, I2C_MODULE_CLK);
		if (freq == I2C_MODULE_CLK)
			ret = clk_set_rate(dev->i2c_clk, freq);
		else
			goto round_rate_failed;
		clk_prepare(dev->i2c_clk);
		clk_enable(dev->i2c_clk);
		clk_put(dev->i2c_clk);
		module_clk_enable(MOD_ID_I2C0);
		break;

	case 1:
		dev->i2c_clk = clk_get(NULL, CLKNAME_I2C1_CLK);
		freq = clk_round_rate(dev->i2c_clk, I2C_MODULE_CLK);
		if (freq == I2C_MODULE_CLK)
			ret = clk_set_rate(dev->i2c_clk, freq);
		else
			goto round_rate_failed;
		clk_prepare(dev->i2c_clk);
		clk_enable(dev->i2c_clk);
		clk_put(dev->i2c_clk);
		module_clk_enable(MOD_ID_I2C1);
		break;

	case 2:
		dev->i2c_clk = clk_get(NULL, CLKNAME_I2C2_CLK);
		freq = clk_round_rate(dev->i2c_clk, I2C_MODULE_CLK);
		if (freq == I2C_MODULE_CLK)
			ret = clk_set_rate(dev->i2c_clk, freq);
		else
			goto round_rate_failed;
		clk_prepare(dev->i2c_clk);
		clk_enable(dev->i2c_clk);
		clk_put(dev->i2c_clk);
		module_clk_enable(MOD_ID_I2C2);
		break;

	case 3:
		dev->i2c_clk = clk_get(NULL, CLKNAME_I2C3_CLK);
		freq = clk_round_rate(dev->i2c_clk, I2C_MODULE_CLK);
		if (freq == I2C_MODULE_CLK)
			ret = clk_set_rate(dev->i2c_clk, freq);
		else
			goto round_rate_failed;
		clk_prepare(dev->i2c_clk);
		clk_enable(dev->i2c_clk);
		clk_put(dev->i2c_clk);
		module_clk_enable(MOD_ID_I2C3);
		break;

	default:
		goto clk_get_failed;
		break;
	}

	return 0;

round_rate_failed:
	i2c_err("Round i2c module rate failed\r\n");
	return -2;

clk_get_failed:
	i2c_err("Get i2c_clk failed\r\n");
	return -1;
}
/*
* Enable i2C module, including:
* device tree
* clock   tree
* gpio
* pin     ctrl
*/
static int owl_i2c_module_init(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	pdev->id = of_alias_get_id(np, "i2c");
	if (pdev->id < 0)
		goto alias_get_id_failed;

	/*Pintrol I2C0 MFP UART0 */
	if (owl_i2c_set_pin_mux(pdev))
		goto set_pin_mux_failed;

	if (owl_i2c_set_clk_tree(pdev))
		goto set_clk_tree_failed;

	return 0;

alias_get_id_failed:
	i2c_err("%s: Alias_get_id failed\r\n", __func__);
	return -EINVAL;

set_pin_mux_failed:
	i2c_err("%s: set_pin_mux failed\r\n", __func__);
	return -EINVAL;

set_clk_tree_failed:
	i2c_err("%s: set_clk_tree failed\r\n", __func__);
	return -EINVAL;

}

static void owl_i2cdev_reinit(struct owl_i2c_dev *dev)
{
	/* u32 iiccon = I2C_CTL_EN | I2C_CTL_IRQE | I2C_CTL_PUEN; */
	u32 iiccon = I2C_CTL_EN | I2C_CTL_IRQE | I2C_CTL_PUEN;
	i2c_info("owl_i2cdev_reinit");

	if (owl_i2c_set_speed(dev))
		i2c_err("owl_i2c_set_speed failed\r\n");

	owl_i2c_writel(dev, 0x8a, I2C_CTL);	/*stop i2c */
	owl_i2c_writel(dev, 0x00, I2C_CTL);	/*disable i2c */
	owl_i2c_writel(dev, 0xff, I2C_STAT);	/*clear state */
	owl_i2c_writel(dev, iiccon, I2C_CTL);	/*enable i2c */
}

/*****************************************************************/
static int owl_i2c_probe(struct platform_device *pdev)
{
	struct owl_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem, *irq, *ioarea;
	int ret = 0;
	int i = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -ENODEV;
	}

	ioarea = request_mem_region(mem->start, resource_size(mem), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}

	dev = kzalloc(sizeof(struct owl_i2c_dev),
		GFP_KERNEL);
	if (!dev) {
		i2c_err("alloc i2c device failed");
		ret = -ENOMEM;
		goto err_release_region;
	}

	mutex_init(&dev->mutex);
	dev->dev = &pdev->dev;
	dev->irq = irq->start;
	spin_lock_init(&dev->lock);

	dev->phys = (unsigned long)mem->start;
	if (!dev->phys) {
		ret = -ENOMEM;
		goto err_free_mem;
	}

	dev->base = (void __iomem *)IO_ADDRESS(dev->phys);

	platform_set_drvdata(pdev, dev);
	init_waitqueue_head(&dev->waitqueue);

	ret = request_irq(dev->irq, owl_i2c_interrupt,
			  IRQF_DISABLED, pdev->name, dev);
	if (ret) {
		dev_err(dev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_unuse_clocks;
	}

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "OWL I2C adapter", sizeof(adap->name));
	adap->algo = &owl_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;

	ret = owl_i2c_module_init(pdev);
	if (ret) {
		dev_err(dev->dev, "owl_i2c_module_init failed\r\n");
		ret = ENOMEM;
		goto err_free_irq;
	}

	ret = owl_i2c_set_speed(dev);
	if (ret) {
		dev_err(dev->dev, "owl_i2c_set_speed failed\r\n");
		ret = ENOMEM;
		goto err_free_irq;
	}

	owl_i2cdev_init(dev);
	
	adap->nr = pdev->id;
	ret = i2c_add_numbered_adapter(adap);
	if (ret) {
		dev_err(dev->dev, "failure adding adapter\n");
		ret = ENOMEM;
		goto err_free_irq;
	}
#ifdef I2C_DEBUG_INFO
	owl_i2c_printifo(dev);
#endif

	for (i = 0; i < ARRAY_SIZE(owl_i2c_attr); i++) {
		ret = device_create_file(&pdev->dev, &owl_i2c_attr[i]);
		if (ret) {
			i2c_err("Add device file failed");
			goto err_free_irq;
		}
	}

	of_i2c_register_devices(&dev->adapter);

	return 0;

err_free_irq:
	free_irq(dev->irq, dev);

err_unuse_clocks:
	/*err_iounmap:
	   iounmap(dev->base); */
err_free_mem:
	platform_set_drvdata(pdev, NULL);
	kfree(dev);
err_release_region:
	release_mem_region(mem->start, resource_size(mem));
	return ret;
}

static const struct of_device_id owl_i2c_of_match[] = {
	{ .compatible = "actions,owl-i2c" },
	{}
};

MODULE_DEVICE_TABLE(of, owl_i2c_of_match);

static const struct dev_pm_ops owl_i2c_pm_ops = {
	.suspend_noirq = owl_i2c_suspend,
	.resume_noirq = owl_i2c_resume,
};

static struct platform_driver owl_fdt_driver = {
	.driver = {
		   .name = "owl-i2c",
		   .owner = THIS_MODULE,
		   .of_match_table = owl_i2c_of_match,
		   },
	.probe = owl_i2c_probe,
	.remove = owl_i2c_remove,
	.driver.pm	= &owl_i2c_pm_ops,
};

static int __init owl_i2c_init(void)
{
	return platform_driver_register(&owl_fdt_driver);
}

static void __exit owl_i2c_exit(void)
{
    platform_driver_unregister(&owl_fdt_driver);
}

MODULE_AUTHOR("lzhou");
MODULE_DESCRIPTION("I2C driver for Actions SOC");
MODULE_LICENSE("GPL");

subsys_initcall(owl_i2c_init);
module_exit(owl_i2c_exit);

