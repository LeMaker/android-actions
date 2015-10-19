/*******************************************************************************
 *                              5003
 *                            Module: aac
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-20 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     parser_aac.h
 * \brief    定义了parser必须提供给aac解码库的初始化数据结构
 * \author   kkli
 * \version 1.0
 * \date  2008/09/20
*******************************************************************************/
#ifndef __PARSER_AAC_H__
#define __PARSER_AAC_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t object_type;
    int32_t sf_index;
    int32_t channels;
} parser_aac_t;

#ifdef __cplusplus
}
#endif
#endif // __PARSER_AAC_H__
