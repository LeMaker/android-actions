/*******************************************************************************
 *                              5003
 *                            Module: musicdec
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-18 15:00     1.0             build this file
*******************************************************************************/
/*!
 * \file     audio_decoder_lib_dev.h
 * \brief    音频解码库所需要的头文件
 * \author   kkli
 * \version 1.0
 * \date  2008/09/18
*******************************************************************************/
#ifndef __AUDIO_DECODER_LIB_DEV_H__
#define __AUDIO_DECODER_LIB_DEV_H__

#include "./actal_libc_dev.h"
#include "./common/audiout_pcm.h"
#include "./common/extdef.h"

typedef enum
{
    EX_OPS_SPDIF_OUTPUT     = 0x455801,
    EX_OPS_CHUNK_RESET      = 0x455802,
} audiodec_ex_ops_cmd_t;
/*!
 * \brief
 *      定义音频解码插件返回的类型
 */
typedef enum
{
    /*! 一般未知错误 */
    AD_RET_UNEXPECTED = -3,
    /*! 内存空间不够 */
    AD_RET_OUTOFMEMORY,
    /*! 格式不支持，不能继续解码 */
    AD_RET_UNSUPPORTED,
    /*! 正常 */
    AD_RET_OK,
    /*! 输入数据不够 */
    AD_RET_DATAUNDERFLOW,
} audiodec_ret_t;
/*!
 * \brief
 *      定义音频解码插件需提供的数据结构
 */
typedef struct
{
    /*! 插件库后缀，大写，例："COOK"，参考头文件定义 */
    char extension[MAX_EXT_SIZE];

    /*!
     * \par  Description:
     *	  打开插件
     * \param[in]   init_buf 初始化信息，由parser和解码库自行约定数据结构
     * \return      插件句柄
     * \retval           others sucess
     * \retval           NULL failed
     */
    void *(*open)(void *init_buf);
    /*!
     * \par  Description:
     *	  解码一帧数据
     * \param[in]   handle 插件句柄
     * \param[in]   input 输入数据的起始地址
     * \param[in]   input_bytes 输入数据的长度
     * \param[out]  aout 解码输出，结构参考audiout_pcm_t
     * \param[out]  bytes_used 解码当前帧用的字节数
     * \return      the result (audiodec_ret_t)
     */
    int (*frame_decode)(void *handle, const char *input, const int input_bytes, audiout_pcm_t *aout, int *bytes_used);
    /*!
     * \par  Description:
     *	  扩展命令，不需要此功能的实现为空即可
     * \param[in]   handle 插件句柄
     * \param[in]   cmd 命令字
     * \param[in]   args 参数
     * \return      the result (audiodec_ret_t)
     */
    int (*ex_ops)(void *handle, int cmd, int args);
	/*!
     * \par  Description:
     *	  关闭插件
     * \param[in]   handle 插件句柄
     */
    void (*close)(void *handle);
} audiodec_plugin_t;

#endif  // __AUDIO_DECODER_LIB_DEV_H__
