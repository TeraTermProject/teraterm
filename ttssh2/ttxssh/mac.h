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

struct SSH2Mac;

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
} SSH2MacId;

char* get_ssh2_mac_name(const struct SSH2Mac *mac);
char* get_ssh2_mac_name_by_id(SSH2MacId id);
const EVP_MD* get_ssh2_mac_EVP_MD(const struct SSH2Mac *mac);
const struct SSH2Mac *get_ssh2_mac(SSH2MacId id);
int get_ssh2_mac_truncatebits(const struct SSH2Mac *mac);
int get_ssh2_mac_etm(const struct SSH2Mac *mac);
void normalize_mac_order(char *buf);
const struct SSH2Mac *choose_SSH2_mac_algorithm(char *server_proposal, char *my_proposal);
void SSH2_update_hmac_myproposal(PTInstVar pvar);

#endif /* SSHCMAC_H */
