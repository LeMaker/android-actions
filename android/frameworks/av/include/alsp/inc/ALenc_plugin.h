/*******************************************************************************
 *                              5003
 *                            Module: AVE recorder
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       noahkong    2009-06-08 15:00     1.0             build this file 
*******************************************************************************/
#ifndef __ALENC_PLUGIN_H__
#define __ALENC_PLUGIN_H__

//#ifdef _OS_UX_
#include "./common/al_libc.h"
#include "./common/stream_input.h"
//#endif

/*disk manager seek*/
#define		DSEEK_SET		0x01
#define		DSEEK_END		0x02
#define		DSEEK_CUR		0x03

#ifndef NULL
#define		NULL		        0
#endif

/* used by muxer packet flag */
#define		IS_AUDIO				0x01			//contains only audio
#define		IS_VIDEO				0x02			//contains only video	
#define		IS_AV					  0x03			//contains audio and video

/* 定义了不同的插件类型常量，这些常量用于插件信息的type字段 */
#define 	PLUGIN_MUX		        0x05			//plugin for muxer
#define		PLUGIN_VIDEO_ENCODER	0x07			//plugin for video encoder
#define 	PLUGIN_IMAGE_ENCODER	0x08			//plugin for img encoder
//#define (_SIG_TEST_)||(_SDK_227D_)
//#ifdef _SIG_TEST_
//  #include "./common/al_libc.h"
//  #include "./common/stream_input.h"
//#else
//  
//  #ifdef _USDK_227D_
//    #include "./common/al_libc.h"
//    #include "./common/stream_input.h"    
//  #else
//    #ifdef _OS_UC_
//      typedef struct stream_input_s
//      {
//              int(*read)(struct stream_input_s *manager, char *buf, unsigned int len);
//              int(*write)(struct stream_input_s *manager, char *buf, unsigned int len);
//              int(*seek)(struct stream_input_s *manager, long long offset, int original);
//              long long(*tell)(struct stream_input_s *manager);
//      }stream_input_t;
//    #endif
//  #endif
  
//#endif

/*!
 * \brief
 *      图像数据格式
 */         
typedef struct{		
	unsigned int formate;
	unsigned int bpp;		
	unsigned int width;
	unsigned int height;		
	unsigned char *buf;
	unsigned int len;
	/*! rgb565背景图 + alpha 图，总bk_width*bk_height*2 + bk_width*bk_height*/
	unsigned int bk_buf;
	/*背景图宽*/ 
	unsigned int bk_width;
	/*背景图高*/
	unsigned int bk_height;
}image_info_t;

/*!
 * \brief
 *      拍照初始化信息
 */
typedef struct
{
         /*! 标志照片是否打上日期 */  
         unsigned int photo_data_flag;
         /*! 日期信息(点阵) */
         unsigned char *photo_data; 
         /*! 日期信息(点阵)宽 */
         unsigned int  pd_width;
         /*! 日期信息(点阵)高 */
         unsigned int  pd_height;
         /*! exif设置信息 */
         unsigned char *exif_info; 
         /*! 是否缩放,0: normal; 1: scaler */
         unsigned int   scale_mode;
         /*! 是否启动大头帖, 0: 无; 1: 启动 */
         unsigned int   func_mode;
         /*! 图片编码后的宽 */
         unsigned int dst_width; 
         /*! 图片编码后的高 */
         unsigned int dst_height;
         /*! 数码变焦参数的分子 */
         unsigned int fr; 
         /*! 数码变焦参数的分母 */
         unsigned int br; 
}camera_init_t;


#define SET_FACE_MASK_ON 0xb100
#if 0
/*!
 * \brief
 *      拍照时，所需人脸信息进行特效组合
 */
typedef struct {  
        /*! 人脸特征信息 */
        ALFace_appout_t *face_app;
        /*! 拍照大图指针 */
        image_info_t img;
        /*! 备份信息 */
        unsigned int bk_info;
}ALFace_info_t;
#endif
/*!
 * \brief
 *      拍照接口
 */
typedef struct camera_plugin_s{
	int (*init)(void *plugin, camera_init_t *param);
	int (*encode_img)(void *plugin, image_info_t *img);
	int (*dispose)(void *plugin);
//#ifdef _USDK_227D_
	int (*exop)(void *plugin,int cmd,unsigned int args);
//#endif
}camera_plugin_t;                                                         


/*!
 * \brief
 *      显示使能接口（无使用）
 */ 
typedef  struct{
        /* encoder显示功能是否使能 1：使能；0：不使能 */
        int    enable_display;     
        int    width;
        int    height;
} display_resolution_t;


/*!
 * \brief
 *      音频编码器初始化参数
 */ 
typedef struct{
        /* fmt of audio output */
        char *audio_fmt;		         
        /* 当前音频数据的分辨率 */
        unsigned int bpp;
        /* 编码后的比特率 */		         
        unsigned int bitrate;	
        /* 采样率 */
        unsigned int sample_rate;	   	
        /* 当前的通道数 */
        unsigned int channels;		   
        /* 编码模式立体声、联合立体声等等 */
        unsigned int encode_mode;	   
}ae_param_t;

/*!
 * \brief
 *      音频编码器初始化参数
 */ 
typedef struct{
        /* 编码后的宽 */
        int width;
        /* 编码后的高 */
        int height;
        /* 数据源的宽 */
        int src_width;  
        /* 数据源的高 */              
        int src_height;    
        /* 默认1 */            
        int fincr;  
        /* 与framerate等同 */                   
        int fbase; 
        /* 帧率 */                    
        int framerate;	
        /* I帧间距，（强制） xvid：3，mjpg：1 */	             
        int max_key_interval;	
        /* 初始量化比 */         
        int quanty;
        /* frame dropping: 0=drop none... 100=drop all */ 		                 
        int frame_drop_ratio;  
        /* B祯的个数,（强制）：0 */       
        int max_bframes;
        /* 暂时无效，0 */             
        int video_bitrate;
        /* 暂时无效，0 */             
        int bQOffset;	
        /* 暂时无效，0 */                 
        int bQRatio;	
        /* 存储外设 */	               
        int cardtype;
        /* reserved[2] is used for backup vidfmt*/
        int reserved[9];
}ve_param_t;

/*!
 * \brief
 *      mux初始化参数
 */ 
typedef struct{
        /* 文件格式 */
        char file_fmt[12];
        /* 流类型 */
        unsigned int streamer_type;	
        /* 音频参数 */	
        ae_param_t audio_param;
        /* 视频参数 */
        ve_param_t video_param;
        /* 音频格式　*/
        char audio_fmt[12];
        /* 视频格式　*/
        char video_fmt[12];
        /* no use　*/
        unsigned char *tbl_buf; 		
        unsigned int tbl_buf_len;
}ave_param_t;

/*!
 * \brief
 *      编码数据格式
 */ 
typedef struct{
	unsigned char *data;
	int  data_len;
	/* 音频数据时为 0；视频 0 - 非关键祯，1 - 关键祯 */
	int  is_key_frame; 
	/* 时间戳 */	
	int  encode_type;	
}encode_frame_t;

/*!
 * \brief
 *      打包数据格式
 */ 
typedef struct{
        /*chunk类型：使用常量定义IS_AUDIO//IS_VIDEO// */
	int media_type; 	
	char *data;
	int data_len;	
	int is_key_frame;
	/* 时间戳 */
	int  time_stamp;	
	int reserved[9];
}av_chunk_t;


typedef struct {
  ave_param_t *ave_param;
  stream_input_t *input;
	unsigned int 	 reserved[2];
}mux_input_attr_t;	
					
/* mux当前需要audio packet还是video packet */
#define GET_PACKET_TYPE     0xc001    
/* 获取mux状态信息 */                                  
#define GET_MUXER_STATUS    0xc002   
/* 重新初始化 */
#define RESET_MUXER         0xc003


/*!
 * \brief
 *      mux当前的编码信息
 */
typedef  enum{
    EITHER_OK,
    AUDIO_PACKET,
    VIDEO_PACKET	
}packet_type_t;

/*!
 * \brief
 *      mux当前的状态信息
 */
typedef struct {
        /* including audio and video frames or other */
	unsigned int     total_frames;		 
	unsigned int     video_frames;
	unsigned int     audio_frames;
	/* 文件位置 低32位 */
	unsigned int     movi_length_L;	
	/* 文件位置 高32位 */     
	unsigned int     movi_length_H;		 
	unsigned int 	 reserved[5];	
}mux_output_attr_t;						         

/*!
 * \brief
 *      编码输出数据
 */
typedef struct{
	char *data;
	int data_len;
	//音频数据时为 0；视频 0 - 非关键祯，1 - 关键祯
	int is_key_frame;
	//时间戳 	
	int  encode_type;	
}av_frame_t;

/* 获取encoder属性信息，包括是否具有动静监测功能 */
#define GET_ENCODER_ATTR     0xb001  
/* 获取encoder状态信息，当前帧是否运动 */
#define GET_ENCODER_STATUS   0xb002  
/* display_resolution_t结构指针，显示分辨率 */
#define SET_DISPLAY_PARAM    0xb005    

/*!
 * \brief
 *      编码器当前状态
 */
typedef struct {
        /* current(accurately last) frame encode type I/P/B/SKip */
	unsigned int frame_type;
	/* current(accurately last) frame encode length (bytes)	*/    
	unsigned int frame_size;
	/* current(accurately last) quality */	    
	unsigned int cur_quality;	
	/* current(accurately last) I maxinterval */   
	unsigned int cur_interval;
	/* 当前检测得到的状态 */	  
	unsigned int cur_suv_status; 
	/* 输出的display buffer */
	unsigned int display_buf; 
	/* 输出的中间参数 */    
	unsigned int inter_param;   
	/* display buffer中图像的宽 */  
	unsigned short out_width;   
	/* display buffer中图像的高 */ 
	unsigned short out_height;  
	/* display buffer中图像格式; 1: 4Y4U4Y4V, 2:YUV_4_2_0_PLANAR */  
	unsigned short out_format;    
}videoenc_output_attr_t;		    

/* 设置动静监测参数 */
#define  SET_MOTION_PARAM      0xb003   
/* 设置数码变焦参数 */
#define  SET_ZOOM_PARAM        0xb004   
/* 设置数码变焦窗口 */ 
#define  SET_SOURCE_WINDOWN    0xb008
/* 设置IP间隔 */
#define  SET_FRM_INTVAL        0xb009

/*!
 * \brief
 *      动静监测参数
 */
typedef struct {
        /* current(accurately last) quality */
	unsigned int cur_quality;	
	/* current(accurately last) I maxinterval */     
	unsigned int cur_interval;
	/* 检测模式, 动静监测是否使能 */	   
	int          sur_field_mode; 
	/* default:3 */  
	unsigned short md_throld0;
	/* default:2 */ 
	unsigned short md_throld1;
	/* default:2 */
	unsigned short md_throld2;
	/* default:2 */   
	unsigned short md_throld3;
	unsigned int   reserved[3];
}videoenc_input_attr_t;			   

/*!
 * \brief
 *      数码变焦参数
 */
typedef  struct{
        /* 数码变焦功能是否使能 1：使能；0：不使能 */
        int    enable_zoom ;      
        int    fr;
        int    br;
} encoder_zoom_t;

/*!
 * \brief
 *      输入图像窗口参数
 */
typedef  struct{
        int    width;
        int    height;
} encoder_srcwin_t;

/*!
 * \brief
 *      插件结构
 */
typedef struct{
        char type;
        char *extension;
        void *(*open)(void *plugio);		/*open插件时需要告诉当前插件的输入输出*/
        int (*get_file_info)(stream_input_t *input,void *file_info);
}plugin_info_t;

/*!
 * \brief
 *      视频编码器插件
 */
typedef struct video_encoder_s{
        int (*init)(struct video_encoder_s *plugin,ve_param_t *param);
        int (*encode_data)(struct video_encoder_s *plugin,av_frame_t *src_frame,av_frame_t *dest_frame);
        int (*set_attribute)(struct video_encoder_s *plugin,int attrib_id,void *param);
        int (*get_attribute)(struct video_encoder_s *plugin,int attrib_id,void *param);
        int (*get_err)(struct video_encoder_s *plugin);
        int (*dispose)(struct video_encoder_s *plugin);
}video_encoder_t;

/*!
 * \brief
 *      音视频合流器插件
 */
typedef struct mux_plugin_s{
        int (*init)(struct mux_plugin_s *plugin,ave_param_t *param);
        int (*write_header)(struct mux_plugin_s *plugin);
        int (*write_chunk)(struct mux_plugin_s *plugin,av_chunk_t *chunk);
        int (*set_attribute)(struct mux_plugin_s *plugin,int attrib_id,void *param);
        int (*get_attribute)(struct mux_plugin_s *plugin,int attrib_id,void *param);
        int (*get_error)(struct mux_plugin_s *plugin);
        int (*dispose)(struct mux_plugin_s *plugin);
}mux_plugin_t;

#endif
