/*******************************************************************************
 *                              5003
 *                            Module: musicdec
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-05 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     music_info.h
 * \brief    定义parser输出的歌曲信息结构
 * \author   kkli
 * \version 1.0
 * \date  2008/09/05
*******************************************************************************/
#ifndef __MUSIC_INFO_H__
#define __MUSIC_INFO_H__

/*!
 * \brief  
 *      parser插件库返回的信息结构
 */
typedef struct
{
    /*! 解码库的后缀，大写，例："COOK" */
    char extension[8];
    /*! 最大的chunk大小，单位byte */
    int max_chunksize;
    /*! 总时间，单位seconds */
    int total_time;
    /*! 平均比特率，单位kbps */
    int avg_bitrate;
	/*! 采样率，单位hz */
    int sample_rate;
    /*! 声道数 */
    int channels;
    /*! 打开音频解码插件所需的输入参数，结构由特定格式约定 */
    void *buf;
} music_info_t;

#endif // __MUSIC_INFO_H__
