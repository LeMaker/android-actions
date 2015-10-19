/*******************************************************************************
ec_ethtool.c -- ethtool hooks for ethernet controller embedded in GL5201

author : yunchf @Actions
date : 2010-06-10
version 0.1

date : 2010-08-10
version 1.0

questions:
1. set_pauseparam() hook should be modified later

*******************************************************************************/

#include "ethctrl.h"


static const char *g_driver_name = "GL5201-eth-ctrl";
static const char *g_driver_version = "Version - 0.1";

#define ETH_STATS_LEN  (sizeof(struct net_device_stats) / sizeof(unsigned long))

#define ETHTOOL_INFO_STR_LEN 32

#define ECMD_SUPPORTED_MEDIA  \
    (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full | \
    SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full)

#define ECMD_ADVERTISED_MEDIA \
    (ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full | \
    ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full)

typedef enum {
    EC_FC_NONE = 0,
    EC_FC_TX_PAUSE,
    EC_FC_RX_PAUSE,
    EC_FC_FULL
} ec_flowctrl_type_t;


struct hw_regs_ranges {
    int     start;
    int     end;
    int     interval;
};


/*------------------------------ ethtool hooks ------------------------------*/

/**
 * et_get_settings -- get transceiver's settings that are specified by @cmd
 * @dev : target net device
 * @cmd : requested command
 * return 0 if success, otherwise fail
 */
static int et_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    ec_priv_t *ecp = netdev_priv(dev);
    struct mii_if_info *mii = &ecp->mii_info;
    unsigned int advert;
    unsigned int bmcr;


    cmd->supported = (ECMD_SUPPORTED_MEDIA | SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII);

    cmd->port = PORT_MII;       /* twisted-pair (TP) supported only */
    cmd->transceiver = XCVR_INTERNAL;   /* internal transceiver supported only */
    cmd->phy_address = mii->phy_id;
    cmd->advertising = ADVERTISED_TP | ADVERTISED_MII;

    /* get phy's supported advertise */
    advert = mii->mdio_read(dev, mii->phy_id, MII_ADVERTISE);
    if (advert & ADVERTISE_10HALF) {
        cmd->advertising |= ADVERTISED_10baseT_Half;
    }
    if (advert & ADVERTISE_10FULL) {
        cmd->advertising |= ADVERTISED_10baseT_Full;
    }
    if (advert & ADVERTISE_100HALF) {
        cmd->advertising |= ADVERTISED_100baseT_Half;
    }
    if (advert & ADVERTISE_100FULL) {
        cmd->advertising |= ADVERTISED_100baseT_Full;
    }

    /* get current speed, duplex and auto-negotiation mode */
    bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
    if (bmcr & BMCR_ANENABLE) {
        unsigned int lpa;
        unsigned int neg;

        cmd->autoneg = AUTONEG_ENABLE;
        cmd->advertising |= ADVERTISED_Autoneg;

        lpa = mii->mdio_read(dev, mii->phy_id, MII_LPA);
        neg = mii_nway_result(advert & lpa);

        if (neg == LPA_100FULL || neg == LPA_100HALF) {
            cmd->speed = SPEED_100;
        } else {
            cmd->speed = SPEED_10;
        }

        if (neg == LPA_100FULL || neg == LPA_10FULL) {
            cmd->duplex = DUPLEX_FULL;
            mii->full_duplex = 1;       /* set-settings() does not set it if auto-neg */
        } else {
            cmd->duplex = DUPLEX_HALF;
            mii->full_duplex = 0;
        }
    } else {
        cmd->autoneg = AUTONEG_DISABLE;
        cmd->speed = (bmcr & BMCR_SPEED100) ? SPEED_100 : SPEED_10;
        cmd->duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
    }

    /* ignore maxtxpkt, maxrxpkt for now */

    return 0;

}

#if 0
static int et_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    ec_priv_t *ecp = netdev_priv(dev);
    int     err;

    spin_lock_irq(&ecp->lock);
    err = mii_ethtool_gset(&ecp->mii_info, cmd);
    spin_unlock_irq(&ecp->lock);

    /* the ethernet controller does not support 1000baseT */
    cmd->supported &= ~(SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full);

    return (err);
}


static int et_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    ec_priv_t *ecp = netdev_priv(dev);
    int     err;

    printk("%s() autoneg : %d, speed: %d, duplex: %d\n", __func__, cmd->autoneg,
           cmd->speed, cmd->duplex);
    spin_lock_irq(&ecp->lock);
    err = mii_ethtool_sset(&ecp->mii_info, cmd);
    spin_unlock_irq(&ecp->lock);

    return (err);
}
#endif


/**
 * verify_et_cmd -- check the validity of requested command
 * @mii : mii interface to setup 
 * @cmd : requested command
 * return 0 if requested command is valid, -EINVAL if not
 */
static inline int verify_et_cmd(const struct ethtool_cmd *cmd, const struct mii_if_info *mii)
{
    /* only support 10M and 100M currently */
    if (SPEED_10 != cmd->speed && SPEED_100 != cmd->speed) {
        return (-EINVAL);
    }
    if (DUPLEX_HALF != cmd->duplex && DUPLEX_FULL != cmd->duplex) {
        return (-EINVAL);
    }
    if (PORT_MII != cmd->port) {
        return (-EINVAL);
    }
    if (XCVR_INTERNAL != cmd->transceiver) {
        return (-EINVAL);
    }
    if (cmd->phy_address != mii->phy_id) {
        return (-EINVAL);
    }
    if (AUTONEG_DISABLE != cmd->autoneg && AUTONEG_ENABLE != cmd->autoneg) {
        return (-EINVAL);
    }
    if (AUTONEG_ENABLE == cmd->autoneg) {
        if (0 == (cmd->advertising & ECMD_ADVERTISED_MEDIA)) {
            return (-EINVAL);
        }
    }

    return (0);
}


/**
 * set_autoneg_settings -- to setup @mii if the requested @cmd supports auto-negotiation 
 */
static void set_autoneg_settings(const struct ethtool_cmd *cmd, struct mii_if_info *mii)
{
    struct net_device *dev = mii->dev;
    unsigned int bmcr;
    unsigned int advert;
    unsigned int tmp;


    /* advertise only what has been requested */
    advert = mii->mdio_read(dev, mii->phy_id, MII_ADVERTISE);
    tmp = advert & ~(ADVERTISE_ALL | ADVERTISE_100BASE4);

    if (cmd->advertising & ADVERTISED_10baseT_Half) {
        tmp |= ADVERTISE_10HALF;
    }
    if (cmd->advertising & ADVERTISED_10baseT_Full) {
        tmp |= ADVERTISE_10FULL;
    }
    if (cmd->advertising & ADVERTISED_100baseT_Half) {
        tmp |= ADVERTISE_100HALF;
    }
    if (cmd->advertising & ADVERTISED_100baseT_Full) {
        tmp |= ADVERTISE_100FULL;
    }

    if (advert != tmp) {
        mii->mdio_write(dev, mii->phy_id, MII_ADVERTISE, tmp);
        mii->advertising = tmp;
    }

    /* enable auto-negotiation, and force a re-negotiate */
    bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
    bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
    mii->mdio_write(dev, mii->phy_id, MII_BMCR, bmcr);

    mii->force_media = 0;

    return;
}


/**
 * set_forced_settings -- to setup @mii if the requested @cmd does not support auto-negotiation 
 */
static void set_forced_settings(const struct ethtool_cmd *cmd, struct mii_if_info *mii)
{
    struct net_device *dev = mii->dev;
    unsigned int bmcr;
    unsigned int new_bmcr;


    /* disable auto-negotiation, set speed and duplexity */
    bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
    new_bmcr = bmcr & ~(BMCR_ANENABLE | BMCR_SPEED100 | BMCR_FULLDPLX);

    if (SPEED_100 == cmd->speed) {
        new_bmcr |= BMCR_SPEED100;
    }

    if (DUPLEX_FULL == cmd->duplex) {
        new_bmcr |= BMCR_FULLDPLX;
        mii->full_duplex = 1;
    } else {
        mii->full_duplex = 0;
    }

    if (bmcr != new_bmcr) {

        /* you can get a explanation from ec_netdev_ioctl() for SIOCSMIIREG command */
        if (need_change_speed(new_bmcr, bmcr)) {
            unsigned int changed = new_bmcr;

            changed ^= BMCR_SPEED100;   /* exchange speed only */
            mii->mdio_write(dev, mii->phy_id, MII_BMCR, changed);
        }
        mii->mdio_write(dev, mii->phy_id, MII_BMCR, new_bmcr);
    }

    mii->force_media = 1;

    return;
}


/**
 * et_set_settings -- setup transceiver of @dev net device according the requested @cmd
 * @dev : net device to configure 
 * @cmd : requested command
 * return 0 if success, otherwise fail
 */
static int et_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    ec_priv_t *ecp = netdev_priv(dev);
    struct mii_if_info *mii = &ecp->mii_info;


    if (verify_et_cmd(cmd, mii)) {
        return (-EINVAL);
    }

    if (AUTONEG_ENABLE == cmd->autoneg) {
        set_autoneg_settings(cmd, mii);
    } else {
        set_forced_settings(cmd, mii);
    }

    return (0);
}


/**
 * et_get_drvinfo -- get driver's information of @dev net device
 */
static void et_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
    strncpy(info->driver, g_driver_name, ETHTOOL_INFO_STR_LEN);
    strncpy(info->version, g_driver_version, ETHTOOL_INFO_STR_LEN);
}




/**
 * et_get_regs_len -- get the number of registers of MAC
 */
static int et_get_regs_len(struct net_device *dev)
{
    int     len = 0;



    return (len);
}


/**
 * et_get_regs -- dump all register of MAC
 */
static void et_get_regs(struct net_device *dev, struct ethtool_regs *regs, void *buf)
{
//    ec_priv_t *ecp = netdev_priv(dev);
//    struct hw_regs_ranges *rgs = BUILD_REGS_RANGES_ARRAY();
//    unsigned long *bufp = (unsigned long *) buf;
//    unsigned long *reg = NULL;
//
//    spin_lock_irq(&ecp->lock);
//    regs->version = 0;
//    while (rgs->start < rgs->end) {
//        reg = (unsigned long *) ((char *) ecp->hwrp + rgs->start);
//        while ((char *) reg - (char *) ecp->hwrp <= rgs->end) {
//            *bufp = *reg;
//            reg += rgs->interval;
//            bufp++;
//        }
//        rgs++;
//    }
//
//    spin_unlock_irq(&ecp->lock);
}

//static void    (*get_wol)(struct net_device *, struct ethtool_wolinfo *);
//static int (*set_wol)(struct net_device *, struct ethtool_wolinfo *);


static u32 et_get_msglevel(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
    return (ecp->msg_enable);
}


static void et_set_msglevel(struct net_device *dev, u32 level)
{
    ec_priv_t *ecp = netdev_priv(dev);
    ecp->msg_enable = level;
}


/**
 * et_nway_reset -- re-negotiate the transceiver of @dev net device
 * @dev : target net device
 * return 0 if success, -EINVAL if fail
 */
static int et_nway_reset(struct net_device *dev)
{
    ec_priv_t *ecp = netdev_priv(dev);
    struct mii_if_info *mii = &ecp->mii_info;
    int     err = -EINVAL;
    int     bmcr;

    /* if autoneg is off, it's an error */
    bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);

    if (bmcr & BMCR_ANENABLE) {
        bmcr |= BMCR_ANRESTART;
        mii->mdio_write(dev, mii->phy_id, MII_BMCR, bmcr);
        err = 0;
    }

    return (err);
}


static u32 et_get_link(struct net_device *dev)
{

   // ec_priv_t *ecp = netdev_priv(dev);
    //return (ecp->phy_ops->phy_get_link(ecp));
    return (netif_carrier_ok(dev) ? 1 : 0);
}

//static int (*get_eeprom_len)(struct net_device *);
//static int (*get_eeprom)(struct net_device *, struct ethtool_eeprom *, u8 *);
//static int (*set_eeprom)(struct net_device *, struct ethtool_eeprom *, u8 *);
//static int (*get_coalesce)(struct net_device *, struct ethtool_coalesce *);
//static int (*set_coalesce)(struct net_device *, struct ethtool_coalesce *);
//static void    ec_get_ringparam(struct net_device *dev, struct ethtool_ringparam *ring)

//static int (*set_ringparam)(struct net_device *, struct ethtool_ringparam *);


static void et_get_pauseparam(struct net_device *dev, struct ethtool_pauseparam *epp)
{
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *regs = ecp->hwrp;

    epp->autoneg = ecp->autoneg ? 1 : 0;
    epp->tx_pause = regs->er_flowctrl & EC_FLOWCTRL_TPE ? 1 : 0;
    epp->rx_pause = regs->er_flowctrl & EC_FLOWCTRL_RPE ? 1 : 0;
}


static inline int set_flowctrl_type(struct net_device *dev, ec_flowctrl_type_t fc)
{
    ec_priv_t *ecp = netdev_priv(dev);
    volatile ethregs_t *hw_regs = ecp->hwrp;

    // modify later,  relationship between different bits of flow control of csr20
    switch (fc) {
        case EC_FC_FULL:
            hw_regs->er_flowctrl |= EC_FLOWCTRL_ENALL;
            break;
        case EC_FC_RX_PAUSE:
            hw_regs->er_flowctrl &= ~EC_FLOWCTRL_ENALL;
            hw_regs->er_flowctrl |= EC_FLOWCTRL_FCE | EC_FLOWCTRL_RPE;
            break;
        case EC_FC_TX_PAUSE:
            hw_regs->er_flowctrl &= ~EC_FLOWCTRL_ENALL;
            hw_regs->er_flowctrl |= EC_FLOWCTRL_TUE | EC_FLOWCTRL_TPE | EC_FLOWCTRL_FCE;
            break;
        case EC_FC_NONE:
            hw_regs->er_flowctrl &= ~EC_FLOWCTRL_ENALL;
            break;
        default:
            return (-1);
    }
    return (0);
}


/**
 * et_set_pauseparam --  
 */
// XXXX NOTE :  modify later , in fact i dont know how to do it  currently
static int et_set_pauseparam(struct net_device *dev, struct ethtool_pauseparam *epp)
{
    ec_priv_t *ecp = netdev_priv(dev);
    ec_flowctrl_type_t type;
    int     err = 0;

    if (epp->autoneg) {

        //set advertise to support pause
        if (ecp->autoneg) {
            if (netif_running(dev)) {
                netif_carrier_off(dev); // stop rx and tx is better
            }
            ecp->phy_ops->phy_setup_advert(ecp, ADVERTISE_ALL | ADVERTISE_PAUSE_CAP);
            ecp->phy_ops->phy_setup_aneg(ecp, true);    // can rise link change interrupt?
            ecp->phy_ops->phy_read_status(ecp);

            //ecp->phy_ops->phy_get_link(ecp);
        } else {
            return (-1);
        }
    } else {
        if (epp->rx_pause && epp->tx_pause) {
            type = EC_FC_FULL;
        } else if (epp->rx_pause && !epp->tx_pause) {
            type = EC_FC_RX_PAUSE;
        } else if (!epp->rx_pause && epp->tx_pause) {
            type = EC_FC_TX_PAUSE;
        } else {
            type = EC_FC_NONE;
        }
        err = set_flowctrl_type(dev, type);
    }
    return (err);
}


struct stats_str {
    char    string[ETH_GSTRING_LEN];
};


static struct stats_str g_stats_descs[ETH_STATS_LEN] = {
    {"rx_packets"}, {"tx_packets"},
    {"rx_bytes"}, {"tx_bytes"},
    {"rx_errors"}, {"tx_errors"},
    {"rx_dropped"}, {"tx_dropped"},
    {"multicast"}, {"collisions"},
    {"rx_length_errors"}, {"rx_over_errors"},
    {"rx_crc_errors"}, {"rx_frame_errors"},
    {"rx_fifo_errors"}, {"rx_missed_errors"},
    {"tx_aborted_errors"}, {"tx_carrier_errors"},
    {"tx_fifo_errors"}, {"tx_heartbeat_errors"},
    {"tx_window_errors"}, {"rx_compressed"}, {"tx_compressed"}
};


static void et_get_strings(struct net_device *dev, u32 stringset, u8 * buf)
{
    if (ETH_SS_STATS == stringset) {
        memcpy(buf, g_stats_descs, sizeof(g_stats_descs));
    }
}

// led blinking
//static int ec_phys_id(struct net_device *dev, u32 id)

static void et_get_stats(struct net_device *dev, struct ethtool_stats *stats, u64 * data)
{

    //ec_priv_t *ecp = netdev_priv(dev);
    int     len = ETH_STATS_LEN;
    int     i;

    for (i = 0; i < len; i++) {
        data[i] = ((unsigned long *) &dev->stats)[i];;
    }
}
//static int ec_begin(struct net_device *dev)

//static void    ec_complete(struct net_device *dev)

//static u32     (*get_ufo)(struct net_device *);
//static int     (*set_ufo)(struct net_device *, u32);

//ethtool_op_get_flags()
//static u32     ec_get_flags(struct net_device *dev)

// ethtool_op_set_flags()
//static int     ec_set_flags(struct net_device *dev, u32 flags)

//static u32     ec_get_priv_flags(struct net_device *dev)

//static int     ec_set_priv_flags(struct net_device *dev, u32 flags)


static int et_get_sset_count(struct net_device *dev, int sset)
{
    switch (sset) {
            //case ETH_SS_TEST:
            //return EC_NUM_TEST;
        case ETH_SS_STATS:
            return ETH_STATS_LEN;
        default:
            return -EOPNOTSUPP;
    }
}


static struct ethtool_ops ec_ethtool_ops = {
    .get_settings = et_get_settings,
    .set_settings = et_set_settings,
    .get_drvinfo = et_get_drvinfo,
    .get_regs_len = et_get_regs_len,
    .get_regs = et_get_regs,
    .get_msglevel = et_get_msglevel,
    .set_msglevel = et_set_msglevel,
    .nway_reset = et_nway_reset,
    .get_link = et_get_link,
    .get_pauseparam = et_get_pauseparam,
    .set_pauseparam = et_set_pauseparam,
    //.get_sg = ethtool_op_get_sg,
    .get_strings = et_get_strings,
    .get_ethtool_stats = et_get_stats,
    .get_sset_count = et_get_sset_count
};


void ec_set_ethtool_ops(struct net_device *dev)
{
    SET_ETHTOOL_OPS(dev, &ec_ethtool_ops);
}
