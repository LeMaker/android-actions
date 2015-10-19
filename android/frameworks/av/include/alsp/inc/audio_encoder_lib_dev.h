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
 * \file     audio_encoder_lib_dev.h
 * \brief    定义了audioenc插件通用的接口
 * \author   kkli
 * \version 1.0
 * \date  2008/9/18
*******************************************************************************/
#ifndef __AUDIO_ENCODER_LIB_DEV_H__
#define __AUDIO_ENCODER_LIB_DEV_H__

#include "./common/audioin_pcm.h"
#include "./common/enc_audio.h"
#include "./common/extdef.h"

typedef enum
{
    /*! 一般未知错误 */
    AE_RET_UNEXPECTED = -7,
    /*! 编码库加载出错 */
    AE_RET_LIBLOAD_ERROR,
    /*! 加载了非法的编码库 */
    AE_RET_LIB_ERROR,
     /*! 编码出错 */
    AE_RET_ENCODER_ERROR,
     /*! 采样率错误 */
    AE_RET_FS_ERROR,
    /*! 内存空间不够 */
    AE_RET_OUTOFMEMORY,
    /*! 格式不支持，不能继续解码 */
    AE_RET_UNSUPPORTED,
    /*! 正常 */
    AE_RET_OK,
    /*! 输出数据超过缓冲区大小 */
    AE_RET_DATAOVERFLOW,
} audioenc_ret_t;

typedef struct
{
    /*! 编码一帧所需的样本数 */
    int samples_per_frame;
    /*! payload/packet大小，单位字节 */
    int chunk_size;
    /*! 初始写入文件起始的数据 */
    /*! 数据指针 */
    void *buf;
    /*! 数据长度 */
    int buf_len;
} audioenc_attribute_t;

typedef struct
{
    char extension[MAX_EXT_SIZE];

    void *(*open)(enc_audio_t *enc_audio);
    int (*get_attribute)(void *handle, audioenc_attribute_t *attribute);
    int (*update_header)(void *handle, char **header_buf, int *header_len);
    int (*frame_encode)(void *handle, audioin_pcm_t *ain, char *output, int *bytes_used);
    void (*close)(void *handle);
} audioenc_plugin_t;

#endif // __AUDIO_ENCODER_LIB_DEV_H__
