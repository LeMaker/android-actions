/*
 *         camera buffer operate
 *  mainly about qbuf dqbuf.
 *
 *
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm-generic/errno-base.h>
 
#include "buf_op.h"
#include "device_op.h"




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>        /* getopt_long() */
#include <fcntl.h>        /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>        /* for videodev2.h */
#include <linux/videodev2.h>
#include <semaphore.h>

#include "display.h"
#include "device_op.h"
#include "buf_op.h"
#include "kernel_list.h"



unsigned int BUF_COUNT;
unsigned int CAPTURE_FRAME_SIZE_ID, PREVIEW_FRAME_SIZE_ID;

io_method io = IO_METHOD_USERPTR;    //IO_METHOD_READ;//IO_METHOD_MMAP;

LIST_HEAD(qlist);
LIST_HEAD(dqlist);

unsigned int calc_buffer_size(unsigned int width, unsigned int height, unsigned int format)
{
    unsigned int size;
    switch(format)
    {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_NV12:
        size = width*height*3/2;
        break;
        default:
        printf("buffer format error\n");
    }
    return size;
}

static void init_mmap(void)
{
//    struct v4l2_requestbuffers req;
//    int ret;
//    //CLEAR(req);
//    req.count = BUF_COUNT;
//    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    req.memory = V4L2_MEMORY_MMAP;
//    if (ret = ioctl(fd_camera, VIDIOC_REQBUFS, &req))
//    {
//        return ret;
//    }
//    if (req.count < 2) {
//        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
//        return -ENOBUFS;
//    }
//    buffers = calloc(req.count, sizeof(*buffers));
//    if (!buffers) {
//        fprintf(stderr, "Out of memory\n");
//        exit(EXIT_FAILURE);
//    }
//    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
//        struct v4l2_buffer buf;
//        CLEAR(buf);
//        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        buf.memory = V4L2_MEMORY_MMAP;
//        buf.index = n_buffers;
//        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
//            errno_exit("VIDIOC_QUERYBUF");
//        buffers[n_buffers].length = buf.length;
//        buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
//                        buf.length,
//                        PROT_READ | PROT_WRITE
//                        /* required */ ,
//                        MAP_SHARED /* recommended */ ,
//                        fd, buf.m.offset);
//        if (MAP_FAILED == buffers[n_buffers].start)
//            errno_exit("mmap");
//    }
}

static int init_userp(unsigned int bsize)
{
    struct data_buf *pbuffer;
    struct v4l2_buffer *pvbuffer;
    unsigned int buffer_size;
    unsigned int page_size;
    struct v4l2_requestbuffers req;
    int ret;
    int i = 0;
    buffer_size = bsize;
    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

  req.count  = BUF_COUNT;
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  ret = ioctl(fd_camera, VIDIOC_REQBUFS, &req);
  if(ret)
  return ret;

    do{
            pbuffer = list_entry(qlist.next,
                           struct data_buf, list);
            pbuffer->data_length = bsize;
            pvbuffer = &(pbuffer->vbuffer);
            pvbuffer->index = i;
          pvbuffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          pvbuffer->memory = V4L2_MEMORY_USERPTR;
          pvbuffer->length = buffer_size;
            ret = sys_mem_allocate(pvbuffer->length, &(pbuffer->vir_addr), &(pbuffer->p_ion_handle));
            if (ret)
            {
                break;
            }
          printf("vir_addr %p \n",pbuffer->vir_addr);
          ret = sys_get_phyaddr(&(pvbuffer->m.userptr), pbuffer->p_ion_handle);
          if (ret)
            {
                break;
            }
          printf("phyaddr %x \n",(unsigned int)(pvbuffer->m.userptr));

            printf("index %d \n",pvbuffer->index);

            ret = ioctl(fd_camera, VIDIOC_QBUF, pvbuffer);
            if (ret)
            {
                printf("VIDIOC_QBUF in %s() fail\n",__FUNCTION__);
                break;
            }
            list_del_init(&(pbuffer->list));
            list_add_tail(&(pbuffer->list), &dqlist);
            i++;
    }while(!list_empty(&qlist));
    return ret;
}

int uninit_userp(void)
{
    int ret;
    while (!list_empty(&dqlist))
    {
            struct data_buf *pbuffer;

            pbuffer = list_entry(dqlist.next,
                           struct data_buf, list);
            if ((ret = sys_mem_free(pbuffer->p_ion_handle)))
                  return ret;

            list_del_init(&(pbuffer->list));
    }
    return 0;
}

int init_buf(unsigned int buffer_size)
{
    int i;
    struct data_buf *p;
    int ret = 0;
    for(i = 0; i<(int)(BUF_COUNT); i++)
    {
        p = calloc(1,sizeof(struct data_buf));
        if (!p) {
        printf("Out of memory\n");
        return -ENOMEM;
        }
        list_add_tail(&(p->list), &qlist);
    }
    switch (io)
    {
        case IO_METHOD_MMAP:
            init_mmap();
            break;
        case IO_METHOD_USERPTR:
            ret = init_userp(buffer_size);
            break;
    }
    return ret;
}

int uninit_buf(void)
{
    int ret;
    switch (io)
    {
        case IO_METHOD_MMAP:
            //init_mmap();
            break;
        case IO_METHOD_USERPTR:
            ret = uninit_userp();
            break;
    }
    return ret;
}


 
