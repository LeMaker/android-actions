
/*
 * Ethenet pin control
 */
#define RMII_TXD01_MFP_CTL0	(0x0 << 16)
#define RMII_RXD01_MFP_CTL0	(0x0 << 8)
#define RMII_TXEN_TXER_MFP_CTL0	(0x0 << 13)
#define RMII_REF_CLK_MFP_CTL0	(0x0 << 6)
#define CLKO_25M_EN_MFP_CTL3 (0x1<<30)

#define ASOC_ETHERNET_PHY_ADDR (0x1)

#define MULTICAST_LIST_LEN 14
#define ETH_MAC_LEN 6
#define ETH_CRC_LEN 4

#define RX_RING_SIZE 1 //PKTBUFSRX
#define RX_RING_MOD_MASK  (RX_RING_SIZE - 1)
#define TX_RING_SIZE 1
#define TX_RING_MOD_MASK (TX_RING_SIZE - 1)

#define ETH_SPEED_10M 10
#define ETH_SPEED_100M 100
#define ETH_DUP_HALF 1
#define ETH_DUP_FULL 2
#define ETH_PKG_MIN 64
#define ETH_PKG_MAX 1518
#define PKG_RESERVE 18
#define PKG_MIN_LEN (ETH_PKG_MIN)
#define PKG_MAX_LEN (ETH_PKG_MAX + PKG_RESERVE)
#define ETH_LINK_UP 1
#define ETH_LINK_DOWN 0

#define REG_CLEAR 0x0

/*
 * ethernet clock enable
 */
#define ASOC_ETH_CLOCK_EN  (0x1 << 22)
#define ASOC_ETH_CLOCK_RST (0x1 << 20)
#define CMU_ETHERNETPLL_ENABLE (0x3)
#define CMU_ETHERNETPLL_RMII_REF_CLK_DIV (0x1 << 2)

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
#define EC_IEN_ALL (0x1CDE3)    /* TU interrupt disabled */

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
 * mac control register
 */
#define EC_MACCTRL_RRSB (0x1 << 8)      /*RMII_REFCLK select bit */
#define EC_MACCTRL_SSDC(x) (((x) & 0xF) << 4) /*SMII SYNC delay half cycle */
#define EC_MACCTRL_RCPS (0x1 << 1)      /*REF_CLK phase select */
#define EC_MACCTRL_RSIS (0x1 << 0)      /*RMII or SMII interface select bit */
#define MAC_CTRL_RMII (0x0)  /* use rmii */

/*
 * receive and transmit buffer descriptor
 */
struct buffer_descriptor {
	unsigned long status;
	unsigned long control;
	unsigned long buf_addr;
	unsigned long reserved;     /* we don't use second buffer address */
}__aligned(ARCH_DMA_MINALIGN);

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


#define RX_ERROR_CARED \
	(RXBD_STAT_DE | RXBD_STAT_RF | RXBD_STAT_TL | RXBD_STAT_CS \
	| RXBD_STAT_DB | RXBD_STAT_CE | RXBD_STAT_ZERO)

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

