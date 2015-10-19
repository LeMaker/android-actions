

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
#include "video_dev.h"

pthread_t q_thread_id; 
pthread_t dq_thread_id; 
thread_state_t thread_state;
process_t PROCESS_MODE;

int CAPTURE_NUM;
char * CAMERA_TYPE = NULL;
int frame_num;
pthread_mutex_t list_semi;
char *SAVEFILE_PATH = NULL;


int save_frame(struct data_buf *pbuf, unsigned int num)
{
    int fd;
    char path[256];
    int w_byte;

    char fname[256] = "/saveframe";
    strcpy(path, SAVEFILE_PATH);
    strcat(path,fname);
    sprintf(path, "%s_%s%d.yuv", path, CAMERA_TYPE, num);

    fd = open(path, O_RDWR | O_CREAT, 0777);
    if (fd < 0) {
        printf("Create file error!\n");
        return -1;
    }
  //printf("write file vir_addr %x data_length %d \n",pbuf->vir_addr,pbuf->data_length);
    w_byte = write(fd, pbuf->vir_addr, pbuf->data_length);

    if (w_byte != pbuf->data_length)
    {
        printf("error to write byte_write=%d", w_byte);
        return -1;

    }

    close(fd);
    return 0;
}

int process_frame(struct data_buf *pbuf, unsigned int num)
{
    int ret;
    switch(PROCESS_MODE)
    {
        case MODE_DISP:
            ret = disp_frame(pbuf);
            break;
        case MODE_SAVEFILE:
            ret = save_frame(pbuf, num);
            break;
        default:
            printf("PROCESS_MODE error int %s()\n",__FUNCTION__);
    }
    return ret;
}




void * dq_thread_routine()
{
    struct list_head *dqbuf_list;
    int ret;
    dqbuf_list = &dqlist;

    for(;;)
    {

        //printf("%s\n",__FUNCTION__);
        if (list_empty(dqbuf_list))
        {
            //printf("dq_thread_routine sleep\n");
            usleep(15000);
        }
        else
        {
            struct data_buf *pbuffer;
            struct v4l2_buffer *pvbuffer;

            pthread_mutex_lock(&list_semi);
            pbuffer = list_entry(dqbuf_list->next,
                           struct data_buf, list);
            pvbuffer = &(pbuffer->vbuffer);
          pvbuffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          pvbuffer->memory = V4L2_MEMORY_USERPTR;

            ret = ioctl(fd_camera, VIDIOC_DQBUF, pvbuffer);
            if (ret)
            {
                    printf("dq fail\n");
//                fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
                usleep(15000);
            }
            else
            {
                printf("dq success\n");
                frame_num--;
                ret = process_frame(pbuffer, CAPTURE_NUM-frame_num);
                if (ret)
                {printf("process_frame fail errno %d \n",errno);}

                list_del_init(&(pbuffer->list));
                list_add_tail(&(pbuffer->list), &qlist);
                if (!frame_num)
                {
                    printf("frame_num 0 THREAD_STOP\n");
                    thread_state = THREAD_STOP;
                    pthread_mutex_unlock(&list_semi);
                    return NULL;
                }

            }
            pthread_mutex_unlock(&list_semi);
        }
    }
}

void * q_thread_routine()
{
    struct list_head *qbuf_list;
    int ret;
    qbuf_list = &qlist;

    for(;;)
    {
        //printf("%s\n",__FUNCTION__);
        if (list_empty(qbuf_list))
        {
            if (thread_state == THREAD_STOP)
            {
                //printf("q_thread_routine stop\n");
                return NULL;
            }
            //printf("q_thread_routine sleep\n");
            usleep(15000);
        }
        else
        {
            if (thread_state == THREAD_STOP)
            {
                printf("q_thread_routine stop2\n");
                return NULL;
            }

            struct data_buf *pbuffer;
            struct v4l2_buffer *pvbuffer;

            pthread_mutex_lock(&list_semi);
            pbuffer = list_entry(qbuf_list->next,
                           struct data_buf, list);
            pvbuffer = &(pbuffer->vbuffer);
          pvbuffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          pvbuffer->memory = V4L2_MEMORY_USERPTR;

            ret = ioctl(fd_camera, VIDIOC_QBUF, pvbuffer);
            if (ret)
            {
                printf("qbuf fail\n");
            }
            else
            {

                printf("qbuf ok\n");
                list_del_init(&(pbuffer->list));
                list_add_tail(&(pbuffer->list), &dqlist);
            }
            pthread_mutex_unlock(&list_semi);
        }
    }
}


















