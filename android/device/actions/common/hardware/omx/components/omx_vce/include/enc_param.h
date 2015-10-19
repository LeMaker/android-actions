#ifndef _ENC_PARAM_H_
#define _ENC_PARAM_H_

#include "enc_types.h"
#include "vce_common.h"
#include "vce_cfg.h"

typedef struct
{
	/* 原始图像的宽高 */
	int bExif;
	int dstWidth;
	int dstHeight;
	unsigned char *thuming;
	unsigned int thumblen;
	unsigned int thumboffset;
	/*
	 1 YUV420SP yyyy/uvuv
	 2 YUV420SP yyyy/vuvu
	 3 yuv420p  yyyy/uu/vv
	 4 yuv420p  yyyy/vv/uu
	 5 yuv422p  yyyy/uu/vv
	 6 yuyvyuyv
	 */
	int srcFormat;
	/* 图像的方向，默认为0 */
	int ImageOri;
	/* 图像EXIF的偏移, 内部使用无需付值 */
	int exifofIFD;
	/* GPS 信息是否含有，-1表示无GPS信息 */
	int exifofGPS;
	/* 拍照日期 */
	char dataTime[128];
	/* make and model*/
	char exifmake[128];
	char exifmodel[128];
	/* 焦距 */
	int focalLengthL;
	int focalLengthH;
	/* 纬度与径度 */
	int gpsLATL[3];
	int gpsLATH[3];
	int gpsLONGL[3];
	int gpsLONGH[3];
	char gpsprocessMethod[128];
	int gpsTimeL[3];
	int gpsTimeH[3];
	char gpsData[128];
	/* 缩略图宽高 */
	int thumbWidth;
	int thumbHeight;
	/*  备份,中间过程不能被修改 */
	unsigned char *bitstreamBuf;
	int bitstreamLen;
	int endian;
	unsigned short bitpS[4];
	unsigned long reved[8];
	int gpsALTIL[1];
	int gpsALTIH[1];
	int gpsLATREF;//N:0 S:1
	int gpsLONGREF;//E:0 W:1
	int gpsALTIREF;//Sea level:0
} JpegExif_t;

#define  Enable_Fix_SCALE 1

typedef enum I_FRAME_MODEL
{
	MOD_ONLY_PPS_SPS = 1,
	MOD_EXECEPT_PPS_SPS,
	MOD_INCLUDE_PPS_SPS,
} AVC_INFO_MODEL;

enum slice_type_e
{
    SLICE_TYPE_P  = 0,
    SLICE_TYPE_B  = 1,
    SLICE_TYPE_I  = 2,
    SLICE_TYPE_SP = 3,//not support
    SLICE_TYPE_SI = 4 //not support
};

typedef struct
{
	int jpg_quality;
	int b_use_new_tbl;
	unsigned int i_pic_width;
	unsigned int i_pic_height;
	char i_pic_fmt; //2 411 5 422
	char b_semi;
#ifdef  IC_TYPE_GL5203
	char b_uv_reversal;
#endif
	char b_exif;
	char b_thumb;
	short i_thumb_w;
	short i_thumb_h;

	JpegExif_t mJpegExif;
	unsigned int rev[2];
} enc_jpeg_param_t;

typedef struct
{
	unsigned int i_pic_width;
	unsigned int i_pic_height;
	unsigned int i_bitrate;
	unsigned int i_framerate;
	int i_profile;//0 baseline,1 main,2 high
	int i_level_idc;
	int i_init_qp;
	unsigned int i_bframes;
	unsigned int b_cabac;
	unsigned int i_pic_fmt; //2 411 5 422
	int b_semi;
#ifdef  IC_TYPE_GL5203
	char b_uv_reversal;
#endif
	int kIntraPeroid;
	int b_mvc;
	int b_wfd_mode;  /*wifi display mode*/
} enc_h264_param_t;

typedef struct
{
	unsigned int b_bld;
	int i_bld_fmt;
	int b_downscale;
	int i_downscale_level;
	int i_ts_en;//0 no,1 mpeg2ts,2 bluts
	int d_width;
	int d_height;
	int b_semi;
	int i_fmt;
#ifdef  IC_TYPE_GL5203
	char b_uv_reversal;
#endif
} enc_prp_param_t;

typedef struct
{
	enc_jpeg_param_t *jpg_param;
	enc_h264_param_t *h264_param;
	enc_prp_param_t *prp_param;
	int enc_mode;
	int enc_codec;
} enc_param_t;

typedef struct
{
	int type;
	int view_id;
	int mdpb;
	int64_t pts;
	int frmtime; /*帧间隔时间us*/

	int64_t frame_cnt;
	short width;
	short height;
	int src_stride;
	void *src_planar_addr[3]; /*phy addr*/
	void *bld_planar_addr[2]; /*phy addr*/
	unsigned char *src_planar[3]; /*vir addr,jpg缩略图使用*/

#ifdef  IC_TYPE_GL5203
	vce_blend_t bld_rect;
#else
	vce_rect_t bld_rect;
#endif

	vce_rect_t fine_rect;
	int fine_rect_en;

	/*下采样用*/
	void *down_planar_addr;
	int i_ds_lv;

	int b_ds;
	int b_bld;
	int i_alpha;
	int i_bld_fmt;
#if Enable_Fix_SCALE
	int i_semi;
#endif

	/*PPS_SPS mode*/
	int avc_info_mode;
} enc_frame_t;

typedef struct
{
	unsigned char *stream_buf;    /*vir addr*/
	unsigned long phy_stream_buf; /*phy addr*/
	unsigned int i_offset;
	unsigned int i_len;
	unsigned int i_offset_next;
	unsigned int i_len_next;
	unsigned int i_flag;
} enc_stream_t;

enum
{
	/* Get The Fd of VceDrv */
	ENC_GET_DRV_FD,
};

#endif
