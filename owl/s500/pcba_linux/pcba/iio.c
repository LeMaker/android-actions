#include "case.h"
bool test_iio(case_t *iio_case, iio_info *info)
{
	fd_set rfds;
	struct input_event buffer;
	int sensor_fd = -1;
	int ret = -1;
	char event_name[100];
	char pass_back[100];
	int x, y, z, last_x, last_y, last_z;
	bool first_x, first_y, first_z;
	bool pass_x, pass_y, pass_z;
	
	first_x = first_y = first_z =true;
	pass_x = pass_y = pass_z = false; 
	strcpy(pass_back, iio_case->pass_string);
	
	while(true)
	{
		if(0 == info->lock)
		{
			// printf("enter xx\n");
			x = info->x;
			if(!first_x && last_x != x)
			{
				pass_x = true;
			}
			last_x = x;
			if(first_x)
				first_x = false;
			
			y = info->y;
			if(!first_y && last_y != y)
			{
				pass_y = true;
			}
			last_y = y;
			if(first_y)
				first_y = false;
			
			z = info->z;
			if(!first_z && last_z != z)
			{
				pass_z = true;
			}
			last_z = z;
			if(first_z)
				first_z = false;
			// printf("x:y:z = %d:%d:%d\n", x, y, z);
		}
		if(!first_x && !first_y && !first_z)
		{
			if(pass_z && pass_x && pass_y)
				sprintf(iio_case->pass_string, "%s (%d, %d, %d)", pass_back, x, y, z);
			else
				sprintf(iio_case->pass_string, "%s (%d, %d, %d)", iio_case->init_string, x, y, z);
			draw_result(iio_case, true);
		}

		direct_thread_sleep(20000);
	}
	close(sensor_fd);
	return true;
}