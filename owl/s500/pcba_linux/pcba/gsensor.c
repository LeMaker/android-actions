#include "case.h"
bool test_gsensor(case_t *sensor_case)
{
	fd_set rfds;
	struct input_event buffer;
	int sensor_fd = -1;
	int ret = -1;
	int havez = 1;
	char event_name[100];
	char pass_back[100];
	char enable_nod[100];
	int x, y, z, last_x, last_y, last_z;
	bool first_x, first_y, first_z, first_flg;
	bool pass_x, pass_y, pass_z, pass_flg;
	
	first_x = first_y = first_z =true;
	pass_x = pass_y = pass_z = false; 
	strcpy(pass_back, sensor_case->pass_string);
	
	ret = get_input_event_name(sensor_case->dev_name, event_name);
	if(-1 == ret)
	{
		printf("can't find gsensor device\n");
		return false;
	}
	sprintf(enable_nod, "echo 1 > /sys/class/input/input%d/enable", ret);
	printf("enable_nod = %s\n", enable_nod);
	// sprintf(enable_nod, "echo 1 > %s", enable_nod);
	pcba_system(enable_nod);
	sensor_fd = open(event_name, O_RDWR | O_NONBLOCK);
	if(sensor_fd < 0)
	{
		printf("open gsensor error, nod : %s\n", event_name);
		return false;
	}
	
	havez = !atoi(sensor_case->nod_path);
	printf("havez = %d\n", havez);

	FD_ZERO(&rfds);
	FD_SET(sensor_fd, &rfds);
	printf("begin read gsensor dev\n");
	while(true)
	{
		ret = select(sensor_fd+1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET(sensor_fd, &rfds))
		{
			memset(&buffer, 0, sizeof(struct input_event));
			ret = read(sensor_fd, &buffer, sizeof(struct input_event));
			fsync(sensor_fd);
			// printf("type=%d, code=%d, value=%d\n", buffer.type, buffer.code, buffer.value);
			if(buffer.type == EV_ABS)
			{
				// if(abs(buffer.value) > 50)
				// {
					// printf("value is too big! %d\n", buffer.value);
					// return false;
				// }
				switch(buffer.code)
				{
					case 0:
						x = buffer.value;
						if(!first_x && last_x != x)
						{
							pass_x = true;
						}
						last_x = x;
						if(first_x)
							first_x = false;
						break;
					case 1:
						y = buffer.value;
						if(!first_y && last_y != y)
						{
							pass_y = true;
						}
						last_y = y;	
						if(first_y)
							first_y = false;
						break;
					case 2:
						z = buffer.value;
						if(!first_z && last_z != z)
						{
							pass_z = true;
						}
						last_y = z;	
						if(first_z)
							first_z = false;
						break;
					default:
						printf("gsensor %d not support\n", buffer.code);
						break;
				}
			}
		}
		if(havez)
		{
			first_flg = !first_x && !first_y && !first_z;
			pass_flg = pass_z && pass_x && pass_y;
		}
		else
		{
			first_flg = !first_x && !first_y;
			pass_flg = pass_x && pass_y;
		}
		if(first_flg)
		{
			if(pass_flg)
			{
				if(havez)
				{
					sprintf(sensor_case->pass_string, "%s (%d, %d, %d)", pass_back, x, y, z);
				}
				else
				{
					sprintf(sensor_case->pass_string, "%s (%d, %d)", pass_back, x, y);
				}
			}
			else
			{
				if(havez)
				{
					sprintf(sensor_case->pass_string, "%s (%d, %d, %d)", sensor_case->init_string, x, y, z);
				}
				else
				{
					sprintf(sensor_case->pass_string, "%s (%d, %d)", sensor_case->init_string, x, y);
				}
			}
			draw_result(sensor_case, true);
		}

		direct_thread_sleep(20000);
	}
	close(sensor_fd);
	return true;
}