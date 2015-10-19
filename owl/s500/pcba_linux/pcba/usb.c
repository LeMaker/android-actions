#include "case.h"

//insmod drv under folder /misc/modules
bool insmod_drv(char *drv_name, char *opt, int flg)
{
	struct stat st;
	char drv_path[20] = "/misc/modules/";
	char cmd[100];
	
	if(flg)
	{
		sprintf(cmd, "%s%s.ko", drv_path, drv_name);
		if(-1 == stat(cmd, &st))
		{
			printf("drv not exist! file : %s\n", cmd);
			return false;
		}
		sprintf(cmd, "insmod %s%s.ko %s", drv_path, drv_name, opt);
	}
	else
	{
		sprintf(cmd, "rmmod %s %s", drv_name, opt);
	}
	
	// printf("insmod drv : %s\n", cmd);
	
	if(pcba_system(cmd))
	{
		return true;
	}
	
	return false;
}

bool test_usb(case_t *usb_case)
{
	int ret = 0;
	char u_disk[20] = "usb-storage";
	char usb_mouse[20] = "usbhid";
	char usb_file[50] = "/sys/kernel/debug/usb/devices";
	char usb_cmd[100];
	char result[50];
	char usb_size[10];
	bool passflg = false;
	int cur_flg = 0;
	char *connect = "a";
	char *remove = "b";
	
	int fid;
	
	while(true)
	{
		ret = cat_file(usb_case->nod_path);
		if(ret == -1)
			break;
		if(ret != cur_flg)
		{
			cur_flg = ret;
			if(ret)
			{
				printf("usb connected\n");
				if (!strcmp(usb_case->dev_name, "7059") || !strcmp(usb_case->dev_name, "9009"))
				{
					printf("%s-------\n",usb_case->dev_name);
					direct_thread_sleep(1000000);
					sprintf(usb_cmd, "echo USB_B_OUT > /sys/monitor/usb_port/config/usb_con_msg");
					pcba_system(usb_cmd);
					direct_thread_sleep(1000000);
					sprintf(usb_cmd, "echo USB_A_IN > /sys/monitor/usb_port/config/usb_con_msg");
					pcba_system(usb_cmd);

					passflg = true;
					direct_thread_sleep(5000000);
					pcba_system("mount /dev/sda /mnt/uhost");
					pcba_system("df -h /mnt/uhost | tail +2 | awk '{print$2}' > /tmp.usb.size");
					if(cat_file_s("/tmp.usb.size", usb_size))
					{
						sprintf(usb_case->pass_string, "%s (%s)", usb_case->pass_string, usb_size);
					}
					draw_result(usb_case, true);
				}
				else  //other ic type
				{
					if(!strcmp(usb_case->dev_name, "7021"))
					{
						printf("gs702c------------------\n");
						fid = open("/proc/acts_hcd", O_WRONLY);
						if(-1 == fid)
							return false;
						write(fid, connect, 1);
						close(fid);
					}
					else
					{
						printf("gs702a---------------\n");
						if(!insmod_drv("dwc3", "host_config=1", 1))
							return false;
						if(!insmod_drv("dwc3-actions", "", 1))
							return false;
						if(!insmod_drv("xhci-hcd", "", 1))
							return false;
					}

					direct_thread_sleep(1000000);
					sprintf(usb_cmd, "grep -E \"%s|%s\" %s > /tmp.usb", u_disk, usb_mouse, usb_file);
					// system("cat /sys/kernel/debug/usb/devices");
					// printf("usb_cmd = %s\n", usb_cmd);
					if(pcba_system(usb_cmd))
					{
						if(cat_file_s("/tmp.usb", result))
						{
							passflg = true;
							direct_thread_sleep(1500000);
							pcba_system("mount /dev/sda1 /mnt/uhost");
							pcba_system("df -h /mnt/uhost | tail +2 | awk '{print$2}' > /tmp.usb.size");
							if(cat_file_s("/tmp.usb.size", usb_size))
							{
								sprintf(usb_case->pass_string, "%s (%s)", usb_case->pass_string, usb_size);
							}
							draw_result(usb_case, true);
						}
					}
					else
					{
						printf("usb cmd error\n");
						return false;
					}
				}
			}
			else
			{
				printf("usb connect remove\n");
				
			
				if(!strcmp(usb_case->dev_name, "7059") || !strcmp(usb_case->dev_name, "9009"))
				{
					pcba_system("umount /dev/sda");
				}
				else if(!strcmp(usb_case->dev_name, "7021"))
				{
					pcba_system("umount /dev/sda1");
					fid = open("/proc/acts_hcd", O_WRONLY);
					if(-1 == fid)
						return false;
					write(fid, remove, 1);
					close(fid);
				}
				else
				{
					pcba_system("umount /dev/sda1");
					if(!insmod_drv("xhci-hcd", "", 0))
						return false;
					if(!insmod_drv("dwc3-actions", "", 0))
						return false;
					if(!insmod_drv("dwc3", "", 0))
						return false;
				}
				if(passflg)
					return true;
			}
		}
		else
			direct_thread_sleep(500000);
	}
}
