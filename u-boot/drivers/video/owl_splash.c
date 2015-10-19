#include <common.h>
#include <malloc.h>
#include <version.h>
#include <nand.h>
#include <mmc.h>
#include <fat.h>
#include <fs.h>
#include <linux/compiler.h>

#include <video.h>
#include <lcd.h>

#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <owl_dss.h>
#include <asm/arch/owl_afi.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

static  char *splash_image_select(void)
{
	if(gd->flags & GD_FLG_RECOVERY  ) {
		return CONFIG_SYS_RECOVERY_LOGO_NAME;
	} else if(gd->flags & GD_FLG_CHARGER ) {	
		return CONFIG_SYS_CHARGER_LOGO_NAME;
	} else if(gd->flags & GD_FLG_LOWPOWER) {
		return CONFIG_SYS_BATTERY_LOW_NAME;
	} else {	
		return CONFIG_SYS_VIDEO_LOGO_NAME;
	}
}

static int splash_image_load(void)
{
	int ret;
	char *filename;
	void *splash_image_addr;
	char splash_image_char[16];
	
	splash_image_addr = memalign(128, CONFIG_SYS_VIDEO_LOGO_MAX_SIZE);
	if(splash_image_addr == NULL) {
		printk("Malloc size for splash image failed!\n");
		return -1;
	}

	filename = splash_image_select();
	if (!filename) {
		printk("No splash image loaded\n");
		return -1;
	}
	
	ret = file_fat_read(filename, splash_image_addr, CONFIG_SYS_VIDEO_LOGO_MAX_SIZE);

	if(ret < 0) {
		printk("Fail to load splash image\n");
		free(splash_image_addr);
		return -1;
	}	
	sprintf(splash_image_char, "%x", (unsigned int) splash_image_addr);
	setenv("splashimage", splash_image_char);
    return 0;
}

static int lcd_gamma_load(void)
{
	int ret;
	char gamma_char[16];
	struct gamma_info *gamma_info;

	gamma_info = &kinfo->gamma;
	ret = file_fat_read(CONFIG_SYS_GAMMA_NAME, gamma_info->gamma_table,
			CONFIG_SYS_GAMMA_SIZE);
	if(ret < 0) {
		printk("Fail to load gamma\n");
		gamma_info->is_valid = 0;
		return -1;
	}

	gamma_info->is_valid = 1;
	sprintf(gamma_char, "%x", (unsigned int)gamma_info);
	setenv("lcdgamma", gamma_char);
	return 0;
}


int splash_image_init(void)
{
	int ret, dev, part;
	block_dev_desc_t *dev_desc;
	
	if(owl_get_boot_mode() == BOOT_MODE_PRODUCE)
		return 0;
	
	char *s = getenv("devif");
	char *spart = getenv("bootpart");
	dev = get_boot_dev_num();
	part = (int)((char)spart[0] - '0');
   
	dev_desc = get_dev(s, dev);
	if (dev_desc == NULL) {
		printf("Failed to find %s:%d\n", s, dev);
		return 0;
	}
	printf("bootlogo: if=%s, dev=%d\n",s, dev);

	ret = fat_register_device(dev_desc, part);
	if (ret) {
		printf("Failed to register %s:\n",s);
		return 0;
	}

	ret = splash_image_load();
	ret = lcd_gamma_load();

	return 0;
}