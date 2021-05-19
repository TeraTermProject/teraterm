/* Imported from OpenSSH-8.5p1, TeraTerm Project */

/* $OpenBSD: poly1305.h,v 1.4 2014/05/02 03:27:54 djm Exp $ */

/* 
 * Public Domain poly1305 from Andrew Moon
 * poly1305-donna-unrolled.c from https://github.com/floodyberry/poly1305-donna
 */

#ifndef POLY1305_H
#define POLY1305_H

#include <stdio.h>
#include <sys/types.h>
typedef unsigned char u_char;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define POLY1305_KEYLEN		32
#define POLY1305_TAGLEN		16

void poly1305_auth(u_char out[POLY1305_TAGLEN], const u_char *m, size_t inlen,
    const u_char key[POLY1305_KEYLEN]);

#endif	/* POLY1305_H */
