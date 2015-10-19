/******************************************************************************
 ethctrl.c -- ethernet controller embedded in GL5202/GL5302
 
 author : zhouyiliang  @Actions 
 date : 2012-07-04 
 version 0.1

 1. MAC address filter mode : only supports perfect filtering currently
 2. supports up to 14 multicast mac address
 
******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <linux/clk.h>
#include <mach/clkname.h>
#include <mach/module-owl.h>
#include <linux/ctype.h>

#include "ethctrl.h"

// test needed
#include <linux/stddef.h>
#include <linux/of_gpio.h>

#define SETUP_FRAME_LEN 192
#define SETUP_FRAME_PAD 16

/* 0xD0, reserve 16 bytes for align */
#define SETUP_FRAME_RESV_LEN  (SETUP_FRAME_LEN + SETUP_FRAME_PAD)

#define EC_SKB_ALIGN_BITS_MASK  0x3
#define EC_SKB_ALIGNED  0x4

/* 'mii -v' will read first 8 phy registers to get status */
#define PHY_REG_ADDR_RANGE 0x7  
#define PHY_ADDR_MASK (PHY_ADDR_LIMIT - 1)
#define PHY_REG_NUM_MASK 0x7
#define PHY_REG_BITS 16

#define PHY_RESUME_DELAY 2000 //唤醒时间，2000 ms
#define PHY_DETECT_DUTY 300		//轮询周期，300 ms
#define PHY_DETECT_RESUME 500		//唤醒后 phy detect时间 500ms
// need modify it later
#ifdef EC_DEBUG
#define NET_MSG_ENABLE  0 //( NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK )
#else
#define NET_MSG_ENABLE  0
#endif
#define EC_TX_TIMEOUT (2*HZ) /* 2s */
#define MAX_DEVICES 1


static struct delayed_work resume_work;
static struct workqueue_struct *resume_queue = NULL;

#ifdef DETECT_POWER_SAVE
static struct workqueue_struct *power_save_queue = NULL;
#endif

#ifdef PHY_USE_POLL
static struct workqueue_struct *phy_detect_queue= NULL;
#endif

static char g_default_mac_addr[ETH_MAC_LEN] = {0x00, 0x18, 0xFE, 0x61, 0xD5, 0xD6};
static struct net_device *g_eth_dev[MAX_DEVICES];

static char *macaddr = "?";

int suspend_flag = 0;
struct clk *e_clk;

module_param(macaddr, charp, 0);
MODULE_PARM_DESC(macaddr, "MAC address");

static const char g_banner[] __initdata = KERN_INFO "Ethernet controller driver\
    for Actions GL5203, @2012 Actions.\n";

#ifdef ETH_TEST_MODE
static void print_frame_data(void *frame, int len);
static struct sk_buff *get_skb_aligned(unsigned int len);
static int ec_netdev_start_xmit(struct sk_buff *skb, struct net_device *dev);
void print_mac_register(ec_priv_t *ecp);
void print_phy_register(ec_priv_t *ecp);
static void set_mac_according_aneg(ec_priv_t *ecp);
#endif

ssize_t netif_msg_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    struct net_device *ndev = to_net_dev(dev);
    ec_priv_t *ecp = netdev_priv(ndev);

    return sprintf(buf, "0x%x\n", ecp->msg_enable);
}

ssize_t netif_msg_store(struct device *dev, struct device_attribute *attr,
         const char *buf, size_t count)
{
    struct net_device *ndev = to_net_dev(dev);
    ec_priv_t *ecp = netdev_priv(ndev);
    char *endp;
    unsigned long new;
    int ret = -EINVAL;

    new = simple_strtoul(buf, &endp, 0);
    if (endp == buf)
        goto err;

    ecp->msg_enable = (int)new;
    return count;
err:
    return ret;
}

DEVICE_ATTR(netif_msg, S_IRUGO | S_IWUSR, netif_msg_show, netif_msg_store);

#ifdef ETH_TEST_MODE
/* default -1 denote send single frame, [0, n] denote interval n us between 2 frames */
static long continuity_interval = -1;

ssize_t continuity_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    return sprintf(buf, "continuity_interval: %ld\n", continuity_interval);
}

ssize_t continuity_store(struct device *dev, struct device_attribute *attr,
         const char *buf, size_t count)
{
    char *endp;
    unsigned long new;
    int ret = -EINVAL;

    new = simple_strtoul(buf, &endp, 0);
    if (endp == buf)
        goto err;

    continuity_interval = (long)new;
    return count;
err:
    return ret;
}

DEVICE_ATTR(continuity, S_IRUGO | S_IWUGO, continuity_show, continuity_store);

ssize_t send_pattern_show(struct device *dev, struct device_attribute *attr,
	char *buf)
{
    return sprintf(buf, "pattern 0: all zero, 1: all one, 2: pseudo-random\n");
}

static void gen_frame_pattern(char *buf, int len, int pattern)
{
	//printk(KERN_INFO "buf: %p, len: %d, pattern: %d\n", buf, len, pattern);
	switch (pattern) {
	case 0:
		memset(buf, 0, len);
		break;
	case 1:
		memset(buf, 0xff, len);
		break;
	case 2:
		get_random_bytes(buf, len);
		break;
	default:
		printk(KERN_INFO "not supported pattern: %d\n", pattern);
		break;
	}
}

static int send_pattern_continuous(int n, int pat, struct net_device *ndev)
{
	int ret = -EINVAL;
	struct sk_buff *skb;
	int i = 0;

	while (i++ < n) {
		if (NULL == (skb = get_skb_aligned(PKG_MAX_LEN))) {
			printk(KERN_INFO "no memory!\n");
			goto err;
		}
		skb->len = 1500;
		gen_frame_pattern(skb->data, skb->len, pat);

		ec_netdev_start_xmit(skb, ndev);
		if (continuity_interval > 0)
			udelay(continuity_interval);
	}
	return 0;
err:
	return ret;
}

ssize_t send_pattern_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct net_device *ndev = to_net_dev(dev);
	char *endp;
	unsigned long pat = 5;
	int ret = -EINVAL;
	struct sk_buff *skb;

	pat = simple_strtoul(buf, &endp, 0);
	if ((endp == buf) || (pat > 2))
	    goto err;

	if (NULL == (skb = get_skb_aligned(PKG_MAX_LEN))) {
	    printk(KERN_INFO "no memory!\n");
	    goto err;
	}
	skb->len = 1500;

	gen_frame_pattern(skb->data, skb->len, (int)pat);
	//printk(KERN_INFO "frame pattern:\n");
	//print_frame_data(skb->data, skb->len);
	ec_netdev_start_xmit(skb, ndev);

	if (continuity_interval >= 0) {
		if (continuity_interval > 0)
			udelay(continuity_interval);
		send_pattern_continuous(TX_RING_SIZE - 1, (int)pat, ndev);
	}

	return count;
err:
	return ret;
}

DEVICE_ATTR(send_pattern, S_IRUGO | S_IWUGO,
	send_pattern_show, send_pattern_store);

ssize_t test_mode_show(struct device *dev, struct device_attribute *attr,
	char *buf)
{
    return sprintf(buf, "test mode: 10 or 100 Mbps\n");
}

ssize_t test_mode_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct net_device *ndev = to_net_dev(dev);
	ec_priv_t *ecp = netdev_priv(ndev);
	char *endp;
	unsigned long mode = 0;
	int ret = -EINVAL;
	ushort temp;

	mode = simple_strtoul(buf, &endp, 0);
	if ((endp == buf) || ((mode != 10) && (mode != 100)))
	    goto err;

	ecp->test_mode = mode;
	ecp->speed = mode;
	if (ETH_SPEED_10M == ecp->speed) {
		ecp->duplex = ETH_DUP_FULL;
		temp = read_phy_reg(ecp, PHY_REG_FTC1);
		temp  &= ~0x40;  //clear bit 6
		temp  |= 0x01;  //bit 0: force 10M link; bit6: force 100M
		write_phy_reg(ecp, PHY_REG_FTC1, temp); //Set bit0
	} else {
		ecp->duplex = ETH_DUP_FULL;
		temp = read_phy_reg(ecp, PHY_REG_FTC1);
		temp  &= ~0x01;  //clear bit 0, 100M
		temp  |= 0x40;  // bit6: force 100M
		write_phy_reg(ecp, PHY_REG_FTC1, temp); //clear bit0

		/* adjust tx current */
		temp = read_phy_reg(ecp, 0x12);
		temp  &= ~0x780;
		//temp  |= 0x600; //1100, add 50+100uA
		//temp  |= 0x680; //1101
		//temp  |= 0x480; //1001
		temp  |= 0x100; //0010
		//temp  |= 0x280; //0101
		//temp  |= 0x80; //0001
		//temp  |= 0x180; //0011, minus 50uA  max 2.57V
		//temp  |= 0x780; //1111
		write_phy_reg(ecp, 0x12, temp);
	}

	printk(KERN_INFO "PHY_REG_FTC1 = 0x%x\n", (u32)read_phy_reg(ecp, PHY_REG_FTC1));
	printk(KERN_INFO "PHY_REG_TXPN = 0x%x\n", (u32)read_phy_reg(ecp, 0x12));

	/* shut auto-negoiation */
	temp = read_phy_reg(ecp, MII_BMCR);
	temp &= ~BMCR_ANENABLE;
	write_phy_reg(ecp, MII_BMCR, temp);

	set_mac_according_aneg(ecp);

	printk(KERN_INFO "new_duplex = 0x%x\n", ecp->duplex);
	printk(KERN_INFO "new_speed = 0x%x\n", ecp->speed);
	print_phy_register(ecp);
	print_mac_register(ecp);

	return count;
err:
	return ret;
}

DEVICE_ATTR(test_mode, S_IRUGO | S_IWUGO, test_mode_show, test_mode_store);
#endif /* ETH_TEST_MODE */

static int ethernet_set_ref_clk(struct clk *clk, ulong tfreq)
{
	int ret = -1;
	ulong freq;

	printk(KERN_INFO "target freq: %lu\n", tfreq);
	freq= clk_round_rate(clk, tfreq);
	if (freq == tfreq) {
		ret = clk_set_rate(clk, freq);
		if (ret)
			printk(KERN_INFO "set RMII_REF_CLK: %lu failed, errno: %d\n",
				tfreq, -ret);
	} else
		printk(KERN_INFO "wrong RMII_REF_CLK: %lu\n", tfreq);

	return ret;
}

static int ethernet_clock_config(int phy_mode)
{
	struct clk *clk;
	int ret = -1;
	ulong tfreq;

	printk(KERN_INFO "phy_mode: %d\n", phy_mode);
	clk = clk_get(NULL, CLKNAME_RMII_REF_CLK);
	switch (phy_mode) {
	case ETH_PHY_MODE_RMII:
		tfreq = 50 * 1000 * 1000;
		ret = ethernet_set_ref_clk(clk, tfreq);
		break;
	case ETH_PHY_MODE_SMII:
		tfreq = 125 * 1000 * 1000;
		ret = ethernet_set_ref_clk(clk, tfreq);
		break;
	default:
		printk(KERN_INFO "not support phy mode: %d\n", phy_mode);
	}

	clk_put(clk);

	return ret;
}

static void ethernet_clock_enable(int reset_flg)
{
#if 0
    printk(KERN_ERR " %s  LINE:%d\n",__FUNCTION__,__LINE__);
	/* enable ethernet clk */
	putl(getl(0xb01600a4)|0x400000, 0xb01600a4);
	udelay(100);
	putl(getl(0xb0160084)|0x3,0xb0160084);
	udelay(100);
	 if(reset_flg){	
	/* reset ethernet clk */
		putl(getl(CMU_DEVRST1) & ~ASOC_ETH_CLOCK_RST, CMU_DEVRST1);
		udelay(100);
		printk(KERN_ERR " %s  LINE:%d\n",__FUNCTION__,__LINE__);
		putl(getl(CMU_DEVRST1) | ASOC_ETH_CLOCK_RST, CMU_DEVRST1);
		udelay(100);
		//printk(KERN_ERR " %s  LINE:%d\n",__FUNCTION__,__LINE__);
	 }
	 printk(KERN_ERR " %s  LINE:%d\n",__FUNCTION__,__LINE__);

#else
//	struct clk *clk;
	int ret;
//	clk = clk_get(NULL, CLKNAME_CMUMOD_ETHERNET);
//	ret = clk_prepare(e_clk);
//	if (ret)
//		printk(KERN_INFO "prepare ethernet clock failed, errno: %d\n", -ret);
  if(e_clk!=NULL){
		ret = clk_enable(e_clk);
		if (ret)
			printk(KERN_INFO "enable ethernet clock failed, errno: %d\n", -ret);
//	clk_put(e_clk);
	
		udelay(100);

	/* reset ethernet clk */
  	if(reset_flg){	
			  printk(KERN_INFO "func:%s: ethernet clk reset\n", __func__);
	    	module_reset(MODULE_RST_ETHERNET);
		}		    
		udelay(100);
	}
#endif
}

static void ethernet_clock_disable(void)
{
#if 0
	/* disable ethernet clk */
	putl(getl(CMU_DEVCLKEN1) & ~ASOC_ETH_CLOCK_EN, CMU_DEVCLKEN1);
#else
//	struct clk *clk;

//	clk = clk_get(NULL, CLKNAME_CMUMOD_ETHERNET);
  if(e_clk!=NULL){
//		clk_disable(e_clk);
//	clk_unprepare(e_clk);
//	clk_put(e_clk);
	  udelay(100);
	}
#endif
}

/* data is a ethernet frame */
static void check_icmp_sequence(const void *data, const char *msg)
{
#define ptr_to_u32(data, off) (ntohl(*(u32*)((char *)data + off)))

	printk(KERN_INFO "-- %s -- %p, icmp: 0x%x\n", msg, (char *)data + 0x14, (ptr_to_u32(data, 0x14) & 0xff));
	if ((ptr_to_u32(data, 0x14) & 0xff) == 0x01) {// protocol icmp 0x01
		printk(KERN_INFO "ICMP ");
		if (((ptr_to_u32(data, 0x20) >> 8) & 0xff) == 0x8) //icmp type
			printk(KERN_INFO "ping echo request, ");
		else if (((ptr_to_u32(data, 0x20) >> 8) & 0xff) == 0x0)
			printk(KERN_INFO "ping echo reply, ");
		else
			printk(KERN_INFO "not ping echo request or reply, ");
		printk(KERN_INFO "sequence number: %u\n", ptr_to_u32(data, 0x28) >> 16);
	} else {
		printk(KERN_INFO "not a ICMP packet\n");
	}
}

static void print_mac_address(const char *mac)
{
    int i;
    for (i = 0; i < ETH_MAC_LEN - 1; i++) {
        printk(KERN_INFO "%02x-", (unsigned int)mac[i] & 0xFF);
    }
    printk(KERN_INFO "%02x\n", (unsigned int)mac[i] & 0xFF);
    return;
}

static int ctox(int c)
{
	int tmp;

	if (!isxdigit(c)) {
		printk(KERN_INFO "'%c' is not hex digit\n", (char)c);
		return -1;
	}

	if ((c >= '0') && (c <= '9'))
		tmp = c - '0';
	else
		tmp = (c | 0x20) - 'a' + 10;

	return tmp;
}

static int parse_mac_address(const char *mac, int len)
{
	int tmp, tmp2;
	int i = 0;
	int j = 0;
	char s[16] = "";
	int c;

	printk(KERN_INFO "ethernet mac address string: %s, len: %d\n", mac, strlen(mac));
	if (17 == len) {
		if (strlen(mac) > 17) {
			printk(KERN_INFO "ethernet mac address string too long\n");
			return -1;
		}
		while ((c = mac[i++]) != '\0') {
			if (c == ':')
				continue;
			s[j++] = c;
		}
		s[j] = '\0';
		printk(KERN_INFO "mac address string stripped colon: %s\n", s);
	} else if (12 == len) {
		if (strlen(mac) > 12) {
			printk(KERN_INFO "ethernet mac address string too long\n");
			return -1;
		}
		memcpy(s, mac, 12);
		s[12] = '\0';
	} else {
		printk(KERN_INFO "length of ethernet mac address is not 12 or 17\n");
		return -1;
	}

	for (i = 0; i < ETH_MAC_LEN; i++) {
		tmp = ctox(s[i * 2]);
		tmp2 = ctox(s[i * 2 + 1]);
		tmp = (tmp * 16) + tmp2;
		*(char *)(g_default_mac_addr + i) = tmp & 0xFF;
	}

	return 0;
}

static int read_mac_address(struct file *filp, char *mac, int *len)
{
	loff_t l;
	loff_t offset = 0;
	mm_segment_t fs;
	int _len, i;

	l = vfs_llseek(filp, 0, SEEK_END);
	offset = vfs_llseek(filp, 0, SEEK_SET);
	printk(KERN_INFO "file's actual len: %d\n", (int)l);
	if (l >= 17) {
		_len = 17;
	} else if (l >= 12) {
		_len = 12;
	} else {
		printk( "mac address is too short\n");
		return -1;
	}
	printk(KERN_INFO "file's len to be read: %d\n", _len);

	fs = get_fs();
	set_fs(get_ds());

	l = vfs_read(filp, (char __user *)mac, (size_t)_len, &offset);
	set_fs(fs);
	printk(KERN_INFO "mac string len actually read: %d\n", (int)l);
	if (l > 12) {
		if ((mac[2] == ':') && (mac[5] == ':'))
			_len = 17;
		else
			_len = 12;
	} else if (12 == l) {
		_len = 12;
	} else {
		printk(KERN_INFO "ethernet mac address not valid: %s\n", mac);
		return -1;
	}

	*len = _len;
	mac[_len] = '\0';
	printk(KERN_INFO "ethernet mac address read from file: %s, len: %d\n", mac, _len);
	for (i = 0; i < _len; i++) {
		if (!isxdigit(mac[i]) && ':' != mac[i]) {
			printk(KERN_INFO "mac address has invalid char: %c\n", mac[i]);
			return -1;
		}
	}

	return 0;
}

/* if all the paths are not usable, use random mac each boot */
static const char *get_def_mac_addr(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct file *filp;
    loff_t offset = 0;
    char def_mac[20] = "";
    const char *str;
    mm_segment_t fs;
    int len;
    int ret;

#define ETH_MAC_ADDR_BURNED_PATH "/sys/miscinfo/infos/ethmac"
//#define ETH_MAC_ADDR_BURNED_PATH "/data/mac_address"
#define ETH_MAC_ADDR_PATH "/config/mac_address.bin"
#define ETH_MAC_ADDR_RANDDOM_PATH "/data/mac_address_random"

    filp = filp_open(ETH_MAC_ADDR_BURNED_PATH, O_RDONLY, 0);
    if (IS_ERR_OR_NULL(filp)) {
        printk(KERN_INFO "file %s can't be opened\n", ETH_MAC_ADDR_BURNED_PATH);
    } else {
        if (!read_mac_address(filp, def_mac, &len)) {
            parse_mac_address(def_mac, len);
            printk(KERN_INFO "use burned mac address: ");
            print_mac_address(g_default_mac_addr);
            filp_close(filp, current->files);
            return g_default_mac_addr;
        }
        filp_close(filp, current->files);
    }

    str = of_get_property(np, "local-mac-address", NULL);
    if (str == NULL) {
        printk(KERN_INFO "no local-mac-address in dts\n");
    } else {
        printk(KERN_INFO "local-mac-address: ");
        print_mac_address(str);
        memcpy(g_default_mac_addr, str, ETH_MAC_LEN);
    }

    ret = of_property_read_string(np, "random-mac-address", &str);
    if (ret) {
        printk(KERN_INFO "no random-mac-address in dts\n");
    } else {
        printk(KERN_INFO "random-mac-address: %s\n", str);
        if (!strcmp("okay", str))
            goto random_mac;
    }

    printk(KERN_INFO "no mac burned, use default mac address: ");
    print_mac_address(g_default_mac_addr);
    return g_default_mac_addr;

random_mac:
    filp = filp_open(ETH_MAC_ADDR_PATH, O_RDONLY, 0);
    if (IS_ERR_OR_NULL(filp)) {
        printk(KERN_INFO "file %s can't be opened\n", ETH_MAC_ADDR_PATH);
    } else {
        if (!read_mac_address(filp, def_mac, &len)) {
            parse_mac_address(def_mac, len);
            printk(KERN_INFO "use mac stored in file: ");
            print_mac_address(g_default_mac_addr);
            filp_close(filp, current->files);
            return g_default_mac_addr;
        }
        filp_close(filp, current->files);
    }

    get_random_bytes(def_mac, ETH_MAC_LEN);
    memcpy(g_default_mac_addr + 3, def_mac + 3, ETH_MAC_LEN - 3);

    fs = get_fs();
    set_fs(get_ds());

    filp = filp_open(ETH_MAC_ADDR_RANDDOM_PATH, O_RDONLY, 0);
    if (IS_ERR_OR_NULL(filp)) {
        filp = filp_open(ETH_MAC_ADDR_RANDDOM_PATH, O_WRONLY | O_CREAT, 0640);
        if (!IS_ERR_OR_NULL(filp)) {
            printk(KERN_INFO "use random mac generated: ");
            print_mac_address(g_default_mac_addr);
            vfs_write(filp, (char __user *)g_default_mac_addr, ETH_MAC_LEN, &offset);
            filp_close(filp, current->files);
        }
    } else {
        if (vfs_read(filp, (char __user *)def_mac, ETH_MAC_LEN, &offset)
            == ETH_MAC_LEN) {
            memcpy(g_default_mac_addr, def_mac, ETH_MAC_LEN);
            printk(KERN_INFO "use random mac stored: ");
            print_mac_address(g_default_mac_addr);
        }
        filp_close(filp, current->files);
    }

    set_fs(fs);

    return g_default_mac_addr;
}

static void print_frame_data(void *frame, int len)
{
    int i;
    unsigned char *tmp = (unsigned char *)frame;

    for (i = 0; i < len; i++) {
        printk(KERN_INFO "%02x ",  (unsigned int)(*tmp));
        if (0xF == (i & 0xF)) {
            printk(KERN_INFO "\n");
        }
        tmp++;
    }
    printk(KERN_INFO "\n");
}

static void print_tx_bds(ec_priv_t * priv)
{
    int     i;
    ec_bd_t *tx_bds = priv->tx_bd_base;
    volatile ec_bd_t *buf;

    printk(KERN_INFO "---- tx ring status ----\n");
    printk(KERN_INFO "tx_bd_base = 0x%p, tx_full = %u\n", priv->tx_bd_base, (unsigned)priv->tx_full);
    printk(KERN_INFO "cur_tx = 0x%p, skb_cur = %u\n", priv->cur_tx, (unsigned)priv->skb_cur);
    printk(KERN_INFO "dirty_tx = 0x%p, skb_dirty = %u\n", priv->dirty_tx, (unsigned)priv->skb_dirty);

    printk(KERN_INFO "---- tx bds ----\n");
    printk(KERN_INFO "     status\t control\t buf addr\n");
    for (i = 0; i < TX_RING_SIZE; i++) {
        buf = &tx_bds[i];
        printk(KERN_INFO "%03d: 0x%08x\t 0x%08x\t 0x%08x\n", i, (unsigned int) buf->status,
               (unsigned int) buf->control, (unsigned int) buf->buf_addr);
    }
}

static void print_rx_bds(ec_priv_t * priv)
{
    int     i;
    ec_bd_t *rx_bds = priv->rx_bd_base;
    volatile ec_bd_t *buf;

    printk(KERN_INFO "---- rx ring status ----\n");
    printk(KERN_INFO "rx_bd_base = 0x%p\n", priv->rx_bd_base);
    printk(KERN_INFO "cur_rx = 0x%p\n", priv->cur_rx);

    printk(KERN_INFO "---- rx bds ----\n");
    printk(KERN_INFO "     status\t control\t buf addr\n");
    for (i = 0; i < RX_RING_SIZE; i++) {
        buf = &rx_bds[i];
        printk(KERN_INFO "%03d: 0x%08x\t 0x%08x\t 0x%08x\n", i, (unsigned int) buf->status,
               (unsigned int) buf->control, (unsigned int) buf->buf_addr);
    }
}

void print_phy_register(ec_priv_t *ecp)
{
#if 0
	printk(KERN_INFO "phy MII_BMCR: 0x%x\n", (uint)read_phy_reg(ecp, MII_BMCR));
	printk(KERN_INFO "phy MII_BMSR: 0x%x\n", (uint)read_phy_reg(ecp, MII_BMSR));
	printk(KERN_INFO "phy MII_PHYSID1: 0x%x\n", (uint)read_phy_reg(ecp, MII_PHYSID1));
	printk(KERN_INFO "phy MII_PHYSID2: 0x%x\n", (uint)read_phy_reg(ecp, MII_PHYSID2));
	printk(KERN_INFO "phy MII_ADVERTISE: 0x%x\n", (uint)read_phy_reg(ecp, MII_ADVERTISE));
	printk(KERN_INFO "phy MII_LPA: 0x%x\n", (uint)read_phy_reg(ecp, MII_LPA));
	printk(KERN_INFO "phy MII_CTRL1000: 0x%x\n", (uint)read_phy_reg(ecp, MII_CTRL1000));
	printk(KERN_INFO "phy MII_STAT1000: 0x%x\n", (uint)read_phy_reg(ecp, MII_STAT1000));
	printk(KERN_INFO "phy MII_MMD_CTRL: 0x%x\n", (uint)read_phy_reg(ecp, MII_MMD_CTRL));
	printk(KERN_INFO "phy MII_MMD_DATA: 0x%x\n", (uint)read_phy_reg(ecp, MII_MMD_DATA));
	printk(KERN_INFO "phy MII_ESTATUS: 0x%x\n", (uint)read_phy_reg(ecp, MII_ESTATUS));
	printk(KERN_INFO "phy MII_DCOUNTER: 0x%x\n", (uint)read_phy_reg(ecp, MII_DCOUNTER));
	printk(KERN_INFO "phy MII_FCSCOUNTER: 0x%x\n", (uint)read_phy_reg(ecp, MII_FCSCOUNTER));
	printk(KERN_INFO "phy MII_NWAYTEST: 0x%x\n", (uint)read_phy_reg(ecp, MII_NWAYTEST));
	printk(KERN_INFO "phy MII_RERRCOUNTER: 0x%x\n", (uint)read_phy_reg(ecp, MII_RERRCOUNTER));
	printk(KERN_INFO "phy MII_SREVISION: 0x%x\n", (uint)read_phy_reg(ecp, MII_SREVISION));
	printk(KERN_INFO "phy MII_EXPANSION: 0x%x\n", (uint)read_phy_reg(ecp, MII_EXPANSION));
	printk(KERN_INFO "phy MII_RESV1: 0x%x\n", (uint)read_phy_reg(ecp, MII_RESV1));
	printk(KERN_INFO "phy MII_LBRERROR: 0x%x\n", (uint)read_phy_reg(ecp, MII_LBRERROR));
	printk(KERN_INFO "phy MII_PHYADDR: 0x%x\n", (uint)read_phy_reg(ecp, MII_PHYADDR));
	printk(KERN_INFO "phy MII_RESV2: 0x%x\n", (uint)read_phy_reg(ecp, MII_RESV2));
	printk(KERN_INFO "phy MII_TPISTATUS: 0x%x\n", (uint)read_phy_reg(ecp, MII_TPISTATUS));
	printk(KERN_INFO "phy MII_NCONFIG: 0x%x\n", (uint)read_phy_reg(ecp, MII_NCONFIG));	
#endif
}

void print_mac_register(ec_priv_t *ecp)
{
#if 0
    volatile ethregs_t *hw_regs = ecp->hwrp;

	printk(KERN_INFO "CMU_DEVCLKEN1:0x%x\n", act_readl(CMU_DEVCLKEN1));
	printk(KERN_INFO "MFP_CTL0:0x%x\n", act_readl(MFP_CTL0));
	printk(KERN_INFO "PAD_DRV0:0x%x\n", act_readl(PAD_DRV0));
	printk(KERN_INFO "PAD_PULLCTL0:0x%x\n", act_readl(PAD_PULLCTL0));
	printk(KERN_INFO "GPIO_AOUTEN:0x%x\n", act_readl(GPIO_AOUTEN));
	printk(KERN_INFO "GPIO_AINEN:0x%x\n", act_readl(GPIO_AINEN));
	printk(KERN_INFO "GPIO_ADAT:0x%x\n", act_readl(GPIO_ADAT));

    /* CSR0~20 */
    printk(KERN_INFO "MAC_CSR0:0x%08lx, address:%p\n", hw_regs->er_busmode, &hw_regs->er_busmode);
    printk(KERN_INFO "MAC_CSR1:0x%08lx, address:%p\n", hw_regs->er_txpoll, &hw_regs->er_txpoll);
    printk(KERN_INFO "MAC_CSR2:0x%08lx, address:%p\n", hw_regs->er_rxpoll, &hw_regs->er_rxpoll);
    printk(KERN_INFO "MAC_CSR3:0x%08lx, address:%p\n", hw_regs->er_rxbdbase, &hw_regs->er_rxbdbase);
    printk(KERN_INFO "MAC_CSR4:0x%08lx, address:%p\n", hw_regs->er_txbdbase, &hw_regs->er_txbdbase);
    printk(KERN_INFO "MAC_CSR5:0x%08lx, address:%p\n", hw_regs->er_status, &hw_regs->er_status);
    printk(KERN_INFO "MAC_CSR6:0x%08lx, address:%p\n", hw_regs->er_opmode, &hw_regs->er_opmode);
    printk(KERN_INFO "MAC_CSR7:0x%08lx, address:%p\n", hw_regs->er_ienable, &hw_regs->er_ienable);
    printk(KERN_INFO "MAC_CSR8:0x%08lx, address:%p\n", hw_regs->er_mfocnt, &hw_regs->er_mfocnt);
    printk(KERN_INFO "MAC_CSR9:0x%08lx, address:%p\n", hw_regs->er_miimng, &hw_regs->er_miimng);
    printk(KERN_INFO "MAC_CSR10:0x%08lx, address:%p\n", hw_regs->er_miism, &hw_regs->er_miism);
    printk(KERN_INFO "MAC_CSR11:0x%08lx, address:%p\n", hw_regs->er_imctrl, &hw_regs->er_imctrl);
    printk(KERN_INFO "MAC_CSR16:0x%08lx, address:%p\n", hw_regs->er_maclow, &hw_regs->er_maclow);
    printk(KERN_INFO "MAC_CSR17:0x%08lx, address:%p\n", hw_regs->er_machigh, &hw_regs->er_machigh);
    printk(KERN_INFO "MAC_CSR18:0x%08lx, address:%p\n", hw_regs->er_cachethr, &hw_regs->er_cachethr);
    printk(KERN_INFO "MAC_CSR19:0x%08lx, address:%p\n", hw_regs->er_fifothr, &hw_regs->er_fifothr);
    printk(KERN_INFO "MAC_CSR20:0x%08lx, address:%p\n", hw_regs->er_flowctrl, &hw_regs->er_flowctrl);

    printk(KERN_INFO "MAC_CTRL: 0x%x:0x%x\n", MAC_CTRL, act_readl(MAC_CTRL));
#endif
    return;
}

/*----------------------------------- mii hooks -------------------------------*/

/**
 * ec_mdio_read -  hook for struct mii_if_info{.mdio_read}
 */
static int ec_mdio_read(struct net_device *dev, int phy_addr, int reg_addr)
{
    ec_priv_t *ecp = netdev_priv(dev);
    printk(KERN_INFO "read phy reg-%x\n", reg_addr);
    return (read_phy_reg(ecp, reg_addr));
}


/**
 * ec_mdio_write -  hook for struct mii_if_info{.mdio_write}
 */
static void ec_mdio_write(struct net_device *dev, int phy_addr, int reg_addr, int val)
{
    ec_priv_t *ecp = netdev_priv(dev);
    write_phy_reg(ecp, reg_addr, val);
    printk(KERN_INFO "write phy reg-%x, value-%x\n", reg_addr, val);
}


/*---------------------------------- MAC routines -----------------------------*/

static inline void raw_tx_bd_init(ec_priv_t * ecp)
{
    int     i;
    volatile ec_bd_t *tx_bds = ecp->tx_bd_base;

    for (i = 0; i < TX_RING_SIZE; i++) {
        tx_bds[i] = (ec_bd_t) {
            .status = 0,        /* host own it */
            .control = TXBD_CTRL_IC,
            .buf_addr = 0,
            .reserved = 0
        };
    }
    tx_bds[i - 1].control |= TXBD_CTRL_TER;

    return;
}


static inline void raw_rx_bd_init(ec_priv_t * ecp)
{
    int     i;
    volatile ec_bd_t *rx_bds = ecp->rx_bd_base;

    for (i = 0; i < RX_RING_SIZE; i++) {
        rx_bds[i] = (ec_bd_t) {
            .status = RXBD_STAT_OWN,
            .control = RXBD_CTRL_RBS1(PKG_MAX_LEN),
            .buf_addr = 0,
            .reserved = 0
        };
    }
    rx_bds[i - 1].control |= RXBD_CTRL_RER;

    return;
}


/**
 * get_skb_aligned - get a skb which the address of skb->data is 4B aligned
 */
static struct sk_buff *get_skb_aligned(unsigned int len)
{
    int     offset;
    struct sk_buff *skb = NULL;

    if (NULL == (skb = dev_alloc_skb(len))) {
        return (NULL);
    }
    
    offset = (unsigned long) skb->data & EC_SKB_ALIGN_BITS_MASK;
    if (unlikely(offset)) {
        skb_reserve(skb, EC_SKB_ALIGNED - offset);
    }

    return (skb);
}


static inline void free_rxtx_skbs(struct sk_buff **array, int len)
{
    int     i;

    for (i = 0; i < len; i++) {
        if (NULL != array[i]) {
            dev_kfree_skb_any(array[i]);
            array[i] = NULL;
        }
    }
    return;
}


/**
 * prepare_tx_bd -- preparation for tx buffer descripters
 *
 * always success
 */
static inline int prepare_tx_bd(ec_priv_t * ecp)
{

    volatile ec_bd_t *tx_bds_head = ecp->tx_bd_base;

    ecp->cur_tx = (ec_bd_t *)tx_bds_head;
    ecp->dirty_tx = (ec_bd_t *)tx_bds_head;
    ecp->skb_cur = 0;
    ecp->skb_dirty = 0;
    ecp->tx_full = false;

    free_rxtx_skbs(ecp->tx_skb, TX_RING_SIZE);
    raw_tx_bd_init(ecp);

    return (0);
}


/**
 * prepare_rx_bd -- preparation for rx buffer descripters
 *
 * return 0 if success, return -1 if fail
 */
static int prepare_rx_bd(ec_priv_t * ecp)
{
    int     i;
    struct sk_buff *skb = NULL;
    volatile ec_bd_t *rx_bds_head = ecp->rx_bd_base;

    printk(KERN_INFO "ecp->rx_bd_base: %p\n", ecp->rx_bd_base);
    ecp->cur_rx = (ec_bd_t *)rx_bds_head;
    raw_rx_bd_init(ecp);

    free_rxtx_skbs(ecp->rx_skb, RX_RING_SIZE);
    
    for (i = 0; i < RX_RING_SIZE; i++) {
        if ((skb = get_skb_aligned(PKG_MAX_LEN))) {
            ecp->rx_skb[i] = skb; 
            /* should be 4-B aligned */
            rx_bds_head[i].buf_addr = dma_map_single(&ecp->netdev->dev,
                    skb->data, PKG_MAX_LEN, DMA_FROM_DEVICE);
            //printk(KERN_INFO "rx_bds_head[%d].buf_addr:0x%lx\n", i, rx_bds_head[i].buf_addr);
        } else {
            printk(KERN_INFO "can't alloc skb\n");
            free_rxtx_skbs(ecp->rx_skb, i);
            raw_rx_bd_init(ecp);
            return (-1);
        }
    }
    printk(KERN_INFO "ecp->cur_rx: %p\n", ecp->cur_rx);

    return (0);
}


/* suitable for less than 7 chars of string */
static inline int string_to_hex(char *str, int len)
{
    int     val;
    int     i;
    char    ch;

    val = 0;
    for (i = 0; i < len; i++) {
        ch = str[i];
        if ('0' <= ch && ch <= '9') {
            val = (val << 4) + ch - '0';
        } else if ('a' <= ch && ch <= 'f') {
            val = (val << 4) + 10 + ch - 'a';
        } else if ('A' <= ch && ch <= 'F') {
            val = (val << 4) + 10 + ch - 'A';
        } else {
            return (-1);
        }
    }

    return (val);
}


/**
 * parse_mac_addr -- parse string of mac address to number mac address
 *
 * return 0 if success, negative value if fail
 */
static inline int parse_mac_addr(ec_priv_t * ecp, char *mac)
{
    int     i;
    int     j;
    int     result;
    
    /* string of mac - such as "01:02:03:04:05:06" */
    if (17 != strlen(mac)) {
        return (-1);
    }

    for (i = 0, j = 2; i < 5; i++, j += 3) {
        if (':' != mac[j]) {
            return (-1);
        }
    }

    for (i = 0, j = 0; i < 6; i++, j += 3) {
        result = string_to_hex(mac + j, 2);
        if (-1 != result) {
            ecp->overrided_mac[i] = (char) result;
        } else {
            return (result);
        }
    }

    return (0);
}


static inline void fill_macaddr_regs(ec_priv_t * ecp, const char *mac_addr)
{
    volatile ethregs_t *hw_regs = ecp->hwrp;

    hw_regs->er_maclow = *(unsigned long *) mac_addr;
    hw_regs->er_machigh = *(unsigned short *) (mac_addr + 4);
    return;
}


static inline void set_mac_addr(ec_priv_t * ecp)
{
    if (ecp->mac_addr) {
        fill_macaddr_regs(ecp, ecp->mac_addr);
        printk(KERN_INFO "using previous one\n");
        return;
    }
    
    if ('?' != macaddr[0]) {
        printk(KERN_INFO "parse mannual mac address\n");
        if ((0 == parse_mac_addr(ecp, macaddr)) && 
            compare_ether_addr(macaddr, g_default_mac_addr)) {

            fill_macaddr_regs(ecp, ecp->overrided_mac);
            ecp->mac_addr = ecp->overrided_mac;
            ecp->mac_overrided = true;
            return;
        }
    }

    printk(KERN_INFO "set default mac address \n");

    fill_macaddr_regs(ecp, g_default_mac_addr);
    ecp->mac_addr = g_default_mac_addr;
    ecp->mac_overrided = false;
    return;
}


/* NOTE: it has side effect for dest parameter */
#define COPY_MAC_ADDR(dest, mac)  do {\
    *(unsigned short *)(dest) = *(unsigned short *)(mac); \
    *(unsigned short *)((dest) + 4) = *(unsigned short *)((mac) + 2); \
    *(unsigned short *)((dest) + 8) = *(unsigned short *)((mac) + 4); \
    (dest) += 12; \
}while (0)


/**
 * build_setup_frame -- build setup-frame of mac address filter  in @buffer
 *
 * @buf_len should be longer than or equal  SETUP_FRAME_LEN (192 bytes), but we only 
 * use SETUP_FRAME_LEN bytes exactly.
 *
 * return the address of @buffer if success, or NULL if not
 */
static char *build_setup_frame(ec_priv_t * ecp, char *buffer, int buf_len)
{
    char    broadcast_mac[ETH_MAC_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char   *frame = buffer;
    char   *mac;

    if (NULL == buffer || buf_len < SETUP_FRAME_LEN) {
        printk(KERN_INFO "error parameters\n");
        return (NULL);
    }

    memset(frame, 0, SETUP_FRAME_LEN);

    mac = (char *) ecp->mac_addr;
    COPY_MAC_ADDR(frame, mac);
    
    mac = broadcast_mac;
    COPY_MAC_ADDR(frame, mac);

    /* fill multicast addresses */
    if (!ecp->all_multicast && ecp->multicast) {
        int     i;
        int     count = ecp->multicast_list.count;

        if (count > MULTICAST_LIST_LEN) {
            count = MULTICAST_LIST_LEN;
        }

        for (i = 0; i < count; i++) {
            mac = ecp->multicast_list.mac_array[i];
            COPY_MAC_ADDR(frame, mac);
        }
    }

    if (ecp->msg_enable) {
        INFO_GREEN("overrided : %s -- multicast : %s\n",
            ecp->mac_overrided ? "true" : "false",
            ecp->multicast ? "true" : "false");
    }

    return (buffer);
}


/**
 * transmit_setup_frame -- transmit setup-frame of  mac address filter
 *
 * the function is not thread-safety, thus in multithread envirement should hold ec_prive_t{.lock}.
 * and before call it, ec_prive_t{.mac_addr} should point to a suitalbe mac address used
 *
 * MAC will raise CSR5.ETI interrupt after transmission of setup-frame, so we use uniform 
 * manner  to deal with it.
 *
 * return 0 if success, -1 if fail
 */
static int transmit_setup_frame(ec_priv_t *ecp)
{
    struct sk_buff *skb = NULL;
    volatile ec_bd_t *buf_des = ecp->cur_tx;

    if (ecp->tx_full) {
        /* may happen when change macs if not in open ethdev  */
        printk(KERN_INFO "error : tx buffer is full.\n");
        return (-1);
    }
    
    /* will build a setup-frame in a skb */
    skb = get_skb_aligned(SETUP_FRAME_RESV_LEN);
    if (NULL == skb) {
        printk( "error : no memory for setup frame.\n");
        return (-1);
    }

    skb_put(skb, SETUP_FRAME_LEN);

    /* address of skb->data should be 4-bytes aligned */
    if (NULL == build_setup_frame(ecp, skb->data, SETUP_FRAME_LEN)) {
        printk( "error : building of setup-frame failed.\n");
        dev_kfree_skb_any(skb);
        return (-1);
    }

    /* send it out as normal packet */
    ecp->tx_skb[ecp->skb_cur] = skb;
    ecp->skb_cur = (ecp->skb_cur + 1) & TX_RING_MOD_MASK;

    /*
     * Push the data cache so the NIC does not get stale memory data.
     */
    buf_des->buf_addr = dma_map_single(&ecp->netdev->dev, skb->data, PKG_MAX_LEN, DMA_TO_DEVICE);

    buf_des->control &= (TXBD_CTRL_TER | TXBD_CTRL_IC); /* maintain these bits */
    buf_des->control |= TXBD_CTRL_SET;
    buf_des->control |= TXBD_CTRL_TBS1(SETUP_FRAME_LEN);
    mb();
    buf_des->status = TXBD_STAT_OWN;
    mb();

    /* when call the routine, TX and Rx should have already stopped */
    ecp->hwrp->er_opmode |= EC_OPMODE_ST;
    ecp->hwrp->er_txpoll = EC_TXPOLL_ST;  

    if (buf_des->control & TXBD_CTRL_TER)
        ecp->cur_tx = ecp->tx_bd_base;
    else
        ecp->cur_tx++;
    
    if (ecp->cur_tx == ecp->dirty_tx) {
        ecp->tx_full = true;
        netif_stop_queue(ecp->netdev);
    }

    /* resume old status */    
    ecp->hwrp->er_opmode &= ~EC_OPMODE_ST;  
	netif_stop_queue(ecp->netdev);
    printk(KERN_INFO "The end of transmit_setup_frame\n");
    return (0);
}


/**
  * when reconfigrate mac's mode, should stop tx and rx first.  so before call the following 
  * tow function, make sure tx and rx have stopped
 */
static inline void set_mode_promisc(ec_priv_t * ecp, bool supported)
{
    volatile ethregs_t *hw_regs = ecp->hwrp;

    ecp->promiscuous = supported;
    if (supported) {
        hw_regs->er_opmode |= EC_OPMODE_PR;
        EC_NOP;
    } else {
        hw_regs->er_opmode &= ~EC_OPMODE_PR;
        EC_NOP;
    }
    return;
}

static inline void set_mode_all_multicast(ec_priv_t * ecp, bool supported)
{
    volatile ethregs_t *hw_regs = ecp->hwrp;

    ecp->all_multicast = supported;
    if (supported)
        hw_regs->er_opmode |= EC_OPMODE_PM;
    else
        hw_regs->er_opmode &= ~EC_OPMODE_PM;

    return;
}

#if 0
static void mac_rmii_get_pin(void)
{
    unsigned long mfp_ctl0;
	unsigned long pad_drive;
    unsigned long pad_pull_ctl0;
	unsigned long mac_ctl_temp;
	
	//pad enable
    act_writel(0x2, PAD_CTL);
        
	/*mac mfp config*/
    mfp_ctl0 = act_readl(MFP_CTL0);
    mfp_ctl0 &= 0xfff8003f;        // RMII pin
    mfp_ctl0 |= 0x00000000;
    act_writel(mfp_ctl0, MFP_CTL0);

    mac_ctl_temp = act_readl(MAC_CTRL);
	act_writel(mac_ctl_temp | 0x1<<1, MAC_CTRL);	// use RMII
	
    /*mac rmii pad drive - level2*/
	pad_drive = act_readl(PAD_DRV0);
    pad_drive &= ~0x00ffc000;
	pad_drive |= 0x00554000;
	act_writel(pad_drive, PAD_DRV0);

	/*mac rmii pad drive - pad pull ctl*/
	pad_pull_ctl0 = act_readl(PAD_PULLCTL0);
    pad_pull_ctl0 &= ~0x00010000;
	pad_pull_ctl0 |= 0x0;
	act_writel(pad_pull_ctl0, PAD_PULLCTL0);
}

static int get_pin_count = 0;

static int hw_rmii_get_pin(void) {
	int result = 0;

	get_pin_count++;
	//printk(KERN_INFO "get_pin_count:%d\n", get_pin_count);
	if (get_pin_count > 1) {
		return 0;
	}
	
	result = asoc_mfp_lock(MOD_ID_ETHERNET, MFP_OPT_CAN_SLEEP, NULL);
	return result;
}

static int hw_rmii_release_pin(void) {
	int result = 0;

	get_pin_count--;
	//printk(KERN_INFO "get_pin_count:%d\n", get_pin_count);
	if (get_pin_count < 0) {
		get_pin_count = 0;
		return 0;
	}

	result = asoc_mfp_unlock(MOD_ID_ETHERNET, MFP_OPT_CAN_SLEEP);
	return result;
}
#endif

static void mac_init(ec_priv_t *ecp)
{
	volatile ethregs_t *hw_regs = ecp->hwrp;

//	printk(KERN_INFO "%s %d\n",__FUNCTION__,__LINE__);
	/* hardware soft reset, and set bus mode */
	hw_regs->er_busmode |= EC_BMODE_SWR;
	do {
		udelay(10);
	} while (hw_regs->er_busmode & EC_BMODE_SWR);

	/* select clk input from external phy */

//	printk(KERN_INFO "before MAC_CTRL: 0x%x\n", (unsigned)getl(MAC_CTRL));
	putl(getl(MAC_CTRL) &(~(0x1<<1)), MAC_CTRL);			//no need to inverter ref clk
//	printk(KERN_INFO "after MAC_CTRL: 0x%x\n", (unsigned)getl(MAC_CTRL));

	hw_regs->er_miism &= 0x0;
	hw_regs->er_miism |= 0xcc000000;
	//putl(getl(MAC_CSR10) | 0x40000000, MAC_CSR10);
//	printk(KERN_INFO "MAC_CSR10: 0x%x\n", (unsigned)getl(MAC_CSR10));
//    printk(KERN_INFO "------------MAC_CSR10:0x%x\n", (unsigned)hw_regs->er_miism);


//	printk(KERN_INFO "----------CMU_DEVCLKEN1:0x%x\n", act_readl(CMU_DEVCLKEN1));
//	printk(KERN_INFO "----------CMU_DEVRST1:0x%x\n", act_readl(CMU_DEVRST1));
	
	/* for gl5202 fpga test, PBL = 16, 5203 default 16 */
	//hw_regs->er_busmode |= 0x1000;

	/* physical address */
	hw_regs->er_txbdbase = ecp->tx_bd_paddr;
	hw_regs->er_rxbdbase = ecp->rx_bd_paddr;
//	printk(KERN_INFO "csr4-txbdbase:0x%x\n", (unsigned)hw_regs->er_txbdbase);
//	printk(KERN_INFO "csr3-rxbdbase:0x%x\n", (unsigned)hw_regs->er_rxbdbase);

	/* set flow control mode, force transmitor pause about 100ms */
/*
	hw_regs->er_cachethr = EC_CACHETHR_CPTL(0x0) | EC_CACHETHR_CRTL(0x0) | EC_CACHETHR_PQT(0x4FFF);
//	hw_regs->er_fifothr = EC_FIFOTHR_FPTL(0x40) | EC_FIFOTHR_FRTL(0x10);
	hw_regs->er_fifothr = EC_FIFOTHR_FPTL(0x150) | EC_FIFOTHR_FRTL(0x84);
*/
	hw_regs->er_cachethr = EC_CACHETHR_CPTL(0x0) | EC_CACHETHR_CRTL(0x0) | EC_CACHETHR_PQT(0x4FFF);
	hw_regs->er_fifothr = EC_FIFOTHR_FPTL(0x40) | EC_FIFOTHR_FRTL(0x10);//hw_regs->er_fifothr = EC_FIFOTHR_FPTL(0x150) | EC_FIFOTHR_FRTL(0x84);
	hw_regs->er_flowctrl = EC_FLOWCTRL_ENALL;

	hw_regs->er_opmode |= EC_OPMODE_FD;
	hw_regs->er_opmode |= EC_OPMODE_SPEED(0); //100M
//	hw_regs->er_opmode |= EC_OPMODE_SF;  /* start transmit ONLY after whole packet get in tfifo */

    //hw_regs->er_busmode |= EC_BMODE_TAP(0x1);
    //hw_regs->er_busmode |= EC_BMODE_BAR;
//    printk(KERN_INFO "hw_regs->er_busmode:0x%x\n", (unsigned)hw_regs->er_busmode);

	/* default support PR, here clear it
	 * XXX: due to MAC constraint, after write a reg, can't read it immediately
	 * (write of regs has tow beats delay).
	 */
	hw_regs->er_opmode &= ~EC_OPMODE_PR;
//	printk(KERN_INFO "hw_regs->er_opmode:0x%x\n", (unsigned)hw_regs->er_opmode);

	
	//interrupt mitigation control register 
	hw_regs->er_imctrl = 0x004e0000; //NRP =7,RT =1,CS=0

#if defined(ETHENRET_MAC_LOOP_BACK) || defined(ETHENRET_PHY_LOOP_BACK)
	hw_regs->er_opmode |= EC_OPMODE_RA;
#endif

#ifdef ETHENRET_MAC_LOOP_BACK
	/* mac internal loopback */
	hw_regs->er_opmode |= EC_OPMODE_LP;
	printk(KERN_INFO "MAC operation mode: 0x%x\n", (unsigned)hw_regs->er_opmode);
#endif
	//hw_regs->er_ienable = EC_IEN_ALL;   //all interrupt enable
}

static int _init_hardware(ec_priv_t *ecp,int clk_reset,int phy_reset)
{
	ethernet_clock_enable(clk_reset);

	/* ATC2605 ONLY support output 50MHz RMII_REF_CLK to MAC, so phy hw init first */
	if (ecp->phy_ops->phy_hw_init)
		ecp->phy_ops->phy_hw_init(ecp);
//	printk(KERN_INFO "%s %d\n",__FUNCTION__,__LINE__);
	mac_init(ecp);
	print_mac_register(ecp);

if(phy_reset){
	if (gpio_is_valid(ecp->phy_power_gpio.gpio)) { 
		gpio_direction_output(ecp->phy_power_gpio.gpio,  !ecp->phy_power_gpio.active);
	}	
	if (gpio_is_valid(ecp->phy_reset_gpio.gpio)){	        
		gpio_direction_output(ecp->phy_reset_gpio.gpio,   ecp->phy_reset_gpio.active);		
	}
	mdelay(10);
	if (gpio_is_valid(ecp->phy_reset_gpio.gpio)){	        
		gpio_direction_output(ecp->phy_reset_gpio.gpio,   !ecp->phy_reset_gpio.active);		
	}	
}
	/**********TEST FOR TAIWAN START******/
	#if 0
	putl(getl(GPIO_AOUTEN) | (0x1 << 22), GPIO_AOUTEN);
    printk(KERN_INFO "%s %d gpioen:0x%x\n",__FUNCTION__,__LINE__,getl(GPIO_COUTEN));
	putl(getl(GPIO_ADAT) & (0x0 << 22), GPIO_ADAT);
	printk(KERN_INFO "%s %d gpio:0x%x\n",__FUNCTION__,__LINE__,getl(GPIO_CDAT));
    mdelay(150);
	putl(getl(GPIO_AOUTEN) & (0x0 << 22), GPIO_AOUTEN);
	#endif//0
	 /**********TEST FOR TAIWAN end******/
	set_mac_addr(ecp);
//	INFO_BLUE("mac address: ");
//	print_mac_address(ecp->mac_addr);

if(phy_reset){
	if (ecp->phy_ops->phy_init(ecp)) {
		printk("error : initialize PHY fail\n");
	}
	printk(KERN_INFO "%s %d\n",__FUNCTION__,__LINE__);
	print_phy_register(ecp);
}		
	/*enable RMII_REF_CLK and EXTIRQ pad and disable the other 6 RMII pad, gl5302 bug amend*/
	/*enable the other 6 RMII pad, gl5302 bug amend*/
    #if 0	
      if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
		atc260x_reg_setbits(ecp->atc260x, atc2603_PAD_EN, 0x7e, 0x7e);
		udelay(100);
	}
      #endif //0
	return 0;
}

static int _deinit_hardware(ec_priv_t *ecp){
	if (gpio_is_valid(ecp->phy_power_gpio.gpio)) { 
		gpio_direction_output(ecp->phy_power_gpio.gpio,  ecp->phy_power_gpio.active);
	}
}

#ifdef DETECT_POWER_SAVE
static void detect_power_save_timer_func(unsigned long data);

static void init_power_save_timer(ec_priv_t *ecp)
{
    printk(KERN_INFO "\n");
    init_timer(&ecp->detect_timer);
    ecp->detect_timer.data = (unsigned long)ecp;
    ecp->detect_timer.function = detect_power_save_timer_func;
}

static void start_power_save_timer(ec_priv_t *ecp, const unsigned ms)
{
    printk(KERN_INFO "\n");
    mod_timer(&ecp->detect_timer, jiffies + msecs_to_jiffies(ms));
}

static void stop_power_save_timer(ec_priv_t *ecp)
{
    printk(KERN_INFO "\n");
    if (timer_pending(&ecp->detect_timer))
        del_timer_sync(&ecp->detect_timer);

    cancel_work_sync(&ecp->power_save_work);
    flush_workqueue(power_save_queue);
}

static void enable_hardware(ec_priv_t *ecp)
{
    unsigned long flags = 0;
    int temp;

    temp = read_phy_reg(ecp, MII_BMCR);
    printk(KERN_INFO "MII_BMCR: 0x%x\n", (u32)temp);
    write_phy_reg(ecp, MII_BMCR, temp & ~BMCR_PDOWN);
    printk(KERN_INFO "exit POWER DOWN, MII_BMCR: 0x%x\n", (u32)read_phy_reg(ecp, MII_BMCR));

    spin_lock_irqsave(&ecp->lock, flags);
    ecp->enable = true;
    spin_unlock_irqrestore(&ecp->lock, flags);
}

static void disable_hardware(ec_priv_t *ecp)
{
    unsigned long flags = 0;
    unsigned short phy_int_status;
    int temp;

    phy_int_status = atc260x_reg_read(ecp->atc260x, atc2603_PHY_INT_STAT);
    if (phy_int_status & 0x1) {
        printk(KERN_INFO "already linked, not to power down\n");
        return;
    }

    spin_lock_irqsave(&ecp->lock, flags);
    if (ecp->linked) {
        spin_unlock_irqrestore(&ecp->lock, flags);
        printk(KERN_INFO "already linked, not to power down\n");
        return;
    }
    ecp->enable = false;
    spin_unlock_irqrestore(&ecp->lock, flags);

    temp = read_phy_reg(ecp, MII_BMCR);
    printk(KERN_INFO "MII_BMCR: 0x%x\n", (u32)temp);
    write_phy_reg(ecp, MII_BMCR, temp | BMCR_PDOWN);
    printk(KERN_INFO "enter POWER DOWN, MII_BMCR: 0x%x\n", (u32)read_phy_reg(ecp, MII_BMCR));
}


static void ethernet_power_save(struct work_struct *work)
{
    ec_priv_t *ecp = (ec_priv_t *)container_of(work, ec_priv_t, power_save_work);
    printk(KERN_INFO "ecp->enable: %u\n", ecp->enable);

    if (ecp->enable) {
        disable_hardware(ecp);
    } else {
        enable_hardware(ecp);
    }
}

static void detect_power_save_timer_func(unsigned long data)
{
    ec_priv_t *ecp = (ec_priv_t *)data;
    unsigned long flags;

    printk(KERN_INFO "ecp->enable: %u\n", ecp->enable);
    if (!ecp->opened) {
        printk(KERN_INFO "not opened yet\n");
        return;
    }

    spin_lock_irqsave(&ecp->lock, flags);
    if (ecp->linked) {
        spin_unlock_irqrestore(&ecp->lock, flags);
        printk(KERN_INFO "not linked yet\n");
        return;
    }
    spin_unlock_irqrestore(&ecp->lock, flags);

    if (ecp->enable) {
        mod_timer(&ecp->detect_timer, jiffies + msecs_to_jiffies(4000));
    } else {
        mod_timer(&ecp->detect_timer, jiffies + msecs_to_jiffies(4000));
    }

    queue_work(power_save_queue, &ecp->power_save_work);
}
#endif /* DETECT_POWER_SAVE */

static void set_phy_according_aneg(ec_priv_t *ecp)
{
	unsigned short old_bmcr;
	unsigned long phy_ctrl_status;

	old_bmcr = read_phy_reg(ecp, MII_BMCR);
//	printk(KERN_INFO "old MII_BMCR: 0x%04x\n", (unsigned)old_bmcr);
   #if 0
	if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
		phy_ctrl_status = atc260x_reg_read(ecp->atc260x, atc2603_PHY_CTRL);
		if (ETH_SPEED_10M == ecp->speed)
			atc260x_reg_write(ecp->atc260x, atc2603_PHY_CTRL, phy_ctrl_status & 0x1e);
		else
			atc260x_reg_write(ecp->atc260x, atc2603_PHY_CTRL, phy_ctrl_status | 0x1);

		printk(KERN_INFO "atc2603_PHY_CTRL: 0x%04x\n", (unsigned)atc260x_reg_read(ecp->atc260x, atc2603_PHY_CTRL));
	}
 	#endif
	if (ETH_SPEED_10M == ecp->speed)
		old_bmcr &= ~BMCR_SPEED100;
	else
		old_bmcr |= BMCR_SPEED100;

	if (ETH_DUP_FULL == ecp->duplex)
		old_bmcr |= BMCR_FULLDPLX;
	else
		old_bmcr &= ~BMCR_FULLDPLX;

	write_phy_reg(ecp, MII_BMCR, old_bmcr);
//	printk(KERN_INFO "new MII_BMCR: 0x%04x\n", (unsigned)read_phy_reg(ecp, MII_BMCR));
}




static void set_mac_according_aneg(ec_priv_t *ecp)
{
	volatile ethregs_t *hw_regs = ecp->hwrp;
	unsigned long old_mode;

//	 printk(KERN_INFO "%s\n",__func__);
	old_mode = hw_regs->er_opmode;
//	printk(KERN_INFO "opmode regs old value - 0x%x\n", (int)old_mode);

	hw_regs->er_opmode &= ~(EC_OPMODE_ST | EC_OPMODE_SR);

	if (ETH_SPEED_10M == ecp->speed)
		old_mode |= EC_OPMODE_10M;
	else
		old_mode &= ~EC_OPMODE_10M;

	if (ETH_DUP_FULL == ecp->duplex)
		old_mode |= EC_OPMODE_FD;
	else
		//old_mode &= ~EC_OPMODE_FD;
		/* always set full duplex to work around for both half/full mode!*/
		old_mode |= EC_OPMODE_FD;  /* mac bug! */

	/*set phy during mac stopped */
	set_phy_according_aneg(ecp);

	hw_regs->er_opmode = old_mode;
//	printk(KERN_INFO "hw_regs->er_opmode:0x%lx\n", hw_regs->er_opmode);
}


/**
 * ec_enet_tx -- sub-isr for tx interrupt
 */
static void subisr_enet_tx(ec_priv_t *ecp)
{
    struct net_device *dev = ecp->netdev;
    volatile ec_bd_t *bdp;
    struct sk_buff *skb;
    unsigned long status;

    spin_lock(&ecp->lock);
    bdp = ecp->dirty_tx;

    if (0 == ((status = bdp->status) & TXBD_STAT_OWN)) { /* don't enable CSR11 interrupt mitigation */
#if 0
	while (0 == ((status = bdp->status) & TXBD_STAT_OWN)) {
        if (bdp == ecp->cur_tx && !ecp->tx_full) 
            break;    /* tx queue is empty */
#endif

        skb = ecp->tx_skb[ecp->skb_dirty];
        dma_unmap_single(&ecp->netdev->dev, bdp->buf_addr, skb->len, DMA_TO_DEVICE);

        /* check for errors */
        if (status & TXBD_STAT_ES) {
            printk(KERN_INFO "tx error status : 0x%x\n", (unsigned int)status);
            if (netif_msg_tx_err(ecp)) {
                printk(KERN_INFO "position: %d\n", ecp->dirty_tx - ecp->tx_bd_base);
                print_tx_bds(ecp);
            }
            dev->stats.tx_errors++;
            if (status & TXBD_STAT_UF) 
			{
                dev->stats.tx_fifo_errors++;
				printk(KERN_INFO "tx error status : 0x%x\n", (unsigned int)status);
            }
            if (status & TXBD_STAT_EC) 
			{
                dev->stats.tx_aborted_errors++;
				printk(KERN_INFO "tx error status : 0x%x\n", (unsigned int)status);
            }
            if (status & TXBD_STAT_LC) 
			{
                dev->stats.tx_window_errors++;
				printk(KERN_INFO "tx error status : 0x%x\n", (unsigned int)status);
            }
            if (status & TXBD_STAT_NC) 
			{
                dev->stats.tx_heartbeat_errors++;
				//the mac ip has such a bug(misinformation)
				//printk(KERN_INFO "tx error status : 0x%x\n", (unsigned int)status);
            }
            if (status & TXBD_STAT_LO) 
			{
                dev->stats.tx_carrier_errors++;
				printk(KERN_INFO "tx error status : 0x%x\n", (unsigned int)status);
            }
        } 
		else 
		{
            dev->stats.tx_packets++;
        }

        /* some collions occurred, but sent packet ok eventually */
        if (status & TXBD_STAT_DE) 
		{
            dev->stats.collisions++;
        }

        if (netif_msg_tx_err(ecp)) {
            check_icmp_sequence(skb->data, __func__);
            printk(KERN_INFO "bdp->buf_addr:0x%lx\n", bdp->buf_addr);
            printk(KERN_INFO "tx frame:\n");
            print_frame_data(skb->data, skb->len);
        }

        dev_kfree_skb_any(skb);

        ecp->tx_skb[ecp->skb_dirty] = NULL;
        ecp->skb_dirty = (ecp->skb_dirty + 1) & TX_RING_MOD_MASK;

        if (bdp->control & TXBD_CTRL_TER)
            bdp = ecp->tx_bd_base;
        else
            bdp++;

        if (ecp->tx_full) {
            printk(KERN_INFO "tx bds available, skb_dirty:%d\n", ecp->skb_dirty);
            ecp->tx_full = false;
            if (netif_queue_stopped(dev))
                netif_wake_queue(dev);
        }
        if (netif_queue_stopped(dev))
	    netif_wake_queue(dev);
    }
//	else
//		printk(KERN_INFO "tx bds status:0x%x\n", (unsigned int)status);
#if 1
    else if (ecp->tx_full) {
        volatile ec_bd_t *bdp_next;

        /* handle the case that bdp->status own bit not cleared by hw but the interrupt still comes */
        if (bdp->control & TXBD_CTRL_TER)
            bdp_next = ecp->tx_bd_base;
        else
            bdp_next = bdp + 1;

        while (bdp_next != bdp) {
            /* when tx full, if we find that some bd(s) has own bit is 0, which
             * indicates that mac hw has transmitted it but the own bit left not cleared. */
            if (!(bdp_next->status & TXBD_STAT_OWN)) {
             printk(KERN_INFO "tx bd own bit not cleared!!!\n");

#ifdef TX_DEBUG
                print_mac_register(ecp);
                print_tx_bds(ecp);
#endif
                bdp->status &= ~TXBD_STAT_OWN; /* clear own bit */
                skb = ecp->tx_skb[ecp->skb_dirty];
                dma_unmap_single(&ecp->netdev->dev, bdp->buf_addr, skb->len, DMA_TO_DEVICE);
                dev_kfree_skb_any(skb);

                ecp->tx_skb[ecp->skb_dirty] = NULL;
                ecp->skb_dirty = (ecp->skb_dirty + 1) & TX_RING_MOD_MASK;

                if (bdp->control & TXBD_CTRL_TER)
                    bdp = ecp->tx_bd_base;
                else
                    bdp++;

                ecp->tx_full = false;
                if (netif_queue_stopped(dev))
                    netif_wake_queue(dev);
                break;
            }
            if (bdp_next->control & TXBD_CTRL_TER)
                bdp_next = ecp->tx_bd_base;
            else
                bdp_next++;
        }
    }
#endif
    ecp->dirty_tx = (ec_bd_t *) bdp;

    spin_unlock(&ecp->lock);
    return;
}


/**
 * ec_enet_rx -- sub-isr for rx interrupt
 */
static void subisr_enet_rx(ec_priv_t *ecp)
{
	struct net_device *dev = ecp->netdev;
	volatile ethregs_t *hw_regs = ecp->hwrp;
	volatile ec_bd_t *bdp;
	struct sk_buff *new_skb;
	struct sk_buff *skb_to_upper;
	unsigned long status;
	unsigned int  pkt_len;
	int	index;
//printk(KERN_INFO "%s %d\n",__FUNCTION__,__LINE__);
#define RX_ERROR_CARED \
	(RXBD_STAT_DE | RXBD_STAT_RF | RXBD_STAT_TL | RXBD_STAT_CS \
	| RXBD_STAT_DB | RXBD_STAT_CE | RXBD_STAT_ZERO)

	spin_lock(&ecp->lock);
	BUG_ON(!ecp->opened);
	bdp = ecp->cur_rx;
#ifdef RX_DEBUG
	if (unlikely(netif_msg_rx_err(ecp)))
		printk(KERN_INFO "bdp: 0x%p\n", bdp);
#endif

	while (0 == ((status = bdp->status) & RXBD_STAT_OWN)) {
		//printk(KERN_INFO "bdp->status:0x%08lx\n", bdp->status);
		if (!(status & RXBD_STAT_LS))
			printk(KERN_INFO "not last descriptor of a frame - status: 0x%08x.\n",
				(unsigned int)status);
#ifdef RX_DEBUG
		if (unlikely(netif_msg_rx_err(ecp))) {
			printk(KERN_INFO "bdp: 0x%p, bdp->status: 0x%08x\n", bdp, (u32)bdp->status);
			printk(KERN_INFO "status: 0x%08x\n", (u32)status);
		}
#endif

		/* check for rx errors. RXBD_STAT_ES includes RXBD_STAT_RE, and
		* RXBD_STAT_RE always set, coz RE pin of 5201 ether mac is NC to
		* 5302 ether phy. Don't care it now, it'll be fixed in 5203. */
		//if (status & (RXBD_STAT_ES | RXBD_STAT_DB))
		if (status & RX_ERROR_CARED) {
			printk("%d: RX_ERROR status:0x%08lx\n", __LINE__, status);
			dev->stats.rx_errors++;
			if (status & RXBD_STAT_TL)
				dev->stats.rx_length_errors++;
			if (status & RXBD_STAT_CE)
				dev->stats.rx_crc_errors++;
			if (status & (RXBD_STAT_RF | RXBD_STAT_DB))
				dev->stats.rx_frame_errors++;
			if (status & RXBD_STAT_ZERO)
				dev->stats.rx_fifo_errors++;
			if (status & RXBD_STAT_DE)
				dev->stats.rx_over_errors++;
			if (status & RXBD_STAT_CS)
				dev->stats.collisions++;
			goto rx_done;
		}

		pkt_len = RXBD_STAT_FL(status);
		if (pkt_len > ETH_PKG_MAX) { /* assure skb_put() not panic */
			printk(KERN_INFO "pkt_len = %u\n", pkt_len);
			dev->stats.rx_length_errors++;
			goto rx_done;
		}

		if (NULL == (new_skb = get_skb_aligned(PKG_MAX_LEN))) {
			dev->stats.rx_dropped++;
			printk(KERN_INFO "no memory, just drop it.\n"); // no release version ??
			goto rx_done;
		}

		dma_unmap_single(&ecp->netdev->dev, bdp->buf_addr, PKG_MAX_LEN, DMA_FROM_DEVICE);

		dev->stats.rx_packets++;
		dev->stats.rx_bytes += pkt_len;

		index = bdp - ecp->rx_bd_base;
		skb_to_upper = ecp->rx_skb[index];
		ecp->rx_skb[index] = new_skb;

		skb_put(skb_to_upper, pkt_len - ETH_CRC_LEN); /* modify its data length, remove CRC */
//#define RX_DEBUG
#ifdef RX_DEBUG
		if (unlikely(netif_msg_rx_err(ecp))) {
			check_icmp_sequence(skb_to_upper->data, __func__);
			printk(KERN_INFO "receive %u bytes\n", pkt_len);
			printk(KERN_INFO "source mac - ");
			print_mac_address(skb_to_upper->data + ETH_MAC_LEN);
			printk(KERN_INFO "dest mac - ");
			print_mac_address(skb_to_upper->data);
			printk(KERN_INFO "receive data:\n");
			print_frame_data(skb_to_upper->data, skb_to_upper->len);
		}
#endif
		skb_to_upper->protocol = eth_type_trans(skb_to_upper, dev);
		netif_rx(skb_to_upper);

		bdp->buf_addr = dma_map_single(&ecp->netdev->dev, new_skb->data, PKG_MAX_LEN, DMA_FROM_DEVICE);
		if (!bdp->buf_addr)
			printk(KERN_INFO "dma map new_skb->data:%p failed\n", new_skb->data);

rx_done:
		/* mark MAC AHB owns the buffer, and clear other status */
		bdp->status = RXBD_STAT_OWN;
#ifdef RX_DEBUG
		if (unlikely(netif_msg_rx_err(ecp)))
			printk(KERN_INFO "bdp->status: 0x%08x\n", (u32)bdp->status);
#endif
		if (bdp->control & RXBD_CTRL_RER)
			bdp = ecp->rx_bd_base;
		else
			bdp++;
#ifdef RX_DEBUG
		if (unlikely(netif_msg_rx_err(ecp)))
			printk(KERN_INFO "bdp: 0x%p\n", bdp);
#endif
		/* start to receive packets, may be good in heavily loaded net */
//		hw_regs->er_opmode |= EC_OPMODE_SR;  // maybe not need it here
	} /* while */
	
	ecp->cur_rx = (ec_bd_t *) bdp;
	spin_unlock(&ecp->lock);
	return;
}


static void phy_detect_func(struct work_struct *work)
{
    #define MII_TIME_OUT 50
	
    ec_priv_t *ecp = (ec_priv_t *)container_of(work, ec_priv_t, phy_detect_work);
    unsigned short phy_int_status = 0;
    unsigned short bmode = 0;
    unsigned long flags;

   /**for rtl8201***********/
   if (ecp->phy_model==ETH_PHY_MODEL_RTL8201)
   {
		phy_int_status = read_phy_reg(ecp, 0x1e);
		printk(KERN_INFO "phy_int_status: 0x%x\n", (unsigned)phy_int_status);
		bmode =read_phy_reg(ecp, 0x1);
		printk(KERN_INFO "bmode_status: 0x%x\n", (unsigned)bmode);
	}
	   if (ecp->phy_model==ETH_PHY_MODEL_SR8201G)
   {
		phy_int_status = read_phy_reg(ecp, 0x18);
//		printk(KERN_INFO "phy_int_status: 0x%x\n", (unsigned)phy_int_status);
//		printk(KERN_INFO "intterupt linkstatus chg : 0x%x\n", read_phy_reg(ecp, 14));
		bmode =read_phy_reg(ecp, 0x1);
//		printk(KERN_INFO "bmode_status: 0x%x\n", (unsigned)bmode);
	}

#if 1
	/* FIXME!used by KSZ8041, other phy may not need this */
	if (ecp->phy_model != ETH_PHY_MODEL_ATC2605)
		/* if SIRQ edge-triggered, pending bit must be cleared */
		putl(getl(INTC_EXTCTL) | (0x1 << 16), INTC_EXTCTL);
#endif
 

	   /**for rtl8201***********/
   if (ecp->phy_model==ETH_PHY_MODEL_RTL8201)
   {
		if((phy_int_status & (0x1<<11)) == 0)
		return IRQ_HANDLED;

	ecp->linked = (bmode & 0x1<<2) ? true : false;

    }
    if (ecp->phy_model==ETH_PHY_MODEL_SR8201G)
   {
	ecp->linked = (bmode & (0x1<<2)) ? true : false;
    }
    if(ecp->last_link_status!=ecp->linked){ 
	if (ecp->linked) {
		int i;
#ifdef DETECT_POWER_SAVE
		bool enable;

		stop_power_save_timer(ecp);
		spin_lock_irqsave(&ecp->lock, flags);
		enable = ecp->enable;
		spin_unlock_irqrestore(&ecp->lock, flags);
		if (!enable) {
			/* rarely occur, work ethernet_power_save() disabled phy! */
			ecp->linked = false;
			printk(KERN_INFO "ecp->enable is false!\n");
			goto out;
		}
#endif
//		printk(KERN_INFO "old_duplex = 0x%x\n", ecp->duplex);
//		printk(KERN_INFO "old_speed = 0x%x\n", ecp->speed);

		ecp->phy_ops->phy_read_status(ecp);
		set_mac_according_aneg(ecp);

//		printk(KERN_INFO "new_duplex = 0x%x\n", ecp->duplex);
//		printk(KERN_INFO "new_speed = 0x%x\n", ecp->speed);

		for (i = 0; i < MII_TIME_OUT; i++) {
			if (read_phy_reg(ecp, MII_BMSR) & BMSR_LSTATUS) {
				ecp->linked = true;
//				printk(KERN_INFO "link established.\n");
				break;
			}
		}
		if (MII_TIME_OUT == i) {
			ecp->linked = false;
//			printk(KERN_INFO "link fail.\n");
			goto out;
		}
	} else {
		ecp->linked = false;
	}
   } 
out:
   if(ecp->last_link_status!=ecp->linked){	
	if (ecp->linked) {
		netif_carrier_on(ecp->netdev);
		if (netif_queue_stopped(ecp->netdev))
			netif_wake_queue(ecp->netdev);
	} else {
#ifdef DETECT_POWER_SAVE
		start_power_save_timer(ecp, 1000);
#endif
		netif_carrier_off(ecp->netdev);
	}
	}
	//INFO_GREEN("link:%s\n", ecp->linked ? "linked" : "disconnected");	
	ecp->last_link_status=ecp->linked;	
      queue_delayed_work(phy_detect_queue, &ecp->phy_detect_work,msecs_to_jiffies(PHY_DETECT_DUTY));
}
/**
 * ec_netdev_isr -- interrupt service routine for ethernet controller
 */
static irqreturn_t ec_netphy_isr(int irq, void *cookie)
{
#define MII_TIME_OUT 100

	ec_priv_t *ecp = netdev_priv((struct net_device *) cookie);
	unsigned short phy_int_status = 0;
	unsigned short bmode = 0;
	unsigned long flags;

  #if 0
	if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
		phy_int_status = atc260x_reg_read(ecp->atc260x, atc2603_PHY_INT_STAT);
		/* clear phy int pending */
		atc260x_reg_write(ecp->atc260x, atc2603_PHY_INT_STAT, phy_int_status);
		printk(KERN_INFO "phy_int_status: 0x%04x\n", (unsigned)phy_int_status);
	} else {
   #endif //0
   /**for rtl8201***********/
   if (ecp->phy_model==ETH_PHY_MODEL_RTL8201)
   {
		phy_int_status = read_phy_reg(ecp, 0x1e);
		printk(KERN_INFO "phy_int_status: 0x%x\n", (unsigned)phy_int_status);
		bmode =read_phy_reg(ecp, 0x1);
		printk(KERN_INFO "bmode_status: 0x%x\n", (unsigned)bmode);
	}
	   if (ecp->phy_model==ETH_PHY_MODEL_SR8201G)
   {
		phy_int_status = read_phy_reg(ecp, 0x18);
//		printk(KERN_INFO "phy_int_status: 0x%x\n", (unsigned)phy_int_status);
//		printk(KERN_INFO "intterupt linkstatus chg : 0x%x\n", read_phy_reg(ecp, 14));
		bmode =read_phy_reg(ecp, 0x1);
//		printk(KERN_INFO "bmode_status: 0x%x\n", (unsigned)bmode);
	}
	 /************************/
#ifdef ETH_TEST_MODE
	if (ecp->test_mode) {
		printk(KERN_INFO "test mode: %u\n", ecp->test_mode);
		return IRQ_HANDLED;
	}
#endif

#if 1
	/* FIXME!used by KSZ8041, other phy may not need this */
	if (ecp->phy_model != ETH_PHY_MODEL_ATC2605)
		/* if SIRQ edge-triggered, pending bit must be cleared */
		putl(getl(INTC_EXTCTL) | (0x1 << 16), INTC_EXTCTL);
#endif
 

	   /**for rtl8201***********/
   if (ecp->phy_model==ETH_PHY_MODEL_RTL8201)
   {
		if((phy_int_status & (0x1<<11)) == 0)
		return IRQ_HANDLED;

	spin_lock_irqsave(&ecp->lock, flags);
	ecp->linked = (bmode & 0x1<<2) ? true : false;
	spin_unlock_irqrestore(&ecp->lock, flags);

	}
	   if (ecp->phy_model==ETH_PHY_MODEL_SR8201G)
   {
/*
		if((phy_int_status & (0x1<<0)) == 1)
		return IRQ_HANDLED;
*/
	spin_lock_irqsave(&ecp->lock, flags);
	ecp->linked = (bmode & 0x1<<2) ? true : false;
	spin_unlock_irqrestore(&ecp->lock, flags);

	}
	
	if (ecp->linked) {
		int i;
#ifdef DETECT_POWER_SAVE
		bool enable;

		stop_power_save_timer(ecp);
		spin_lock_irqsave(&ecp->lock, flags);
		enable = ecp->enable;
		spin_unlock_irqrestore(&ecp->lock, flags);
		if (!enable) {
			/* rarely occur, work ethernet_power_save() disabled phy! */
			ecp->linked = false;
			printk(KERN_INFO "ecp->enable is false!\n");
			goto out;
		}
#endif
//		printk(KERN_INFO "old_duplex = 0x%x\n", ecp->duplex);
//		printk(KERN_INFO "old_speed = 0x%x\n", ecp->speed);

		ecp->phy_ops->phy_read_status(ecp);
		set_mac_according_aneg(ecp);

//		printk(KERN_INFO "new_duplex = 0x%x\n", ecp->duplex);
//		printk(KERN_INFO "new_speed = 0x%x\n", ecp->speed);

		for (i = 0; i < MII_TIME_OUT; i++) {
			if (read_phy_reg(ecp, MII_BMSR) & BMSR_LSTATUS) {
				ecp->linked = true;
//				printk(KERN_INFO "link established.\n");
				break;
			}
			//udelay(1);
		}
		if (MII_TIME_OUT == i) {
			ecp->linked = false;
//			printk(KERN_INFO "link fail.\n");
			goto out;
		}
	} else {
		ecp->linked = false;
	}

out:
	if (ecp->linked) {
		netif_carrier_on(ecp->netdev);
		if (netif_queue_stopped(ecp->netdev))
			netif_wake_queue(ecp->netdev);
	} else {
#ifdef DETECT_POWER_SAVE
		start_power_save_timer(ecp, 1000);
#endif
		netif_carrier_off(ecp->netdev);
	}

	INFO_GREEN("link:%s\n", ecp->linked ? "linked" : "disconnected");
	//print_phy_register(ecp);
	//print_mac_register(ecp);

	return IRQ_HANDLED;
}

/**
 * ec_netdev_isr -- interrupt service routine for ethernet controller
 */
static irqreturn_t ec_netmac_isr(int irq, void *cookie)
{
    ec_priv_t *ecp = netdev_priv((struct net_device *) cookie);
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long status = 0;
    unsigned long intr_bits = 0;
    unsigned long flags = 0;    
	struct net_device *dev = ecp->netdev;
    static unsigned long tx_cnt,rx_cnt;
	unsigned long mac_status;
    int ru_cnt = 0;
     int i = 0;
    intr_bits = EC_STATUS_NIS | EC_STATUS_AIS;
//   	spin_lock(&ecp->lock);    
    disable_irq_nosync(ecp->mac_irq);
    /* xmit setup frame raise ETI, but not TI, this is only reason to pay attention to it */
//    interested = EC_STATUS_TI | EC_STATUS_RI | EC_STATUS_ETI | EC_STATUS_RU;
//	 printk(KERN_INFO "%s %d hw_regs->er_status=0x%x\n",__FUNCTION__,__LINE__,hw_regs->er_status);
    while ((status = hw_regs->er_status) & intr_bits) {
        hw_regs->er_status = status;    /* clear status */
        if (netif_msg_intr(ecp)) {
//            printk(KERN_INFO "interrupt status: 0x%8x\n", (int)status);
//            printk(KERN_INFO "status after clear: 0x%x\n", (int)hw_regs->er_status);
        }

        //if (status & EC_STATUS_TU)
        //    printk(KERN_INFO "---TU interrupt---\n");

        /* when set CSR0.TAP will induce endless loopback, get rid of uninteresting ones */
//        if (!(status & interested)) {
//            printk(KERN_INFO "NOT interested status: 0x%08x\n", (u32)status);
//            break;
//        }

        if (status & (EC_STATUS_TI | EC_STATUS_ETI)) {
            subisr_enet_tx(ecp);
			tx_cnt = 0;
			mac_status = status & EC_STATUS_RSM;
			if((mac_status == EC_RX_fetch_dsp)||(mac_status ==EC_RX_run_dsp)||(mac_status ==EC_RX_close_dsp) )
				rx_cnt++;
        }

        /* RI & RU may come at same time, if RI handled, then RU needn't handle.
         * If RU comes & RI not comes, then we must handle RU interrupt. */
        if (status & EC_STATUS_RI) {
		    //printk(KERN_INFO "%s %d  RX INTERRUPT\n",__FUNCTION__,__LINE__);
            subisr_enet_rx(ecp);
			rx_cnt = 0;
			mac_status = status & EC_STATUS_TSM;
			if((mac_status == EC_STATUS_TSM)||(mac_status == EC_TX_run_dsp))
				tx_cnt++;
        } else if (status & EC_STATUS_RU) {
            ru_cnt++;
            /* set RPD could help if rx suspended & bd available */
            if (ru_cnt == 2)
                hw_regs->er_rxpoll = 0x1;
			printk(KERN_INFO "---RU interrupt---, status: 0x%08x\n", (u32)status);
#ifdef RX_DEBUG
            printk(KERN_INFO "---while loops: %d, RU count: %d\n", i, ru_cnt);
            print_rx_bds(ecp);
            ecp->msg_enable |= 0x40;
#endif
            subisr_enet_rx(ecp);
#ifdef RX_DEBUG
            ecp->msg_enable &= ~0x40;
            print_rx_bds(ecp);
#endif
            /* guard against too many RU interrupts to avoid long time ISR handling */
            if (ru_cnt > 3)
                break;
        }
    }
    enable_irq(ecp->mac_irq);
    	 
	if((tx_cnt>10)||(rx_cnt>10)){
		if(tx_cnt>10)
			printk(KERN_INFO "TX ERROR status: 0x%08x\n", (u32)status);
		else{
			printk(KERN_INFO "RX ERROR status: 0x%08x\n", (u32)status);
			ecp->rx_timeout=true;
		}	
		rx_cnt = 0;
		tx_cnt = 0;
		netif_stop_queue(dev);
		schedule_work(&ecp->hardware_reset_work);
	}
	
    return (IRQ_HANDLED);
}

/* phy int pin connected to sirq0 */
static void phy_cable_plug_irq_enable(int level)
{
    /* sirq0: clear pending */
    putl((getl(INTC_EXTCTL) | (0x1 << 16)) & ~0x0101, INTC_EXTCTL);
    printk(KERN_INFO "INTC_EXTCTL: 0x%08x\n", getl(INTC_EXTCTL));

    if (!level) { /* low level active, pull up */
        putl((getl(PAD_PULLCTL0) & 0xffff7fff) | 0x00004000, PAD_PULLCTL0);
        putl((getl(INTC_EXTCTL) | (0x3 << 21)) & ~(0x1 << 23), INTC_EXTCTL);
    } else { /* high level active, pull down*/
        putl((getl(PAD_PULLCTL0) & 0xffffbfff) | 0x00008000, PAD_PULLCTL0);
        putl(getl(INTC_EXTCTL) | (0x1 << 21), INTC_EXTCTL);
    }
    printk(KERN_INFO "PAD_PULLCTL0: 0x%08x\n", getl(PAD_PULLCTL0));
    printk(KERN_INFO "INTC_EXTCTL: 0x%08x\n", getl(INTC_EXTCTL));
}

static void phy_cable_plug_irq_disable(void)
{
    putl(getl(INTC_EXTCTL) & ~(0x1 << 21) & ~0x0101, INTC_EXTCTL);
    putl(getl(PAD_PULLCTL0) & ~(0x3 << 14), PAD_PULLCTL0);
    printk(KERN_INFO "PAD_PULLCTL0: 0x%08x\n", getl(PAD_PULLCTL0));
    printk(KERN_INFO "INTC_EXTCTL: 0x%08x\n", getl(INTC_EXTCTL));
}

/**
 * ec_netdev_open -- open ethernet controller
 *
 * return 0 if success, negative value if fail
 */
static int ec_netdev_open(struct net_device *dev)
{
    int ret = 0;
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long flags = 0;

	
	
    if (ecp->opened) {
        printk(KERN_INFO "already opened\n");
        return (0);
    }
#ifdef ETH_TEST_MODE
    ecp->test_mode = 0;
#endif

    if (prepare_rx_bd(ecp) || prepare_tx_bd(ecp)) {
        printk(KERN_INFO "error: NO memery for bds.\n");
        return (-1);
    }

    	if (_init_hardware(ecp,1,1)) {
        printk(KERN_INFO "error: harware initialization failed.\n");
        return (-1);
    }

    /* should after set_mac_addr() which will set ec_priv_t{.mac_addr} */
    memcpy(dev->dev_addr, ecp->mac_addr, ETH_MAC_LEN);

    spin_lock_irqsave(&ecp->lock, flags);

    /* send out a mac setup frame */
    if (transmit_setup_frame(ecp)) {
        printk(KERN_INFO "error : transmit setup frame failed.\n");
        spin_unlock_irqrestore(&ecp->lock, flags);
        return (-1);
    }
    
    /* start to tx & rx packets */
    hw_regs->er_ienable = EC_IEN_ALL;
    hw_regs->er_opmode |= EC_OPMODE_ST | EC_OPMODE_SR;

#ifdef DETECT_POWER_SAVE
    ecp->enable = true;
#endif
    ecp->opened = true;

    /* init  mii_if_info struct */
    ecp->mii_info.phy_id_mask = PHY_ADDR_MASK;
    ecp->mii_info.reg_num_mask = PHY_REG_NUM_MASK;
    ecp->mii_info.dev = dev;
    ecp->mii_info.mdio_read = ec_mdio_read;
    ecp->mii_info.mdio_write = ec_mdio_write;
    ecp->mii_info.phy_id = ecp->phy_addr;
    ecp->mii_info.force_media = 0;
    ecp->mii_info.full_duplex = 1;
    ecp->mii_info.supports_gmii = 0;
    ecp->msg_enable = NET_MSG_ENABLE;

    spin_unlock_irqrestore(&ecp->lock, flags);

    if (ecp->linked) 
        netif_carrier_on(dev);
    else 
        netif_carrier_off(dev);

    printk(KERN_INFO "%s link %s.\n", dev->name, ecp->linked ? "on" : "off");

#if 1
    /* FIXME!used by KSZ8041, other phy may not need this */
    if (ecp->phy_model != ETH_PHY_MODEL_ATC2605)
        phy_cable_plug_irq_enable(0);/* enable high level active */
#endif
//rc=  request_irq(client->irq, gsl_ts_irq, IRQF_TRIGGER_RISING | IRQF_DISABLED, client->name, ts);

#ifdef PHY_USE_POLL
    queue_delayed_work(phy_detect_queue, &ecp->phy_detect_work,msecs_to_jiffies(PHY_DETECT_DUTY));
        printk(KERN_INFO "phy detect by work queue\n");	
#else
ret = request_irq(dev->irq,  ec_netphy_isr,
                IRQF_TRIGGER_FALLING , "ethernet_phy", dev);
    if (ret < 0) {
        printk(KERN_INFO "Unable to request IRQ: %d, ec_netphy_isr\n", ret);
        return ret;
    }
    printk(KERN_INFO "IRQ %d requested, dev: %p\n", dev->irq, dev);
#endif	

    ret = request_irq(OWL_IRQ_ETHERNET, (irq_handler_t)ec_netmac_isr,
                0, "ethernet_mac", dev);
    if (ret < 0) {
        printk(KERN_INFO "Unable to request IRQ: %d, ec_netmac_isr\n", ret);
        goto err_irq;
    }
    printk(KERN_INFO "IRQ %d requested\n", OWL_IRQ_ETHERNET);

    print_mac_register(ecp);

#ifdef DETECT_POWER_SAVE
    init_power_save_timer(ecp);
    start_power_save_timer(ecp, 4000);
#endif

    netif_start_queue(dev);
    return (0);

err_irq:
#ifndef PHY_USE_POLL
    free_irq(dev->irq, dev);
#endif
    return ret;
}


/**
 * ec_netdev_close -- close ethernet controller
 *
 * return 0 if success, negative value if fail
 */
static int ec_netdev_close(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
    //struct atc260x_dev *atc260x = ecp->atc260x;
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long flags = 0;
    //unsigned short atc260x_temp;

    printk(KERN_INFO "\n");
    if (!ecp->opened) {
        printk(KERN_INFO "already closed\n");
        return (0);
    }

#ifdef DETECT_POWER_SAVE
    stop_power_save_timer(ecp);
#endif

#if 1
    /* FIXME!used by KSZ8041, other phy may not need this */
    if (ecp->phy_model != ETH_PHY_MODEL_ATC2605)
        phy_cable_plug_irq_disable();
#endif

	_deinit_hardware(ecp);

    netif_stop_queue(dev);

    spin_lock_irqsave(&ecp->lock, flags);
#ifdef DETECT_POWER_SAVE
    ecp->enable = false;
#endif
    ecp->opened = false;
    ecp->linked = false;	
    hw_regs->er_opmode &= ~(EC_OPMODE_ST | EC_OPMODE_SR);

    hw_regs->er_ienable = 0;

    spin_unlock_irqrestore(&ecp->lock, flags);

    free_irq(OWL_IRQ_ETHERNET, dev);

#ifndef PHY_USE_POLL
    free_irq(dev->irq, dev);
#endif
    if (ecp->phy_model != ETH_PHY_MODEL_ATC2605)
        goto phy_not_atc2605;
   #if 0
    /* turn off both link status & speed LEDs after ethernet down */
    atc260x_reg_write(atc260x, atc2603_PHY_HW_RST, 0x01);  //reset the phy
    udelay(100);  //! not work if removed
    do {
        atc260x_temp = atc260x_reg_read(atc260x, atc2603_PHY_HW_RST);
    }while(atc260x_temp & 0x1); //waite for reset process completion

    //need power down phy and mac
    /*ethernet wrapper rest and phy has no power*/
    atc260x_reg_setbits(atc260x, atc2603_CMU_DEVRST, 0x08, 0x08);
    udelay(100);
    atc260x_reg_setbits(atc260x, atc2603_CMU_DEVRST, 0x08, 0x0);
    udelay(100);
    atc260x_reg_setbits(atc260x, atc2603_CMU_DEVRST, 0x200, 0x0);
    udelay(100);
    #endif //0
phy_not_atc2605:
    ethernet_clock_disable();
    return (0);
}


/**
 * ec_netdev_start_xmit -- transmit a skb
 *
 * NOTE: if CSR6.ST is not set, the xmit frame will fail
 *
 * return NETDEV_TX_OK if success, others if fail
 */
static int ec_netdev_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = ecp->hwrp;
    volatile ec_bd_t *bdp;
    unsigned long flags = 0;

#ifndef ETH_TEST_MODE
    if (!ecp->linked) {
        printk(KERN_INFO "haven't setup linkage\n");
        return (-2);
    }
#endif

    if (ecp->tx_full) {
		if(printk_ratelimit())
        	printk(KERN_INFO "warnig : tx buffer list is full\n");
        return (NETDEV_TX_BUSY);
    }

    spin_lock_irqsave(&ecp->lock, flags);
    bdp = ecp->cur_tx;

    if (bdp->status & TXBD_STAT_OWN) {
        printk(KERN_INFO "%s: tx is full. should not happen\n", dev->name);
        spin_unlock_irqrestore(&ecp->lock, flags);
        return (NETDEV_TX_BUSY);
    }

    /* Push the data cache so the NIC does not get stale memory data. */
    bdp->buf_addr = dma_map_single(&ecp->netdev->dev, skb->data, skb->len, DMA_TO_DEVICE);
    if (!bdp->buf_addr)
        printk(KERN_INFO "dma map skb->data:%p failed\n", skb->data);

    bdp->status = 0;
    bdp->control &= TXBD_CTRL_IC | TXBD_CTRL_TER; /* clear others */
    bdp->control |= TXBD_CTRL_TBS1(skb->len);
    bdp->control |= TXBD_CTRL_FS | TXBD_CTRL_LS;
    mb();
    bdp->status = TXBD_STAT_OWN;
    mb();
    {
        volatile u32 tmp;
        tmp = (u32)(bdp->status + bdp->control + bdp->buf_addr);
    }
	if(skb->len > 1518)
		printk(KERN_INFO "tx length:%d\n", skb->len);
    //printk(KERN_INFO "bdp->status: 0x%x\n", bdp->status);

    //!for FPGA test, or else tx transmission gonna disorder
    //udelay(1);

    /* 
      * leave xmit suspended mode to xmit running mode; another method is that first 
      * stop xmit, then start xmit through clearing CSR6.13-ST then setting it again.
      * before call this function, CSR6.13-ST should be set
      */
    hw_regs->er_txpoll = EC_TXPOLL_ST;

    dev->stats.tx_bytes += skb->len; 
    dev->trans_start = jiffies;

    ecp->tx_skb[ecp->skb_cur] = skb;
    ecp->skb_cur = (ecp->skb_cur + 1) & TX_RING_MOD_MASK;

    if (!(hw_regs->er_status & EC_STATUS_TSM))
        printk(KERN_INFO "tx stopped, hw_regs->er_status:0x%lx\n", hw_regs->er_status);

    if (netif_msg_tx_err(ecp)) {
        check_icmp_sequence(skb->data, __func__);
        printk(KERN_INFO "%d, hw_regs->er_status:0x%lx\n", __LINE__, hw_regs->er_status);
        printk(KERN_INFO "bdp->status:0x%x\n", (unsigned int)bdp->status);
        printk(KERN_INFO "bdp->control:0x%x\n", (unsigned int)bdp->control);
        printk(KERN_INFO "bdp->buf_addr:0x%x\n", (unsigned int)bdp->buf_addr);
        printk(KERN_INFO "tx frame len:0x%x, skb->data:0x%x\n", skb->len, (int)skb->data);
        printk(KERN_INFO "tx src mac: ");
        print_mac_address(skb->data + ETH_MAC_LEN);
        printk(KERN_INFO "tx dst mac: ");
        print_mac_address(skb->data);
        printk(KERN_INFO "tx frame:\n");
        print_frame_data(skb->data, skb->len);
    }

    if (bdp->control & TXBD_CTRL_TER)
        bdp = ecp->tx_bd_base;
    else
        bdp++;

    if (bdp == ecp->dirty_tx) {
        printk(KERN_INFO "tx is full, skb_dirty:%d\n", ecp->skb_dirty);
        ecp->tx_full = true;
        netif_stop_queue(dev);
    }
    netif_stop_queue(dev);

    ecp->cur_tx = (ec_bd_t *)bdp;
    spin_unlock_irqrestore(&ecp->lock, flags);

    /*---------------debug -------------------*/
#if 0
    {
        static timeout_reset_cnt = 0;
        timeout_reset_cnt++;
        if(timeout_reset_cnt >100000) {
            timeout_reset_cnt =0;
            ec_netdev_transmit_timeout(ecp->netdev);
        }
    }
#endif

    return (NETDEV_TX_OK);
}


/**
 * ec_netdev_query_stats -- query statistics of ethernet controller
 */
static struct net_device_stats *ec_netdev_query_stats(struct net_device *dev)
{
    return (&dev->stats);
}


/**
 * ec_netdev_set_mac_addr -- set mac a new address
 *
 * NOTE: when ethernet device has opened, can't change mac address, otherwise return 
 * EBUSY error code.
 *
 * return 0 if success, others if fail
 */
static int ec_netdev_set_mac_addr(struct net_device *dev, void *addr)
{
    struct sockaddr *address = (struct sockaddr *) addr;
    ec_priv_t *ecp = netdev_priv(dev); 
    unsigned long flags = 0;
    char    old_mac_addr[ETH_MAC_LEN];
    bool old_overrided;

    printk(KERN_INFO "\n");

    if (!is_valid_ether_addr(address->sa_data)) {
        printk(KERN_INFO "not valid mac address\n");
        return (-EADDRNOTAVAIL);
    }

    /* if new mac address is the same as the old one, nothing to do */
    if (!compare_ether_addr(ecp->mac_addr, address->sa_data)) {
        printk(KERN_INFO "the same address ! \n");
        return (0);
    }

    if (netif_running(dev)) {
        printk(KERN_INFO "error : queue is busy\n");
        return (-EBUSY);
    }

    spin_lock_irqsave(&ecp->lock, flags);

    memcpy(old_mac_addr, ecp->mac_addr, ETH_MAC_LEN);
    old_overrided = ecp->mac_overrided;

    memcpy(ecp->overrided_mac, address->sa_data, dev->addr_len);

    if (compare_ether_addr(g_default_mac_addr, ecp->overrided_mac)) {
        ecp->mac_addr = ecp->overrided_mac;
        ecp->mac_overrided = true;
    } else {
        ecp->mac_addr = g_default_mac_addr;
        ecp->mac_overrided = false;
    }

    memcpy(dev->dev_addr, ecp->mac_addr, dev->addr_len);

    /* if netdev is close now, just save new addr tmp, set it when open it */
    if (!ecp->opened) {
        spin_unlock_irqrestore(&ecp->lock, flags);
        return (0);
    }

    fill_macaddr_regs(ecp, ecp->mac_addr);  /* for flow control */

    /*
     * errors only occur before frame is put into tx bds. in fact if frame is successfully built, 
     * xmit never fail, otherwise mac controller may be something wrong.
     */
    if (transmit_setup_frame(ecp)) {
        printk(KERN_INFO "error : transmit setup frame failed\n");

        /* set back to old one */
        fill_macaddr_regs(ecp, old_mac_addr);
        
        if (old_overrided) {
            memcpy(ecp->overrided_mac, old_mac_addr, ETH_MAC_LEN);
            ecp->mac_addr = ecp->overrided_mac;
        } else {
            ecp->mac_addr = g_default_mac_addr;
        }
        ecp->mac_overrided = old_overrided;
        memcpy(dev->dev_addr, ecp->mac_addr, dev->addr_len);
        spin_unlock_irqrestore(&ecp->lock, flags);
        return (-1);
    }

    spin_unlock_irqrestore(&ecp->lock, flags);

    return (0);
}


/**
 * copy_multicast_list -- copy @list to local ec_priv_t{.multicast_list}
 * may use it if multicast is supported when building setup-frame 
 */
static inline void copy_multicast_list(ec_priv_t *ecp, struct netdev_hw_addr_list *list)
{
    char (*mmac_list)[MULTICAST_LIST_LEN][ETH_MAC_LEN] = NULL;
    struct netdev_hw_addr *ha = NULL;
    int mmac_sum = 0;

    mmac_list = &ecp->multicast_list.mac_array;
#if 0
    while (list && mmac_sum < MULTICAST_LIST_LEN) {
        if (!is_multicast_ether_addr(list->dmi_addr)) {
            //printk(KERN_INFO "there is one non-multicast addr\n");
            continue;
        }
        
#if EC_TRACED
        printk(KERN_INFO "ok, add one : ");
        print_mac_address(list->dmi_addr);
#endif

        memcpy((*mmac_list)[mmac_sum], list->dmi_addr, ETH_MAC_LEN);
        list = list->next;
        mmac_sum++;
    }
#endif

    netdev_hw_addr_list_for_each(ha, list) {
        if (mmac_sum >= MULTICAST_LIST_LEN) {
            break;
        }
        if (!is_multicast_ether_addr(ha->addr)) {
            //printk(KERN_INFO "there is one non-multicast addr\n");
            continue;
        }
        memcpy((*mmac_list)[mmac_sum], ha->addr, ETH_MAC_LEN);
        mmac_sum++;
    }

    ecp->multicast_list.count = mmac_sum; 

    return;
}


static inline void parse_interface_flags(ec_priv_t *ecp, unsigned int flags)
{
    printk(KERN_INFO "\n");
    set_mode_promisc(ecp, (bool)(flags & IFF_PROMISC));
    set_mode_all_multicast(ecp, (bool)(flags & IFF_ALLMULTI));
    ecp->multicast = (bool)(flags & IFF_MULTICAST);
    return;
}

/**
 * ec_netdev_set_multicast_list -- set mac multicast address, meanwhile set promiscuous
 * and all_multicast mode according to dev's flags
 */
static void ec_netdev_set_multicast_list(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long flags = 0;
    unsigned long old_srst_bits;

#if EC_TRACED
    printk(KERN_INFO "--- enter %s()\n", __func__);
    printk(KERN_INFO "dev->flags - 0x%x\n", dev->flags);
#endif

    spin_lock_irqsave(&ecp->lock, flags);

    old_srst_bits = hw_regs->er_opmode & (EC_OPMODE_SR | EC_OPMODE_ST);

    /* stop tx & rx first */
    hw_regs->er_opmode &= ~(EC_OPMODE_SR | EC_OPMODE_ST);

    parse_interface_flags(ecp, dev->flags);

    if (!ecp->all_multicast && ecp->multicast && dev->mc.count) {
        if (dev->mc.count <= MULTICAST_LIST_LEN) {
	        copy_multicast_list(ecp, &dev->mc);
            transmit_setup_frame(ecp);
        } else {
            printk(KERN_INFO "too multicast addrs to support, open all_multi\n");
            /* list is too long to support, so receive all multicast packets */
            set_mode_all_multicast(ecp, true);
        }
    }
    printk(KERN_INFO "\n");

    hw_regs->er_opmode |= old_srst_bits;
 
    spin_unlock_irqrestore(&ecp->lock, flags);

    return;

}

static inline void modify_mii_info(struct mii_ioctl_data *data, struct mii_if_info *info)
{
    unsigned short val = data->val_in;

    if (data->phy_id != info->phy_id)
        return;

    switch (data->reg_num) {
    case MII_BMCR:
    {
        info->force_media = (val & (BMCR_RESET | BMCR_ANENABLE)) ? 0 : 1;

        if (info->force_media && (val & BMCR_FULLDPLX))
            info->full_duplex = 1;
        else
            info->full_duplex = 0;
        break;
    }
    case MII_ADVERTISE:
        info->advertising = val;
        break;
    default:
        /* nothing to do */
        break;
    }

    return;
}


/**
 * ec_netdev_ioctrl -- net device's ioctrl hook
 */
static int ec_netdev_ioctrl(struct net_device *dev, struct ifreq *ifr, int cmd)
{    
    ec_priv_t *ecp = netdev_priv(dev);
    struct mii_ioctl_data *data = if_mii(ifr);
    struct mii_if_info *info = &ecp->mii_info;
    int     err = 0;

#if EC_TRACED
    printk(KERN_INFO "--- enter %s()\n", __func__);
    printk(KERN_INFO "phy reg num - 0x%x, data - 0x%x, cmd:0x%x\n", data->reg_num, data->val_in, cmd);
#endif

    data->phy_id &= info->phy_id_mask;
    data->reg_num &= info->reg_num_mask;

    switch (cmd) {
    case SIOCGMIIPHY:
        data->phy_id = info->phy_id;
        /* FALL THROUGH */
    case SIOCGMIIREG:
        //data->val_out = read_phy_reg(ecp, info->phy_id, data->reg_num);
        data->val_out = read_phy_reg(ecp, 0x500 + data->reg_num);
        break;

    case SIOCSMIIREG:
    {
        unsigned short val = data->val_in;
        //unsigned short old_val;

        if (!capable(CAP_NET_ADMIN)) {
            return (-EPERM);
        }

        modify_mii_info(data, info);
        //printk(KERN_INFO "val : 0x%x\n", val);

        /* when keep speed unchanged, but change duplex, CSR5.LCIQ will not raise
         *  interrupt. if so ISR can't capture duplex status change.
         * here we change speed first and then set it back to raise a LCIQ interrupt.
         * has other better way???
         */
//            if (unlikely(info->force_media && MII_BMCR == data->reg_num)) {
//                //old_val = read_phy_reg(ecp, info->phy_id, data->reg_num);
//                old_val = read_phy_reg(ecp, data->reg_num);
//                if (need_change_speed(val, old_val)) {
//                    //printk(KERN_INFO "old bmcr: 0x%x\n", old_val);
//                    old_val &= ~(BMCR_ANENABLE | BMCR_RESET);
//                    old_val ^= BMCR_SPEED100;
//                    //printk(KERN_INFO "exchanged old bmcr: 0x%x\n", old_val);
//                    //write_phy_reg(ecp, info->phy_id, data->reg_num, old_val);
//                    write_phy_reg(ecp, data->reg_num, old_val);
//                }
//            }

        //write_phy_reg(ecp, info->phy_id, data->reg_num, val);
        write_phy_reg(ecp, 0x500 + data->reg_num, val);
        break;
    }  /* end SIOCSMIIREG */

    default:
        err = -EOPNOTSUPP;
        break;
    }

    return (err);

    //return generic_mii_ioctl(&ecp->mii_info, data, cmd, NULL);
}

/**
 * do hardware reset,this function is called when transmission timeout.
*/
static void hardware_reset_do_work(struct work_struct *work)
{
	struct net_device *dev =  g_eth_dev[0];
	ec_priv_t *ecp = netdev_priv(dev);//(ec_priv_t *)container_of(work, ec_priv_t, hardware_reset_work);//
	volatile ethregs_t *hw_regs = ecp->hwrp;
  bool phy_reinit =false;
	//spin_lock_irqsave(&ecp->lock, flags);

//    netif_carrier_off(dev);

    hw_regs->er_opmode &= ~(EC_OPMODE_SR | EC_OPMODE_ST);       // stop tx and rx
    hw_regs->er_ienable = 0;

    disable_irq(ecp->mac_irq);
#ifdef PHY_USE_POLL
    cancel_delayed_work(&ecp->phy_detect_work);
    flush_workqueue(phy_detect_queue);
#else
    disable_irq(dev->irq);
#endif
		//if tx timeout 只复位mac，不复位phy, 否则都做reset
#if 0   //timeout 总是不reset phy
    if(ecp->rx_timeout){
    	  phy_reinit=true;
				ecp->rx_timeout=false;    			
				write_phy_reg(ecp, 0x0, 0x1<<15);
				mdelay(5);
				_deinit_hardware(ecp);
				mdelay(30);
		}		
#endif
		
    ethernet_clock_disable();

    /* set default value of status */
//    ecp->linked = false;
//    ecp->opened = false;
//    ecp->speed = ETH_SPEED_100M;
//    ecp->duplex = ETH_DUP_FULL;
    //spin_unlock_irqrestore(&ecp->lock, flags);
	
    if (prepare_rx_bd(ecp) || prepare_tx_bd(ecp)) {
        INFO_RED("error : prepare bds failed\n");
        return;
    }
    if(phy_reinit){
    	phy_reinit=false;
    	if (_init_hardware(ecp,1,1)) {
      	  INFO_RED("error : harware init failed.\n");
        	return;
    	}
    }else{
     	if (_init_hardware(ecp,1,0)) {
      	  INFO_RED("error : harware init failed.\n");
        	return;
    	}    
    }		

    memcpy(dev->dev_addr, ecp->mac_addr, ETH_MAC_LEN);

    parse_interface_flags(ecp, dev->flags);

    if (transmit_setup_frame(ecp)) {  // recovery previous macs
        INFO_RED("error : xmit setup frame failed\n");
        return;
    }

    hw_regs->er_ienable = EC_IEN_ALL;
    hw_regs->er_opmode |= (EC_OPMODE_SR | EC_OPMODE_ST);        /* start tx, rx */

//#ifdef DETECT_POWER_SAVE
//    ecp->enable = true;
//      start_power_save_timer(ecp, 4000);
//#endif
//    ecp->opened = true;

#ifdef PHY_USE_POLL
    queue_delayed_work(phy_detect_queue, &ecp->phy_detect_work,msecs_to_jiffies(PHY_DETECT_DUTY));
#else
    enable_irq(dev->irq);
#endif
    enable_irq(ecp->mac_irq);
	return;
	
}
/**
 * ec_netdev_transmit_timeout -- process tx fails to complete within a period.
 *
 * This function is called when a packet transmission fails to complete within a  
 * resonable period, on the assumption that an interrupts have been failed or the 
 * interface is locked up. This function will reinitialize the hardware 
 */
static void ec_netdev_transmit_timeout(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
	static int reset = 0;
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long flags = 0;    
#if 0
    //unsigned long flags = 0;
    /* times of timeout handled by subisr_enet_tx() */
//    static int times = 0;
    /* times of hardware reset, (times * reset) is the overall times of timeout */
    volatile ec_bd_t *bdp;

#define TX_TIMEOUT_TIMES_2_RESET 1
//    printk(KERN_INFO "before tx isr, reg status : 0x%x\n", (int)hw_regs->er_status);
    print_mac_register(ecp);
    print_tx_bds(ecp);


    /* handle situation that no TX interrupt comes & TX process is dead */
    if (times < TX_TIMEOUT_TIMES_2_RESET) {
        hw_regs->er_ienable &= ~(EC_IEN_TIE |EC_IEN_ETE);

        bdp = ecp->dirty_tx;
        if (bdp->status & TXBD_STAT_OWN) {
            times++;
        } else {
            //subisr_enet_tx(ecp);
//            printk(KERN_INFO "---TX timeout times: %d\n", times);
            times = 0;
        }
        hw_regs->er_ienable |= EC_IEN_TIE | EC_IEN_ETE;
#if 0
        printk(VT_GREEN "after tx isr, reg status : 0x%x\n" VT_NORMAL, (int)hw_regs->er_status);
        print_mac_register(ecp);
        print_tx_bds(ecp);
#endif
        /* if tx still time out, then reset hardware */
        return;
    }
    times = 0;

    printk(KERN_INFO "before tx isr, reg status : 0x%x\n", (int)hw_regs->er_status);
    print_mac_register(ecp);
    print_tx_bds(ecp);
#endif
	netif_stop_queue(dev);
    reset++;
#if 0    
    printk(KERN_INFO "---TX timeout reset: %d\n", reset);
	printk(KERN_INFO "MAC flowctrl : 0x%x\n", (int)hw_regs->er_flowctrl);
	printk(KERN_INFO "MAC reg status : 0x%x\n", (int)hw_regs->er_status);
#endif	  
	schedule_work(&ecp->hardware_reset_work);
#if 0
    spin_lock_irqsave(&ecp->lock, flags);

    netif_stop_queue(dev);
    netif_carrier_off(dev);

    hw_regs->er_opmode &= ~(EC_OPMODE_SR | EC_OPMODE_ST);       // stop tx and rx
    hw_regs->er_ienable = 0;

    disable_irq(ecp->mac_irq);
    disable_irq(dev->irq);
    ethernet_clock_disable();

    /* set default value of status */
    ecp->linked = false;
    ecp->opened = false;
    ecp->speed = ETH_SPEED_100M;
    ecp->duplex = ETH_DUP_FULL;
    spin_unlock_irqrestore(&ecp->lock, flags);
		

    if (prepare_rx_bd(ecp) || prepare_tx_bd(ecp)) {
        INFO_RED("error : prepare bds failed\n");
        return;
    }

    if (_init_hardware(ecp,1)) {
        INFO_RED("error : harware init failed.\n");
        return;
    }

    memcpy(dev->dev_addr, ecp->mac_addr, ETH_MAC_LEN);

    parse_interface_flags(ecp, dev->flags);

    if (transmit_setup_frame(ecp)) {  // recovery previous macs
        INFO_RED("error : xmit setup frame failed\n");
        return;
    }

    hw_regs->er_ienable = EC_IEN_ALL;
    hw_regs->er_opmode |= (EC_OPMODE_SR | EC_OPMODE_ST);        /* start tx, rx */

#ifdef DETECT_POWER_SAVE
    ecp->enable = true;
    start_power_save_timer(ecp, 4000);
#endif
    ecp->opened = true;

    enable_irq(dev->irq);
    enable_irq(ecp->mac_irq);

    /* we need make sure all right before arrive here. */
    //netif_wake_queue(dev);
#endif
    return;
}


/*
 * Internal function. Flush all scheduled work from the Ethernet work queue.
 */
static void ethernet_flush_scheduled_work(void)
{
	flush_workqueue(resume_queue);
}


static int asoc_ethernet_suspend(struct platform_device *pdev, pm_message_t m)
{
    struct net_device *dev =  g_eth_dev[0];
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long flags = 0;

    dev_info(&pdev->dev, "asoc_ethernet_suspend()\n");

    suspend_flag = 1;

    disable_irq(ecp->mac_irq);
    
	  _deinit_hardware(ecp);
	  mdelay(5);  
#ifdef PHY_USE_POLL 	
    cancel_delayed_work_sync(&ecp->phy_detect_work);
//    flush_workqueue(phy_detect_queue);
#else
    disable_irq(dev->irq);
#endif

    ecp->linked = false;
            
    cancel_delayed_work_sync(&resume_work);
    ethernet_flush_scheduled_work();

    netif_stop_queue(dev);
    netif_carrier_off(dev);

    spin_lock_irqsave(&ecp->lock, flags);
    
    hw_regs->er_opmode &= ~(EC_OPMODE_SR | EC_OPMODE_ST);  // stop tx and rx
    hw_regs->er_ienable = 0;

    free_rxtx_skbs(ecp->rx_skb, RX_RING_SIZE);
    free_rxtx_skbs(ecp->tx_skb, TX_RING_SIZE);

    spin_unlock_irqrestore(&ecp->lock, flags);



    ethernet_clock_disable();

    return 0;
}


static int ethernet_parse_gpio(struct device_node * of_node, const char * propname, struct owl_ethernet_gpio * gpio)
{
    enum of_gpio_flags gflags;
    int	gpio_num;

    gpio_num = of_get_named_gpio_flags(of_node, propname, 0, &gflags);
    if (gpio_num >= 0) {
        gpio->gpio = gpio_num;
    } else {
        gpio->gpio = -1;
	 return -1;		
    }

    gpio->active= (gflags&OF_GPIO_ACTIVE_LOW) ;

    return 0;
}


static void ethernet_resume_handler(struct work_struct *work)
{
    struct net_device *dev =  g_eth_dev[0];

    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = ecp->hwrp;
    unsigned long flags = 0;
 
    printk(KERN_INFO "\n");
    suspend_flag = 0;
 #if 0
    if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
        /*For gl5302 INT Pending unusual */
        ethernet_clock_enable();
        ecp->phy_ops->phy_hw_init(ecp);

        /*ethernet wrapper rest and phy has no power*/
        atc260x_reg_setbits(ecp->atc260x, atc2603_CMU_DEVRST, 0x08, 0x08);
        udelay(100);
        atc260x_reg_setbits(ecp->atc260x, atc2603_CMU_DEVRST, 0x08, 0x0);
        udelay(100);
  #endif //0
       // ethernet_clock_disable();
    

    if (ecp->opened == false) {
        printk(KERN_INFO "The ethernet is turned off\n");
        return;
    }
   
    ecp->last_link_status=ecp->linked = false;
    			
    spin_lock_irqsave(&ecp->lock, flags);
    if (prepare_rx_bd(ecp) || prepare_tx_bd(ecp)) {
        printk(KERN_INFO "error: prepare bds failed\n");
        goto err_unlock;
    }
    spin_unlock_irqrestore(&ecp->lock, flags);

    /*can't sleep */

    if (_init_hardware(ecp,0,1)) {
        printk(KERN_INFO "error: harware init failed.\n");
        return;
    }

    spin_lock_irqsave(&ecp->lock, flags);

    memcpy(dev->dev_addr, ecp->mac_addr, ETH_MAC_LEN);

    parse_interface_flags(ecp, dev->flags);

    /* send out a mac setup frame */
    if (transmit_setup_frame(ecp)) {  // recovery previous macs
        printk(KERN_INFO "error: xmit setup frame failed\n");
        goto err_unlock;
    }

    hw_regs->er_ienable = EC_IEN_ALL;
    hw_regs->er_opmode |= (EC_OPMODE_SR | EC_OPMODE_ST);
    spin_unlock_irqrestore(&ecp->lock, flags);
#ifndef PHY_USE_POLL 
    enable_irq(dev->irq);
#endif
    enable_irq(ecp->mac_irq);
#ifdef DETECT_POWER_SAVE
    ecp->enable = true;
    start_power_save_timer(ecp, 4000);
#endif

#ifdef PHY_USE_POLL
     queue_delayed_work(phy_detect_queue, &ecp->phy_detect_work,msecs_to_jiffies(PHY_DETECT_RESUME));		//必须等待resume后才能做detect
     printk(KERN_INFO "phy detect by work queue\n");	
#endif
    //netif_wake_queue(dev);
    return;

err_unlock:
    spin_unlock_irqrestore(&ecp->lock, flags);
}

static int asoc_ethernet_resume(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "asoc_ethernet_resume()\n");
    struct net_device *dev =  g_eth_dev[0];
    ec_priv_t *ecp = netdev_priv(dev);
    queue_delayed_work(resume_queue, &resume_work, msecs_to_jiffies(PHY_RESUME_DELAY));
	
    return 0;
}

/**
 * ec_netdev_init -- initialize ethernet controller
 *
 * return 0 if success, others if fail
 */
static int ec_netdev_init(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = NULL;

    printk(KERN_INFO "ETHERNET_BASE: 0x%x\n", ETHERNET_BASE);
    printk(KERN_INFO "ecp->hwrp: 0x%p\n", ecp->hwrp);
    /* ETHERNET_BASE address is taken from dts when driver probe,
     * instead of hard-coding here */
    /* ecp->hwrp = (volatile ethregs_t *)IO_ADDRESS(ETHERNET_BASE); */
    ecp->netdev = dev;
    hw_regs = ecp->hwrp;

    /* set default value of status */
#ifdef DETECT_POWER_SAVE
    ecp->enable = false;
#endif
    ecp->linked = false;
    ecp->opened = false;
    ecp->mac_overrided = false;
    ecp->multicast = false;
    ecp->all_multicast = false;
    ecp->promiscuous = false;
    ecp->autoneg = true;
    ecp->speed = ETH_SPEED_100M;
    ecp->duplex = ETH_DUP_FULL;
    ecp->mac_addr = NULL;

    memset(&ecp->multicast_list, 0, sizeof(struct mac_list));
    spin_lock_init(&ecp->lock);

    //uncache address
    ecp->tx_bd_base = (ec_bd_t *)dma_alloc_coherent(NULL, sizeof(ec_bd_t) * TX_RING_SIZE, &ecp->tx_bd_paddr, GFP_KERNEL);
    ecp->rx_bd_base = (ec_bd_t *)dma_alloc_coherent(NULL, sizeof(ec_bd_t) * RX_RING_SIZE, &ecp->rx_bd_paddr, GFP_KERNEL);

    printk(KERN_INFO "ecp->tx_bd_base:%p, ecp->tx_bd_paddr:%p\n", ecp->tx_bd_base, (void *)ecp->tx_bd_paddr);
    printk(KERN_INFO "ecp->rx_bd_base:%p, ecp->rx_bd_paddr:%p\n", ecp->rx_bd_base, (void *)ecp->rx_bd_paddr);
    printk(KERN_INFO "virt_to_phys(ecp->tx_bd_base):%p\n", (void *)virt_to_phys(ecp->tx_bd_base));

    if (ecp->tx_bd_base == NULL || ecp->rx_bd_base == NULL) {
        printk(KERN_INFO "dma_alloc mem failed, tx_bd_base:%p, rx_bd_base:%p\n",
                ecp->tx_bd_base, ecp->rx_bd_base);

        if (ecp->tx_bd_base)
            dma_free_coherent(NULL, sizeof(ec_bd_t) * TX_RING_SIZE,
                    ecp->tx_bd_base, ecp->tx_bd_paddr);
        if (ecp->rx_bd_base)
            dma_free_coherent(NULL, sizeof(ec_bd_t) * RX_RING_SIZE,
                    ecp->rx_bd_base, ecp->rx_bd_paddr);

        return -ENOMEM;
    }
   #if 0
    if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
        /*For gl5302 INT Pending unusual */
        ethernet_clock_enable();
        ecp->phy_ops->phy_hw_init(ecp);
    }
  #endif //0
    ecp->mac_addr = g_default_mac_addr;
    ecp->mac_overrided = false;

    /* should after set_mac_addr() which will set ec_priv_t{.mac_addr} */
    memcpy(dev->dev_addr, ecp->mac_addr, ETH_MAC_LEN);

    ether_setup(dev);

    INFO_GREEN("net device : %s init over.\n", dev->name);
  #if 0
    if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
        hw_regs->er_opmode &= ~(EC_OPMODE_SR | EC_OPMODE_ST);  // stop tx and rx
        hw_regs->er_ienable = 0;

        /*ethernet wrapper rest and phy has no power*/
        atc260x_reg_setbits(ecp->atc260x, atc2603_CMU_DEVRST, 0x08, 0x08);
        udelay(100);
        atc260x_reg_setbits(ecp->atc260x, atc2603_CMU_DEVRST, 0x08, 0x0);
        udelay(100);

        ethernet_clock_disable();
    }
    #endif //0
    return (0);
}

static void ec_netdev_uninit(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);

    printk(KERN_INFO "\n");
#if 0
    if (ecp->tx_wq) {
        destroy_workqueue(ecp->tx_wq);
    }

    if (ecp->rx_wq) {
        destroy_workqueue(ecp->rx_wq);
    }
#endif

    /* after all works have been completed and destroyed */
    free_rxtx_skbs(ecp->rx_skb, RX_RING_SIZE);
    free_rxtx_skbs(ecp->tx_skb, TX_RING_SIZE);

    if (ecp->tx_bd_base)
        dma_free_coherent(NULL, sizeof(ec_bd_t) * TX_RING_SIZE,
                ecp->tx_bd_base, ecp->tx_bd_paddr);
    if (ecp->rx_bd_base)
        dma_free_coherent(NULL, sizeof(ec_bd_t) * RX_RING_SIZE,
                ecp->rx_bd_base, ecp->rx_bd_paddr);
}

static const struct net_device_ops ec_netdev_ops = {
    .ndo_init = ec_netdev_init,
    .ndo_uninit = ec_netdev_uninit,
    .ndo_open = ec_netdev_open,
    .ndo_stop = ec_netdev_close,
    .ndo_start_xmit = ec_netdev_start_xmit,
    .ndo_set_rx_mode = ec_netdev_set_multicast_list,
    .ndo_set_mac_address = ec_netdev_set_mac_addr,
    .ndo_get_stats = ec_netdev_query_stats,
    .ndo_tx_timeout = ec_netdev_transmit_timeout,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_do_ioctl = ec_netdev_ioctrl,
};

static struct pinctrl *ppc;
//static struct pinctrl_state *pps;

static int ethernet_set_pin_mux(struct platform_device *pdev)
{
	int ret = 0;

	ppc = pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(ppc)) {
		pr_err("pinctrl_get_select_default() error, ret = %ld\n",
			PTR_ERR(ppc));
		goto error;
	}

	pr_info("ethernet pinctrl select state successfully\n");

	putl((getl(PAD_DRV0) & 0xffff3fff) | 0x8000, PAD_DRV0);
	printk(KERN_INFO "PAD_DRV0:0x%x\n", act_readl(PAD_DRV0));

	
	return ret;

error:
	ret = (int)PTR_ERR(ppc);
	ppc = NULL;
	return ret;

}

static void ethernet_put_pin_mux(void)
{
	if (ppc)
		pinctrl_put(ppc);
}

static int ethernet_phy_probe(struct platform_device *pdev)
{
	struct device_node *phy_node = pdev->dev.of_node;
	struct net_device *dev = g_eth_dev[0];
	struct resource *res;
	ec_priv_t *ecp = netdev_priv(dev);
	const char *str;
	printk(KERN_INFO " %s  %d\n", __FUNCTION__,__LINE__);
	str = of_get_property(phy_node, "compatible", NULL);
	printk(KERN_INFO "ethernet phy's compatible: %s\n", str);

	/* The standard phy that register accessed via MDIO only support ksz8841tl now */
	if (of_device_is_compatible(phy_node, "realk,rtl8201")) {
		ecp->phy_model = ETH_PHY_MODEL_RTL8201;
		printk(KERN_INFO "phy model is rtl8201\n");
	} else if (of_device_is_compatible(phy_node, "SR8201G,sr8201g")) {
		ecp->phy_model = ETH_PHY_MODEL_SR8201G;
		printk(KERN_INFO "phy model is sr8201g\n");
	} { /* ATC2605 or error */
		printk(KERN_INFO "compatible of %s: %s\n", phy_node->full_name, str);
	}

	printk(KERN_INFO "phy_node->full_name: %s\n", phy_node->full_name);

#ifndef PHY_USE_POLL
	        dev->irq = irq_of_parse_and_map(phy_node, 0);
			printk(KERN_INFO "dev->irq=%d\n",dev->irq);
    	if (dev->irq < 0) {
            printk(KERN_INFO "No IRQ resource for tp\n");
    		return -ENODEV;
    	}
#endif

	#if 0
	if (ecp->phy_model == ETH_PHY_MODEL_ATC2605)
		ecp->atc260x = dev_get_drvdata(pdev->dev.parent);
	else
		ecp->atc260x = NULL;
     #endif//0
	/*res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_INFO "phy %s has no irq resource\n", phy_node->full_name);
		return -EINVAL;
	}
	/dev->irq = res->start;
	printk(KERN_INFO "phy irq is %u\n", dev->irq);
*/
	return 0;
}

static int __exit ethernet_phy_remove(struct platform_device *pdev)
{
	return 0;
}

/* ethernet phy match table */
static const struct of_device_id ethernet_phy_of_match[] __initconst = {
	{ .compatible = "SR8201G,sr8201g", },
	{},
};
MODULE_DEVICE_TABLE(of, ethernet_phy_of_match);

static struct platform_driver ethernet_phy_driver = {
    .probe      = ethernet_phy_probe,
    .remove     = __exit_p(ethernet_phy_remove),
    .driver     = {
        .name   = "rtl8201",
        .owner  = THIS_MODULE,
	.of_match_table = ethernet_phy_of_match,
    },
};

static int __init asoc_ethernet_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	ec_priv_t *ecp;
	int ret;
	struct resource *res;
	int phy_mode = ETH_PHY_MODE_RMII; /* use rmii as default */
	const char *phy_mode_str,*phy_status_str;
	struct device_node *phy_node;
	const char *str;

	if (!get_def_mac_addr(pdev))
		printk(KERN_INFO "use same default mac\n");

	ret = ethernet_set_pin_mux(pdev);
	if (ret)
		return ret;

	ret = of_property_read_string(pdev->dev.of_node, "status", &phy_status_str);
	if (ret == 0 && strcmp(phy_status_str, "okay") != 0) {
		dev_info(&pdev->dev, "disabled by DTS\n");
		return -ENODEV;
	}
	
	ret = of_property_read_string(pdev->dev.of_node, "phy-mode", &phy_mode_str);
	if (ret) {
		/* if get phy mode failed, use default rmii */
		printk(KERN_INFO "get phy mode failed, use rmii\n");
	} else {
		printk(KERN_INFO "phy_mode_str: %s\n", phy_mode_str);
		if (!strcmp(phy_mode_str, "rmii"))
			phy_mode = ETH_PHY_MODE_RMII;
		else if (!strcmp(phy_mode_str, "smii"))
			phy_mode = ETH_PHY_MODE_SMII;
		else
			printk(KERN_INFO "unknown phy mode: %s, use rmii\n", phy_mode_str);
	}

	ret = ethernet_clock_config(phy_mode);
	 putl(getl(CMU_ETHERNETPLL) | (0x1<<1), CMU_ETHERNETPLL);
	if (ret) {
		ethernet_put_pin_mux();
		return ret;
	}

	printk(KERN_INFO "pdev->name: %s\n", pdev->name ? pdev->name : "<null>");
	dev = alloc_etherdev(sizeof(struct dev_priv));
	SET_NETDEV_DEV(dev,&pdev->dev);
	if (NULL == dev)
		return -ENOMEM;
	sprintf(dev->name, "eth%d", 0);
	ecp = netdev_priv(dev);
	g_eth_dev[0] = dev;
	ecp->last_link_status=false;
	ecp->rx_timeout=false;
	ecp->phy_mode = phy_mode;
	ecp->phy_addr=0xff;
	printk(KERN_INFO "phy_mode: %u\n", ecp->phy_mode);

	phy_node = of_parse_phandle(pdev->dev.of_node, "phy-handle", 0);
	if (!phy_node) {
		printk(KERN_INFO "phy-handle of ethernet node can't be parsed\n");
		return -EINVAL;
	}

	ecp->phy_model = ETH_PHY_MODEL_SR8201G; /* default ATC2605 phy */

  if (of_property_read_u32(pdev->dev.of_node, "phy_addr", &ecp->phy_addr)) {
		printk(KERN_INFO "can't get phy addr form DTS \n");      
  }
    
	ret=ethernet_parse_gpio(pdev->dev.of_node, "phy-power-gpios",&ecp->phy_power_gpio);
	if(ret<0){
		printk(KERN_INFO " %s  can't get phy power gpio\n", __FUNCTION__);
	}
		
	ret=ethernet_parse_gpio(pdev->dev.of_node, "phy-reset-gpios",&ecp->phy_reset_gpio);

      if (gpio_is_valid(ecp->phy_power_gpio.gpio)) {       
	  	gpio_request(ecp->phy_power_gpio.gpio, "phy_power_gpio");        
	}  
	if (gpio_is_valid(ecp->phy_reset_gpio.gpio)) {     
		 gpio_request(ecp->phy_reset_gpio.gpio, "phy_reset_gpio");       
	} 
	
	str = of_get_property(phy_node, "compatible", NULL);
	if (of_device_is_compatible(phy_node, "actions,atc260x-ethernet")) {
		ecp->phy_model = ETH_PHY_MODEL_ATC2605;
		printk(KERN_INFO "phy model is ATC2605\n");
		/* platform device atc260x-ethernet's been created by atc260x_dev.c */
	} else {
		/* all other standard phy that register accessed via MDIO are
		 * recognized by ethernet_phy_probe() 
		struct platform_device *phy_pdev;
		const char *name = strstr(str, ",");
		phy_pdev = of_platform_device_create(phy_node, ++name, &pdev->dev);
		printk(KERN_INFO "ethernet phy name: %s\n", name);
		if (!phy_pdev) {
			printk(KERN_INFO "of_platform_device_create(%s) failed\n", name);
			goto err_free;
		}*/
	}
	
	e_clk = clk_get(NULL, CLKNAME_CMUMOD_ETHERNET);

	ret = clk_prepare(e_clk);
	if (ret)
		printk(KERN_INFO "prepare ethernet clock failed, errno: %d\n", -ret);
		
	ret = platform_driver_register(&ethernet_phy_driver);
	if (ret) {
		printk(KERN_INFO "register ethernet phy driver failed\n");
		goto err_free;
	}
	printk(KERN_INFO "phy_model: %u\n", ecp->phy_model);

	ret = -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_INFO "no mem resource\n");
		goto err_free;
	}
	ecp->hwrp = (volatile ethregs_t *)IO_ADDRESS(res->start);
	printk(KERN_INFO "res->start: %p, ecp->hwrp: %p\n",
			(void *)res->start, (void *)ecp->hwrp);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		printk(KERN_INFO "no irq resource\n");
		goto err_free;
	}
	ecp->mac_irq = res->start;

	printk(KERN_INFO "ecp->mac_irq: %u, dev->irq: %u\n", ecp->mac_irq, dev->irq);
#if 0
	ecp->hwrp = (volatile ethregs_t *)IO_ADDRESS(ETHERNET_BASE);
	ecp->mac_irq = OWL_IRQ_ETHERNET;
	dev->irq = ASOC_ETHERNET_PHY_IRQ;
#endif
	//dev->irq = ASOC_ETHERNET_PHY_IRQ;
	if (ETH_PHY_MODE_SMII == phy_mode)
		ecp->hwrp->er_macctrl = MAC_CTRL_SMII; /* use smii interface */
	else
		ecp->hwrp->er_macctrl = MAC_CTRL_RMII; /* use rmii interface */

	dev->watchdog_timeo = EC_TX_TIMEOUT;
	dev->netdev_ops = &ec_netdev_ops;
	ec_set_ethtool_ops(dev);
	ep_set_phy_ops(ecp);

	ret = register_netdev(dev);
	if (ret < 0) {
		printk(KERN_INFO "register netdev ret:%d, [irq:%d, dev name:%s]\n",
				ret, dev->irq, dev->name ? dev->name : "Unknown");
		g_eth_dev[0] = NULL;
		goto err_free;
	}
	device_create_file(&dev->dev, &dev_attr_netif_msg);
#ifdef ETH_TEST_MODE
	device_create_file(&dev->dev, &dev_attr_continuity);
	device_create_file(&dev->dev, &dev_attr_send_pattern);
	device_create_file(&dev->dev, &dev_attr_test_mode);
#endif
	resume_queue = create_workqueue("kethernet_resume_work");
	if (!resume_queue) {
		printk(KERN_INFO "create workqueue kethernet_resume_work failed\n");
		ret = -ENOMEM;
		goto err_remove;
	}

	INIT_DELAYED_WORK(&resume_work, ethernet_resume_handler);

#ifdef DETECT_POWER_SAVE
	power_save_queue = create_workqueue("kethernet_power_save_work");
	if (!power_save_queue) {
		ret = -ENOMEM;
		destroy_workqueue(resume_queue);
		goto err_remove;
	}
	INIT_WORK(&ecp->power_save_work, ethernet_power_save);
#endif
#ifdef PHY_USE_POLL
	phy_detect_queue = create_workqueue("phy_detect_work");
	if (!phy_detect_queue) {
		ret = -ENOMEM;
		goto err_remove;
	}
	INIT_DELAYED_WORK(&ecp->phy_detect_work, phy_detect_func);
#endif
	INIT_WORK(&ecp->hardware_reset_work,hardware_reset_do_work);
	return 0;

err_remove:
	device_remove_file(&dev->dev, &dev_attr_netif_msg);
#ifdef ETH_TEST_MODE
	device_remove_file(&dev->dev, &dev_attr_send_pattern);
	device_remove_file(&dev->dev, &dev_attr_test_mode);
	device_remove_file(&dev->dev, &dev_attr_continuity);
#endif
	unregister_netdev(dev);

err_free:
	if (dev) {
		free_netdev(dev);
		dev = NULL;
	}
	return ret;
}

static int __exit asoc_ethernet_remove(struct platform_device *pdev)
{
	struct net_device *dev = g_eth_dev[0];
  ec_priv_t *ecp = netdev_priv(dev);
#ifdef PHY_USE_POLL
    if(phy_detect_queue!=NULL){     	
      cancel_delayed_work_sync(&ecp->phy_detect_work);   
	  	destroy_workqueue(phy_detect_queue);
	  	phy_detect_queue=NULL;
	  }
#endif	
	if (dev) {
		device_remove_file(&dev->dev, &dev_attr_netif_msg);
#ifdef ETH_TEST_MODE
		device_remove_file(&dev->dev, &dev_attr_send_pattern);
		device_remove_file(&dev->dev, &dev_attr_test_mode);
		device_remove_file(&dev->dev, &dev_attr_continuity);
#endif
		unregister_netdev(dev);
		free_netdev(dev);
	}
	if (resume_queue)
		destroy_workqueue(resume_queue);

#ifdef DETECT_POWER_SAVE
	destroy_workqueue(power_save_queue);
#endif

	platform_driver_unregister(&ethernet_phy_driver);
	if(e_clk!=NULL){
  clk_disable(e_clk);
		clk_unprepare(e_clk);  	
  clk_put(e_clk);
  }

	
	ethernet_put_pin_mux();
	return 0;
}


/* Match table for of_platform binding */
static const struct of_device_id actions_ethernet_of_match[] __initconst = {
	{ .compatible = "actions,owl-ethernet", },
	{},
};
MODULE_DEVICE_TABLE(of, actions_ethernet_of_match);

static struct platform_driver asoc_ethernet_driver = {
    .probe      = asoc_ethernet_probe,
    .remove     = __exit_p(asoc_ethernet_remove),
    .driver     = {
        .name   = "owl-ethernet",
        .owner  = THIS_MODULE,
	.of_match_table = actions_ethernet_of_match,
    },
    .suspend     = asoc_ethernet_suspend,
    .resume     = asoc_ethernet_resume,
};

static int __init asoc_ethernet_init(void)
{
	printk(KERN_INFO "%s %d\n",__FUNCTION__,__LINE__);
    return platform_driver_register(&asoc_ethernet_driver);
}
module_init(asoc_ethernet_init);

static void __exit asoc_ethernet_exit(void)
{
    platform_driver_unregister(&asoc_ethernet_driver);
}
module_exit(asoc_ethernet_exit);

MODULE_ALIAS("platform:asoc-ethernet");
MODULE_DESCRIPTION("asoc_ethernet pin");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions Semi, Inc");



