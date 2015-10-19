//#include <asm/mach-actions/actions_soc.h>//T:\mydroid_23\actsdk\psp\kernel\linux-2.6.35\arch\mips\include\asm\mach-actions
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/errno.h>    /* error codes */
#include <linux/vmalloc.h>
#include <linux/init.h>     /* module_init/module_exit */
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <mach/hardware.h>
#include <linux/of.h>
#include <mach/irqs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/prom.h>
#include "bisp.h"
#define CLT_CH_1

#define DEVDRV_NAME_BISP      "bisp"       //device_driver.h
#define DEVICE_BISP           "/dev/bisp"  //major.h


#define PUT_USER                        put_user
#define GET_USER                        get_user
//#define GET_USER_STRUCT                 copy_from_user
//#define SET_USER_STRUCT                 copy_to_user

#define  		ISP_BASE_PHY  0xb0270000
#define     ISP_CB_TIME_OFFSET                                                       (0x00)
#define     ISP_ENABLE_OFFSET                                                        (0x10)
#define     ISP_CTL_OFFSET                                                           (0x20)
#define     ISP_CHANNEL_1_STATE_OFFSET                                               (0x24)
#define     ISP_CHANNEL_1_BA_OFFSET_OFFSET                                           (0x28)
#define     ISP_CHANNEL_1_ROW_RANGE_OFFSET                                           (0x2C)
#define     ISP_CHANNEL_1_COL_RANGE_OFFSET                                           (0x30)
#define     ISP_CHANNEL_1_ADDR0_OFFSET                                               (0x34)//~0x50

#define     VNC_LUT_PORT_OFFSET                                                      (0x54)
#define     VNC_RST_OFFSET                                                           (0x58)
#define     ISP_LSC_LUT_PORT_OFFSET                                                  (0x60)
#define     ISP_LSC_PIX_INC_OFFSET                                                   (0x64)
#define     ISP_LSC_SCALING_OFFSET                                                   (0x68)
#define     ISP_LSC_CENTER0_OFFSET                                                   (0x6C)
#define     ISP_LSC_CENTER1_OFFSET                                                   (0x70)
#define     ISP_LSC_RST_OFFSET                                                       (0x74)
#define     ISP_NR_CONTROL_OFFSET                                                    (0x80)
#define     ISP_DPC_THRESHOLD_OFFSET                                                 (0x88)
#define     ISP_CG_B_GAIN_OFFSET                                                     (0x90)
#define     ISP_CG_G_GAIN_OFFSET                                                     (0x94)
#define     ISP_CG_R_GAIN_OFFSET                                                     (0x98)
#define     ISP_STAT_REGION_Y_OFFSET                                                 (0xA0)
#define     ISP_STAT_REGION_X_OFFSET                                                 (0xA4)
#define     ISP_STAT_HIST_Y_OFFSET                                                   (0xA8)
#define     ISP_STAT_HIST_X_OFFSET                                                   (0xAC)
#define     ISP_WB_THRESHOLD_OFFSET                                                  (0xB0)
#define     ISP_CSC_OFFSET1_OFFSET                                                   (0xC0)
#define     ISP_CSC_OFFSET2_OFFSET                                                   (0xC4)
#define     ISP_CSC_OFFSET3_OFFSET                                                   (0xC8)
#define     ISP_CSC_Y_R_OFFSET                                                       (0xCC)
#define     ISP_CSC_Y_G_OFFSET                                                       (0xD0)
#define     ISP_CSC_Y_B_OFFSET                                                       (0xD4)
#define     ISP_CSC_CB_R_OFFSET                                                      (0xD8)
#define     ISP_CSC_CB_G_OFFSET                                                      (0xDC)
#define     ISP_CSC_CB_B_OFFSET                                                      (0xE0)
#define     ISP_CSC_CR_R_OFFSET                                                      (0xE4)
#define     ISP_CSC_CR_G_OFFSET                                                      (0xE8)
#define     ISP_CSC_CR_B_OFFSET                                                      (0xEC)
#define     ISP_CSC_CONTROL_OFFSET                                                  (0xF0)
#define     ISP_GC_COEFF_0_OFFSET                                                    (0x100)//~0x13c

#define     ISP_OUT_FMT_OFFSET                                                       (0x148)
#define     ISP_OUT_ADDRY_OFFSET                                                     (0x14C)
#define     ISP_OUT_ADDRU_OFFSET                                                     (0x150)
#define     ISP_OUT_ADDRV_OFFSET                                                     (0x154)
#define     ISP_OUT_ADDR1UV_OFFSET                                                   (0x158)
#define     ISP_STAT_ADDR_OFFSET                                                     (0x160)
#define     ISP_COLOR_REPLACE1_OFFSET                                                (0x170)//~0x1ac

#define     ISP2_CTL_OFFSET                                                          (0x1EC)
#define     ISP_CHANNEL_2_STATE_OFFSET                                               (0x1F0)
#define     ISP_CHANNEL_2_BA_OFFSET_OFFSET                                          (0x1F4)
#define     ISP_CHANNEL_2_ROW_RANGE_OFFSET                                           (0x1F8)
#define     ISP_CHANNEL_2_COL_RANGE_OFFSET                                           (0x1FC)
#define     ISP_CHANNEL_2_ADDR0_OFFSET                                               (0x200)
//#define     ISP_CHANNEL_2_ADDR1_OFFSET                                               (0x204)//~0x21c

#define     ISP2_LSC_LUT_PORT_OFFSET                                                 (0x22C)
#define     ISP2_LSC_PIX_INC_OFFSET                                                  (0x230)
#define     ISP2_LSC_SCALING_OFFSET                                                  (0x234)
#define     ISP2_LSC_CENTER0_OFFSET                                                  (0x238)
#define     ISP2_LSC_CENTER1_OFFSET                                                  (0x23C)
#define     ISP2_LSC_RST_OFFSET                                                      (0x240)
#define     ISP2_NR_CONTROL_OFFSET                                                   (0x24C)
#define     ISP2_DPC_THRESHOLD_OFFSET                                                (0x254)
#define     ISP2_CG_B_GAIN_OFFSET                                                    (0x25C)
#define     ISP2_CG_G_GAIN_OFFSET                                                    (0x260)
#define     ISP2_CG_R_GAIN_OFFSET                                                    (0x264)
#define     ISP2_STAT_REGION_Y_OFFSET                                                (0x26C)
#define     ISP2_STAT_REGION_X_OFFSET                                                (0x270)
#define     ISP2_STAT_HIST_Y_OFFSET                                                  (0x274)
#define     ISP2_STAT_HIST_X_OFFSET                                                  (0x278)
#define     ISP2_WB_THRESHOLD_OFFSET                                                 (0x27C)
#define     ISP2_CSC_OFFSET1_OFFSET                                                  (0x28C)
#define     ISP2_CSC_OFFSET2_OFFSET                                                  (0x290)
#define     ISP2_CSC_OFFSET3_OFFSET                                                  (0x294)
#define     ISP2_CSC_Y_R_OFFSET                                                      (0x298)
#define     ISP2_CSC_Y_G_OFFSET                                                      (0x29C)
#define     ISP2_CSC_Y_B_OFFSET                                                      (0x2A0)
#define     ISP2_CSC_CB_R_OFFSET                                                     (0x2A4)
#define     ISP2_CSC_CB_G_OFFSET                                                     (0x2A8)
#define     ISP2_CSC_CB_B_OFFSET                                                     (0x2AC)
#define     ISP2_CSC_CR_R_OFFSET                                                     (0x2B0)
#define     ISP2_CSC_CR_G_OFFSET                                                     (0x2B4)
#define     ISP2_CSC_CR_B_OFFSET                                                     (0x2B8)
#define     ISP2_CSC_CONTROL_OFFSET                                                  (0x2BC)
#define     ISP2_GC_COEFF_0_OFFSET                                                   (0x2CC)//~0x308

#define     ISP2_OUT_FMT_OFFSET                                                      (0x314)
#define     ISP2_OUT_ADDRY_OFFSET                                                    (0x318)
#define     ISP2_OUT_ADDRU_OFFSET                                                    (0x31C)
#define     ISP2_OUT_ADDRV_OFFSET                                                    (0x320)
#define     ISP2_OUT_ADDR1UV_OFFSET                                                  (0x324)
#define     ISP2_STAT_ADDR_OFFSET                                                    (0x32C)
#define     ISP2_COLOR_REPLACE1_OFFSET                                               (0x33C)//~0x398

#define     ISP_FREEMODE_IMAGE_SIZE_OFFSET                                           (0x398)
#define     ISP_FREEMODE_SET_OFFSET                                                  (0x39C)
#define     ISP_FREEMODE_ADDRESS_OFFSET                                              (0x3A0)
#define     AF_ENABLE_OFFSET                                                         (0x3A4)
#define     AF_CTRL_OFFSET                                                           (0x3A8)
#define     AF_WP0_OFFSET                                                            (0x3AC)//~0x3cc
#define     AF_FV0_OFFSET                                                            (0x3D0)//~0x3f0
#define     AF_TEN_THOD_OFFSET                                                       (0x3F4)
#define     ISP_INT_STAT_OFFSET                                                      (0x400)

#define     BISP_FRONT_SENSOR 0
#define     BISP_REAR_SENSOR  1

enum{
	UNUSED,
	FREED,
	PREPARED,
	ACTIVED,
	READY,	
	USED,
};


typedef struct{
	unsigned int pHyaddr;
	unsigned int pViraddr;
	int					 mState;
	short				 Pred;
	short				 Next;
}bisp_buffer_node_t;

typedef struct{
	int ch_id;
	int open_count;
	int total_node_num;
	int ready_num;
	int free_idx;
	int read_idx;
	int baf_enabled;
	af_pv_t				af_sharpness;
	af_param_t    af_param;
	unsigned int  af_count;
	unsigned int  af_status;
	unsigned int  af_ready;
	bisp_buffer_node_t buffer_queue[16];
}bisp_internal_t;

static unsigned int bisp_drv_opened = 0;

typedef struct{
	unsigned int id;
	unsigned int update_awb;
	unsigned int rgain;
	unsigned int ggain;
	unsigned int bgain;
	unsigned int update_gamma;
	unsigned int gamma_tatle[16];
	unsigned int updata_csc;
	unsigned int csc_table[9];
	unsigned int on_running;
}isp_realtime_gain_t;

typedef struct{
	unsigned int stat_addr;
	unsigned int rgain;
	unsigned int ggain;
	unsigned int bgain;
}stat_updata_t;

typedef struct{
	unsigned int bset_alogs;
	unsigned int alogs_param;
	unsigned int pv_num;
	unsigned int bset_pvv;
	unsigned int pvv[9];
	unsigned int bset_pvs;
	unsigned int pvs[9];
	unsigned int noise_th;
	unsigned int noise_set;
}af_pv_data_t;

static af_pv_data_t af_pv_data;
static unsigned int af_enable_count = 0;

typedef struct {
    unsigned int channel;
    const char *name;
} bisp_sensor_info_t;

typedef struct {
    int rear;  /* 1: exist, 0: or not */
    int front;
    bisp_sensor_info_t sinfo[2];  /* 0: front, 1: rear */
} bisp_dts_t;

static bisp_dts_t g_bisp_dts;

#define AF_ISP_REG_BASE                    0xb02703A4 //Actions_reg_gl5202.h
#define AF_ISP_ENABLE                     (AF_ISP_REG_BASE+0x00)
#define AF_ISP_CTRL                       (AF_ISP_REG_BASE+0x04)//
#define AF_ISP_WP0                        (AF_ISP_REG_BASE+0x08)
#define AF_ISP_WP1                        (AF_ISP_REG_BASE+0x0C)
#define AF_ISP_WP2                        (AF_ISP_REG_BASE+0x10)
#define AF_ISP_WP3                        (AF_ISP_REG_BASE+0x14)
#define AF_ISP_WP4                        (AF_ISP_REG_BASE+0x18)
#define AF_ISP_WP5                        (AF_ISP_REG_BASE+0x1C)
#define AF_ISP_WP6                        (AF_ISP_REG_BASE+0x20)
#define AF_ISP_WP7                        (AF_ISP_REG_BASE+0x24)
#define AF_ISP_WP8                        (AF_ISP_REG_BASE+0x28)

#if 1
#define AF_ISP_FV0                        (AF_ISP_REG_BASE+0x2C)
#define AF_ISP_FV1                        (AF_ISP_REG_BASE+0x30)
#define AF_ISP_FV2                        (AF_ISP_REG_BASE+0x34)
#define AF_ISP_FV3                        (AF_ISP_REG_BASE+0x38)
#define AF_ISP_FV4                        (AF_ISP_REG_BASE+0x3C)
#define AF_ISP_FV5                        (AF_ISP_REG_BASE+0x40)
#define AF_ISP_FV6                        (AF_ISP_REG_BASE+0x44)
#define AF_ISP_FV7                        (AF_ISP_REG_BASE+0x48)
#define AF_ISP_FV8                        (AF_ISP_REG_BASE+0x4C)
#define AF_ISP_TEN_THLD                        (AF_ISP_REG_BASE+0x50)
#else

#define AF_ISP1_REG_BASE                  0xb0270034 //Actions_reg_gl5202.h
#define AF_ISP2_REG_BASE                  0xb0270200 //Actions_reg_gl5202.h
#define AF_ISP1_ST                         (AF_ISP1_REG_BASE+0x00)
#define AF_ISP1_FV0                        (AF_ISP1_REG_BASE+0x04)
#define AF_ISP1_FV1                        (AF_ISP1_REG_BASE+0x08)
#define AF_ISP1_FV2                        (AF_ISP1_REG_BASE+0x0c)
#define AF_ISP1_FV3                        (AF_ISP1_REG_BASE+0x10)
#define AF_ISP1_FV4                        (AF_ISP1_REG_BASE+0x14)
#define AF_ISP1_FV5                        (AF_ISP1_REG_BASE+0x18)
#define AF_ISP1_FV6                        (AF_ISP1_REG_BASE+0x1c)

#define AF_ISP2_ST                         (AF_ISP2_REG_BASE+0x00)
#define AF_ISP2_FV0                        (AF_ISP2_REG_BASE+0x04)
#define AF_ISP2_FV1                        (AF_ISP2_REG_BASE+0x08)
#define AF_ISP2_FV2                        (AF_ISP2_REG_BASE+0x0c)
#define AF_ISP2_FV3                        (AF_ISP2_REG_BASE+0x10)
#define AF_ISP2_FV4                        (AF_ISP2_REG_BASE+0x14)
#define AF_ISP2_FV5                        (AF_ISP2_REG_BASE+0x18)
#define AF_ISP2_FV6                        (AF_ISP2_REG_BASE+0x1c)

#endif



#define AF_ISP_TEN_THOD                   (AF_ISP_REG_BASE+0x50) //noise throld
#define AF_ISP_STAT                       (AF_ISP_REG_BASE+0x5c)
#define _CH_MX_NUM_ 64
static isp_realtime_gain_t isp_rl_gain[_CH_MX_NUM_];
static DEFINE_MUTEX(gisp_cmd_mutex);
static stat_updata_t gstat_data[_CH_MX_NUM_][2];
static bisp_internal_t *gbisp_fifo[_CH_MX_NUM_] = {0};

static int finder_ch(int ch_id){
	int i = 0;
	int idx = -1;
	for(i = 0; i < _CH_MX_NUM_; i++){
		if(gbisp_fifo[i]){
			//printk("info cnt %d,%d\n",gbisp_fifo[i]->open_count,gbisp_fifo[i]->ch_id);
			if(gbisp_fifo[i]->ch_id == (ch_id+1)){
				idx = i;
				return idx;
			}
		}
	}
	return idx;
}

static void init_node(bisp_internal_t *bisp_fifo,bisp_stat_buffers_info_t *buffers_node){
	int i;
	//memset(bisp_fifo,0,sizeof(bisp_internal_t));
	bisp_fifo->total_node_num = 0;
	bisp_fifo->ready_num = 0;
	bisp_fifo->free_idx = 0;
	bisp_fifo->read_idx = 0;
	memset(bisp_fifo->buffer_queue,0,sizeof(bisp_buffer_node_t)*16);
	
	printk("init_node %d\n",buffers_node->buffer_num);
	for(i = 0; i < buffers_node->buffer_num; i++){
		bisp_fifo->buffer_queue[i].pHyaddr = buffers_node->pHyaddr[i];
		bisp_fifo->buffer_queue[i].pViraddr = buffers_node->pViaddr[i];
		printk("init_node adr %x\n",bisp_fifo->buffer_queue[i].pHyaddr);
		bisp_fifo->buffer_queue[i].mState = FREED;
		if(i == 0){
			bisp_fifo->buffer_queue[i].Pred  = buffers_node->buffer_num-1;
			bisp_fifo->buffer_queue[i].Next  = i+1;
		}
		else if(i == buffers_node->buffer_num-1){
			bisp_fifo->buffer_queue[i].Pred  = i-1;
			bisp_fifo->buffer_queue[i].Next  = 0;
		}
		else {
			bisp_fifo->buffer_queue[i].Pred  = i-1;
			bisp_fifo->buffer_queue[i].Next  = i+1;
		}			
	}
	bisp_fifo->free_idx = 0;
	bisp_fifo->read_idx = -1;
	bisp_fifo->total_node_num = buffers_node->buffer_num;
}

static void change_buffer_state(bisp_internal_t *bisp_fifo,unsigned int *pPhyAddr){
	int i,k;
	int fidx;
	int has_idx = -1;
	bisp_buffer_node_t *buff_node_tmp = NULL;	
	if(bisp_fifo->total_node_num == 0) return;
		
	for(i = 0; i < bisp_fifo->total_node_num; i++){
		buff_node_tmp = &bisp_fifo->buffer_queue[i];
		if(buff_node_tmp->mState == PREPARED){
			buff_node_tmp->mState = ACTIVED;
		}
		else if(buff_node_tmp->mState == ACTIVED){			
			buff_node_tmp->mState = READY;
			if(bisp_fifo->read_idx < 0){
				bisp_fifo->read_idx = i;
				bisp_fifo->ready_num = 1;
			}
			else {
				if(bisp_fifo->buffer_queue[bisp_fifo->read_idx].mState == READY){
					bisp_fifo->buffer_queue[bisp_fifo->read_idx].mState = FREED;
					bisp_fifo->read_idx = i;
					bisp_fifo->ready_num = 1;
				}else if(bisp_fifo->buffer_queue[bisp_fifo->read_idx].mState == USED){
					bisp_fifo->read_idx = i;
					bisp_fifo->ready_num = 1;
				}else if(bisp_fifo->buffer_queue[bisp_fifo->read_idx].mState == FREED){
					bisp_fifo->read_idx = i;
					bisp_fifo->ready_num = 1;
				}
			}
		}
	}
	*pPhyAddr = (unsigned int)NULL;		
	buff_node_tmp = (bisp_buffer_node_t*)&bisp_fifo->buffer_queue[bisp_fifo->free_idx];
	if(buff_node_tmp->mState == FREED){
		buff_node_tmp->mState = PREPARED;
		*pPhyAddr = buff_node_tmp->pHyaddr;
	}	
	else{
		printk("free_idx err %d,%d\n",bisp_fifo->free_idx,buff_node_tmp->mState);
		k = bisp_fifo->free_idx+1;
		fidx = bisp_fifo->free_idx;
		do{
			if(k >= bisp_fifo->total_node_num) k = 0;
			buff_node_tmp = &bisp_fifo->buffer_queue[k];
			if(buff_node_tmp->mState == FREED){
				bisp_fifo->free_idx = k;
				has_idx = 1;
				break;
			}
			k++;
			//printk("fidx %d\n",k);
		}while(k != fidx);
		if(has_idx == 1){
			buff_node_tmp->mState = PREPARED;
			*pPhyAddr = buff_node_tmp->pHyaddr;
		}else{
			printk("free_idx err2 %d,%d\n",bisp_fifo->free_idx,buff_node_tmp->mState);
		}
	}
	k = bisp_fifo->free_idx+1;
	fidx = bisp_fifo->free_idx;
	do{
		if(k >= bisp_fifo->total_node_num) k = 0;
		buff_node_tmp = &bisp_fifo->buffer_queue[k];
		if(buff_node_tmp->mState == FREED){
			bisp_fifo->free_idx = k;
			break;
		}
		k++;
		//printk("fidx %d\n",k);
	}while(k != fidx);
	
}

static bisp_buffer_node_t* get_buffer_node(bisp_internal_t *bisp_fifo){
	int idx = bisp_fifo->read_idx;
	if(bisp_fifo->read_idx == -1) return NULL;
	if(idx != bisp_fifo->read_idx) return NULL;
	if(bisp_fifo->buffer_queue[idx].mState == READY){
		bisp_fifo->buffer_queue[idx].mState = USED;
		//printk("fidx %d,%x\n",idx,bisp_fifo->buffer_queue[idx].pHyaddr);
		return &bisp_fifo->buffer_queue[idx];
	}else {
		return NULL;
	}
}

static void set_buffer_node(bisp_internal_t *bisp_fifo,int pHyaddr){
	int i;
	bisp_buffer_node_t *buff_node_tmp = NULL;		
	int bufidx = -1;
	//mutex_lock(&gisp_cmd_mutex);
	for(i = 0; i < bisp_fifo->total_node_num;i++){
		buff_node_tmp = &bisp_fifo->buffer_queue[i];
		if(pHyaddr == buff_node_tmp->pHyaddr){
			bufidx = i;
			i = bisp_fifo->total_node_num + 1;
		}
	}
	
	if(bufidx == -1){
		printk("set_buffer_node %d,%x,%x\n",bufidx,pHyaddr,bisp_fifo->total_node_num);
//		mutex_unlock(&gisp_cmd_mutex);
		 return;
		}
		
	if(bisp_fifo->buffer_queue[bufidx].mState == USED){
		bisp_fifo->buffer_queue[bufidx].mState = FREED;
	}
//	mutex_unlock(&gisp_cmd_mutex);
}

static unsigned int reg_read(unsigned int reg)
{
	unsigned int value = act_readl(reg);
	//printk(KERN_ERR"Read Reg:%x ,value:%x...\n",reg,value);
	return value;
}

static void reg_write(unsigned int reg,unsigned int value)
{
	//printk(KERN_ERR"Wr Reg:%x ,value:%x ...%x\n",reg,value,act_readl(0xb0210010));
	act_writel(value,reg);
	//printk(KERN_ERR"redo Read reg:%x,%x,%x...\n",reg,value,act_readl(reg));
}


static void ov5647_af_clk_enable(void)
{    
	printk(KERN_ERR"ov5647_af_clk_enable\n");
	//if (owl_powergate_is_powered(OWL_POWERGATE_VCE_BISP)){
  	act_writel(0x1,AF_ISP_ENABLE);
	//}
}

static void ov5647_af_clk_disable(void)
{    
	printk(KERN_ERR"ov5647_af_clk_disable\n");
  act_writel(0x0,AF_ISP_ENABLE);
}

/*
AF irq enable/disable
*/
static inline void ov5647_af_irq_enable(void)
{
	int af_status = reg_read(AF_ISP_STAT);
	af_status = af_status&0x000000ff;
	af_status = af_status|(1<<4)|(1<<12);	
	act_writel(af_status,AF_ISP_STAT);
}
    
static inline void ov5647_af_irq_disable(void)
{
  int af_status = reg_read(AF_ISP_STAT);
  if(af_status&(1<<12))
  {
  	af_status &= 0xff;
		af_status |= (1<<12);
		act_writel(af_status,AF_ISP_STAT);
	}
	else
	{
		printk("af irq err %x\n",af_status);
	}
}

/*
AF info read
*/
static int ov5647_af_fv_read(af_pv_t *af_wv,int win_num)
{
	int i;
	int ret = 0;
	if(af_pv_data.bset_pvv == 2){
		af_pv_data.bset_pvv = 1;
		for(i = 0; i < win_num+1;i ++){
			af_wv->pv[i] = af_pv_data.pvv[i];
		}
		af_pv_data.bset_pvv = 0;
		af_wv->bPrepared = 0;		
	}else {
		af_wv->bPrepared = -1;
	}
	
	printk("working status 0x%x,0x%x,0x%x\n",af_wv->pv[0],af_wv->pv[1],af_wv->pv[2]);
	return ret;
}

int af_updata(int s){
	int i = 0;
	if(af_pv_data.bset_alogs){
		act_writel(af_pv_data.alogs_param,AF_ISP_CTRL);
		af_pv_data.bset_alogs = 0;
	}
	if(af_pv_data.bset_pvs){
		for(i = 0; i < af_pv_data.pv_num; i++)
		{
			act_writel(af_pv_data.pvs[i],AF_ISP_WP0+i*4);
		}
		af_pv_data.bset_pvs = 0;
	}
	if(af_pv_data.noise_set == 1){
			act_writel(af_pv_data.noise_th,AF_ISP_TEN_THLD);
			af_pv_data.noise_set = 0;
		}
	if(af_pv_data.bset_pvv != 1){
		af_pv_data.bset_pvv = 3;
		for(i = 0; i < af_pv_data.pv_num; i++){
				af_pv_data.pvv[i] = reg_read(AF_ISP_FV0+i*4);
			}
			//printk("PV0 %x\n",af_pv_data.pvv[0]);
			af_pv_data.bset_pvv = 2;
	}	
	return 0;
}
EXPORT_SYMBOL(af_updata);

static void ov5647_af_param_set(bisp_internal_t *info,af_param_t *af_param)
{
	int maf_enable = 0;
	unsigned int preline = af_param->win_h/2 + 34;
	int i;
	//maf_enable = af_param->win_num | (af_param->win_size << 4)|(0 << 8)|(0 << 10)|(0 << 13)|(af_param->af_mode << 27) | (af_param->af_inc << 28)|(af_param->af_al << 30) | (preline << 16);

	maf_enable = af_param->win_num | (af_param->win_size << 4)|(af_param->tshift << 8)|(af_param->thold << 10)|(af_param->rshift << 13)|\
								(af_param->af_mode << 27) | (af_param->af_inc << 28)|(af_param->af_al << 30) | (preline << 16);
	af_pv_data.pv_num = af_param->win_num + 1;
	
	
	if(af_enable_count == 0){
		act_writel(maf_enable,AF_ISP_CTRL);
		act_writel(af_param->af_pos[0],AF_ISP_WP0);
		act_writel(af_param->af_pos[1],AF_ISP_WP1);
		act_writel(af_param->af_pos[2],AF_ISP_WP2);
		act_writel(af_param->af_pos[3],AF_ISP_WP3);
		act_writel(af_param->af_pos[4],AF_ISP_WP4);
		act_writel(af_param->af_pos[5],AF_ISP_WP5);
		act_writel(af_param->af_pos[6],AF_ISP_WP6);
		act_writel(af_param->af_pos[7],AF_ISP_WP7);
		act_writel(af_param->af_pos[8],AF_ISP_WP8);
		act_writel(af_param->noise_th,AF_ISP_TEN_THLD);
	}
	else {
		if(af_pv_data.alogs_param != maf_enable){
			af_pv_data.alogs_param = maf_enable;
			af_pv_data.bset_alogs = 1;				
		}
		if(af_param->noise_th != af_pv_data.noise_th){
			af_pv_data.noise_th = af_param->noise_th;
			af_pv_data.noise_set = 1;
		}
		for(i = 0; i < af_param->win_num; i++){
			af_pv_data.pvs[i] = af_param->af_pos[i];
		}
		af_pv_data.bset_pvs = 1;
	}	
	
	//ov5647_af_Wr_Reg(af_param->win_num+1,AF_ISP2_ST);
}
#if 0
static void ov5647_af_param_get(af_param_t *af_param){
		unsigned int maf_enable = reg_read(AF_ISP_CTRL);		
		
		af_param->win_num = maf_enable&0xf;
		af_param->win_size = (maf_enable>>4)&0x7;
		af_param->tshift = (maf_enable>>8)&0x3;
		af_param->thold = (maf_enable>>10)&0x7;
		af_param->rshift = (maf_enable>>13)&0x7;
		af_param->af_mode = (maf_enable>>27)&0x1;
		af_param->af_inc = (maf_enable>>28)&0x3;
		af_param->af_al = (maf_enable>>30)&0x1;
		
		af_param->af_pos[0] =  reg_read(AF_ISP_WP0);
		af_param->af_pos[1] =  reg_read(AF_ISP_WP1);
		af_param->af_pos[2] =  reg_read(AF_ISP_WP2);
		af_param->af_pos[3] =  reg_read(AF_ISP_WP3);
		af_param->af_pos[4] =  reg_read(AF_ISP_WP4);
		af_param->af_pos[5] =  reg_read(AF_ISP_WP5);
		af_param->af_pos[6] =  reg_read(AF_ISP_WP6);
		af_param->af_pos[7] =  reg_read(AF_ISP_WP7);
		af_param->af_pos[8] =  reg_read(AF_ISP_WP8);
		af_param->noise_th =  reg_read(AF_ISP_TEN_THLD);
}

static int ov5647_af_status_query(void)
{
   int af_status = reg_read(AF_ISP_ENABLE);
	 return af_status;
}
#endif

static int set_info(bisp_info_t *isp_info)
{
	//int ctl_id = isp_info->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
	
	reg_val = reg_val&(~(3<<11));
	reg_val = reg_val | (isp_info->stat_en << 11);// | (isp_info->eis_en << 12);
	printk(KERN_ERR"stat %x,%x\n",(unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);
	reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset, reg_val);
	
	return 0;
}
static int set_blc(blc_info_t *blc_info){
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_BA_OFFSET_OFFSET + reg_offset);
	/*
	typedef struct{
	int r_off;
	int g1_off;
	int g2_off;
	int b_off;
	}blc_info_t;
	*/
	reg_val = ((blc_info->r_off&0xff) << 24)|((blc_info->g1_off&0xff) << 16)|((blc_info->g2_off&0xff) << 8)|((blc_info->b_off&0xff) << 0);// | (isp_info->eis_en << 12);
	printk(KERN_ERR"set_blc %x,%x,%x,%x\n",blc_info->r_off,blc_info->b_off,(unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_BA_OFFSET_OFFSET + reg_offset,reg_val);
	reg_write((unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_BA_OFFSET_OFFSET + reg_offset, reg_val);
	
	return 0;
}

static int set_hvb(bisp_bank_t *isp_bank)
{
	unsigned int reg_val = 0;
	reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CB_TIME_OFFSET);
	
	reg_val = reg_val&(0xffff0000);
	reg_val = reg_val | (isp_bank->hbank << 0) | (isp_bank->vbank << 8);
	
	reg_write((unsigned int)ISP_BASE_PHY + ISP_CB_TIME_OFFSET,reg_val);
	
	return 0;
}

static int set_cop(bisp_region_t *isp_bar)
{
	//int ctl_id = isp_bar->chid;
	unsigned int reg_val0 = 0,reg_val1 = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_ROW_RANGE_OFFSET + reg_offset;
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_COL_RANGE_OFFSET + reg_offset;
	
	reg_val0 = reg_read(reg_addr_0);
	reg_val1 = reg_read(reg_addr_1);
	
	reg_val0 = (isp_bar->ey<<16) + (isp_bar->sy<<0);
	reg_val1 = (isp_bar->ex<<16) + (isp_bar->sx<<0);
	
	reg_write(reg_addr_0,reg_val0);
	reg_write(reg_addr_1,reg_val1);
	return 0;
}

static int get_cop(bisp_region_t *isp_bar)
{
	//int ctl_id = isp_bar->chid;
	unsigned int reg_val0 = 0,reg_val1 = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_ROW_RANGE_OFFSET + reg_offset;
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_CHANNEL_1_COL_RANGE_OFFSET + reg_offset;
	
	reg_val0 = reg_read(reg_addr_0);
	reg_val1 = reg_read(reg_addr_1);
	isp_bar->ey = reg_val0>>16;
	isp_bar->sy = reg_val0&0xffff;
	isp_bar->ex = reg_val1>>16;
	isp_bar->sx = reg_val1&0xffff;
	return 0;
}
#define STAT_ISP2_L1   (0xB0210218)
#define STAT_ISP2_L2   (0xB021021c)

static int get_sds(bisp_internal_t *bisp_fifo,bisp_ads_t *clt_param)
{
	#if 0
	int ctl_id = clt_param->chid;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_STAT_ADDR_OFFSET + reg_offset;
	clt_param->ads = reg_read(reg_addr_0);	
	clt_param->ready_addr = reg_read(STAT_ISP2_L2);
	clt_param->last_addr = reg_read(STAT_ISP2_L1);
	reg_write(STAT_ISP2_L1,0);
	
	#endif	
	
	return 0;
}

static int get_yds(bisp_ads_t *clt_param)
{
	//int ctl_id = clt_param->chid;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_OUT_ADDRY_OFFSET + reg_offset;
	clt_param->ads = reg_read(reg_addr_0);
	return 0;
}

static int set_vnc(bisp_vnc_t *vnc_info)
{
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + VNC_LUT_PORT_OFFSET;
	int i,j;
	unsigned int reg_val = 0;
	
	if(vnc_info->b_en){
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET);	
		//reg_val = reg_val & (~(1<<15));
		//reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET,reg_val);
		if(((reg_val >> 15)&0x1) == 0){
			reg_write((unsigned int)ISP_BASE_PHY+VNC_RST_OFFSET,1);//reset
			for(i = 0; i < 3; i++)
			{
				for(j =0; j< vnc_info->vnc_num; j++){
					reg_write((unsigned int)reg_addr_0,vnc_info->color_info[i][j]);//reset
					//printk(KERN_ERR"set_vnc val %x,%x\n",reg_addr_0,vnc_info->color_info[i][j]);
				}
				if(vnc_info->vnc_num < 128){
					for(j =0; j< 128 - vnc_info->vnc_num; j++)
						reg_write((unsigned int)reg_addr_0,0);//reset
				}
			}
			
			reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET);	
			reg_val = reg_val | ((1<<15));
			printk(KERN_ERR"set_vnc %x,%x\n",reg_val,vnc_info->vnc_num);
			reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET,reg_val);
		}
		
	}else {
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET);
		reg_val = reg_val & (~(1<<15));
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET,reg_val);
	}
	
	return 0;
}

static int set_lsc(bisp_lsc_t *lut_data)
{
	//int ctl_id = lut_data->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_LSC_PIX_INC_OFFSET + reg_offset;
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_LSC_SCALING_OFFSET + reg_offset;
	unsigned int reg_addr_2 = (unsigned int)ISP_BASE_PHY + ISP_LSC_CENTER0_OFFSET + reg_offset;
	unsigned int reg_addr_3 = (unsigned int)ISP_BASE_PHY + ISP_LSC_CENTER1_OFFSET + reg_offset;
	unsigned int reg_addr_4 = (unsigned int)ISP_BASE_PHY + ISP_LSC_RST_OFFSET + reg_offset;
	unsigned int reg_addr_5 = (unsigned int)ISP_BASE_PHY + ISP_LSC_LUT_PORT_OFFSET + reg_offset;
	int i;
	  
	if(lut_data->lsc_en){
		reg_write(reg_addr_0,(lut_data->hinc&0xf) + ((lut_data->vinc&0xf) << 4));
		reg_write(reg_addr_1,lut_data->scaleidx);
		reg_write(reg_addr_2,lut_data->xcenter + (lut_data->ycenter << 16));
		reg_write(reg_addr_3,lut_data->xcenter_1);
		reg_write(reg_addr_4,1);//reset
		printk(KERN_ERR"lsc %d,%d\n",lut_data->hinc,lut_data->vinc);
		for(i = 0; i < 64; i++){
			//printk(KERN_ERR"lsc %x,%x\n",lut_data->lsc_coeff[i],reg_read(0xB0210010));
			reg_write(reg_addr_5,lut_data->lsc_coeff[i]);
		}
		reg_write(reg_addr_4,0);//reset
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
		reg_val = reg_val & (~(1<<7));
		reg_val = reg_val | (1 << 7) ;
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);
	}
	else
	{
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
		reg_val = reg_val & (~(1<<7));
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);
	}

	return 0;
}

static int set_nr(bisp_nr_t *isp_nr)
{
	//int ctl_id = isp_nr->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_NR_CONTROL_OFFSET + reg_offset;
		
	reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
	reg_val = reg_val & (~(1<<8));
	reg_val = reg_val | (isp_nr->nr_en << 8);
	reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);

	reg_write(reg_addr_0,isp_nr->nr_level);
	
	return 0;
}

static int set_ee(bisp_nr_t *clt_param)
{
	//int ctl_id = clt_param->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
	reg_val = reg_val & (~(1<<9));
	reg_val = reg_val | ((clt_param->ee_en&0x1) << 9);// | (isp_nr->dpc_en << 10);
	reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);

	return 0;
}

static int set_dpc(bisp_nr_t *isp_nr)
{
	//int ctl_id = isp_nr->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_DPC_THRESHOLD_OFFSET + reg_offset;
	
	reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
	reg_val = reg_val & (~(4<<8));
	reg_val = reg_val |  (isp_nr->dpc_en << 10);//(isp_nr->nr_en << 8) |
	reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);
	reg_write(reg_addr_1,isp_nr->dpc_level);
	
	return 0;
}

static int set_srg(bisp_region_t *isp_bar)
{
	//int ctl_id = isp_bar->chid;
	unsigned int reg_val0 = 0,reg_val1 = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_STAT_REGION_Y_OFFSET + reg_offset;
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_STAT_REGION_X_OFFSET + reg_offset;
	
	
	reg_val0 = (isp_bar->ey<<16) + (isp_bar->sy<<0);
	reg_val1 = (isp_bar->ex<<16) + (isp_bar->sx<<0);
	
	reg_write(reg_addr_0,reg_val0);
	reg_write(reg_addr_1,reg_val1);
	
	return 0;
}

static int set_hrg(bisp_region_t *isp_bar)
{
	//int ctl_id = isp_bar->chid;
	unsigned int reg_val0 = 0,reg_val1 = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_STAT_HIST_Y_OFFSET + reg_offset;
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_STAT_HIST_X_OFFSET + reg_offset;
	
	reg_val0 = (isp_bar->ey<<16) + (isp_bar->sy<<0);
	reg_val1 = (isp_bar->ex<<16) + (isp_bar->sx<<0);
	
	reg_write(reg_addr_0, reg_val0);
	reg_write(reg_addr_1,reg_val1);
	
	return 0;
}

static int set_wbp(bisp_wp_t *isp_bar)
{
	//int ctl_id = isp_bar->chid;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_WB_THRESHOLD_OFFSET + reg_offset;
	
	reg_write(reg_addr_0,(isp_bar->wb_thr<<24) + (isp_bar->wb_thg<<16) +(isp_bar->wb_thb<<8) +(isp_bar->wb_cnt<<0));
	return 0;
}

static int set_csc(bisp_csc_t *lut_data)
{
	//int ctl_id = lut_data->chid;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CSC_OFFSET1_OFFSET + reg_offset;	
	
	reg_write(reg_addr_0,lut_data->offset1);
	reg_write(reg_addr_0 + 4, lut_data->offset2);
	reg_write(reg_addr_0 + 8, lut_data->offset3);
	reg_write(reg_addr_0 + 12, lut_data->y_r);
	reg_write(reg_addr_0 + 16, lut_data->y_g);
	reg_write(reg_addr_0 + 20, lut_data->y_b);
	reg_write(reg_addr_0 + 24, lut_data->cb_r);
	reg_write(reg_addr_0 + 28, lut_data->cb_g);
	reg_write(reg_addr_0 + 32, lut_data->cb_b);
	reg_write(reg_addr_0 + 36, lut_data->cr_r);
	reg_write(reg_addr_0 + 40, lut_data->cr_g);
	reg_write(reg_addr_0 + 44, lut_data->cr_b);
	return 0;
}

static int set_gc(bisp_gc_t *lut_data)
{
	//int ctl_id = lut_data->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_GC_COEFF_0_OFFSET + reg_offset;
	int i;
	
	if(lut_data->gc_en){		
		for(i = 0; i < 16; i++){
			reg_write(reg_addr_0 + i * 4, lut_data->gc_coeff[i]);
		}
		
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
		reg_val = reg_val & (~(1<<13));
		reg_val = reg_val | (1 << 13) ;
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset, reg_val);
	}
	else
	{
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
		reg_val = reg_val & (~(1<<13));
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset, reg_val);
	}

	return 0;
}

static int set_cfx(bisp_color_t *cr_data)
{
	//int ctl_id = cr_data->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CSC_CONTROL_OFFSET + reg_offset;
	
	//int i;
	reg_val = reg_read(reg_addr_0);

	if(cr_data->cr < 0)		reg_val = reg_val & (~(1<<2));
	else
	{
		reg_val = reg_val & 0x00ffffff;
		reg_val = reg_val | ((1<<2));
		reg_val = reg_val | (cr_data->cr << 24);
	}
	if(cr_data->cb < 0)		reg_val = reg_val & (~(1<<1));
	else
	{
		reg_val = reg_val & 0xff00ffff;
		reg_val = reg_val | ((1<<1));
		reg_val = reg_val | (cr_data->cb << 16);
	}

	if(cr_data->yinc <= 0) reg_val = reg_val & (~(1<<0));
	else reg_val = reg_val | (1<<0);

	reg_write(reg_addr_0, reg_val);
	
	return 0;
}

static int set_crp(bisp_color_adjust_t *lut_data)
{
	//int ctl_id = lut_data->chid;
	unsigned int reg_val = 0;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_COLOR_REPLACE1_OFFSET + reg_offset;
	int i;
	
	if(lut_data->b_en){		
		for(i = 0; i < 16; i++){
			reg_write(reg_addr_0 + i * 4, lut_data->color_info[i]);
		}
		
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
		reg_val = reg_val & (~(1<<14));
		reg_val = reg_val | (1 << 14) ;
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset,reg_val);
	}
	else
	{
		reg_val = reg_read((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset);
		reg_val = reg_val & (~(1<<14));
		reg_write((unsigned int)ISP_BASE_PHY + ISP_CTL_OFFSET + reg_offset, reg_val);
	}

	return 0;
}
#if 0
static int set_gn(bisp_bg_t *lut_data)
{
	int ctl_id = lut_data->chid;
#ifdef CLT_CH_1
	unsigned int reg_offset = 0;
#else
	unsigned int reg_offset = (ctl_id==2)?0x1cc:0;
#endif
	unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CG_B_GAIN_OFFSET + reg_offset;
	unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_CG_G_GAIN_OFFSET + reg_offset;
	unsigned int reg_addr_2 = (unsigned int)ISP_BASE_PHY + ISP_CG_R_GAIN_OFFSET + reg_offset;	
	
	reg_write(reg_addr_0,lut_data->bgain);
	reg_write(reg_addr_1, lut_data->ggain);
	reg_write(reg_addr_2, lut_data->rgain);
	return 0;
}
#endif

void bisp_updata(unsigned int *pHyaddr,int isp_ch){
	unsigned int pyaddr = 0;
	int ch_id = finder_ch(1);
	//printk("ch id %d,%d\n",ch_id,isp_ch);
	if(ch_id == -1) return;
	
	//mutex_lock(&gisp_cmd_mutex);
	isp_rl_gain[ch_id].on_running = 1;
	change_buffer_state(gbisp_fifo[ch_id],&pyaddr);	
	gstat_data[ch_id][0].stat_addr = gstat_data[ch_id][1].stat_addr;
	gstat_data[ch_id][0].rgain = gstat_data[ch_id][1].rgain;
	gstat_data[ch_id][0].ggain = gstat_data[ch_id][1].ggain;
	gstat_data[ch_id][0].bgain = gstat_data[ch_id][1].bgain;
	if(isp_ch == 1){
		gstat_data[ch_id][1].stat_addr = reg_read(0xb027032c);
		gstat_data[ch_id][1].rgain = reg_read(0xb0270264);
		gstat_data[ch_id][1].ggain = reg_read(0xb0270260);
		gstat_data[ch_id][1].bgain = reg_read(0xb027025c);
		if(pyaddr)
			reg_write(0xb027032c,pyaddr);		
	
		if(isp_rl_gain[ch_id].update_awb){
			unsigned int reg_offset = 0x1cc;
			unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CG_B_GAIN_OFFSET + reg_offset;
			unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_CG_G_GAIN_OFFSET + reg_offset;
			unsigned int reg_addr_2 = (unsigned int)ISP_BASE_PHY + ISP_CG_R_GAIN_OFFSET + reg_offset;		
			reg_write(reg_addr_0,isp_rl_gain[ch_id].bgain);
			reg_write(reg_addr_1, isp_rl_gain[ch_id].ggain);
			reg_write(reg_addr_2, isp_rl_gain[ch_id].rgain);
			isp_rl_gain[ch_id].update_awb = 0;
		}		
	}else if(isp_ch == 0){
		gstat_data[ch_id][1].stat_addr = reg_read(0xb0270160);
		gstat_data[ch_id][1].rgain = reg_read(0xb0270098);
		gstat_data[ch_id][1].ggain = reg_read(0xb0270094);
		gstat_data[ch_id][1].bgain = reg_read(0xb0270090);
		if(pyaddr)
			reg_write(0xb0270160,pyaddr);
		
	
		if(isp_rl_gain[ch_id].update_awb){
			unsigned int reg_offset = 0;
			unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CG_B_GAIN_OFFSET + reg_offset;
			unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_CG_G_GAIN_OFFSET + reg_offset;
			unsigned int reg_addr_2 = (unsigned int)ISP_BASE_PHY + ISP_CG_R_GAIN_OFFSET + reg_offset;		
			reg_write(reg_addr_0,isp_rl_gain[ch_id].bgain);
			reg_write(reg_addr_1, isp_rl_gain[ch_id].ggain);
			reg_write(reg_addr_2, isp_rl_gain[ch_id].rgain);
			isp_rl_gain[ch_id].update_awb = 0;
		}
	}
	isp_rl_gain[ch_id].on_running = 0;
	//printk("set pyaddr %x \n",pyaddr);
	//mutex_unlock(&gisp_cmd_mutex);
}

void isp_set_gain(bisp_bg_t *lut_data,int ch_id){
	int i = 1000;
//	mutex_lock(&gisp_cmd_mutex);
	while(isp_rl_gain[ch_id].update_awb == 1 && isp_rl_gain[ch_id].on_running == 1 && i-- > 0){
		mdelay(1);
	}
	if(isp_rl_gain[ch_id].on_running == 0){
		isp_rl_gain[ch_id].bgain = lut_data->bgain;
		isp_rl_gain[ch_id].ggain = lut_data->ggain;
		isp_rl_gain[ch_id].rgain = lut_data->rgain;
		isp_rl_gain[ch_id].update_awb = 1;
	}
	
	if(1){
		unsigned int isp_en = reg_read(ISP_BASE_PHY + ISP_ENABLE_OFFSET);
		if((isp_en&0x1) == 0){
			unsigned int reg_offset = 0;
			unsigned int reg_addr_0 = (unsigned int)ISP_BASE_PHY + ISP_CG_B_GAIN_OFFSET;
			unsigned int reg_addr_1 = (unsigned int)ISP_BASE_PHY + ISP_CG_G_GAIN_OFFSET;
			unsigned int reg_addr_2 = (unsigned int)ISP_BASE_PHY + ISP_CG_R_GAIN_OFFSET;		
			reg_write(reg_addr_0,isp_rl_gain[ch_id].bgain);
			reg_write(reg_addr_1, isp_rl_gain[ch_id].ggain);
			reg_write(reg_addr_2, isp_rl_gain[ch_id].rgain);
			isp_rl_gain[ch_id].update_awb = 0;
		}		
	}
//	mutex_unlock(&gisp_cmd_mutex);
}

void isp_get_gain(bisp_internal_t *bisp_fifo,bisp_bg_t *lut_data){
	int ch_id = 0;
//	mutex_lock(&gisp_cmd_mutex);
	bisp_buffer_node_t *buffer_node_tmp = NULL;
	ch_id = bisp_fifo->open_count;

	//lut_data->stat_addr = gstat_data[0].stat_addr;
	lut_data->bgain = gstat_data[ch_id][0].bgain;
	lut_data->ggain = gstat_data[ch_id][0].ggain;
	lut_data->rgain = gstat_data[ch_id][0].rgain;	
	
	buffer_node_tmp = get_buffer_node(bisp_fifo);
	if(buffer_node_tmp){
		lut_data->stat_addr = buffer_node_tmp->pHyaddr;
		lut_data->stat_vddr = buffer_node_tmp->pViraddr;
	}
	else {
		lut_data->stat_addr = 0;
		lut_data->stat_vddr = 0;
	}
	
//	mutex_unlock(&gisp_cmd_mutex);
}
EXPORT_SYMBOL(bisp_updata); 

static module_name_t module_name;
void bisp_set_module_name(char *buf)
{
	strcpy(module_name.buf, buf);
}
EXPORT_SYMBOL(bisp_set_module_name);

long bisp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	bisp_internal_t *info = (bisp_internal_t*) filp->private_data;
	void __user *from = (void __user *)arg;
	int ret_value = 0;
	int ret_count = 0;
	mutex_lock(&gisp_cmd_mutex);
	//printk("bisp_ioctl in \n");

	switch (cmd)
	{
		case BRI_BLC:
		{
			blc_info_t blc_inf;
			ret_count=copy_from_user(&blc_inf,from,sizeof(blc_info_t));
			set_blc(&blc_inf);
		}
		break;
		case AF_CMD_GET_INFO:
  	{
  		void __user *to = (void __user *)arg;
  		//printk("AF_CMD_GET_INFO===\n");
  		//mutex_lock(&af_cmd_mutex);
      ret_value = 0;
      info->af_ready--;
      ret_value = ov5647_af_fv_read(&info->af_sharpness,info->af_param.win_num);
      if(ret_value == 0){
      	//printk(KERN_ERR"AF_CMD_GET_INFO===\n");
    		ret_count = copy_to_user(to,&info->af_sharpness,sizeof(af_pv_t));
    	}
  		//mutex_unlock(&af_cmd_mutex);  
		}    
    break; 
   
    case AF_CMD_ENABLE:
    {
    	//mutex_lock(&af_cmd_mutex);    	
    	ov5647_af_clk_enable();
    	ov5647_af_irq_enable();
    	info->baf_enabled = 1;
    	af_enable_count++;
    	//mutex_unlock(&af_cmd_mutex);
    }
    break;
    case AF_CMD_DISABLE:
    {
    	//mutex_lock(&af_cmd_mutex);   
    	info->baf_enabled = 0;  	
    	ov5647_af_irq_disable();
    	ov5647_af_clk_disable();
    	af_enable_count--;
    	memset(&af_pv_data,0,sizeof(af_pv_data_t));
    	//mutex_unlock(&af_cmd_mutex); 
    }
    break;
    case AF_CMD_SET_WINPS:
    {
    	void __user *from = (void __user *)arg;
    	//mutex_lock(&af_cmd_mutex);  	
    	printk(KERN_ERR"AF_CMD_SET_WINPS \n");
    	ret_count=copy_from_user(&info->af_param,from,sizeof(af_param_t));
    	ov5647_af_param_set(info,&info->af_param);
    	//mutex_unlock(&af_cmd_mutex);
    }
    break;
    
	case BRI_SEN:
	{
		bisp_info_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_info_t));
		set_info(&binf);
	}
	break;
	case BRI_SBS:
	{
		bisp_stat_buffers_info_t bisp_buffers;
		ret_count=copy_from_user(&bisp_buffers,from,sizeof(bisp_stat_buffers_info_t));
		printk("BRI_SBS \n");
		init_node(info,&bisp_buffers);
		printk("BRI_SBS 2\n");
	}
	break;
	case BRI_FTB:
	{
		bisp_ads_t bisp_ads;
		ret_count=copy_from_user(&bisp_ads,from,sizeof(bisp_ads_t));
		//printk("BRI_FTB %x \n",bisp_ads.ads);
		set_buffer_node(info,bisp_ads.ads);
	}
	break;
	case BRI_COP:
	{
		bisp_region_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_region_t));
		set_cop(&binf);
	}
	break;
	case BRI_VNC:
	{
		bisp_vnc_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_vnc_t));
		set_vnc(&binf);
	}
	break;
	case BRI_LUT:
	{
		bisp_lsc_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_lsc_t));
		set_lsc(&binf);
	}
	break;
	case BRI_NR:
	{
		bisp_nr_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_nr_t));
		set_nr(&binf);
	}
	break;
	case BRI_EE:
	{
		bisp_nr_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_nr_t));
		set_ee(&binf);
	}
	break;
	case BRI_DPC:
	{
		bisp_nr_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_nr_t));
		set_dpc(&binf);
	}
	break;
	case BRI_GN:
	{
		bisp_bg_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_bg_t));
		//set_gn(&binf);
		isp_set_gain(&binf,info->open_count);
	}
	break;
	case BRI_GET_GN:
	{
		bisp_bg_t binf;
		isp_get_gain(info,&binf);
		ret_count = copy_to_user(from,&binf,sizeof(bisp_bg_t));
	}
	break;
	case BRI_SRG:
	{
		bisp_region_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_region_t));
		set_srg(&binf);
	}
	break;
	case BRI_SHG:
	{
		bisp_region_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_region_t));
		set_hrg(&binf);
	}
	break;
	case BRI_WBP:
	{
		bisp_wp_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_wp_t));
		set_wbp(&binf);
	}
	break;
	case BRI_CSC:
	{
		bisp_csc_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_csc_t));
		set_csc(&binf);
	}
	break;
	case BRI_CRP:
	{
		bisp_color_adjust_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_color_adjust_t));
		set_crp(&binf);
	}
	break;
	case BRI_CFX:
	{
		bisp_color_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_color_t));
		set_cfx(&binf);
	}
	break;
	case BRI_GC:
	{
		bisp_gc_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_gc_t));
		set_gc(&binf);
	}
	break;
	case BRI_HVB:
	{
		bisp_bank_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_bank_t));
		set_hvb(&binf);
	}
	break;
	case BRI_GET_SDS:
	{
		bisp_ads_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_ads_t));
		get_sds(info,&binf);
		ret_count = copy_to_user(from,&binf,sizeof(bisp_ads_t));
		
	}
	break;
	case BRI_GET_YDS:
	{
		bisp_ads_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_ads_t));
		get_yds(&binf);
		ret_count = copy_to_user(from,&binf,sizeof(bisp_ads_t));
	}
	break;
	case BRI_GET_CRP:
	{
		bisp_region_t binf;
		ret_count=copy_from_user(&binf,from,sizeof(bisp_region_t));
		get_cop(&binf);
		ret_count = copy_to_user(from,&binf,sizeof(bisp_region_t));
	}
	break;

	case BRI_SID:
	{
		//int ch_id = (int)arg;
		//int channel_num = 0;

		//if(ch_id == 1){
		//	get_config((char*)"cameraf.channel_num",(char*) &channel_num, sizeof(channel_num));
		//}else {
		//	get_config((char*)"camerab.channel_num",(char*) &channel_num, sizeof(channel_num));
		//}
		info->ch_id = 2;//channel_num + 1;
		
		//printk("info->ch_id %d %d %d,%d\n",info->ch_id,channel_num,info->open_count,gbisp_fifo[info->open_count]->ch_id);
	}
	break;
	case BRI_GET_MODULE_NAME:
		{
	
		int ch_id = (int)arg;
		//char module_name[20];
		//module_name_t module_name = {"ov5648",};
		//if(ch_id == 1){
		//	get_config("cameraf.module_name", &module_name.buf[0], 16);
		//}
		//else {
		//	get_config("camerab.module_name", &module_name.buf[0], 16);
		//}
		printk("info->module_name %s\n",module_name.buf);
		ret_count = copy_to_user(from,&module_name,sizeof(module_name_t));
	}
	break;

	default:
		mutex_unlock(&gisp_cmd_mutex);
		return -EIO;
	}

//out:
	mutex_unlock(&gisp_cmd_mutex);
	//printk("bisp_ioctl out \n");
	return 0;
}

int bisp_open(struct inode *inode, struct file *filp)
{  
  bisp_internal_t *info = NULL;
	int i = 0;
	int opened_cnt = 0;
	mutex_lock(&gisp_cmd_mutex);
  bisp_drv_opened++;
  if (bisp_drv_opened > _CH_MX_NUM_)
  {
  	 printk("bisp_drv open count %d\n",bisp_drv_opened);
  		bisp_drv_opened -= 1;
  		mutex_unlock(&gisp_cmd_mutex);
      return -1;
  }
  if(bisp_drv_opened == 1){
  	 for(i = 0; i < _CH_MX_NUM_; i++){
   		gbisp_fifo[i] = NULL;
   	}
  }
   for(i = 0; i < _CH_MX_NUM_; i++){
   	if(gbisp_fifo[i] == NULL){
   		opened_cnt = i;
   		i = _CH_MX_NUM_+1;   		
   	}
   	else{
   		printk(KERN_ERR"xxxbisp_openx %d,%d,%d\n",bisp_drv_opened,opened_cnt,gbisp_fifo[i]->ch_id);
   	}
  }  
  info = (bisp_internal_t*)kzalloc(sizeof(bisp_internal_t),GFP_KERNEL | GFP_DMA);
  if(info == NULL)
  {
  	printk("bisp_drv malloc failed!\n");
  	mutex_unlock(&gisp_cmd_mutex);
  	return -1;
  }
   
  info->open_count = opened_cnt;
  
  memset(info,0,sizeof(bisp_internal_t));
  memset(&isp_rl_gain[opened_cnt],0,sizeof(isp_realtime_gain_t));    
  filp->private_data = (void*)info;  
  gbisp_fifo[opened_cnt] = info;
  printk(KERN_ERR"xxxbisp_open %d,%d,%d\n",bisp_drv_opened,opened_cnt,gbisp_fifo[0]->ch_id);
  if(bisp_drv_opened == 1){  		
    	memset(&af_pv_data,0,sizeof(af_pv_data_t));
    }
  mutex_unlock(&gisp_cmd_mutex);
	return 0;
}

int bisp_release(struct inode *inode, struct file *filp)
{
	bisp_internal_t *info = (bisp_internal_t *) filp->private_data;
	mutex_lock(&gisp_cmd_mutex);
  bisp_drv_opened--;
  if(bisp_drv_opened <= 0){
  	bisp_drv_opened = 0;  	
  }
  if(info){
  	gbisp_fifo[info->open_count] = NULL;
		kfree(info);
		info = NULL;
  }
  mutex_unlock(&gisp_cmd_mutex);
	printk(KERN_ERR"bisp_release\n");
	return 0;

}

static void bisp_dts_init(void)
{
    bisp_sensor_info_t *sinfo;
    g_bisp_dts.front = -1;
    g_bisp_dts.rear = -1;
    sinfo = &g_bisp_dts.sinfo[BISP_FRONT_SENSOR];
    sinfo->channel = 0;
    sinfo->name = NULL;
    sinfo = &g_bisp_dts.sinfo[BISP_REAR_SENSOR];
    sinfo->channel = 0;
    sinfo->name = NULL;
}

static int parse_sensor_dts(struct device_node *dn, bisp_sensor_info_t *si)
{
    unsigned int channel;
    const char *name;
    
    if (of_property_read_u32(dn, "channel", &channel)) {
        printk(KERN_ERR "err: bisp fail to get sensor channel\n");
        goto fail;
    }
    
    si->channel = channel;
#if 0
    if (of_property_read_string(dn, "compatible", &name)) {
        printk(KERN_ERR "err: bisp faild to get sensor's name\n");
        goto fail;
    }
    si->name = name;
#endif
    return 0;
fail:
    return -EINVAL;
}

static int parse_bisp_dts(void)
{
    struct device_node *fdt_node;
    struct device_node *rear_dn;
    struct device_node *front_dn;
    bisp_sensor_info_t *sinfo;
    int err = 0;

    static const char *sensorName[2] = {"sensor0", "sensor1"};

    fdt_node = of_find_compatible_node(NULL, NULL, DEVDRV_NAME_BISP);
    if (NULL == fdt_node) {
        printk(KERN_ERR "err: no sensor [%s]\n", DEVDRV_NAME_BISP);
        goto fail;
    }
    rear_dn = of_parse_phandle(fdt_node, "rear", 0);
    if (NULL == rear_dn) {
        printk(KERN_WARNING "not support rear sensor ??\n");  /* ignore it */
    } else {
        err = parse_sensor_dts(rear_dn, &g_bisp_dts.sinfo[BISP_REAR_SENSOR]);
        if (0 == err) {
            g_bisp_dts.sinfo[BISP_REAR_SENSOR].name = sensorName[BISP_REAR_SENSOR];
            g_bisp_dts.rear = 1;
        } else {
            printk(KERN_ERR "bisp parse rear sensor failed\n");
            goto fail;
        }
    }

    front_dn = of_parse_phandle(fdt_node, "front", 0);
    if (NULL == rear_dn) {
        printk(KERN_WARNING "not support front sensor ??\n");
    } else {
        err = parse_sensor_dts(front_dn, &g_bisp_dts.sinfo[BISP_FRONT_SENSOR]);
        if (0 == err) {
            g_bisp_dts.sinfo[BISP_FRONT_SENSOR].name = sensorName[BISP_FRONT_SENSOR];
            g_bisp_dts.front = 1;
        } else {
            printk(KERN_ERR "bisp parse front sensor failed\n");
            goto fail;
        }
    }
    if (g_bisp_dts.front) {
        sinfo = &g_bisp_dts.sinfo[BISP_FRONT_SENSOR];
        printk("front sensor: channel[%d], name: %s\n", sinfo->channel, sinfo->name);
    } else {
        printk("front sensor: don't support!!\n");
    }
    if (g_bisp_dts.rear) {
        sinfo = &g_bisp_dts.sinfo[BISP_REAR_SENSOR];
        printk("rear sensor: channel[%d], name: %s\n", sinfo->channel, sinfo->name);
    } else {
        printk("rear sensor: don't support!!\n");
    }
    return 0;

fail:
    return -EINVAL;
}

/* 进入低功耗之前，必须保证vpx已经解码完当前帧。
 * 
 */
int bisp_suspend (struct platform_device * dev, pm_message_t state)
{    
		printk(KERN_ERR"bisp_suspend! %d,%d\n",bisp_drv_opened,af_enable_count);
		if(bisp_drv_opened > 0 && af_enable_count > 0){
			ov5647_af_irq_disable();
    	//ov5647_af_clk_disable();
  	}
  	printk(KERN_ERR"bisp_suspend ok\n");
    return 0;
}


int bisp_resume (struct platform_device * dev)
{
		printk(KERN_ERR"bisp_resume! %d,%d\n",bisp_drv_opened,af_enable_count);
		if(bisp_drv_opened > 0 && af_enable_count > 0) {
			//ov5647_af_clk_enable();
		}
		printk(KERN_ERR"bisp_resume ok\n");
    return 0;
}

static void bisp_dummy_release(struct device *dev)
{
}


static struct file_operations bisp_fops = 
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = bisp_ioctl,
	.open = bisp_open,
	.release = bisp_release,
};


static struct platform_device bisp_platform_device = 
{
    .name   = DEVDRV_NAME_BISP,
    .id = -1,
    .dev = {
        .release = bisp_dummy_release,
    },
};

static struct platform_driver bisp_platform_driver = 
{
   .driver = 
	 {
        .name = DEVDRV_NAME_BISP,
        .owner = THIS_MODULE,
    },
    .suspend = bisp_suspend,
    .resume = bisp_resume,
};


static struct miscdevice bisp_miscdevice = 
{
     .minor = MISC_DYNAMIC_MINOR,
     .name = DEVDRV_NAME_BISP,
      .fops = &bisp_fops,
};


static int bisp_init(void)
{
	int ret;

    bisp_dts_init();
    parse_bisp_dts();
    
	//自动insmod，注册设备
    ret = misc_register(&bisp_miscdevice);
    if (ret) 
	  {
        printk(KERN_ERR"register bisp misc device failed!...\n");
        goto err0;
    }
    printk(KERN_ERR"bisp_drv_init ...\n");
 

    ret = platform_device_register(&bisp_platform_device);
    if (ret)
    {
        printk(KERN_ERR"register bisp_platform_device error!...\n");
        goto err1;
    }

    ret = platform_driver_register(&bisp_platform_driver);
    if (ret) 
    {
        printk(KERN_ERR"register bisp platform driver4pm error!...\n");
        goto err2;
    } 


    return 0;

err2:
	platform_device_unregister(&bisp_platform_device);
	
err1:
	misc_deregister(&bisp_miscdevice);
	
err0:
    return ret;
}

static void bisp_exit(void)
{
	printk(KERN_ERR"bisp_drv_exit ...\n");
	misc_deregister(&bisp_miscdevice);
	platform_device_unregister(&bisp_platform_device);
	platform_driver_unregister(&bisp_platform_driver);
}

module_init(bisp_init);
module_exit(bisp_exit);

MODULE_LICENSE("GPL");

