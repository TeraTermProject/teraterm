#pragma once

#include <windows.h>
#include <time.h>

#if defined(_MSC_VER)
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif

int gettimeofday(struct timeval *tv /*, struct timezone *tz*/ );
struct timeval tvdiff(struct timeval a, struct timeval b);
struct timeval tvshift(struct timeval tv, int shift);
int tvcmp(struct timeval a, struct timeval b);
