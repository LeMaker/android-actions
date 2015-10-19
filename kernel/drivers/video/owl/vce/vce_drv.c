#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* pr_err() */
#include <linux/errno.h>    /* error codes */
#include <linux/vmalloc.h>
#include <linux/init.h>     /* module_init/module_exit */
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/uaccess.h> /*#include <asm/uaccess.h>*/
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/powergate.h>
#include <mach/module-owl.h>
#include <mach/clkname.h>
#include "vce_drv.h"
#include "vce_reg.h"

#define DEVDRV_NAME_VCE      "vce"       /*device_driver.h*/
#define DEVICE_VCE           "/dev/vce"  /*major.h*/
#define MINOR_VCE        32
#define _MAX_HANDLES_ 1024
#define IRQ_VCE OWL_IRQ_H264_JPEG_ENCODER/*81*/
#define WAIT_TIMEOUT HZ  /*1s <--> HZ*/
#define WAIT_TIMEOUT_MS  3000 /*ms*/
#define VCE_DEFAULT_WIDTH   320
#define VCE_DEFAULT_HEIGHT  240
#define VCE_DEFAULT_FREQ    240/*MHZ*/
#define IC_TYPE_ATM7039	 0x7039
#define IC_TYPE_ATM7021	 0x7021
//#define VOL_INIT 975000
//#define VOL_RUG 1050000
/*wurui: raise 0.05v for dcdc voltage precision distance*/
#define VOL_INIT 1050000
#define VOL_RUG 1125000
#define VOL_MAX 1200000


#define PUT_USER          put_user
#define GET_USER          get_user
#define GET_USER_STRUCT   copy_from_user
#define SET_USER_STRUCT   copy_to_user

#define Enable_More_PrintK 0  /*show more print*/
#define Enable_Debug_PrintK  0 /*check register*/
#if Enable_More_PrintK
#define vce_info(fmt, ...)  printk("vce_drv: " fmt, ## __VA_ARGS__);
#else
#define vce_info(fmt, ...)  {}
#endif
#define vce_warning(fmt, ...) printk(KERN_WARNING"vce_drv: warning: " fmt, ## __VA_ARGS__)    
#define vce_err(fmt, ...) printk(KERN_ERR "vce_drv: %s(L%d) error: " fmt, __func__, __LINE__, ## __VA_ARGS__)

/*
 SPS_PG_CTL,b4 VCE&BISP&AISP power on enable
 SPS_PG_ACK,b4 VCE&BISP&AISP power on enable
 CMU_VCECLK
 CMU_DEVCLKEN0 bit26 VCE interface clock enable
 */
/*sram的控制权*/
#define Share_Mem_REG      0xB0240004
/*实现 Reset*/
/*#define CMU_DEVRST0     (0xB0160000 + 0xA8)*/ /*redefined*/
#define VCE_RESET          0x00100000
/*实现power gating*/
/*#define SPS_PG_CTL       0xB01B0100*/         /*redefined*/
#define VCE_BISP_POWERON   0x00000002
#define SPS_PG_ACK         0xB01B0100
/*实现clk切换到vce中*/
#define VCE_CLKEN  	       0x04000000
/*#define CMU_DEVCLKEN0    (0xB0160000 + 0xA0)*/ /*redefined*/
/*CMU_VCECLK选择时钟源和设置分频数*/
/*#define  CMU_DEVPLL      (0xB0160000 + 0x04)*/  /*redefined*/
/*#define  CMU_DDRPLL      (0xB0160000 + 0x08)*/  /*redefined*/
/*#define  CMU_DISPLAYPLL  (0xB0160000 + 0x10)*/  /*redefined*/
/*#define  CMU_VCECLK      (0xB0160000 + 0x44)*/  /*redefined*/

enum {
	STOPED,
	RUNNING,
	ERR,
};

typedef struct{
	vce_input_t  vce_in;
	vce_output_t vce_out;
	unsigned int vce_count;
	unsigned int vce_status;
}vce_info_t;

static DEFINE_MUTEX(vce_ioctl_mutex);
static DEFINE_MUTEX(vce_reset_mutex);
static struct completion vce_complete;
static int vce_status = 0;
static int vce_irq_registered = 0;
static int vce_open_count = 0;
static int vce_last_handle = 0;
static void *pAbuf[_MAX_HANDLES_];
static int vce_clk_isEnable = 0;
static vce_multi_freq_t vce_freq_multi = {VCE_DEFAULT_WIDTH,VCE_DEFAULT_HEIGHT,VCE_DEFAULT_FREQ}; 
static int vce_freq_is_init = 0;   
static int vce_resumed_flag = 0;
static wait_queue_head_t vce_wait;
static int gPownOnOffCount = 0;
static unsigned int gVceStatus = 0;
static int ic_type = IC_TYPE_ATM7021;
static unsigned int irq_vce = -1;
static unsigned long iobase_vce = 0;  /*VCE_BASE_ADDR*/
static unsigned long iobase_sram = 0; /*Share_Mem_REG*/
static unsigned long iobase_cmu_devclken0  = 0;  /*CMU_DEVCLKEN0*/
static unsigned long iobase_sps_pg_ctl     = 0;  /*SPS_PG_CTL*/
static struct regulator *power = NULL;/*voltage regulation*/

#if Enable_Debug_PrintK
static unsigned long iobase_cmu_devrst0    = 0;  /*CMU_DEVRST0*/
static unsigned long iobase_cmu_vceclk     = 0;  /*CMU_VCECLK*/
static unsigned long iobase_sps_pg_ack     = 0;  /*SPS_PG_ACK*/ 
static unsigned long iobase_cmu_devpll     = 0;  /*CMU_DEVPLL*/ 
static unsigned long iobase_cmu_ddrpll     = 0;  /*CMU_DDRPLL*/ 
static unsigned long iobase_cmu_displaypll = 0;  /*CMU_DISPLAYPLL*/ 
#endif

static void *vce_malloc(int32_t size)
{
	return kzalloc(size, GFP_KERNEL | GFP_DMA);
}

static void vce_free(void *ptr)
{
	kfree(ptr);
	ptr = NULL;
}

static unsigned int Re_Reg(unsigned int reg)
{
	unsigned int value = readl((void*)(iobase_vce + reg));
	return value;
}

static void Wr_Reg(unsigned int reg, unsigned int value)
{
	writel(value, (void*)(iobase_vce + reg));
}

static unsigned int Re_Reg_Self(unsigned long reg)
{
	unsigned int value = readl((void*)(reg));
	return value;
}

static void Wr_Reg_Self(unsigned long reg, unsigned int value)
{
	writel(value, (void*)(reg));
}

static int query_status(void)
{
	int ret_value = 0;
	int vce_status = Re_Reg(VCE_STATUS);
	gVceStatus = vce_status;

	if ((vce_status & 0x1) && ((vce_status & 0x100) == 0)){
		ret_value = VCE_BUSY;/*codec is runing*/		
	}
	else if (((vce_status & 0x1) == 0) && (vce_status & 0x100) &&(vce_status & 0x1000)){
		ret_value = VCE_READY;
	}
	else if (((vce_status & 0x1) == 0) && ((vce_status & 0x100) == 0)){
		ret_value = VCE_IDLE;
	}
	else if ((vce_status & 0x100) && (vce_status & 0x8000)){
		ret_value = VCE_ERR_STM_FULL;
	}
	else if ((vce_status & 0x100) && (vce_status & 0x10000)){
		ret_value = VCE_ERR_STM;
	}
	else if ((vce_status & 0x100) && (vce_status & 0x40000)){
		ret_value = VCE_ERR_TIMEOUT;
	}
	return ret_value;
}

static void vce_reset(void)
{
	vce_info("vce_reset\n");
	module_reset(MOD_ID_VCE);
}

static void vce_stop(void)
{
	if (owl_powergate_is_powered(OWL_POWERGATE_VCE_BISP)) {
		int i = 0x10000000;
		while ((VCE_BUSY == query_status())) {
			mdelay(1);
			if (i-- < 0)
				break;
			vce_err("OWL_POWERGATE_VCE_BISP is PownOn when VCE_BUSY\n");
		}
	}
}

static void vce_power_on(void)
{
	vce_info("vce_power_on in\n");
	vce_stop();

	if (owl_powergate_is_powered(OWL_POWERGATE_VCE_BISP)){
		vce_info("OWL_POWERGATE_VCE_BISP is PownOn\n");
	}

	owl_powergate_power_on(OWL_POWERGATE_VCE_BISP);

	gPownOnOffCount++;
	vce_info("vce_power_on out %d\n", gPownOnOffCount);
}

static void vce_power_off(void)
{
	vce_info("vce_power_off in\n");
	if (owl_powergate_is_powered(OWL_POWERGATE_VCE_BISP)) {
		vce_info("OWL_POWERGATE_VCE_BISP is PownOn\n");
	}

	owl_powergate_power_off(OWL_POWERGATE_VCE_BISP);
	gPownOnOffCount--;
	vce_info("vce_power_off out %d\n", gPownOnOffCount);
}

#if 0
static void vce_drv_cfg(void)
{
	vce_info("vce_drv_cfg in\n");
	unsigned int value;

	value = Re_Reg_Self(iobase_cmu_devrst0);
	value = value & (~VCE_RESET);
	Wr_Reg_Self(iobase_cmu_devrst0, value);

	value = Re_Reg_Self(iobase_cmu_devclken0);
	value = value | VCE_CLKEN;
	Wr_Reg_Self(iobase_cmu_devclken0, value);

	value = Re_Reg_Self(iobase_sps_pg_ctl);
	value = value | VCE_BISP_POWERON;
	Wr_Reg_Self(iobase_sps_pg_ctl, value);

loop: value = Re_Reg_Self(iobase_sps_pg_ack);
	value = value & VCE_BISP_POWERON;
	if (value == 0)
		goto loop;

	value = Re_Reg_Self(iobase_cmu_devrst0);
	value = value | VCE_RESET;
	Wr_Reg_Self(iobase_cmu_devrst0, value);

	value = 0x2;
	Wr_Reg_Self(iobase_cmu_vceclk, value);
	vce_info("vce_drv_cfg out\n");
}

static void vce_drv_cfg_close(void)
{
	unsigned int value;
	value = Re_Reg_Self(iobase_sps_pg_ctl);
	value = value & (~VCE_BISP_POWERON);
	Wr_Reg_Self(iobase_sps_pg_ctl, value);
}
#endif

static void share_mem_reg_enable(void)
{
	if(ic_type == IC_TYPE_ATM7039)
	{
		unsigned int value;
		value = Re_Reg_Self(iobase_sram);
		value = value | 0x2;
		Wr_Reg_Self(iobase_sram, value);
	}	
}

static void share_mem_reg_disable(void)
{
	if(ic_type == IC_TYPE_ATM7039)
	{
		unsigned int value;
		value = Re_Reg_Self(iobase_sram);
		value = value & (~0x2);
		Wr_Reg_Self(iobase_sram, value);
	}
}

static void vce_clk_enable(void)
{
#if 1
	struct clk *vce_clk;
	/*vce reset should be prior to vce clk enable , or vce may work abnormally*/
	vce_power_on();	
	if ((vce_open_count == 1) || (vce_resumed_flag == 1)) {
		vce_reset();
	}
	module_clk_enable(MOD_ID_VCE);
	vce_clk = clk_get_sys(NULL, CLKNAME_VCE_CLK);
	if (IS_ERR(vce_clk)) {
		vce_err("clk_get_sys(NULL, CLKNAME_VCE_CLK) failed\n");
		return;
	}
	clk_prepare(vce_clk);
	clk_enable(vce_clk);
#else
	vce_drv_cfg();
#endif
    vce_clk_isEnable = 1;
}

static void vce_clk_disable(void)
{
#if 1
	struct clk *vce_clk;
	vce_clk = clk_get_sys(NULL, CLKNAME_VCE_CLK);
	if (IS_ERR(vce_clk)) {
		vce_err("clk_get_sys(NULL,CLKNAME_VCE_CLK) failed\n");
		return;
	}	
	clk_disable(vce_clk);	
	clk_unprepare(vce_clk);
	
	if (vce_open_count == 0) {		
		module_clk_disable(MOD_ID_VCE);
	}	
	vce_power_off();
#else
	vce_drv_cfg_close();
#endif
    vce_clk_isEnable = 0;
}

/*================== 调频相关  =================*/
/* 设置vce频率，返回实际设置成功的频率点，返回-1表示设置失败；*/
static unsigned long  vce_setFreq(vce_multi_freq_t*  freq_MHz) 
{
	static struct clk *vce_clk;
	unsigned long rate, new_rate = freq_MHz->freq;
	unsigned int voltage;
	int ret;

	vce_info("width:%d  height:%d freq:%d\n",
	freq_MHz->width, freq_MHz->height, (int) freq_MHz->freq);

	if (new_rate > 500) {
		vce_err("cannot set vce freq to : %ld MHz\n",new_rate);
		return -1;
	}

	if (vce_clk_isEnable == 0)
		vce_warning("vce clk is not enable yet\n");

	vce_clk = clk_get_sys(NULL, CLKNAME_VCE_CLK);
	if (IS_ERR(vce_clk)) {
		vce_err("clk_get_sys(NULL,CLKNAME_VCE_CLK) failed\n");
		return -1;
	}

	rate = clk_get_rate(vce_clk);/*获取clk当前频率hz，如hosc：24000000*/
	if (rate == new_rate * 1000 * 1000) {
		vce_info("requested rate (%ld Mhz)is the same as the current rate,do nothing\n",new_rate);
		return new_rate;
	}

	if ((vce_open_count > 1) && (vce_freq_is_init == 1)) {
		if (freq_MHz->width * freq_MHz->height <
			vce_freq_multi.width * vce_freq_multi.height) {
			vce_info("multi instance, vce freq %ld MHz force to %ld MH!\n",new_rate, vce_freq_multi.freq);
			new_rate = vce_freq_multi.freq;
		} else {
			vce_info("multi instance, vce freq %ld MHz\n",new_rate);
			vce_freq_multi.width = freq_MHz->width;
			vce_freq_multi.height = freq_MHz->height;
			vce_freq_multi.freq = new_rate;
		}
	} else {
		vce_freq_multi.width = freq_MHz->width;
		vce_freq_multi.height = freq_MHz->height;
		vce_freq_multi.freq = new_rate;
		vce_freq_is_init = 1;
	}

	if(!IS_ERR(power))
	{
		/*720p/1280p 调压*/
		if (vce_freq_multi.width * vce_freq_multi.height >= 1280*720)
	    {
			voltage=VOL_RUG;	  	
	    }
		else
		{
			voltage=VOL_INIT;
		}
		if(regulator_set_voltage(power, voltage, VOL_MAX))
		{
	        vce_err("cannot set corevdd to %duV !\n", voltage);
	        return -1;
	    }
		vce_info("voltage regulator is ok!,to %duV.\n",voltage);
	}
	
	/*通过round rate 找到与设置频率最接近的频率*/
	rate = clk_round_rate(vce_clk, new_rate*1000*1000);	
	if (rate > 0) {		
		ret = clk_set_rate(vce_clk, rate); /*设置clk频率hz*/		
		if (ret != 0) {			
			vce_err("clk_set_rate failed\n");
			return -1;
		}		
	}

	rate = clk_get_rate(vce_clk); /*获取clk当前频率hz，如hosc：24000000*/
	vce_info("new rate (%ld MHz) is set\n",rate / (1000*1000));

	return rate / (1000 * 1000);
}

/* 返回vce频率，返回0表示失败*/
static unsigned long vce_getFreq(void)
{

	static struct clk *vce_clk;
	unsigned long rate;

	if(vce_clk_isEnable == 0) {
		vce_warning("vce clk is not enable yet\n");
	}
	/*根据clk_name获取clk结构体*/
	vce_clk = clk_get_sys(NULL, CLKNAME_VCE_CLK);
	if (IS_ERR(vce_clk)) {
		vce_err("clk_get_sys(CLK_NAME_VCE_CLK, NULL) failed\n");
		return 0;
	}

	rate = clk_get_rate(vce_clk); /*获取clk当前频率hz，如hosc：24000000*/
	vce_info("cur vce freq : %ld MHz\n",rate / (1000*1000));

	return rate / (1000 * 1000);
}

/* enable int */
static inline void enable_vce_irq(void)
{
}

static inline void disable_vce_irq(void)
{
	int vce_status;

	vce_status = Re_Reg(VCE_STATUS);

	/*vce_status = 0; vce_status&(~0x101);*/
	vce_status = vce_status & (~0x100);
	Wr_Reg(VCE_STATUS, vce_status); /*new*/
	gVceStatus = vce_status;
}

/**
 * This function is vce ISR.
 */
irqreturn_t vce_ISR(int irq, void *dev_id)
{
	if(vce_open_count > 0)
	{
		disable_vce_irq();
		complete(&vce_complete);
		wake_up_interruptible(&vce_wait);
	}

	return IRQ_HANDLED;
}

typedef struct{
	unsigned int vce_status;
	unsigned int vce_outstanding;
	unsigned int vce_cfg;
	unsigned int vce_param0;
	unsigned int vce_param1;
	unsigned int vce_strm;
	unsigned int vce_strm_addr;
	unsigned int vce_yaddr;
	unsigned int vce_list0;
	unsigned int vce_list1;
	unsigned int vce_me_param;
	unsigned int vce_swindow;
	unsigned int vce_scale_out;
	unsigned int vce_rect;
	unsigned int vce_rc_param1;
	unsigned int vce_rc_param2;
	unsigned int vce_rc_hdbits;
	unsigned int vce_ts_info;
	unsigned int vce_ts_header;
	unsigned int vce_ts_blu;
	unsigned int vce_ref_dhit;
	unsigned int vce_ref_dmiss;

	unsigned int ups_yas;
	unsigned int ups_cacras;
	unsigned int ups_cras;
	unsigned int ups_ifomat;
	unsigned int ups_ratio;
	unsigned int ups_ifs;
}vce_input_t_7021;

static int set_registers_7021(vce_input_t_7021  *vce_in)
{
	Wr_Reg(UPS_YAS_7021,vce_in->ups_yas);
	Wr_Reg(UPS_CBCRAS_7021,vce_in->ups_cacras);
	Wr_Reg(UPS_CRAS_7021,vce_in->ups_cras);
	Wr_Reg(UPS_IFORMAT_7021,vce_in->ups_ifomat);
	Wr_Reg(UPS_RATIO_7021,vce_in->ups_ratio);
	Wr_Reg(UPS_IFS_7021,vce_in->ups_ifs); 

	Wr_Reg(VCE_CFG,vce_in->vce_cfg);
	Wr_Reg(VCE_PARAM0,vce_in->vce_param0);
	Wr_Reg(VCE_OUTSTANDING_7021,vce_in->vce_outstanding);
	Wr_Reg(VCE_PARAM1,vce_in->vce_param1);
	Wr_Reg(VCE_STRM,vce_in->vce_strm);
	Wr_Reg(VCE_STRM_ADDR,vce_in->vce_strm_addr);
	Wr_Reg(VCE_YADDR,vce_in->vce_yaddr);
	Wr_Reg(VCE_LIST0_ADDR,vce_in->vce_list0);
	Wr_Reg(VCE_LIST1_ADDR,vce_in->vce_list1);
	Wr_Reg(VCE_ME_PARAM,vce_in->vce_me_param);
	Wr_Reg(VCE_SWIN,vce_in->vce_swindow);
	Wr_Reg(VCE_SCALE_OUT,vce_in->vce_scale_out);
	Wr_Reg(VCE_RECT,vce_in->vce_rect);
	Wr_Reg(VCE_RC_PARAM1,vce_in->vce_rc_param1);
	Wr_Reg(VCE_RC_PARAM2,vce_in->vce_rc_param2);
	Wr_Reg(VCE_TS_INFO,vce_in->vce_ts_info);
	Wr_Reg(VCE_TS_HEADER,vce_in->vce_ts_header);
	Wr_Reg(VCE_TS_BLUHD,vce_in->vce_ts_blu);
	Wr_Reg(VCE_RC_HDBITS,vce_in->vce_rc_hdbits);
	Wr_Reg(VCE_REF_DHIT,vce_in->vce_ref_dhit);
	Wr_Reg(VCE_REF_DMISS,vce_in->vce_ref_dmiss);

	return 0;
}

static unsigned int get_ups_ifomat_7021(vce_input_t  *vce_in)
{
	unsigned int ups_ifomat = 0;
	unsigned int stride = (vce_in->ups_str &0x3ff)  *8;
	unsigned int input_fomat = vce_in->input_fomat;
	ups_ifomat = input_fomat |  ( stride  << 16) ;

	return ups_ifomat;
}

static unsigned int get_ups_ratio_7021(vce_input_t  *vce_in)
{
	unsigned int ups_ratio = 0;
	unsigned int ups_ifs= vce_in->ups_ifs;
	unsigned int ups_ofs = vce_in->ups_ofs;
	int srcw = (ups_ifs & 0xffff)*8;
	int srch =  ((ups_ifs>>16) & 0xffff)*8;
	int dstw = (ups_ofs & 0xffff)*16;
	int dsth = ((ups_ofs>>16) & 0xffff)*16;

	unsigned int upscale_factor_h_int,upscale_factor_v_int;
	if(dstw<=srcw)
		upscale_factor_h_int =  (srcw*8192) / dstw;
	else
		upscale_factor_h_int = (srcw/2 - 1)*8192  / (dstw/2 - 1) ;

	if(dsth <= srch)
		upscale_factor_v_int =  (srch*8192) / dsth;
	else
		upscale_factor_v_int = (srch/2 - 1)*8192  / (dsth/2 - 1) ;
	ups_ratio = (upscale_factor_v_int & 0xffff) <<16 | (upscale_factor_h_int & 0xffff);

	return ups_ratio;
}

static void registers_7039_to_7021(vce_input_t  *vce_in,vce_input_t_7021  *vce_in_7021)
{
	vce_in_7021->ups_ifomat = get_ups_ifomat_7021(vce_in);
	vce_in_7021->ups_ratio = get_ups_ratio_7021(vce_in);
	vce_in_7021->ups_ifs = vce_in->ups_ifs<<3;  //*
	vce_in_7021->ups_yas = vce_in->ups_yas;
	vce_in_7021->ups_cacras = vce_in->ups_cacras;
	vce_in_7021->ups_cras = vce_in->ups_cras;
	vce_in_7021->vce_outstanding = (1<<31) |  (12<<16) | (1<<15) | 128;

	vce_in_7021->vce_status = vce_in->vce_status;
	vce_in_7021->vce_cfg = vce_in->vce_cfg;
	vce_in_7021->vce_param0 = vce_in->vce_param0;
	vce_in_7021->vce_param1 = vce_in->vce_param1;
	vce_in_7021->vce_strm = vce_in->vce_strm;
	vce_in_7021->vce_strm_addr = vce_in->vce_strm_addr;
	vce_in_7021->vce_yaddr = vce_in->vce_yaddr;
	vce_in_7021->vce_list0 = vce_in->vce_list0;
	vce_in_7021->vce_list1 = vce_in->vce_list1;
	vce_in_7021->vce_me_param = vce_in->vce_me_param;
	vce_in_7021->vce_swindow = vce_in->vce_swindow;
	vce_in_7021->vce_scale_out = vce_in->vce_scale_out;
	vce_in_7021->vce_rect = vce_in->vce_rect;
	vce_in_7021->vce_rc_param1 = vce_in->vce_rc_param1;
	vce_in_7021->vce_rc_param2 = vce_in->vce_rc_param2;
	vce_in_7021->vce_ts_info = vce_in->vce_ts_info;
	vce_in_7021->vce_ts_header = vce_in->vce_ts_header;
	vce_in_7021->vce_ts_blu = vce_in->vce_ts_blu;
	vce_in_7021->vce_rc_hdbits = vce_in->vce_rc_hdbits;
	vce_in_7021->vce_ref_dhit = vce_in->vce_ref_dhit;
	vce_in_7021->vce_ref_dmiss = vce_in->vce_ref_dmiss;
}

#if Enable_Debug_PrintK
static void print_vce_input_t_7021(vce_input_t_7021* vce_input)
{
	vce_info("ko.vce_input_t1!vce_status:%x,vce_cfg:%x,vce_param0:%x,vce_param1:%x\n",
		vce_input->vce_status,vce_input->vce_cfg,vce_input->vce_param0,vce_input->vce_param1);

	vce_info("ko.vce_input_t2!vce_strm:%x,vce_strm_addr:%x,vce_yaddr:%x,vce_list0:%x\n",
		vce_input->vce_strm,vce_input->vce_strm_addr,vce_input->vce_yaddr,vce_input->vce_list0);

	vce_info("ko.vce_input_t3!vce_list1:%x,vce_me_param:%x,vce_swindow:%x,vce_scale_out:%x\n",
		vce_input->vce_list1,vce_input->vce_me_param,vce_input->vce_swindow,vce_input->vce_scale_out);

	vce_info("ko.vce_input_t4!vce_rect:%x,vce_rc_param1:%x,vce_rc_param2:%x,vce_rc_hdbits:%x\n",
		vce_input->vce_rect,vce_input->vce_rc_param1,vce_input->vce_rc_param2,vce_input->vce_rc_hdbits);

	vce_info("ko.vce_input_t5!vce_ts_info:%x,vce_ts_header:%x,vce_ts_blu:%x\n",
		vce_input->vce_ts_info,vce_input->vce_ts_header,vce_input->vce_ts_blu);

	vce_info("ko.vce_input_t6!ups_ifomat:%x,ups_ratio:%x,ups_ifs:%x\n",
		vce_input->ups_ifomat,vce_input->ups_ratio,vce_input->ups_ifs);

	vce_info("ko.vce_input_t7!ups_yas:%x,ups_cacras:%x,ups_cras:%x,\n",
		vce_input->ups_yas,vce_input->ups_cacras,vce_input->ups_cras);

	vce_info("ko.vce_input_t8!vce_ref_dhit:%x,vce_ref_dmiss:%x\n",
		vce_input->vce_ref_dhit,vce_input->vce_ref_dmiss); 
}
#endif

static int set_registers_atm7021(vce_input_t  *vce_in)
{
	vce_input_t_7021  vce_in_7021;
	registers_7039_to_7021(vce_in,&vce_in_7021);
#if Enable_Debug_PrintK
	print_vce_input_t_7021(&vce_in_7021);
#endif
	set_registers_7021(&vce_in_7021);
	return 0;
}

static int set_registers_atm7039(vce_input_t  *vce_in)
{
	Wr_Reg(UPS_IFS_7039, vce_in->ups_ifs);
	Wr_Reg(UPS_STR_7039, vce_in->ups_str);
	Wr_Reg(UPS_OFS_7039, vce_in->ups_ofs);
	Wr_Reg(UPS_RATH_7039, vce_in->ups_rath);
	Wr_Reg(UPS_RATV_7039, vce_in->ups_ratv);
	Wr_Reg(UPS_YAS_7039, vce_in->ups_yas);
	Wr_Reg(UPS_CBCRAS_7039, vce_in->ups_cacras);
	Wr_Reg(UPS_CRAS_7039, vce_in->ups_cras);
	Wr_Reg(UPS_DWH_7039, vce_in->ups_dwh);
	Wr_Reg(UPS_BCT_7039, vce_in->ups_bct);
	Wr_Reg(UPS_SAB0_7039, vce_in->ups_sab0);
	Wr_Reg(UPS_SAB1_7039, vce_in->ups_sab1);
	Wr_Reg(UPS_DAB_7039, vce_in->ups_dab);
	Wr_Reg(UPS_CTL_7039, vce_in->ups_ctl);/*enable blding*/
	Wr_Reg(UPS_RGB32_SR_7039, vce_in->ups_rgb32_sr);
	Wr_Reg(UPS_BLEND_W_7039, vce_in->ups_blend_w);

	Wr_Reg(VCE_CFG, vce_in->vce_cfg);
	Wr_Reg(VCE_PARAM0, vce_in->vce_param0);
	Wr_Reg(VCE_PARAM1, vce_in->vce_param1);
	Wr_Reg(VCE_STRM, vce_in->vce_strm);
	Wr_Reg(VCE_STRM_ADDR, vce_in->vce_strm_addr);
	Wr_Reg(VCE_YADDR, vce_in->vce_yaddr);
	Wr_Reg(VCE_LIST0_ADDR, vce_in->vce_list0);
	Wr_Reg(VCE_LIST1_ADDR, vce_in->vce_list1);
	Wr_Reg(VCE_ME_PARAM, vce_in->vce_me_param);
	Wr_Reg(VCE_SWIN, vce_in->vce_swindow);
	Wr_Reg(VCE_SCALE_OUT, vce_in->vce_scale_out);
	Wr_Reg(VCE_RECT, vce_in->vce_rect);
	Wr_Reg(VCE_RC_PARAM1, vce_in->vce_rc_param1);
	Wr_Reg(VCE_RC_PARAM2, vce_in->vce_rc_param2);
	Wr_Reg(VCE_TS_INFO, vce_in->vce_ts_info);
	Wr_Reg(VCE_TS_HEADER, vce_in->vce_ts_header);
	Wr_Reg(VCE_TS_BLUHD, vce_in->vce_ts_blu);
	Wr_Reg(VCE_RC_HDBITS, vce_in->vce_rc_hdbits);
	Wr_Reg(VCE_REF_DHIT, vce_in->vce_ref_dhit);
	Wr_Reg(VCE_REF_DMISS, vce_in->vce_ref_dmiss);

	return 0;
}

static int get_registers(vce_output_t *vce_out)
{
	vce_out->vce_strm = Re_Reg(VCE_STRM);
	vce_out->vce_rc_param3 = Re_Reg(VCE_RC_PARAM3);
	vce_out->vce_rc_hdbits = Re_Reg(VCE_RC_HDBITS);

	vce_out->strm_addr = Re_Reg(VCE_STRM_ADDR);
	vce_out->i_ts_offset = Re_Reg(VCE_TS_INFO);
	vce_out->i_ts_header = Re_Reg(VCE_TS_HEADER);

	vce_out->vce_ref_dhit = Re_Reg(VCE_REF_DHIT);
	vce_out->vce_ref_dmiss = Re_Reg(VCE_REF_DMISS);
	return 0;
}

static void print_all_regs(char *s)
{
	if(ic_type == IC_TYPE_ATM7039)
	{
	vce_info("%s", s);

	vce_info("ShareSRam_CTL:%x\n", Re_Reg_Self(iobase_sram));

	vce_info("VCE_ID:%x,VCE_STATUS:%x,VCE_CFG:%x\n",
	Re_Reg(VCE_ID), Re_Reg(VCE_STATUS), Re_Reg(VCE_CFG));

	vce_info("VCE_PARAM0:%x,VCE_PARAM1:%x,VCE_STRM:%x\n",
	Re_Reg(VCE_PARAM0), Re_Reg(VCE_PARAM1), Re_Reg(VCE_STRM));

	vce_info("VCE_STRM_ADDR:%x,VCE_YADDR:%x,VCE_LIST0_ADDR:%x\n",
	Re_Reg(VCE_STRM_ADDR), Re_Reg(VCE_YADDR), Re_Reg(VCE_LIST0_ADDR));

	vce_info("VCE_LIST1_ADDR:%x,VCE_ME_PARAM:%x,VCE_SWIN:%x\n",
	Re_Reg(VCE_LIST1_ADDR), Re_Reg(VCE_ME_PARAM), Re_Reg(VCE_SWIN));

	vce_info("VCE_SCALE_OUT:%x,VCE_RECT:%x,VCE_RC_PARAM1:%x\n",
	Re_Reg(VCE_SCALE_OUT), Re_Reg(VCE_RECT), Re_Reg(VCE_RC_PARAM1));

	vce_info("VCE_RC_PARAM2:%x,VCE_RC_PARAM3:%x,VCE_RC_HDBITS:%x\n",
	Re_Reg(VCE_RC_PARAM2), Re_Reg(VCE_RC_PARAM3), Re_Reg(VCE_RC_HDBITS));

	vce_info("VCE_TS_INFO:%x,VCE_TS_HEADER:%x,VCE_TS_BLUHD:%x\n",
	Re_Reg(VCE_TS_INFO), Re_Reg(VCE_TS_HEADER), Re_Reg(VCE_TS_BLUHD));

	vce_info("UPS_CTL:%x,UPS_IFS:%x,UPS_STR:%x\n",
	Re_Reg(UPS_CTL_7039), Re_Reg(UPS_IFS_7039), Re_Reg(UPS_STR_7039));

	vce_info("UPS_OFS:%x,UPS_RATH:%x,UPS_RATV:%x\n",
	Re_Reg(UPS_OFS_7039), Re_Reg(UPS_RATH_7039), Re_Reg(UPS_RATV_7039));

	vce_info("UPS_YAS:%x,UPS_CBCRAS:%x,UPS_CRAS:%x\n",
	Re_Reg(UPS_YAS_7039), Re_Reg(UPS_CBCRAS_7039), Re_Reg(UPS_CRAS_7039));

	vce_info("UPS_BCT:%x,UPS_DAB:%x,UPS_DWH:%x\n",
	Re_Reg(UPS_BCT_7039), Re_Reg(UPS_DAB_7039), Re_Reg(UPS_DWH_7039));

	vce_info("UPS_SAB0:%x,UPS_SAB1:%x\n",
	Re_Reg(UPS_SAB0_7039), Re_Reg(UPS_SAB1_7039));

	vce_info("%s", s);
	}
}

#if Enable_Debug_PrintK
static void print_vce_input_t(vce_input_t* vce_input)
{
	vce_info("ko.vce_input_t1!vce_status:%x,vce_cfg:%x,vce_param0:%x,vce_param1:%x\n",\
		vce_input->vce_status, vce_input->vce_cfg,vce_input->vce_param0, vce_input->vce_param1);

	vce_info("ko.vce_input_t2!vce_strm:%x,vce_strm_addr:%x,vce_yaddr:%x,vce_list0:%x\n",\
		vce_input->vce_strm, vce_input->vce_strm_addr,vce_input->vce_yaddr, vce_input->vce_list0);

	vce_info("ko.vce_input_t3!vce_list1:%x,vce_me_param:%x,vce_swindow:%x,vce_scale_out:%x\n",\
		vce_input->vce_list1, vce_input->vce_me_param,vce_input->vce_swindow, vce_input->vce_scale_out);

	vce_info("ko.vce_input_t4!vce_rect:%x,vce_rc_param1:%x,vce_rc_param2:%x,vce_rc_hdbits:%x\n",\
		vce_input->vce_rect, vce_input->vce_rc_param1,vce_input->vce_rc_param2, vce_input->vce_rc_hdbits);

	vce_info("ko.vce_input_t5!vce_ts_info:%x,vce_ts_header:%x,vce_ts_blu:%x\n",\
		vce_input->vce_ts_info, vce_input->vce_ts_header,vce_input->vce_ts_blu);

	vce_info("ko.vce_input_t6!ups_ctl:%x,ups_ifs:%x,ups_str:%x,ups_ofs:%x\n",\
		vce_input->ups_ctl, vce_input->ups_ifs,vce_input->ups_str, vce_input->ups_ofs);

	vce_info("ko.vce_input_t7!ups_rath:%x,ups_ratv:%x,ups_yas:%x,ups_cacras:%x\n",\
		vce_input->ups_rath, vce_input->ups_ratv,vce_input->ups_yas, vce_input->ups_cacras);

	vce_info("ko.vce_input_t8!ups_cras:%x,ups_bct:%x,ups_dab:%x,ups_dwh:%x\n",\
		vce_input->ups_cras, vce_input->ups_bct,vce_input->ups_dab, vce_input->ups_dwh);

	vce_info("ko.vce_input_t9!ups_sab0:%x,ups_sab1:%x\n",\
		vce_input->ups_sab0, vce_input->ups_sab1);

	vce_info("ko.vce_input_t10!vce_ref_dhit:%x,vce_ref_dmiss:%x,ups_rgb32_sr:%x,ups_blend_w:%x\n",\
		vce_input->vce_ref_dhit, vce_input->vce_ref_dmiss,vce_input->ups_rgb32_sr, vce_input->ups_blend_w);
}

static void print_vce_output_t(vce_output_t* vce_output)
{
	vce_info("ko.vce_output_t1!vce_strm:%x,vce_rc_param3:%x,vce_rc_hdbits:%x\n",\
		vce_output->vce_strm, vce_output->vce_rc_param3,vce_output->vce_rc_hdbits);

	vce_info("ko.vce_output_t2!strm_addr:%x,i_ts_offset:%x,i_ts_header:%x\n",\
		vce_output->strm_addr, vce_output->i_ts_offset,vce_output->i_ts_header);
}

static void print_ShareSRam_CTL(char *str)
{
	vce_info("%s!ShareSRam_CTL:%x\n", str, Re_Reg_Self(iobase_sram));
}

void print_CMU_Reg(char *s)
{
	vce_info("%s\n", s);
	vce_info("Share_Mem_REG:%x  CMU_DEVRST0:%x  SPS_PG_CTL:%x\n",
		Re_Reg_Self(iobase_sram), Re_Reg_Self(iobase_cmu_devrst0), Re_Reg_Self(iobase_sps_pg_ctl));
	vce_info("DEV:%x,DDR:%x,DISPLAY:%x\n",
		Re_Reg_Self(iobase_cmu_devpll), Re_Reg_Self(iobase_cmu_ddrpll), Re_Reg_Self(iobase_cmu_displaypll));
	vce_info("VCECLK:%x,DEVCLKEN0:%x\n",
		Re_Reg_Self(iobase_cmu_vceclk), Re_Reg_Self(iobase_cmu_devclken0));
}
#endif

static void pAbuf_release(int vce_count)
{
	int i;
	vce_info_t *info = NULL;
	if (vce_count >= 1 && vce_count <= vce_open_count) {
		/*delete该节点*/
		for (i = vce_count; i < vce_open_count;i++) {
			pAbuf[i-1] = pAbuf[i];              /*用后面往前挪动*/
			info = (vce_info_t*)pAbuf[i-1];
			info->vce_count--;                   /*注意-1*/
		}
		pAbuf[vce_open_count - 1] = NULL;

		if (vce_last_handle == vce_count)
			vce_last_handle = 0;
		else if (vce_last_handle > vce_count)
			vce_last_handle--;
	} else {
		vce_warning("vce_count(%d) is out of range(%d)!\n",vce_count,vce_open_count);
	}
};

long vce_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	vce_info_t *info = (vce_info_t*) filp->private_data;
	
	int ret_value = 0;	
	void __user *from,*to;
	long time_rest = 0;
	int cur_status = 0;	
	unsigned long timeout;
	unsigned long expire;	
	int left_time;	
	vce_multi_freq_t  vce_freq;

	if ((Re_Reg_Self(iobase_cmu_devclken0) & VCE_CLKEN) == 0) {
		vce_err("vce clk is not enabled,CMU_DEVCLKEN0 = %x\n",Re_Reg_Self(iobase_cmu_devclken0));		
		return VCE_ERR_UNKOWN;
	}

	if ((Re_Reg_Self(iobase_sps_pg_ctl) & VCE_BISP_POWERON) == 0) {		
		vce_err("vce power gate is not enabled,SPS_PG_CTL = %x\n",Re_Reg_Self(iobase_sps_pg_ctl));		
		return VCE_ERR_UNKOWN;
	}

	switch (cmd) {
	case VCE_CHECK_VERSION:
	{
		mutex_lock(&vce_ioctl_mutex);
		if (Re_Reg(VCE_ID) != 0x78323634) {
			vce_err("VCE_ID ERR....");
		}
		mutex_unlock(&vce_ioctl_mutex);
	}
	break;

	case VCE_CMD_ENC_RUN/*VCE_RUN*/:
	{
		mutex_lock(&vce_ioctl_mutex);
		from = (void __user *)arg;
		
		/*若超时*/
		cur_status = query_status();
		time_rest = wait_event_interruptible_timeout(vce_wait,
			(!((gVceStatus & 0x1) == 1) &&
			((gVceStatus & 0x100) == 0)), WAIT_TIMEOUT);

		cur_status = query_status();

		if (time_rest <= 0 && (cur_status == VCE_BUSY)) {
			print_all_regs("vce_dev timeout when runing!reginfo:");
			vce_reset();
			mutex_unlock(&vce_ioctl_mutex);
			return VCE_ERR_BUSY;
		} else {
			/*already done,clear all & save irqs*/
			/*把上一个驱动保存结果*/
			if (vce_last_handle >= 1 && vce_last_handle < _MAX_HANDLES_) {
				vce_info_t *pinfo = (vce_info_t *)pAbuf[vce_last_handle - 1];
				if (pinfo != NULL) {
					get_registers(&pinfo->vce_out);
					pinfo->vce_status = STOPED;
				}
			}
		}
		if( GET_USER_STRUCT(&info->vce_in,from,sizeof(vce_input_t)) ) {
			vce_err("copy_from_user fail!\n");
			mutex_unlock(&vce_ioctl_mutex);
			return VCE_ERR_UNKOWN;
		}

#if Enable_Debug_PrintK
		//print_vce_input_t(&info->vce_in);
#endif

		/* init completion var */
		init_completion(&vce_complete);

		if(ic_type == IC_TYPE_ATM7039)
			set_registers_atm7039(&info->vce_in);
		else
			set_registers_atm7021(&info->vce_in);

#if Enable_Debug_PrintK
		print_ShareSRam_CTL("b4 vce run");
#endif
		Wr_Reg(VCE_STATUS, 0); /**new add*/

		vce_status = info->vce_in.vce_status;/*Re_Reg(VCE_STATUS);*/
		vce_status = (vce_status|0x1)&(~(0x1<<8));
		Wr_Reg(VCE_STATUS, vce_status); /**newW*/
		vce_last_handle = info->vce_count;
		info->vce_status = RUNNING;
		mutex_unlock(&vce_ioctl_mutex);
		break;
	}

	case VCE_GET_ENC_STATUS:
	{
		mutex_lock(&vce_ioctl_mutex);
		ret_value = query_status();
		mutex_unlock(&vce_ioctl_mutex);
		break;
	}

	case VCE_CMD_QUERY_FINISH:
	{
		mutex_lock(&vce_ioctl_mutex);
		to = (void __user *)arg;

		if (info->vce_status == STOPED) {
			if( SET_USER_STRUCT(to,&info->vce_out,sizeof(vce_output_t)) ){
				vce_err("copy_to_user fail!\n");
				mutex_unlock(&vce_ioctl_mutex);
				return VCE_ERR_UNKOWN;
			}
			mutex_unlock(&vce_ioctl_mutex);
			return 0;
		}

#if Enable_Debug_PrintK
		print_ShareSRam_CTL("b4 vce finish");
#endif

		/*已解完一帧*/
		ret_value = query_status();

		if (ret_value != VCE_BUSY) {
			get_registers(&info->vce_out);
			if( SET_USER_STRUCT(to,&info->vce_out,sizeof(vce_output_t)) ){
				vce_err("copy_to_user fail!\n");
				mutex_unlock(&vce_ioctl_mutex);
				return VCE_ERR_UNKOWN;
			}
			info->vce_status = STOPED;
			mutex_unlock(&vce_ioctl_mutex);
			goto out;
		}

		timeout = msecs_to_jiffies(WAIT_TIMEOUT_MS) + 1;

		/*pr_err(KERN_ERR"timeout:%d  jiffies ...\n",timeout);*/
		if (vce_irq_registered) {
			enable_vce_irq();
			left_time = wait_for_completion_timeout(&vce_complete,
				 timeout);
			if (unlikely(left_time == 0)) {
				vce_status = Re_Reg(VCE_STATUS);
				vce_err("time out!\n");

				if (vce_status & 0x100) {
					ret_value = 0;
					get_registers(&info->vce_out);
					if( SET_USER_STRUCT(to,&info->vce_out,sizeof(vce_output_t)) ){
						vce_err("copy_to_user fail!\n");
						mutex_unlock(&vce_ioctl_mutex);
						return VCE_ERR_UNKOWN;
					}

					info->vce_status = STOPED;
					disable_vce_irq();
					mutex_unlock(&vce_ioctl_mutex);
					goto out;
				}

				info->vce_status = STOPED;
				ret_value = VCE_ERR_TIMEOUT;
				disable_vce_irq();
				vce_err("timeout when QUERY_FINISH\n");
				vce_reset();
				mutex_unlock(&vce_ioctl_mutex);
				goto out;
			} else {
				/* normal case */
				ret_value = 0;
				get_registers(&info->vce_out);
				if( SET_USER_STRUCT(to,&info->vce_out,sizeof(vce_output_t)) ){
					vce_err("copy_to_user fail!\n");
					mutex_unlock(&vce_ioctl_mutex);
					return VCE_ERR_UNKOWN;
				}

				info->vce_status = STOPED;
				disable_vce_irq();
				mutex_unlock(&vce_ioctl_mutex);
				goto out;
			}
		}

		/*
		*全局变量jiffies取值为自操作系统启动以来的时钟滴答的数目，
		*在头文件<linux/sched.h>中定义,数据类型为unsigned long volatile
		*系统中采用jiffies来计算时间，
		*但由于jiffies溢出可能造成时间比较的错误，
		*因而强烈建议在编码中使用time_after等宏来比较时间先后关系，
		*这些宏可以放心使用
		*/
		expire = timeout + jiffies;
		do {
			ret_value = query_status();

			if (ret_value != VCE_BUSY) {
				get_registers(&info->vce_out);
				if( SET_USER_STRUCT(to,&info->vce_out,sizeof(vce_output_t)) ){
					vce_err("copy_to_user fail!\n");
					mutex_unlock(&vce_ioctl_mutex);
					return VCE_ERR_UNKOWN;
				}

				info->vce_status = STOPED;
				disable_vce_irq();
				mutex_unlock(&vce_ioctl_mutex);
				goto out;
			}

			if (time_after(jiffies, expire)) {
				ret_value = VCE_ERR_TIMEOUT;
				info->vce_status = STOPED;
				disable_vce_irq();
				vce_err("timeout when QUERY_FINISH jiffies\n");
				vce_reset();
				mutex_unlock(&vce_ioctl_mutex);
				goto out;
			}
		} while (1);

		mutex_unlock(&vce_ioctl_mutex);
	}
	break;

	case VCE_SET_DISABLE_CLK/*VCE_DISABLE_CLK*/:
	vce_info("vce_ioctl get clk disable cmd!\n");
	/*vce_clk_disable();*/
	break;

	case VCE_SET_ENABLE_CLK/*VCE_ENABLE_CLK*/:
	vce_info("vce_ioctl get clk enable cmd!\n");
	/*vce_clk_enable();*/
	break;

	case VCE_SET_FREQ:
	{
		mutex_lock(&vce_ioctl_mutex);
		vce_info("VCE_SET_FREQ...\n");
		from = (void __user *)arg;
		if( GET_USER_STRUCT(&vce_freq,from,sizeof(vce_multi_freq_t)) ){
			vce_err("copy_from_user fail!\n");
			mutex_unlock(&vce_ioctl_mutex);
			return VCE_ERR_UNKOWN;
		}
		ret_value = vce_setFreq(&vce_freq);
		mutex_unlock(&vce_ioctl_mutex);
	}
	break;

	case VCE_GET_FREQ:
	{
		mutex_lock(&vce_ioctl_mutex);
		vce_info("VCE_GET_FREQ...\n");
		ret_value = vce_getFreq();
		mutex_unlock(&vce_ioctl_mutex);
	}
	break;

	default:
	vce_err("no such cmd ...\n");
	return -EIO;
}
	
out:
	return ret_value;
}

int vce_open(struct inode *inode, struct file *filp)
{
	vce_info_t *info = NULL;

	int ret_frep;
	
	mutex_lock(&vce_ioctl_mutex);
	vce_open_count++;
	filp->private_data = NULL;

	if (vce_open_count > _MAX_HANDLES_) {
		vce_open_count--;
		vce_err("max vce_drv_open ...%d.\n", vce_open_count);
		mutex_unlock(&vce_ioctl_mutex);
		return -1;
	}

	info = (vce_info_t*)vce_malloc(sizeof(vce_info_t));
	printk("vce_drv: vce_open!info:%p,count:%d\n", info, vce_open_count);
	if (info == NULL) {
		vce_open_count--;
		vce_err("vce info malloc failed!...\n");
		mutex_unlock(&vce_ioctl_mutex);
		return -1;
	}

	/* init completion var */
	if (vce_open_count == 1)
		init_completion(&vce_complete);

	share_mem_reg_enable();
	vce_clk_enable(); /**new add*/
	disable_vce_irq();
	vce_info("vce disable_vce_irq ok\n");	
#if Enable_Debug_PrintK
	print_CMU_Reg("CMU_Reg");
#endif

	if (vce_open_count == 1) {
		vce_freq_multi.width = VCE_DEFAULT_WIDTH;
		vce_freq_multi.height = VCE_DEFAULT_HEIGHT;
		vce_freq_multi.freq = VCE_DEFAULT_FREQ;
		ret_frep = vce_setFreq(&vce_freq_multi);
		if (ret_frep < 0) {
			vce_freq_is_init = 0;	
			vce_open_count--;
			vce_free(info);
			share_mem_reg_disable();
			vce_clk_disable();
			mutex_unlock(&vce_ioctl_mutex);
			vce_err("freq_init to %d MHZ fail %d\n",VCE_DEFAULT_FREQ, vce_open_count);
			return -1;
		}
		vce_info("freq_init to %d MHZ!!\n", ret_frep);
  }
  
	pAbuf[vce_open_count - 1] = (void *)info;
	info->vce_count = vce_open_count; /*当前vce info序列号，从1开始*/
	info->vce_status = STOPED;
	filp->private_data = (void *)info;
	mutex_unlock(&vce_ioctl_mutex);
	printk("vce_drv: out of vce_drv_open ...%d.\n", vce_open_count);

	return 0;
}

int vce_release(struct inode *inode, struct file *filp)
{
	vce_info_t *info = (vce_info_t *) filp->private_data;
	printk("vce_drv: vce_drv_release..count:%d,info:%p\n",
		vce_open_count, info);

	if (info == NULL) {
		vce_err("Vce Info is Null ,return\n");
		return 0;
	}
	mutex_lock(&vce_ioctl_mutex);
	
	if (vce_open_count >= 1) 
		pAbuf_release(info->vce_count);
	
	vce_open_count--;
	
	if (vce_open_count >= 0) {
			vce_free(info);
			info = filp->private_data = NULL;
	} else if (vce_open_count < 0) {
		vce_err("count %d,%p\n",
			vce_open_count, info);
		vce_open_count = 0;
	}

	vce_stop();
	if (vce_open_count == 0) {
		vce_freq_multi.width = VCE_DEFAULT_WIDTH;
		vce_freq_multi.height = VCE_DEFAULT_HEIGHT;
		vce_freq_multi.freq = VCE_DEFAULT_FREQ;
		vce_freq_is_init = 0;

		vce_last_handle = 0;
		disable_vce_irq();
		share_mem_reg_disable();
	}

	vce_clk_disable();
	mutex_unlock(&vce_ioctl_mutex);

	return 0;
}

/* 进入低功耗之前，必须保证已经解码完当前帧。
 *
 */
int vce_suspend(struct platform_device *dev, pm_message_t state)
{
	vce_info("vce_suspend in %d,%d\n",
		vce_clk_isEnable, vce_open_count);
	mutex_lock(&vce_ioctl_mutex);
	if ((vce_open_count > 0) && (vce_clk_isEnable == 1)) {
		vce_stop();		
		disable_vce_irq();		
		share_mem_reg_disable();		
		vce_clk_disable();		
		vce_info("vce_suspend!clk disable!\n");
	}	
	disable_irq(irq_vce);
	if (!IS_ERR(power)){
	    regulator_disable(power);
		vce_info("vce_suspend!vdd regulator disable!\n");
	}
	mutex_unlock(&vce_ioctl_mutex);
	vce_info("vce_suspend out %d,%d\n",
		vce_clk_isEnable, vce_open_count);
	return 0;
}

int vce_resume(struct platform_device *dev)
{
	vce_info("vce_resume in %d,%d\n",
		vce_clk_isEnable, vce_open_count);
	mutex_lock(&vce_ioctl_mutex);
	vce_resumed_flag = 1;
	if ((vce_open_count > 0) && (vce_clk_isEnable == 0)) {
		share_mem_reg_enable();
		vce_clk_enable();
		disable_vce_irq();	
	} else {	
		/*
			there may be invalid vce interrupt when cpu resume, though vce module is not enabled;
			vce isr is crashed due to the non-initialized variable vce_complete;
			to clear the invalid vce interrupt,  enable the vce module first , clear vce pending bit, disable the module at last.		
		*/
		share_mem_reg_enable();
		module_reset(MOD_ID_VCE);
		module_clk_enable(MOD_ID_VCE);		
		disable_vce_irq();
		module_clk_disable(MOD_ID_VCE);		
		share_mem_reg_disable();	
	}

	enable_irq(irq_vce);
    if (!IS_ERR(power)){
	    if(regulator_enable(power)!=0){
            vce_err("vce_resume!vdd regulator err!\n");
            return -1;
	    }
		vce_info("vce_resume!vdd regulator enable!\n");
	}
	vce_resumed_flag = 0;
	mutex_unlock(&vce_ioctl_mutex);
	vce_info("vce_resume out %d,%d\n",
		vce_clk_isEnable, vce_open_count);
	return 0;
}

struct ic_info {
	int ic_type;
};

static struct ic_info atm7039_data = {
   .ic_type = IC_TYPE_ATM7039,
};
 
static  struct ic_info atm7021_data = {
   .ic_type = IC_TYPE_ATM7021,
};

static  struct ic_info atm6082_data = {
   .ic_type = IC_TYPE_ATM7021,
};

static const struct of_device_id owl_vce_of_match[] = {
	{.compatible = "actions,atm7039c-vce", .data = &atm7039_data},
	{.compatible = "actions,atm7059a-vce", .data = &atm7021_data},
	{.compatible = "actions,atm7059tc-vce", .data = &atm6082_data},
	{}
};

MODULE_DEVICE_TABLE(of, owl_vce_of_match);

static int vce_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	const struct of_device_id *id = of_match_device(owl_vce_of_match, &pdev->dev);
	if(id != NULL) {
		struct ic_info * info = (struct ic_info *)id->data;	
		if(info != NULL){
			ic_type = info->ic_type;
			vce_info("ic_type(0x%x)!\n",ic_type);
		}else{
			vce_warning("info is null!\n");
		}
		//platform_set_drvdata(pdev, info);
	}
	else {
		vce_warning("id is null!\n");
	}
	
	irq_vce = platform_get_irq(pdev, 0);
	vce_info("vce->irq =%d\n",irq_vce);
	if (irq_vce < 0)
		return irq_vce;

	ret = devm_request_irq(&pdev->dev, irq_vce, (void *) vce_ISR, 0, "vce_isr", 0);
	if (ret) {
		vce_err("register vce irq failed!...\n");
		return ret;
	} else {
		vce_irq_registered = 1;
	}
	//vce_info("aft request_irq ...\n");

	power = devm_regulator_get(&pdev->dev,"corevdd");
	vce_info("power:%p\n",power);

	if (IS_ERR(power))
	{
		/*6082样机corevdd不可调，所以在dts里面不配置，这里也获取不到，也不会进入设置电压的流程*/
		vce_warning("cannot get corevdd regulator,may be this board not need, or lost in dts!\n");
	}
	else
	{
		if(regulator_set_voltage(power, VOL_INIT, VOL_MAX))
		{
	        vce_err("cannot set corevdd to %duV !\n",VOL_INIT);
	        return -1;
	    }
		vce_info("init vdd:%d\n",VOL_INIT);
	    if(regulator_enable(power)!=0)
	    {
            vce_err("vdd regulator err!\n");
            return -1;
	    }
		vce_info("vdd regulator enable!\n");
	}
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res != NULL){
		if( request_mem_region(res->start, resource_size(res), "vce") != NULL) {
			
			/*res->start:VCE_BASE_ADDR;size:(256,gl5203) or (148,gl5206)*/
			vce_info("vce_probe!start = %p,size = %d!\n",(void*)res->start, resource_size(res));
			iobase_vce = (unsigned long)ioremap(res->start, resource_size(res));
			vce_info("vce_probe!iobase_vce = %p!\n",(void*)iobase_vce);
			if (iobase_vce == 0) {
				vce_err("iobase_vce is NULL!\n");
				goto err;
			}
			
			iobase_sram = (unsigned long)ioremap(Share_Mem_REG, 4);
			vce_info("vce_probe!iobase_sram = %p!\n",(void*)iobase_sram);
			if (iobase_sram == 0) {
				vce_err("iobase_sram is NULL!\n");
				goto err;
			}
			
			iobase_cmu_devclken0 = (unsigned long)ioremap(CMU_DEVCLKEN0, 4);
			vce_info("vce_probe!iobase_cmu_devclken0 = %p!\n",(void*)iobase_cmu_devclken0);
			if (iobase_cmu_devclken0 == 0) {
				vce_err("iobase_cmu_devclken0 is NULL!\n");
				goto err;
			}
			
			iobase_sps_pg_ctl = (unsigned long)ioremap(SPS_PG_CTL, 4);
			vce_info("vce_probe!iobase_sps_pg_ctl = %p!\n",(void*)iobase_sps_pg_ctl);
			if (iobase_sps_pg_ctl == 0) {
				vce_err("iobase_sps_pg_ctl is NULL!\n");
				goto err;
			}
				
#if Enable_Debug_PrintK
			iobase_cmu_devrst0 = (unsigned long)ioremap(CMU_DEVRST0, 4);
			vce_info("vce_probe!iobase_cmu_devrst0 = %p!\n",(void*)iobase_cmu_devrst0);
			if (iobase_cmu_devrst0 == 0) {
				vce_err("iobase_cmu_devrst0 is NULL!\n");
				goto err;
			}
				
			iobase_cmu_vceclk = (unsigned long)ioremap(CMU_VCECLK, 4);
			vce_info("vce_probe!iobase_cmu_vceclk = %p!\n",(void*)iobase_cmu_vceclk);
			if (iobase_cmu_vceclk == 0) {
				vce_err("iobase_cmu_vceclk is NULL!\n");
				goto err;
			}
			
			iobase_sps_pg_ack = (unsigned long)ioremap(SPS_PG_ACK, 4);
			vce_info("vce_probe!iobase_sps_pg_ack = %p!\n",(void*)iobase_sps_pg_ack);
			if (iobase_sps_pg_ack == 0) {
				vce_err("iobase_sps_pg_ack is NULL!\n");
				goto err;
			}
				
			iobase_cmu_devpll = (unsigned long)ioremap(CMU_DEVPLL, 4);
			vce_info("vce_probe!iobase_cmu_devpll = %p!\n",(void*)iobase_cmu_devpll);
			if (iobase_cmu_devpll == 0) {
				vce_err("iobase_cmu_devpll is NULL!\n");
				goto err;
			}
			
			iobase_cmu_ddrpll = (unsigned long)ioremap(CMU_DDRPLL, 4);
			vce_info("vce_probe!iobase_cmu_ddrpll = %p!\n",(void*)iobase_cmu_ddrpll);
			if (iobase_cmu_ddrpll == 0) {
				vce_err("iobase_cmu_ddrpll is NULL!\n");
				goto err;
			}
			
			iobase_cmu_displaypll = (unsigned long)ioremap(CMU_DISPLAYPLL, 4);
			vce_info("vce_probe!iobase_cmu_displaypll = %p!\n",(void*)iobase_cmu_displaypll);
			if (iobase_cmu_displaypll == 0) {
				vce_err("iobase_cmu_displaypll is NULL!\n");
				goto err;
			}
#endif
			
		}else{
			vce_err("request_mem_region is fail!\n");
			return -1;
		}
	}else{
		vce_err("res is null!\n");
		return -1;
	}
	
	return 0;
	
	err:
	if(iobase_vce != 0){iounmap((void *)iobase_vce);iobase_vce = 0;};
	if(iobase_sram != 0){iounmap((void *)iobase_sram);iobase_sram = 0;};
	if(iobase_cmu_devclken0 != 0){iounmap((void *)iobase_cmu_devclken0);iobase_cmu_devclken0 = 0;}
	if(iobase_sps_pg_ctl != 0){iounmap((void *)iobase_sps_pg_ctl);iobase_sps_pg_ctl = 0;}
#if Enable_Debug_PrintK
	if(iobase_cmu_devrst0 != 0){iounmap((void *)iobase_cmu_devrst0);iobase_cmu_devrst0 = 0;}
	if(iobase_cmu_vceclk != 0){iounmap((void *)iobase_cmu_vceclk);iobase_cmu_vceclk = 0;}
	if(iobase_sps_pg_ack != 0){iounmap((void *)iobase_sps_pg_ack);iobase_sps_pg_ack = 0;}
	if(iobase_cmu_devpll != 0){iounmap((void *)iobase_cmu_devpll);iobase_cmu_devpll = 0;}
	if(iobase_cmu_ddrpll != 0){iounmap((void *)iobase_cmu_ddrpll);iobase_cmu_ddrpll = 0;}
	if(iobase_cmu_displaypll != 0){iounmap((void *)iobase_cmu_displaypll);iobase_cmu_displaypll = 0;}
#endif
	return -1;
}

static int vce_remove(struct platform_device *pdev)
{
	struct resource *res;
	
	if (!IS_ERR(power))
	{
	    regulator_disable(power);
		vce_info("vdd regulator disable!\n");
	}
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res) {
		vce_info("vce_remove!start = %p,size = %d!\n",(void*)res->start, resource_size(res));
		release_mem_region(res->start, resource_size(res));
	}else{
		vce_warning("res is null!\n");
	}

	vce_info("vce_remove!iobase_vce = %p!\n",(void*)iobase_vce);
	if(iobase_vce != 0){iounmap((void *)iobase_vce);iobase_vce = 0;}
	if(iobase_sram != 0){iounmap((void *)iobase_sram);iobase_sram = 0;};
	if(iobase_cmu_devclken0 != 0){iounmap((void *)iobase_cmu_devclken0);iobase_cmu_devclken0 = 0;}
	if(iobase_sps_pg_ctl != 0){iounmap((void *)iobase_sps_pg_ctl);iobase_sps_pg_ctl = 0;}
#if Enable_Debug_PrintK
	if(iobase_cmu_devrst0 != 0){iounmap((void *)iobase_cmu_devrst0);iobase_cmu_devrst0 = 0;}
	if(iobase_cmu_vceclk != 0){iounmap((void *)iobase_cmu_vceclk);iobase_cmu_vceclk = 0;}
	if(iobase_sps_pg_ack != 0){iounmap((void *)iobase_sps_pg_ack);iobase_sps_pg_ack = 0;}
	if(iobase_cmu_devpll != 0){iounmap((void *)iobase_cmu_devpll);iobase_cmu_devpll = 0;}
	if(iobase_cmu_ddrpll != 0){iounmap((void *)iobase_cmu_ddrpll);iobase_cmu_ddrpll = 0;}
	if(iobase_cmu_displaypll != 0){iounmap((void *)iobase_cmu_displaypll);iobase_cmu_displaypll = 0;}
#endif
	return 0;
}

static const struct file_operations vce_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = vce_ioctl,
	.open = vce_open,
	.release = vce_release,
};

#ifndef CONFIG_OF
static void vce_platform_device_release(struct device * dev)
{
  return ;
}

static struct platform_device vce_platform_device = {
	.name = DEVDRV_NAME_VCE,
	.id = -1,
	.dev = {
		.release = vce_platform_device_release,
	}, 
	
};
#endif

static struct platform_driver vce_platform_driver = {
	.driver = {
		.name = DEVDRV_NAME_VCE,
		.owner = THIS_MODULE,
		.of_match_table = owl_vce_of_match,
	},
	.probe = vce_probe,
	.remove = vce_remove,
	.suspend = vce_suspend,
	.resume = vce_resume,
};

static struct miscdevice vce_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVDRV_NAME_VCE,
	.fops = &vce_fops,
};

static int vce_init(void)
{
	int ret;

	/*自动insmod，注册设备*/
	ret = misc_register(&vce_miscdevice);
	if (ret) {
		vce_err("register vce misc device failed!...\n");
		goto err0;
	}
	vce_info("vce_init!(ic_type:0x%x)......\n",ic_type);

#ifndef CONFIG_OF
	ret = platform_device_register(&vce_platform_device);
	if (ret) {
		vce_err("register vce platform_device error!...\n");
		goto err1;
	}
#endif

	ret = platform_driver_register(&vce_platform_driver);
	if (ret) {
		vce_err("register vce platform driver error!...\n");
		goto err2;
	}

	init_waitqueue_head(&vce_wait);

	return 0;

err2: 
    
#ifndef CONFIG_OF
    platform_device_unregister(&vce_platform_device);
err1:
#endif

	misc_deregister(&vce_miscdevice);

err0: return ret;
}

static void vce_exit(void)
{
	vce_info("vce_exit!(ic_type:0x%x)......\n",ic_type);
	misc_deregister(&vce_miscdevice);
#ifndef CONFIG_OF
	platform_device_unregister(&vce_platform_device);
#endif
	platform_driver_unregister(&vce_platform_driver);
}

module_init(vce_init);
module_exit(vce_exit);

MODULE_LICENSE("GPL");

