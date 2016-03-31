
#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_lcd.h>
#include <asm/io.h>
#include <asm/arch/pwm_backlight.h>
#include <asm/gpio.h>

#include <common.h>
#include <malloc.h>

#include <video_fb.h>
#include <owl_dss.h>
#include <linux/list.h>
#include <linux/fb.h>

#include "owl_cvbs.h"
#include "gl5206_tvout_reg.h"

#ifdef CONFIG_CMD_EXT4
#include <ext4fs.h>
#include <mmc.h>
#endif


DECLARE_GLOBAL_DATA_PTR;


#define	 GL5206_CMU_BASE				(0xB0160000)
#define	 GL5206_CMU_TVOUTPLL			(GL5206_CMU_BASE + 0x0088)
#define	 GL5206_CMU_DEVCLKEN1			(GL5206_CMU_BASE + 0x00a4)


struct data_fmt_param {
    const char *name;
    s32 data_fmt;
};

static struct data_fmt_param date_fmts[] = {
	{"PAL", OWL_TV_MOD_PAL},
	{"NTSC", OWL_TV_MOD_NTSC},
};

static u32 string_to_data_fmt(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(date_fmts); i++) {
		if (!strcmp(date_fmts[i].name, name))
			return date_fmts[i].data_fmt;
	}

	return -1;
}

static int valide_vid(int vid)
{
    switch(vid) {      
        case OWL_TV_MOD_PAL:
        case OWL_TV_MOD_NTSC:
            return 1;
        default:
            return 0;
    }

}

struct fb_videomode1 cvbs_display_mode;

void print_cvbsreg()
{
	printf("\n\nfollowing list all tvout register's value!\n");
	printf("register CMU_TVOUTPLL(0x%08x) value is 0x%08x\n",
		0xb0160088, readl(0xb0160088));
	printf("register CMU_DEVCLKEN0(0x%08x) value is 0x%08x\n",
		CMU_DEVCLKEN0, readl(CMU_DEVCLKEN0));
	printf("register CMU_DEVCLKEN1(0x%08x) value is 0x%08x\n",
		CMU_DEVCLKEN1, readl(CMU_DEVCLKEN1));
	printf("register CMU_DEVRST0(0x%08x) value is 0x%08x\n",
		CMU_DEVRST0, readl(CMU_DEVRST0));
	printf("register CMU_DEVRST1(0x%08x) value is 0x%08x\n",
		CMU_DEVRST1, readl(CMU_DEVRST1));
	
	printf("register TVOUT_EN(0x%08x) value is 0x%08x\n",
		TVOUT_EN, readl(TVOUT_EN));
	printf("register TVOUT_OCR(0x%08x) value is 0x%08x\n",
		TVOUT_OCR, readl(TVOUT_OCR));
	printf("register TVOUT_STA(0x%08x) value is 0x%08x\n",
		TVOUT_STA, readl(TVOUT_STA));
	printf("register TVOUT_PRL(0x%08x) value is 0x%08x\n",
		TVOUT_PRL, readl(TVOUT_PRL));
	printf("register TVOUT_CCR(0x%08x) value is 0x%08x\n",
		TVOUT_CCR, readl(TVOUT_CCR));
	printf("register TVOUT_BCR(0x%08x) value is 0x%08x\n",
		TVOUT_BCR, readl(TVOUT_BCR));
	printf("register TVOUT_CSCR(0x%08x) value is 0x%08x\n",
		TVOUT_CSCR, readl(TVOUT_CSCR));
	printf("register CVBS_MSR(0x%08x) value is 0x%08x\n",
		CVBS_MSR, readl(CVBS_MSR));
	printf("register CVBS_AL_SEPO(0x%08x) value is 0x%08x\n",
		CVBS_AL_SEPO, readl(CVBS_AL_SEPO));
	printf("register CVBS_AL_SEPE(0x%08x) value is 0x%08x\n",
		CVBS_AL_SEPE, readl(CVBS_AL_SEPE));
	printf("register CVBS_AD_SEP(0x%08x) value is 0x%08x\n",
		CVBS_AD_SEP, readl(CVBS_AD_SEP));
	printf("register CVBS_HUECR(0x%08x) value is 0x%08x\n",
		CVBS_HUECR, readl(CVBS_HUECR));
	printf("register CVBS_SCPCR(0x%08x) value is 0x%08x\n",
		CVBS_SCPCR, readl(CVBS_SCPCR));
	printf("register CVBS_SCFCR(0x%08x) value is 0x%08x\n",
		CVBS_SCFCR, readl(CVBS_SCFCR));
	printf("register CVBS_CBACR(0x%08x) value is 0x%08x\n",
		CVBS_CBACR, readl(CVBS_CBACR));
	printf("register CVBS_SACR(0x%08x) value is 0x%08x\n",
		CVBS_SACR, readl(CVBS_SACR));
	printf("register TVOUT_DCR(0x%08x) value is 0x%08x\n",
		TVOUT_DCR, readl(TVOUT_DCR));
	printf("register TVOUT_DDCR(0x%08x) value is 0x%08x\n",
		TVOUT_DDCR, readl(TVOUT_DDCR));
	printf("register TVOUT_DCORCTL(0x%08x) value is 0x%08x\n",
		TVOUT_DCORCTL, readl(TVOUT_DCORCTL));
	printf("register TVOUT_DRCR(0x%08x) value is 0x%08x\n",
		TVOUT_DRCR, readl(TVOUT_DRCR));
}


/*-----------------------configure cvbs---------------------*/
static void configure_ntsc(void)//ntsc(480i),pll1:432M,pll0:594/1.001
{	
	
	writel(CVBS_MSR_CVBS_NTSC_M | CVBS_MSR_CVCKS, CVBS_MSR);
	
	writel((readl(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALEP_MASK)) |
			    CVBS_AL_SEPO_ALEP(0xfe),CVBS_AL_SEPO);
	writel((readl(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALSP_MASK)) |
			   CVBS_AL_SEPO_ALSP(0x15),CVBS_AL_SEPO);	

	writel((readl(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALEPEF_MASK)) |
			   CVBS_AL_SEPE_ALEPEF(0x205),CVBS_AL_SEPE);
	writel((readl(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALSPEF_MASK)) |
			   CVBS_AL_SEPE_ALSPEF(0x11c),CVBS_AL_SEPE);

	writel((readl(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADEP_MASK)) |
			   CVBS_AD_SEP_ADEP(0x2cf),CVBS_AD_SEP);
	writel((readl(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADSP_MASK)) |
			   CVBS_AD_SEP_ADSP(0x0),CVBS_AD_SEP);
	
}

static void configure_pal(void)//pal(576i),pll1:432M,pll0:594M
{	

	writel(CVBS_MSR_CVBS_PAL_D | CVBS_MSR_CVCKS, CVBS_MSR);

	writel((readl(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALEP_MASK)) |
			   CVBS_AL_SEPO_ALEP(0x136),CVBS_AL_SEPO);
	writel((readl(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALSP_MASK)) |
			   CVBS_AL_SEPO_ALSP(0x17),CVBS_AL_SEPO);		   

	writel((readl(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALEPEF_MASK)) |
			   CVBS_AL_SEPE_ALEPEF(0x26f),CVBS_AL_SEPE);
	writel((readl(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALSPEF_MASK)) |
			   CVBS_AL_SEPE_ALSPEF(0x150),CVBS_AL_SEPE);

	writel((readl(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADEP_MASK)) |
			   CVBS_AD_SEP_ADEP(0x2cf),CVBS_AD_SEP);
	writel((readl(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADSP_MASK)) |
			   CVBS_AD_SEP_ADSP(0x0),CVBS_AD_SEP);

}

/**
 * @tv_mode: see enum TV_MODE_TYPE
 * This function sets register for CVBS(PAL/NTSC)
 */
 
static int configure_cvbs(int current_vid)
{
	switch (current_vid) {
	case OWL_TV_MOD_PAL:
		configure_pal();
		break;
		
	case OWL_TV_MOD_NTSC:
		configure_ntsc();
		break;
		
	default:
		printf("error! mode not supported\n");
		return -1;
	}
	return 0;
}


 void cvbs_show_colorbar()
{

	configure_cvbs(OWL_TV_MOD_PAL);
	
	/*enable color bar ,cvbs HDAC*/
	writel(readl(TVOUT_OCR) | TVOUT_OCR_DACOUT | TVOUT_OCR_DAC3 
		| TVOUT_OCR_INACEN | TVOUT_OCR_INREN, TVOUT_OCR);
	
	
	writel(readl(CMU_TVOUTPLL) | CMU_TVOUTPLL_PLL1EN |
		CMU_TVOUTPLL_TK0SS | CMU_TVOUTPLL_CVBS_PLL1FSS(0x4), CMU_TVOUTPLL);

	/*eable cvbs output*/
	writel(readl(TVOUT_EN) | TVOUT_EN_CVBS_EN, TVOUT_EN);

}

static int fdtdec_get_cvbs_par(int *  bootable, int * bootvid)
{
    int dev_node;
    const char *resolution ;
    int len;
    unsigned int vid = 0;
    int ret = 0;

    
    dev_node = fdtdec_next_compatible(gd->fdt_blob, 0, COMPAT_ACTIONS_OWL_CVBS);
    if (dev_node <= 0) {
        printf("%s: failed get default vid ,we used  default \n",__func__);
        return -1;
    }
    *bootable = fdtdec_get_int(gd->fdt_blob, dev_node, "bootable", 0);
  
    if(*bootable != 0)
    {
    	
    	resolution = fdt_getprop(gd->fdt_blob, dev_node, "default_mode", &len);
	    printf("%s: resolution  = %s\n",__func__, resolution);
	
	    vid = string_to_data_fmt(resolution);
	    
	    printf("boot cvbs vid =%d\n",vid);
	    
	    if (valide_vid(vid)){
	        * bootvid = vid;
	    } else {    	
	    	* bootvid = OWL_TV_MOD_PAL; 
	        printf("%s: not support %s ,we used default vid  = %d\n",__func__, resolution, bootvid);
	    }
    }
    return ret;
}



/**********************detecte cvbs status***********************************/
int get_cvbs_state(void)
{
    unsigned int status_val,boot_status;

    boot_status = readl(TVOUT_OCR);
    writel(TVOUT_OCR_PI_ADEN | TVOUT_OCR_PO_ADEN |TVOUT_OCR_PI_IRQEN, TVOUT_OCR);
    


    status_val = readl(TVOUT_STA) & TVOUT_STA_DAC3ILS;
    

    
    mdelay(500);
    
    status_val = readl(TVOUT_STA) & TVOUT_STA_DAC3ILS;

    if (status_val)
    	{
    	writel(boot_status, TVOUT_OCR); 
        return 1;        
      }
    else 
        return 0;
}


#ifdef CONFIG_CMD_EXT4
#define EXT4_CACHE_PART 5


const char* CVBS_SETTING_MODE_PATH = "setting/setting_cvbs_mode";

int cvbs_read_usr_cfg_file(const char* file_name, char* buf)
{
    static disk_partition_t info;
    block_dev_desc_t *dev_desc = NULL;
    int dev = 0;
    int part = EXT4_CACHE_PART;
    int err;
    int index=0;
    loff_t len;
    loff_t actread;
    const char* ifname;
    
    printf("read_usr_cfg_file\n");

    ifname = getenv("devif");
    if ( ifname == NULL ) {
        ifname = "nand";
        printf("get devif fail\n");
    }
    dev = get_boot_dev_num();
    
    dev_desc = get_dev(ifname, dev);
    if (dev_desc == NULL) {
        printf("Failed to find %s%d\n", ifname, dev);
        return 1;
    }

    if (get_partition_info(dev_desc, part, &info)) {
        printf("** get_partition_info %s%d:%d\n",
                ifname, dev, part);

        if (part != 0) {
            printf("** Partition %d not valid on device %d **\n",
                    part, dev_desc->dev);
            return -1;
        }

        info.start = 0;
        info.size = dev_desc->lba;
        info.blksz = dev_desc->blksz;
        info.name[0] = 0;
        info.type[0] = 0;
        info.bootable = 0;
#ifdef CONFIG_PARTITION_UUIDS
        info.uuid[0] = 0;
#endif
    }

    ext4fs_set_blk_dev(dev_desc, &info);

    if (!ext4fs_mount(info.size)) {
        printf("Failed to mount %s%d:%d\n",
            ifname, dev, part);
        ext4fs_close();
        return 1;
    }

    err = ext4fs_open(file_name, &len);
    if (err < 0) {
        printf("** File not found: %s\n", file_name);
        ext4fs_close();
        return 1;
    }
    
    err = ext4fs_read(buf, 64, &actread);
    if (err < 0) {
        printf("** File read error: %s\n", file_name);
        ext4fs_close();
        return 1;
    }
    ext4fs_close();
    
    printf("buf=%s", buf);
    
  
    return 0;
}
#endif

static int skip_atoi(char *s)
{
#define is_digit(c)    ((c) >= '0' && (c) <= '9')
   int i = 0;
   while (is_digit(*s))
       i = i * 10 + *((s)++) - '0';
   return i;
}



void cvbs_init()
{
	

	/*------------GET CONFIG FROM SETTING------------*/
	char mode[64]={0};
	char buf[256]={0};
	const char* ntsc_mode = "S:720x480i-60";
	int len = strlen(ntsc_mode);
	int bootable,bootvid;

	printf("cvbs_init uboot\n");
	if (fdtdec_get_cvbs_par(&bootable,&bootvid)) {
       printf("%s: error, fdtdec_get_cvbs_par: fdt No cvbs par, and now do nothing temply\n", __func__);
    }
    
  printf("cvbs boot init\n");  
  
  	/*------------CVBS INIT------------*/
	
	  writel(0x322, 0xb0160088);
	  mdelay(50);
	
	    /*configure TVOUT_DDCR*/
		writel(0x110050, TVOUT_DDCR);
		
		writel(0x7<<8,TVOUT_PRL);
		/* disable before registering irq handler */
		writel(0x0, TVOUT_OCR);
		
		/* clear pendings before registering irq handler */
		//writel(readl(TVOUT_STA), TVOUT_STA);
		
		
	if(bootable != 0&&get_cvbs_state()==1)
	{
		
		 if (!cvbs_read_usr_cfg_file(CVBS_SETTING_MODE_PATH, buf)){
	        bootvid = skip_atoi(&buf[0]);   
	        if(bootvid==8)
	        	{
	        		bootvid=OWL_TV_MOD_PAL;
	        	}  
	        	if(bootvid==9)
	        	{
	        		bootvid=OWL_TV_MOD_NTSC;
	        	}    
	    } 
		 		
		 printf("vid mode =%d\n",bootvid);
	 	 cvbs_display_mode = cvbs_display_modes[bootvid].mode;	
		
		/*------------CONFIG CVBS and ENABLE ------------*/
		printf("configure_cvbs vid =%d\n",cvbs_display_mode.vid);
		configure_cvbs(cvbs_display_mode.vid);	   
		//cvbs_enable();
	
		owl_display_register(TV_CVBS_DISPLAYER,"cvbs",&cvbs_ops, &cvbs_display_mode,24,1);
	}
}

int cvbs_enable()
{
	writel(readl(TVOUT_EN) | TVOUT_EN_CVBS_EN, TVOUT_EN);
	writel((readl(TVOUT_OCR) | TVOUT_OCR_DAC3 | TVOUT_OCR_INREN) &
		~TVOUT_OCR_DACOUT, TVOUT_OCR);
	return 0;
}

void cvbs_disable()
{
	writel(readl(TVOUT_EN) &  ~TVOUT_EN_CVBS_EN, TVOUT_EN);
	writel(readl(TVOUT_OCR) & ~(TVOUT_OCR_DAC3 | TVOUT_OCR_INREN), TVOUT_OCR);
}

struct display_ops cvbs_ops = {
	.enable = cvbs_enable,
	.disable = cvbs_disable,
};
