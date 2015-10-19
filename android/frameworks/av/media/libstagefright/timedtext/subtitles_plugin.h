#ifndef _SUBTITLE_PLUGIN_H
#define _SUBTITLE_PLUGIN_H

/*! \cond SUBTITLES_API*/

#ifdef __cplusplus 
extern "C" { 
#endif

//SUB?????????
#define BLACK_YUV 0
#define WHITE_YUV 255
#define BACKGROUD_YUV 128
#define BLACK_RGB 0
#define WHITE_RGB 0xffff
#define BACKGROUD_RGB 0x8410

enum {SUB_TEXT,SUB_IMAGE};
typedef enum {SUB_SRT,SUB_SUB,SUB_ASS,SUB_SSA,SUB_RAW,SUB_SMI,UNKNOWN,SUB_PGS,SUB_XSUB} SUB_TYPE;
typedef enum {ENC_ASCII,ENC_UTF8,ENC_UTF16} ENC_TYPE;

//????????28?bytes???,???28bytes?????
typedef struct{
	/*???*/
	unsigned int header_type;
	/*?????*/
	unsigned int block_len;
	/*??????*/
	unsigned int packet_offset;
	/*?????*/
	unsigned int packet_ts;
	/*????1*/
	unsigned int reserved1;
	/*????2*/
	unsigned int reserved2;
	/*h264????????????*/
	unsigned char stream_end_flag;/*1???, 0??*/
	unsigned char parser_format;
	unsigned char seek_reset_flag;
	unsigned char reserved_byte2;
}packet_header_t;

/*!
 * \brief
 *      sub????????
 */
typedef struct tSubtitlesLanguege
{
	/*! ?????? */
	int iLangNum;	
	/*! ??????????"zh""en"? */	
	char *pLangStr[16];	
}tLang;

/*!
 * \brief
 *      sub??
 */
typedef struct tSubtitleParam
{
	/*! ?????*.srt *.ass *.sub *.smi ??:filename[0]=test.srt */
	char* filename[10];
	/*! ?????,????????????buffer?? */	
	char* pCanvas;	
	/*! ?????,???????buffer??? */	
	int Canvas_width;
	/*! ?????,???????buffer??? */	
	int Canvas_height;	
}tSubParam;

/*!
 * \brief
 *      ???????????????
 */
typedef struct tSubtitle
{
	/*! ?????? */
	int iStartTime;	
	/*! ?????? */
	int iEndTime;	
	/*! ??????????{SUB_TEXT,SUB_IMAGE} */		
	int iSubtitleType;
	/*! ????????????? */	
	char *pSubtitle_buf;	
	/*! ??????? */
	int iText_Len;	
	/*! ??????? */	
	int iImage_Width;
	/*! ??????? */	
	int iImage_Height;	
	/*! ?????????,ASCII?UTF8? */
	ENC_TYPE encoding;	
}tSub;

/*!
 * \brief
 *      ???????????
 */
typedef struct tSubtitleParser
{
	/*! ???????????? */
	tSub Sub;
	/*! ????,???? */
	tLang Lang;
	/*! ????????? ??srt?ass?sub?smi? */
	SUB_TYPE my_sub_type;	
	/*! ??????????????ASCII?UTF8? */
	ENC_TYPE my_encoding;
	/*! ??????????? */
	void *sub_dec;
}tSubParser;

typedef struct{
	  int (*subtitle_open)(void *sub_handle,unsigned char *textbuf,int buflen,char *mimeType);
		tSub *(*get_subtitles)(void *sub_handle,int index);
		void *(*subtitle_close)(void *sub_handle); 
		int (*subtitle_open_buf)(void *sub_handle,char *buf);
		tSub *(*decode_subtitles_buf)(void *sub_handle,char *buf);
		void *(*subtitle_close_buf)(void *sub_handle);
}subdec_plugin_t;

/*reset parameter used in android project *.sub format file only*/
int subtitle_reset_param(void *sub_handle, tSubParam *sub_param);

/*get total time of subtitles, used in android project srt file only*/
int subtitle_get_totaltime(void *sub_handle);

/*return a subtitle according time_stamp*/
tSub *find_subtitles(void *sub_handle,int time_stamp);


/*initial*/
int subtitle_open(void *sub_handle,unsigned char *textbuf,int buflen,char *mimeType);


/*dispose*/
void *subtitle_close(void *sub_handle);

/*decode a subtitle*/ 
tSub *decode_subtitles_buf(void *sub_handle,char *buf);

/*return a sub subtitle languege group*/
tLang *get_sub_lang(void *sub_handle);

/*return a sub subtitle languege group*/
void *select_sub_lang(void *sub_handle,char *lang);

/*return a sub subtitle languege group*/
void *select_sub_lang_index(void *sub_handle, int wanted_lang_index);

/*initial*/
int subtitle_open_buf(void *sub_handle,char *buf);

/*dispose*/
void *subtitle_close_buf(void *sub_handle);

/*return a subtitle according text index*/
tSub *get_subtitles(void *sub_handle,int index);

#ifdef __cplusplus 
} 
#endif 

/*! \endcond*/

#endif
