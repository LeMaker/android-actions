

#ifndef __OWL_DSI_H
#define __OWL_DSI_H

#include <linux/platform_device.h>
#include <video/owldss.h>

	#define DSI_PRINTa
	#ifdef DSI_PRINT
	#define DEBUG_DSI(format, ...) \
		do { \
			printk(KERN_ERR "OWL_DSI: " format, ## __VA_ARGS__); \
		} while (0)
	#else
	#define DEBUG_DSI(format, ...)
	#endif

	/*dsihw.c*/
	void dsihw_fs_dump_regs(struct device *dev);
	void test_fs_dsi(struct device *dev);
	void test_fs_longcmd(struct device *dev);

	void dsihw_send_long_packet(struct platform_device *pdev, int data_type, int word_cnt, int * send_data, int trans_mode);
	void dsihw_send_short_packet(struct platform_device *pdev,int data_type, int sp_data, int trans_mode);
	void owl_dsi_select_video_timings(struct owl_dss_device *dssdev, u32 num,
                                   struct owl_video_timings *timings);
	/*dsi_sysfs.c*/
	int owl_dsi_create_sysfs(struct device *dev);

	/*dsi_cmd.c*/
	void send_cmd(struct platform_device *pdev);
	void send_cmd_test(struct platform_device *pdev);
	
#endif 
