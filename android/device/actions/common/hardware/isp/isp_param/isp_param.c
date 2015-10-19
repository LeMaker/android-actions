#include "isp_ctl.h"
#include "isp_log.h"
#include "isp_imx0219.h"
#include "isp_gc2755.h"

typedef void (*isphal_set_param_func)(act_isp_init_param_t* init_param);

static int isphal_param_support_sensor_id[] = 
{
	0x0219, //imx0219
	0x2655, //gc2755
};

static isphal_set_param_func isphal_param_support_func[] = 
{
	isphal_set_param_imx0219, 
	isphal_set_param_gc2755,
};

int isphal_set_param_by_sensor_id(act_isp_init_param_t* init_param,int sensor_id)
{
	int i; 
	int nsupport = sizeof(isphal_param_support_sensor_id) / sizeof(int);
	for (i = 0; i < nsupport; i++)
	{
		if(isphal_param_support_sensor_id[i] == sensor_id)
		{
			break;
		}
	}

	if(i < nsupport)
	{
		isphal_param_support_func[i](init_param);
		printf("info!find sensor_id(%x) sucess!\n",sensor_id);
		return 0;
	}

	printf_err("err!can not support the sensor_id(%x)!\n",sensor_id);
	return -1;
}