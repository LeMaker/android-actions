#ifdef XHCI_SUPPORT_SUPERSPEED       

#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include "xhci_usb3_regs.h"

extern int usb_set_vbus_power(int value);
#define  LfpsSrc_Mode_LPFS_DET	(1)
#define  LfpsSrc_Mode_NSQ		(2)
#define  LfpsSrc_Mode_BOTH		(3)

//unsigned int capltssm[64/4];  //char[64]
//unsigned int capltssm_curr_idx;
static int lfps_src_mode = LfpsSrc_Mode_LPFS_DET; 
unsigned int ltssm_old,ltssm_curr;
static void set_reg32_val(void __iomem *base , unsigned int mask, unsigned int val, unsigned int reg);
void  ss_mac_sample_edge_init(void __iomem *base );
#ifdef XHCI_SS_SUPPORT_RETRY   
static int ss_detect_ltssm_change(void __iomem *base)
{
	volatile unsigned int Wtmp ;
	//int ret=1;
	//volatile unsigned int  tmp_ts1;
	//unsigned int  cap;
	//unsigned char * pU8= (unsigned char *)capltssm;
	int cnt = 0xffff;
    
    	for(cnt = 0;cnt <100;cnt++){
            	mdelay(1);    
		Wtmp =readl(GDBGLTSSM+base);
		if((Wtmp&(0xf<<22))==0) {
			// connected
			printk(KERN_ERR"\n== in U0 ,don't need retry ==\n");
			return 1;
		}  		    
	}
	printk(KERN_ERR"\n----wait U0 fail!!---\n");    
    	return 0;
        
#if 0    
	Wtmp =readl(USB3_GDBGLTSSM+base);
	ltssm_curr =  Wtmp & ((0xf<<22)|(0xf<<18));
	if(ltssm_curr != ltssm_old )
	{
		//pU8[capltssm_curr_idx++] = (ltssm_curr>>18);
		//capltssm_curr_idx &= (64-1);
		printk("\n-----detectLtssChange-----USB3_GDBGLTSSM=0x%x----Ltssm_curr=0x%x--Ltssm_old=0x%x--\n",\
            		Wtmp,ltssm_curr,ltssm_old);

		if((Wtmp &(0xf<<22))==(0x7<<22))
		{
			while(1)
			{
				tmp_ts1 = readl(USB3_GDBGLTSSM+base) & ((0xf<<22)|(0xf<<18));
				Wtmp =tmp_ts1 & (0xf<<22);
				if(Wtmp==0){
					printk("\n=============in U0============\n");
					break;  //if=U0
				}
				else if(Wtmp==(0xa<<22))  //if=Complaint
				{
					Wtmp =Wtmp>>18;  //add for easy to see Ltssm State
					ret =0; 
					return ret;
				}else if(Wtmp==(0x5<<22)) //if =RxDetect again
				{
					printk("\n===========RxDetect again========\n\n");
					while(1)
					{
						Wtmp = readl(USB3_GDBGLTSSM+base) & (0xf<<22);
						Wtmp = Wtmp>>22;
						if(Wtmp==(0xa<<0))   {
							ret =0; 
							return ret;
						}  
						else if(Wtmp==0 )  
						break;
                        			cnt--;
                                    	if(cnt <= 0)
                                            return 0;
					}
					break;
				}
				else if(Wtmp==(0x7<<22))
					cap = tmp_ts1>>18;
			}
		}
	}
	ltssm_old= ltssm_curr;	
	ret = 1;// connect
	return ret;
#endif    
} 
#endif
static void ss_try_next_mode(void __iomem *base)
{  
    	printk(" ss_try_next_mode:lfps_src_mode=%d\n",lfps_src_mode );
	switch(lfps_src_mode)
	{
		case LfpsSrc_Mode_LPFS_DET :
		{
			set_reg32_val(base ,  (1<<9), (0<<9), GUSB3PIPECTL); //  for Test=LFPS Filter 
			set_reg32_val(base , ((1<<8)|(1<<15)),((0<<8)|(0<<15)), USB3_FLD0); 
			set_reg32_val(base , (1<<3),(0<<3), USB3_PAGE1_REG0B);  //active--will remove false Lfps In JiaJun--20150330
			break;
		}
		
		case LfpsSrc_Mode_NSQ :
		{
			set_reg32_val(base , (1<<9), (0<<9), GUSB3PIPECTL); //  for Test=LFPS Filter 
			set_reg32_val(base , ((1<<8)|(1<<15)),((0<<8)|(0<<15)), USB3_FLD0); 
			set_reg32_val(base , ((1<<3)|(1<<5)),((1<<3)|(1<<5)), USB3_PAGE1_REG0B);  //active--will remove false Lfps In JiaJun--20150330
			break;
		}	
		
		case LfpsSrc_Mode_BOTH :
		{  
			set_reg32_val(base , (1<<9), (0<<9), GUSB3PIPECTL); //  for Test=LFPS Filter 
			set_reg32_val(base , ((1<<8)|(1<<15)),((0<<8)|(0<<15)), USB3_FLD0); 
			set_reg32_val(base , ((1<<3)|(1<<5)),((1<<3)|(1<<5)), USB3_PAGE1_REG0B);  //active--will remove false Lfps In JiaJun--20150330
			set_reg32_val(base , (1<<6),(1<<6), USB3_ANA09); 
			break;
		}
		
		default:
		{
			break;
		}		
	}
	//if(++lfps_src_mode >LfpsSrc_Mode_BOTH)
	//	lfps_src_mode = LfpsSrc_Mode_LPFS_DET;
}

void ss_retry_phy_softrest(void __iomem *base)
{
	set_reg32_val(base ,  (1<<31),(1<<31), GUSB3PIPECTL);  //PHY softRest
	mdelay(10);
	set_reg32_val(base ,  (1<<31),(0<<31), GUSB3PIPECTL);  //PHY softRest
}

void ss_retry_mode(void __iomem *base)
{
	unsigned int val;
    
	writel((1<<14),USB3_ANA0F+base);
	
	usb_set_vbus_power(0);
    
	ss_retry_phy_softrest(base);
	mdelay(5);
    	ss_mac_sample_edge_init(base ); 
	ss_try_next_mode(base);	
	mdelay(200);
    
	usb_set_vbus_power(1);
    
	mdelay(5);
    
    	val = 0x200;
	writel(val,base+XHCI_PORT2_STATUS);	
	set_reg32_val(base ,0xff, 2, GBDGFIFOSPACE); //select port2
	
}

void  ss_mac_sample_edge_init(void __iomem *base )
{
	set_reg32_val(base , (1<<22) , (0<<22), BACKDOOR); //LUP timing select--20140624
	set_reg32_val(base ,(1<<9)|(1<<8) , (1<<9)|(0<<8), BACKDOOR); //Mac SampleEdge select--20140624	
	set_reg32_val(base , (1<<9)  , (1<<9) , BACKDOOR); //Mac SampleEdge select
}

void ss_lfps_init(void __iomem *base)
{
	set_reg32_val(base ,  (1<<8), (0<<8), USB3_GCTL); //Not Infinit
	set_reg32_val(base , (1<<12), (1<<12), GUSB3PIPECTL); //  for Test=LFPS P0 Align (LFPSP0Algn)
	set_reg32_val(base ,  (1<<9), (0<<9), GUSB3PIPECTL); //  for Test=LFPS Filter 
	set_reg32_val(base , (3<<12), (1<<12), USB3_GCTL);
} 
#if 0
void ss_detect_ltssm_init(void __iomem *base)
{												
	set_reg32_val(base ,0xff, 2, USB3_GBDGFIFOSPACE); //select port2
	ltssm_old =readl(USB3_GDBGLTSSM+base) & ((0xf<<22)|(0xf<<18)) ;
	ltssm_curr = ltssm_old;
	capltssm_curr_idx =0;

	unsigned char * pU8= (unsigned char *)capltssm;
	pU8[capltssm_curr_idx++] = (ltssm_curr>>18);
}
#endif
void ss_retry_mode_init(void __iomem *base)
{
#if 0
	int i;   
        
    	void __iomem *cmu_base = (void __iomem *)IO_ADDRESS(0xB0168000);
    	set_reg32_val(cmu_base ,((1<<7)|(1<<6)),((1<<7)|(1<<6)), USB3_ECS);//softvbus =1
	set_reg32_val(cmu_base ,((1<<31)|(7<<28)),((1<<31)|(4<<28)), USB3_ECS); //USB3 USB2PLL LDO Enable	
#endif	

	ss_mac_sample_edge_init(base ); 

#if 0      
    	for (i=0;i<1000;i++);	
   		ss_detect_ltssm_init(base);   
     
    	writel(0x00001448 ,base + USB3_GUSB2CFG0);
    	set_reg32_val(base , (3<<4), (0<<4), USB3_GCTL);  //clean the scale down
#endif          
	set_reg32_val(base , (0xff<<0), (0x6<<0), GSBUSCFG0);//enable burst 8 and burst 4
	set_reg32_val(base , (0xf<<8), (0x7<<8), GSBUSCFG1); //AXI busrt requst limit is 3  
	
  	/*set_reg32_val(base , (0x1<<29), (1<<29), USB3_GTXTHRCFG);
      set_reg32_val(base , (0xf<<24), (0xf<<24), USB3_GTXTHRCFG);
      set_reg32_val(base , (0xf<<16), (0xf<<16), USB3_GTXTHRCFG);
      
      set_reg32_val(base , (0x1<<29), (1<<29), USB3_GRXTHRCFG);
      set_reg32_val(base , (0xf<<24), (0x3<<24), USB3_GRXTHRCFG);
      set_reg32_val(base , (0xf<<19), (0x3<<19), USB3_GRXTHRCFG);*/
      
}
#endif
