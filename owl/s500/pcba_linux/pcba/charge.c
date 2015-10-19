#include "case.h"

const static int max_charge_num = 2;

bool test_charge(case_t *charge_case)
{
	char usb_charge_path[100] = "/sys/class/power_supply/atc260x-usb/online";
	char wall_charge_path[100] = "/sys/class/power_supply/atc260x-wall/online";
	IDirectFBFont *font;
	int width = 0, height = 0;
	int ret = -1;
	int i, charge_num, tmp;
	int charge_width, charge_index;
	bool got_all;
	key_para charge_adc[max_charge_num];
	
	DFBCHECK(charge_case->surface->GetSize(charge_case->surface, &width, &height));
	DFBCHECK(charge_case->surface->GetFont(charge_case->surface, &font));
		
	charge_case->nod_path[max_charge_num] = '\0';
	charge_num = 0;
	for(i = 0; i < max_charge_num; i++)
	{
		charge_adc[i].enable = charge_case->nod_path[i] - '0';
		charge_adc[i].detected = false;
		if(charge_adc[i].enable)
			charge_num++;		
	}
	charge_width = width / charge_num;
	
	tmp = 0;
	for(i = 0; i < max_charge_num; i++)
	{
		if(charge_adc[i].enable)
		{
			charge_adc[i].position = tmp * charge_width;
			tmp++;
		}
	}
	
	strcpy(charge_adc[0].name, "USB");
	strcpy(charge_adc[1].name, "充电器");
	
	charge_adc[0].para = usb_charge_path;
	charge_adc[1].para = wall_charge_path;
	
	got_all = false;
	tmp = 0;
	
	DFBCHECK(charge_case->surface->SetColor(charge_case->surface, 0xc0, 0xc0, 0xc0, 0xff));
	for(i = 0; i < max_charge_num; i++)
	{
		if(charge_adc[i].enable)
		{
			DFBCHECK(charge_case->surface->DrawString(charge_case->surface, charge_adc[i].name, -1, charge_adc[i].position, 0, DSTF_TOPLEFT));
		}
	}
	DFBCHECK(charge_case->surface->Flip(charge_case->surface, NULL, 0));
	DFBCHECK(charge_case->surface->SetColor(charge_case->surface, 0, 0xff, 0xff, 0xff));
	
	while(!got_all)
	{
		for(i = 0; i < max_charge_num; i++)
		{
			if(charge_adc[i].enable && !charge_adc[i].detected)
			{
				ret = cat_file(charge_adc[i].para);
				if(1 == ret)
				{
					printf("got charge %s\n", charge_adc[i].name);
					DFBCHECK(charge_case->surface->DrawString(charge_case->surface, charge_adc[i].name, -1, charge_adc[i].position, 0, DSTF_TOPLEFT));
					DFBCHECK(charge_case->surface->Flip(charge_case->surface, NULL, 0));
					charge_adc[i].detected = true;
					tmp++;
				}
			}
		}

		if(tmp >= charge_num)
		{
			got_all = true;
		}
		direct_thread_sleep(500000);
	}
	return true;
}