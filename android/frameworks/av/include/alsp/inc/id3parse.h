#ifndef __ID3PARSE_H__
#define __ID3PARSE_H__
#include <utils/Log.h>

#include <media/stagefright/DataSource.h>
#include "actal_posix_dev.h"

namespace android {
#ifdef __cplusplus
extern "C" {
#endif

#define __ITEM_ENCODING__ 1
#define __ALBUM_ART__  0
#define  __TRACK_NUM__ 1
#define  IS_MIN_CHAR(wch)   (((wch) > 0x60) && ((wch) < 0x7b))
#define  UPPER(s)  (IS_MIN_CHAR(s)?(s-32):(s))

#define ID3_SECTORSIZE  512*10  //cz_20121112 ID3死机原因是这个太小了，目前扩大到5K

enum 
{
    ENCODING_NORMAL = 0,
    ENCODING_UNICODE = 1,
    ENCODING_UTF8 = 2,
};

typedef struct _item_info 
{
    char encoding;
    int length;
    char* content;
}id3_item_info_t;

typedef struct _image_info
{
    int offset;
    int length;
    char imageType[8];
}id3_image_t;

//typedef struct {
//    int Address;
//    int len;
//}id3_item_t;

typedef struct _id3_info {
  id3_item_info_t author;
  id3_item_info_t composer;
  id3_item_info_t album;
  id3_item_info_t genre;
  id3_item_info_t track;
  id3_item_info_t year;
  id3_item_info_t title;
  id3_item_info_t comment;
  id3_item_info_t autoLoop;
#if __ALBUM_ART__ > 0
  id3_item_info_t albumArt;
#endif
  id3_image_t imageInfo;
}id3_info_t;

typedef struct
{
  unsigned int bitrate;
  unsigned int sample_rate;
  unsigned int channel;
  unsigned int total_time;
}id3_ext_info;

typedef struct
{
  id3_info_t   tag;
  id3_ext_info extra_info;
}id3_info_total;

typedef struct {
    off_t mOffset;
    off_t mFileSize;
    sp<DataSource> mSource;
} ID3file_t;

//void TransToLittleEnd(char *buffer, int bufferLen);
//int utf8_to_utf16(const unsigned char *utf8, int cc, unsigned short *unicode16);
int get_audio_id3_info(ID3file_t* fp, char *fileinfo, id3_info_total* info);

//for mp3 id3;
void get_mp3_audio_info(void* fp,id3_info_total* info);
void freeallmemory(id3_info_total *Tag);

// for ogg id3
void get_ogg_audio_info(void* fp,id3_info_total* info);

//for wma id3
void get_wma_audio_info(void* fp,id3_info_total* info);

//for audible
//void get_audible_audio_info(char* fp,id3_info_total* info);

//for ape
void get_ape_audio_info(void* fp,id3_info_total* info);
void get_flac_audio_info(void *fd, id3_info_total* info);

//void get_mp3_image (FS_Handle pFile, id3_image_t *imageInfo);

int ID3_fseek(void *stream, long offset, int whence);
int ID3_fread(void *buffer, size_t size, size_t count, void* stream);
int ID3_getfilelength(void *stream);
int parse_id3V1(void *fp, id3_info_total* pid3Info);
int parse_id3V2_3(void *fp, id3_info_total* pid3Info);
void getgenre(char c, id3_item_info_t* temp);
void TransToLittleEnd(char *buffer, int bufferLen);
int Unicode2utf8(char * string, int length);
int wstrlen(char *buffer, int bufferLen);
void item_itoa(id3_item_info_t *pitem);
void freeallmemory(id3_info_total *info);
void freeItem(id3_item_info_t *item);
#ifdef __cplusplus
}
#endif // __cplusplus
} // namespace android

#endif
