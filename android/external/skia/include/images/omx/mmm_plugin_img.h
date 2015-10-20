/********************************************************************************
*                              usdk1100
*                            Module: act_decoder.c
*                 Copyright(c) 2003-2008 Actions Semiconductor,
*                            All Rights Reserved.
*
* History:
*      <author>    <time>           <version >             <desc>
*       jszeng    2009-01-05 10:00     1.0             build this file
********************************************************************************/
/*!
* \file     mmm_plugin_dev.h
* \brief    定义与应用层的接口及参数
* \author   丁剑
* \version 1.0
* \date  2010/08/31
*******************************************************************************/
#ifndef __MMM_PLUGIN_DEV_H__
#define	__MMM_PLUGIN_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef     NULL
#define		NULL		0
#endif

/*!
 * \brief
 *      定义了不同的插件类型常量，这些常量用于插件信息的type字段
 */
typedef enum
{
    /*! plugin for demuxer */
    MMM_PLUGIN_DEMUX = 0x01,
    /*! plugin for audio decoder */
    MMM_PLUGIN_AUDIO_DECODER,
    /*! plugin for video decoder */
    MMM_PLUGIN_VIDEO_DECODER,
    /*! plugin for img decoder */
    MMM_PLUGIN_IMAGE_DECODER,
    /*! plugin for muxer */
    MMM_PLUGIN_MUX,
    /*! plugin for audio encoder */
    MMM_PLUGIN_AUDIO_ENCODER,
    /*! plugin for video encoder */
    MMM_PLUGIN_VIDEO_ENCODER,
    /*! plugin for img encoder */
    MMM_PLUGIN_IMAGE_ENCODER,
    /*! plugin for image dec ipp */
    MMM_PLUGIN_IPP
}mmm_plugin_type_t;

/*!
 * \brief
 *      get file information
 */
typedef struct
{
    /*! 后缀*/
	char *extension;
	/*! 文件长度*/
	unsigned int file_len;
}mmm_file_info_t;
/*!
 * \brief
 *      stream_input seek的方式
 */

#if 1
typedef enum
{
    /*! 从文件头开始定位*/
    MMM_DSEEK_SET=0x0,
    /*! 从文件当前位置开始定位*/
    MMM_DSEEK_CUR,
    /*! 从文件尾往回定位*/
    MMM_DSEEK_END
}mmm_stream_seek_t;
#else
typedef enum
{
    /*! 从文件头开始定位*/
    MMM_DSEEK_SET=0x01,
    /*! 从文件尾往回定位*/
    MMM_DSEEK_END,
    /*! 从文件当前位置开始定位*/
    MMM_DSEEK_CUR
}mmm_stream_seek_t;
#endif

/*!
 * \brief
 *      数据操作函数结构
 */
typedef struct mmm_stream_input_s
{
	void *io_ptr;
    /*! 读数据*/
	int (*read)(struct mmm_stream_input_s *input,unsigned char* buf,unsigned int len);
	/*! 写数据*/
	int (*write)(struct mmm_stream_input_s *input,unsigned char* buf,unsigned int len);
	/*! 文件指针定位*/
	int (*seek)(struct mmm_stream_input_s *input,int offset,int original);
	/*! 获取当前文件位置*/
	int (*tell)(struct mmm_stream_input_s *input);
	/*! 获取文件信息*/
	int (*get_file_info)(struct mmm_stream_input_s *input,mmm_file_info_t *info);
	/*! 释放stream_input_t操作句柄*/
	int (*dispose)(struct mmm_stream_input_s *input);
	/*! 获取数据的大小*/
	int (*get_data_size)(struct mmm_stream_input_s *input);
}mmm_stream_input_t;

/*!
 * \brief
 *  在执行image decoder插件的open函数时传入
 */
typedef struct
{
    /*! 数据操作句柄*/
	mmm_stream_input_t *input;
}mmm_imagedec_plugio_t;

/*!
 * \brief
 *  该结构记录了不同插件的信息，中间件在完成插件加载时使用
 */
typedef struct
{
    /*! 文件类型*/
	char type;
	/*! 文件后缀*/
	char *extension;
	/*! open插件时需要告诉当前插件的输入输出*/
	void *(*open)(void *plugio);
	/*! 获取文件信息*/
	int (*get_file_info)(mmm_stream_input_t *input,void *file_info);
}mmm_plugin_info_t;

/*!
 * \brief
 *  图片解码输出格式
 */
typedef enum
{
    /*! RGB*/
    MMM_FMT_RGB=0x00,
    /*! ARGB*/
    MMM_FMT_ARGB,
    /*! YUV*/
    MMM_FMT_YUV
}mmm_img_dec_type_t;
/*!
 * \brief
 *  图片解码输出颜色空间
 */
typedef enum
{
	/*! RGB565*/
	PPRGB565 = 0,
	/*! RGB888*/
	PPRGB888,
	/*! ARGB565*/
	PPARGB5565,
	/*! ARGB888*/
	PPARGB8888,
	/*! YUV400*/
	PPYUV_400,
	/*! YUV420planar*/
	PPYUV_420,
	/*! YUV422planar大头贴功能新增*/
	PPYUV_422,
	/*! YUV 420planar with alpha*/
	PPYUVA_420,
	/*! YUV422planar with alpha大头贴功能新增*/
	PPYUVA_422,
	/*! YUV420semiplanar*/        
	PPYUV_SEMI_420,
	/*! YUV422semiplanar*/ 
	PPYUV_SEMI_422,
	/*! YUYV*/
	PPYCBYCR,
}mmm_img_color_t;

/*!
 * \brief
 *  图片解码输出数据排列方式
 */
typedef enum
{
    /*! 确省模式*/
    MMM_DSP_DIR_DEFAULT=0x00,
    /*! 横屏模式*/
    MMM_DSP_DIR_SIDELONG_FLIP,
    /*! 竖屏模式*/
    MMM_DSP_DIR_UPRIGHT_FLIP,
    /*! 自适应屏方式*/
    MMM_DSP_DIR_BOTH_FLIP
}mmm_img_data_array_type_t;
/*!
 * \brief
 *  图片日期
 */
typedef struct
{
    /*! 年*/
	unsigned int year;
	/*! 月*/
	unsigned int month;
	/*! 日*/
	unsigned int day;
}mmm_image_date_t;
/*!
 * \brief
 *  图片文件信息
 */

typedef struct
{
	/*! 图片解码输出颜色空间*/
	mmm_img_color_t color_space;
	/*! 宽*/
	unsigned int width;
	/*! 高*/
	unsigned int height;
	/*! 日期*/
	mmm_image_date_t date;
	/*! 附加信息*/
	unsigned char exif[2*1024];
	/*! JPG缩略图在exif信息中的位置*/
	unsigned int exif_pos;
}mmm_image_file_info_t;

/*!
 * \brief
 *      YUV信息
 */
typedef struct 
{
	/*! Y地址*/
	unsigned char *y_buf;
	/*! Y地址长度*/
	unsigned int y_buf_len;
	/*! U地址*/
	unsigned char *u_buf;
	/*! U地址长度*/
	unsigned int u_buf_len;	
	/*! V地址*/
	unsigned char *v_buf;
	/*! V地址长度*/
	unsigned int v_buf_len;	
	/*! alpha地址*/
	unsigned char *alpha_buf;
	/*! alpha地址长度*/
	unsigned int alpha_buf_len;			
	/*! Y数据stride*/
	unsigned int  y_stride;
	/*! U数据stride*/
	unsigned int  u_stride;
	/*! V数据stride*/
	unsigned int  v_stride;
	/*! alpha数据stride*/
	unsigned int  alpha_stride;
}mmm_img_yuv_info_t; 

/*!
 * \brief
 *      RGB信息
 */
typedef struct 
{
	/*! rgb地址*/
	unsigned char *rgb_buf;
	/*! rgb地址长度*/
	unsigned int  rgb_buf_len;
	/*! rgbstride*/		
	unsigned int  rgb_stride;	
}mmm_img_rgb_info_t;

/*!
 * \brief
 *      YCBYCR信息
 */
typedef struct 
{
	/*! YCBYCR地址*/
	unsigned char *ycbycr_buf;
	/*! YCBYCR地址长度*/
	unsigned int  ycbycr_buf_len;
	/*! YCBYCR*/		
	unsigned int  ycbycr_stride;	
}mmm_img_ycbycr_info_t;

/*!
 * \brief
 *  传给图片解码库要求其输出的图片规格及其输出返回
 */
typedef struct
{
    /*! 输出颜色*/
	mmm_img_dec_type_t format;
	/*! 是否运用增强*/
	unsigned int doEnhance;			
	/*! 实际图片的宽*/
	unsigned int width;
	/*! 实际图片的高*/
	unsigned int height;
	/*! 输出的x起始位置*/
	int  xpos;
	/*! 输出的y起始位置*/
	int  ypos;
	unsigned long phyaddr;	
	unsigned char *viraddr;
    union {
    	/*! RGB信息*/
        mmm_img_rgb_info_t rgb_info;
        /*! YUV信息*/
        mmm_img_yuv_info_t yuv_info;
	/*! YCBYCR信息*/
	mmm_img_ycbycr_info_t ycbycr_info;
    };
	/*! 时间戳*/
	unsigned int time_stamp;
	/*! JPG缩略图在exif信息中的位置*/
	unsigned int exif_pos;	
}mmm_image_info_t;

/*!
 * \brief
 *  alpha图片解码参数设置
 */
typedef struct
{
    char *alpha_buffer;
    unsigned int alpha_buf_len;
}imgdec_alpha_param_t;
/*!
 * \brief
 *  图片解码扩展接口
 */
typedef enum
{
    MMM_SET_ALPHA_PARAM=0x00
}imgdec_ex_ops_t;
/*!
 * \brief
 *  图片解码插件
 */
typedef struct mmm_image_plugin_s
{
    /*! 初始化*/
	int (*init)(struct mmm_image_plugin_s *plugin);
	/*! 解码*/
	int (*decode_img)(struct mmm_image_plugin_s *plugin,mmm_image_info_t *img);
	/*! 扩展接口*/
	int (*ex_ops)(struct mmm_image_plugin_s *plugin,int cmd,unsigned int arg);
	/*! 释放插件*/
	int (*dispose)(struct mmm_image_plugin_s *plugin);
}mmm_image_plugin_t;

/*!
 * \brief
 *  图片移动方向
 */
typedef enum
{
    /*! 上移*/
    MMM_MOVE_UP=0x01,
    /*! 下移*/
    MMM_MOVE_DOWN,
    /*! 左移*/
    MMM_MOVE_LEFT,
    /*! 右移*/
    MMM_MOVE_RIGHT
}mmm_img_move_type_t;
/*!
 * \brief
 *  图片旋转方向
 */
typedef enum
{
    /*! 左转90度*/
    MMM_ROTATE_LEFT90=0x01,
    /*! 右转90度*/
    MMM_ROTATE_RIGHT90,
    /*! 右转180度*/
    MMM_ROTATE_180
}mmm_img_rotate_type_t;
/*!
 * \brief
 *  图片缩放参数
 */
typedef enum
{
    /*! 放大*/
    MMM_ZOOM_IN=0x01,
    /*! 缩小*/
    MMM_ZOOM_OUT
}mmm_img_zoom_type_t;
/*!
 * \brief
 *  图片对称变换方式
 */
typedef enum
{
    /*! 水平对称变换*/
    MMM_MIRROR_HORIZONTAL=0x01,
    /*! 上下对称变换*/
    MMM_MIRROR_VERTICAL
}mmm_img_mirror_type_t;
/*!
 * \brief
 *  图片解码ipp插件提供的函数
 */
typedef struct mmm_ipp_proc_s
{
    /*! 初始化ipp*/
	int (*init)(struct mmm_ipp_proc_s *handle);
	/*! 解码时的后处理变换*/
	int (*ipp_convert)(struct mmm_ipp_proc_s *handle,mmm_image_info_t *img_in,mmm_image_info_t *img_out);//ipp后处理(从大图buf转至小图buf)，传递目标buf
	/*! 移动*/
	int (*move)(struct mmm_ipp_proc_s *handle,unsigned int dir);
	/*! 旋转*/
	int (*rotate)(struct mmm_ipp_proc_s *handle,unsigned int dir);
	/*! 缩放*/
	int (*zoom)(struct mmm_ipp_proc_s *handle,unsigned int size);
	/*! 对称变换*/
	int (*mirror)(struct mmm_ipp_proc_s *handle,unsigned int dir);
	/*! 调整亮度*/
	int (*adjust_light)(struct mmm_ipp_proc_s *handle,unsigned int level);
	/*! 重新定位大小*/
	int (*resize)(struct mmm_ipp_proc_s *handle,mmm_image_info_t *img_out);
	/*! 释放插件*/
	int (*dispose)(struct mmm_ipp_proc_s *handle);
}mmm_ipp_proc_t;
mmm_ipp_proc_t *mmm_ipp_open(void);

typedef struct 
{
	unsigned char *buf_start;
	unsigned int  buf_len;
	unsigned int  cur_len;
}buf_stream_info_t;

typedef struct {
	mmm_stream_input_t input;
	buf_stream_info_t *buf_info;
	mmm_image_plugin_t *p_imgInterface;
}actImage_caller_t;


#ifdef __cplusplus
}
#endif

#endif

