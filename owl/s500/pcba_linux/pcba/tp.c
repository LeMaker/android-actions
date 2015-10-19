#include <pthread.h>
#include "case.h"

extern bool record_flg;
extern bool play_avoid_flg;
#ifndef __GS702C__
extern pthread_mutex_t mutex;
extern pthread_cond_t condition;
#endif
// {1 2 3 4 -1 -2 -3 -4}
static int tp_rotate[8][2][3] = 
	{{{0, 1, 0}, {0, 0, 1}},
	{{0, 0, 1},  {1, -1, 0}},
	{{1, -1, 0}, {1, 0, -1}},
	{{1, 0, -1}, {0, 1, 0}},
	{{0, 0, 1},  {0, 1, 0}},
	{{1, -1, 0}, {0, 0, 1}},
	{{1, 0, -1}, {1, -1, 0}},
	{{0, 1, 0},  {1, 0, -1}}};

static int width = 0, height = 0;
static int tp_width = 0, tp_height = 0;

typedef struct
{
	bool enable;
	int id;
	int tp_x;
	int tp_y;
	int lcd_x;
	int lcd_y;
	int last_x;
	int last_y;
	bool sync_flg;
} mul_tp;

void mtp_clean(mul_tp *p, bool init)
{
	p->enable = false;
	p->id = -1;
	
	if(init)
	{
		p->tp_x = -1;
		p->tp_y = -1;
		p->lcd_x = -1;
		p->lcd_y = -1;
	}
	p->last_x = -1;
	p->last_y = -1;
	p->sync_flg = false;
}

void mtp_sync(mul_tp *p, int ro[2][3], int width, int height)
{
	if(p->tp_x < 0 || p->tp_y < 0)
	{
		printf("got -1\n");
		return;
	}
	p->lcd_x = ro[0][0] * width + ro[0][1] * p->tp_x + ro[0][2] * p->tp_y;
	p->lcd_y = ro[1][0] * height + ro[1][1] * p->tp_x + ro[1][2] * p->tp_y;
	if(p->lcd_x >= width) p->lcd_x = width - 1;
	if(p->lcd_y >= height) p->lcd_y = height - 1;
	p->sync_flg = false;
}

void draw_line(IDirectFBSurface *surface, mul_tp *p)
{
	DFBRegion linespace;
	if(p->last_x >= 0 && p->last_y >= 0)
	{
		DFBCHECK(surface->DrawLine(surface, p->last_x, p->last_y, p->lcd_x, p->lcd_y));
		if(p->last_x < p->lcd_x)
		{
			linespace.x1 = p->last_x;
			linespace.x2 = p->lcd_x;
		}
		else
		{
			linespace.x1 = p->lcd_x;
			linespace.x2 = p->last_x;
		}
		if(p->last_y < p->lcd_y)
		{
			linespace.y1 = p->last_y;
			linespace.y2 = p->lcd_y;
		}
		else
		{
			linespace.y1 = p->lcd_y;
			linespace.y2 = p->last_y;
		}
		DFBCHECK(surface->Flip(surface, &linespace, DSFLIP_NONE));
	}
	p->last_x = p->lcd_x;
	p->last_y = p->lcd_y;
}

int wait_tp(char *tp_name)
{
	int ret = -1;
	struct stat st;
	char command[100];
	
	sprintf(command, "cat /proc/interrupts | grep \"%s\" > /tmp.ts", tp_name);
	while(true)
	{
		ret = system(command);
		if(stat("/tmp.ts", &st) >= 0) //获取文件信息
		{
			if(st.st_size > 0)
				break;
		}
		printf("wait tp\n");
		direct_thread_sleep(1000000);
	}

	return 0;
}

static int position_resize(int tp_value, bool is_x, bool tp_rev)
{
	int ret = 0;
	
	if(is_x == tp_rev)
		ret = tp_value * height;
	else
		ret = tp_value * width;
	
	if(is_x)
	{
		ret /= tp_width;
	}
	else
	{
		ret /= tp_height;
	}
	return ret;
}

int track_tp(IDirectFBSurface *surface, char *dev_name, int tp_ro)
{
	void            *devices = NULL;
	int ret = -1, i;
	int tp_fd = -1;
	fd_set rfds;
	bool handle_tp = false;
	bool tp_rev = false;
	mul_tp mtp[10] = {0};
	int cur_slot = 0;
	struct input_event buffer;
    struct input_absinfo info;
	char event_path[100];
	
	if(-1 == get_input_event_name(dev_name, event_path))
	{
		printf("find tp device error\n");
		return 0;
	}
	
	DFBCHECK(surface->GetSize(surface, &width, &height));
	
	tp_fd = open(event_path, O_RDWR | O_NONBLOCK);

	if(ioctl(tp_fd, EVIOCGABS(ABS_MT_POSITION_X), &info) >= 0)
	{
		tp_width = info.maximum;
	}
    if(ioctl(tp_fd, EVIOCGABS(ABS_MT_POSITION_Y), &info) >= 0)
	{
        tp_height = info.maximum;
    }
	printf("tp  = %d * %d\n", tp_width, tp_height);
	if(tp_ro == 1 || tp_ro == 3 || tp_ro == 4 || tp_ro == 6)
	{
		tp_rev = true;
	}
		
	for(i = 0; i < 10; i++)
	{
		mtp_clean(&mtp[i], true);
	}
	FD_ZERO(&rfds);
	FD_SET(tp_fd, &rfds);
	do
	{	
		ret = select(tp_fd+1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET(tp_fd, &rfds))
		{
			memset(&buffer, 0, sizeof(struct input_event));
			ret = read(tp_fd, &buffer, sizeof(struct input_event));
			fsync(tp_fd);

			switch(buffer.type)
			{
				case EV_ABS:
					switch(buffer.code)
					{
						case ABS_MT_POSITION_X:
							// printf("x : %d\n", buffer.value);
							mtp[cur_slot].tp_x = position_resize(buffer.value, true, tp_rev);
							mtp[cur_slot].sync_flg = true;
							break;
						case ABS_MT_POSITION_Y:
							// printf("y : %d\n", buffer.value);
							mtp[cur_slot].tp_y = position_resize(buffer.value, false, tp_rev);
							mtp[cur_slot].sync_flg = true;
							break;
						case ABS_MT_TRACKING_ID:
							// printf("tracking_id : %d\n", buffer.value);
							if(buffer.value != -1)
							{
								handle_tp = true;
								mtp[cur_slot].id = buffer.value;
							}
							else
							{
								mtp_clean(&mtp[cur_slot], false);
							}
							break;
						case ABS_MT_SLOT:
							// printf("slot : %d\n", buffer.value);
							if(mtp[cur_slot].sync_flg)
							{
								mtp_sync(&mtp[cur_slot], tp_rotate[tp_ro], width, height);
								draw_line(surface, &mtp[cur_slot]);
								mtp[cur_slot].sync_flg = false;
							}
							cur_slot = buffer.value;
							mtp[cur_slot].enable = true;
							break;
						default:
							printf("axis %x not support\n", buffer.code);
							break;
					}
					break;
				case EV_SYN:
					// printf("sync!!!\n");
					if(-1 == mtp[cur_slot].id)
					{
						printf("this slot is up, ignore sync\n");
						break;
					}
					if(mtp[cur_slot].tp_x >= 0 && mtp[cur_slot].tp_y >= 0)
					{
						mtp_sync(&mtp[cur_slot], tp_rotate[tp_ro], width, height);
						draw_line(surface, &mtp[cur_slot]);
					}
					else
					{
						break;
					}
					if(handle_tp && (mtp[cur_slot].lcd_x > width / 2) && (mtp[cur_slot].lcd_y > height / 4 * 3))
					{
						printf("got touch on record button, x:%d y:%d\n", mtp[cur_slot].lcd_x, mtp[cur_slot].lcd_y);
						if(!record_flg && play_avoid_flg)
						{
							printf("record button ignore\n");
						}
						else
						{
						#ifndef __GS702C__
							pthread_mutex_lock(&mutex); 
							record_flg = !record_flg;
							pthread_mutex_unlock(&mutex);
							pthread_cond_broadcast(&condition);
							handle_tp = false;
						#else
							record_flg = !record_flg;
							handle_tp = false;
						#endif
							
						}
					}
					break;
				default:
					printf("event type %d not handle\n", buffer.type);
			}
		}
	}while(true);
	
	close(tp_fd);
}