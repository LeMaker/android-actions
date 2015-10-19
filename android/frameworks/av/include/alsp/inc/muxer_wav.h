/*******************************************************************************
 *                              5003
 *                            Module: musicenc
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-20 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     muxer_wav.h
 * \brief    定义了wav编码需要的一些特殊的字段
 * \author   kkli
 * \version 1.0
 * \date  2008/09/20
*******************************************************************************/
#ifndef __MUXER_WAV_H__
#define __MUXER_WAV_H__
#ifdef __cplusplus
extern "C" {
#endif

#define WAV_LPCM        0x1
#define WAV_MS_ADPCM    0x2
#define WAV_ALAW        0x6
#define WAV_ULAW        0x7
#define WAV_IMA_ADPCM   0x11

typedef struct
{
    /*! 采样率，单位Hz */
    int32_t sample_rate;
    /*! 比特率，单位bps */
    int32_t bitrate;
    /*! 声道 */
    int32_t channels;
    /*! chunk时间，单位毫秒 */
    int32_t chunk_time;
    /*! 0:纯音乐录音, 1:others，区分是否需要在attribute中预留header的空间 */
    int32_t mode;
    /*! 0x1: linear-pcm; 0x2: ms-adpcm; 0x6: alaw; 0x7: ulaw; 0x11: ima-adpcm */
    int32_t format;
} wav_audio_t;

#ifdef __cplusplus
}
#endif
#endif // __MUXER_WAV_H__
