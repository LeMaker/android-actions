#ifndef ANDROID_DEFAULT_CODE
#include "SkImageDecoder_Act.h"
#include "Img_En.h"

static int fDoEnhance;
static int fSdramCapacity;
    /**
     *  actions_code(lishiyuan, NEW_FEATURE:Skia decode pictures by 
     *  native standard or actions standard, depend on 
     *  ro.skia.img.decode.standard)
     *
     *  fDecodeStandard 0: ACTIONS, 1: NATIVE
     */
static int fDecodeStandard; 
static int fHWMaxWidth;
static int fHWMaxHeight;	

  
int  ACT_getDoEnhance() { 
    return fDoEnhance; 
}
void ACT_setDoEnhance(int doEnhance){
    fDoEnhance = doEnhance;
}

void image_scale(scale_param_t *dst, scale_param_t *src)
{
	unsigned int hs,hd;
	unsigned int ws,wd;
	unsigned int src_stride;
	unsigned int i,j;
	unsigned int h_sample;
	unsigned int v_sample;
	unsigned int t1,t2,tx,ty;
	u_scale *pSrc;
	u_scale *pDest;	
	
	ws = src->width;
	hs = src->height;
	wd = dst->width;  
	hd = dst->height;
	src_stride = src->stride;
	pDest = (u_scale *)dst->addr;
	
	h_sample = (1<<SHIFTVAL)*hs/hd;
	v_sample = (1<<SHIFTVAL)*ws/wd;
	t1 = 0;	
    for(i = 0; i < hd; i++)
	{
    	ty = t1 >> SHIFTVAL;
    	t1 += h_sample;
    	pSrc = (u_scale *)(src->addr + ty*src_stride);
    	t2 = 0;
    	for (j = 0; j < wd; j++)
    	{    		
    		tx = t2 >> SHIFTVAL;
    		t2 += v_sample;		
    		*pDest++ = *(pSrc + tx);    		
        }
    }
    return;
}

void image_scale_rgb8888(scale_param_t *dst, scale_param_t *src)
{
	unsigned int hs,hd;
	unsigned int ws,wd;
	unsigned int src_stride;
	unsigned int i,j;
	unsigned int h_sample;
	unsigned int v_sample;
	unsigned int t1,t2,tx,ty;
	unsigned int *pSrc;
	unsigned int *pDest;	
	
	ws = src->width;
	hs = src->height;
	wd = dst->width;  
	hd = dst->height;
	src_stride = src->stride;
	pDest = (unsigned int *)dst->addr;
	
	h_sample = (1<<SHIFTVAL)*hs/hd;
	v_sample = (1<<SHIFTVAL)*ws/wd;
	t1 = 0;	
    for(i = 0; i < hd; i++)
	{
    	ty = t1 >> SHIFTVAL;
    	t1 += h_sample;
    	pSrc = (unsigned int *)(src->addr + ty*src_stride);
    	t2 = 0;
    	for (j = 0; j < wd; j++)
    	{    		
    		tx = t2 >> SHIFTVAL;
    		t2 += v_sample;		
    		*pDest++ = *(pSrc + tx);    		
        }
    }
    return;
}

int image_enhance_func(int w, int h, SkColorType config, unsigned char *src)
{
    int rt = 0;
    void *en_handle;        
    En_Parm_t en_private;          
    En_Input_t en_input;
    
    if(w*h < 4096)//64*64
    {
       //SkDebugf("don't do enhance for small image...");
       return 0;
    }
    if(config == kN32_SkColorType)
       en_input.data_format = EN_FORMAT_ABGR8888;
    else
       en_input.data_format = EN_FORMAT_RGB565;
       
    en_input.media_format = EN_MEDIA_IMAGE;
    en_input.width = w;
    en_input.height = h;
    
    en_handle = ImgEn_Open(&en_input); 
    if(en_handle == NULL)
    {
        return -1;
    }                       
    en_private.src = src;
    en_private.dst = src; 
            
    rt = ImgEn_Run(en_handle,&en_private);    
    ImgEn_Close(en_handle); 
    return rt;               
}

int ACT_getSdramCapacity() {return fSdramCapacity;}
int ACT_getDecodeStandard() {return fDecodeStandard;}
int ACT_getHWCodecMaxWidth() {return fHWMaxWidth;}
int ACT_getHWCodecMaxHeight() {return fHWMaxHeight;}

void ACT_setSdramCapacity(int sdramcap){
    fSdramCapacity = sdramcap;
}
void ACT_setDecodeStandard(int decodeStandard)  {
    fDecodeStandard = decodeStandard;
}
void ACT_setHWCodecMaxWidth(int HWMaxWidth) {
    fHWMaxWidth = HWMaxWidth;
}
void ACT_setHWCodecMaxHeight(int HWMaxHeight){
    fHWMaxHeight = HWMaxHeight;
}
#endif /*ANDROID_DEFAULT_CODE*/	

