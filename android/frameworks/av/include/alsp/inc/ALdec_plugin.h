#ifndef ALDEC_PLUGIN_H
#define ALDEC_PLUGIN_H

#ifdef WIN32
#include "al_libc_dll.h"
#endif
#include "./common/stream_input.h"
#include "./common/extdef.h"
#include "./common/buf_header.h"

#define _ISPRAM_CODE_
#define _ISPRAM_SECTION_NAME_

#ifdef __cplusplus
extern "C" {
#endif

/*图像格式类型*/
#define YUV420_SEMI_PLANAR         0x00
#define YUV420_PLANAR              0x01

/*最大audio和video包长度*/
#define AUDIO_MAX_PKT_SIZE        204800
#define VIDEO_MAX_PKT_SIZE        2048000

/* 定义插件返回错误号*/
typedef enum
{
	PLUGIN_RETURN_ERR = -1,
	PLUGIN_RETURN_NORMAL = 0,
	PLUGIN_RETURN_FILE_END,
	PLUGIN_RETURN_NOT_SUPPORT,
	PLUGIN_RETURN_PKT_NOTEND,
	PLUGIN_RETURN_SEEK_ERR
}plugin_err_no_t;


/*parser和decoder之间的传递包的数据结构*/
typedef struct
{
	unsigned char *data;
	unsigned int data_len;
}av_buf_t;

/*中间件发给插件ex_ops接口的命令字（后续会有拓展）*/
typedef enum
{
	SET_SUBPIC,
	SET_TRACK,
	NORMAL_PLAY,
	DISCARD_FRAMES,
	NOTIFY_REF_FRAME_NUM,
	NOTIFY_FIFO_RESET,
	WAIT_FOR_HANTRO,
	SET_FOURCC,
	FLUSH_BUFFERS_REMAINING,
	SET_SEEK_OPTIONS=0x100,//0:CLOSEST 1:FORWARD 2:BACKWARD
	NOTIFY_MEDIA_TYPE, //0:Local 1:Streaming
	RESET_PARSER,      //for loop play when file can not seek
	EX_RESERVED1,         //reserved
	EX_RESERVED2
}plugin_ex_ops_cmd_t;

typedef struct{
	unsigned int a_pos;	
	unsigned int v_pos;	
	int a_pts;
	int v_pts;
	int cur_time;
	int subpic_num;
}subpic_param_t;

typedef struct{
	unsigned int a_pos;	
	unsigned int v_pos;	
	int a_pts;
	int v_pts;
	int cur_time;
	int track_num;	
	int out_basetime;//返回值
}track_param_t;

/*parser_format可选值*/
typedef enum
{
	AVI = 0,
	MP4,
	MKV
} fileFormat_t;

//中间件切音轨/字幕时传给parser的数据结构
typedef struct{
	unsigned int audio_offset;
	unsigned int audio_ts;
	unsigned int video_offset;
	unsigned int video_ts;
	unsigned int subpic_offset;
	unsigned int subpic_ts;
}switch_audio_subpic_t;

typedef enum
{
	IS_AUDIO = 1,
	IS_VIDEO,
	IS_AV
}media_type_t;

typedef struct
{
    char extension[8];      /* 解码库的后缀 */	
    unsigned int sample_rate;		/* 采样率，单位hz */
    unsigned int channels;			/* 声道数 */
    void *buf;  
    void *private_data;            /* 指向特定的音频格式数据结构 */
    unsigned int audio_bitrate;
    unsigned int a_max_pkt_size;
} parser_audio_t;

typedef struct{
	char extension[8];
	unsigned int width;
	unsigned int height;
	unsigned int frame_rate;
	void *buf;		
	unsigned int video_bitrate;
	unsigned int v_max_pkt_size;	
}parser_video_t;

typedef struct{
	unsigned int drm_flag;
	char *license_info;
	char *special_info;
}parser_drm_t;

typedef struct
{
    char extension[8];      /* 解码库的后缀 */    
    void *buf;  
    void *private_data;            /* 指向特定的音频格式数据结构 */
} parser_subtitle_t;

typedef struct{	
	parser_audio_t parser_audio[16];//多达16条音轨
	parser_video_t parser_video;
	parser_drm_t parser_drm;
	parser_subtitle_t parser_subtitle[16];
	unsigned int sub_num;
	unsigned int audio_num;		
    unsigned int media_type;
	unsigned int total_time;
	unsigned int first_audio_time;	
	unsigned int index_flag;    	
	unsigned int a_len_array[32];
	unsigned int v_len_array[32];
}media_info_t;


/* 定义codec不支持特性*/
typedef enum
{
	PLUGIN_SUPPORTED,//支持
	PLUGIN_NOT_SUPPORTED_FIELDMOD,//h264/xvid场模式不支持
	PLUGIN_NOT_SUPPORTED_YUV444,//h264的yuv444不支持
	PLUGIN_NOT_SUPPORTED_GMC,//xvid sprite,gmc特性不支持
	PLUGIN_NOT_SUPPORTED_RPR,//rv变分辨率不支持
	PLUGIN_NOT_SUPPORTED_ADVANCED,//vc1的advanced profile不支持
	PLUGIN_NOT_SUPPORTED_OTHER//其他不支持
}video_plugin_supported_t;

typedef struct{
         unsigned int width;//宽
         unsigned int height;//高
         unsigned int src_width;//源宽
         unsigned int src_height;//源高
         unsigned int xpos;//起始刷屏点x轴,无padding则为0
         unsigned int ypos;//起始刷屏点y轴,无padding则为0 
         unsigned int ref_num; //参考帧个数
         unsigned int extra_frame_size;//根据该值中间件分配相应内存
         video_plugin_supported_t supported; // 根据spec设计玫举类型
}video_codec_info_t;

typedef struct fb_port_s 
{
    void *(*get_wbuf)(struct fb_port_s *port,unsigned int buf_size);     
    void *(*try_get_wbuf)(struct fb_port_s *port,unsigned int buf_size);     
} fb_port_t;


typedef struct {
	unsigned int display_flag;    //本帧可以显示--1，不能显示--0
	unsigned int use_flag;  //本帧正在被使用--1，没被使用--0    
	unsigned int time_stamp;    //本帧的时间戳
	unsigned int width;        //高
	unsigned int height;         //宽
	unsigned int format;         //数据格式，虽然与open接口处的format重复，但还是保留   
	unsigned int reserved1;
	unsigned int reserved2;
}dec_buf_t;

/*解码器和中间件之间传递的空白帧的数据结构*/
typedef struct
{
	dec_buf_t *vo_frame_info;
	unsigned char* vir_addr;
	unsigned char* phy_addr;
	unsigned int size;
}frame_buf_handle;
/*parser和decoder之间的传递包的数据结构*/
typedef struct
{
	unsigned char *vir_addr;
	unsigned char *phy_addr;
	unsigned int data_len;
	void *reserved;
}stream_buf_handle;

typedef struct{
		char file_extension[8];
		void *(*open)(void *input,void *media_info);		/*open插件时需要告诉当前插件的输入输出*/		
		int (*parse_stream)(void *handle,av_buf_t *ao_buf,av_buf_t *vo_buf);
		int (*seek)(void *handle,void *time4seek);
		int (*dispose)(void *handle);	
		int (*ex_ops)(void *handle,int cmd,void *arg);
}demux_plugin_t;

typedef struct{
		char extension[8];
		void *(*open)(void *ap_param,void *init_param,void *fb_vo);		
		int (*decode_data)(void *handle,stream_buf_handle *bitstream_buf);
		int (*dispose)(void *handle); 
		int (*ex_ops)(void *handle,int cmd,unsigned int arg);
		int (*probe)(void *init_buf,stream_buf_handle *bitstream_buf,video_codec_info_t *info);          
}videodec_plugin_t;

typedef struct
{
/*用于输入*/
  unsigned int seek_time;// 需要跳到的时间
  unsigned int cur_time; // 当前正播放到的时间，可能用于无index表搜索，无需求的不用该项
/*用于输出*/
  unsigned int found_audio_time;//跳到的文件位置的audio时间
  unsigned int found_video_time;//跳到的文件位置的video时间
} time_stuct_t;

#ifdef __cplusplus
}
#endif
#endif
