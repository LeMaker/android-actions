#ifndef _ACTVIDEO_DECODER_H_
#define _ACTVIDEO_DECODER_H_

#include "actal_posix_dev.h"
#include <ALdec_plugin.h>


#define FB_FIFO_MAX_BUF_NUM 36 


typedef struct port_s{
    frame_buf_handle *(*get_wbuf)(struct port_s *port,unsigned int buf_size);
    frame_buf_handle *(*try_get_wbuf)(struct port_s *port,unsigned int buf_size);
    void (*put_wbuf)(struct port_s *port,unsigned int size);
    frame_buf_handle *(*get_rbuf)(struct port_s *port);
    void (*put_rbuf)(struct port_s *port);        
    void (*dump_info)(struct port_s *port);
    int (*get_rbuf_num)(struct port_s *port);
        
    void (*put_more_wbuf)(struct port_s *port,unsigned int *buf_addr);
    int (*fifo_init)(struct port_s *port,unsigned int mpool_size,unsigned int max_pkt_size,unsigned int type_flag);
    void (*fifo_reset)(struct port_s *port);
    void (*fifo_wakeup)(struct port_s *port);        
}port_t;    
typedef struct vde_private_handle{
	   int reservd[3];
	   int fd;


    int     magic;
    int     flags;
    int     size;
    int     offset;

    // FIXME: the attributes below should be out-of-line
    int     base;
    int     lockState;
    int     writeOwner;
    int     pid;
    int     width;
    int     height;
    int     format;
 

    // add for get phy addr by hw device
    int phys_addr;
    int ion_handle_t;
	  int     usage;
	
}vde_private_handle_t;

typedef struct buf_ele_s{
    struct buf_ele_s  *next;
    unsigned int type; 
    unsigned int size;
    void *pAppPrivate;
    frame_buf_handle *buf_handle;
}buf_element_t;

typedef struct raw_fifo_s
{
    port_t port;//º¯ÊýÖ¸Õë
    
    unsigned int buf_size;
    unsigned int mpool_size;
    
    buf_element_t *buf_element[FB_FIFO_MAX_BUF_NUM];
    buf_element_t *node_full;
    buf_element_t *node_empty;
    buf_element_t *node_for_display;
    buf_element_t *node_for_decode;
        
    frame_buf_handle *av_buf_full;
    frame_buf_handle *av_buf_empty;
    int cur_num_buffers;
    pthread_mutex_t mutex;
    pthread_cond_t  wbuf_cond;
    
    unsigned char *virt_addr;
    buf_element_t *node_temp;
}raw_fifo_t;

typedef struct fb_fifo_io_s
{
    fb_port_t fb_port;
    
    port_t *port;
    
}fb_fifo_io_t;




fb_port_t *fb_fifo_open(void **fb_vo);
int fb_fifo_dispose(fb_port_t *fb_port);
int raw_fifo_init(port_t *port,int total_buf_num,unsigned int size);
int try_get_wbuf_timeout(av_buf_t **raw_buf_dec, port_t *port, int *suspend_flag,int to_ms);
void raw_fifo_timeout_wakeup(port_t *port,int *suspend_flag);
int raw_fifo_internal_dispose(port_t *port,int is_ANativeWindow_buffer_handle);


#endif
