#include "gettimeofday.h"

// unix time epoch from windows file time 
#if defined(_MSC_VER)
#define FTEPOCHDIFF 116444736000000000i64
#else
#define FTEPOCHDIFF 116444736000000000LL
#endif

int gettimeofday(struct timeval *tv /*, struct timezone *tz*/ ) {
	FILETIME ft;
	__int64 t;

	if (tv) {
		GetSystemTimeAsFileTime(&ft);
		t = (((__int64)ft.dwHighDateTime << 32 | ft.dwLowDateTime) - FTEPOCHDIFF) / 10;
		tv->tv_sec  = (long)(t / 1000000);
		tv->tv_usec = (long)(t % 1000000);
	}

#if 0
	if (tz) {
		long tzsec;
		int dst;
		if (_get_timezone(&tzsec) == 0 && _get_daylight(&dst) == 0) {
			tz->tz_minuteswest = tzsec / 60;
			tz->tz_dsttime = dst;
		}
		else {
			return -1;
		}
	}
#endif

	return 0;
}

struct timeval tvdiff(struct timeval a, struct timeval b) {
	struct timeval diff;

	diff.tv_sec = b.tv_sec - a.tv_sec;
	diff.tv_usec = b.tv_usec - a.tv_usec;
	if (diff.tv_usec < 0) {
		diff.tv_usec += 1000000;
		diff.tv_sec--;
	}

	return diff;
}

struct timeval tvshift(struct timeval t, int shift) {
	struct timeval tmp;
	int mask;

	if (shift < 0 ) {
		if (shift < -8) {
			shift = -8;
		}
		shift = -shift;
		tmp.tv_usec = t.tv_usec << shift;
		tmp.tv_sec = (t.tv_sec << shift) + tmp.tv_usec / 1000000;
		tmp.tv_usec = tmp.tv_usec % 1000000;
	}
	else if (shift > 0) {
		if (shift > 8) {
			shift = 8;
		}
		mask = 0xff >> (8 - shift);
		tmp.tv_usec = ((t.tv_sec & mask) * 1000000 + t.tv_usec) >> shift;
		tmp.tv_sec = t.tv_sec >> shift;
	}
	else {	// shift == 0
		tmp = t;
	}

	return tmp;
}

int tvcmp(struct timeval a, struct timeval b) {
	if (a.tv_sec < b.tv_sec) {
		return -1;
	}
	else if (a.tv_sec > b.tv_sec) {
		return 1;
	}
	else if (a.tv_usec < b.tv_usec) {
		return -1;
	}
	else if (a.tv_usec > b.tv_usec) {
		return 1;
	}
	else {
		return 0;
	}
}
