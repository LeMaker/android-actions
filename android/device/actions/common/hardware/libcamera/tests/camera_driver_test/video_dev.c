
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <asm-generic/errno-base.h>

#include "display.h"
#include "device_op.h"
#include "buf_op.h"
#include "kernel_list.h"



int fd_video = -1;
unsigned int SCREEN_WITDH;
unsigned int SCREEN_HEIGHT;

char *DEV_VIDEO_NAME = NULL;
LIST_HEAD(vlist);



static int init_mmap()
{
    struct list_head *p;
    struct data_buf *dbuf;
    struct v4l2_buffer *vbuf;
    int i=0;
    list_for_each(p, &vlist)
    {
        dbuf = container_of(p, struct data_buf, list);
        vbuf = &(dbuf->vbuffer);
        vbuf->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        vbuf->memory = V4L2_MEMORY_MMAP;
        vbuf->index = i;
        if (-1 == ioctl(fd_video, VIDIOC_QUERYBUF, vbuf)){
            printf("VIDIOC_QUERYBUF called error \n");
            return -1;
        }
        dbuf->data_length = vbuf->length;
        dbuf->vir_addr = mmap(NULL, vbuf->length, PROT_READ
                | PROT_WRITE, MAP_SHARED, fd_video, vbuf->m.offset);
        if (MAP_FAILED == dbuf->vir_addr)
            return -1;
    }

    return 0;
}


int init_userp(void)
{

    struct list_head *p;
    struct data_buf *dbuf;
    struct v4l2_buffer *vbuf;
    int i=0;
    list_for_each(p, &vlist)
    {

        dbuf = container_of(p, struct data_buf, list);
        vbuf = &(dbuf->vbuffer);
        vbuf->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        vbuf->memory = V4L2_MEMORY_USERPTR;
        vbuf->index = i;
        vbuf->field = V4L2_FIELD_NONE;
        vbuf->flags = 0;
        printf("vbuf type %d\n",vbuf->type);

    }
    return 0;

}

int video_stream_on()
{
    unsigned int type;
    int ret=0;
    // stream on
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    ret = ioctl(fd_video, VIDIOC_STREAMON, &type);
    if (ret)
    {
        printf("VIDIOC_STREAMON ERROR ,Error Code = %d\n",ret);
        return ret;
    }
    return 0;
}


int video_stream_off()
{
    unsigned int type;
    int ret=0;
    // stream off
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    ret = ioctl(fd_video, VIDIOC_STREAMOFF, &type);
    if (ret)
    {
        printf("VIDIOC_STREAMOFF ERROR ,Error Code = %d\n",ret);
        return ret;
    }
    return 0;
}



//设置输出视频分辨率大小以及截取视频大小，默认为整个视频全部输出
int set_video_format(void)
{
    int ret=0;
    struct v4l2_format format;
    struct v4l2_crop crop;

    // get format
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    ret = ioctl(fd_video, VIDIOC_G_FMT, &format);
    if (ret < 0)
    {
        printf("VIDIOC_G_FMT ERROR\n");
        return ret;
    }

    format.fmt.pix.width = framesize_array[FRAME_SIZE_ID].discrete.width;
    format.fmt.pix.height = framesize_array[FRAME_SIZE_ID].discrete.height;
    format.fmt.pix.pixelformat = FRAME_FORMAT;


    ret = ioctl(fd_video, VIDIOC_S_FMT, &format);
    if (ret < 0)
    {
        printf("VIDIOC_S_FMT ERROR V4L2_BUF_TYPE_VIDEO_OUTPUT ,Error Code = %d\n",ret);
        return ret;
    }
    // get crop
    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ret = ioctl(fd_video, VIDIOC_G_CROP, &crop);
    if(ret<0)
    {
        printf("get crop error!!!!ret =%d \n",ret);
        return 0;
    }

    //set crop  从视频源截取区域
    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    crop.c.left = 0;
    crop.c.top = 0;
    crop.c.width = format.fmt.pix.width;
    crop.c.height = format.fmt.pix.height;

    ret = ioctl(fd_video, VIDIOC_S_CROP, &crop);
    if (ret < 0) {
      printf("VIDIOC_S_CROP ERROR ,Error Code = %d\n",ret);
      return ret;
    }
    return 0;
}



int init_device_video(void)
{
    int ret=0;
    struct v4l2_format format;
    struct v4l2_requestbuffers reqbuf;

    fd_video=open(DEV_VIDEO_NAME,O_RDWR);
    if(-1==fd_video)
    {
        printf("open video dev error!!!\n");
        return -1;
    }

    // get format
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ret = ioctl(fd_video, VIDIOC_G_FMT, &format);
    if (ret < 0)
    {
      printf("VIDIOC_G_FMT ERROR\n");
      return ret;
    }
    //set overlay format
    format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    format.fmt.win.w.left = 0;
    format.fmt.win.w.top = 0;
    format.fmt.win.w.width = SCREEN_WITDH;  //待修改
    format.fmt.win.w.height = SCREEN_HEIGHT;

    ret = ioctl(fd_video, VIDIOC_S_FMT, &format);
    if (ret < 0)
    {
        printf("VIDIOC_S_FMT  ERROR ,Error Code = %d\n",ret);
        return ret;
    }

  int i;
    struct data_buf *p;
    for(i = 0; i<(int)BUF_COUNT; i++)
    {
        p = calloc(1,sizeof(struct data_buf));
        if (!p) {
        printf("Out of memory\n");
        return -ENOMEM;
        }
        list_add_tail(&(p->list), &vlist);
    }



    // request buffers
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    reqbuf.count = BUF_COUNT;
    switch(io)
    {
        case IO_METHOD_MMAP:
            reqbuf.memory = V4L2_MEMORY_MMAP;
            ret = ioctl(fd_video, VIDIOC_REQBUFS, &reqbuf);
            if (ret < 0) {
              printf("reqbuf ioctl ERROR\n");
              return ret;
            }
            init_mmap();
            break;
        case IO_METHOD_USERPTR:
            reqbuf.memory = V4L2_MEMORY_USERPTR;
            ret = ioctl(fd_video, VIDIOC_REQBUFS, &reqbuf);
            if (ret < 0) {
              printf("reqbuf ioctl ERROR\n");
              return ret;
            }
            init_userp();
            break;
        default:
            printf("io type error in %s()\n",__FUNCTION__);
            return -1;
    }

  
  ret = set_video_format();
  if(ret)
    return ret;
  
  ret = video_stream_on();
  if(ret)
    return ret;


    return 0;
}

int uninit_device_video(void)
{
    int ret=0;

    ret = video_stream_off();
    if (ret)
    {return ret;}

    if(-1!=fd_video)
    {
        close(fd_video);
    }
    return 0;
}







static int disp_userp(struct data_buf *pbuf)
{
    //printf("%s %d\n",__FUNCTION__,__LINE__);
    struct data_buf *pbuffer;
    struct v4l2_buffer *pvbuffer;
    struct data_buf *pbuffer_in = pbuf;
    struct v4l2_buffer *pvbuffer_in =&(pbuffer_in->vbuffer);

    int ret;
    static int dqenable = 0;
    pbuffer = list_entry(vlist.next,
                           struct data_buf, list);
    pvbuffer = &(pbuffer->vbuffer);
    pvbuffer->index = pvbuffer_in->index;
  pvbuffer->length = pvbuffer_in->length;
  pvbuffer->m.userptr = pvbuffer_in->m.userptr;
     
//  printf("vir_addr %x \n",pbuffer->vir_addr);
//  printf("phyaddr %x \n",pvbuffer->m.userptr);
//    printf("index %d \n",pvbuffer->index);
//    printf("type %d \n",pvbuffer->type);
//  printf("memory %d \n",pvbuffer->memory);

    ret = ioctl(fd_video, VIDIOC_QBUF, pvbuffer);
    if (ret)
    {
        printf("VIDIOC_QBUF,Error in %s()\n",__FUNCTION__);
        return ret;
    }
    if (!dqenable)
    {
        dqenable = 1;
        return 0;
    }
    else
    {
        struct v4l2_buffer buf;
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_USERPTR;
        ret = ioctl(fd_video, VIDIOC_DQBUF, &buf);
        if(ret)
        {
            printf("VIDIOC_DQBUF,Error in %s()\n",__FUNCTION__);
            return ret;
        }
        list_del_init(&(pbuffer->list));
        list_add_tail(&(pbuffer->list), &vlist);
        return 0;
    }
}


int disp_frame(struct data_buf *pbuf)
{
    //printf("%s %d\n",__FUNCTION__,__LINE__);
    int ret;
    switch(io)
    {
        case IO_METHOD_MMAP:
            break;
        case IO_METHOD_USERPTR:
            ret = disp_userp(pbuf);
            if (ret)
            {
                return ret;
            }
            break;
        default:
            printf("err in %s()",__FUNCTION__);
    }

    return 0;

}

















