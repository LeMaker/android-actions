/*******************************************************************************
 *                              5003
 *                            Module: common
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       kkli     2009-01-01 15:00     1.0             build this file
*******************************************************************************/
/*!
 * \file     al_libc.h
 * \brief    al内部实现的C库函数声明，对应于有一个确定的实现
 * \author   kkli
 * \version 1.0
 * \date  2009/1/1
*******************************************************************************/
#ifndef __AL_LIBC_H__
#define __AL_LIBC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INLINE      inline

typedef int64_t mmm_off_t;

/*!
 * \brief
 *      内存相关操作声明
 */
void *actal_memcpy(void *, const void *, int32_t);
void *actal_memset(void *, int32_t, int32_t);

/*!
 * \brief
 *      内存管理相关操作声明
 */
/*! 申请逻辑地址，参数为长度 */
void *actal_malloc(int32_t);
void actal_free(void *);
/*! 申请物理地址连续的地址，第一个参数为长度，第二个参数为输出的物理地址 */
void *actal_malloc_dma(int32_t,void *);
void actal_free_dma(void *);
/*! 申请物理地址连续的write-through地址，第一个参数为长度，第二个参数为输出的物理地址
    write-through的特点是：保证了读的速度，写的时候不只是更新到cache
 */
//申请带cache的连续物理内存，cache由系统自动同步(dma等操作会同步cache)
void *actal_malloc_wt(int32_t, void *);
void actal_free_wt(void *);
//申请带cache的连续物理内存，cache必须手动同步
void *actal_malloc_cached_manual(int32_t,void *);
void actal_free_cached_manual(void *);
//手动同步cache
void actal_cache_flush(void *);
void actal_cache_env(void *);
/*! 申请物理地址连续的不经过cache的地址，第一个参数为长度，第二个参数为输出的物理地址
    不经过cache的特点是：对该地址进行操作后可以直接进行dma操作
 */
void *actal_malloc_uncache(int32_t, void *);
void actal_free_uncache(void *);
/*! 获取物理地址 */
long actal_get_phyaddr(void *);
void * actal_get_virtaddr(long );
/*!
 * \brief
 *      按规则打印一段内存地址
 */
void actal_dump(int *, int32_t);

/*!
 * \brief
 *      获取系统当前时间，单位ms，用于测定性能
 */
int64_t actal_get_ts(void);

/*!
 * \brief
 *      设定打印输出，可被统一打开或关闭
 */
int32_t actal_printf(const char *format, ...);
/*!
 * \brief
 *      设定打印输出，始终打开，字符限制为256个
 */
int32_t actal_error(const char *format, ...);
/*!
 * \brief
 *      设定打印输出，发布时关闭
 */
int32_t actal_info(const char *format, ...);

/*!
 * \brief
 *      设置睡眠一段时间，输入参数毫秒，会导致任务切换
 */
void actal_sleep_ms(int32_t);
/*!
 * \brief
 *      获取IC信息，0~7bit表示ic类型，8~15bit表示ic版本，如：0x4303表示5003 C版
 */
int actal_get_icinfo(void);

int actal_check_utf8(const char *utf8, int length);
int actal_convert_ucnv(char *from_charset, char *to_charset, const char *inbuf, int inlen,
		char *outbuf, int outlen);
int actal_encode_detect(const char *src, char *encoding);
int actal_get_appxml_bool(const char *attribute);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __AL_LIBC_H__
