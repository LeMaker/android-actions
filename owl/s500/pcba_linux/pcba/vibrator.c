#include "case.h"

#define VIB_DEVICE "/sys/class/timed_output/vibrator/enable"

void test_vibrate(int time_out)
{
	int nwr, ret, fd;
    char value[20];

    fd = open(VIB_DEVICE, O_RDWR);
    if(fd < 0)
	{
		printf("open vibrate device error\n");
        return;
	}

    nwr = sprintf(value, "%d\n", time_out * 1000);
    ret = write(fd, value, nwr);

    close(fd);
}