/*
 * (C) 2021- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SSHMAC_H
#define SSHMAC_H

#include "ttxssh.h"
#include "digest.h"

typedef enum {
	HMAC_NONE,      /* disabled line */
	HMAC_SHA1,
	HMAC_MD5,
	HMAC_SHA1_96,
	HMAC_MD5_96,
	HMAC_RIPEMD160,
	HMAC_SHA2_256,
	HMAC_SHA2_256_96,
	HMAC_SHA2_512,
	HMAC_SHA2_512_96,
	HMAC_SHA1_EtM,
	HMAC_MD5_EtM,
	HMAC_SHA1_96_EtM,
	HMAC_MD5_96_EtM,
	HMAC_RIPEMD160_EtM,
	HMAC_SHA2_256_EtM,
	HMAC_SHA2_512_EtM,
	HMAC_IMPLICIT,
	HMAC_UNKNOWN,
	HMAC_MAX = HMAC_UNKNOWN,
} mac_algorithm;

typedef struct ssh2_mac_t {
	mac_algorithm id;
	char *name;
	digest_algorithm alg;
	int truncatebits; /* truncate digest if != 0 */
	int etm;		  /* Encrypt-then-MAC */
} ssh2_mac_t;

struct Mac;

char* get_ssh2_mac_name(const struct ssh2_mac_t *mac);
char* get_ssh2_mac_name_by_id(mac_algorithm id);
// const EVP_MD* get_ssh2_mac_EVP_MD(const struct SSH2Mac *mac);
const digest_algorithm get_ssh2_mac_hash_algorithm(const struct ssh2_mac_t *mac);
const struct ssh2_mac_t *get_ssh2_mac(mac_algorithm id);
int get_ssh2_mac_truncatebits(const struct ssh2_mac_t *mac);
int get_ssh2_mac_etm(const struct ssh2_mac_t *mac);
void normalize_mac_order(char *buf);
const struct ssh2_mac_t *choose_SSH2_mac_algorithm(char *server_proposal, char *my_proposal);
void SSH2_update_hmac_myproposal(PTInstVar pvar);

/*
 * Import from OpenSSH 7.9p1
 * $OpenBSD: mac.h,v 1.10 2016/07/08 03:44:42 djm Exp $
 */

int mac_setup_by_alg(struct Mac *mac, const struct ssh2_mac_t *macalg);
int mac_init(struct Mac *);
int mac_compute(struct Mac *, u_int32_t, const u_char *, int,
    u_char *, size_t);
int mac_check(struct Mac *, u_int32_t, const u_char *, size_t,
    const u_char *, size_t);
void mac_clear(struct Mac *);

#endif /* SSHCMAC_H */
