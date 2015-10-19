#ifndef __PERF_H__
#define __PERF_H__

#include <time.h>
static double now_ms()
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return 1000.0*res.tv_sec + (double)res.tv_usec/1e3;
}

static long long get_current_time()
{
    struct  timeval    tv;
    struct  timezone   tz;
    long long ncurrent_time = 0x0LL;
    gettimeofday(&tv, &tz);
    ncurrent_time = ((long long)tv.tv_sec) * 1000000 + (long long)tv.tv_usec;
    return ncurrent_time;
}

#endif
