#ifndef __DSI_OWL_H__
#define __DSI_OWL_H__

/*=================================================================*/
/* rgb lcd stuff */
#include <linux/fb.h>
#include <linux/delay.h>

#include <mach/dss-owl.h>
#include <mach/hardware.h>

#if 0
#define DSI_PRINT(fmt, args...) printk(KERN_ALERT fmt, ##args)
#else
#define DSI_PRINT(fmt, args...)
#endif

enum owl_mipi_dsi_data_fmt {
	MIPI_RGB565,
	MIPI_RGB666,
	MIPI_RGB666_LP,
	MIPI_RGB888,
};


struct owl_dsi_data {
	struct owl_videomode *modes;
	u32 num_modes;
};



#endif
