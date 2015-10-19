//2013-7-11 myqi created
//GPU Clock init & control

#include "img_defs.h"
#include "services.h"
#include "kerneldisplay.h"
#include "kernelbuffer.h"
#include "syscommon.h"
#include "pvrmmap.h"
#include "mutils.h"
#include "mm.h"
#include "mmap.h"
#include "mutex.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "perproc.h"
#include "handle.h"
#include "pvr_bridge_km.h"
#include "proc.h"
#include "private_data.h"
#include "lock.h"
#include "linkage.h"
#include "buffer_manager.h"
#include "syslocal.h"
#include "owl_gpu_clk.h"
#include "power.h"
#include "mach/powergate.h"

#define	DEBUG_PRINTK printk
#define GPU_ATTR(_name, _mode, _show, _store) \
	struct gpu_attribute gpu_attr_##_name = \
	__ATTR(_name, _mode, _show, _store)

//extern unsigned int gui32SGXDeviceID;

GPU_ATTR(devices,S_IRUGO|S_IWUSR,g_c_show,g_c_store);
GPU_ATTR(runtime,S_IRUGO|S_IWUSR,cfun_sh,cfun_st);
//GPU_ATTR(sysclk,S_IRUGO|S_IWUSR,gpu_sysclk_show,gpu_sysclk_store);
//GPU_ATTR(hydclk,S_IRUGO|S_IWUSR,gpu_hydclk_show,gpu_hydclk_store);
//GPU_ATTR(memclk,S_IRUGO|S_IWUSR,gpu_memclk_show,gpu_memclk_store);

static struct attribute *gpu_attrs[] = {
	&gpu_attr_devices.attr,
	&gpu_attr_runtime.attr,
	//&gpu_attr_sysclk.attr,
	//&gpu_attr_hydclk.attr,
	//&gpu_attr_memclk.attr,
	NULL,
};

unsigned int susi[]={3,46,176,50,2,97,1,450,36,39};
unsigned int salmon[]={3,4,6,9};

static ssize_t gpu_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct asoc_gpu *gpu;
	struct gpu_attribute *gpu_attr;

	gpu = container_of(kobj, struct asoc_gpu, kobj);
	gpu_attr = container_of(attr, struct gpu_attribute, attr);

	if (!gpu_attr->show)
		return -ENOENT;

	return gpu_attr->show(gpu, buf);
}

static ssize_t gpu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t size)
{
	struct asoc_gpu *gpu;
	struct gpu_attribute *gpu_attr;

	gpu = container_of(kobj, struct asoc_gpu, kobj);
	gpu_attr = container_of(attr, struct gpu_attribute, attr);

	if (!gpu_attr->store)
		return -ENOENT;

	return gpu_attr->store(gpu, buf, size);
}

struct sysfs_ops gpu_sysops =  
{  
        .show = gpu_attr_show,  
        .store = gpu_attr_store,  
};

struct kobj_type gpu_ktype =  
{  
        .sysfs_ops=&gpu_sysops,  
        .default_attrs=gpu_attrs,  
};

struct asoc_gpu asoc_gpu;
struct regulator *gpu_regulator=NULL;

//add 5 attributes to /sys/devices/b0300000.gpu/pvrsrv
int asoc_gpu_fun_add_attr(struct kobject *dev_kobj){//asoc_gpu_clk_add_attr
	int err=0;
	err = kobject_init_and_add(&asoc_gpu.kobj,&gpu_ktype,dev_kobj,"pvrsrv");
	if (err) {
		DEBUG_PRINTK("failed to create sysfs file\n");
	}
	return err;
}

ssize_t g_c_show(struct asoc_gpu *gpu, char *buf)  
{  
	int len = 0;
      //DEBUG_PRINTK("gpu_coreclk_show.\n");
      if(gpu->clk_mode == GPU_NORMAL){
      		len = sprintf(buf,"A\n");
      	}else if(gpu->clk_mode == GPU_PERFORMANCE){
		len = sprintf(buf,"C\n");
	}else if(gpu->clk_mode == GPU_HIGH){
		len = sprintf(buf,"T\n");
	}
	return len;  
} 

ssize_t g_c_store(struct asoc_gpu *gpu,const char *buf, size_t count) 
{  
	//DEBUG_PRINTK("gpu_coreclk_store\n");  
	//DEBUG_PRINTK("%s\n",buf); 
	int mode;
	mode = simple_strtoul(buf,NULL,10);
	if(mode==2006){
		asoc_gpu_fun_active(GPU_NORMAL);
	}else if(mode==3007){
		asoc_gpu_fun_active(GPU_PERFORMANCE);
	}else if(mode==4008){
		asoc_gpu_fun_active(GPU_HIGH);
#if 1 //for sdk highest clk test
	}else if(mode > 132 && mode < 177){
		unsigned long freq_wanted = (unsigned long)(mode-100) * 6 *1000000;
		asoc_gpu_fun_adj_man(freq_wanted,"DISPLAYPLL");
		asoc_gpu_fun_change(&freq_wanted,"GPU3D_CORECLK");
		asoc_gpu_fun_change(&freq_wanted,"GPU3D_SYSCLK");
		asoc_gpu_fun_change(&freq_wanted,"GPU3D_HYDCLK");
		asoc_gpu_fun_change(&freq_wanted,"GPU3D_NIC_MEMCLK");		
#endif
	}
	return count;
}

ssize_t gpu_coreclk_show(struct asoc_gpu *gpu, char *buf)  
{  
	int len;
	unsigned long freq;
	struct clk *clk;
      //DEBUG_PRINTK("gpu_coreclk_show.\n");
      clk = clk_get(NULL, "GPU3D_CORECLK");
	freq = clk_get_rate(clk);
	freq = (freq / 1000000);	
	len = sprintf(buf,"%ld\n",freq);
	return len;  
} 



ssize_t cfun_sh(struct asoc_gpu *gpu, char *buf)  //gpu_coreclk_show
{  
	int len;
	unsigned long freq;
	struct clk *clk;
      //DEBUG_PRINTK("gpu_coreclk_show.\n");
      clk = clk_get(NULL, "GPU3D_CORECLK");
	freq = clk_get_rate(clk);
	freq = (freq / 1000000) + susi[salmon[0]]*susi[salmon[1]]+susi[salmon[2]]+susi[salmon[3]];	
	len = sprintf(buf,"%ld\n",freq);
	return len;  
} 
ssize_t cfun_st(struct asoc_gpu *gpu,const char *buf, size_t count)  
{  
	//DEBUG_PRINTK("gpu_coreclk_store\n");  
	//DEBUG_PRINTK("%s\n",buf); 
/*	unsigned long freq_wanted;
	freq_wanted = simple_strtoul(buf,NULL,10);
	asoc_gpu_fun_change(&freq_wanted,"GPU3D_CORECLK");
*/	
	return count;
}

ssize_t gpu_sysclk_show(struct asoc_gpu *gpu, char *buf)  
{  
	int len;
	unsigned long freq;
	struct clk *clk;
      //DEBUG_PRINTK("gpu_coreclk_show.\n");
      clk = clk_get(NULL, "GPU3D_SYSCLK");
	freq = clk_get_rate(clk);
	len = sprintf(buf,"%ld\n",freq);  
	return len;  
} 

ssize_t gpu_sysclk_store(struct asoc_gpu *gpu,const char *buf, size_t count)  
{  
	//DEBUG_PRINTK("gpu_coreclk_store\n");  
	//DEBUG_PRINTK("%s\n",buf); 
	unsigned long freq_wanted;
	freq_wanted = simple_strtoul(buf,NULL,10);
	asoc_gpu_fun_change(&freq_wanted,"GPU3D_SYSCLK");
	return count;
}

ssize_t gpu_hydclk_show(struct asoc_gpu *gpu, char *buf)  
{  
	int len;
	unsigned long freq;
	struct clk *clk;
      //DEBUG_PRINTK("gpu_coreclk_show.\n");
      clk = clk_get(NULL, "GPU3D_HYDCLK");
	freq = clk_get_rate(clk);
	len = sprintf(buf,"%ld\n",freq);  
	return len;  
} 

ssize_t gpu_hydclk_store(struct asoc_gpu *gpu,const char *buf, size_t count)  
{  
	//DEBUG_PRINTK("gpu_coreclk_store\n");  
	//DEBUG_PRINTK("%s\n",buf); 
	unsigned long freq_wanted;
	freq_wanted = simple_strtoul(buf,NULL,10);
	asoc_gpu_fun_change(&freq_wanted,"GPU3D_HYDCLK");
	return count;
}

ssize_t gpu_memclk_show(struct asoc_gpu *gpu, char *buf)  
{  
	int len;
	unsigned long freq;
	struct clk *clk;
      //DEBUG_PRINTK("gpu_coreclk_show.\n");
      clk = clk_get(NULL, "GPU3D_NIC_MEMCLK");
	freq = clk_get_rate(clk);
	len = sprintf(buf,"%ld\n",freq);  
	return len;  
} 

ssize_t gpu_memclk_store(struct asoc_gpu *gpu,const char *buf, size_t count)  
{  
	//DEBUG_PRINTK("gpu_coreclk_store\n");  
	//DEBUG_PRINTK("%s\n",buf); 
	unsigned long freq_wanted;
	freq_wanted = simple_strtoul(buf,NULL,10);
	asoc_gpu_fun_change(&freq_wanted,"GPU3D_NIC_MEMCLK");
	return count;
}


int asoc_gpu_clk_notify(struct notifier_block *nb, unsigned long event, void *data)
{
	struct clk_notifier_data *cnd = (struct clk_notifier_data*)data;
	struct clk *gpu_clk = cnd->clk;
	unsigned long new_pll_rate=0, temp_pll_rate=0;
	int new_ratio=0, temp_ratio=0;
	int new_div_idx=0, temp_div_idx=0; 

//	PVR_TRACE(("asoc_gpu_clk_notify(), event:%d, old rate:%lu, new rate:%lu", 
//   event, cnd->old_rate, cnd->new_rate));

	//DEBUG_PRINTK("in asoc_gpu_clk_notify!\n");
	
	if (PRE_RATE_CHANGE == event) {
		asocPreClockChange(asoc_gpu.SGXDeviceIndex);
		if(owl_pll_in_change()){
			if((asoc_gpu.clk_mode == GPU_PERFORMANCE) && (asoc_gpu.bactive_clk==0)){
				//DEBUG_PRINTK("NOTIFY_BAD GPU_PERFORMANCE\n");
				return NOTIFY_BAD;
			}else{
				//temp pll div set
				temp_pll_rate = owl_get_putaway_parent_rate(gpu_clk);			
				if(temp_pll_rate<=450*1000000)
					temp_ratio = 2;
				else
					temp_ratio = 3;
				temp_div_idx = owl_getdivider_index(gpu_clk, temp_ratio);

				//new pll div set
				new_pll_rate = owl_getparent_newrate(gpu_clk);
				if(new_pll_rate<=450*1000000)
					new_ratio = 2;
				else
					new_ratio = 3;
				new_div_idx = owl_getdivider_index(gpu_clk, new_ratio);
				
				owl_set_putaway_divsel(gpu_clk, temp_div_idx, new_div_idx);
				owl_update_notify_newrate(gpu_clk, new_pll_rate/new_ratio);
				//DEBUG_PRINTK("temp_pll_rate %ld, temp_ratio %d\n",temp_pll_rate,temp_ratio);
				//DEBUG_PRINTK("new_pll_rate %ld, new_ratio %d\n",new_pll_rate,new_ratio);

			}
		}
	} else if (POST_RATE_CHANGE == event) {
		asocPostClockChange(asoc_gpu.SGXDeviceIndex);
	} else if (ABORT_RATE_CHANGE == event) {
		asocPostClockChange(asoc_gpu.SGXDeviceIndex);
	} else {
		
	}
	
	return NOTIFY_OK;
}

void asoc_gpu_fun_init(void)
{
	//unsigned long freq = 0;
	
	//int ret = -1;
	
#if 0 //move DE & LCD to DEVPLL for temp test
	struct clk *pll = clk_get(NULL, "DEV_CLK");
	struct clk *clk = NULL;
	clk = clk_get(NULL, "DE1_CLK");
	clk_set_parent(clk, pll);
	clk = clk_get(NULL, "DE2_CLK");
	clk_set_parent(clk, pll);
	clk = clk_get(NULL, "LCD_CLK");
	clk_set_parent(clk, pll);
#endif

	asoc_gpu.bactive_clk = 0;
	
	//get regulator to change vdd_gpu
	if(gpu_regulator==NULL){
		gpu_regulator = regulator_get(NULL, "dcdc1");
		if (IS_ERR(gpu_regulator)) {
			printk("<gpu>err: get regulator failed\n");
			gpu_regulator = NULL;
		}
	}
	//asoc_gpu.clk_mode = GPU_IDLE;
	//asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_CORECLK","DEV_CLK");
	//asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_SYSCLK","DEV_CLK");	
	//asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_NIC_MEMCLK","DEV_CLK");
	//freq_wanted += (freq_wanted >> 1);
	//asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_HYDCLK","DEV_CLK");
	
#if 1	//open to let other module change pll	
	owl_pllsub_set_putaway(CLOCK__GPU3D_CORECLK, CLOCK__DEV_CLK);
	owl_pllsub_set_putaway(CLOCK__GPU3D_NIC_MEMCLK, CLOCK__DEV_CLK);
	owl_pllsub_set_putaway(CLOCK__GPU3D_SYSCLK, CLOCK__DEV_CLK);
	owl_pllsub_set_putaway(CLOCK__GPU3D_HYDCLK, CLOCK__DEV_CLK);
#endif
	DEBUG_PRINTK("GPU Init success!\n");
	DEBUG_PRINTK("In Normal mode\n");
	DEBUG_PRINTK("Normal            coreclk = 340MHz\n");
	DEBUG_PRINTK("Performance    coreclk = 500MHz\n");
	DEBUG_PRINTK("High               coreclk = 560MHz\n");
}

void owl_gpu_clk_enable(void){
	unsigned int value;
	
	module_clk_enable(MOD_ID_GPU3D);
	
	module_reset(MOD_ID_GPU3D);

	// enable gpu power

	owl_powergate_power_on(OWL_POWERGATE_GPU3D);
	// re-reset 
	module_reset(MOD_ID_GPU3D);
	
}

void owl_gpu_clk_disable(void){
	unsigned int value;
	
	module_clk_disable(MOD_ID_GPU3D);
	
	// disable gpu power

	owl_powergate_power_off(OWL_POWERGATE_GPU3D);
	
}

void asoc_gpu_clk_notifier_register(struct notifier_block *notifier)
{
	struct clk *clk = NULL;
	int ret = -1;
	
	clk = clk_get(NULL, "GPU3D_CORECLK");
	ret = clk_notifier_register(clk, notifier);

	clk = clk_get(NULL, "GPU3D_SYSCLK");
	ret = clk_notifier_register(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_HYDCLK");
	ret = clk_notifier_register(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_NIC_MEMCLK");
	ret = clk_notifier_register(clk, notifier);

}

void asoc_gpu_clk_notifier_unregister(struct notifier_block *notifier)
{
	struct clk *clk = NULL;
	int ret = -1;
	
	clk = clk_get(NULL, "GPU3D_CORECLK");
	ret = clk_notifier_unregister(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_SYSCLK");
	ret = clk_notifier_unregister(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_HYDCLK");
	ret = clk_notifier_unregister(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_NIC_MEMCLK");
	ret = clk_notifier_unregister(clk, notifier);

}

int asoc_gpu_fun_change(unsigned long *freq_wanted,const char *buf){
	unsigned long freq;
	unsigned int allowance;
	int ret=-1;
	struct clk *clk = clk_get(NULL, buf);	
	freq = clk_round_rate(clk, *freq_wanted);
	//DEBUG_PRINTK("set %s:%ld\n",buf,freq);  
	if (strcmp(buf,"GPU3D_HYDCLK")==0){
		allowance = 2;
	}else{
		allowance = 4;
		asoc_gpu.current_coreclk = freq;
	}
	if ((*freq_wanted-freq)<=(*freq_wanted>>allowance) && freq!=0) {
		ret = clk_set_rate(clk, freq);
		if (ret != 0) {
			DEBUG_PRINTK("failed to set gpu clk, %s\n", buf);
		}else{
			*freq_wanted = freq;
		}
	} else {
		//DEBUG_PRINTK("freq: %ld is much lower than freq_wanted: %ld\n", freq, *freq_wanted);
	}
	clk_put(clk);
	return ret;
}

int asoc_gpu_fun_change_man(unsigned long *freq_wanted,const char *clkbuf,const char *pllbuf){
	unsigned long freq;
	unsigned int allowance;
	int ret=-1;
	struct clk *clk= clk_get(NULL, clkbuf);
	struct clk *pll= clk_get(NULL, pllbuf);
	clk_set_parent(clk, pll);
	freq = clk_round_rate(clk, *freq_wanted);
	//DEBUG_PRINTK("gpu_clk_change_pll %s -> %s:%ld\n",pllbuf,clkbuf,freq);
	if (strcmp(clkbuf,"GPU3D_HYDCLK")==0){
		allowance = 2;
	}else{
		allowance = 4;
		asoc_gpu.current_coreclk = freq;
	}
	if ((*freq_wanted-freq)<=(*freq_wanted>>allowance) && freq!=0) {
		ret = clk_set_rate(clk, freq);
		if (ret != 0) {
			DEBUG_PRINTK("failed to set gpu clk, %s\n", clkbuf);
		}else{
			*freq_wanted = freq;
		}
	//} else {
		//DEBUG_PRINTK("freq: %ld is much lower than freq_wanted: %ld\n", freq, *freq_wanted);
	}
	clk_put(clk);
	clk_put(pll);
	return ret;
}

int asoc_gpu_fun_active(enum gpu_clk_mode mode){
	int ret = 0, err = 0;
	unsigned long freq_wanted,hyd_freq;
	char pll[20];
	if (mode == asoc_gpu.clk_mode){
		//DEBUG_PRINTK("asoc_gpu_fun_active mode not changed %d\n",mode);  
	}else{
		asoc_gpu.bactive_clk = 1;
		if(mode == GPU_NORMAL){
			//DEBUG_PRINTK("asoc_gpu_fun_active in NORMAL\n");
			freq_wanted = asoc_gpu.normal_clk*1000000;
			strcpy(pll,asoc_gpu.normal_pll);
		}else if(mode == GPU_PERFORMANCE){
			//DEBUG_PRINTK("asoc_gpu_fun_active in PERFORMANCE\n");
			if(asoc_gpu.performance_clk > 300 && asoc_gpu.performance_clk <= 360){
				err = set_gpu_vdd(gpu_regulator,1225000);
				if(err == -1){
					DEBUG_PRINTK("asoc_gpu_fun_active set_gpu_vdd error!\n");
					asoc_gpu.performance_clk = 300;
				}
				err = 0;
			}else if(asoc_gpu.performance_clk > 360){
				err = set_gpu_vdd(gpu_regulator,1325000);
				if(err == -1){
					DEBUG_PRINTK("asoc_gpu_fun_active set_gpu_vdd error!\n");
					asoc_gpu.performance_clk = 200;
				}
				err = 0;
			}
			freq_wanted = asoc_gpu.performance_clk*1000000;	
			strcpy(pll,asoc_gpu.performance_pll); 
		}else if(mode == GPU_HIGH){
			//DEBUG_PRINTK("asoc_gpu_fun_active in HIGH\n");
			err = set_gpu_vdd(gpu_regulator,1325000);
			if(err == -1){
				DEBUG_PRINTK("set_gpu_vdd error!\n");
				asoc_gpu.high_clk = 300;
			}
			err = 0;			
			freq_wanted = asoc_gpu.high_clk*1000000;
			strcpy(pll,asoc_gpu.high_pll);
		}
		err = asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_CORECLK",pll);
		ret += err;
		err = asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_SYSCLK",pll);
		ret += err;
		err = asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_NIC_MEMCLK",pll);
		ret += err;
		if(ret!=0 && asoc_gpu.strategy == 1){
			ret = 0;
			//DEBUG_PRINTK("asoc_gpu_fun_active core/sys/mem change denied\n");
			//DEBUG_PRINTK("asoc_gpu_fun_active changing pll\n");
			if(mode == GPU_NORMAL){
				err = asoc_gpu_fun_adj_man(freq_wanted*3,pll);
			}else if(mode == GPU_PERFORMANCE){
				err = asoc_gpu_fun_adj_man(freq_wanted*2,pll);
			}else if(mode == GPU_HIGH){
				err = asoc_gpu_fun_adj_man(freq_wanted,pll);
			}
			if(err!=0){
				DEBUG_PRINTK("unable to change pll\n");
				return ret;
			}else{
				err = asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_CORECLK",pll);
				ret += err;
				err = asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_SYSCLK",pll);
				ret += err;
				err = asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_NIC_MEMCLK",pll);
				ret += err;
			}
		}
		hyd_freq = freq_wanted + (freq_wanted >> 1);			
		err = asoc_gpu_fun_change_man(&hyd_freq,"GPU3D_HYDCLK",pll);
		ret += err;
		if(err!=0){
			asoc_gpu_fun_change_man(&freq_wanted,"GPU3D_HYDCLK",pll);
		}
		asoc_gpu.bactive_clk = 0;
		asoc_gpu.clk_mode = mode;
		if(mode == GPU_NORMAL){
			err = set_gpu_vdd(gpu_regulator,1125000);
			if(err == -1){
				DEBUG_PRINTK("set_gpu_vdd error!\n");
			}
		}

	}

	return ret;
}

int asoc_gpu_fun_adj_man(unsigned long freq_wanted,const char *pllbuf){
	int ret=-1;
	unsigned long freq;
	struct clk *pll= clk_get(NULL, pllbuf);
	freq = clk_round_rate(pll, freq_wanted);
	if (freq!=0) {
		//DEBUG_PRINTK("asoc_gpu_fun_adj_man %s :%ld\n",pllbuf,freq);  
		ret = clk_set_rate(pll, freq);
		if (ret != 0) {
			DEBUG_PRINTK("failed to set pll, %s\n", pllbuf);
		}
	} else {
		//DEBUG_PRINTK("freq: %ld is much lower than freq_wanted: %ld\n", freq, freq_wanted);
	}
	clk_put(pll);
	return ret;
}

int asoc_gpu_fun_param_set(u32 freq_in_mhz,const char* pll,enum gpu_clk_mode clk_mode){
	if(clk_mode == GPU_NORMAL){
		asoc_gpu.normal_clk = freq_in_mhz;
		strcpy(asoc_gpu.normal_pll, pll);
	}else if(clk_mode == GPU_PERFORMANCE){
		asoc_gpu.performance_clk = freq_in_mhz;
		strcpy(asoc_gpu.performance_pll, pll);
	}else if(clk_mode == GPU_HIGH){
		asoc_gpu.high_clk = freq_in_mhz;
		strcpy(asoc_gpu.high_pll, pll);
	}else if(clk_mode == STRATEGY){
		asoc_gpu.strategy = freq_in_mhz;
	}else {
		return -1;
	}
	return 0;
}

int set_gpu_vdd(struct regulator *gpu_regulator,int voltage)
{
	int ret;
	if(gpu_regulator!=NULL){
		ret = regulator_set_voltage(gpu_regulator, voltage, voltage);
	}else{
		printk("gpu regulator NULL!\n");
		ret = -1;
	}
	return ret;
}

int set_gpu_index(u32 index){
	asoc_gpu.SGXDeviceIndex = index;
	return 0;
}

int asocPreClockChange(u32 SGXIndex){
	asoc_gpu.ClockChangeCounter++;
	if(asoc_gpu.ClockChangeCounter<=1)
		PVRSRVDevicePreClockSpeedChange(SGXIndex,1,NULL);
	return 0;
}

int asocPostClockChange(u32 SGXIndex){
	if(asoc_gpu.ClockChangeCounter<=1)
		PVRSRVDevicePostClockSpeedChange(SGXIndex,1,NULL);
	asoc_gpu.ClockChangeCounter--;
	return 0;
}

long GPU_Coreclk_Get(){
	return asoc_gpu.current_coreclk;
}
