#pragma once

#include <windows.h>
#include <time.h>

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#define FTEPOCHDIFF 116444736000000000i64

int gettimeofday(struct timeval *tv, struct timezone *tz);
struct timeval tvdiff(struct timeval a, struct timeval b);
struct timeval tvshift(struct timeval tv, int shift);
int tvcmp(struct timeval a, struct timeval b);
