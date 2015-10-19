
#include <linux/types.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include "set_serial_number.h"


#define MAX_SERIAL_NUMBER_LEN 32


typedef int (*mi_item_rw)(char *name, void *buf, int size);

int get_serial_number_len(void)
{
	int ret;
	char sn_buf[MAX_SERIAL_NUMBER_LEN] = {0};
	mi_item_rw read_mi_item;

	read_mi_item = (mi_item_rw)kallsyms_lookup_name("read_mi_item");
	if(!read_mi_item) {
		printk("%s lookup read_mi_item failed\n", __FUNCTION__);
		return -1;
	}
	
	ret = read_mi_item("SN", sn_buf, MAX_SERIAL_NUMBER_LEN - 1);
	if(ret <= 0) {
		printk("%s failed, ret=%d\n", __FUNCTION__, ret);
		return -1;
	}

	printk("%s len=%d\n", __FUNCTION__, ret);

	return ret;
}


int get_serial_number(char *buf, int len)
{
	int ret;
	char sn_buf[MAX_SERIAL_NUMBER_LEN] = {0};
	mi_item_rw read_mi_item;

	if(!buf || len <= 0) {
		printk("%s, invalid param\n", __FUNCTION__);
		return -1;
	}
	
	read_mi_item = (mi_item_rw)kallsyms_lookup_name("read_mi_item");
	if(!read_mi_item) {
		printk("%s lookup read_mi_item failed\n", __FUNCTION__);
		return -1;
	}
	
	ret = read_mi_item("SN", sn_buf, MAX_SERIAL_NUMBER_LEN - 1);
	if(ret <= 0) {
		printk("%s failed, ret=%d\n", __FUNCTION__, ret);
		return -1;
	}
	printk("%s, ret=%d len=%d serial number=%s\n", __FUNCTION__, ret, len, sn_buf);
	memcpy(buf, sn_buf, ret);
	
	return ret;
}

int set_serial_number(char *buf, int len)
{
	int ret;
	mi_item_rw write_mi_item;

	if(!buf || len <= 0) {
		printk("%s, invalid param\n", __FUNCTION__);
		return -1;
	}
	printk("%s, len=%d serial number=%s\n", __FUNCTION__, len, buf);

	if(len > MAX_SERIAL_NUMBER_LEN)
		return -1;

	write_mi_item = (mi_item_rw)kallsyms_lookup_name("write_mi_item");
	if(!write_mi_item) {
		printk("%s lookup write_mi_item failed\n", __FUNCTION__);
		return -1;
	}
	
	ret = write_mi_item("SN", buf, len);
	if(ret <= 0) {
		printk("%s failed, ret=%d\n", __FUNCTION__, ret);
		return -1;
	}

	return ret;
}


typedef int(*mi_rw)(void*, int);

int get_miscinfo(char *buf)
{
	int ret=0;
	mi_rw read_misc_info;

	read_misc_info = (mi_rw)kallsyms_lookup_name("read_misc_info");
	if(!read_misc_info) {
		printk("%s lookup read_misc_info failed\n", __FUNCTION__);
		return -1;
	}
	
	ret = read_misc_info(buf, MAX_MISCINFO_LEN);
	if(ret < 0) {
		printk("%s failed, ret=%d\n", __FUNCTION__, ret);
		return -1;
	}
	
	printk("%s:len=%d\n", __FUNCTION__, ret);

	return ret;
}

int set_miscinfo(char *buf, int len)
{
	int ret;
	mi_rw write_misc_info;
	
	if(len > MAX_MISCINFO_LEN)
		return -1;

	write_misc_info = (mi_rw)kallsyms_lookup_name("write_misc_info");
	if(!write_misc_info) {
		printk("%s lookup write_misc_info failed\n", __FUNCTION__);
		return -1;
	}
	
	ret = write_misc_info(buf, len);
	if(ret < 0) {
		printk("%s failed, ret=%d\n", __FUNCTION__, ret);
		return -1;
	}

	return ret;
}





#define CHIP_ID "ATM7059"

int get_chipID_len(void)
{
	int ret;

	ret = strlen(CHIP_ID);

	printk("%s len=%d\n", __FUNCTION__, ret);

	return ret;
}

int get_chipID(char *buf, int len)
{
	int ret =0;
	struct chipID_packet *chipID =(struct chipID_packet *) buf;

	chipID->len = cpu_to_le32(len);
	strcpy(chipID->data, CHIP_ID);
	printk("%s:ret=%d len=%d chipID len:0x%x 0x%x 0x%x 0x%x data:%s\n",
		__FUNCTION__, ret, len,
		buf[0], buf[1], buf[2], buf[3], &buf[4]);

	if(ret < 0)
		return -1;

	return 0;
}


void set_upgrade_flags_and_restart(void)
{
	mdelay(100);  /*wait until csw has send!*/
	kernel_restart("adfu");
}

extern void owl_android_send_special_uevent(int val);
static void send_shutdown_machine_state_uevent(void)
{
	owl_android_send_special_uevent(0);
}

typedef int (*set_halt_flag_func)(int);

int gadget_andorid_shutdown_machine(void)
{
	set_halt_flag_func do_set_halt_flag;

	printk("%s\n", __FUNCTION__);

	do_set_halt_flag = (set_halt_flag_func)kallsyms_lookup_name("set_halt_flag");
	if(do_set_halt_flag){
		printk("%s set_halt_flag 0\n", __FUNCTION__);
		do_set_halt_flag(0);
	}
	
	send_shutdown_machine_state_uevent();

	return 0;
}

