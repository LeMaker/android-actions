#ifndef __FRAME_MNG_H__
#define __FRAME_MNG_H__

#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#endif
#include "ACT_OMX_IVCommon.h"
#include "ALFace_on.h"
#include "face_engine.h"
#include "enc_param.h"
#include "buffer_mng.h"
#include "Actbuffer.h"
#include "vce_cfg.h"

#define  MMM_FNG_EXIT 0x100

/* wifi-display input is RGB */
typedef struct
{
	unsigned long wfd_phy[3];
	unsigned long wfd_vir[3];
	int wfd_w;
	int wfd_h;
} wfd_intput_t;

#include "queue.h"
#include "tsemaphore.h"
typedef struct
{
	int vce_fd;
	unsigned long video_phyaddr;
	unsigned long video_viraddr;
	unsigned long srcy;
	unsigned long srccb;
	unsigned long srccr;
	int srcstride;
	int srcw;
	int srch;
	int dstw;
	int dsth;
	int issemi;
	int isyuv420;
}resize_input_t;

typedef struct
{
	unsigned long inter_phyaddr;
	unsigned long inter_viraddr;
	unsigned int inter_len;
	int interw;
	int interh;
}resize_output_t;

typedef struct
{
	unsigned long scale_phy;
	unsigned long scale_vir;
	unsigned int scale_flag;
	unsigned int nead_recover;
	unsigned int last_len;
}resize_buffer_t;

enum
{
	SET_FD,
	GET_FD_INFO,
	GET_PREPARE_FRAMES,
	SET_EXIF,
	SET_THUMB,
};

typedef struct act_rect
{
	int x;
	int y;
	int w;
	int h;
} act_rect;

typedef struct act_point
{
	int x;
	int y;
} act_point;

typedef struct act_fd_info
{
	int face_num;
	int img_w;
	int img_h;
	act_rect fd_rect[8];
	act_point fd_LE[8];
	act_point fd_RE[8];
	act_point fd_CM[8];
} act_fd_info;

typedef struct
{
	int b_fD_Enable;
	unsigned int pScaleAddr[2];
	int cur_index;
	int fd_status;
} fd_info_t;

typedef struct
{
	void *vce_handle;
	int frame_rate;
	int maxIinterval;
	int bIDR;
	int i_last_type;
	int i_bframes;
	int bmvc;
	int i_video_fmt;
	int b_semi;
	int bdownscale;
	int i_view_id;
	int i_source_width;
	int i_source_height;
	int i_source_stride;
	int64_t cntFrm;
	int64_t encFrm;
	int bexit;

	queue_t *queue_video;
	queue_t *queue_out;
	enc_frame_t *enc_frame;
	enc_stream_t *enc_strm;
	queue_t *queue_enc;
	queue_t *queue_strm;
	queue_t *queue_prepare_video;
	queue_t *queue_strm_return; /* for android CTS_4_4 */
	tsem_t* pInputSem;
	tsem_t* pOutputSem;

	int bchanged;
	int rect_x;
	int rect_y;
	int rect_w;
	int rect_h;
	int IDR_Refresh;
	int b_store_in_video[2];

	/* face detection */
	void *face_eng;
	int bface_en;
	ALFace_img_t face_img;
	face_input_crop face_crop;
	omx_camera_frame_metadata_t mFaceAppout;
	fd_info_t fd_info;
	act_fd_info fd_out;
	int fd_det;
	int fd_nAngle;
	int fd_isFront;
	int outport_enable;

	/* wifi-display */
	int is_argb8888;
	wfd_intput_t wfd_intput;
	int is_formatconvert;
	unsigned char* formatbuffer;

	/*stride 32-aligned */
	int stride32_flag;
	void *fbuffer_phy;
	unsigned char *fbuffer_vir;

	/* PPS_SPS */
	int get_avc_info_ready;

	/* ring buffer */
	int ringbuf; //only used in outport, ringbuf flag
	int ringbuf_size;
	void * bufferpool;
	
	/* buffer management */
	OMX_VCE_Buffers_List* pInBuffersMng_List;
	OMX_VCE_Buffers_List* pOutBuffersMng_List;

	int b_encoded;
	pthread_mutex_t filter_mutex;
	int ndst_width;
	int ndst_height;
} mng_internal_t;

int mng_open(mng_internal_t *mng_info);
int mng_init(mng_internal_t *mng_info, void *enc_param);
int mng_cmd(mng_internal_t *mng_info, unsigned cmd, void *cmdata);
int mng_deinit(mng_internal_t *mng_info);
int mng_free(mng_internal_t *mng_info);
int mng_try_encode(mng_internal_t *mng_info, OMX_PTR *pBVideo, OMX_PTR *pBStrm);
int mng_get(mng_internal_t *mng_info, OMX_PTR *pBufferVideo, OMX_PTR *pBufferStrm);
int mng_get_avc_info(mng_internal_t *mng_info, OMX_PTR *pBStrm);

#endif
