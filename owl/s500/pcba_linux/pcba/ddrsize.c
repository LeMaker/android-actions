#include "case.h"

bool test_ddrsize(case_t *ddrsize_case)
{
	char meminfo_path[20] = "/proc/meminfo";
	char cmd[100];
	int ddrsize = 0;

	sprintf(cmd, "cat %s | head -1 | awk '{print$2}' > /tmp.ddrsize", meminfo_path);
	printf("cmd = %s\n", cmd);
	
	pcba_system(cmd);
	//kb
	ddrsize = cat_file("/tmp.ddrsize");
	if(ddrsize == -1)
	{
		printf("--------------------ddrsize fail\n");
		return false;
	}
	//mb
	ddrsize /= 1024;
	printf("ddrsize = %d\n", ddrsize);
	sprintf(ddrsize_case->pass_string, "%s(%dMB)",ddrsize_case->pass_string, ddrsize);
	
	return true;
}