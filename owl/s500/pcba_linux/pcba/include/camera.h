#include<stdio.h>
#include <linux/videodev2.h>
#include <directfb.h>
#include <fcntl.h>

#define BUF_COUNT 6  // 4

typedef struct {
    unsigned char *buf;
    unsigned int width;        //高
    unsigned int height;         //宽
    unsigned int format;         //数据格式，虽然与open接口处的format重复，但还是保留
    unsigned int stride;
    unsigned int stride_h;
}video_frame_t;
	 
struct buffer
{
	void *start;
	unsigned long phy;
	int length;
};

extern IDirectFBSurface *windowsurface;
extern IDirectFBSurface *camera_source;
extern void *actal_malloc_wt(int32_t, void *);
extern void actal_free_wt(void *);

int open_camera_device(int, int);
int init_camera_device(int *, int *);
void camera_start_capturing();
void camera_mainloop();
void camera_stop_capturing();
void uninit_camera_device();
void close_camera_device();