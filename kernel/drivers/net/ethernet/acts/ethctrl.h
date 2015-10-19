/******************************************************************************
 ethctrl.h -- ethernet controller header for GL5201
 
 author : yunchf  @Actions 
 date : 2010-04-08 
 version 0.1

 date : 2010-08-10
 version 1.0
 
******************************************************************************/
#ifndef _ETHCTRL_H_
#define _ETHCTRL_H_

/*****************************************************************************/

//#define DETECT_POWER_SAVE

#include <linux/spinlock.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
//#include <mach/atc260x.h>
#include "ec_hardware.h"

#ifdef DETECT_POWER_SAVE
#include <linux/timer.h>
#endif

//#define ETH_TEST_MODE

#define MULTICAST_LIST_LEN 14
#define ETH_MAC_LEN 6
#define ETH_CRC_LEN 4
#define RX_RING_SIZE 64

#define RX_RING_MOD_MASK  (RX_RING_SIZE - 1)
#ifndef ETH_TEST_MODE
#define TX_RING_SIZE 32
#else
#define TX_RING_SIZE 128
#endif

#define TX_RING_MOD_MASK (TX_RING_SIZE - 1)
#define TATOL_TXRX_BDS (RX_RING_SIZE + TX_RING_SIZE)

#ifdef CONFIG_POLL_PHY_STATE
#define PHY_RESUME_DELAY 2000 //唤醒时间，2000 ms
#define PHY_DETECT_DUTY 300		//轮询周期，300 ms
#define PHY_DETECT_RESUME 500		//唤醒后 phy detect时间 500ms
#endif

#define ETH_SPEED_10M 10
#define ETH_SPEED_100M 100
#define ETH_DUP_HALF 1
#define ETH_DUP_FULL 2
#define ETH_PKG_MIN 64
#define ETH_PKG_MAX 1518
#define PKG_RESERVE 18

#define PKG_MIN_LEN (ETH_PKG_MIN)

/* 0x600, reserve 18 byes for align adjustment */
#define PKG_MAX_LEN (ETH_PKG_MAX + PKG_RESERVE)

#define PHY_ADDR_LIMIT 0x20

/* ANSI Color codes */
#define VT(CODES)  "\033[" CODES "m"
#define VT_NORMAL  VT("")
#define VT_RED	   VT("0;32;31")
#define VT_GREEN   VT("1;32")
#define VT_YELLOW  VT("1;33")
#define VT_BLUE    VT("1;34")
#define VT_PURPLE  VT("0;35")

#define EC_TRACED 0
//#define EC_DEBUG
#undef RX_DEBUG
#undef TX_DEBUG
//#define RX_DEBUG
//#define TX_DEBUG

//#define ETHENRET_MAC_LOOP_BACK
//#define ETHENRET_PHY_LOOP_BACK
//#define WORK_IN_10M_MODE

#define _DBG(level, fmt, ...)  \
    printk(level "%s():%d: " fmt, \
        __func__, __LINE__, ## __VA_ARGS__)

//#ifdef EC_DEBUG
//#define printk(fmt, args...)  _DBG(KERN_INFO, fmt, ## args)
//#else
//#define printk(fmt, args...)  do {} while(0)
//#endif

//#define printk(fmt, args...)   _DBG(KERN_ERR, fmt, ## args)

#define _INFO(color, fmt, ...) \
    printk(color "" fmt ""VT_NORMAL, ## __VA_ARGS__)

/* mainly used in test code */
#define INFO_PURLPLE(fmt, args...) _INFO(VT_PURPLE, fmt, ## args)
#define INFO_RED(fmt, args...)     _INFO(VT_RED, fmt, ## args)
#define INFO_GREEN(fmt, args...)   _INFO(VT_GREEN, fmt, ## args)
#define INFO_BLUE(fmt, args...)    _INFO(VT_BLUE, fmt, ## args)

//#define EC_NOP __asm__ __volatile__ ("nop; nop; nop; nop" : :)
#define EC_NOP

enum eth_phy_model {
	ETH_PHY_MODEL_NONE = 0,
	ETH_PHY_MODEL_ATC2605, /* same as fdp110 */
	ETH_PHY_MODEL_KSZ8041TL, /* Micrel KSZ8041TL */
	ETH_PHY_MODEL_RTL8201, /* Realtek RTL8201 */
	ETH_PHY_MODEL_SR8201G, /* Corechip SR8201G */
	ETH_PHY_MODEL_MAX = ETH_PHY_MODEL_SR8201G,
};

enum eth_phy_mode {
	ETH_PHY_MODE_RMII = 0,
	ETH_PHY_MODE_SMII,
	ETH_PHY_MODE_MAX,
};

typedef struct mac_list {
    int     count;
    char    mac_array[MULTICAST_LIST_LEN][ETH_MAC_LEN];
} mac_list_t;

struct owl_ethernet_gpio {
    int gpio;
    int active;
};

typedef struct phy_info phy_info_t;

typedef struct dev_priv {
    volatile ethregs_t *hwrp;
    spinlock_t lock;
    struct net_device *netdev;
    struct atc260x_dev *atc260x;
    unsigned int mac_irq;
    unsigned int phy_irq;
    unsigned int phy_model;
    unsigned int phy_mode;
#ifdef ETH_TEST_MODE
    unsigned int test_mode;
#endif

    struct sk_buff *tx_skb[TX_RING_SIZE];       /* temp. save transmited skb */
    ec_bd_t *tx_bd_base;
    ec_bd_t *cur_tx;            /* the next tx free ring entry */
    ec_bd_t *dirty_tx;          /* the ring entry to be freed */
    ushort  skb_dirty;          /* = dirty_tx - tx_bd_base */
    ushort  skb_cur;            /* = cur_tx - tx_bd_base */
    dma_addr_t tx_bd_paddr;
    bool    tx_full;

    struct sk_buff *rx_skb[RX_RING_SIZE];       /* rx_bd buffers */
    ec_bd_t *rx_bd_base;
    ec_bd_t *cur_rx;            /* the next rx free ring entry */
    dma_addr_t	rx_bd_paddr;

#if 0
    struct workqueue_struct *tx_wq;
    struct work_struct tx_work;
    struct work_struct tx_timeout_work;
    struct sk_buff_head local_tx_queue;

    struct workqueue_struct *rx_wq;
    struct work_struct rx_work;
#endif

#ifdef DETECT_POWER_SAVE
    struct timer_list detect_timer;
    struct work_struct power_save_work;
#endif
	struct work_struct hardware_reset_work;
	struct workqueue_struct *ethernet_work_queue;
#ifdef CONFIG_POLL_PHY_STATE
	struct workqueue_struct *phy_detect_queue;
	struct delayed_work phy_detect_work;
#else
	struct work_struct netphy_irq_handle_work;
#endif
    //int phy_id;
    int     phy_addr;
    int     speed;
    int     duplex;
    bool    pause;
    bool    autoneg;
    phy_info_t *phy_ops;
    struct mii_if_info mii_info;

    int     msg_enable;
    const char *mac_addr;       /* XXX : careful later added */
    char    overrided_mac[ETH_MAC_LEN];
    mac_list_t multicast_list;
    bool    mac_overrided;
    bool    multicast;
    bool    all_multicast;
    bool    promiscuous;
    bool    opened;
    bool    linked;
#ifdef CONFIG_POLL_PHY_STATE
	bool    last_link_status;
#endif
    bool    rx_timeout;
#ifdef DETECT_POWER_SAVE
    bool    enable;             // hardware enabled
#endif
	struct platform_device *phy_pdev;
	struct owl_ethernet_gpio phy_power_gpio;
	struct owl_ethernet_gpio phy_reset_gpio;
} ec_priv_t;


struct phy_info {
    int     id;
    char   *name;
    int     (*phy_hw_init) (ec_priv_t * ecp);
    int     (*phy_init) (ec_priv_t * ecp);
    int     (*phy_suspend) (ec_priv_t * ecp, bool power_down);
    int     (*phy_setup_advert) (ec_priv_t * ecp, int advertising);
    int     (*phy_setup_forced) (ec_priv_t * ecp, int speed, int duplex);
    int     (*phy_setup_aneg) (ec_priv_t * ecp, bool autoneg);
    int     (*phy_setup_loopback) (ec_priv_t * ecp, bool loopback);
    int     (*phy_read_status) (ec_priv_t * ecp);
    int     (*phy_get_link) (ec_priv_t * ecp);
};


extern unsigned short (*read_phy_reg)(ec_priv_t *, unsigned short);
extern int (*write_phy_reg)(ec_priv_t *, unsigned short, unsigned short);

void ep_set_phy_ops(ec_priv_t * ecp);
void ec_set_ethtool_ops(struct net_device *netdev);


static inline bool need_change_speed(unsigned short new_bmcr, unsigned short old_bmcr)
{
    return (((new_bmcr ^ old_bmcr) & (BMCR_SPEED100 | BMCR_FULLDPLX)) == BMCR_FULLDPLX);
}



/******************************************************************************/

#endif                          /* _ETHCTRL_H_ */
