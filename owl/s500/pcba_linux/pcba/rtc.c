#include "case.h"
#include <linux/rtc.h>

bool compare_time(struct rtc_time rtc, struct timeval tv)
{
	time_t tmp_time;
	struct tm rtc_tm;
	int sub_result = 0;
	
	rtc_tm.tm_sec = rtc.tm_sec;
    rtc_tm.tm_min = rtc.tm_min;
    rtc_tm.tm_hour = rtc.tm_hour;
    rtc_tm.tm_mday = rtc.tm_mday;
    rtc_tm.tm_mon = rtc.tm_mon;
    rtc_tm.tm_year = rtc.tm_year;
	
	
	tmp_time = timegm(&rtc_tm);
	
	sub_result = tv.tv_sec - tmp_time;
	
	if(abs(sub_result) <= 1)
	{
		return true;
	}
	else
	{
		printf("result = %d\n", sub_result);
		return false;
	}
}

bool test_rtc(case_t *rtc_case)
{
	int fd = -1;
	int ret = -1;
	struct rtc_time rtc;
	struct tm sys_clock;
	time_t tmp_time;
	struct timeval tv;
	
	char rtc_hardware[50] = "/sys/class/rtc/rtc0/ext_osc";
	
	ret = cat_file(rtc_hardware);
	if(1 != ret)
		return false;

	fd = open("/dev/rtc0", O_RDWR);
	if(!fd)
	{
		printf("open rtc device error\n");
		return false;
	}

	sscanf(rtc_case->nod_path, "%d-%d-%d %d:%d:%d", &rtc.tm_year, &rtc.tm_mon, &rtc.tm_mday,&rtc.tm_hour, &rtc.tm_min, &rtc.tm_sec);

	rtc.tm_year -= 1900;
	rtc.tm_mon -= 1;
	printf("\nCurrentRTC data/time is %d-%d-%d, %02d:%02d:%02d.\n", rtc.tm_mday, rtc.tm_mon + 1,rtc.tm_year + 1900, rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
	ret = ioctl(fd, RTC_SET_TIME, &rtc);
	if(-1 == ret)
	{
		printf("set rtc device error\n");
		printf("errno = %d\n", errno);
		return false;
	}
	direct_thread_sleep(5000000); 		// modified wait more time for rtc to set time
	ret = ioctl(fd, RTC_RD_TIME, &rtc);
	if(-1 == ret)
	{
		printf("read rtc device error\n");
		return false;
	}
	printf("\nCurrentRTC data/time is %d-%d-%d, %02d:%02d:%02d.\n", rtc.tm_mday, rtc.tm_mon + 1,rtc.tm_year + 1900, rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
	sys_clock.tm_sec = rtc.tm_sec;
    sys_clock.tm_min = rtc.tm_min;
    sys_clock.tm_hour = rtc.tm_hour;
    sys_clock.tm_mday = rtc.tm_mday;
    sys_clock.tm_mon = rtc.tm_mon;
    sys_clock.tm_year = rtc.tm_year;

	tmp_time = mktime(&sys_clock);

	tv.tv_sec = tmp_time;
    tv.tv_usec = 0;
    if(settimeofday(&tv, NULL) < 0)
    {
		printf("Set system datatime error!\n");
		printf("errno = %d\n", errno);
		return false;
	}
	
	direct_thread_sleep(4000000);
	
	ret = ioctl(fd, RTC_RD_TIME, &rtc);
	if(-1 == ret)
	{
		printf("read rtc device error\n");
		return false;
	}
	
	if(gettimeofday(&tv, NULL) < 0)
    {
		printf("get system datatime error!\n");
		printf("errno = %d\n", errno);
		return false;
	}
	
	if(!compare_time(rtc, tv))
	{
		printf("rtc != linux time\n");
		return false;
	}
	else
	{
		sprintf(rtc_case->pass_string, "%s(%d-%d-%d, %02d:%02d:%02d)",rtc_case->pass_string, rtc.tm_mday, rtc.tm_mon + 1,rtc.tm_year + 1900, rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
	}
	return true;

}