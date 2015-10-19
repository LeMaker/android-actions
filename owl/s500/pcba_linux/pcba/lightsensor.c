#include "case.h"
bool test_lightsensor(case_t *sensor_case)
{
	printf("enter lightsensor\n");
	fd_set rfds;
	struct input_event buffer;
	int sensor_fd = -1;
	int ret = -1;
	char event_name[100];
	char pass_back[100];
	char *tmp;
	char *nod;
	int min, max;
	bool got_dark, got_light;
	
	min = max = -1;
	got_dark = got_light = false;
	
	strcpy(pass_back, sensor_case->pass_string);
	
	if(-1 == get_input_event_name(sensor_case->dev_name, event_name))
	{
		printf("can't find gsensor device\n");
		return false;
	}

	sensor_fd = open(event_name, O_RDWR | O_NONBLOCK);
	if(sensor_fd < 0)
	{
		printf("open gsensor error, nod : %s\n", event_name);
		return false;
	}
	
	nod = sensor_case->nod_path;
	printf("nod_path = %s\n", sensor_case->nod_path);
	tmp = strsep(&nod, "#");
	printf("tmp = %s\n", tmp);
	max = atoi(tmp);
	printf("enter lightsensor\n");
	if(nod != NULL)
		min = atoi(nod);
	printf("max = %d, min = %d\n", max, min);
	if(0 == min)
		got_dark = true;

	FD_ZERO(&rfds);
	FD_SET(sensor_fd, &rfds);
	while(true)
	{
		ret = select(sensor_fd+1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET(sensor_fd, &rfds))
		{
			memset(&buffer, 0, sizeof(struct input_event));
			ret = read(sensor_fd, &buffer, sizeof(struct input_event));
			fsync(sensor_fd);
			if(buffer.type == EV_ABS && buffer.code == ABS_MISC)
			{
				if(buffer.value > max)
					got_light = true;
				if(buffer.value < min)
					got_dark = true;
			
				if(got_dark && got_light)
				{
					sprintf(sensor_case->pass_string, "%s (%d)", pass_back, buffer.value);	
				}
				else
				{
					sprintf(sensor_case->pass_string, "%s (%d)", sensor_case->init_string, buffer.value);
				}
				draw_result(sensor_case, true);
			}
		}
	}
	close(sensor_fd);
	return true;
}