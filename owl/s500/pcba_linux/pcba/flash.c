#include "case.h"

bool test_flash(case_t *flash_case)
{
	char storage_type[20] = "/proc/storage_type";
	char log_nod[20] = "/proc/nand/log_cap";
	char emmc_nod[20] = "/proc/card/cap";
	char type_result[20];
	int storage_len = 0;
	int storage_size = 0;
	float healthy_size = 0;
	
	//nand nod give a num of MB, emmc give a num of 512byte
	int div = 1024;
	int ret = 0;
	
	ret = cat_file_s(storage_type, type_result);
	
	//hope it's nand
	if(0 != ret)
	{
		if(!strcmp(type_result, "emmc"))
		{
			strcpy(flash_case->nod_path, emmc_nod);
			div *= 2048;
			//Todo:change emmc log nod
		}
		// return false;
	}
	else
		printf("can't find storage_type\n");

	storage_len = cat_file(flash_case->nod_path);
	if(storage_len == -1)
		return false;
	printf("flash : %d\n", storage_len);
	storage_size = storage_len / div;
	
	ret = cat_file(log_nod);
	//got healthy storage block num
	if(ret > 0)
	{
		healthy_size = (float)ret / div;
		sprintf(flash_case->pass_string, "%s(%.1fG/%dG)",flash_case->pass_string, healthy_size, storage_size);
	}
	else
	{
		printf("can't find healthy block!\n");
		sprintf(flash_case->pass_string, "%s(%dG)",flash_case->pass_string, storage_size);
	}
	
	return true;
}
