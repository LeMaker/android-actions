#ifndef __MISC_INFO_H_
#define __MISC_INFO_H_



#ifdef OS_LINUX
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#define PRINT(x...)                     	printk(x)
#define PRINT_DBG(x...)						if(debug_enable)printk(x)
#define PRINT_ERR(x...)						printk(KERN_ERR x)
#define INFO(x...)
#define MALLOC(x)                       	kmalloc(x, GFP_KERNEL)
#define FREE(x)                         	kfree(x)

#else
#define PRINT(x...)                     	printf(x)
#define PRINT_DBG(x...)						if(debug_enable)printf(x)
#define PRINT_ERR(x...)						printf(x)
#define INFO(x...)
#define MALLOC(x)                       	malloc(x)
#define FREE(x)                         	free(x)
#endif

extern void *malloc(unsigned int size);
extern void free(void *ptr);
extern int debug_enable;

#define MISC_INFO_MAGIC          0x55aa55aa
#define MISC_INFO_OFFSET         4*1024*1024
#define MISC_INFO_MAX_SIZE       1024*1024
#define MISC_INFO_MAX_ITEM_NUM   16
#define MISC_INFO_HEAD_ALIGN     1024

#define GET_ITEM_DATA            0
#define SET_ITEM_DATA            1
#define FORMAT_MISC_INFO         2

typedef struct
{
  unsigned char          name[8];
  unsigned int           size;
  unsigned char          *data; 
}ioctl_item_t;

typedef struct
{
  unsigned int           magic;
  unsigned char          name[8];
  unsigned int           size;
  unsigned char          data[]; 
}packet_item_t;

typedef struct
{
    unsigned int      length;//packet length
    packet_item_t     item[];
}usb_packet_t;


typedef struct
{
	unsigned char  name[8];
	unsigned int   magic;
//	unsigned short rw_attr;
//	unsigned short key;
	unsigned short size;		//size of item data
	unsigned short offset;		//offset from the beginning
	unsigned short chk_sum;
	unsigned char  reserved[16];
}__attribute__((aligned(4)))item_head_t;

typedef struct
{    
    unsigned int   magic;
//	unsigned short key;
	unsigned short length;		//total length of misc info
	unsigned short item_num;	
	unsigned char  reserved[8];
	item_head_t item_head[MISC_INFO_MAX_ITEM_NUM];
}__attribute__((aligned(MISC_INFO_HEAD_ALIGN)))misc_info_head_t;

typedef struct
{    
    misc_info_head_t head;
    unsigned char *data;
}misc_info_t;



/*
 * get checksum on the base of 2 bytes
 */
static unsigned short get_checksum(unsigned short *buf,unsigned int len)
{
    unsigned int loop;
    unsigned short sum = 0;
	
	if(!buf || len == 0)
		return 0;
	
    for(loop = 0; loop <len; loop++){
        sum += buf[loop];
    }
    sum ^= (unsigned short)0x55aa;

    return sum;
}

static void dump_mem(void *buf, unsigned int start, unsigned int size)
{
	unsigned char *ptr;
    int i;

	if(!buf){
		PRINT_ERR("%s, buf is null\n", __FUNCTION__);
		return;
	}
	
    ptr = (unsigned char *)buf + start;
    for(i = 0; i < size; i++){
        if(i % 16 == 0)
            PRINT("%d: ", start + i);
        PRINT("%.2x ", *ptr++);
        if(i % 16 == 15)
            PRINT("\n");
    }
    PRINT("\n");
}

#endif