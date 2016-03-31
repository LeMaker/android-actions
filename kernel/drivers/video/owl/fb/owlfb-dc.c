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
#include <linux/of.h>
#include <video/owldss.h>
#include <video/owlfb.h>
#include <video/owldisplay.h>

#include "owlfb.h"

#define OWLFB_DC_DISCARD_FRAME

#ifdef OWLFB_DC_DISCARD_FRAME
static atomic_t queue_cnt = ATOMIC_INIT(0);
#endif
struct owlfb_dc owl_dc;

static struct owlfb_device * my_fbdev = NULL;
static bool is_fb_memory_freed_after_dc = false;

extern int dss_mgr_enable(struct owl_overlay_manager *mgr);

extern int dss_mgr_disable(struct owl_overlay_manager *mgr);

extern bool dss_check_channel_boot_inited(enum owl_de_path_id channel);

enum disp_info_state {
	STATUS_DISPINFO_NEEDS_INIT = 0,
	STATUS_DISPINFO_PREPARED   = 1,
	STATUS_DISPINFO_QUEUED     = 2,
	STATUS_DISPINFO_ACTIVE     = 3,
	STATUS_DISPINFO_DONE       = 4,
	STATUS_DISPINFO_ERROR      = 5,
	STATUS_DISPINFO_IDLE       = 6,
};
static int owlfb_dc_queue_buffer(struct owlfb_dc * dispc , setup_dispc_data_t * psDispcData, callback cb, callback_arg cba)
{
	int rc = 0;
	int i = 0;
	unsigned long flags;
	
	struct owl_disp_info * new_info = NULL;		
	
	for(i = 0; i < OWL_DCQ_DEPTH; i++){		
		
		struct owl_disp_info * disp_info  = &dispc->dc_queue[i];		
		spin_lock_irqsave(&disp_info->info_lock,flags);		
		if(disp_info->state == STATUS_DISPINFO_NEEDS_INIT){
		   spin_unlock_irqrestore(&disp_info->info_lock,flags);		   
		   new_info = disp_info;

		   break;
		}
		spin_unlock_irqrestore(&disp_info->info_lock,flags);
	}	
	
	if(new_info == NULL){		
		printk("qeueu is full \n");
		rc = -ENOMEM;		
		goto q_err;
	}
	
	memcpy(&(new_info->psDispcData), psDispcData,	sizeof(setup_dispc_data_t));	
	
	spin_lock_irqsave(&new_info->info_lock,flags);	
	new_info->mCallBack = cb;
		   
	new_info->mCallBackArg = cba;		 
    
	new_info->state = STATUS_DISPINFO_QUEUED;
	spin_unlock_irqrestore(&new_info->info_lock,flags);
	mutex_lock(&dispc->dc_lock);
	
	/* Add it to the incoming queue */
	list_add_tail(&(new_info->list), &dispc->q_list); 
    
	mutex_unlock(&dispc->dc_lock);

	if(unlikely(!is_fb_memory_freed_after_dc) && my_fbdev != NULL){
		
		owlfb_free_all_fbmem_after_dc(my_fbdev);
		
		is_fb_memory_freed_after_dc = true;
		
	}
	return rc;

q_err:
	mutex_unlock(&dispc->dc_lock);
	return rc;
}

static int owlfb_dc_mark_buffer_done(struct owlfb_dc * dispc, int index)
{
	unsigned long flags;
	int rc = 0;
	
	int i = 0;

	for(i = 0 ; i < OWL_DCQ_DEPTH; i++){
    	    		
		struct owl_disp_info * disp_info  = &dispc->dc_queue[i];
		
		if(disp_info != NULL){			
			if(disp_info->state == STATUS_DISPINFO_ACTIVE && index != i){
				
				if(disp_info->mCallBack != NULL){
#ifdef DEBUG
					trace_buffer_release(disp_info->mCallBackArg);
#endif 
					disp_info->mCallBack(disp_info->mCallBackArg,1);													
				#ifdef OWLFB_DC_DISCARD_FRAME
					atomic_dec(&queue_cnt);
				#endif
				}
				spin_lock_irqsave(&disp_info->info_lock,flags);								
				disp_info->state = STATUS_DISPINFO_NEEDS_INIT;
				spin_unlock_irqrestore(&disp_info->info_lock,flags);
			}
		}
	}	
	return rc;
}

/*
 * Dynamic assign overlayers according to the display information, the rules is:
 * 	Put all primary layers from layer1 to layerN, and put all second layers
 * 	from layerN to layer1, while N is the available overlay number.
 * 	It is used to avoid that layers switch between two path frequently,
 *	which will lead to blurred screen on LCD or HDMI.
 */

static int boot_hdmi_enable = 0;
static int boot_hdmi_status = 0;
static int boot_hdmi_rotate = 0;
#define HDMI_STATUS_NOT_INIT 0
#define HDMI_STATUS_BOOT_INIT 1
#define HDMI_STATUS_ANDROID_INIT 2
static int boot_cvbs_rotate = 0;
static int boot_cvbs_status = 0;
static int boot_cvbs_enable = 0;
#define CVBS_STATUS_BOOT_INIT 1
#define CVBS_STATUS_NOT_INIT 0
#define CVBS_STATUS_ANDROID_INIT 2
atomic_t want_close_external_devices = ATOMIC_INIT(false);

static int owlfb_dc_arrange_overlay(setup_dispc_data_t *psDispcData,
					struct owl_overlay **used_ovl,struct owl_overlay **boot_external_used_ovl) {
	const int num_ovls = owl_dss_get_num_overlays();
	int primary_used_overlay = 0;
	int external_used_overlay = 0;
	int no_used_overlay = 0;
	int used_overlay_num = 0;
	int i = 0;

	for (i = 0; i < num_ovls; i++) {
		if (i < psDispcData->primary_display_layer_num) {
			struct owl_overlay *ovl = owl_dss_get_overlay(primary_used_overlay);
			if (ovl->manager != NULL
				&& ovl->manager->id != owl_dc.primary_manager->id) {
					  ovl->disable(ovl);
			    	ovl->unset_manager(ovl);
			    	ovl->set_manager(ovl,owl_dc.primary_manager); 
			}
			used_ovl[used_overlay_num++] = ovl;
			primary_used_overlay ++;	
		} else if (i < psDispcData->post2_layers) {
			struct owl_overlay *ovl = owl_dss_get_overlay(num_ovls - external_used_overlay - 1);			
			if (ovl->manager != NULL
				&& ovl->manager->id != owl_dc.external_manager->id) {
					  ovl->disable(ovl);
			    	ovl->unset_manager(ovl);
			    	ovl->set_manager(ovl,owl_dc.external_manager); 
			}
			used_ovl[used_overlay_num++] = ovl;
			external_used_overlay ++;	
		} else {
			struct owl_overlay *ovl = owl_dss_get_overlay(primary_used_overlay
									+ no_used_overlay);
			if (ovl->is_enabled(ovl)) {
					ovl->disable(ovl);
			}
			no_used_overlay ++;
		}
	}

	if (primary_used_overlay != 0) {
		 dss_mgr_enable(owl_dc.primary_manager);
	}
	
	if (external_used_overlay != 0) {
		dss_mgr_enable(owl_dc.external_manager);
		boot_hdmi_status = HDMI_STATUS_ANDROID_INIT;
		boot_cvbs_status=CVBS_STATUS_ANDROID_INIT;
	
	}else{
		if(boot_hdmi_status == HDMI_STATUS_ANDROID_INIT||boot_cvbs_status==CVBS_STATUS_ANDROID_INIT){
			if(atomic_read(&want_close_external_devices)){	
				owl_dc.external_manager->apply(owl_dc.external_manager);
				owl_dc.external_manager->wait_for_go(owl_dc.external_manager);	
				owl_dc.external_manager->device->driver->disable(owl_dc.external_manager->device);
				atomic_set(&want_close_external_devices,false);
			}
		}else{
			if(boot_hdmi_status == HDMI_STATUS_NOT_INIT 
				&& dss_check_channel_boot_inited(OWL_DSS_OVL_MGR_EXTERNAL)
				&& boot_hdmi_enable == 1)
			{
				boot_hdmi_status = HDMI_STATUS_BOOT_INIT;
			}
			if(boot_cvbs_status == CVBS_STATUS_NOT_INIT
				&& dss_check_channel_boot_inited(OWL_DSS_OVL_MGR_EXTERNAL)
				&& boot_cvbs_enable == 1)
			{
				boot_cvbs_status = CVBS_STATUS_BOOT_INIT;
			}


			
			if(boot_hdmi_status == HDMI_STATUS_BOOT_INIT||boot_cvbs_status == CVBS_STATUS_BOOT_INIT){
				for(i = 0 ; i < primary_used_overlay; i++)
				{
					struct owl_overlay *ovl = owl_dss_get_overlay(num_ovls - i - 1);
					if (ovl->manager != NULL
					&& ovl->manager->id != owl_dc.external_manager->id) {
						  ovl->disable(ovl);
				    	ovl->unset_manager(ovl);
				    	ovl->set_manager(ovl,owl_dc.external_manager); 
					}
					boot_external_used_ovl[external_used_overlay++] = ovl;			
				}
				dss_mgr_enable(owl_dc.external_manager);
			}	
		}		
	}
	
	return used_overlay_num;
}
static int owlfb_dc_check_frame_scale_by_display(struct owl_dss_device *dssdev,
		struct owl_overlay_info *info)
{
	u16 dw, dh;
	
	dssdev->driver->get_resolution(dssdev,&dw,&dh);
	
	if(my_fbdev->xres == dw && my_fbdev->yres == dh)
	{
		return 0;
	}
	//printk("dw %d  dh %d my_fbdev->xres %d ,my_fbdev->yres %d \n",dw, dh,my_fbdev->xres ,my_fbdev->yres);
	//printk("window (%d %d %d %d ) \n",info->pos_x,info->pos_y,info->out_width,info->out_height);
	info->out_width = info->out_width * dw / my_fbdev->xres; 
	info->out_height = info->out_height *  dh / my_fbdev->yres;
			
	info->pos_x =  info->pos_x * dw /  my_fbdev->xres;
	info->pos_y =  info->pos_y * dh / my_fbdev->yres ;

}

static int hdmi_discard_frame = 0;

static int owlfb_dc_update_overlay(struct owl_disp_info * disp_info)
{
	const int num_ovls = owl_dss_get_num_overlays();
	int used_overlay = 0;
	bool need_enable_mmu = false;
	struct owl_overlay *used_ovl[num_ovls];
	struct owl_overlay *boot_external_used_ovl[num_ovls];
	int rc = 0;
	int i = 0;
	
	setup_dispc_data_t *psDispcData = &disp_info->psDispcData;
	
	if (psDispcData == NULL){
		printk("psDispcData  is NULL \n");
		return -1;
	}

	used_overlay = owlfb_dc_arrange_overlay(psDispcData, used_ovl,boot_external_used_ovl);

	for(i = 0 ;i < used_overlay ;i++){			
		struct owl_overlay * ovl = used_ovl[i];	
		__disp_layer_info_t * layer = &psDispcData->layer_info[i];	
				
		struct owl_overlay_info info;	

		ovl->get_overlay_info(ovl, &info);

		info.color_mode = layer->fb.format;
		info.img_width = layer->fb.size.width;
		info.img_height = layer->fb.size.height;
		
		info.xoff =  layer->src_win.x;
	  	info.yoff =  layer->src_win.y;
		info.width =  layer->src_win.width;
		info.height =  layer->src_win.height;
			
		info.pos_x = layer->scn_win.x;
		info.pos_y = layer->scn_win.y;
		info.out_width = layer->scn_win.width;
		info.out_height = layer->scn_win.height;
		if (i < psDispcData->primary_display_layer_num) {
			owlfb_dc_check_frame_scale_by_display(ovl->manager->device,&info);
		}	
				
		info.rotation =	layer->rotate;
		if(layer->fb.buffer_id != -1){
			info.buffer_id    =  layer->fb.buffer_id;
			info.paddr = 0;
			need_enable_mmu = true;
		}else{
			info.paddr =  layer->fb.addr[0];
			need_enable_mmu = false;
		}
		info.global_alpha_en = layer->alpha_en;
		info.global_alpha =  layer->alpha_val; 
		info.pre_mult_alpha_en =  layer->fb.pre_multiply;  
		  
		ovl->set_overlay_info(ovl,&info);
				    
		ovl->enable(ovl);
	}

	if(boot_hdmi_status == HDMI_STATUS_BOOT_INIT||boot_cvbs_status == CVBS_STATUS_BOOT_INIT){
		hdmi_discard_frame ++;
		for(i = 0 ;i < used_overlay ;i++){	
			struct owl_overlay * ovl = boot_external_used_ovl[i];	
			__disp_layer_info_t * layer = &psDispcData->layer_info[i];	
					
			struct owl_overlay_info info;	
	
			ovl->get_overlay_info(ovl, &info);
	
			info.color_mode = layer->fb.format;
			info.img_width = layer->fb.size.width;
			info.img_height = layer->fb.size.height;
			
			info.xoff =  layer->src_win.x;
		  	info.yoff =  layer->src_win.y;
			info.width =  layer->src_win.width;
			info.height =  layer->src_win.height;
				
			info.pos_x = 0;
			info.pos_y = 0;
			ovl->manager->device->driver->get_resolution(
			 ovl->manager->device, 
			 &info.out_width, 
			 &info.out_height);
				
			if(boot_hdmi_status == HDMI_STATUS_BOOT_INIT)
			{
				info.rotation =	boot_hdmi_rotate;
			}
			if(boot_cvbs_status == CVBS_STATUS_BOOT_INIT)
			{
				info.rotation =	boot_cvbs_rotate;
			}
			if(layer->fb.buffer_id != -1){
				info.buffer_id    =  layer->fb.buffer_id;
				info.paddr = 0;
			}else{
				info.paddr =  layer->fb.addr[0];
			}
			info.global_alpha_en = layer->alpha_en;
			info.global_alpha =  layer->alpha_val; 
			info.pre_mult_alpha_en =  layer->fb.pre_multiply;  
			  
			ovl->set_overlay_info(ovl,&info);		
			if(hdmi_discard_frame < 15){
				ovl->disable(ovl);
				ovl->manager->apply(ovl->manager);
				ovl->manager->wait_for_go(ovl->manager);
			}else{
				ovl->enable(ovl);
			}
		}
		
	}

#ifdef DEBUG
	trace_buffer_put_to_dehw(disp_info->mCallBackArg);
#endif 

	for(i = 0 ; i < 2 ; i++){
		struct owl_overlay_manager *mgr;
		mgr = owl_dss_get_overlay_manager(i);
		if(need_enable_mmu && mgr->id == OWL_DSS_OVL_MGR_PRIMARY){
			mgr->set_mmu_state(mgr,MMU_STATE_PRE_ENABLE);
		}	
		mgr->apply(mgr);
	}

	if(!owl_dss_is_devices_suspended())
	{
		for(i = 0 ; i < 2 ; i++){
			struct owl_overlay_manager *mgr;
			mgr = owl_dss_get_overlay_manager(i);
			mgr->wait_for_go(mgr);	
		}
	}

	return rc ;
}

static void owlfb_dc_perform_update(struct work_struct *work)
{
	struct owlfb_dc * dispc;
	struct owl_disp_info * disp_info;
	unsigned long flags;
    
	dispc = container_of(work, struct owlfb_dc, dc_work);
	
	if(dispc == NULL){
		printk("dispc is null\n");
		return ;
	}
	
	while(true){
		
		dispc->working  = true;
		
		mutex_lock(&dispc->dc_lock);
		
		if(!list_empty(&dispc->q_list)){			
			disp_info = list_entry(dispc->q_list.next, struct owl_disp_info, list); 
        	list_del(&(disp_info->list));
		}else{
			mutex_unlock(&dispc->dc_lock);
			dispc->working = false;	
			return;
		}		
		mutex_unlock(&dispc->dc_lock);
		
		
		spin_lock_irqsave(&disp_info->info_lock,flags);
		
		disp_info->state = STATUS_DISPINFO_ACTIVE;   
		
		spin_unlock_irqrestore(&disp_info->info_lock,flags);
    	     				
	#ifdef OWLFB_DC_DISCARD_FRAME
		if (atomic_read(&queue_cnt) < OWL_DCQ_DEPTH - 1) {
			owlfb_dc_update_overlay(disp_info);		
		} else {
			printk(KERN_INFO "%s: cnt %d, discard a frame!\n", 
				__func__, atomic_read(&queue_cnt));
		}
	#else
		owlfb_dc_update_overlay(disp_info);		
	#endif

		owlfb_dc_mark_buffer_done(dispc,disp_info->index);	
		
	}
	
	dispc->working = false;	
	return;
}

void check_boot_hdmi_rotate_config(void)
{
	struct device_node *np = NULL;
	struct device_node *cp = NULL;
	
	np = of_find_compatible_node(NULL, NULL, "actions,atm7059a-hdmi");
	cp = of_find_compatible_node(NULL, NULL, "actions,atm7059a-cvbs");
	
	printk("np %p \n",np);
	if(np != NULL){
		if (of_property_read_u32(np, "bootrotate", &boot_hdmi_rotate)){
			boot_hdmi_rotate = 0;
		}
		if (of_property_read_u32(np, "bootable", &boot_hdmi_enable)){
			boot_hdmi_enable = 0;
		}	
		printk("boot_hdmi_rotate %d \n",boot_hdmi_rotate);	
	}
	
	if(cp != NULL){
		
		if (of_property_read_u32(cp, "bootable", &boot_cvbs_enable)){
				printk("of_property_read_u32 is error\n");
			boot_cvbs_enable = 0;
		}	
		if (of_property_read_u32(cp, "bootrotate", &boot_cvbs_rotate)){
				printk("of_property_read bootrotate is error\n");
			boot_cvbs_rotate = 0;
		}	
		printk("boot_cvbs_enable %d \n",boot_cvbs_enable);	
	}
	
	boot_cvbs_rotate =0;
	
	printk("boot_hdmi_rotate %d boot_cvbs_rotate=%d \n",boot_hdmi_rotate,boot_cvbs_rotate);
	return;
}

int owlfb_dc_init(struct owlfb_device * fbdev)
{

	int rc = 0;	
	int i = 0;
	
	my_fbdev = fbdev;
	
	owl_dc.dev = fbdev->dev;
	
	owl_dc.primary_manager = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_PRIMARY);
	owl_dc.external_manager = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_EXTERNAL);
	
	/*we always used path1 connected to primary display */
	owl_dc.primary_manager->de_path_id = OWL_DSS_PATH1_ID;	
	owl_dc.external_manager->de_path_id = OWL_DSS_PATH2_ID;	
	 
	mutex_init(&owl_dc.dc_lock);
	
	INIT_LIST_HEAD(&owl_dc.q_list);	
	
	for(i = 0; i < OWL_DCQ_DEPTH; i++){
		
		struct owl_disp_info * disp_info  = &owl_dc.dc_queue[i];
		
		spin_lock_init(&disp_info->info_lock);
		
		disp_info->state = STATUS_DISPINFO_NEEDS_INIT;
		
		disp_info->index = i;
	}
	
	owl_dc.working  = false;
	
		
	owl_dc.dc_workqueue = create_singlethread_workqueue("owlfb");
	
	if (owl_dc.dc_workqueue == NULL){
		rc = -ENOMEM;
		goto failed;
	}
	
#ifdef DEBUG	
	//owlfb_dc_create_debug_sysfs(fbdev);
#endif

	INIT_WORK(&(owl_dc.dc_work), owlfb_dc_perform_update);	
	
	check_boot_hdmi_rotate_config();

failed:	
	return rc;
}

int owlfb_dc_uninit(struct owlfb_device * fbdev)
{
	flush_workqueue(owl_dc.dc_workqueue);
	
	destroy_workqueue(owl_dc.dc_workqueue);
	
	owl_dc.dc_workqueue = NULL;
	
	mutex_destroy(&owl_dc.dc_lock);
	
	return 0;
}

int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, int ui32DispcDataLength, void (*cb_fn)(void *, int),void *cb_arg)
{
	
	int rc = 0;
	
#ifdef OWLFB_DC_DISCARD_FRAME
	atomic_inc(&queue_cnt);
#endif

#ifdef DEBUG
	trace_buffer_put_to_queue(cb_arg);
#endif
	rc = owlfb_dc_queue_buffer(&owl_dc, psDispcData, cb_fn, cb_arg);
		
	if (rc)
		return rc;

	if(owl_dc.dc_workqueue != NULL){
		queue_work(owl_dc.dc_workqueue, &(owl_dc.dc_work));
	}	
	return rc;
}

EXPORT_SYMBOL(dispc_gralloc_queue);
