#include "case.h"
#define BTHWCTL_DEV_NAME             "/dev/bthwctl"
#define BTHWCTL_IOC_MAGIC            0xf6
#define BTHWCTL_IOCTL_SET_POWER      _IOWR(BTHWCTL_IOC_MAGIC, 0, uint32_t) 

static int rfkill_id = -1;
static char *rfkill_state_path = NULL;

static int is_file_exist(char *file)
{
	int ret;
	struct stat buf;
	ret = stat(file, &buf);
	return ret;
}

static int init_rfkill() {
    char path[64];
    char buf[16];
    int fd;
    int sz;
    int id;
    for (id = 0; ; id++) {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            printf("open(%s) failed: %s (%d)\n", path, strerror(errno), errno);
            return -1;
        }
        sz = read(fd, &buf, sizeof(buf));
        close(fd);
        if (sz >= 9 && memcmp(buf, "bluetooth", 9) == 0) {
            rfkill_id = id;
            break;
        }
    }

    asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", rfkill_id);
    return 0;
}

static int set_bluetooth_power_rfkill(int on) {
    int sz;
    int fd = -1;
    int ret = -1;
    const char buffer = (on ? '1' : '0');

    if (rfkill_id == -1) {
        if (init_rfkill()) goto out;
    }

    fd = open(rfkill_state_path, O_WRONLY);
    if (fd < 0) {
        printf("open(%s) for write failed: %s (%d)", rfkill_state_path,
             strerror(errno), errno);
        goto out;
    }
    sz = write(fd, &buffer, 1);
    if (sz < 0) {
        printf("write(%s) failed: %s (%d)", rfkill_state_path, strerror(errno),
             errno);
        goto out;
    }
    ret = 0;

out:
    if (fd >= 0) close(fd);
    return ret;
}

static int set_bluetooth_power_8723as(int on) {
    int sz;
    int fd = -1;
    int ret = -1;
    const uint32_t buf = (on ? 1 : 0);

    fd = open(BTHWCTL_DEV_NAME, O_RDWR);
    if (fd < 0) {
        printf("Open %s to set BT power fails: %s(%d)\n", BTHWCTL_DEV_NAME,
             strerror(errno), errno);
        goto out;
    }
    
    ret = ioctl(fd, BTHWCTL_IOCTL_SET_POWER, &buf);
    if(ret < 0) {
        printf("Set BT power %d fails: %s(%d)\n", buf, 
             strerror(errno), errno);
        goto out;
    }

out:
    if (fd >= 0) close(fd);
    return ret;
}

static int set_bluetooth_power_8723usb(int on) {
//	  return 0;
    int sz;
    int fd = -1;
    int ret = -1;
    const char buffer = (on ? 'c' : 'd');
    char rfkill_state_path[64];

	if (is_file_exist("/proc/acts_hcd") != -1)
		strcpy(rfkill_state_path, "/proc/acts_hcd");
	else	
		strcpy(rfkill_state_path, "/proc/acts_hub");

    fd = open(rfkill_state_path, O_WRONLY);
    if (fd < 0) {
        printf("open (%s) for write failed: %s (%d)\n", rfkill_state_path, 
        strerror(errno), errno);
        goto out;
    }
    sz = write(fd, &buffer, 1);
    if (sz < 0) {
        printf("write(%s) failed: %s (%d)\n", rfkill_state_path, strerror(errno),
             errno);
        goto out;
    }
    ret = 0;
    if (on)
    {
    	  usleep(1000000);
    }
    else
    {
    	  usleep(200000);
    }
out:
    if (fd >= 0) close(fd);
    return ret;
}

static int bluetooth_power(case_t *bt_case, int on)
{
	//mt6620 no need to power up
	if(!strcmp(bt_case->dev_name, "mt6620"))
	{
		return 0;
	}
	if(!strcmp(bt_case->dev_name, "rtl8723as"))
	{
		printf("got as\n");
		return set_bluetooth_power_8723as(on);
	}
	if(!strcmp(bt_case->dev_name, "rtl8723au") || !strcmp(bt_case->dev_name, "rtl8723bu"))
	{
		printf("got au or bu\n");
		return set_bluetooth_power_8723usb(on);
	}
	
	//use rfkill, support ap6210,ap6330,ap6476,rtl8723bs,rtl8723vq0,rtl8761
	printf("got %s\n", bt_case->dev_name);
	return set_bluetooth_power_rfkill(on);
}

static void bluetooth_setup(case_t *bt_case)
{
	char command[200];
	char firmware_name[50];
	//mt6620 no need to do anything
	if(!strcmp(bt_case->dev_name, "mt6620"))
	{
		return;
	}
	
	if(!strcmp(bt_case->dev_name, "rtl8723as"))
	{
		sprintf(command, "hciattach -n -s 115200 /dev/%s rtk_h5 &", bt_case->nod_path);
		printf("bt command = %s\n", command);
		pcba_system(command);
		direct_thread_sleep(3000000);
		return;
	}

	if(!strcmp(bt_case->dev_name, "rtl8723bs") 
		|| !strcmp(bt_case->dev_name, "rtl8761") 
		|| !strcmp(bt_case->dev_name, "rtl8723vq0"))
	{
		direct_thread_sleep(3000000);
		sprintf(command, "rtk_hciattach -d %s -n -s 115200 /dev/%s rtk_h5 &", bt_case->dev_name, bt_case->nod_path);
		printf("bt command = %s\n", command);
		pcba_system(command);
		// printf("debug\n");
		direct_thread_sleep(3000000);
		return;
	}
	
	if(!strcmp(bt_case->dev_name, "ap6210") 
		|| !strcmp(bt_case->dev_name, "ap6330") 
		|| !strcmp(bt_case->dev_name, "ap6476"))
	{
		if(!strcmp(bt_case->dev_name, "ap6210"))
		{
			strcpy(firmware_name, "bcm20710a1.hcd");
		}
		else if(!strcmp(bt_case->dev_name, "ap6330"))
		{
			strcpy(firmware_name, "bcm40183b2.hcd");
		}
		else if(!strcmp(bt_case->dev_name, "ap6476"))
		{
			strcpy(firmware_name, "bcm2076b1.hcd");
		}
		
		sprintf(command, "brcm_patchram_plus --enable_hci --no2bytes --tosleep 200000 --baudrate 1000000 --patchram /system/etc/firmware/%s /dev/%s &", firmware_name, bt_case->nod_path);
		printf("uart = %s\n", bt_case->nod_path);

		printf("bt command = %s\n", command);
		
		pcba_system(command);
		direct_thread_sleep(4000000);
	}
	return;
}

static void bluetooth_scan(case_t *bt_case)
{
	if(!strcmp(bt_case->dev_name, "mt6620"))
	{
		pcba_system("autobt inquiry | grep \":..:\" | head -1 > /tmp.bt");
		printf("out of ap6620 bt scan\n");
		return;
	}
	pcba_system("hciconfig hci0 up");
	//cut -d' ' -f2-
	pcba_system("hcitool scan | tail +2 | awk '{print$2$3}'	> /tmp.bt");
	return;
}


bool test_bt(case_t *bt_case)
{
	int ret = 0;
	char result[100];
	
	
	if(strlen(bt_case->nod_path) == 0)
	{
		printf("set default as ttyS3\n");
		strcpy(bt_case->nod_path, "ttyS3");
	}
	ret = bluetooth_power(bt_case, 1);
	printf("set bt power on result = %d\n", ret);
	if(ret < 0)
	{
		printf("bt device power on failed\n");
		return false;
	}
	bluetooth_setup(bt_case);
	bluetooth_scan(bt_case);
	ret = cat_file_s("/tmp.bt", result);
	if(!ret)
	{
		printf("bt can't find any device\n");
		return false;
	}
	sprintf(bt_case->pass_string, "%s(%s)",bt_case->pass_string, result);
	return true;
}
