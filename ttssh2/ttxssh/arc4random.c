/* OPENBSD ORIGINAL: lib/libc/crypto/arc4random.c */

/*	$OpenBSD: arc4random.c,v 1.31 2014/05/31 10:32:12 jca Exp $	*/

/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2013, Markus Friedl <markus@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * ChaCha based random number generator for OpenBSD.
 *   openssh-portable: openbsd-compat/arc4random.c
 */

/*
 * with LibreSSL, use getentropy() instead of RAND_bytes().
 *   OpenBSD: lib/libcrypto/arc4random/getentropy_win.c
 *   $OpenBSD: getentropy_win.c,v 1.6 2020/11/11 10:41:24 bcook Exp $
 */


#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
#include <process.h>

#define KEYSTREAM_ONLY
#include "ttxssh.h"
#include "arc4random.h"
#include "chacha.h"

#ifndef LIBRESSL_VERSION_NUMBER
#include <openssl/rand.h>
#include <openssl/err.h>
#else
#include <bcrypt.h>
#endif

/* OpenSSH isn't multithreaded */
#define _ARC4_LOCK()
#define _ARC4_UNLOCK()

#define KEYSZ	32
#define IVSZ	8
#define BLOCKSZ	64
#define RSBUFSZ	(16*BLOCKSZ)
static int rs_initialized;
static int rs_stir_pid;
static struct chacha_ctx rs;	/* chacha context for random keystream */
static u_char rs_buf[RSBUFSZ];	/* keystream blocks */
static size_t rs_have;		/* valid bytes at end of rs_buf */
static size_t rs_count;		/* bytes till reseed */

static void _rs_rekey(u_char *dat, size_t datlen);

static void
_rs_init(u_char *buf, size_t n)
{
	if (n < KEYSZ + IVSZ)
		return;
	chacha_keysetup(&rs, buf, KEYSZ * 8);
	chacha_ivsetup(&rs, buf + KEYSZ, NULL);
}

#ifdef LIBRESSL_VERSION_NUMBER
/*
 * On Windows, BCryptGenRandom with BCRYPT_USE_SYSTEM_PREFERRED_RNG is supposed
 * to be a well-seeded, cryptographically strong random number generator.
 * https://docs.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom
 */
static int
getentropy(void *buf, size_t len)
{
	if (len > 256) {
		return (-1);
	}

	if (FAILED(BCryptGenRandom(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
		return (-1);
	}

	return (0);
}
#endif /* LIBRESSL_VERSION_NUMBER */

static void
_rs_stir(void)
{
	u_char rnd[KEYSZ + IVSZ];

#ifndef LIBRESSL_VERSION_NUMBER
	if (RAND_bytes(rnd, sizeof(rnd)) <= 0) {
		return;
	}
#else
	if (getentropy(rnd, sizeof(rnd)) <= 0) {
		return;
	}
#endif

	if (!rs_initialized) {
		rs_initialized = 1;
		_rs_init(rnd, sizeof(rnd));
	} else
		_rs_rekey(rnd, sizeof(rnd));
	SecureZeroMemory(rnd, sizeof(rnd));

	/* invalidate rs_buf */
	rs_have = 0;
	memset(rs_buf, 0, RSBUFSZ);

	rs_count = 1600000;
}

static void
_rs_stir_if_needed(size_t len)
{
	int pid = _getpid();

	if (rs_count <= len || !rs_initialized || rs_stir_pid != pid) {
		rs_stir_pid = pid;
		_rs_stir();
	} else
		rs_count -= len;
}

static void
_rs_rekey(u_char *dat, size_t datlen)
{
#ifndef KEYSTREAM_ONLY
	memset(rs_buf, 0,RSBUFSZ);
#endif
	/* fill rs_buf with the keystream */
	chacha_encrypt_bytes(&rs, rs_buf, rs_buf, RSBUFSZ);
	/* mix in optional user provided data */
	if (dat) {
		size_t i, m;

		m = MIN(datlen, KEYSZ + IVSZ);
		for (i = 0; i < m; i++)
			rs_buf[i] ^= dat[i];
	}
	/* immediately reinit for backtracking resistance */
	_rs_init(rs_buf, KEYSZ + IVSZ);
	memset(rs_buf, 0, KEYSZ + IVSZ);
	rs_have = RSBUFSZ - KEYSZ - IVSZ;
}

static void
_rs_random_buf(void *_buf, size_t n)
{
	u_char *buf = (u_char *)_buf;
	size_t m;

	_rs_stir_if_needed(n);
	while (n > 0) {
		if (rs_have > 0) {
			m = MIN(n, rs_have);
			memcpy(buf, rs_buf + RSBUFSZ - rs_have, m);
			memset(rs_buf + RSBUFSZ - rs_have, 0, m);
			buf += m;
			n -= m;
			rs_have -= m;
		}
		if (rs_have == 0)
			_rs_rekey(NULL, 0);
	}
}

static void
_rs_random_u32(uint32 *val)
{
	_rs_stir_if_needed(sizeof(*val));
	if (rs_have < sizeof(*val))
		_rs_rekey(NULL, 0);
	memcpy(val, rs_buf + RSBUFSZ - rs_have, sizeof(*val));
	memset(rs_buf + RSBUFSZ - rs_have, 0, sizeof(*val));
	rs_have -= sizeof(*val);
}

uint32
arc4random(void)
{
	uint32 val;

	_ARC4_LOCK();
	_rs_random_u32(&val);
	_ARC4_UNLOCK();
	return val;
}

/*
 * If we are providing arc4random, then we can provide a more efficient 
 * arc4random_buf().
 */
void
arc4random_buf(void *buf, size_t n)
{
	_ARC4_LOCK();
	_rs_random_buf(buf, n);
	_ARC4_UNLOCK();
}
