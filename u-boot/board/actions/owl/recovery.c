/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <netdev.h>
#include <linux/input.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <search.h>
#include <errno.h>
#include <ext4fs.h>
#include <mmc.h>
#include <asm/arch/pmu.h>
#include <asm/arch/sys_proto.h>
#include <fat.h>
#include <fs.h>

DECLARE_GLOBAL_DATA_PTR;

#define RECOVERY_PART_NUM                 "2"
#define EXT4_CACHE_PART                   6
#define CONFIG_ANDROID_RECOVERY_CMD_FILE  "recovery/command"
#define CONFIG_RECOVERYFILE_SIZE	      1024

static int check_recovery_cmd_file(void)
{
	static disk_partition_t info;
	char buf[CONFIG_RECOVERYFILE_SIZE];
	int err;
	block_dev_desc_t *dev_desc = NULL;
	int dev = 0;
	int part = EXT4_CACHE_PART;
	loff_t filelen;
	const char *ifname;

	debug("check_recovery_cmd_file\n");
	ifname = getenv("devif");
	if ( ifname == NULL) {
		ifname = "nand";
		printf("get devif fail\n");
	}
    dev = get_boot_dev_num();

	dev_desc = get_dev(ifname, dev);
	if (dev_desc == NULL) {
		printf("Failed to find %s%d\n", ifname, dev);
		return 1;
	}

	debug("part = %d\n", part);

	if (get_partition_info(dev_desc, part, &info)) {
		printf("** get_partition_info %s%d:%d\n",
				ifname, dev, part);

		if (part != 0) {
			printf("** Partition %d not valid on device %d **\n",
					part, dev_desc->dev);
			return -1;
		}

		info.start = 0;
		info.size = dev_desc->lba;
		info.blksz = dev_desc->blksz;
		info.name[0] = 0;
		info.type[0] = 0;
		info.bootable = 0;
#ifdef CONFIG_PARTITION_UUIDS
		info.uuid[0] = 0;
#endif
	}

	ext4fs_set_blk_dev(dev_desc, &info);

	debug("info.size = %d\n", (int) info.size);

	if (!ext4fs_mount(info.size)) {
		printf("Failed to mount %s%d:%d\n",
			ifname, dev, part);
		ext4fs_close();
		return 1;
	}

	err = ext4fs_open(CONFIG_ANDROID_RECOVERY_CMD_FILE, &filelen);
	if (err  < 0) {
		printf("** File not found %s\n",
			CONFIG_ANDROID_RECOVERY_CMD_FILE);
		ext4fs_close();
		return 1;
	}

	debug("filelen = %lld\n", filelen);

	err = ext4fs_read(buf, CONFIG_RECOVERYFILE_SIZE, &filelen);
	if (err < 0) {
		printf("** File read error:  %s\n",
			CONFIG_ANDROID_RECOVERY_CMD_FILE);
		ext4fs_close();
		return 1;
	}

	ext4fs_close();

	return 0;
}

static int check_update_file(void)
{
	int ret, dev;
	block_dev_desc_t *dev_desc;
	
	char *sdev = getenv("mmcdev");
	dev = (int)((char)sdev[0] - '0');
	
	dev_desc = get_dev("mmc", dev);
	if (dev_desc == NULL) {
		printf("Failed to find %s:%d\n", "mmc", dev);
		return -1;
	}

	ret = fat_register_device(dev_desc, 0);
	if (ret) {
		printf("Failed to register %s: 0:0\n","mmc");
		ret = fat_register_device(dev_desc, 1);
		if (ret) {
			printf("Failed to register %s: 0:1\n","mmc");
			return -1;
		}
	}

	ret = fat_exists("update.zip");
	if(ret == 0) {
		printf("not find update.zip, ret=%d\n", ret);
		return -1;
	}
	printf("is card update, ret=%d\n", ret);
	
	return 0;
}

void setup_recovery_env(void)
{
	debug("setup env for recovery..\n");
	setenv("bootpart", RECOVERY_PART_NUM);
	setenv("bootdelay", "0");
	gd->flags |= GD_FLG_RECOVERY;
}

void check_recovery_mode(void)
{
	if (atc260x_pstore_get_noerr(ATC260X_PSTORE_TAG_REBOOT_RECOVERY) != 0) {
		printf("PMU recovery flag founded!\n");
		atc260x_pstore_set(ATC260X_PSTORE_TAG_REBOOT_RECOVERY, 0);
		setup_recovery_env();
	}

	if (check_recovery_cmd_file() == 0) {
		printf("Recovery command file founded!\n");
		setup_recovery_env();
	}

	if (check_update_file() == 0) {
		printf("Update.zip file founded!\n");
		setup_recovery_env();
	}
}
