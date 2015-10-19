/*******************************************************************************
 *                              5003
 *                            Module: musicenc
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-18 11:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     enc_audio.h
 * \brief    定义打开一个音频编码库所需的信息
 * \author   kkli
 * \version 1.0
 * \date  2008/9/18
*******************************************************************************/
#ifndef __ENC_AUDIO_H__
#define __ENC_AUDIO_H__

/*!
 * \brief  
 *      打开音频编码库所需的信息
 */
typedef struct
{
	/*! 采样率，单位hz */
    int sample_rate;
    /*! 声道数 */
    int channels;
    /*! 歌曲比特率 */
    int bitrate;
    /*! 编码库格式，一个加载的编码库里可能包含多个格式，例：IMADPCM */
    int audio_format;
    /*! 一次获取到的音频编码后数据播放时长，0说明不限制，单位ms */
    int chunk_time;   
} enc_audio_t;

#endif // __ENC_AUDIO_H__
