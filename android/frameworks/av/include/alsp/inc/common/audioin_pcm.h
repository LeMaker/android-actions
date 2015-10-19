/*******************************************************************************
 *                              5003
 *                            Module: audio in
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-05 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     audioin_pcm.h
 * \brief    解码输出的数据结构
 * \author   kkli
 * \version 1.0
 * \date  2008/09/05
*******************************************************************************/
#ifndef __AUDIOIN_PCM_H__
#define __AUDIOIN_PCM_H__

/*!
 * \brief  
 *      定义最大的输入声道数
 */
#define MAX_CHANNEL_IN     2
/*!
 * \brief  
 *      音频编码插件帧解码的输入或音频输入模块的输出
 */
typedef struct
{
    /*! pcm数据指针数组 */
    intptr_t pcm[MAX_CHANNEL_IN];
    /*! 当前输入的channel数 */
    int channels;
    /*! 当前输入的sample数 */
    int samples;
} audioin_pcm_t;

#endif // __AUDIOIN_PCM_H__
