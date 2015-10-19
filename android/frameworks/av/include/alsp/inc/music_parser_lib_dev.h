/*******************************************************************************
 *                              5003
 *                            Module: musicdec
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-12 15:00     1.0             build this file
*******************************************************************************/
/*!
 * \file     music_parser_lib_dev.h
 * \brief    定义和parser插件有关的数据结构
 * \author   kkli
 * \version 1.0
 * \date  2008/09/18
*******************************************************************************/
#ifndef __MUSIC_PARSER_LIB_DEV_H__
#define __MUSIC_PARSER_LIB_DEV_H__

#include "./common/al_libc.h"
#include "./common/storageio.h"
#include "./common/music_info.h"
#include "./common/extdef.h"

typedef enum
{
    EX_OPS_GET_RESTIME     = 0x555801,
} audioparser_ex_ops_cmd_t;
/*!
 * \brief
 *      定义parser插件返回的类型
 */
typedef enum
{
    /*! 一般未知错误 */
    MP_RET_UNEXPECTED = -3,
    /*! 内存空间不够 */
    MP_RET_OUTOFMEMORY,
    /*! 格式不支持 */
    MP_RET_UNSUPPORTED,
    /*! 正常 */
    MP_RET_OK,
    /*! 文件结束 */
    MP_RET_ENDFILE
} music_parser_ret_t;
/*!
 * \brief
 *      定义parser插件需提供的数据结构
 */
typedef struct
{
    /*! 插件库后缀，大写，例："RM" */
    char extension[MAX_EXT_SIZE];

    /*!
     * \par  Description:
     *	  打开插件
     * \param[in]   storage_io 对输入进行读/写/定位等文件操作的函数指针，结构参考storage_io_t
     * \return      插件句柄
     * \retval           others sucess
     * \retval           NULL failed
     */
    void *(*open)(storage_io_t *storage_io);
    /*!
     * \par  Description:
     *	  解析文件头
     * \param[in]   handle 插件句柄
     * \param[out]  music_info 由插件返回的音乐信息，结构参考music_info_t
     * \return      the result (music_parser_ret_t)
     */
    int (*parser_header)(void *handle, music_info_t *music_info);
    /*!
     * \par  Description:
     *	  获取一个chunk的数据(chunk可以认为是多个帧的集合，建议持续时间一秒钟左右)
     * \param[in]   handle 插件句柄
     * \param[in]  output 数据输出的起始地址
     * \param[out]  chunk_bytes 当前输出chunk的字节数
     * \return      the result (music_parser_ret_t)
     */
    int (*get_chunk)(void *handle, char *output, int *chunk_bytes);
    /*!
     * \par  Description:
     *	  定位到文件相应的时间（毫秒）
     * \param[in]   handle 插件句柄
     * \param[in]   time_offset 时间偏移量
     * \param[in]   whence 参照位置（取值同文件seek）
     * \param[out]  chunk_start_time 完成之后的起始播放时间
     * \return      the result (music_parser_ret_t)
     */
    int (*seek_time)(void *handle, int time_offset, int whence, int *chunk_start_time);
    /*!
     * \par  Description:
     *	  扩展命令，不需要此功能的实现为空即可
     * \param[in]   handle 插件句柄
     * \param[in]   cmd 命令字
     * \param[in]   args 参数
     * \return      the result (music_parser_ret_t)
     */
    int (*ex_ops)(void *handle, int cmd, int args);
    /*!
     * \par  Description:
     *	  关闭插件
     * \param[in]   handle 插件句柄
     */
    void (*close)(void *handle);
} music_parser_plugin_t;

#endif // __MUSIC_PARSER_LIB_DEV_H__
