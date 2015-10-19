
#ifndef _BUF_OP_H
#define _BUF_OP_H

#include <linux/videodev2.h>
#include <pthread.h>
#include "kernel_list.h"
#include "ion.h"


typedef enum {
IO_METHOD_MMAP, IO_METHOD_USERPTR,
} io_method;

struct data_buf{
    struct v4l2_buffer vbuffer;
    struct list_head   list;  //等待执行q操作而获取camera数据的buffer链表
    void *vir_addr;
    int data_length;
    struct ion_handle * p_ion_handle;
};

extern unsigned int BUF_COUNT;
extern io_method io;
extern struct list_head qlist;
extern struct list_head dqlist;

extern int init_buf(unsigned int buffer_size);
extern int uninit_buf(void);
extern unsigned int calc_buffer_size(unsigned int width, unsigned int height, unsigned int format);






#endif //_BUF_OP_H
