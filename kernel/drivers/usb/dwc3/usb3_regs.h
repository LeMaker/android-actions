#ifndef  __USB3_REGS_H__
#define  __USB3_REGS_H__

//--------------USB3_Register-------------------------------------------//
//--------------Register Address---------------------------------------//
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


#define USB3_ACTIONS_START        (0xcd00)
#define USB3_ACTIONS_END          (0xcd58)

#define DWC3_CDR_KIKD          (0x00)
#define DWC3_CDR_KP1           (0x04)
#define DWC3_TIMER_INIT        (0x08)
#define DWC3_CDR_CONTROL       (0x0c)
#define DWC3_RX_OFFSET_PS      (0x10)
#define DWC3_EQ_CONTROL        (0x14)
#define DWC3_RX_OOBS_SSC0      (0x18)
#define DWC3_CMU_SSC1          (0x1C)
#define DWC3_CMU_DEBUG_LDO     (0x20)
#define DWC3_TX_AMP_DEBUG      (0x24)
#define DWC3_Z0                (0x28)
#define DWC3_DMR_BACR          (0x2C)
#define DWC3_IER_BCSR          (0x30)
#define DWC3_BPR               (0x34)
#define DWC3_BFNR              (0x38)
#define DWC3_BENR_REV          (0x3C)
#define DWC3_FLD               (0x40)
#define DWC3_CMU_PLL2_BISTDEBUG    (0x44)

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

//=============== USB3_P0_CTL define=======================
#define     USB3_P0_CTL_LDOVREFSEL_SHIFT_IC1                                      28
#define     USB3_P0_CTL_PLLLDOEN_IC1                                              31

#define     USB3_P0_CTL_LDOVREFSEL_E                                          30
#define     USB3_P0_CTL_LDOVREFSEL_SHIFT                                      29
#define     USB3_P0_CTL_LDOVREFSEL_MASK                                       (0x3<<29)
#define     USB3_P0_CTL_PLLLDOEN                                              28
#define     USB3_P0_CTL_PLUGIN                                                25
#define     USB3_P0_CTL_CONDET_EN                                             24
#define     USB3_P0_CTL_WKUPBVLDEN                                            23
#define     USB3_P0_CTL_WKUPDPEN                                              22
#define     USB3_P0_CTL_WKUPIDUSEN                                            21
#define     USB3_P0_CTL_WKUPVBUSEN                                            20
#define     USB3_P0_CTL_WKUPBVLD                                              19
#define     USB3_P0_CTL_WKUPDP                                                18
#define     USB3_P0_CTL_WKUPID                                                17
#define     USB3_P0_CTL_WKUPVBUS                                              16
#define     USB3_P0_CTL_DMPUEN_P0                                             15
#define     USB3_P0_CTL_DPPUEN_P0                                             14
#define     USB3_P0_CTL_DMPDDIS_P0                                            13
#define     USB3_P0_CTL_DPPDDIS_P0                                            12
#define     USB3_P0_CTL_ID_P0                                                 11
#define     USB3_P0_CTL_BVALID_P0                                             10
#define     USB3_P0_CTL_SOFTID_P0                                             9
#define     USB3_P0_CTL_SOFTIDEN_P0                                           8
#define     USB3_P0_CTL_SOFTVBUS_P0                                           7
#define     USB3_P0_CTL_SOFTVBUSEN_P0                                         6
#define     USB3_P0_CTL_VBUS_P0                                               5
#define     USB3_P0_CTL_LS_P0_E                                               4
#define     USB3_P0_CTL_LS_P0_SHIFT                                           3
#define     USB3_P0_CTL_LS_P0_MASK                                            (0x3<<3)
#define     USB3_P0_CTL_VBUSTH_P0_E                                           2
#define     USB3_P0_CTL_VBUSTH_P0_SHIFT                                       0
#define     USB3_P0_CTL_VBUSTH_P0_MASK                                        (0x7<<0)


#endif
