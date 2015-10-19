/******************************************************************************
ec_hardware.h -- data structure and macros  about hardware architecture and 
    registers of MAC and PHY

author : yunchf @Actions
date : 2010-04-08
version 0.1

data : 2010-08-10
version 1.0

******************************************************************************/

#ifndef _EC_HARDWARE_H_
#define _EC_HARDWARE_H_
/*****************************************************************************/
#include <mach/hardware.h>

#undef getl
#undef putl
#define getl(a)    act_readl(a)
#define putl(v, a) act_writel(v, a)

#define ASOC_ETHERNET_PHY_ADDR (0x1)		//phy地址：默认0x1,可通过DTS中配置phy_addr来配置
#define ASOC_ETHERNET_PHY_IRQ  OWL_IRQ_SIRQ0

/*
 * Should not read and write er_reserved* elements
 */
typedef struct ethregs {
    unsigned long er_busmode;   /* offset 0x0; bus mode reg. */
    unsigned long er_reserved0;
    unsigned long er_txpoll;    /* 0x08; transmit poll demand reg. */
    unsigned long er_reserved1;
    unsigned long er_rxpoll;    /* 0x10; receive poll demand reg. */
    unsigned long er_reserved2;
    unsigned long er_rxbdbase;  /* 0x18; receive descriptor list base address reg. */
    unsigned long er_reserved3;
    unsigned long er_txbdbase;  /* 0x20; transmit descriptor list base address reg. */
    unsigned long er_reserved4;
    unsigned long er_status;    /* 0x28; status reg. */
    unsigned long er_reserved5;
    unsigned long er_opmode;    /* 0x30; operation mode reg. */
    unsigned long er_reserved6;
    unsigned long er_ienable;   /* 0x38; interrupt enable reg. */
    unsigned long er_reserved7;
    unsigned long er_mfocnt;    /* 0x40; missed frames and overflow counter reg. */
    unsigned long er_reserved8;
    unsigned long er_miimng;    /* 0x48; software mii, don't use it here  */
    unsigned long er_reserved9;
    unsigned long er_miism;     /* 0x50; mii serial management */
    unsigned long er_reserved10;
    unsigned long er_imctrl;    /* 0x58; general-purpose timer and interrupt mitigation control */
    unsigned long er_reserved11[9];
    unsigned long er_maclow;    /* 0x80; mac address low */
    unsigned long er_reserved12;
    unsigned long er_machigh;   /* 0x88; mac address high */
    unsigned long er_reserved13;
    unsigned long er_cachethr;  /* 0x90; pause time and cache thresholds */
    unsigned long er_reserved14;
    unsigned long er_fifothr;   /* 0x98; pause control fifo thresholds */
    unsigned long er_reserved15;
    unsigned long er_flowctrl;  /* 0xa0; flow control setup and status */
    unsigned long er_reserved16[3];
    unsigned long er_macctrl;   /* 0xb0; mac control */
    unsigned long er_reserved17[83];

    unsigned long er_rxstats[31];       /* 0x200~0x278; receive statistics regs. */
    unsigned long er_reserved18[33];
    unsigned long er_txstats[41];       /* 0x300~0x3A0; transmit statistics regs. */
    unsigned long er_reserved19[23];
} ethregs_t;

/*
 * statistical counter of transmission and reception
 * NOTE : NOT using these registers in fact
 */
#define er_tx64         er_txstats[0]   /* offset:0x200 */
#define er_tx65to127    er_txstats[2]
#define er_tx128to255   er_txstats[4]
#define er_tx256to511   er_txstats[6]
#define er_tx512to1023  er_txstats[8]
#define er_tx1024to1518 er_txstats[10]
#define er_txtoolong    er_txstats[12]
#define er_txoctok      er_txstats[14]
#define er_txuniok      er_txstats[16]
#define er_txmultiok    er_txstats[18]
#define er_txbroadok    er_txstats[20]
#define er_txpauseok    er_txstats[22]
#define er_txcoll0      er_txstats[24]
#define er_txcoll1      er_txstats[26]
#define er_txcollmulti  er_txstats[28]
#define er_txcoll16     er_txstats[30]
#define er_txdefer      er_txstats[32]
#define er_txlcerr      er_txstats[34]
#define er_txmacerr     er_txstats[36]
#define er_txtxcserr    er_txstats[38]  /* offset:0x298 */

#define er_rx64         er_rxstats[0]   /* offset:0x300 */
#define er_rx65to127    er_rxstats[2]
#define er_rx128to255   er_rxstats[4]
#define er_rx256to511   er_rxstats[6]
#define er_rx512to1023  er_rxstats[8]
#define er_rx1024to1518 er_rxstats[10]
#define er_rxtoolong    er_rxstats[12]
#define er_rxoctok      er_rxstats[14]
#define er_rxuniok      er_rxstats[16]
#define er_rxmultiok    er_rxstats[18]
#define er_rxbroadok    er_rxstats[20]
#define er_rxpauseok    er_rxstats[22]
#define er_rxalignerr   er_rxstats[24]
#define er_rxfcserr     er_rxstats[26]
#define er_rxrxmiierr   er_rxstats[28]  /* offset:0x370 */

/*
 * receive and transmit buffer descriptor
 */
typedef struct buffer_descriptor {
    unsigned long status;
    unsigned long control;
    unsigned long buf_addr;
    unsigned long reserved;     /* we don't use second buffer address */
} ec_bd_t;


/*
 * ethernet clock enable
 */
#define ASOC_ETH_CLOCK_EN  (0x1 << 22)
#define ASOC_ETH_CLOCK_RST (0x1 << 20)


/*
 * rx bd status and control information
 */
#define RXBD_STAT_OWN (0x1 << 31)
#define RXBD_STAT_FF  (0x1 << 30)       /*filtering fail */
#define RXBD_STAT_FL(x) (((x) >> 16) & 0x3FFF)  /* frame leng */
#define RXBD_STAT_ES  (0x1 << 15)       /* error summary */
#define RXBD_STAT_DE  (0x1 << 14)       /* descriptor error */
#define RXBD_STAT_RF  (0x1 << 11)       /* runt frame */
#define RXBD_STAT_MF  (0x1 << 10)       /* multicast frame */
#define RXBD_STAT_FS  (0x1 << 9)        /* first descriptor */
#define RXBD_STAT_LS  (0x1 << 8)        /* last descriptor */
#define RXBD_STAT_TL  (0x1 << 7)        /* frame too long */
#define RXBD_STAT_CS  (0x1 << 6)        /* collision */
#define RXBD_STAT_FT  (0x1 << 5)        /* frame type */
#define RXBD_STAT_RE  (0x1 << 3)        /* mii error */
#define RXBD_STAT_DB  (0x1 << 2)        /* byte not aligned */
#define RXBD_STAT_CE  (0x1 << 1)        /* crc error */
#define RXBD_STAT_ZERO  (0x1)

#define RXBD_CTRL_RER (0x1 << 25)       /* receive end of ring */
#define RXBD_CTRL_RCH (0x1 << 24)       /* using second buffer, not used here */
#define RXBD_CTRL_RBS1(x) ((x) & 0x7FF) /* buffer1 size */

/*
 * tx bd status and control information
 */
#define TXBD_STAT_OWN (0x1 << 31)
#define TXBD_STAT_ES  (0x1 << 15)       /* error summary */
#define TXBD_STAT_LO  (0x1 << 11)       /* loss of carrier */
#define TXBD_STAT_NC  (0x1 << 10)       /* no carrier */
#define TXBD_STAT_LC  (0x1 << 9)        /* late collision */
#define TXBD_STAT_EC  (0x1 << 8)        /* excessive collision */
#define TXBD_STAT_CC(x)  (((x) >> 3) & 0xF)     /*  */
#define TXBD_STAT_UF  (0x1 << 1)        /* underflow error */
#define TXBD_STAT_DE  (0x1)     /* deferred */

#define TXBD_CTRL_IC   (0x1 << 31)      /* interrupt on completion */
#define TXBD_CTRL_LS   (0x1 << 30)      /* last descriptor */
#define TXBD_CTRL_FS   (0x1 << 29)      /* first descriptor */
#define TXBD_CTRL_FT1  (0x1 << 28)      /* filtering type  */
#define TXBD_CTRL_SET  (0x1 << 27)      /* setup packet */
#define TXBD_CTRL_AC   (0x1 << 26)      /* add crc disable */
#define TXBD_CTRL_TER  (0x1 << 25)      /* transmit end of ring */
#define TXBD_CTRL_TCH  (0x1 << 24)      /* second address chainded */
#define TXBD_CTRL_DPD  (0x1 << 23)      /* disabled padding */
#define TXBD_CTRL_FT0  (0x1 << 22)      /* filtering type, togethor with 28bit */
#define TXBD_CTRL_TBS2(x)  (((x) & 0x7FF) << 11)        /* buf2 size, no use here */
#define TXBD_CTRL_TBS1(x)  ((x) & 0x7FF)        /* buf1 size */
#define TXBD_CTRL_TBS1M (0x7FF)

/*
 * bus mode register
 */
#define EC_BMODE_DBO    (0x1 << 20)     /*descriptor byte ordering mode */
#define EC_BMODE_TAP(x) (((x) & 0x7) << 17)     /*transmit auto-polling */
#define EC_BMODE_PBL(x) (((x) & 0x3F) << 8)     /*programmable burst length */
#define EC_BMODE_BLE    (0x1 << 7)      /*big or little endian for data buffers */
#define EC_BMODE_DSL(x) (((x) & 0x1F) << 2)     /*descriptors skip length */
#define EC_BMODE_BAR    (0x1 << 1)      /*bus arbitration mode */
#define EC_BMODE_SWR    (0x1)   /*software reset */

/*
 * transmit and receive poll demand register
 */
#define EC_TXPOLL_ST  (0x1)     /* leave suspended mode to running mode to start xmit */
#define EC_RXPOLL_SR  (0x1)     /* leave suspended to running mode */

/*
 * status register 
 */
#define EC_STATUS_TSM  (0x7 << 20)      /*transmit process state */
#define EC_TX_run_dsp  (0x3 << 20)
#define EC_STATUS_RSM  (0x7 << 17)      /*receive process state */
#define EC_RX_fetch_dsp (0x1 <<17)
#define EC_RX_close_dsp (0x5 <<17)
#define EC_RX_run_dsp 	(0x7 <<17)
#define EC_STATUS_NIS  (0x1 << 16)      /*normal interrupt summary */
#define EC_STATUS_AIS  (0x1 << 15)      /*abnormal interrupt summary */
#define EC_STATUS_ERI  (0x1 << 14)      /*early receive interrupt */
#define EC_STATUS_GTE  (0x1 << 11)      /*general-purpose timer expiration */
#define EC_STATUS_ETI  (0x1 << 10)      /*early transmit interrupt */
#define EC_STATUS_RPS  (0x1 << 8)       /*receive process stopped */
#define EC_STATUS_RU   (0x1 << 7)       /*receive buffer unavailable */
#define EC_STATUS_RI   (0x1 << 6)       /*receive interrupt */
#define EC_STATUS_UNF  (0x1 << 5)       /*transmit underflow */
#define EC_STATUS_LCIS (0x1 << 4)       /* link change status */
#define EC_STATUS_LCIQ (0x1 << 3)       /* link change interrupt */
#define EC_STATUS_TU   (0x1 << 2)       /*transmit buffer unavailable */
#define EC_STATUS_TPS  (0x1 << 1)       /*transmit process stopped */
#define EC_STATUS_TI   (0x1)    /*transmit interrupt */

/*
 * operation mode register
 */
#define EC_OPMODE_RA  (0x1 << 30)       /*receive all */
#define EC_OPMODE_TTM (0x1 << 22)       /*transmit threshold mode */
#define EC_OPMODE_SF  (0x1 << 21)       /*store and forward */
#define EC_OPMODE_SPEED(x) (((x) & 0x3) << 16)  /*eth speed selection */
#define EC_OPMODE_10M (0x1 << 17)       /* set when work on 10M, otherwise 100M */
#define EC_OPMODE_TR(x)    (((x) & 0x3) << 14)  /*threshold control bits */
#define EC_OPMODE_ST  (0x1 << 13)       /*start or stop transmit command */
#define EC_OPMODE_LP  (0x1 << 10)       /*loopback mode */
#define EC_OPMODE_FD  (0x1 << 9)        /*full duplex mode */
#define EC_OPMODE_PM  (0x1 << 7)        /*pass all multicast */
#define EC_OPMODE_PR  (0x1 << 6)        /*prmiscuous mode */
#define EC_OPMODE_IF  (0x1 << 4)        /*inverse filtering */
#define EC_OPMODE_PB  (0x1 << 3)        /*pass bad frames */
#define EC_OPMODE_HO  (0x1 << 2)        /*hash only filtering mode */
#define EC_OPMODE_SR  (0x1 << 1)        /*start or stop receive command */
#define EC_OPMODE_HP  (0x1)     /*hash or perfect receive filtering mode */

/*
 * interrupt enable register
 */
#define EC_IEN_LCIE (0x1 << 17) /*link change interrupt enable */
#define EC_IEN_NIE (0x1 << 16)  /*normal interrupt summary enable */
#define EC_IEN_AIE (0x1 << 15)  /*abnormal interrupt summary enable */
#define EC_IEN_ERE (0x1 << 14)  /*early receive interrupt enable */
#define EC_IEN_GTE (0x1 << 11)  /*general-purpose timer overflow */
#define EC_IEN_ETE (0x1 << 10)  /*early transmit interrupt enable */
#define EC_IEN_RSE (0x1 << 8)   /*receive stopped enable */
#define EC_IEN_RUE (0x1 << 7)   /*receive buffer unavailable enable */
#define EC_IEN_RIE (0x1 << 6)   /*receive interrupt enable */
#define EC_IEN_UNE (0x1 << 5)   /*underflow interrupt enable */
#define EC_IEN_TUE (0x1 << 2)   /*transmit buffer unavailable enable */
#define EC_IEN_TSE (0x1 << 1)   /*transmit stopped enable */
#define EC_IEN_TIE (0x1)        /*transmit interrupt enable */
//#define EC_IEN_ALL (0x3CDE7)
#define EC_IEN_ALL (0x1CDE3)    /* TU interrupt disabled */

/*
 * missed frames and overflow counter register
 */
#define EC_MFOCNT_OCO  (0x1 << 28)      /*overflow flag */
#define EC_MFOCNT_FOCM (0x3FF << 17)    /*fifo overflow counter */
#define EC_MFOCNT_MFO  (0x1 << 16)      /*missed frame flag */
#define EC_MFOCNT_MFCM (0xFFFF) /*missed frame counter */

/*
 * the mii serial management register
 */
#define MII_MNG_SB  (0x1 << 31) /*start transfer or busy */
#define MII_MNG_CLKDIV(x) (((x) & 0x7) << 28)   /*clock divider */
#define MII_MNG_OPCODE(x) (((x) & 0x3) << 26)   /*operation mode */
#define MII_MNG_PHYADD(x) (((x) & 0x1F) << 21)  /*physical layer address */
#define MII_MNG_REGADD(x) (((x) & 0x1F) << 16)  /*register address */
#define MII_MNG_DATAM (0xFFFF)  /*register data mask */
#define MII_MNG_DATA(x)   ((MII_MNG_DATAM) & (x))       /* data to write */
#define MII_OP_WRITE 0x1
#define MII_OP_READ  0x2
#define MII_OP_CDS   0x3

/*
 * general purpose timer and interrupt mitigation control register
 */
#define EC_IMCTRL_CS     (0x1 << 31)    /*cycle size */
#define EC_IMCTRL_TT(x)  (((x) & 0xF) << 27)    /*transmit timer */
#define EC_IMCTRL_NTP(x) (((x) & 0x7) << 24)    /*number of transmit packets */
#define EC_IMCTRL_RT(x)  (((x) & 0xF) << 20)    /*receive timer */
#define EC_IMCTRL_NRP(x) (((x) & 0x7) << 17)    /*number of receive packets */
#define EC_IMCTRL_CON    (0x1 << 16)    /*continuous mode */
#define EC_IMCTRL_TIMM   (0xFFFF)       /*timer value */
#define EC_IMCTRL_TIM(x) ((x) & 0xFFFF) /*timer value */

/*
 * pause time and cache thresholds register
 */
#define EC_CACHETHR_CPTL(x) (((x) & 0xFF) << 24)        /*cache pause threshold level */
#define EC_CACHETHR_CRTL(x) (((x) & 0xFF) << 16)        /*cache restart threshold level */
#define EC_CACHETHR_PQT(x)  ((x) & 0xFFFF)      /*flow control pause quanta time */

/*
 * fifo thresholds register
 */
#define EC_FIFOTHR_FPTL(x) (((x) & 0xFFFF) << 16)       /*fifo pause threshold level */
#define EC_FIFOTHR_FRTL(x) ((x) & 0xFFFF)       /*fifo restart threshold level */

/*
 * flow control setup and status
 */
#define EC_FLOWCTRL_FCE (0x1 << 31)     /*flow control enable */
#define EC_FLOWCTRL_TUE (0x1 << 30)     /*transmit un-pause frames enable */
#define EC_FLOWCTRL_TPE (0x1 << 29)     /*transmit pause frames enable */
#define EC_FLOWCTRL_RPE (0x1 << 28)     /*receive pause frames enable */
#define EC_FLOWCTRL_BPE (0x1 << 27)     /*back pressure enable (only half-dup) */
#define EC_FLOWCTRL_ENALL (0x1F << 27)
#define EC_FLOWCTRL_PRS (0x1 << 1)      /*pause request sent */
#define EC_FLOWCTRL_HTP (0x1)   /*host transmission paused */

/*
 * mac control register
 */
#define EC_MACCTRL_RRSB (0x1 << 8)      /*RMII_REFCLK select bit */
#define EC_MACCTRL_SSDC(x) (((x) & 0xF) << 4) /*SMII SYNC delay half cycle */
#define EC_MACCTRL_RCPS (0x1 << 1)      /*REF_CLK phase select */
#define EC_MACCTRL_RSIS (0x1 << 0)      /*RMII or SMII interface select bit */

#define MAC_CTRL_SMII (0x41) /* use smii; bit8: 0 REFCLK output, 1 input*/
#define MAC_CTRL_RMII (0x0)  /* use rmii */


/*
 * phy control register
 */
#define EC_PHYCTRL_PDMC(x) (((x) & 0x3) << 16)  /*power down mode control */
#define EC_PHYCTRL_LOOP    (0x1 << 12)  /*loopback mode */
#define EC_PHYCTRL_MHZ(x)  (((x) & 0x3) << 4)   /* (x+1) times of 25MHZ */

/*
 * phy status register
 */
#define EC_PHYSTATUS_LINKED (0x1)       /*link status */

/*
 * phy reset control signals
 */
#define EC_PHYRESET_RST  (0x1)  /*low active reset signal */

/*
 * fxedp110hc0a - FARADAY Inc. PHY registers address
 * prefer to using macros in mii.h
 */
#define PHY_REG_FTC1 0x10
#define PHY_REG_FTC2 0x18

#define PHY_FTC_PRL  (0x1 << 12)        /* support full parallel detection function */
#define PHY_FTC_CMPT (0x3 << 2) /* prevent compatibility issues */


/*
 * phy KSZ8041TF
 */
#define MII_ICSR 0x19 /* interrupt control & status register */

#define ICSR_LINKUP_EN   (0x1 << 13)
#define ICSR_LINKDOWN_EN (0x1 << 12)
#define ICSR_LINKDOWN_EN_2 (0x1 << 11)

#define ICSR_LINKUP   (0x1 << 0)
#define ICSR_LINKDOWN (0x1 << 2)


#define MII_PHY_CTL2 0x1f
#define PHY_CTL2_INT_LEVEL (0x1 << 9) /* interrupt pin active high:1, low:0 */


/******************************************************************************/

#endif                          /* _EC_HARDWARE_H_ */
