#ifdef XHCI_SUPPORT_SUPERSPEED       

static void set_reg32_val(void __iomem *base , unsigned int mask, unsigned int val, unsigned int reg)
{
	unsigned int Wtmp;

	Wtmp =readl(base +  reg);
	Wtmp &= ~(mask);
	val &= mask;
	Wtmp |= val ;    
	writel(Wtmp,base +  reg);
}

static void ss_phy_rx_init(void __iomem *base)
{

	//modified by lchen
	set_reg32_val(base , 0x1f<<1, 0x15<<1, USB3_ANA03); 	//for lowest OOBS_sensitivity
	set_reg32_val(base ,0x3<<14, 0x2<<14, USB3_ANA0B); 	//for OOBS CM, default 10
	set_reg32_val(base ,0x3<<12, 0x2<<12, USB3_ANA0B); 	//for OOBS DM, default 10
	writel( 0x9555, base +  USB3_ANA04);	//for CMU_LDO=2.1V	
	writel(0x2d91,base + USB3_ANA08);	//for proper DC gain
	
	set_reg32_val(base ,0x1<<6, 0x0<<6, USB3_ANA09); 	//for RXIDLE=LFPS_DET & NSQ(1) or NSQ(0)
	set_reg32_val(base , 0x7<<12, 0x3<<12, USB3_ANA09); 	//for IB_TX and IB_OOBS	
	writel(0xFF68 ,base + USB3_ANA0D);	//for CDR_PI and RX_Z0	
	writel(0x2020,base + USB3_ANA0E);	//for TX_CM mode
	writel(0xFF0C, base + USB3_REV2);		//for CDR and PI in 0x1B---[0xFF08--BAD]
	set_reg32_val(base , 0x7<<2, 0x7<<2, USB3_REV3); 	//for CP0 selected	

	//set_reg32_val(base , 0x1<<0, 0x0<<0, USB3_FLD0); 	//for disable BER checker	

	writel(0xD4FF,base + USB3_PAGE1_REG00);		//for 0x20 TX_DRV
	writel(0xAAFF,base + USB3_PAGE1_REG01);		// TX_DRV
	writel(0x0051,base + USB3_PAGE1_REG02);		// TX_DRV   and BER ckecker sel
	writel(0xDB60,base + USB3_PAGE1_REG03);		// TX_DRV

	//for force offset value
	set_reg32_val(base , 0xf<<5, 0x9<<5, USB3_ANA0B); 			
	//set_reg32_val(base , 0x1<<13, 0x0<<13, USB3_ANA0D); //manual set when 0
	
	//for CDR loop setup earlier
	set_reg32_val(base ,0x1<<1, 0x1<<1, USB3_ANA0B); 	//for PIEN select change
	set_reg32_val(base ,0x1<<15, 0x0<<15, USB3_ANA0C); 	//for POW_CLK(PSAVE) select change
	set_reg32_val(base ,0x1<<3, 0x0<<3, USB3_ANA00); 	//for cdr RESET select change(POWER DISPPU)
	set_reg32_val(base ,0x1<<10, 0x0<<10, USB3_ANA0D); 	//for CDR digitalLFP ENABLE select change	

	//for manual mode CDR_ST<31:0>
	set_reg32_val(base ,0x1<<15, 0x1<<15, USB3_ANA02); 	//for enable manual set ST value
	set_reg32_val(base ,0xffff<<0, 0x0<<0, USB3_PAGE1_REG0C); 	//for ST[31:16]
	set_reg32_val(base ,0xffff<<0, 0xff<<0, USB3_PAGE1_REG0D); 	//for ST[15:0]
	
	//for PI bias	
	set_reg32_val(base ,0x3<<3, 0x1<<3, USB3_REV2); 	//for PI bias[3:2] default=1
	set_reg32_val(base ,0x3<<11, 0x2<<11, USB3_ANA0D); 	//for PI bias[1:0] default=2	

	//for force EQ value      
	//setReg32_val(0x1<<14, 0x0<<14, USB3_PAGE1_REG00);	//for manual mode when REG_CDR_SEL=0			
	set_reg32_val(base ,0x1<<5, 0x1<<5, USB3_ANA0A);	       //for manual set when REG_RX_EQ_SELREG=1	
	set_reg32_val(base ,0x1f<<11, 0x7<<11, USB3_REV0);	       //for REG_FILTER_OUT=from 11100 to 10000	
	
	//APHY Dbg Switch 
	set_reg32_val(base ,  1<<0, 1<<0, USB3_ANA0D); //for TX_TEST_EN=1	
	set_reg32_val(base ,1<<6, 1<<6, USB3_ANA0D);	//for RX_TEST_EN=1	
	set_reg32_val(base , 1<<9, 1<<9, USB3_ANA02); 	//for CDR_TEST_EN=1	
	set_reg32_val(base , 0x7<<10, 0x2<<10, USB3_ANA02); 	//for CDR_TESTOUT_SEL=CLKDEBUG	
}

static void ss_phy_analog_setting(void __iomem *base)
{
	writel(0x8000,base + USB3_ANA0F);
	writel(0x6046,base + USB3_ANA02);
	writel(0x2020,base + USB3_ANA0E);

	writel(0x0,base + USB3_REV1);
	writel(0x0013,base + USB3_PAGE1_REG02);
	writel(0x0004,base + USB3_PAGE1_REG06);
	writel(0x22ed,base + USB3_PAGE1_REG07);
	writel(0xf802,base + USB3_PAGE1_REG08);
	writel(0x3080,base + USB3_PAGE1_REG09);
	writel(0x2030,base + USB3_PAGE1_REG0B);

	ss_phy_rx_init(base);
    
	set_reg32_val(base , ((1<<5)|(1<<4)),((1<<5)|(0<<4)),USB3_ANA0E);//bit4: [0=cmfb mode=usingRxDetect]  [1=opab  mode=Nomal Using]
	set_reg32_val(base , (3<<12),(2<<12),USB3_ANA0E);//Tx_SCE_VCM
	set_reg32_val(base , (3<<2),(0<<2),USB3_ANA0C);//RxSel---time
	set_reg32_val(base , (1<<15),(0<<15),USB3_FLD0);

	set_reg32_val(base , (3<<0),(2<<0),USB3_ANA08); //Rx DC gain
	set_reg32_val(base ,(3<<1),(3<<1),USB3_PAGE1_REG06); // DC gain--DB
	set_reg32_val(base ,((7<<0)|(7<<3)|(7<<6)),((1<<0)|(1<<3)|(4<<6)),USB3_ANA01);
}
/*
* host_dev: 0: device; 1: host
*/
void ss_phy_init(void __iomem *base,int host_dev)
{	     
	ss_phy_analog_setting(base);

	set_reg32_val(base ,((1<<5)|(1<<3)),((1<<5)|(0<<3)), USB3_PAGE1_REG0B);//must bit3=0---20140925
	set_reg32_val(base ,(1<<7),(0<<7), USB3_PAGE1_REG0A);//must bit7=0---20140925
	set_reg32_val(base ,(0xF<<2),(0x7<<2), USB3_ANA0D);
	//set_reg32_val(base ,(1<<3),(0<<3), USB3_PAGE1_REG0B);//
	//set_reg32_val(base ,((1<<8)|(1<<10)),((0<<8)|(0<<10)), USB3_FLD1);//	
	//set_reg32_val(base ,(1<<7),(0<<7), USB3_PAGE1_REG0A);//
	set_reg32_val(base ,(1<<0),(0x1<<0), USB3_FLD0); //Biterr Checker Function Disable---20141127
	writel(0,base + USB3_IER);
	set_reg32_val(base ,(1<<11),(1<<11), USB3_REV3);//rcv clock sample	

	set_reg32_val(base ,(0xf<<12),(0x6<<12), USB3_ANA02); //care---or will go to HS
	set_reg32_val(base ,(0xf<<0),(0xd<<0), USB3_ANA03); //care---or will sometime goto HS
	set_reg32_val(base ,(0xff<<8),(0xfc<<8), USB3_IER); //care
	if(host_dev==1){
		set_reg32_val(base ,(0xf<<0),(0x8<<0), USB3_PAGE1_REG0B);//care
		set_reg32_val(base , (3<<0),(3<<0),USB3_ANA08); //Rx DC gain
	}
	else{		

		set_reg32_val(base , (1<<22) , (0<<22), BACKDOOR); //LUP timing select--20140624
		set_reg32_val(base ,(1<<9)|(1<<8) , (1<<9)|(1<<8), BACKDOOR); //Mac SampleEdge select--20140624			
		set_reg32_val(base ,((1<<3)|(0x3f<<4)), ((0<<3)|(36<<4)), USB3_DMR);//enableDebug Out		
		set_reg32_val(base , (1<<9)  , (1<<9) , BACKDOOR); //Mac SampleEdge select
	
	}
	set_reg32_val(base ,(1<<14),(1<<14), GUCTL);//fix asix usbnet dongle retry hangup
	set_reg32_val(base ,(1<<17),(1<<17), GUCTL);//fix some udisk control transfer too slow,set config fail
	writel(0x2408,base + GUSB2CFG0);
} 
EXPORT_SYMBOL_GPL(ss_phy_init);
#endif
