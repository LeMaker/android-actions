#ifndef __GL520X_CVBS_H
#define __GL520X_CVBS_H

void print_cvbsreg();
void cvbs_init();
int cvbs_enable();
void cvbs_disable();
extern struct display_ops cvbs_ops;
extern struct fb_videomode1 cvbs_display_mode;

/*enum __owl_tv_mode_t {	
    OWL_TV_MOD_PAL                 = 0,
    OWL_TV_MOD_NTSC                = 1,
   
};
*/


#define OWL_TV_MOD_PAL 0
#define OWL_TV_MOD_NTSC 1

#define CVBS_IN  	1
#define CVBS_OUT	0

struct fb_videomode1 {
	const char *name;	/* optional */
	int refresh;		/* optional */
	int xres;
	int yres;
	int pixclock;
	int left_margin;
	int right_margin;
	int upper_margin;
	int lower_margin;
	int hsync_len;
	int vsync_len;
	int sync;
	int vmode;
	int flag;
	int vid;		/*optional*/
};

struct asoc_videomode{
	int valid;
	struct fb_videomode1 mode;
};

static const struct asoc_videomode cvbs_display_modes[] = {
	[0] = {
	       .valid = 1,
	       .mode = {
			.name = "CVBS-PAL",
			.refresh = 50,
			.xres = 720,
			.yres = 576,
			.pixclock = 74074,	/*pico second, 1.e-12s */
			.left_margin = 48,
			.right_margin = 16,
			.upper_margin = 33,
			.lower_margin = 2,
			.hsync_len = 1,
			.vsync_len = 1,
			.sync = 0,
			.vmode = FB_VMODE_INTERLACED,
			.flag = FB_MODE_IS_STANDARD,
			.vid = OWL_TV_MOD_PAL,
			}
	       },
	[1] = {
	       .valid = 1,
	       .mode = {
			.name = "CVBS-NTSC",
			.refresh = 60,
			.xres = 720,
			.yres = 480,
			.pixclock = 74074,	/*pico second, 1.e-12s */
			.left_margin = 68,
			.right_margin = 12,
			.upper_margin = 39,
			.lower_margin = 5,
			.hsync_len = 1,
			.vsync_len = 1,
			.sync = 0,
			.vmode = FB_VMODE_INTERLACED,
			.flag = FB_MODE_IS_STANDARD,
			.vid = OWL_TV_MOD_NTSC,
			}
	       },
};




#endif