/*******************************************************************************
 *                              5003
 *                            Module: ra
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2008-09-20 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     parser_ra.h
 * \brief    定义了rm parser必须提供给音频解码的初始化数据结构
 * \author   kkli
 * \version 1.0
 * \date  2008/09/20
*******************************************************************************/
#ifndef __PARSER_RA_H__
#define __PARSER_RA_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t FlavorIndex;
    int32_t BitsPerFrame;
    int32_t Channels;
    
	int32_t version;
	int32_t samples;
	int32_t regions;

	int32_t delay;
	int32_t cpl_start;
	int32_t cpl_qbits;

} parser_ra_t;

#ifdef __cplusplus
}
#endif
#endif // __PARSER_RA_H__
