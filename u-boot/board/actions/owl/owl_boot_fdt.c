
/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 */
#include <common.h>
#include <linux/ctype.h>
#include <libfdt.h>
#include <fdt_support.h>

#include <asm/arch/owl_afi.h>

 
DECLARE_GLOBAL_DATA_PTR;

#define ENUM_HEADER			1
#define ENUM_TAIL			2
#define ENUM_REMOVE			3

extern int read_mi_item(char *name, void *buf, unsigned int count);

static void cmd_string_remove(char *o, char *s)
{
	char *m, *f, *p;
	unsigned int len;
	char str[64];

	p = s;
	while (p) {
		m = strstr(p, " ");
		if (m)
			len = m - p;
		else
			len = strlen(p);

		strncpy(str, p, len);
		str[len] = 0;

		f = strstr(o, str);
		if (f) {
			len = strlen(f);
			memmove(f, f + strlen(str), len - strlen(str));
			*(f + len - strlen(str)) = 0;
		}
		if (!m)
			break;

		while(m && isspace(*m))
			m++;
		p = m;
	}
}


static void boot_append_remove_args(char *arg, int action)
{
	char *s;
	char cmdline[CONFIG_SYS_BARGSIZE];
	const char *ns = cmdline;

	if (!arg)
		return;

	if ((s = getenv("bootargs")) == NULL)
		s = "";

	if(action == ENUM_HEADER)
		sprintf(cmdline, "%s %s", arg, s);
	else if(action == ENUM_TAIL)
		sprintf(cmdline, "%s %s", s, arg);
	else if(action == ENUM_REMOVE)
		cmd_string_remove(cmdline, arg);
	else
		return;
	debug("cmdline: %s\n", cmdline);

	setenv("bootargs", ns);
}

static int boot_fdt_setprop(void *blob)
{
	int node;
	char new_prop[CONFIG_SYS_BARGSIZE];
    char *s;

	s = getenv("bootargs");
	strcpy(new_prop, s);
	node = fdt_find_or_add_subnode(blob, 0, "chosen");
	if (node < 0){
		printf("error when find chosen node: %d", node);
		return -1;
	}

	fdt_setprop(blob, node, "bootargs", new_prop, strlen(new_prop) + 1);
	return 0;
}


void owl_boot_fdt_setup(void *blob)
{
	char buf[64], sn[32];
	char *s;
	int index;
	int boot_dev = owl_get_boot_dev();
	bool is_android_os = true;

	s = getenv("os_type");
	if(s!=NULL && strcmp(s, "android")){
		is_android_os = false;
	} 

	printf("owl_boot_fdt_setup %s\n", s);
	if((gd->flags & GD_FLG_RECOVERY) ||
		owl_get_boot_mode() == (int)BOOT_MODE_PRODUCE){
		is_android_os = true;
		strcpy(buf, "earlyprintk");
		boot_append_remove_args(buf, ENUM_TAIL);
		strcpy(buf, "clk_ignore_unused");
		boot_append_remove_args(buf, ENUM_TAIL);
		strcpy(buf, "selinux=0");
		boot_append_remove_args(buf, ENUM_TAIL);
	}

	if(owl_get_boot_mode() == (int)BOOT_MODE_PRODUCE){
		is_android_os = false;	
		strcpy(buf, "powersave");
		boot_append_remove_args(buf, ENUM_TAIL);
		strcpy(buf, "androidboot.mode=upgrade");
		boot_append_remove_args(buf, ENUM_TAIL);
	}else{
		if(is_android_os) {
			boot_append_remove_args("androidboot.dvfslevel=0x705x", ENUM_TAIL);
			if(gd->flags & GD_FLG_CHARGER) {
				strcpy(buf, "androidboot.mode=charger");
				printf("=======androidboot.mode:charger=========\n");
				boot_append_remove_args(buf, ENUM_TAIL);
			}
		}		
	}
	
	index = owl_afi_get_serial_number();
	sprintf(buf, "console=ttyS%d", index);
	boot_append_remove_args(buf, ENUM_HEADER);

	/*
	strcpy(buf, "board_opt=0,0x1");
	boot_append_remove_args(buf, NULL);
	*/
	
	if(is_android_os){
		if(boot_dev == OWL_BOOTDEV_NAND){
			strcpy(buf, "androidboot.bootdev=nand");
		} else {
			strcpy(buf, "androidboot.bootdev=sd");
		}
		boot_append_remove_args(buf, ENUM_TAIL);
	}
	
    switch(boot_dev){
    	case OWL_BOOTDEV_NAND:
    		sprintf(buf, "boot_dev=%s", "nand");
			break;
		case OWL_BOOTDEV_SD0:
			sprintf(buf, "boot_dev=%s", "sd0");
			break;		
		case OWL_BOOTDEV_SD1:
			sprintf(buf, "boot_dev=%s", "sd1");
			break;
		case OWL_BOOTDEV_SD2:
			sprintf(buf, "boot_dev=%s", "sd2");
			break;			
		case OWL_BOOTDEV_SD02NAND:
			sprintf(buf, "boot_dev=%s", "sd02nand");
			break;				
		case OWL_BOOTDEV_SD02SD2:
			sprintf(buf, "boot_dev=%s", "sd02sd2");
			break;			
		case OWL_BOOTDEV_NOR:
			sprintf(buf, "boot_dev=%s", "nor");
			break;
    }
	boot_append_remove_args(buf, ENUM_TAIL);
	
	if(owl_get_boot_mode() != (int)BOOT_MODE_PRODUCE){
		memset(buf, 0, sizeof(buf));
		memset(sn, 0, sizeof(sn));
		strcpy(buf, "androidboot.serialno=");
		if(read_mi_item("SN", sn, sizeof(sn) - 1) < 0){
			printf("read SN failed\n");
		}else{
			printf("read SN %s\n", sn);
			strcat(buf,sn);
			boot_append_remove_args(buf, ENUM_TAIL);
		}
	}

	printf("cmdline: %s\n", getenv("bootargs"));
    boot_fdt_setprop(blob);
}
