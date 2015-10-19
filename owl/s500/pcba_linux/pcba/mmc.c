#include "case.h"

bool test_mmc(case_t *mmc_case)
{
	char mmc_size_file[100];
	int mmc_size;
	int unit = 2048;
	float mmc_size_g = 0;
	float mmc_size_m = 0;
	char boot_type[20];
	char boot_type_nod[20] = "/proc/bootdev";

	cat_file_s(boot_type_nod, boot_type);
	if (!strcmp(mmc_case->dev_name, "7029a"))
	{
		strcpy(mmc_size_file, "/sys/block/mmcblk0/size");
	}
	else if (!strcmp(mmc_case->dev_name, "9009")
			|| !strcmp(mmc_case->dev_name, "7059"))
	{
		/*ic 7059 or 9009*/
		if(!strcmp(boot_type, "sd0"))
		{
			strcpy(mmc_size_file, "/proc/mmc/logic_cap");
		}
		else if (!strcmp(boot_type, "sd2"))
		{
			strcpy(mmc_size_file, "/sys/block/mmcblk1/size");
		}
		else //nand boot
		{
			strcpy(mmc_size_file, "/sys/block/mmcblk0/size");
		}
	}
	else
	{
		/*ic other*/
		if(!strcmp(boot_type, "sd0"))
		{
			strcpy(mmc_size_file, "/proc/card/cap");
		}
		else if (!strcmp(boot_type, "sd2"))
		{
			strcpy(mmc_size_file, "/sys/block/mmcblk1/size");
		}
		else //nand boot
		{
			strcpy(mmc_size_file, "/sys/block/mmcblk0/size");
		}
	}
	
	printf("mmc_size file = %s\n", mmc_size_file);
	if(wait_dev_plugin(mmc_size_file))
	{
		mmc_size = cat_file(mmc_size_file);
		if(mmc_size == -1)
			return false;
		
		if (!strcmp(mmc_case->dev_name, "9009")
				|| !strcmp(mmc_case->dev_name, "7059"))
		{
			/*ic 7059 or 9009*/
			if(!strcmp(boot_type, "sd0"))
			{
				unit = 1;
			}
		}
		
		mmc_size_m = (float)mmc_size / unit;  //get MB size;
		mmc_size_g = mmc_size_m / 1024;        //get GB size;
		
		if(mmc_size_m > 1024)
		{
			sprintf(mmc_case->pass_string, "%s(%.2fG)",mmc_case->pass_string, mmc_size_g);
		}
		else
		{
			sprintf(mmc_case->pass_string, "%s(%.2fM)",mmc_case->pass_string, mmc_size_m);
		}
	}
	return true;
}
