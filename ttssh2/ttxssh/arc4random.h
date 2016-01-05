#pragma once

#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

typedef unsigned long uint32;

#ifdef __cplusplus
extern "C" {
#endif

void arc4random_stir(void);
void arc4random_addrandom(unsigned char *dat, int datlen);
uint32 arc4random(void);
void arc4random_buf(void *buf, size_t n);

#ifdef __cplusplus
}
#endif
