#include <dlfcn.h>
#include <pthread.h>
#include "imxapi_adapter.h"
#include "imx_api.h"
#include "actal_posix_dev.h"
#include "perf.h"
#include "log.h"

#define MAX_CAPTURE_WIDTH 3264
#define MAX_CAPTURE_HEIGHT 2448
#define MAX_OFFSET 64
#define MAX_IMG_SIZE ((MAX_CAPTURE_WIDTH+MAX_OFFSET)*(MAX_CAPTURE_HEIGHT+MAX_OFFSET))

typedef struct
{
    void *phy_addr;
    void *vir_addr;
} buff_t;

typedef struct
{
    pthread_mutex_t cmd_mutex;
    
    void *handle;
    void *imx_lib;
    void *(*imx_open)(void);
    int (*imx_cmd)(void *handle, int id, void *arg);
    int (*imx_close)(void *handle);
    int (*imx_process)(void *handle, imx_img_t *imgin, imx_img_t *imgout, imx_nr_t *nr_para, imx_ee_t *ee_para);

    int res_num;
    act_isp_imx_param_t param[IMX_MAX_RES_NUM];
    void *imx_param_lib;
    int (*get_sensor_imx_param)(int sensor_id, act_isp_imx_param_t *param);
    int param_inited;

    buff_t tmp_buff;
    buff_t tmp_buff_1;
    
    /*sw ee*/
    unsigned char* detail_sw_buff0;
    unsigned char* detail_sw_buff1;
    int detail_sw_res_idx;
    int detail_sw_iso_idx;
    int detail_sw_table_init;
    unsigned char detail_sw_table[256*256];
} imxctl_inter_t;

static void *malloc_phy(int len, unsigned long *vir_addr)
{
	unsigned long phy_add;
    
	len = (len + 4095) & (~4095);
	*vir_addr = (unsigned long) actal_malloc_uncache(len, &phy_add);
	if (0 == *vir_addr) {
		OMXDBUG(OMXDBUG_ERR, "actal_malloc_uncache failed!\n");
		return NULL;
	}

	OMXDBUG(OMXDBUG_PARAM, "ion malloc %d: phy %lx, vir %lx\n", len, phy_add, *vir_addr);
	return (void *)phy_add;
}

static void free_phy(void *vir_addr)
{
	if(vir_addr) {
		OMXDBUG(OMXDBUG_PARAM, "ion free: vir %lx\n", vir_addr);
		actal_free_uncache(vir_addr);
	}
	else {
		OMXDBUG(OMXDBUG_ERR, "ion free: null address!\n");
	}
	
	return;
}

static void * enc_malloc(int size)
{
	unsigned char *mem_ptr;
	unsigned char alignment = 32;
	if (!alignment)
	{
		if ((mem_ptr = (unsigned char *) calloc(size + 1,1)) != NULL) 
		{
			*mem_ptr = (unsigned char)1;
			return ((void *)(mem_ptr+1));
		}
	} 
	else 
	{
		unsigned char *tmp;
		if ((tmp = (unsigned char *) calloc(size + alignment,1)) != NULL) 
		{
			mem_ptr =	(unsigned char *) ((uintptr_t) (tmp + alignment - 1) & (~(uintptr_t) (alignment - 1)));
			if (mem_ptr == tmp)
				mem_ptr += alignment;
			*(mem_ptr - 1) = (unsigned char) (mem_ptr - tmp);
			return ((void *)mem_ptr);
		}
	}

	printf("err!en_malloc fail,size:%d!%s,%d\n",size,__FILE__,__LINE__);
	return NULL;
}

static void enc_free( void* mem_ptr)
{
	unsigned char *ptr;
	if (mem_ptr == NULL)
	{
		printf("Warning!mem_ptr is NULL!%s,%d\n",__FILE__,__LINE__);
		return ;
	}

	ptr = (unsigned char *)mem_ptr;
	ptr -= *(ptr - 1);
	free(ptr);
	return ;
}

static int find_idx(imxctl_inter_t *handle, int width, int height)
{
    int idx = -1;
    int i;

    //OMXDBUG(OMXDBUG_PARAM, "%s, %d, %d, %d\n", __func__, handle->res_num, width, height);

    for(i = 0; i < handle->res_num; i++)
    {
        //OMXDBUG(OMXDBUG_PARAM, "%s, %d, %d\n", __func__, handle->param[i].width, handle->param[i].height);
        if(width == handle->param[i].width && height == handle->param[i].height)
        {
            idx = i;
            break;
        }
    }

    return idx;
}

static int get_imx_lib(imxctl_inter_t *handle)
{
    void *pCodaModule;
    char buf[50] = "libACT_IMX.so";
    int ret = 0;

    pCodaModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
    if(pCodaModule == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "dlopen %s failed because %s\n", buf, dlerror());
        ret = -1;
        goto err0;
    }

    handle->imx_open = dlsym(pCodaModule, "imx_open");
    if(handle->imx_open == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->imx_process = dlsym(pCodaModule, "imx_process");
    if(handle->imx_process == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->imx_close = dlsym(pCodaModule, "imx_close");
    if(handle->imx_close == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->imx_lib = pCodaModule;
    return ret;

err1:
    dlclose(pCodaModule);
    pCodaModule = NULL;

err0:
    handle->imx_lib = pCodaModule;
    return ret;
}

static int get_imx_param_lib(imxctl_inter_t *handle)
{
    void *pCodaModule;
    char buf[50] = "libACT_IMX_PARM.so";
    int ret = 0;

    pCodaModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
    if(pCodaModule == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "dlopen %s failed because %s\n", buf, dlerror());
        ret = -1;
        goto err0;
    }

    handle->get_sensor_imx_param = dlsym(pCodaModule, "get_sensor_imx_param");
    if(handle->get_sensor_imx_param == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
        ret = -2;
        goto err1;
    }

    handle->imx_param_lib = pCodaModule;
    return ret;

err1:
    dlclose(pCodaModule);
    pCodaModule = NULL;

err0:
    handle->imx_param_lib = pCodaModule;
    return ret;
}

int imxctl_init(void *handle, int sensor_id)
{
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;
    int ret = 0;
    
    ctl_handle->detail_sw_table_init = 0;
    if(0 == get_imx_param_lib(ctl_handle))
    {
        ctl_handle->res_num = ctl_handle->get_sensor_imx_param(sensor_id, ctl_handle->param);
        if(0 == ctl_handle->res_num)
        {
            OMXDBUG(OMXDBUG_PARAM, "failed to get imx param by sensor id\n");
            ctl_handle->param_inited = 0;
        }
        else
        {
            ctl_handle->param_inited = 1;
        }
    }
    else
    {
        ctl_handle->param_inited = 0;
    }

    {
//	        int idx;
//	        for(idx = 0; idx < ctl_handle->res_num; idx++)
//	        {
//	            OMXDBUG(OMXDBUG_PARAM, "%s, %dx%d;%d-%d;%d-%d;%d,%d,%d;%g,%g,%g,%d,%d\n", __func__, ctl_handle->param[idx].width, ctl_handle->param[idx].height, 
//	                ctl_handle->param[idx].min_shutter, ctl_handle->param[idx].max_shutter, ctl_handle->param[idx].min_gain, ctl_handle->param[idx].max_gain, 
//	                ctl_handle->param[idx].iso_param[0].nr_enable, ctl_handle->param[idx].iso_param[0].nr_level, ctl_handle->param[idx].iso_param[0].nr_bl_size,
//	                ctl_handle->param[idx].iso_param[0].ee_k1, ctl_handle->param[idx].iso_param[0].ee_k2, ctl_handle->param[idx].iso_param[0].ee_k3, ctl_handle->param[idx].iso_param[0].ee_t1, ctl_handle->param[idx].iso_param[0].ee_t2);
//	        }
    }
    
    return ret;
}

int imxctl_set_param(void *handle, act_isp_imx_param_t *param)
{
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;

    pthread_mutex_lock(&ctl_handle->cmd_mutex);

    if(ctl_handle->res_num)
    {
        int idx = find_idx(ctl_handle, param->width, param->height);
        if(-1 != idx)
        {
            memcpy(&ctl_handle->param[idx], param, sizeof(act_isp_imx_param_t));
        }
        else
        {
            ctl_handle->res_num++;
            memcpy(&ctl_handle->param[ctl_handle->res_num - 1], param, sizeof(act_isp_imx_param_t));
        }
    }
    else
    {
        ctl_handle->res_num = 1;
        memcpy(&ctl_handle->param[0], param, sizeof(act_isp_imx_param_t));
        ctl_handle->param_inited = 1;
    }
    
    pthread_mutex_unlock(&ctl_handle->cmd_mutex);

    return 0;
}

int imxctl_get_param(void *handle, act_isp_imx_param_t *param)
{
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;
    int idx;
    int ret = 0;

    pthread_mutex_lock(&ctl_handle->cmd_mutex);

    idx = find_idx(ctl_handle, param->width, param->height);
    if(-1 != idx)
    {
        memcpy(param, &ctl_handle->param[idx], sizeof(act_isp_imx_param_t));
    }
    else
    {
        OMXDBUG(OMXDBUG_ERR, "%s, no param for this resolution!\n", __func__);
        ret = -1;
    }
    
    pthread_mutex_unlock(&ctl_handle->cmd_mutex);

    return ret;
}

static void imxclt_detail_table(void *handle,imx_ee_t* ee_para)
{
		int i,j;
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;
    unsigned char *detail_sw_table = ctl_handle->detail_sw_table;
    int src,dst,detail,ee;
    int k1 = (int)(ee_para->k1 * (1<<10));
    int k2 = (int)(ee_para->k2 * (1<<10));
    int k3 = (int)(ee_para->k3 * (1<<10));
    int t1 = ee_para->t1;
    int t2 = ee_para->t2;
    
    for(src = 0;src < 256; src++)
    {
    	for(dst = 0;dst < 256; dst++)
    	{
    		if( ((-t1 <= (src - dst)) && ((src - dst) <= t1)) )
    		{
    			detail = (int)((k1 * (src - dst))>>10);
    		}
    		else if( (-t2 <= (src - dst)) && ((src - dst) <= t2) )
    		{
    			detail = (int)((k2 * (src - dst))>>10);
    		}
    		else/*(-t2 > (src - dst)) || (t2 < (src - dst))*/
    		{
    			detail = (int)((k3 * (src - dst))>>10);
    		}

    		ee = src + detail;
    		if(ee < 0)  ee = 0;
    		else if(ee > 255) ee = 255;
    		detail_sw_table[(src<<8) + dst] = ee;
    	}
    }
}

static void imxclt_detail_prepare(void *handle,imx_ee_t* ee_para,int res_idx,int iso_idx)
{
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;
    if(ctl_handle->detail_sw_table_init == 0)
    {
    	ctl_handle->detail_sw_table_init = 1;
    	ctl_handle->detail_sw_res_idx = res_idx;
    	ctl_handle->detail_sw_iso_idx = iso_idx;
    	imxclt_detail_table(handle,ee_para);
    }
    else if( (ctl_handle->detail_sw_res_idx != res_idx) ||  (ctl_handle->detail_sw_iso_idx != iso_idx))
    {
    	ctl_handle->detail_sw_res_idx = res_idx;
    	ctl_handle->detail_sw_iso_idx = iso_idx;
    	imxclt_detail_table(handle,ee_para);
    }
}

static int imxclt_detail_sw(void *handle,int frm_size)
{
    int i;
    double t0, t1;
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;
    unsigned char *detail_sw_table = ctl_handle->detail_sw_table;
    
    //t0 = now_ms();
    memcpy(ctl_handle->detail_sw_buff0,ctl_handle->tmp_buff.vir_addr,frm_size);
    memcpy(ctl_handle->detail_sw_buff1,ctl_handle->tmp_buff_1.vir_addr,frm_size);
    //t1 = now_ms();
    //OMXDBUG(OMXDBUG_PARAM, "time consumption of imx_process ee0: %g\n", t1 - t0);
    
    //t0 = now_ms();
#if 1
    unsigned int* src_addr = (unsigned int*)ctl_handle->detail_sw_buff0;
    unsigned int* dst_addr = (unsigned int*)ctl_handle->detail_sw_buff1;
    unsigned int s,d,v0,v1,v2,v3,idx0,idx1,idx2,idx3;
		for (i = 0; i < frm_size ; i += 4)
		{
			s = *src_addr;
			d = *dst_addr++;
			idx0 = ((s & 0xff)<<8) | (d & 0xff);
			idx1 = (s & 0xff00) | ((d & 0xff00)>>8);
			idx2 = ((s & 0xff0000)>>8) | ((d & 0xff0000)>>16);
			idx3 = ((s & 0xff000000)>>16) | ((d & 0xff000000)>>24);
			v0 = detail_sw_table[idx0];
			v1 = detail_sw_table[idx1];
			v2 = detail_sw_table[idx2];
			v3 = detail_sw_table[idx3];
			
			*src_addr++ = (v3 << 24) | (v2 << 16) | (v1 << 8) | v0;
		}
#else
    unsigned char* src_addr = ctl_handle->detail_sw_buff0;
    unsigned char* dst_addr = ctl_handle->detail_sw_buff1;
		for (i = 0; i < frm_size ; i += 4)
		{
			src_addr[0] = detail_sw_table[(src_addr[0]<<8) + dst_addr[0]];
			src_addr[1] = detail_sw_table[(src_addr[1]<<8) + dst_addr[1]];
			src_addr[2] = detail_sw_table[(src_addr[2]<<8) + dst_addr[2]];
			src_addr[3] = detail_sw_table[(src_addr[3]<<8) + dst_addr[3]];
			
			dst_addr += 4;	
			src_addr += 4;
		}
#endif
	  //t1 = now_ms();
    //OMXDBUG(OMXDBUG_PARAM, "time consumption of imx_process ee1: %g\n", t1 - t0);
       
    //t0 = now_ms();
    memcpy(ctl_handle->tmp_buff.vir_addr,ctl_handle->detail_sw_buff0,frm_size);
    //t1 = now_ms();
    //OMXDBUG(OMXDBUG_PARAM, "time consumption of imx_process ee2: %g\n", t1 - t0);
    
    return 0;
}

int imxctl_process(void *handle, unsigned char *img_phy, unsigned char *img_vir, int width, int height, 
                int xoffset, int yoffset, int format, int shutter, int gain, int mode)

{
    imxctl_inter_t *ctl_handle = (imxctl_inter_t *)handle;
    act_isp_imx_iso_param_t *iso_param;
    imx_nr_t nr_para;
    imx_ee_t ee_para_hw;
    imx_ee_t ee_para_sw;
    imx_img_t frm_in;
    imx_img_t frm_out;
    int iso;
    int frm_size;
    int ee_enable,ee_algs;
    int res_idx = 0;
    int iso_idx = 0;
    int i;
    int ret = 0;
    double t0, t1;

    pthread_mutex_lock(&ctl_handle->cmd_mutex);

    if(ctl_handle->param_inited)
    {
        // match resolution
        res_idx = find_idx(ctl_handle, width, height);
        if(-1 == res_idx)
        {
            OMXDBUG(OMXDBUG_ERR, "%s %d, no param for this resoultion!\n", __func__, __LINE__);
            nr_para.enable = 0;
            ee_para_hw.k1 = ee_para_hw.k2 = ee_para_hw.k3 = 0.0; ee_enable = 0;
            goto step1;
        }
        //OMXDBUG(OMXDBUG_PARAM, "%s %d, %d\n", __func__, __LINE__, res_idx);

        // match param level
        iso = gain / IMX_ISO_BASE;
        iso_param = ctl_handle->param[res_idx].iso_param;
        for(i = 0; i < ctl_handle->param[res_idx].iso_num; i++)
        {
            if(iso <= iso_param[i].max_iso)
            {
                iso_idx = i;
                break;
            }
        }
        //OMXDBUG(OMXDBUG_PARAM, "%s %d, %d\n", __func__, __LINE__, iso_idx);

        // set param
        nr_para.enable =  iso_param[iso_idx].nr_enable && ctl_handle->param[res_idx].nr_enable;
        
        ee_para_hw.k1 = iso_param[iso_idx].ee_k1_hw;
        ee_para_hw.k2 = iso_param[iso_idx].ee_k2_hw;
        ee_para_hw.k3 = iso_param[iso_idx].ee_k3_hw;
        ee_para_hw.t1 = iso_param[iso_idx].ee_t1_hw;
        ee_para_hw.t2 = iso_param[iso_idx].ee_t2_hw;
        ee_para_sw.k1 = iso_param[iso_idx].ee_k1_sw;
        ee_para_sw.k2 = iso_param[iso_idx].ee_k2_sw;
        ee_para_sw.k3 = iso_param[iso_idx].ee_k3_sw;
        ee_para_sw.t1 = iso_param[iso_idx].ee_t1_sw;
        ee_para_sw.t2 = iso_param[iso_idx].ee_t2_sw;
        ee_enable = iso_param[iso_idx].ee_enable && ctl_handle->param[res_idx].ee_enable;
        ee_algs = iso_param[iso_idx].ee_algs;
        if( (!ee_enable) || (ee_algs == 1) || ((ee_algs == 2) && (mode == 1)) ) 
        { 
        	ee_para_hw.k1 = ee_para_hw.k2 = ee_para_hw.k3 = 0.0; 
        }
        
        // calc param for sw ee
        if(ee_enable && (ee_algs > 0) && (mode == 1))
        {
        	t0 = now_ms();
        	imxclt_detail_prepare(handle,&ee_para_sw,res_idx,iso_idx);
        	t1 = now_ms();
        	OMXDBUG(OMXDBUG_PARAM, "time consumption of imxclt_detail_prepare: %g\n",t1 - t0);
        }
    }
    else
    {
        nr_para.enable = 0;
        ee_para_hw.k1 = ee_para_hw.k2 = ee_para_hw.k3 = 0.0; ee_enable = 0;
    }

step1:
    if(!nr_para.enable && !ee_enable) goto stepn;
    
    frm_in.format = format;
    frm_in.canvas_w = width + xoffset;
    frm_in.canvas_h = height + yoffset;
    frm_in.xoffset = xoffset;
    frm_in.yoffset = yoffset;
    frm_in.pic_w = width;
    frm_in.pic_h = height;

    frm_out.format = frm_in.format;
    frm_out.canvas_w = frm_in.canvas_w;
    frm_out.canvas_h = frm_in.canvas_h;
    frm_out.xoffset = 0;
    frm_out.yoffset = 0;
    frm_out.pic_w = frm_in.pic_w;
    frm_out.pic_h = frm_in.pic_h;
    
    frm_size = frm_in.canvas_w * frm_in.canvas_h;
    
    frm_in.ptr[0] = img_phy;
    frm_in.ptr[1] = frm_in.ptr[0] + frm_size;
    frm_in.ptr[2] = frm_in.ptr[1] + frm_size * 1/4;

    frm_out.ptr[0] = img_phy;
    if(0 == xoffset && 0 == yoffset) frm_out.ptr[0] = ctl_handle->tmp_buff.phy_addr;
    frm_out.ptr[1] = frm_out.ptr[0] + frm_size;
    frm_out.ptr[2] = frm_out.ptr[1] + frm_size * 1/4;

    // 1) Y
    //t0 = now_ms();
    if(ctl_handle->param_inited)
    {
        nr_para.level = iso_param[iso_idx].nr_level_y;
        nr_para.bl_size = iso_param[iso_idx].nr_bl_size_y;
        //OMXDBUG(OMXDBUG_PARAM, "%s,%d: %d,%d,%d,%d\n", __func__, __LINE__, gain, nr_para.enable, nr_para.level, nr_para.bl_size);
        if(!nr_para.level) nr_para.enable = 0;
        else  nr_para.enable = 1;
    }
    ret = ctl_handle->imx_process(ctl_handle->handle, &frm_in, &frm_out, &nr_para, &ee_para_hw);
    //t1 = now_ms();
    //OMXDBUG(OMXDBUG_PARAM, "time consumption of imx_process: %g\n", t1 - t0);

    // 2) UV
    //t0 = now_ms();
    if(ctl_handle->param_inited)
    {
        nr_para.level = iso_param[iso_idx].nr_level_uv;
        nr_para.bl_size = iso_param[iso_idx].nr_bl_size_uv;
        if(!nr_para.level) nr_para.enable = 0;
        else  nr_para.enable = 1;
        //OMXDBUG(OMXDBUG_PARAM, "%s,%d: %d,%d,%d,%d\n", __func__, __LINE__, gain, nr_para.enable, nr_para.level, nr_para.bl_size);
        if(!nr_para.level) nr_para.enable = 0;
    }
    frm_in.ptr[0] = ctl_handle->tmp_buff.phy_addr;
    frm_out.ptr[0] = ctl_handle->tmp_buff_1.phy_addr;
    ret = ctl_handle->imx_process(ctl_handle->handle, &frm_in, &frm_out, &nr_para, &ee_para_hw);
    //t1 = now_ms();
    //OMXDBUG(OMXDBUG_PARAM, "time consumption of imx_process: %g\n",t1 - t0);
    
    if(ee_enable && (ee_algs > 0) && (mode == 1))
    {
    	t0 = now_ms();
    	imxclt_detail_sw(handle,frm_size);
    	t1 = now_ms();
    	OMXDBUG(OMXDBUG_PARAM, "time consumption of imxclt_detail_sw: %g\n",t1 - t0);
    }

    // 3) copy back
    frm_in.ptr[0] = img_phy;
    frm_out.ptr[0] = ctl_handle->tmp_buff.phy_addr;
    if(frm_in.ptr[0] != frm_out.ptr[0])
    {
        nr_para.enable = 0;
        ee_para_hw.k1 = ee_para_hw.k2 = ee_para_hw.k3 = 0.0;
        ret = ctl_handle->imx_process(ctl_handle->handle, &frm_out, &frm_in, &nr_para, &ee_para_hw);
    }

stepn:
    pthread_mutex_unlock(&ctl_handle->cmd_mutex);

    return ret;
}

void *imxctl_open(void)
{
    int ret = 0;
    imxctl_inter_t *handle_inter = (imxctl_inter_t *)calloc(1, sizeof(imxctl_inter_t));
    if(NULL == handle_inter)
    {
    	OMXDBUG(OMXDBUG_ERR, "handle_inter is NULL!%s,%d\n",__FILE__,__LINE__);
    	return NULL;
    }
    memset(handle_inter,0,sizeof(imxctl_inter_t));

    ret = get_imx_lib(handle_inter);
    if( ret != 0) goto err0;

    OMXDBUG(OMXDBUG_PARAM, "imx open: 0x%x, 0x%x, 0x%x, 0x%x\n", handle_inter, 
        handle_inter->imx_open, handle_inter->imx_close, handle_inter->imx_process);

    handle_inter->handle = handle_inter->imx_open();
    if(NULL == handle_inter->handle) goto err0;

    handle_inter->tmp_buff.phy_addr = malloc_phy(MAX_IMG_SIZE*3/2, &handle_inter->tmp_buff.vir_addr);
    handle_inter->tmp_buff_1.phy_addr = malloc_phy(MAX_IMG_SIZE, &handle_inter->tmp_buff_1.vir_addr);
    if(NULL == handle_inter->tmp_buff.phy_addr || NULL == handle_inter->tmp_buff_1.phy_addr) goto err1;
		
	handle_inter->detail_sw_buff0 = (unsigned char*)enc_malloc(MAX_IMG_SIZE);
	handle_inter->detail_sw_buff1 = (unsigned char*)enc_malloc(MAX_IMG_SIZE);
    if(NULL == handle_inter->detail_sw_buff0 || NULL == handle_inter->detail_sw_buff1) goto err2;

    pthread_mutex_init(&handle_inter->cmd_mutex, NULL);
    return handle_inter;
    
err2:
    if(NULL != handle_inter->detail_sw_buff0)
    {
        enc_free(handle_inter->detail_sw_buff0);
        handle_inter->detail_sw_buff0 = NULL;
    }
    if(NULL != handle_inter->detail_sw_buff1)
    {
        enc_free(handle_inter->detail_sw_buff1);
        handle_inter->detail_sw_buff1 = NULL;
    }
    
err1:
		if(NULL != handle_inter->tmp_buff.phy_addr)
    {
        free_phy(handle_inter->tmp_buff.vir_addr);
        handle_inter->tmp_buff.phy_addr = NULL;
        handle_inter->tmp_buff.vir_addr = NULL;
    }
    if(NULL != handle_inter->tmp_buff_1.phy_addr)
    {
        free_phy(handle_inter->tmp_buff_1.vir_addr);
        handle_inter->tmp_buff_1.phy_addr = NULL;
        handle_inter->tmp_buff_1.vir_addr = NULL;
    }
    handle_inter->imx_close(handle_inter->handle);
    handle_inter->handle = NULL;
    
err0:
    free(handle_inter);
    handle_inter = NULL;
    
    return NULL;
}

int imxctl_close(void *handle)
{
    imxctl_inter_t *handle_inter = (imxctl_inter_t *)handle;

    OMXDBUG(OMXDBUG_PARAM, "imx close: 0x%x\n", handle_inter);

    if(NULL != handle_inter)
    {
        OMXDBUG(OMXDBUG_PARAM, "imx close: 0x%x, 0x%x, 0x%x\n", 
            handle_inter->handle, handle_inter->imx_close, handle_inter->imx_lib);
        
        if(handle_inter->handle)
        {
            handle_inter->imx_close(handle_inter->handle);
        }
        
        if(handle_inter->imx_lib)
        {
            dlclose(handle_inter->imx_lib);
            handle_inter->imx_lib = NULL;
        }

        if(handle_inter->imx_param_lib)
        {
            dlclose(handle_inter->imx_param_lib);
            handle_inter->imx_lib = NULL;
        }

        if(NULL != handle_inter->tmp_buff.phy_addr)
        {
            free_phy(handle_inter->tmp_buff.vir_addr);
            handle_inter->tmp_buff.phy_addr = NULL;
            handle_inter->tmp_buff.vir_addr = NULL;
        }

        if(NULL != handle_inter->tmp_buff_1.phy_addr)
        {
            free_phy(handle_inter->tmp_buff_1.vir_addr);
            handle_inter->tmp_buff_1.phy_addr = NULL;
            handle_inter->tmp_buff_1.vir_addr = NULL;
        }

        if(NULL != handle_inter->detail_sw_buff0)
        {
            enc_free(handle_inter->detail_sw_buff0);
            handle_inter->detail_sw_buff0 = NULL;
        }
        
        if(NULL != handle_inter->detail_sw_buff1)
        {
            enc_free(handle_inter->detail_sw_buff1);
            handle_inter->detail_sw_buff1 = NULL;
        }

        pthread_mutex_destroy(&handle_inter->cmd_mutex);
        
        free(handle_inter);
    }

    return 0;
}


