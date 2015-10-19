/*******************************************************************************
 *                              这里填写项目名
 *                            Module: 这里填写模块名
 *                 Copyright(c) 2003-2008 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       kkli     2009-01-01 15:00     1.0             build this file
 *******************************************************************************/
/*!
 * \file     libc.c
 * \brief    提供平台差异C库的实现
 * \author   kkli
 * \par      GENERAL DESCRIPTION:
 *               这里对文件进行描述
 * \par      EXTERNALIZED FUNCTIONS:
 *               这里描述调用到外面的模块
 * \version 1.0
 * \date  2009/1/1
 *******************************************************************************/

#define LOG_TAG             "AL_LIBC"
#ifdef ANDROID
#include <utils/Log.h>
#endif
#ifndef ANDROID
#define ALOGV(fmt, args...) do{printf("V " fmt "\n", ##args);}while(0)
#define ALOGD(fmt, args...) do{printf("D " fmt "\n", ##args);}while(0)
#define ALOGI(fmt, args...) do{printf("I " fmt "\n", ##args);}while(0)
#define ALOGW(fmt, args...) do{printf("W " fmt "\n", ##args);}while(0)
#define ALOGE(fmt, args...) do{printf("E " fmt "\n", ##args);}while(0)
#endif

#include "actal_posix_dev.h"
#include <ion/ion.h>

#define ALIGN_BYTES         4096
#define ALIGN_MASK          (ALIGN_BYTES - 1)

#define PRINT_BUF_SIZE      1024
struct actal_mem  {
	int fd;
	ion_user_handle_t handle;
	long len;
	void *ptr;
	int map_fd;
    long phy_add;
	int flag; //cache  auto or manual
	struct actal_mem *next;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_fd = 0;
static int s_pid = 0;
static struct actal_mem *s_top_p = NULL, *s_current_p = NULL;
#define _ALSP_DEBUG_
int actal_printf(const char *format, ...) 
{
#ifdef _ALSP_DEBUG_
	va_list arg;
	char printbuf[PRINT_BUF_SIZE];
	int done;

	va_start(arg, format);
	done = vsnprintf(printbuf, PRINT_BUF_SIZE, format, arg);
	ALOGV("%s", printbuf);
	va_end(arg);

	return done;
#else
	return 0;
#endif
}
int actal_error(const char *format, ...) 
{
	va_list arg;
	char printbuf[PRINT_BUF_SIZE];
	int done;

	va_start(arg, format);
	done = vsnprintf(printbuf, PRINT_BUF_SIZE, format, arg);
	ALOGE("%s", printbuf);
	va_end(arg);

	return done;
}
int actal_info(const char *format, ...) 
{
#ifdef _ALSP_RELEASE_    
	return 0;
#else
	va_list arg;
	char printbuf[PRINT_BUF_SIZE];
	int done;

	va_start(arg, format);
	done = vsnprintf(printbuf, PRINT_BUF_SIZE, format, arg);
	ALOGI("%s", printbuf);
	va_end(arg);

	return done;
#endif    
}

void actal_printf_list()
{
	if(s_top_p == NULL)
	{
		ALOGD("list null");
		return;
	}
    struct actal_mem * user_p = s_top_p;
    while(user_p->next!=NULL)
    {
        user_p = user_p->next;
    }
    ALOGD("list end");
}

long actal_get_phyaddr(void *ptr)
{
    long align = 0;
    long phy_add = 0;
    struct actal_mem * user_p;

	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("get mutex failed");
        return 0;
    }

    user_p = s_top_p->next;
    while(user_p != NULL)
    {
        align = (char*)ptr - (char*)user_p->ptr;
        if((user_p->ptr <= ptr) && (align < user_p->len))
        {
            // ALOGD("getphy: ptr = %X, phy_add = %X, align = %d\n", (unsigned int)ptr, user_p->phy_add + align, align);
            phy_add = user_p->phy_add + align;
			break;
        }
        user_p = user_p->next;
    }
	
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("free mutex failed");
        return 0;
    }

	if(phy_add == 0)
	{
		actal_error("error!,phy_add not found. ptr = %X\n", ptr);
	}
    return phy_add;
}

void * actal_get_virtaddr(long phy)
{
    long align = 0;
    struct actal_mem * user_p;
	void * virt_add = NULL;

	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("get mutex failed");
        return NULL;
    }

    user_p = s_top_p->next;
    while(user_p != NULL)
    {
        align = phy - user_p->phy_add;
        if((user_p->phy_add <= phy) && (align < user_p->len))
        {

            virt_add = (void*)((char*)user_p->ptr +align);
			break;
        }
        user_p = user_p->next;
    }
	
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("free mutex failed");
        return NULL;
    }

	if(NULL == virt_add)
	{
		actal_error("error!,phy_add not found.\n");
	}
    return virt_add;
}

void *actal_malloc(int size)
{
    void *ptr = NULL;

    ptr = (void *)malloc(size);

    return ptr;
}
//
void actal_free(void *ptr)
{
    free(ptr);
}

void check_pid()
{
	struct actal_mem * user_p;
	// int ret = 0;
	
	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("get mutex failed");
        return ;
    }

	if(s_pid != getpid())
	{
		ALOGD("PID changed, reopen ion device");
		ALOGD("parent pid = %d, fd = %d", s_pid, s_fd);
		if(s_top_p != NULL)
		{
			s_current_p = s_top_p->next;
			while((user_p = s_current_p) != NULL)
			{
				s_current_p = user_p->next;
				// ret = ion_free(user_p->fd, user_p->handle);
				munmap(user_p->ptr, user_p->len);
				// close(user_p->map_fd);
				free(user_p);
				user_p = NULL;
			}
			s_top_p->next = NULL;
			s_current_p = s_top_p;
		}
		ion_close(s_fd);
		s_fd = ion_open();
		s_pid = getpid();
		ALOGD("new pid = %d, fd = %d", s_pid, s_fd);
	}
	
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("free mutex failed");
        return ;
    }
}

//alloc ion cached buf
// flag ION_FLAG_CACHED 			cached memory, default auto sync
// flag ION_FLAG_CACHED_NEEDS_SYNC  cached memory, and manual sync
void *actal_malloc_cached_opt(int size, void *phy_add, int flag)
{
	int prot = PROT_READ | PROT_WRITE;
	int map_flags = MAP_SHARED;
	ion_user_handle_t handle;
	int map_fd, ret;
    void *ptr;
	
	if(size <= 0){
		ALOGE("actal_malloc_wt: size must be positive, invaliable size value. \n");
		return NULL; //-EINVAL;
	}

	check_pid();

    if (size & ALIGN_MASK){
        //4k对齐
        size += (ALIGN_BYTES - (size & ALIGN_MASK));
    }

    struct actal_mem * user_p;

	user_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
	user_p->next = NULL;
	
	//fd, size, align, heap_mask, flag, handle
	ret = ion_alloc(s_fd, size, 0, 1, flag, &handle);
	if(ret < 0) {
		actal_error("actal_malloc_wt: ion_alloc(size: %d)  failed(%d)!\n", size, ret);
		return NULL;
	}
		
	ret = ion_map(s_fd , handle, size, prot, map_flags, 0, (unsigned char **)&ptr, &map_fd);

	user_p->handle = handle;
    user_p->len = size;
    user_p->fd = s_fd;
	user_p->ptr = ptr;
	user_p->map_fd = map_fd;
	user_p->flag = flag;

    ret = ion_phys(s_fd, handle, (unsigned long *)phy_add);
    if(ret < 0)
    {
        actal_error("actal_malloc_wt: get phy_addr error!\n");
        return NULL;
    }

    user_p->phy_add = *((long *)phy_add);

	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("actal_malloc_wt: get mutex failed");
        return NULL;
    }

	if(s_top_p == NULL)  //处理头结点，头结点置空
	{
		s_current_p = s_top_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
		s_top_p->fd = 0;
		s_top_p->ptr = NULL;
		s_top_p->map_fd = 0;
		s_top_p->handle = -1;
		s_top_p->len = 0;
		s_top_p->phy_add = 0;
		s_top_p->flag = 0;
	}
	
	s_current_p->next = user_p;
    s_current_p = user_p;
	
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("actal_malloc_wt: free mutex failed");
        return NULL;
    }

    // ALOGD("malloc_wt: ptr = %#X, phy_add = %#X, handle = %x, size = %d\n", (unsigned int)ptr, *phy_add, handle, size);
    return (void *)ptr;
}

void *actal_malloc_wt(int size, void *phy_add)
{
	ALOGI("[memory_malloc] actal_malloc_wt: size: %d", size);
	return actal_malloc_cached_opt(size, phy_add, ION_FLAG_CACHED);
}

void actal_free_wt(void *ptr)
{
    actal_free_uncache(ptr);
}

void *actal_malloc_cached_manual(int size,void *phy_add)
{
	ALOGI("[memory_malloc] actal_malloc_cached_manual: size: %d", size);
	return actal_malloc_cached_opt(size, phy_add, ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC);
}

void actal_free_cached_manual(void *ptr)
{
	actal_cache_flush(ptr);
    actal_free_uncache(ptr);
}

void actal_cache_flush_env(void *ptr)
{
    long ret; 
    long align;
    struct actal_mem *user_p, *parent_p;

	check_pid();
	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("get mutex failed");
        return ;
    }
    user_p = s_top_p;
    while(user_p->next != NULL)
    {
        parent_p = user_p;
        user_p = user_p->next;
		align = (char*)ptr - (char*)user_p->ptr;
		if((user_p->ptr <= ptr) && (align < user_p->len))
        {
			if(user_p->flag & ION_FLAG_CACHED_NEEDS_SYNC)
			{
				ret = ion_cache(user_p->fd, user_p->handle, user_p->map_fd);
			}
			else
			{
				ALOGD("there is no need to flush!\n");
			}
			goto UNLOCK_MUTEX;
        }
    }
    actal_error("cannot find ptr in list_flush\n");
    UNLOCK_MUTEX:
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("free mutex failed");
    }
	return ;
}

void actal_cache_flush(void *ptr)
{
	actal_cache_flush_env(ptr);
}

void actal_cache_env(void *ptr)
{
	actal_cache_flush_env(ptr);
}

void *actal_malloc_uncache(int size,void *phy_add)
{
	int prot = PROT_READ | PROT_WRITE;
	int map_flags = MAP_SHARED;
	ion_user_handle_t handle;
	int map_fd, ret;
    void *ptr;

	ALOGI("[memory_malloc] actal_malloc_uncache: size: %d", size);
	if(size <= 0){
		ALOGE("actal_malloc_uncache: size must be positive\n");
		return NULL; //-EINVAL;
	}

	check_pid();
	// actal_printf_list();
	// actal_error("s_fd = %d\n", s_fd);
    if (size & ALIGN_MASK) {
        //4k对齐
        size += (ALIGN_BYTES - (size & ALIGN_MASK));
    }

    struct actal_mem * user_p;

	user_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
	user_p->next = NULL;
	

	ret = ion_alloc(s_fd, size, 0, 1, 0, &handle);
	if(ret < 0) {
		ALOGE("actal_malloc_uncache: ion_alloc(size: %d) failed(%d)\n", size, ret);
		return NULL;
	}
		
	// ALOGD("handle :%#X\n", handle);

	ret = ion_map(s_fd , handle, size, prot, map_flags, 0, (unsigned char **)&ptr, &map_fd);

	user_p->handle = handle;
    user_p->len = size;
    user_p->fd = s_fd;
	user_p->ptr = ptr;
	user_p->map_fd = map_fd;
	user_p->flag = 0;

    ret = ion_phys(s_fd, handle,(unsigned long*)phy_add);
    if(ret < 0){
        actal_error("actal_malloc_wt: get phy_addr error!\n");
        return NULL;
    }

    user_p->phy_add = *((long*)phy_add);

	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("get mutex failed");
        return NULL;
    }

	if(s_top_p == NULL)  //处理头结点，头结点置空
	{
		s_current_p = s_top_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
		s_top_p->fd = 0;
		s_top_p->ptr = NULL;
		s_top_p->map_fd = 0;
		s_top_p->handle = -1;
		s_top_p->len = 0;
		s_top_p->phy_add = 0;
		s_top_p->flag = 0;
	}
	
	s_current_p->next = user_p;
    s_current_p = user_p;
	
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("actal_malloc_wt: free mutex failed");
        return NULL;
    }

    // ALOGD("malloc_uncache: ptr = %#X, phy_add = %#X, size = %d\n", (unsigned int)ptr, *phy_add, size);
    return (void *)ptr;
}
//
void actal_free_uncache(void *ptr)
{
    int ret;
    struct actal_mem *user_p, *parent_p;
	ALOGI("[memory_free] actal_free_uncache");
	check_pid();
	//避免线程冲突
	if (pthread_mutex_lock(&mutex) != 0)
    {
		ALOGE("actal_free_uncache: get mutex failed");
        return ;
    }
    user_p = s_top_p;
    while(user_p->next != NULL)
    {
        parent_p = user_p;
        user_p = user_p->next;
        if(user_p->ptr == ptr)
        {
            ret = ion_free(user_p->fd, user_p->handle);
            munmap(ptr, user_p->len);
            close(user_p->map_fd);
            // ALOGD("free uncache len = %d, ptr = %#X, handle = %#X\n",user_p->len, (unsigned int)user_p->ptr, (unsigned int)user_p->handle);
            parent_p->next = user_p->next;
            if(user_p == s_current_p)
            {
                s_current_p = parent_p;
            }
            free(user_p);
			goto UNLOCK_MUTEX;
        }
    }
    actal_error("cannot find ptr in list\n");
    UNLOCK_MUTEX:
	if (pthread_mutex_unlock(&mutex) != 0)
    {
		ALOGE("free mutex failed");
    }
	return ;
}
void *actal_malloc_dma(int size, void *phy_add)
{
    actal_error("actal_malloc_dma unsupport now, use uncache instead\n");
    return actal_malloc_uncache(size, phy_add);
}
//
void actal_free_dma(void *ptr)
{
    actal_error("actal_malloc_dma unsupport now, use uncache instead\n");
    actal_free_uncache(ptr);
}
int64_t actal_get_ts() 
{
	return 0;
}
int actal_get_icinfo() 
{
	return 0x03;
}
//
void *actal_memcpy(void *dst, const void *src, int length) 
{
	return memcpy(dst, src, length);
}
//
void *actal_memset(void *dst, int v, int length) 
{
	return memset(dst, v, length);
}
//
void actal_dump(int *address, int len) 
{
	int i = 0;
	for (i = 0; i < len; i++) 
	{
		if ((i % 8) == 0) 
		{
			ALOGI("\n%#08X: ",address);
		}
		ALOGI("%08X ", *address);
		address++;
	}

	ALOGI("\n");
}
//
void actal_sleep_ms(int msec) 
{
	struct timespec tv;

	if (msec <= 0) 
	{
		return;
	}

	tv.tv_sec = msec / 1000;
	tv.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&tv, NULL);
}
static int __attribute__((constructor)) so_init(void) 
{
	s_fd = ion_open();
	s_pid = getpid();
	ALOGD("pid = %d", s_pid);
	return 0;
}
static int __attribute__((destructor)) so_exit(void) 
{
	ion_close(s_fd);
	return 0;
}
