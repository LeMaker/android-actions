/*******************************************************************************
 *                              5003
 *                            Module: encoder linker
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved. 
 *
 * History:         
 *      <author>    <time>           <version >             <desc>
 *       kkli     2009-01-01 15:00     1.0             build this file 
*******************************************************************************/
/*!
 * \file     link_encoder.c
 * \brief    提供不同平台下编码库插件链接所需的库
 * \author   kkli
 * \par      GENERAL DESCRIPTION:
 *               这里对文件进行描述
 * \par      EXTERNALIZED FUNCTIONS:
 *               这里描述调用到外面的模块
 * \version 1.0
 * \date  2009/1/1
*******************************************************************************/

extern int audioenc_plugin_info;

void *get_plugin_info(void)
{
    return (void *)&audioenc_plugin_info;
}
