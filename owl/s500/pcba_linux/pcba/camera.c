#include "camera.h"
struct buffer *buffers = NULL;
static int fd = -1;
static unsigned int n_buffers = 0;
static unsigned int iframecount = 0;
static int frame_count = 50;
static char dev_name[20] = "/dev/video0";

static int camera_height = 0;
static int camera_width = 0;

extern int camera_pixel_width;
extern int camera_pixel_height;

void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

int xioctl(int fd, int request, void *arg)
{
	int r;
	do
		r = ioctl(fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

int open_camera_device(int camera_dev_num, int camera_num)
{
	struct stat st;
	dev_name[10] = '0' + camera_dev_num;
	while (-1 == stat(dev_name, &st)) //获取文件信息
	{
		// printf("wait camera plugin\n");
		return -1;
		// direct_thread_sleep( 1000000 );
	}
	if (!S_ISCHR(st.st_mode)) //判断文件是否是文件夹
	{
		printf("dev file is not a device\n");
		return -1;
	}
	
	fd = open(dev_name, O_RDWR, 0);
	if (-1 == fd)
	{
		printf("open camera device error !! errno = %d\n", errno);
		return -1;
	}
	int flags = fcntl(fd, F_GETFD);
	flags |= FD_CLOEXEC;
	fcntl(fd, F_SETFD, flags);
	//printf("camera fd = %d\n", fd);
	
	if(camera_num <= 0)
		camera_num = 50;
	frame_count = camera_num;
	
	windowsurface->GetSize(windowsurface, &camera_width, &camera_height);
	camera_height /= 2;
	//printf("camera size = %dx%d\n", camera_width, camera_height);
	return 0;
}

static void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;
	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
	printf("init _userptr\n");

	memset(&req, 0, sizeof(req));
	req.count = BUF_COUNT;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				"user pointer i/o\n", dev_name);
			return;
		} else {
			errno_exit("VIDIOC_REQBUFS");
			return;
		}
	}
	printf("req.count=%d\n",req.count);
	buffers = calloc(BUF_COUNT, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return;
	}
	
	for (n_buffers = 0; n_buffers < BUF_COUNT; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
  
		buffers[n_buffers].start = 
					actal_malloc_wt(buffers[n_buffers].length, &buffers[n_buffers].phy);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			return;
		}

		memset(buffers[n_buffers].start, 0xaa,
		       buffers[n_buffers].length);
	}
}

static void v4l2_s_ctrl(unsigned int id, unsigned int value)
{
	int i, ret;
	struct v4l2_control ctrl;  
	printf("set ctr[%x] = %d\n", id, value);  
	ctrl.id = id;    
	ctrl.value = value;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) 
	{        
		printf("error:v4l2_s_ctrl error id:%d, value:%d \n",ctrl.id, ctrl.value);        
		return;    
	}
	
	if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0)
	{    
		perror("get ctrl failed");    
		ctrl.value = -999;  
	}  
	printf("ctr[%x] value = %d\n", id, ctrl.value);
}

int init_camera_device(int *width, int *height)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min = 0;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) //获取设备功能
	{
		printf("not a V4L2 device!!!\n");
		return -1;
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))//判断是否为采集设备
	{
		printf("not a capture device \n");
		return -1;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) //判断是否为流设备
	{
		printf("not support streaming i/o");
		return -1;
	}

	/////set the format

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = *width;
	fmt.fmt.pix.height = *height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;//V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_NONE; //V4L2_FIELD_INTERLACED; 
	
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");
	printf("=====will set fmt to (%d, %d)--", fmt.fmt.pix.width,fmt.fmt.pix.height); 

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))//设置视频格式
	{
		printf("VIDIOC_S_FMT error!\n");
		return -1;
	}

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	//printf("fmt.fmt.pix.bytesperline = %d\n", fmt.fmt.pix.bytesperline);
	//printf("fmt.fmt.pix.sizeimage = %d\n", fmt.fmt.pix.sizeimage);
 	printf("=====after set fmt\n");  

    printf("    fmt.fmt.pix.width = %d\n", fmt.fmt.pix.width);  

    printf("    fmt.fmt.pix.height = %d\n", fmt.fmt.pix.height);  

    printf("    fmt.fmt.pix.sizeimage = %d\n", fmt.fmt.pix.sizeimage);  

    printf("    fmt.fmt.pix.bytesperline = %d\n", fmt.fmt.pix.bytesperline);  

    printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");  

    printf("\n");  
	
	min = fmt.fmt.pix.width * 3 / 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	//////not all capture support crop!!!!!!!
    printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");  

	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
	{
		printf("----->has ability to crop!!\n");  
		printf("cropcap.defrect = (%d, %d, %d, %d)\n", cropcap.defrect.left,  
                cropcap.defrect.top, cropcap.defrect.width,  
                cropcap.defrect.height); 
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c.left = 0;
		crop.c.top = 0;
		crop.c.width = *width;
		crop.c.height = *height;
		//		crop.c = cropcap.defrect;
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
		{
			switch (errno)
			{
			case EINVAL:
				/* Cropping not supported. */
				printf("Cropping not supported.");
				break;
			default:
				/* Errors ignored. */
				break;
			}
			printf("-----!!but crop to (%d, %d, %d, %d) Failed!!\n",
					crop.c.left, crop.c.top, crop.c.width, crop.c.height);
		}
		else
		{
			 printf("----->sussess crop to (%d, %d, %d, %d)\n", crop.c.left,  
                    crop.c.top, crop.c.width, crop.c.height);  
			if (-1 == xioctl(fd, VIDIOC_G_CROP, &crop))
			{
				printf("VIDIOC_G_CROP error!!\n");
			}
			else
			{
				//printf("----->VIDIOC_G_CROP (%d, %d, %d, %d)\n", crop.c.left,
					//	crop.c.top, crop.c.width, crop.c.height);
				*width = crop.c.width;
				*height = crop.c.height;
			}
		}
	}
	printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n"); 

	/////crop finished!
	init_userp(fmt.fmt.pix.sizeimage);
	iframecount = 0;

#ifdef __GS900A__
	v4l2_s_ctrl(V4L2_CID_EXPOSURE, 3000);
	v4l2_s_ctrl(V4L2_CID_GAIN, 1000);
#endif
	return 0;
}

void  camera_start_capturing() 
{  

	unsigned int i;  
	enum v4l2_buf_type type; 
	
	for (i = 0; i < n_buffers; ++i)
	{
		struct v4l2_buffer buf;
		memset(&buf,0,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;
	//			buf.m.userptr = (unsigned long)buffers[i].start;
		buf.m.userptr = (unsigned long)buffers[i].phy;
		buf.length = buffers[i].length;

		if (0xFFFFFFFFUL == buf.m.userptr) {
			errno_exit("sys_get_phyaddr");
		}

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
	{
		printf("stream on error\n");
		return;
	}
	else
	{
		//printf("camera stream on success!!\n");
	}
}

int read_frame()
{
	//todo
	DFBRectangle camera_rect = {0, 0, camera_width, camera_height};
	void *surface_data;
    int surface_pitch;
	struct v4l2_buffer buf;
	unsigned int i = 0;
	int ret = -1;
	int buf_len;

	buf_len = camera_pixel_width * camera_pixel_height * 3 / 2;
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	while (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
	{
		printf("dqbuf error\n");
	}

	// printf("camera dqbuf success!\n");

	if(buf.index >= n_buffers)
	{
		printf("error i < nbuffer\n");
		return -1;
	}

	//printf("buf.lenght = %d , buffers = %d\n", buf.length, buffers[buf.index].length);
	//lock surface and update 
	ret = camera_source->Lock( camera_source, DSLF_WRITE, &surface_data, &surface_pitch );
	if(buf.length > buf_len)
	{
		buf.length = buf_len;
	}
	memcpy(surface_data, buffers[buf.index].start, buf.length);

	ret = camera_source->Unlock( camera_source );
	windowsurface->Blit(windowsurface, camera_source, &camera_rect, 0, 0);
	windowsurface->Flip(windowsurface, NULL, 0);
	iframecount++;
	if (iframecount > 2)
	{
		iframecount--;
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		{
			printf("qbuf error\n");
			return 0;
		}
		// printf("camera qbuf success\n");
	}

	return 1;
}

void camera_mainloop()
{
	while (frame_count-- > 0)
	{
		
		for (;;) {	

			fd_set fds;  

			int r;	

			FD_ZERO(&fds);	

			FD_SET(fd, &fds);  

			r = select(fd + 1, &fds, NULL, NULL, NULL);	

			if (-1 == r) {	

				if (EINTR == errno)  

					continue;  

				errno_exit("select");  

			}  
			if (read_frame())  
			    break;  
		}
		//printf("start read_frame,frame_count=%d\n",frame_count);
		//read_frame();
	}
}

void camera_stop_capturing()
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
	{
		printf("stream off error\n");
		return;
	}
}

void uninit_camera_device()
{
	unsigned int i = 0;
	for (i = 0; i < n_buffers; ++i)
		actal_free_wt(buffers[i].start);
	free(buffers);
}

void close_camera_device()
{
	if (-1 == close(fd))
	{
		printf("close device error\n");
		return;
	}
	fd = -1;
}
