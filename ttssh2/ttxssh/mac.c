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

#include "ttxssh.h"
#include "mac.h"
#include "digest.h"
#include "kex.h"
#include "ssherr.h"
#include "openbsd-compat.h"


static const struct ssh2_mac_t ssh2_macs[] = {
	{HMAC_SHA1,         "hmac-sha1",                     SSH_DIGEST_SHA1,      0,  0}, // RFC4253
	{HMAC_MD5,          "hmac-md5",                      SSH_DIGEST_MD5,       0,  0}, // RFC4253
	{HMAC_SHA1_96,      "hmac-sha1-96",                  SSH_DIGEST_SHA1,      96, 0}, // RFC4253
	{HMAC_MD5_96,       "hmac-md5-96",                   SSH_DIGEST_MD5,       96, 0}, // RFC4253
	{HMAC_RIPEMD160,    "hmac-ripemd160@openssh.com",    SSH_DIGEST_RIPEMD160, 0,  0},
	{HMAC_SHA2_256,     "hmac-sha2-256",                 SSH_DIGEST_SHA256,    0,  0}, // RFC6668
//	{HMAC_SHA2_256_96,  "hmac-sha2-256-96",              SSH_DIGEST_SHA256,    96, 0}, // draft-dbider-sha2-mac-for-ssh-05, deleted at 06
	{HMAC_SHA2_512,     "hmac-sha2-512",                 SSH_DIGEST_SHA512,    0,  0}, // RFC6668
//	{HMAC_SHA2_512_96,  "hmac-sha2-512-96",              SSH_DIGEST_SHA512,    96, 0}, // draft-dbider-sha2-mac-for-ssh-05, deleted at 06
	{HMAC_SHA1_EtM,     "hmac-sha1-etm@openssh.com",     SSH_DIGEST_SHA1,      0,  1},
	{HMAC_MD5_EtM,      "hmac-md5-etm@openssh.com",      SSH_DIGEST_MD5,       0,  1},
	{HMAC_SHA1_96_EtM,  "hmac-sha1-96-etm@openssh.com",  SSH_DIGEST_SHA1,      96, 1},
	{HMAC_MD5_96_EtM,   "hmac-md5-96-etm@openssh.com",   SSH_DIGEST_MD5,       96, 1},
	{HMAC_RIPEMD160_EtM,"hmac-ripemd160-etm@openssh.com",SSH_DIGEST_RIPEMD160, 0,  1},
	{HMAC_SHA2_256_EtM, "hmac-sha2-256-etm@openssh.com", SSH_DIGEST_SHA256,    0,  1},
	{HMAC_SHA2_512_EtM, "hmac-sha2-512-etm@openssh.com", SSH_DIGEST_SHA512,    0,  1},
	{HMAC_IMPLICIT,     "<implicit>",                    SSH_DIGEST_MAX,       0,  0}, // for AEAD cipher
	{HMAC_NONE,         NULL,                            SSH_DIGEST_MAX,       0,  0},
};


char* get_ssh2_mac_name(const struct ssh2_mac_t *mac)
{
	if (mac) {
		return mac->name;
	}
	else {
		return "unknown";
	}
}

char* get_ssh2_mac_name_by_id(const mac_algorithm id)
{
	return get_ssh2_mac_name(get_ssh2_mac(id));
}

const digest_algorithm get_ssh2_mac_hash_algorithm(const struct ssh2_mac_t *mac)
{
	if (mac) {
		return mac->alg;
	}
	else {
		return SSH_DIGEST_MAX;
	}
}

const struct ssh2_mac_t *get_ssh2_mac(mac_algorithm id)
{
	const struct ssh2_mac_t *ptr = ssh2_macs;

	while (ptr->name != NULL) {
		if (ptr->id == id) {
			return ptr;
		}
		ptr++;
	}

	return NULL;
}

int get_ssh2_mac_truncatebits(const struct ssh2_mac_t *mac)
{
	if (mac) {
		return mac->truncatebits;
	}
	else {
		return 0;
	}
}

int get_ssh2_mac_etm(const struct ssh2_mac_t *mac)
{
	if (mac) {
		return mac->etm;
	}
	else {
		return 0;
	}
}

void normalize_mac_order(char *buf)
{
	static char default_strings[] = {
		HMAC_SHA2_512_EtM,
		HMAC_SHA2_256_EtM,
		HMAC_SHA1_EtM,
		HMAC_SHA2_512,
		HMAC_SHA2_256,
		HMAC_SHA1,
		HMAC_RIPEMD160_EtM,
		HMAC_RIPEMD160,
		HMAC_MD5_EtM,
		HMAC_MD5,
		HMAC_NONE,
		HMAC_SHA1_96_EtM,
		HMAC_MD5_96_EtM,
		HMAC_SHA1_96,
		HMAC_MD5_96,
		0, // Dummy for HMAC_SHA2_512_96,
		0, // Dummy for HMAC_SHA2_256_96,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

const struct ssh2_mac_t *choose_SSH2_mac_algorithm(char *server_proposal, char *my_proposal)
{
	char str_hmac[64];
	const struct ssh2_mac_t *ptr = ssh2_macs;

	choose_SSH2_proposal(server_proposal, my_proposal, str_hmac, sizeof(str_hmac));

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, str_hmac) == 0) {
			return ptr;
		}
		ptr++;
	}

	return (NULL);
}

// HMACアルゴリズム優先順位に応じて、myproposal[]を書き換える。
// (2011.2.28 yutaka)
void SSH2_update_hmac_myproposal(PTInstVar pvar)
{
	static char buf[256]; // TODO: malloc()にすべき
	int index;
	int len, i;

	// 通信中に呼ばれるということはキー再作成
	// キー再作成の場合は何もしない
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	buf[0] = '\0';
	for (i = 0 ; pvar->settings.MacOrder[i] != 0 ; i++) {
		index = pvar->settings.MacOrder[i] - '0';
		if (index == HMAC_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_ssh2_mac_name_by_id(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_MAC_ALGS_CTOS] = buf;
	myproposal[PROPOSAL_MAC_ALGS_STOC] = buf;
}

/*
 * Import from OpenSSH 7.9p1
 * $OpenBSD: mac.c,v 1.34 2017/05/08 22:57:38 djm Exp $
 */

// 今のところ umac 系をサポートしていない
// そのため TTSSH の struct Mac は、OpenSSH の struct sshmac と違い type を持っていない

int
mac_setup_by_alg(struct Mac *mac, const struct ssh2_mac_t *macalg)
{
	if ((mac->hmac_ctx = ssh_hmac_start(macalg->alg)) == NULL)
		return SSH_ERR_ALLOC_FAIL;
	mac->key_len = mac->mac_len = ssh_hmac_bytes(macalg->alg);
	if (macalg->truncatebits != 0)
		mac->mac_len = macalg->truncatebits / 8;
	mac->etm = macalg->etm;
	return 0;
}

int
mac_init(struct Mac *mac)
{
	if (mac->key == NULL)
		return SSH_ERR_INVALID_ARGUMENT;
	if (mac->hmac_ctx == NULL ||
	    ssh_hmac_init(mac->hmac_ctx, mac->key, mac->key_len) < 0)
		return SSH_ERR_INVALID_ARGUMENT;
	return 0;
}

int
mac_compute(struct Mac *mac, u_int32_t seqno,
    const u_char *data, int datalen,
    u_char *digest, size_t dlen)
{
	static union {
		u_char m[SSH_DIGEST_MAX_LENGTH];
		u_int64_t for_align;
	} u;
	u_char b[4];

	if (mac->mac_len > sizeof(u))
		return SSH_ERR_INTERNAL_ERROR;
	
	put_u32(b, seqno);
	/* reset HMAC context */
	if (ssh_hmac_init(mac->hmac_ctx, NULL, 0) < 0 ||
	    ssh_hmac_update(mac->hmac_ctx, b, sizeof(b)) < 0 ||
	    ssh_hmac_update(mac->hmac_ctx, data, datalen) < 0 ||
	    ssh_hmac_final(mac->hmac_ctx, u.m, sizeof(u.m)) < 0)
		return SSH_ERR_LIBCRYPTO_ERROR;
	if (digest != NULL) {
		if (dlen > mac->mac_len)
			dlen = mac->mac_len;
		memcpy(digest, u.m, dlen);
	}
	return 0;
}

int
mac_check(struct Mac *mac, u_int32_t seqno,
    const u_char *data, size_t dlen,
    const u_char *theirmac, size_t mlen)
{
	u_char ourmac[SSH_DIGEST_MAX_LENGTH];
	int r;

	if (mac->mac_len > mlen)
		return SSH_ERR_INVALID_ARGUMENT;
	if ((r = mac_compute(mac, seqno, data, dlen,
	    ourmac, sizeof(ourmac))) != 0)
		return r;
	if (timingsafe_bcmp(ourmac, theirmac, mac->mac_len) != 0) {
		// for debug
		// logprintf_hexdump(LOG_LEVEL_VERBOSE, ourmac, mac->mac_len, "ourmac in mac_check():");
		return SSH_ERR_MAC_INVALID;
	}
	return 0;
}

void
mac_clear(struct Mac *mac)
{
	if (mac->hmac_ctx != NULL)
		ssh_hmac_free(mac->hmac_ctx);
	mac->hmac_ctx = NULL;
}
