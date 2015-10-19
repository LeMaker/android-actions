/*
 * for Actions AOTG 
 *
 */

#ifndef  __AOTG_REGS_H__
#define  __AOTG_REGS_H__


#define USB3_REGISTER_BASE          0xB0400000

#define DWC3_DCFG                   0xc700


#define DWC3_ACTIONS_REGS_START     (0xcd00)
#define DWC3_ACTIONS_REGS_END       (0xcd54)





#define DWC3_CDR_KIKD          (0xcd00)
#define DWC3_CDR_KP1           (0xcd04)
#define DWC3_TIMER_INIT        (0xcd08)
#define DWC3_CDR_CONTROL       (0xcd0c)
#define DWC3_RX_OFFSET_PS      (0xcd10)
#define DWC3_EQ_CONTROL        (0xcd14)
#define DWC3_RX_OOBS_SSC0      (0xcd18)
#define DWC3_CMU_SSC1          (0xcd1C)
#define DWC3_CMU_DEBUG_LDO     (0xcd20)
#define DWC3_TX_AMP_DEBUG      (0xcd24)
#define DWC3_Z0                (0xcd28)
#define DWC3_DMR_BACR          (0xcd2C)
#define DWC3_IER_BCSR          (0xcd30)
#define DWC3_BPR               (0xcd34)
#define DWC3_BFNR              (0xcd38)
#define DWC3_BENR_REV          (0xcd3C)
#define DWC3_FLD               (0xcd40)
#define DWC3_CMU_PLL2_BISTDEBUG    (0xcd44)
#define DWC3_USB2_P0_VDCTL     (0xcd48)
#define DWC3_BACKDOOR          (0xcd4C)
#define DWC3_EXT_CTL           (0xcd50)
#define DWC3_EFUSE_CTR         (0xcd54)
#define DWC3_GCTL               (0xc110)


#define USB3_MOD_RST           (1 << 14)
#define CMU_BIAS_EN            (1 << 20)


#define BIST_QINIT(n)          ((n) << 24)
#define EYE_HEIGHT(n)          ((n) << 20)
#define PLL2_LOCK              (1 << 15)
#define PLL2_RS(n)             ((n) << 12)
#define PLL2_ICP(n)            ((n) << 10)
#define CMU_SEL_PREDIV         (1 << 9)
#define CMU_DIVX2              (1 << 8)
#define PLL2_DIV(n)            ((n) << 3)
#define PLL2_POSTDIV(n)        ((n) << 1)
#define PLL2_PU                (1 << 0)

#define DWC3_GCTL_CORESOFTRESET		(1 << 11)


#define DWC3_GCTL_PRTCAP(n)       (((n) & (3 << 12)) >> 12)
#define DWC3_GCTL_PRTCAPDIR(n)    ((n) << 12)
#define DWC3_GCTL_PRTCAP_HOST     1
#define DWC3_GCTL_PRTCAP_DEVICE   2
#define DWC3_GCTL_PRTCAP_OTG	    3

#define HOST_DETECT_STEPS          5
#define DEVICE_DETECT_STEPS        5

#define HOST_CONFIRM_STEPS         3
#define DEVICE_CONFIRM_STEPS       4


#define USB3_P0_CTL_VBUS_P0       5
#define USB3_P0_CTL_ID_P0         11
#define USB3_P0_CTL_DPPUEN_P0     14
#define USB3_P0_CTL_DMPUEN_P0     15
#define USB3_P0_CTL_DPPDDIS_P0    12
#define USB3_P0_CTL_DMPDDIS_P0    13
#define USB3_P0_CTL_SOFTIDEN_P0   8
#define USB3_P0_CTL_SOFTID_P0     9
#define USB3_P0_CTL_SOFTVBUSEN_P0 6
#define USB3_P0_CTL_SOFTVBUS_P0   7
#define USB3_P0_CTL_PLLLDOEN      28
#define USB3_P0_CTL_LDOVREFSEL_SHIFT  29

#define     USB3_P0_CTL_LDOVREFSEL_SHIFT_IC1                                      28
#define     USB3_P0_CTL_PLLLDOEN_IC1                                              31

#define USB3_P0_CTL_LS_P0_SHIFT   3
#define USB3_P0_CTL_LS_P0_MASK    (0x3<<3)


#define VBUS_THRESHOLD		3000 

enum{
   IC_ATM7039C = 0,
   IC_ATM7059A
};
#define ANA00 (0+0xCD00)
#define ANA01 (0+0xCD04)
#define ANA02 (0+0xCD08)
#define ANA03 (0+0xCD0C)
#define ANA04 (0+0xCD10)

#define ANA05 (0+0xCD14)
#define ANA06 (0+0xCD18)
#define ANA07 (0+0xCD1C)
#define ANA08 (0+0xCD20)
#define ANA09 (0+0xCD24)
#define ANA0A (0+0xCD28)
#define ANA0B (0+0xCD2C)
#define ANA0C (0+0xCD30)
#define ANA0D (0+0xCD34)
#define ANA0E (0+0xCD38)
#define ANA0F (0+0xCD3C)
#define DMR (0+0xCD40)
#define BACR (0+0xCD44)
#define IER (0+0xCD48)
#define BCSR (0+0xCD4C)
#define BPR (0+0xCD50)
#define BPNR2 (0+0xCD54)
#define BFNR (0+0xCD58)
#define BRNR2 (0+0xCD5C)
#define BENR (0+0xCD60)
#define REV0 (0+0xCD64)
#define REV1 (0+0xCD68)
#define REV2 (0+0xCD6C)
#define REV3 (0+0xCD70)
#define FLD0 (0+0xCD74)
#define FLD1 (0+0xCD78)
#define ANA1F (0+0xCD7C)
#define PAGE1_REG00 (0+0xCD80)
#define PAGE1_REG01 (0+0xCD84)
#define PAGE1_REG02 (0+0xCD88)
#define PAGE1_REG03 (0+0xCD8C)
#define PAGE1_REG04 (0+0xCD90)
#define PAGE1_REG05 (0+0xCD94)
#define PAGE1_REG06 (0+0xCD98)
#define PAGE1_REG07 (0+0xCD9C)
#define PAGE1_REG08 (0+0xCDA0)
#define PAGE1_REG09 (0+0xCDA4)
#define PAGE1_REG0A (0+0xCDA8)
#define PAGE1_REG0B (0+0xCDAC)
#define PAGE1_REG0C (0+0xCDB0)
#define PAGE1_REG0D (0+0xCDB4)
#define PAGE1_REG0E (0+0xCDB8)
#define PAGE1_REG0F (0+0xCDBC)
#define PAGE1_REG10 (0+0xCDC0)
#define USB2_P0_VDCTRL (0+0xCE00)
#define BACKDOOR (0+0xCE04)
#define EXT_CONTROL (0+0xCE08)
#define EFUSE_CTR (0+0xCE0C)
#define USB2_P1_VDCTRL (0+0xCE10) 

#endif  /* __AOTG_REGS_H__ */
 

