#include "case.h"

bool test_pc(case_t *pc_case)
{
	int drv_on = 0;
	int ret;
	while(true)
	{
		ret = cat_file(pc_case->nod_path);
		if(ret == -1)
			return false;
		if(ret)
		{
			draw_result(pc_case, true);
			if(!drv_on)
			{
				if (!strcmp(pc_case->dev_name, "7059"))
				{
					pcba_system("echo 0 > /sys/class/android_usb/android0/enable");
					pcba_system("echo mass_storage > /sys/class/android_usb/android0/functions");
					pcba_system("echo 1 > /sys/class/android_usb/android0/enable");
				}
				else if (!strcmp(pc_case->dev_name, "7021"))
				{
					if(!insmod_drv("udc", "", 1))
						return false;
					if(!insmod_drv("g_android", "", 1))
						return false;
					pcba_system("echo 0 > /sys/class/android_usb/android0/enable");
					pcba_system("echo mass_storage > /sys/class/android_usb/android0/functions");
					pcba_system("echo 1 > /sys/class/android_usb/android0/enable");
				}
				else if (!strcmp(pc_case->dev_name, "9009"))
				{
					if(!insmod_drv("libcomposite", "", 1))
						return false;
					if(!insmod_drv("u_serial", "", 1))
						return false;
					if(!insmod_drv("usb_f_acm", "", 1))
						return false;
					if(!insmod_drv("g_android", "", 1))
						return false;
					pcba_system("echo 0 > /sys/class/android_usb/android0/enable");
					pcba_system("echo 10d6 > /sys/class/android_usb/android0/idVendor");
					pcba_system("echo 0c02> /sys/class/android_usb/android0/idProduct");
					pcba_system("echo mass_storage > /sys/class/android_usb/android0/functions");
					pcba_system("echo 1 > /sys/class/android_usb/android0/enable");
					//pcba_system("echo /dev/actk > /sys/class/android_usb/f_mass_storage/lun0/file");
				}
				else
				{
					if(!insmod_drv("dwc3", "", 1))
						return false;
					if(!insmod_drv("dwc3-actions", "", 1))
						return false;
					if(!insmod_drv("g_android", "", 1))
						return false;
					pcba_system("echo 0 > /sys/class/android_usb/android0/enable");
					pcba_system("echo mass_storage > /sys/class/android_usb/android0/functions");
					pcba_system("echo 1 > /sys/class/android_usb/android0/enable");
				}
				drv_on = 1;
			}
		}
		else
		{
			if(drv_on)
			{
				if (!strcmp(pc_case->dev_name, "7059"))
				{
					; //do nothing
				}
				else if (!strcmp(pc_case->dev_name, "7021"))
				{
					if(!insmod_drv("g_android", "", 0))
						return false;
					if(!insmod_drv("udc", "", 0))
						return false;
				}
				else if (!strcmp(pc_case->dev_name, "9009"))
				{
					/*if(!insmod_drv("g_android", "", 0))
						return false;
					if(!insmod_drv("usb_f_acm", "", 0))
						return false;
					if(!insmod_drv("u_serial", "", 0))
						return false;
					if(!insmod_drv("libcomposite", "", 0))
						return false;*/
				}
				else
				{
					if(!insmod_drv("g_android", "", 0))
						return false;
					if(!insmod_drv("dwc3-actions", "", 0))
						return false;
					if(!insmod_drv("dwc3", "", 0))
						return false;
				}
				drv_on = 0;
			}
		}
		direct_thread_sleep(1000000);
	}
}