#ifndef __BUF_HEADER_H__
#define __BUF_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

/*包类型*/
typedef enum
{
    AUDIO_PACKET = 0x100,
    VIDEO_PACKET = 0x200,
    SUBPIC_PACKET = 0x300
} packet_type_t;

//音视频包都预留了28个bytes头未用，现将此28bytes数据结构化
typedef struct{
	/*包类型*/
	unsigned int header_type;
	/*包载荷长度*/
	unsigned int block_len;
	/*包的文件位置*/
	unsigned int packet_offset;
	/*包的时间戳*/
	unsigned int packet_ts;
	/*保留字段1*/
	unsigned int reserved1;
	/*保留字段2*/
	unsigned int reserved2;
	/*h264得到正确时间戳需要的信息*/
	unsigned char stream_end_flag;/*1为结束， 0为否*/
	unsigned char parser_format;
	unsigned char seek_reset_flag;
	unsigned char reserved_byte2;
}packet_header_t;
#ifdef __cplusplus
}
#endif
#endif // __BUF_HEADER_H__
