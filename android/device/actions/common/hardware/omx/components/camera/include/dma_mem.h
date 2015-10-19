/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/******************************************************************************/

/******************************************************************************/
#ifndef __DMA_MEM_H__
#define __DMA_MEM_H__

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

/******************************************************************************/
/**
 * sys_mem_allocate() - alloc memory for user mode applications
 * @size: Describe the size of memory to alloc.
 * @mem_flag: Describe the memory type to alloc.
 *
 * This function returns a pointer to the allocated memory.
 **/
void *sys_mem_allocate(u32 size, u32 mem_flag);

/**
 * sys_mem_free() - free memory for user mode applications
 * @pBuffer: Describe the pointer of memory to free.
 *
 * This function returns errno to tell free operations results.
 **/
s32 sys_mem_free(void *pBuffer);

/**
 * sys_get_phyaddr() - translate the virsual address to physical address
 * @pBuffer: Describe the pointer of memory virsual address.
 *
 * This function returns pointer of physical address.
 **/
void *sys_get_phyaddr(void *pBuffer);

/**
 * sys_get_viraddr() - translate the physical address to virsual address
 * @pBuffer: Describe the pointer of memory physical address.
 *
 * This function returns pointer of virsual address in loacl process.
 **/
void *sys_get_viraddr(void *pBuffer, u32 mem_flag);

/**
 * sys_mem_show() - show the memory zone bitmaps informations
 *
 * This function for show the memory zone bitmaps informations.
 **/
void sys_mem_show(void);

/**
 * sys_mem_dump() - show the memory zone nodes informations
 *
 * This function for show the memory zone nodes informations.
 **/
void sys_mem_dump(void);

/**
 * r4k_dma_cache_wback_inv() - invaild writeback type memory
 * @addr: Describe the address to cache operation.
 * @size: Describe the size of address to cache operation.
 *
 **/
void r4k_dma_cache_wback_inv(u32 addr, u32 size);

/**
 * r4k_dma_cache_inv() - invaild memory
 * @addr: Describe the address to cache operation.
 * @size: Describe the size of address to cache operation.
 *
 **/
void r4k_dma_cache_inv(u32 addr, u32 size);
/******************************************************************************/
/* 分配内存类型的标志, 用bit 0来表示是否是连续空间或是非连续空间*/
#define MEM_DISCRETE    (0)
#define MEM_CONTINUOUS  (1 << 0)
/* 内存类型标识的mask*/
#define MEM_TYPE_MASK   (1 << 0)

/* 内存cache方式的标识, 用bit 1和2来标识*/
/* uncache*/
#define UNCACHE_MEM         (1 << 1)
/* write through, no write allocate*/
#define WRITE_THROUGH_MEM   (1 << 2)
/* write back, write allocate*/
#define WRITE_BACK_MEM      (1 << 3)
/* write through, write allocate*/
//#define WRITE_THROUGH_ALLOC 0x00000006
/* mask for cache type */
#define MEM_CACHE_TYPE_MASK (UNCACHE_MEM | WRITE_THROUGH_MEM | WRITE_BACK_MEM)
/******************************************************************************/

#endif

