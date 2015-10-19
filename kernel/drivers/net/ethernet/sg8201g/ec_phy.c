/******************************************************************************
ec_phy.c -- phy driver for fxedp110c0a  of FARADAY Inc.

author : yunchf @Actions
date : 2010-04-12
version 0.1

date : 2010-08-10
version 1.0

******************************************************************************/

#include <linux/delay.h>
#include "ethctrl.h"

/*
 * read and write phy register normally need not to wait for. sometimes about 7us
 * but auto-negotiation may wait for a long time, about 140ms, and less than 2s
 */
#define MII_TIME_OUT 100
#define PHY_AUTONEG_TIME_OUT 50


#if 0
/*------------------------------- phy driver hooks -----------------------------*/

/**
 * read_phy_reg - MII interface  to read  @reg_addr register of phy at @phy_addr
 * return positive and zero value if success, or negative value if fail
 */
 //read PHY internal register by SPI
//register address,return the register data
unsigned short read_phy_reg_atc2605(ec_priv_t * ecp, unsigned short reg_addr)
{
	unsigned short temp;
	struct atc260x_dev *atc260x = ecp->atc260x;

	do {
		temp = atc260x_reg_read(atc260x, atc2603_PHY_SMI_STAT);
	}while(temp & 0x1); 	//waite for the SMI bus process completion

	temp = 0x8100 | reg_addr;		//smi write operate,address
	atc260x_reg_write(atc260x, atc2603_PHY_SMI_CONFIG, temp);
	do {
		temp = atc260x_reg_read(atc260x, atc2603_PHY_SMI_STAT);
	}while(temp & 0x1); 	//waite for the SMI bus process completion

	temp = atc260x_reg_read(atc260x, atc2603_PHY_SMI_DATA);
	temp = temp & 0xffff;
	return (temp);
}

/**
 * write_phy_reg - MII interface  to write  @val to @reg_addr register of phy at @phy_addr
 * return zero if success, negative value if fail
 */
 //write PHY internal register by SPI
//register address,write data
int write_phy_reg_atc2605(ec_priv_t * ecp, unsigned short reg_addr, unsigned short val)
{
    unsigned short temp;
    struct atc260x_dev *atc260x = ecp->atc260x;

	do {
		temp = atc260x_reg_read(atc260x, atc2603_PHY_SMI_STAT);
	}while(temp & 0x1); 	//waite for the SMI bus process completion

	atc260x_reg_write(atc260x, atc2603_PHY_SMI_DATA, val);
	temp = 0xc100 | reg_addr;		//smi write operate,address
	atc260x_reg_write(atc260x, atc2603_PHY_SMI_CONFIG, temp);

	return 0;
}
#endif //PGA0
/**
 * read_phy_reg - MII interface  to read  @reg_addr register of phy at @phy_addr
 * return positive and zero value if success, or negative value if fail
 * may be used by other standard ethernet phy
 */
unsigned short read_phy_reg_rtl8201(ec_priv_t * ecp, unsigned short reg_addr)
{
	u32 op_reg;
	u32 phy_addr;
  if((ecp->phy_addr)!=0xFF)
  	phy_addr=ecp->phy_addr;
  else
  	phy_addr=ASOC_ETHERNET_PHY_ADDR;	
	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	putl(MII_MNG_SB | MII_MNG_OPCODE(MII_OP_READ) | MII_MNG_REGADD(reg_addr) |
		MII_MNG_PHYADD(phy_addr), MAC_CSR10);

	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	return (u16)MII_MNG_DATA(op_reg);
}

/**
 * read_phy_reg - MII interface  to read  @reg_addr register of phy at @phy_addr
 * return positive and zero value if success, or negative value if fail
 * may be used by other standard ethernet phy
 */
unsigned short read_phy_reg_rs8201g(ec_priv_t * ecp, unsigned short reg_addr)
{
	u32 op_reg;
	u32 phy_addr;
  if((ecp->phy_addr)!=0xFF)
  	phy_addr=ecp->phy_addr;
  else
  	phy_addr=ASOC_ETHERNET_PHY_ADDR;	
	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	putl(MII_MNG_SB | MII_MNG_OPCODE(MII_OP_READ) | MII_MNG_REGADD(reg_addr) |
		MII_MNG_PHYADD(phy_addr), MAC_CSR10);

	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	return (u16)MII_MNG_DATA(op_reg);
}








/**
 * write_phy_reg - MII interface  to write  @val to @reg_addr register of phy at @phy_addr
 * return zero if success, negative value if fail
 * may be used by other standard ethernet phy
 */
int write_phy_reg_rtl8201(ec_priv_t * ecp, unsigned short reg_addr, unsigned short val)
{
	u32 op_reg;
	u32 phy_addr;
  if((ecp->phy_addr)!=0xFF)
  	phy_addr=ecp->phy_addr;
  else
  	phy_addr=ASOC_ETHERNET_PHY_ADDR;
  	
	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	putl(MII_MNG_SB | MII_MNG_OPCODE(MII_OP_WRITE) | MII_MNG_REGADD(reg_addr) |
		MII_MNG_PHYADD(phy_addr) | val, MAC_CSR10);

	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	return 0;
}




/**
 * write_phy_reg - MII interface  to write  @val to @reg_addr register of phy at @phy_addr
 * return zero if success, negative value if fail
 * may be used by other standard ethernet phy
 */
int write_phy_reg_rs8201g(ec_priv_t * ecp, unsigned short reg_addr, unsigned short val)
{
	u32 op_reg;
	u32 phy_addr;
  if((ecp->phy_addr)!=0xFF)
  	phy_addr=ecp->phy_addr;
  else
  	phy_addr=ASOC_ETHERNET_PHY_ADDR;	
	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	putl(MII_MNG_SB | MII_MNG_OPCODE(MII_OP_WRITE) | MII_MNG_REGADD(reg_addr) |
		MII_MNG_PHYADD(phy_addr) | val, MAC_CSR10);

	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	return 0;
}






/**
 * read_phy_reg - MII interface  to read  @reg_addr register of phy at @phy_addr
 * return positive and zero value if success, or negative value if fail
 * may be used by other standard ethernet phy
 */
unsigned short read_phy_reg_ksz8041(ec_priv_t * ecp, unsigned short reg_addr)
{
	u32 op_reg;
	u32 phy_addr;
  if((ecp->phy_addr)!=0xFF)
  	phy_addr=ecp->phy_addr;
  else
  	phy_addr=ASOC_ETHERNET_PHY_ADDR;
	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	putl(MII_MNG_SB | MII_MNG_OPCODE(MII_OP_READ) | MII_MNG_REGADD(reg_addr) |
		MII_MNG_PHYADD(phy_addr), MAC_CSR10);

	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	return (u16)MII_MNG_DATA(op_reg);
}


/**
 * write_phy_reg - MII interface  to write  @val to @reg_addr register of phy at @phy_addr
 * return zero if success, negative value if fail
 * may be used by other standard ethernet phy
 */
int write_phy_reg_ksz8041(ec_priv_t * ecp, unsigned short reg_addr, unsigned short val)
{
	u32 op_reg;
	u32 phy_addr;
  if((ecp->phy_addr)!=0xFF)
  	phy_addr=ecp->phy_addr;
  else
  	phy_addr=ASOC_ETHERNET_PHY_ADDR;
	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	putl(MII_MNG_SB | MII_MNG_OPCODE(MII_OP_WRITE) | MII_MNG_REGADD(reg_addr) |
		MII_MNG_PHYADD(phy_addr) | val, MAC_CSR10);

	do {
		op_reg = getl(MAC_CSR10);
	} while (op_reg & MII_MNG_SB);

	return 0;
}

unsigned short (*read_phy_reg)(ec_priv_t *, unsigned short) = NULL;
int (*write_phy_reg)(ec_priv_t *, unsigned short, unsigned short) = NULL;

static inline void phy_reg_set_bits(ec_priv_t * ecp, unsigned short reg_addr, int bits)
{
	unsigned short reg_val;
	reg_val = read_phy_reg(ecp, reg_addr);
	reg_val |= (unsigned short)bits;
	printk(KERN_INFO " %s %d write reg_val:0x%x bits:0x%x\n", __FUNCTION__,__LINE__,reg_val,bits);
	write_phy_reg(ecp, reg_addr, reg_val);
	reg_val = read_phy_reg(ecp, reg_addr);
	printk(KERN_INFO " %s %d write after read reg_val:0x%x\n", __FUNCTION__,__LINE__,reg_val);
}

static inline void phy_reg_clear_bits(ec_priv_t * ecp, unsigned short reg_addr, int bits)
{
	unsigned short reg_val;

	reg_val = read_phy_reg(ecp, reg_addr);
	reg_val &= ~(unsigned short)bits;
	write_phy_reg(ecp, reg_addr, reg_val);
}


extern int suspend_flag;
static inline int wait_for_aneg_completion(ec_priv_t * ecp)
{
    int     wait;
    int     reg_val;
    for (wait = PHY_AUTONEG_TIME_OUT; wait > 0; wait--) {
        if (suspend_flag == 1) {
            suspend_flag = 0;
            break;
        }
        reg_val = read_phy_reg(ecp, MII_BMSR);
        if (reg_val & BMSR_ANEGCOMPLETE)
            break;

        mdelay(100);
        printk(KERN_INFO "%s", (wait % 200) ? "*" : "\n");
    }
    printk(KERN_INFO "exit\n");

    return (!wait);
}
#if 0

static int atc2605_hw_init(ec_priv_t *ecp)
{
	unsigned short atc260x_temp;
	struct atc260x_dev *atc260x = ecp->atc260x;

	/*ethernet mfp0 RMII 1st*/
	//hw_rmii_get_pin();
	//mac_rmii_get_pin();
	/************************phy init*****************************/
	/*enable phy clock*/
	atc260x_reg_setbits(atc260x, atc2603_CMU_DEVRST, 0x200, 0x200);
	udelay(100);

	/*ethernet wrapper rest*/
	atc260x_reg_setbits(atc260x, atc2603_CMU_DEVRST, 0x08, 0x0);
	udelay(100);
	atc260x_reg_setbits(atc260x, atc2603_CMU_DEVRST, 0x08, 0x08);
	udelay(100);

	/*GL5302 PHY avoid reboot bug, reset PLL*/
	atc260x_reg_setbits(atc260x, atc2603_PHY_PLL_CTL0, 0x03, 0x02); //25Mhz
	mdelay(5);
	atc260x_reg_setbits(atc260x, atc2603_PHY_PLL_CTL1, 0x200, 0x200);//bit9=1,enetpll ALV LDO enable
	mdelay(200);

	atc260x_reg_setbits(atc260x, atc2603_PHY_PLL_CTL0, 0x03, 0x01); //bit0=1,bit1=0,ethernet pll enable ,24Mhz
	mdelay(1);
	/* 5203 PAD_CLKO_25M output 25MHz clock to 2605 */
	atc260x_reg_setbits(atc260x, atc2603_PHY_PLL_CTL0, 0x03, 0x03); //25Mhz
	mdelay(5);

	atc260x_reg_write(atc260x, atc2603_PHY_CONFIG, 0x00);	    //auto, phy power on
	atc260x_reg_write(atc260x, atc2603_PHY_ADDR, 0x01);		    //set phy address=  0x01
	atc260x_reg_write(atc260x, atc2603_PHY_CTRL, 0x01);	        //rmii,100Mbps

	//add  modify
	atc260x_reg_write(atc260x, atc2603_PHY_HW_RST, 0x01);	    //reset the phy
	udelay(100);
	do
	{
		atc260x_temp = atc260x_reg_read(atc260x, atc2603_PHY_HW_RST);
	}while(atc260x_temp & 0x1); //waite for reset process completion

	/*enable RMII_REF_CLK and EXTIRQ pad and disable the other 6 RMII pad, gl5302 bug amend*/
	atc260x_reg_setbits(atc260x, atc2603_PAD_EN, 0xff, 0x81);
	udelay(100);

	//pad_drv level 3 for fpga test
	atc260x_reg_setbits(atc260x, atc2603_PAD_DRV1, 0xfff, 0x145);
	//atc260x_reg_setbits(atc260x, atc2603_PAD_DRV1, 0xfff, 0x28a); //level 4
	printk(KERN_INFO "5302 PAD_EN: 0x%x\n", (uint)atc260x_reg_read(atc260x, atc2603_PAD_EN));
	printk(KERN_INFO "5302 PAD_DRV1: 0x%x\n", (uint)atc260x_reg_read(atc260x, atc2603_PAD_DRV1));

	atc260x_reg_setbits(atc260x, atc2603_PHY_INT_CTRL, 0xf, 0xf);   //enable phy int control

	/*clear phy INT_STAT*/
	atc260x_reg_write(ecp->atc260x, atc2603_PHY_INT_STAT, 0xff);
	//printk(KERN_INFO "1f.atc2603_PHY_INT_STAT:0x%x\n", atc260x_reg_read(atc260x, gl5302_PHY_INT_STAT));
	udelay(100);

	printk(KERN_INFO "5302 MFP_CTL0: 0x%x\n", (uint)atc260x_reg_read(atc260x, atc2603_MFP_CTL0));

#ifdef ETHENRET_PHY_LOOP_BACK
	/* phy loopback */
	//atc260x_reg_setbits(atc260x, atc2603_PHY_CONFIG, 0x1 << 11, 0x1 << 11);
	printk(KERN_INFO "phy config: 0x%x\n", (uint)atc260x_reg_read(atc260x, atc2603_PHY_CONFIG));
	{
		/* shut auto-negoiation */
		ushort bmcr = read_phy_reg(ecp, MII_BMCR);
		bmcr &= ~BMCR_ANENABLE;
		write_phy_reg(ecp, MII_BMCR, bmcr);

		bmcr = read_phy_reg(ecp, MII_BMCR);
		bmcr |= BMCR_LOOPBACK;
		write_phy_reg(ecp, MII_BMCR, bmcr);
		printk(KERN_INFO "phy bmcr: 0x%x\n", (uint)read_phy_reg(ecp, MII_BMCR));
	}
#endif
	return 0;
}

//rtl8201_phy_init

#endif //PGA0



/**
 * phy_init -- initialize phy - ATC2605 or KSZ8041
 * return 0 if success, else fail
 */
static int phy_init(ec_priv_t * ecp)
{
	int     reg_val;
	//u16 temp;
		unsigned int cnt = 0;	
	phy_reg_set_bits(ecp, MII_BMCR, BMCR_RESET);
	do {
		reg_val = read_phy_reg(ecp, MII_BMCR);
		if (cnt++ > 1000) {
			printk(KERN_INFO "ethernet phy BMCR_RESET timeout!!!\n");
			break;
		}
	} while (reg_val & BMCR_RESET);

	if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
		printk(KERN_INFO "phy model: %u\n", ecp->phy_model);
		phy_reg_set_bits(ecp, PHY_REG_FTC1, PHY_FTC_PRL);
		phy_reg_set_bits(ecp, PHY_REG_FTC2, PHY_FTC_CMPT);
	} else if (ecp->phy_model == ETH_PHY_MODEL_KSZ8041TL) {
		printk(KERN_INFO "phy model: %u\n", ecp->phy_model);
		/* only turn on link up/down phy interrupt */
		write_phy_reg(ecp, MII_ICSR, ICSR_LINKUP_EN | ICSR_LINKDOWN_EN);
		printk(KERN_INFO "MII_ICSR:0x%x\n", (unsigned)read_phy_reg(ecp, MII_ICSR));
		phy_reg_set_bits(ecp, MII_PHY_CTL2, PHY_CTL2_INT_LEVEL);
		printk(KERN_INFO "MII_PHY_CTL2:0x%x\n", (unsigned)read_phy_reg(ecp, MII_PHY_CTL2));
	} else if (ecp->phy_model == ETH_PHY_MODEL_RTL8201) {
		printk(KERN_INFO "phy model: %u\n", ecp->phy_model);
		write_phy_reg(ecp, 0x1f, 0x7);
		/* only turn on link up/down phy interrupt */
		write_phy_reg(ecp, 0x13, ICSR_LINKUP_EN );
		write_phy_reg(ecp, 0x10, 0x7FFB);
		write_phy_reg(ecp, 0x1f, 0x0);
	} else if (ecp->phy_model == ETH_PHY_MODEL_SR8201G) {
#ifdef PHY_USE_POLL	
		write_phy_reg(ecp, 0xf, 0x7);
		write_phy_reg(ecp, 0xC, 0X1<<12);
		write_phy_reg(ecp, 0xf, 0x0);
#else
		printk(KERN_INFO "phy model: %u\n", ecp->phy_model);
		write_phy_reg(ecp, 0xf, 0x7);
		/* only turn on link up/down phy interrupt */
		write_phy_reg(ecp, 0xE, (0X1<<11) | (1 << 10) );
		write_phy_reg(ecp, 0xC, 0X1<<12);
		write_phy_reg(ecp, 0xf, 0x0);
#endif		
	} else {
		printk(KERN_INFO "NOT supported phy model: %u\n", ecp->phy_model);
	}
	
#if 0
	/* adjust tx current */
	temp = read_phy_reg(ecp, 0x12);
	temp  &= ~0x780;
	//temp	|= 0x600; //1100, add 50+100uA
	//temp	|= 0x680; //1101
	//temp  |= 0x480; //1001
	//temp	|= 0x280; //0101
	//temp	|= 0x80; //0001
	temp	|= 0x180; //0011, minus 50uA  max 2.57V
	//temp	|= 0x780; //1111
	write_phy_reg(ecp, 0x12, temp);
	printk(KERN_INFO "PHY_REG_TXPN = 0x%x\n", (u32)read_phy_reg(ecp, 0x12));
#endif
#ifdef WORK_IN_10M_MODE
	/* limit to 10M for 5203 MAC bug */
	phy_reg_clear_bits(ecp, MII_ADVERTISE, ADVERTISE_100HALF | ADVERTISE_100FULL);
#endif
	printk(KERN_INFO "MII_ADVERTISE: 0x%04x\n", (uint)read_phy_reg(ecp, MII_ADVERTISE));
	printk(KERN_INFO "%s %d MII_BMCR: 0x%04x\n", __FUNCTION__,__LINE__,(uint)read_phy_reg(ecp, MII_BMCR));

	/* auto-negotiate and wait for completion, then get link status */
	phy_reg_set_bits(ecp, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
	printk(KERN_INFO "start aneg...\n");
	printk(KERN_INFO "%s %d MII_BMCR: 0x%04x\n",  __FUNCTION__,__LINE__,(uint)read_phy_reg(ecp, MII_BMCR));

	/* wait_for_aneg_completion(ecp) sleep(), so it shall not be invoked in
	 * ec_netdev_transmit_timeout() which runs in interrupt bottom half. */
#if 0
	printk(KERN_INFO "wait for aneg...\n");
	if (wait_for_aneg_completion(ecp)) {
		printk(KERN_INFO "MII_BMCR:0x%x\n", read_phy_reg(ecp, MII_BMCR));
		printk(KERN_INFO "auto-negotiation is timeout.\n");
		return (1);
	}
   int i = 0 ;
	for (i = 0; i < MII_TIME_OUT; i++) {
		reg_val = read_phy_reg(ecp, MII_BMSR);
		if (reg_val & BMSR_LSTATUS) {
			ecp->linked = true;
			break;
		}
		udelay(1);
	}
	if (MII_TIME_OUT == i) {
		ecp->linked = false;
		printk(KERN_INFO "link fail.\n");
		return (1);
	}
#else
	/* not wait, set link status not connected and return*/
	ecp->linked = false;
#endif

	INFO_GREEN("\nlink status is: %s\n", ecp->linked ? "true" : "false");
	return (0);
}


/**
 * fdp110_setup_aneg - setup  or disable  auto-negotiation;
 * if enable auto-neg, we enable and restart auto-neg, don't wait for it completion
 * link change interrupt can capture it
 * if disable auto-neg, but auto-neg is already disabled, then nothing to do, 
 * else disable auto-neg
 * return 0 if success, negative value if fail
 */
static int fdp110_setup_aneg(ec_priv_t * ecp, bool autoneg)
{
    int     bmcr;
    int     err;

    bmcr = read_phy_reg(ecp, MII_BMCR);
    if (bmcr < 0) {
        return (bmcr);
    }

    if (autoneg) {
        bmcr |= BMCR_ANENABLE | BMCR_ANRESTART;
        err = write_phy_reg(ecp, MII_BMCR, bmcr);
        if (err < 0)
            return (err);
    } else if (bmcr & BMCR_ANENABLE) {
        printk(KERN_INFO "disable auto-neg\n");
        bmcr &= ~BMCR_ANENABLE;
        err = write_phy_reg(ecp, MII_BMCR, bmcr);
        if (err < 0)
            return (err);
    }

    return (0);
}


static inline int restart_autoneg(ec_priv_t * ecp)
{
    int     bmcr_val;

    bmcr_val = read_phy_reg(ecp, MII_BMCR);
    if (bmcr_val < 0) {
        return (bmcr_val);
    }

    bmcr_val |= BMCR_ANENABLE | BMCR_ANRESTART;
    return (write_phy_reg(ecp, MII_BMCR, bmcr_val));
}


/**
 * fdp110_setup_advert -- set auto-negotiation advertisement
 * return positive value wrote to ANAR reg if success, negative value if fail 
 */
static int fdp110_setup_advert(ec_priv_t * ecp, int advertising)
{
    int     adv;
    int     err;
    const int supported = ADVERTISE_ALL | ADVERTISE_PAUSE_CAP;

    adv = read_phy_reg(ecp, MII_ADVERTISE);
    if (adv < 0) {
        return (adv);
    }

    /* remove old supported features, but maintain others bit */
    adv &= ~supported;
    if (!(advertising & supported)) {
        return (-1);
    }
    adv |= (advertising & supported);
    err = write_phy_reg(ecp, MII_ADVERTISE, adv);
    if (err < 0) {
        return (err);
    }

    /* in fact, when we set new value to phy's advertisement reg, phy will auto-neg again,
     * but some times it don't, so we manually force it auto-neg again.
     * we don't wait for auto-neg completion, link change interrupt will capture it
     */
    err = restart_autoneg(ecp);

    return (err < 0 ? err : adv);
}


/**
 * fdp110_setup_forced -- configure phy work on @speed and @duplex mode forciblly
 * NOTE: this will close auto-neg function
 * return 0 if success, negative value if fail
 */
static int fdp110_setup_forced(ec_priv_t * ecp, int speed, int duplex)
{
    int     err = 0;
    int     eval;
    int     bmcr;

    bmcr = read_phy_reg(ecp, MII_BMCR);
    if (bmcr < 0) {
        printk(KERN_INFO "error read bmcr\n");
        return (-1);
    }

    eval = bmcr & ~(BMCR_ANENABLE | BMCR_SPEED100 | BMCR_FULLDPLX);
    if (ETH_SPEED_100M == speed) {
        eval |= BMCR_SPEED100;
    }
    if (ETH_DUP_FULL == duplex) {
        eval |= BMCR_FULLDPLX;
    }
    if (eval != bmcr) {
        err = write_phy_reg(ecp, MII_BMCR, eval);
    }

    return (err);
}


/**
 * fdp110_setup_loopback -- setup or disable loopback
 * return 0 if success, negative value if fail
 */
static int fdp110_setup_loopback(ec_priv_t * ecp, bool loopback)
{
    int     bmcr;
    int     err = 0;
    bool    changed = false;

    bmcr = read_phy_reg(ecp, MII_BMCR);
    if (bmcr < 0) {
        return (bmcr);
    }

    if (loopback) {
        if (!(bmcr & BMCR_LOOPBACK)) {
            bmcr |= BMCR_LOOPBACK;
            changed = true;
        }
    } else {
        if (bmcr & BMCR_LOOPBACK) {
            bmcr &= ~BMCR_LOOPBACK;
            changed = true;
        }
    }

    if (changed) {
        err = write_phy_reg(ecp, MII_BMCR, bmcr);
    }
    printk(KERN_INFO "changed - %s \n", changed ? "true" : "false");

    return (err);
}


/**
 * fdp110_read_status -- read phy's status, according phy ancr, anar & anlpar regs
 * return 0 if success, negative value if fail
 */
static int fdp110_read_status(ec_priv_t * ecp)
{
    int     adv;
    int     lpa;
    //int     bmsr;
    int     bmcr;
    //int     aner;
    int speed;
    int duplex;

    if (wait_for_aneg_completion(ecp)) {
        printk(KERN_INFO "MII_BMCR:0x%x\n", read_phy_reg(ecp, MII_BMCR));
        printk(KERN_INFO "auto-negotiation is timeout.\n");
        return (-1);
    }

    bmcr = read_phy_reg(ecp, MII_BMCR);
    if (bmcr < 0)
        return (bmcr);

#if 0 /* FIXME: should check if both side have auto-negotiation ability */
    bmsr = read_phy_reg(ecp, MII_BMSR);
    if (bmsr < 0)
        return (bmsr);

    aner = read_phy_reg(ecp, MII_EXPANSION);
    if (aner < 0)
        return (aner);

    ecp->autoneg = aner & EXPANSION_NWAY ? true : false;
#else
    ecp->autoneg = bmcr & BMCR_ANENABLE ? true : false;
#endif
    printk(KERN_INFO "ecp->autoneg:%d", (int)ecp->autoneg);

    if (ecp->autoneg) {
        lpa = read_phy_reg(ecp, MII_LPA);
        if (lpa < 0) {
            printk(KERN_INFO "lpa error : 0x%x\n", lpa);
            return (lpa);
        }

        adv = read_phy_reg(ecp, MII_ADVERTISE);
        if (adv < 0) {
            printk(KERN_INFO "adv error : 0x%x\n", adv);
            return (adv);
        }

        /* mii anar and'd anlpar to get mii abilities
         * there is a priority order according to ieee 802.3u, as follow
         * 100M-full, 100MbaseT4, 100M-half, 10M-full and last 10M-half
         * fdp110 don't support 100MbaseT4
         */
        lpa &= adv;
        ecp->speed = ETH_SPEED_10M;
        ecp->duplex = ETH_DUP_HALF;

        if ((lpa & (LPA_100FULL | LPA_100HALF))) {
            ecp->speed = ETH_SPEED_100M;
            if (lpa & LPA_100FULL)
                ecp->duplex = ETH_DUP_FULL;
        } else if ((lpa & LPA_10FULL)) {
            ecp->duplex = ETH_DUP_FULL;
        }

        if (bmcr & BMCR_FULLDPLX) {
            duplex = ETH_DUP_FULL;
        } else {
            duplex = ETH_DUP_HALF;
        }
        if (duplex != ecp->duplex) {
            ecp->duplex = duplex;
            printk(KERN_INFO "BMCR & ANLPAR duplex conflicts!!!\n");
        }

        if (bmcr & BMCR_SPEED100) {
            speed = ETH_SPEED_100M;
        } else {
            speed = ETH_SPEED_10M;
        }
        if (speed != ecp->speed) {
            ecp->speed = speed;
            printk(KERN_INFO "BMCR & ANLPAR speed conflicts!!!\n");
        }

        if (ETH_DUP_FULL == ecp->duplex)
            ecp->pause = (lpa & LPA_PAUSE_CAP) ? true : false;
    } else {
        ecp->duplex = (BMCR_FULLDPLX & bmcr) ? ETH_DUP_FULL : ETH_DUP_HALF;
        ecp->speed = (BMCR_SPEED100 & bmcr) ? ETH_SPEED_100M : ETH_SPEED_10M;
        ecp->pause = false;
    }

    printk(KERN_INFO VT_GREEN "\n%s -> speed:%d, duplex:%s, pause: %s\n" VT_NORMAL,
           ecp->autoneg ? "autoneg" : "forced", ecp->speed,
           (ETH_DUP_FULL == ecp->duplex) ? "full" : "half",
           ecp->pause ? "supported" : "non-supported");

    return (0);
}

/**
 * fdp110_get_link - get link state;
 * we alse can get link status from MAC_CSR5[LCIS]
 * return 0 if link not established, 1 if established, negative value if fail
 */
static int fdp110_get_link(ec_priv_t * ecp)
{
    int     bmsr;

    bmsr = read_phy_reg(ecp, MII_BMSR);
    if (bmsr < 0) {
        return (bmsr);
    }

    return (!!(bmsr & BMSR_LSTATUS));
}

/**
 * fdp110_suspend -- power down or power on the phy
 * return 0 if success, negative if fail
 */
static int fdp110_suspend(ec_priv_t * ecp, bool power_down)
{
    int     reg_val;

    reg_val = read_phy_reg(ecp, MII_BMCR);
    if (reg_val < 0) {
        return (reg_val);
    }

    if (power_down) {
        reg_val |= BMCR_PDOWN;
    } else {
        reg_val &= ~BMCR_PDOWN;
    }

    return (write_phy_reg(ecp, MII_BMCR, reg_val));

}


static struct phy_info fdp110_ops = {
    .id = 0x0,
    .name = "fdp110", /* same to ATC2605 */
    .phy_hw_init = NULL,//atc2605_hw_init,
    .phy_init = phy_init,
    .phy_suspend = fdp110_suspend,
    .phy_setup_advert = fdp110_setup_advert,
    .phy_setup_forced = fdp110_setup_forced,
    .phy_setup_aneg = fdp110_setup_aneg,
    .phy_setup_loopback = fdp110_setup_loopback,
    .phy_read_status = fdp110_read_status,
    .phy_get_link = fdp110_get_link,
};

static struct phy_info ksz8041_ops = {
    .id = 0x0,
    .name = "ksz8041",
    .phy_hw_init = NULL, /* not need */
    .phy_init = phy_init,
    .phy_suspend = fdp110_suspend,
    .phy_setup_advert = fdp110_setup_advert,
    .phy_setup_forced = fdp110_setup_forced,
    .phy_setup_aneg = fdp110_setup_aneg,
    .phy_setup_loopback = fdp110_setup_loopback,
    .phy_read_status = fdp110_read_status,
    .phy_get_link = fdp110_get_link,
};


static struct phy_info rs8201g_ops = {
    .id = 0x0,
    .name = "rs8201g",
    .phy_hw_init = NULL, /* not need */
    .phy_init = phy_init,
    .phy_suspend = fdp110_suspend,
    .phy_setup_advert = fdp110_setup_advert,
    .phy_setup_forced = fdp110_setup_forced,
    .phy_setup_aneg = fdp110_setup_aneg,
    .phy_setup_loopback = fdp110_setup_loopback,
    .phy_read_status = fdp110_read_status,
    .phy_get_link = fdp110_get_link,
};




static struct phy_info rtl8201_ops = {
    .id = 0x0,
    .name = "rtl8201",
    .phy_hw_init = NULL, /* not need */
    .phy_init = phy_init,
    .phy_suspend = fdp110_suspend,
    .phy_setup_advert = fdp110_setup_advert,
    .phy_setup_forced = fdp110_setup_forced,
    .phy_setup_aneg = fdp110_setup_aneg,
    .phy_setup_loopback = fdp110_setup_loopback,
    .phy_read_status = fdp110_read_status,
    .phy_get_link = fdp110_get_link,
};




void ep_set_phy_ops(ec_priv_t * ecp)
{
    if (ecp->phy_model == ETH_PHY_MODEL_ATC2605) {
        printk(KERN_INFO "phy model: ATC2605\n");
        ecp->phy_ops = &fdp110_ops;
        //read_phy_reg = read_phy_reg_atc2605;
        //write_phy_reg = write_phy_reg_atc2605;
    } else if (ecp->phy_model == ETH_PHY_MODEL_KSZ8041TL) {
        printk(KERN_INFO "phy model: KSZ8041TL\n");
        ecp->phy_ops = &ksz8041_ops;
        read_phy_reg = read_phy_reg_ksz8041;
        write_phy_reg = write_phy_reg_ksz8041;
    } else if (ecp->phy_model == ETH_PHY_MODEL_RTL8201) {
        printk(KERN_INFO "phy model: RTL8201\n");
        ecp->phy_ops = &rtl8201_ops;
        read_phy_reg = read_phy_reg_rtl8201;
        write_phy_reg = write_phy_reg_rtl8201;
    } else if (ecp->phy_model == ETH_PHY_MODEL_SR8201G) {
        printk(KERN_INFO "phy model: SR8201G\n");
        ecp->phy_ops = &rs8201g_ops;
        read_phy_reg = read_phy_reg_rs8201g;
        write_phy_reg = write_phy_reg_rs8201g;
    } else {
        printk(KERN_INFO "NOT supported phy model: %u\n", ecp->phy_model);
    }
}
