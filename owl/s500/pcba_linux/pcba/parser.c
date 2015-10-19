#include "case.h"

static case_t *case_p;
static FILE *fp;

bool isNum(char * str)
{
	int i;
	for(i = 0; i < strlen(str); i++)
	{
		if(str[i] > '9' || str[i] < '0')
		{
			return false;
		}
	}
}

int open_config_file(char *name)
{
	fp = fopen(name, "r");
	if(fp == NULL)
	{
		printf("open config file error\n");
		return -1;
	}
	return 0;
}

case_t *get_case(case_t *caseN, char *name)
{
	// printf("name = %s %d\n", name, strcmp(name, "mem"));
	if(!strcmp(name, "mem"))
	{
		return caseN + mem;
	}
	if(!strcmp(name, "ddrsize"))
	{
		return caseN + ddrsize;
	}
	if(!strcmp(name, "flash"))
	{
		return caseN + flash;
	}
	if(!strcmp(name, "wifi"))
	{
		return caseN + wifi;
	}
	if(!strcmp(name, "bt"))
	{
		return caseN + bt;
	}
	if(!strcmp(name, "gsensor"))
	{
		return caseN + gsensor;
	}
	// if(!strcmp(name, "camera"))
	// {
		// return caseN + camera;
	// }
	if(!strcmp(name, "gyro"))
	{
		return caseN + gyro;
	}
	if(!strcmp(name, "comp"))
	{
		return caseN + comp;
	}
	if(!strcmp(name, "lightsensor"))
	{
		return caseN + lightsensor;
	}
	if(!strcmp(name, "rtc"))
	{
		return caseN + rtc;
	}
	if(!strcmp(name, "gps"))
	{
		return caseN + gps;
	}
	if(!strcmp(name, "sdcard"))
	{
		return caseN + sdcard;
	}
	if(!strcmp(name, "usb"))
	{
		return caseN + usb;
	}
	if(!strcmp(name, "usbpc"))
	{
		return caseN + usbpc;
	}
	if(!strcmp(name, "hdmi"))
	{
		return caseN + hdmi;
	}
	if(!strcmp(name, "headphone"))
	{
		return caseN + headphone;
	}
	if(!strcmp(name, "key"))
	{
		return caseN + key;
	}
	if(!strcmp(name, "onoff"))
	{
		return caseN + onoff;
	}
	if(!strcmp(name, "charge"))
	{
		return caseN + charge;
	}
	if(!strcmp(name, "mtv"))
	{
		return caseN + mtv;
	}
	if(!strcmp(name, "tp"))
	{
		return caseN + tp;
	}
	if(!strcmp(name, "global"))
	{
		return caseN + global;
	}
	if(!strcmp(name, "flashlight"))
	{
		return caseN + flashlight;
	}
	if(!strcmp(name, "uart"))
	{
		return caseN + uart;
	}
	if(!strcmp(name, "ethernet"))
	{
		return caseN + ethernet;
	}
	return NULL;
}

int read_line(char *buf, int len)
{
	while(fgets(buf, len, fp))
	{
		if(buf[0] != '\n' && buf[0] != '#')
		{
			return strlen(buf);
		}
	}
	return 0;
}

// '0' = 0
void trim(char *ptr)
{
    char *p,*q;
    if(ptr==NULL)
        return;
    for(p=ptr; *p==' ' || *p=='\t'|| *p=='\n' ; ++p);
    if( *p==0 )
    {
        *ptr=0;
        return;
    }
    for(q=ptr; *p; ++p,++q)
    {
        *q=*p;
    }
    for(p=q-1; *p==' '||*p=='\t'||*p=='\n'; --p);
    *(++p)='\0';
}

int cut_buf(char *buf, char *conf, char *value)
{
	char *ptr = NULL;
	
	trim(buf);
	
	ptr = strchr(buf, '=');
	if(ptr == NULL)
	{
		printf(" %s %d\n", __func__, __LINE__);
		return -1;
	}
	*ptr = '\0';
	strcpy(conf, buf);
	strcpy(value, ptr + 1);
	trim(conf);
	trim(value);
	return 0;
}


int handle_buf(char *buf, case_t *caseN)
{
	char conf[100];
	char value[100];
	
	if(cut_buf(buf, conf, value))
	{
		printf(" %s %d\n", __func__, __LINE__);
		return -1;
	}
	// printf("conf = %s value = %s\n", conf, value);
	if(!strcmp(conf, "name"))
	{
		case_p = get_case(caseN, value);
	}
	
	if(case_p == NULL)
		return -1;
	while(true)
	{
		if(!strcmp(conf, "enable"))
		{
			int enable = value[0] - '0';
			// printf("enable = %d\n", enable);
			if(!enable)
			{
				case_p->enable = 0;
			}
			else
			{
				case_p->enable = 1;
			}
			break;
		}
		if(!strcmp(conf, "essid"))
		{
			strcpy(case_p->dev_name, value);
			printf("got essid = %s\n", case_p->dev_name);
			break;
			// printf("got devname = %s\n", value);
		}
		if(!strcmp(conf, "siglevel"))
		{
			strcpy(case_p->nod_path, value);
			printf("got siglevel = %s\n", case_p->nod_path);
			break;
			// printf("got devname = %s\n", value);
		}
		if(!strcmp(conf, "screenctl") || !strcmp(conf, "rotate") || !strcmp(conf, "date"))
		{
			strcpy(case_p->nod_path, value);
			break;
		}
		if(!strcmp(conf, "tptype"))
		{
			if(!strcmp(value, "interrupt"))
			{
				case_p->nod_path[5] = '1';
			}
			if(!strcmp(value, "poll"))
			{
				case_p->nod_path[5] = '0';
			}
		}
		if(!strcmp(conf, "ic"))
		{
			strcpy(case_p->dev_name, value);
			// printf("got devname = %s\n", value);
			break;
		}
		if(!strcmp(conf, "cnum"))
		{
			sprintf(case_p->nod_path, "%sc%s:", case_p->nod_path, value);
			// strcpy(case_p->nod_path, value);
			printf("camera frame num = %s\n", value);
			break;
		}
		if(!strcmp(conf, "fontsize"))
		{
			sprintf(case_p->nod_path, "%sf%s:", case_p->nod_path, value);
			// strcpy(case_p->nod_path, value);
			printf("config font size = %s\n", value);
			break;
		}
		if(!strcmp(conf, "cpixel"))
		{
			sprintf(case_p->nod_path, "%sp%s:", case_p->nod_path, value);
			// strcpy(case_p->nod_path, value);
			printf("camera pixel num = %s\n", value);
			break;
		}
		if(!strcmp(conf, "size") || !strcmp(conf, "driver") || !strcmp(conf, "uart") || !strcmp(conf, "node"))
		{
			strcpy(case_p->nod_path, value);
			break;
		}
		if(!strcmp(conf, "doubleline"))
		{
			if(atoi(value))
			{
				case_p->doubleline = 1;
			}
			break;
		}
		if(!strcmp(conf, "devname"))
		{
			strcpy(case_p->dev_name, value);
			break;
		}
		if(!strcmp(conf, "max") || !strcmp(conf, "noZ"))
		{
			if(isNum(value))
				strcat(case_p->nod_path, value);
			break;
		}
		if(!strcmp(conf, "min"))
		{
			if(isNum(value))
			{
				strcat(case_p->nod_path, "#");
				strcat(case_p->nod_path, value);
			}
			break;
		}
		if(!strcmp(conf, "vol+"))
		{
			case_p->nod_path[0] = value[0];
			break;
		}
		if(!strcmp(conf, "vol-"))
		{
			case_p->nod_path[1] = value[0];
			break;
		}
		if(!strcmp(conf, "menu"))
		{
			case_p->nod_path[2] = value[0];
			break;
		}
		if(!strcmp(conf, "home"))
		{
			case_p->nod_path[3] = value[0];
			break;
		}
		if(!strcmp(conf, "back"))
		{
			case_p->nod_path[4] = value[0];
			break;
		}
		if(!strcmp(conf, "usbcharge"))
		{
			case_p->nod_path[0] = value[0];
			break;
		}
		if(!strcmp(conf, "wallcharge"))
		{
			case_p->nod_path[1] = value[0];
			break;
		}
		if(!strcmp(conf, "interface"))
		{
			strcpy(case_p->dev_name, value);
			break;
		}
		if(!strcmp(conf, "ip"))
		{
			sprintf(case_p->nod_path, "%si%s:", case_p->nod_path, value);
			// strcpy(case_p->nod_path, value);
			printf("internet ip = %s\n", value);
			break;
		}
		if(!strcmp(conf, "netmask"))
		{
			sprintf(case_p->nod_path, "%sm%s:", case_p->nod_path, value);
			// strcpy(case_p->nod_path, value);
			printf("internet netmask = %s\n", value);
			break;
		}
		if(!strcmp(conf, "ping"))
		{
			sprintf(case_p->nod_path, "%sp%s:", case_p->nod_path, value);
			// strcpy(case_p->nod_path, value);
			printf("internet ping ip = %s\n", value);
			break;
		}
		break;
	}
}

void close_config_file()
{
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
}

int parser_config_file(char *name, case_t *caseN)
{
	char buf[100];

	if(open_config_file(name))
		return -1;

	while(read_line(buf, 100))
	{
		// printf("buf = %s", buf);
		handle_buf(buf, caseN);
	}
	
	// print_case(caseN);
	close_config_file();
	return 0;
}
