#ifndef  __VCE_RESIZE_H__
#define  __VCE_RESIZE_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum VR_ALIGN
{
	ALIGN_16PIXELS = 0,
	ALIGN_32PIXELS,
} VR_ALIGN;

typedef struct VR_Input_t
{
	int dstw_align; /* output buffer align for width */
	int reserve[7];
} VR_Input_t;

typedef struct VR_Parm_t
{
	unsigned long src_addr;
	int srcstride;
	int srcheight;
	int cropw;
	int croph;
	unsigned long dst_addr;
	int wscale;
	int hscale;
	int reserve[8];
} VR_Parm_t;

typedef enum VR_CMD
{
	VR_CROP,
} VR_CMD;

typedef enum VR_FORMAT
{
	FORMAT_YUV420SP = 0,
	FORMAT_YVU420SP,
	FORMAT_YUV420P,
	FORMAT_YVU420P,
} VR_FORMAT;

typedef struct VR_Cmd_t
{
	unsigned long src_addr;/*must be phy*/
	int srcformat; /*VR_FORMAT*/
	int srcstride;
	int srcheight;
	int cropx;
	int cropy;
	int cropw;
	int croph;
	unsigned long dst_addr;/*phy or vir*/
	int bvir_addr; /*phy:0 , vir:1*/
	int dstformat; /*VR_FORMAT*/
	int dstw; /*stride = width*/
	int dsth;
	int reserve[8];
} VR_Cmd_t;

void *VceReSize_Open(void *input/*VR_Input_t*/);
int VceReSize_Run(void *handle, void *param/*VR_Parm_t*/); /*for VideoDecoder*/
int VceReSize_Cmd(void *handle, int cmd, void *param/*VR_Cmd_t*/); /*for CameraHal*/
void VceReSize_Close(void *handle);

#ifdef __cplusplus
}
#endif

#endif
