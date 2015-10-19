/*******************************************************************************
 *                              5003
 *                            Module: audio out
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-05 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     audiout_pcm.h
 * \brief    定义解码输出的数据结构
 * \author   kkli
 * \version 1.0
 * \date  2008/09/05
*******************************************************************************/
#ifndef __AUDIOUT_PCM_H__
#define __AUDIOUT_PCM_H__

/*!
 * \brief  
 *      定义最大的输出声道数
 */
#define MAX_CHANNEL     6
/*!
 * \brief  
 *      音频解码插件帧解码的输出或音频输出模块的输入
 */
typedef struct
{
    /*! pcm数据指针数组，包含各声道输出数据的起始地址 */
    intptr_t pcm[MAX_CHANNEL];
    /*! 当前输出包含的声道数 */
    int channels;
    /*! 当前输出包含的样本数，只计单个声道 */
    int samples;
    /*! 当前输出小数点的位数，整数取值0 */
    int frac_bits;
} audiout_pcm_t;

#endif // __AUDIOUT_PCM_H__
