/*
 * linux/drivers/video/owl/owlfb-main.c
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
 *
 * Some code and ideas taken from drivers/video/owl/ driver
 * by leopard.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <video/owldss.h>
#include <video/owlfb.h>
#include <video/owldisplay.h>
#include <linux/hrtimer.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include "owlfb.h"

#define DCU_BASE_ADDR  0xb0290000
#define CMU_DDRPLL_REG 0xb0160008
#define DEFAULT_SIMPLE_RATE 1000 //ms
#define DEFAULT_MAX_STATISTIC_CNT 1000 //times
#define MAX_MASTER_NUMBER 2 
#define MS_TO_NS(x) (x * 1000000)

enum master_id {
	MASTER_ID_CPU = 0,
	MASTER_ID_USB3,
	MASTER_ID_VCE,
	MASTER_ID_ENTHERNET,
	MASTER_ID_USB2,
	MASTER_ID_DE,
	MASTER_ID_GPU3D,
	MASTER_ID_SI,
	MASTER_ID_DMA,
	MASTER_ID_DAP,
	MASTER_ID_ALL,
	MASTER_ID_IDLE,
};
const char * name_of_master[MASTER_ID_IDLE + 1] = {
	"CPU",
	"USB3",
	"VCE",
	"ENTHERNET",
	"USB2",
	"DE",
	"GPU3D",
	"SI",
	"DMA",
	"DAP",
	"ALL",
	"IDLE",
};
enum master_mode {
	MASTER_MODE_READ = 0,
	MASTER_MODE_WRITE = 1,
	MASTER_MODE_ALL = 2,
};
const char * name_of_mode[MASTER_MODE_ALL + 1] = {
	"R",
	"W",
	"RW",	
};
struct master_info{
	int id;
	int mode;
};

struct staticstic_result{
	int pc[MAX_MASTER_NUMBER];
};

struct dmm_statistic_info{
	struct dentry *fs_rootdir;
	struct dentry *fs_enable;
	struct dentry *fs_statistic_cnt;
	struct dentry *fs_sampling_rate;
	struct dentry *fs_max_statistic_cnt;
	struct dentry *fs_master_root[MAX_MASTER_NUMBER];
	struct dentry *master_id[MAX_MASTER_NUMBER];
	struct dentry *master_mode[MAX_MASTER_NUMBER];
	struct dentry *fs_result;
	
	u32 debug_enable;
	bool actived;
	u32 statistic_cnt;
	u32 max_statistic_cnt;
	struct hrtimer timer;	
    u32 sampling_rate;
	int ddr_clk; // MHZ
	void __iomem * DMM_PM_CTRL0;
	void __iomem * DMM_PC0;
	void __iomem * DMM_PC1;
    void __iomem * CMU_DDRPLL;
	struct master_info master[MAX_MASTER_NUMBER];
	struct staticstic_result * result;
};

static struct dmm_statistic_info dmm;

void dmm_start_statistic(struct dmm_statistic_info * info) 
{
	int i = 0;
	int temp_dmm_ctrl = 0;
	
    writel(0,info->DMM_PM_CTRL0); //WRITE 0 TO CLEAR PC0 AND PC1
    
    for(i = 0; i < MAX_MASTER_NUMBER; i++)
    {
    	int temp = 0;
    	struct master_info * master = &info->master[i];
    	temp |=  master->mode << 4;
    	switch(master->id){
    		case MASTER_ID_ALL:
    		  temp |= (1 << 8);
    		  break;
    		case MASTER_ID_IDLE:
    		  temp |= (2 << 8);
    		  break;
    	    default:
    	      temp |= (0 << 8);
    	      temp |= master->id;
    	      break;
    	}
    	temp |= (1 << 15);
    	temp_dmm_ctrl |= (temp << ( i * 16));
    	
    }
    
    writel(temp_dmm_ctrl,info->DMM_PM_CTRL0); //WRITE 0 TO CLEAR PC0 AND PC1 
}

void dmm_get_statistic_result(struct dmm_statistic_info * info) 
{
	struct staticstic_result * result = NULL;
	if(info->statistic_cnt >=  info->max_statistic_cnt){
		printk(" out of memory ,you set max_statistic cnt is %d \n",info->max_statistic_cnt);
		return ;
	}
    result = &info->result[info->statistic_cnt];
    result->pc[0] = readl(info->DMM_PC0);
    result->pc[1] = readl(info->DMM_PC1);
    
    if((readl(info->DMM_PM_CTRL0) & ( (1<< 22) | (1 << 6) )) != 0){
    	printk("DMM_PM_CTRL0 overlflow 0x%x \n",readl(info->DMM_PM_CTRL0));
    }

}

static enum hrtimer_restart hr_timer_func(struct hrtimer *timer)    
{
	unsigned long missed;
	if(dmm.statistic_cnt != 0){
		dmm_get_statistic_result(&dmm);		
	}
		
	dmm_start_statistic(&dmm);
	
	missed = hrtimer_forward_now(timer, ktime_set(0, MS_TO_NS(dmm.sampling_rate)));
	
	if (missed > 1)
		printk("Missed ticks %ld\n", missed - 1);
	dmm.statistic_cnt ++;	
 	return HRTIMER_RESTART;
}

static ssize_t dmm_debug_read(struct file *filp, char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[3];	

	u32 *val = filp->private_data;

	if (*val)
	{
		buf[0] = 'Y';
	}
	else
	{
		buf[0] = 'N';
	}
	buf[1] = '\n';
	buf[2] = 0x00;

	return simple_read_from_buffer(user_buf, count, ppos, buf, 2);
}
static ssize_t dmm_result_read(struct file *filp, char __user *user_buf, size_t count, loff_t *ppos)
{
	ssize_t out_count = PAGE_SIZE * 30, offset = 0;
	char * buf;
	int i = 0,r;
	int j = 0;
	long total_bandwidth = dmm.ddr_clk * 8 * dmm.sampling_rate / 1000;
	
	buf = kmalloc(out_count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;	
		
	offset += snprintf(buf + offset, out_count - offset, "%16s: ","master id");	
	offset += snprintf(buf + offset, out_count - offset, "%6s","mode");
	offset += snprintf(buf + offset, out_count - offset, "%20s","bandwidth(M byte)");
	offset += snprintf(buf + offset, out_count - offset, "%24s ","percent of total(%)");	
	offset += snprintf(buf + offset, out_count - offset, "%16s: ","master id");	
	offset += snprintf(buf + offset, out_count - offset, "%6s","mode");
	offset += snprintf(buf + offset, out_count - offset, "%20s","bandwidth(M byte)");
	offset += snprintf(buf + offset, out_count - offset, "%24s ","percent of total(%)\n");	
	
	for(i = 0 ; i < dmm.statistic_cnt; i++)
	{
		struct staticstic_result * result = &dmm.result[i];		   
		
		for(j = 0 ; j < MAX_MASTER_NUMBER ; j++)
		{
			long bandwidth = result->pc[j] * 16 / (1024 * 1024);
			offset += snprintf(buf + offset, out_count - offset, "%16s:",name_of_master[dmm.master[j].id]);
			offset += snprintf(buf + offset, out_count - offset, "%6s",name_of_mode[dmm.master[j].mode]);
			
			offset += snprintf(buf + offset, out_count - offset, "%18ld   ",bandwidth);	
			if(dmm.master[j].id == MASTER_ID_IDLE){			
				offset += snprintf(buf + offset, out_count - offset, "%24ld", 100  - bandwidth * 100 / total_bandwidth);			
			}else{
				offset += snprintf(buf + offset, out_count - offset, "%24ld",bandwidth * 100 / total_bandwidth);			
			}
		}
		offset += snprintf(buf + offset, out_count - offset, " \n");	
	}
	r =  simple_read_from_buffer(user_buf, count, ppos, buf, offset);
	kfree(buf);	
	return r;
}

static ssize_t dmm_debug_write(struct file *filp, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	int buf_size;
	
	u32 *val = filp->private_data;
	
	buf_size = min(count, (sizeof(buf)-1));
	
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;
	
	switch (buf[0]) {
		case 'y':
		case 'Y':
		case '1':
			*val = 1;
		break;
		case 'n':
		case 'N':
		case '0':
			*val = 0;
		break;
	}
	
	dmm.debug_enable = *val;
	
	if(dmm.debug_enable){
		if(!dmm.actived){
			dmm.statistic_cnt = 0;
			if(dmm.result != NULL){
				kfree(dmm.result);
			}
			dmm.result = kmalloc(sizeof(struct staticstic_result)* dmm.max_statistic_cnt, GFP_KERNEL);
			hrtimer_start(&dmm.timer, ktime_set(0, MS_TO_NS(dmm.sampling_rate)),HRTIMER_MODE_REL);
			dmm.actived = true;
		}else{
			printk("already actived debug\n");
		}
	}else{
		if(dmm.actived){
			hrtimer_cancel(&dmm.timer);
			dmm.actived = false;			
		}else{
			printk("already stopped debug \n");
		}
	}	
	pr_info("dmm debug enable %d \n",dmm.debug_enable);
	return count;
}
static int dmm_debug_open(struct inode *inode, struct file *filp)
{
    filp->private_data = inode->i_private;
    return 0;
}

static struct file_operations dmm_debug_fops = {
	.open = dmm_debug_open,
	.read = dmm_debug_read,
	.write = dmm_debug_write,
};

static struct file_operations dmm_result_fops = {
	.open = simple_open,
	.read = dmm_result_read,
	.llseek = default_llseek,
};

static int owl_dmm_create_debug_sysfs(void)
{
	int rc = 0;
	int i;
    
    
    dmm.fs_rootdir = debugfs_create_dir("dmm",NULL);
    
    
    if (IS_ERR(dmm.fs_rootdir)) {
		rc = PTR_ERR(dmm.fs_rootdir);
		dmm.fs_rootdir = NULL;
		goto out;
	}
	
	dmm.fs_enable = debugfs_create_file("enable",S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dmm.fs_rootdir, &dmm.debug_enable , &dmm_debug_fops); 
	if (dmm.fs_enable == NULL){
		rc = -EIO;
		pr_err("debug file enable create failed \n");
		goto out;
	}
	
	dmm.fs_result = debugfs_create_file("result",S_IRUSR  | S_IRGRP , dmm.fs_rootdir, NULL , &dmm_result_fops); 
	if (dmm.fs_enable == NULL){
		rc = -EIO;
		pr_err("debug file result create failed \n");
		goto out;
	}
	
	dmm.fs_max_statistic_cnt = debugfs_create_u32("max_statistic_cnt", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dmm.fs_rootdir,&dmm.max_statistic_cnt);
	if (dmm.fs_max_statistic_cnt == NULL){
		rc = -EIO;
		pr_err("debug file max_statistic_cnt create failed \n");
		goto out;
	}
	
	dmm.fs_sampling_rate = debugfs_create_u32("sampling_rate", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dmm.fs_rootdir,&dmm.sampling_rate);
	if (dmm.fs_sampling_rate == NULL){
		rc = -EIO;
		pr_err("debug file sampling_rate create failed \n");
		goto out;
	}
	
	dmm.fs_statistic_cnt = debugfs_create_u32("statistic_cnt", S_IRUSR  | S_IRGRP, dmm.fs_rootdir,&dmm.statistic_cnt);
	if (dmm.fs_statistic_cnt == NULL){
		rc = -EIO;
		pr_err("debug file sampling_rate create failed \n");
		goto out;
	}
		
	for(i = 0 ; i < MAX_MASTER_NUMBER;i++){
		
		if(i == 0){ 
			dmm.fs_master_root[i] = debugfs_create_dir("master0",dmm.fs_rootdir);    
    	}else{
    		dmm.fs_master_root[i] = debugfs_create_dir("master1",dmm.fs_rootdir);
    	}
    	
	    if (IS_ERR(dmm.fs_master_root[i] )) {
			rc = PTR_ERR(dmm.fs_rootdir);
			dmm.fs_rootdir = NULL;
			pr_err("debug file master create failed\n");
			goto out;
		}
		
		dmm.master_id[i] = debugfs_create_u32("id", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dmm.fs_master_root[i] , &dmm.master[i].id);
		if (dmm.master_id[i] == NULL){
			rc = -EIO;
			pr_err("debug file id create failed \n");
			goto out;
		}
		dmm.master_mode[i] = debugfs_create_u32("mode", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,dmm.fs_master_root[i] ,&dmm.master[i].mode);
		if (dmm.master_mode[i] == NULL){
			rc = -EIO;
			pr_err("debug file mode create failed\n");
			goto out;
		}
	}
	pr_info("dmm debugfs support\n");

out:
	if (rc)
		pr_warn("unable to init\n");   
	
	return rc;
}

static void owl_dmm_remove_debug_sysfs(void)
{
	int i;
	for(i = 0 ; i < MAX_MASTER_NUMBER;i++){
		debugfs_remove(dmm.master_mode[i]);
		debugfs_remove(dmm.master_id[i]);
		debugfs_remove(dmm.fs_master_root[i]);
	}
	debugfs_remove(dmm.fs_max_statistic_cnt);
	debugfs_remove(dmm.fs_sampling_rate);
	debugfs_remove(dmm.fs_statistic_cnt);
	debugfs_remove(dmm.fs_enable);
	debugfs_remove(dmm.fs_result);
	debugfs_remove(dmm.fs_rootdir);	
	
	return;
}

int __init dmm_debug_init_function(void)
{
	
	dmm.DMM_PM_CTRL0 = ioremap(DCU_BASE_ADDR + 0x48,4);
	dmm.DMM_PC0 = ioremap(DCU_BASE_ADDR + 0x50,4);
	dmm.DMM_PC1 = ioremap(DCU_BASE_ADDR + 0x54,4);
	dmm.CMU_DDRPLL = ioremap(CMU_DDRPLL_REG,4);
	
	dmm.ddr_clk = (readl(dmm.CMU_DDRPLL)& 0xff) * 12;
	
	hrtimer_init(&(dmm.timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);   
	dmm.timer.function = hr_timer_func;
	dmm.sampling_rate = DEFAULT_SIMPLE_RATE;
	
	dmm.master[0].id = MASTER_ID_ALL;
	dmm.master[0].mode = MASTER_MODE_ALL;

	dmm.master[1].id = MASTER_ID_IDLE;
	dmm.master[1].mode = MASTER_MODE_ALL;
	dmm.max_statistic_cnt = DEFAULT_MAX_STATISTIC_CNT;
	owl_dmm_create_debug_sysfs();
	
	return 0;
		
}

void dmm_debug_uninit_function(void)
{
    iounmap(dmm.DMM_PM_CTRL0);
	iounmap(dmm.DMM_PC0);
	iounmap(dmm.DMM_PC1);
	iounmap(dmm.CMU_DDRPLL);
	hrtimer_cancel(&dmm.timer);
	owl_dmm_remove_debug_sysfs();
	
	if(dmm.result != NULL){
		kfree(dmm.result);
	}
}

arch_initcall(dmm_debug_init_function);


