#ifndef __STREAM_INPUT_H__
#define __STREAM_INPUT_H__

//------------------------------------------------------------------------------------
/*ÎÄ¼ş²Ù×÷*/
#define		DSEEK_SET		0x01
#define		DSEEK_END		0x02
#define		DSEEK_CUR		0x03

typedef struct stream_input_s {
    int (*read)(struct stream_input_s *stream_input,unsigned char *buf,unsigned int len);
    int (*write)(struct stream_input_s *stream_input,unsigned char *buf,unsigned int len);
    int (*seek)(struct stream_input_s *stream_input,mmm_off_t offset,int original);
    mmm_off_t (*tell)(struct stream_input_s *stream_input);
} stream_input_t;
#endif
