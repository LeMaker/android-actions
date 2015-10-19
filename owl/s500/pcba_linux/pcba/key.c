#include "case.h"

//vol+ vol- home menu
#define MAX_KEY_NUM 5


typedef struct location{
	int  x;
	int  y;
	int width;
	int height;
}location;

static struct location position[MAX_KEY_NUM];

void calculate_position(int width, int height, case_t *key_case, int num)
{
	int i, key_width, key_height;

	if (key_case->doubleline)
	{
		key_width = width / 2;
		key_height = height / 2;
		for (i = 0; i < 2; i++)
		{
			position[i].x = key_width * i;
			position[i].y = 0;
			position[i].width = key_width;
			position[i].height = key_height;
		}
		key_width = width / 3;
		for (i = 2; i < 5; i++)
		{
			position[i].x = key_width * (i - 2);
			position[i].y = key_height;
			position[i].width = key_width;
			position[i].height = key_height;
		}
	}
	else
	{
		key_width = width / num;
		key_height = height;
		for (i = 0; i < 5; i++)
		{
			position[i].x = key_width * i;
			position[i].y = 0;
			position[i].width = key_width;
			position[i].height = key_height;
		}
	}
}

int get_index(int key_value)
{
	switch(key_value)
	{
		//音量- 音量+ 菜单
		case KEY_HOME:
		case KEY_HOMEPAGE:
			return 3;
		case KEY_VOLUMEDOWN:
			return 1;
		case KEY_VOLUMEUP:
			return 0;
		case KEY_MENU:
			return 2;
		case KEY_BACK:
			return 4;
	}
	return -1;
}

void update_key_count(IDirectFBSurface *surface, char *s, int pos)
{
	struct location *p = &position[pos];
	DFBCHECK(surface->SetColor(surface, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(surface->FillRectangle(surface, p->x, p->y, p->width, p->height));
	DFBCHECK(surface->SetColor(surface,  0, 0, 0xff, 0xff));
	DFBCHECK(surface->DrawString(surface, s, -1, p->x, p->y, DSTF_TOPLEFT));
	DFBCHECK(surface->Flip(surface, NULL, 0));
}

bool test_key(case_t *key_case)
{
	fd_set rfds;
	struct input_event buffer;
	IDirectFBFont *font;
	int key_fd = -1;
	int ret = -1;
	int width = 0, height = 0;
	int i, key_num, pos;
	char event_name[100];
	char tmp_key_string[30];
	int key_width, key_index;
	key_para key_adc[MAX_KEY_NUM];

	
	if(-1 == get_input_event_name(key_case->dev_name, event_name))
		return false;
	
	key_fd = open(event_name, O_RDWR | O_NONBLOCK);
	
	if(key_fd < 0)
	{
		printf("open event %s faild\n", event_name);
		return false;
	}
	
	DFBCHECK(key_case->surface->GetSize(key_case->surface, &width, &height));
	DFBCHECK(key_case->surface->GetFont(key_case->surface, &font));
	key_case->nod_path[MAX_KEY_NUM] = '\0';
	printf("key config = %s\n", key_case->nod_path);
	key_num = 0;
	for(i = 0; i < MAX_KEY_NUM; i++)
	{
		key_adc[i].enable = key_case->nod_path[i] - '0';
		key_adc[i].detected = false;
		key_adc[i].count = 0;   //add to init count num
		if(key_adc[i].enable)
			key_num++;		
	}
	
	calculate_position(width, height, key_case, key_num);
	pos = 0;
	for (i = 0; i < MAX_KEY_NUM; i++)
	{
		if (key_adc[i].enable)
		{
			key_adc[i].position = pos;
			pos++;
		}
	}
/*
	strcpy(key_adc[0].name, "音量+");
	strcpy(key_adc[1].name, "音量-");
	strcpy(key_adc[2].name, "菜单");
	strcpy(key_adc[3].name, "HOME");
	strcpy(key_adc[4].name, "BACK");*/
	
	strcpy(key_adc[0].name, "vol+");
	strcpy(key_adc[1].name, "vol-");
	strcpy(key_adc[2].name, "menu");
	strcpy(key_adc[3].name, "home");
	strcpy(key_adc[4].name, "back");

	FD_ZERO(&rfds);
//	FD_SET(key_fd, &rfds); modified by xiehaocheng move into while
	
	DFBCHECK(key_case->surface->SetColor(key_case->surface, 0xc0, 0xc0, 0xc0, 0xff));
	for(i = 0; i < MAX_KEY_NUM; i++)
	{
		if(key_adc[i].enable)
		{
			
			DFBCHECK(key_case->surface->DrawString(key_case->surface, key_adc[i].name, -1, 
				position[key_adc[i].position].x, position[key_adc[i].position].y, DSTF_TOPLEFT));
		}
	}
	DFBCHECK(key_case->surface->Flip(key_case->surface, NULL, 0));
	DFBCHECK(key_case->surface->SetColor(key_case->surface, 0, 0xff, 0xff, 0xff));
	
	while(true)
	{
		FD_SET(key_fd, &rfds);
		ret = select(key_fd+1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET(key_fd, &rfds))
		{
			memset(&buffer, 0, sizeof(struct input_event));
			ret = read(key_fd, &buffer, sizeof(struct input_event));
			fsync(key_fd);
			
			switch(buffer.code)
			{
				//音量- 音量+ 菜单
				case KEY_HOME: 			//102
				case KEY_VOLUMEDOWN:	//114
				case KEY_VOLUMEUP:		//115
				case KEY_MENU:			//139
				case KEY_BACK:			//158
				case KEY_HOMEPAGE:
					key_index = get_index(buffer.code);
					if(key_index > (MAX_KEY_NUM - 1) || key_index < 0)
					{
						printf("get key index error\n");
						break;
					}
					if(key_adc[key_index].enable && buffer.value)
					{
						key_adc[key_index].count++;
						sprintf(tmp_key_string, "%s:%d", key_adc[key_index].name, key_adc[key_index].count);
						printf("key_string=%s", tmp_key_string);
						update_key_count(key_case->surface, tmp_key_string, key_adc[key_index].position);	
					}
					break;
				default:
					printf("key not handle\n");
					break;
			}
		}
	}
	close(key_fd);
	return true;
}

bool test_key_onff(case_t *key_case)
{
	fd_set rfds;
	struct input_event buffer;
	int key_fd = -1;
	int ret = -1;
	unsigned count = 0;
	bool power_state = true;
	bool ctrl_screen = false;
	char event_name[100];
	
	if(-1 == get_input_event_name(key_case->dev_name, event_name))
		return false;

	if(!strcmp(key_case->nod_path, "0"))
	{
		ctrl_screen = false;
	}
	else
	{
		ctrl_screen = true;
	}
	
	key_fd = open(event_name, O_RDWR | O_NONBLOCK);
	if(key_fd < 0)
	{
		printf("open event %s faild\n", event_name);
		return false;
	}
	
	FD_ZERO(&rfds);
	FD_SET(key_fd, &rfds);

	while(true)
	{
		ret = select(key_fd+1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET(key_fd, &rfds))
		{
			memset(&buffer, 0, sizeof(struct input_event));
			ret = read(key_fd, &buffer, sizeof(struct input_event));
			fsync(key_fd);
			
			if(buffer.type == EV_KEY && buffer.code == KEY_POWER && buffer.value == 0)
			{
				count++;
				sprintf(key_case->pass_string, "%d\n", count);
				draw_result(key_case, true);
				if(ctrl_screen)
				{
					printf("---------get onoff key---------\n");
					power_state = !power_state;
					if(power_state)
					{
						//screen on 
						system("echo 600 > /sys/class/backlight/act_pwm_backlight/brightness");
					}
					else
					{
						//screen off
						system("echo 0 > /sys/class/backlight/act_pwm_backlight/brightness");
					}
				}
			}
		}
	}
	close(key_fd);
	return true;
}