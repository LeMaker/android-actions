#include "case.h"

bool test_wifi(case_t *wifi_case)
{
	int ret, count = 0;
	char iwlist_command[100] = "iwlist wlan0 scan > /tmp.wifi";
	char result[100];
	char result_name[100];
	int limit_dbm, real_dbm;

	//wait wifi up
	direct_thread_sleep(2000000);
	do
	{
		direct_thread_sleep(1000000);
		
		if(!pcba_system(iwlist_command))
		{
			count++;
			continue;
		}
		
		if(strlen(wifi_case->dev_name) != 0)
		{
			strcpy(result_name, wifi_case->dev_name);
			sprintf(iwlist_command, "grep %s /tmp.wifi -A 100 > /tmp.wifi2", result_name);
			printf("new command = %s\n", iwlist_command);
			pcba_system(iwlist_command);
			if(is_file_empty("/tmp.wifi2"))
			{
				printf("can't find essid %s\n", result_name);
				return false;
			}
			pcba_system("cat /tmp.wifi2 > /tmp.wifi");
		}
		else
		{
			//get wifi name
			if(!pcba_system("cat /tmp.wifi  | grep \"ESSID\" | head -1 > /tmp.wifi.name"))
			{
				count++;
				continue;
			}
			ret = cat_file_s("/tmp.wifi.name", result);
			if(!ret)
			{
				count++;
				continue;
			}
			//ESSID:"Actions_CSRD_ESS"
			sscanf(result,"%*[^\"]\"%[^\"]\"", result_name);
		}
		//get wifi signal level
		if(!pcba_system("cat /tmp.wifi | grep level | head -1 | grep -o [-][0-9]*[0-9] | head -1 > /tmp.wifi.level"))
		{
			count++;
			continue;
		}
		ret = cat_file_s("/tmp.wifi.level", result);
		if(!ret)
		{
			count++;
			continue;
		}
		if(strlen(wifi_case->nod_path) != 0)
		{
			limit_dbm = atoi(wifi_case->nod_path);
			real_dbm = atoi(result);
			printf("limit dbm = %d, real dbm = %d\n", limit_dbm, real_dbm);
			if(real_dbm < limit_dbm)
				return false;
		}
		sprintf(wifi_case->pass_string, "%s(%sdbm:%s)",wifi_case->pass_string, result, result_name);
		return true;
	}while(count < 10);

	printf("wifi cant find any ap in 10s\n");
	return false;
}