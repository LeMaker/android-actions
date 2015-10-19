#ifndef __DSS_USER_OWL_H
#define __DSS_USER_OWL_H

/*owl fb private stuff*****/

/****used in struct fb_var_screeninfo.vmode**********************************/

/*don't touch displayers, adjust layer only*/
/*this option will be cleared after been set,*/
/*so if you want to use it, set it everytime*/
#define FB_VMODE_LAYER_ONLY	1024


/***********************************************************************/
enum owl_pixel_format {
	MONO_1 = 1 << 0,
	MONO_2 = 1 << 1,
	MONO_4 = 1 << 2,
	MONO_8 = 1 << 3,

	PALETTE_1 = 1 << 4,
	PALETTE_2 = 1 << 5,
	PALETTE_4 = 1 << 6,
	PALETTE_8 = 1 << 7,

	RGB_565 = 1 << 8,
	ARGB_1555 = 1 << 9,
	ARGB_8888 = 1 << 10,
	ABGR_8888 = 1 << 11,
	XRGB_8888 = 1 << 12,
	XBGR_8888 = 1 << 13,

	YUV_420_PLANAR = 1 << 14,
	YUV_422_INTERLEAVED = 1 << 15,
	YUV_420_SEMI_PLANAR = 1 << 16,
	YUV_SEMI_PLANAR_COMPRESS = 1 << 17,
};

struct owl_fb_buffer {
	unsigned int paddr;
	void *vaddr;

	unsigned long size;

	enum owl_pixel_format pixfmt;

	unsigned short width;
	unsigned short height;

	/* input information*/
	unsigned short xoff;
	unsigned short yoff;
	unsigned short input_width;
	unsigned short input_height;

	/* output information*/
	unsigned short xcor;
	unsigned short ycor;
	unsigned short output_width;
	unsigned short output_height;

	/*see FB_ROTATE_*/
	unsigned int rotate;
};

#define MAX_OVERLAY_NUM	4

/**************/

struct owl_fb_dss_comp_data {
	int num_buffers;
	struct owl_fb_buffer bufs[MAX_OVERLAY_NUM];

	int is_flip;
};

enum owl_fb_mem_mode {
	OWL_FB_MEM_FRAMEBUFFER	= 0,
	OWL_FB_MEM_USER,
};

typedef void (*owl_dc_complete_t) (void *);

struct fb_info;

extern int owl_fb_switch_mem_mode(struct fb_info *fbi,
	enum owl_fb_mem_mode mode,
	int ovl_ids[], int num_ovls);

extern int owl_fb_dc_queue(struct fb_info *fbi,
	struct owl_fb_dss_comp_data *buf_data,
	owl_dc_complete_t func,
	void *arg);

#endif
