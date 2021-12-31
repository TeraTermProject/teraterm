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
#include "kex.h"


struct SSH2Mac {
	SSH2MacId id;
	char *name;
	const EVP_MD *(*evp_md)(void);
	int truncatebits;
	int etm;
};

static const struct SSH2Mac ssh2_macs[] = {
	{HMAC_SHA1,         "hmac-sha1",                     EVP_sha1,      0,  0}, // RFC4253
	{HMAC_MD5,          "hmac-md5",                      EVP_md5,       0,  0}, // RFC4253
	{HMAC_SHA1_96,      "hmac-sha1-96",                  EVP_sha1,      96, 0}, // RFC4253
	{HMAC_MD5_96,       "hmac-md5-96",                   EVP_md5,       96, 0}, // RFC4253
	{HMAC_RIPEMD160,    "hmac-ripemd160@openssh.com",    EVP_ripemd160, 0,  0},
	{HMAC_SHA2_256,     "hmac-sha2-256",                 EVP_sha256,    0,  0}, // RFC6668
//	{HMAC_SHA2_256_96,  "hmac-sha2-256-96",              EVP_sha256,    96, 0}, // draft-dbider-sha2-mac-for-ssh-05, deleted at 06
	{HMAC_SHA2_512,     "hmac-sha2-512",                 EVP_sha512,    0,  0}, // RFC6668
//	{HMAC_SHA2_512_96,  "hmac-sha2-512-96",              EVP_sha512,    96, 0}, // draft-dbider-sha2-mac-for-ssh-05, deleted at 06
	{HMAC_SHA1_EtM,     "hmac-sha1-etm@openssh.com",     EVP_sha1,      0,  1},
	{HMAC_MD5_EtM,      "hmac-md5-etm@openssh.com",      EVP_md5,       0,  1},
	{HMAC_SHA1_96_EtM,  "hmac-sha1-96-etm@openssh.com",  EVP_sha1,      96, 1},
	{HMAC_MD5_96_EtM,   "hmac-md5-96-etm@openssh.com",   EVP_md5,       96, 1},
	{HMAC_RIPEMD160_EtM,"hmac-ripemd160-etm@openssh.com",EVP_ripemd160, 0,  1},
	{HMAC_SHA2_256_EtM, "hmac-sha2-256-etm@openssh.com", EVP_sha256,    0,  1},
	{HMAC_SHA2_512_EtM, "hmac-sha2-512-etm@openssh.com", EVP_sha512,    0,  1},
	{HMAC_IMPLICIT,     "<implicit>",                    EVP_md_null,   0,  0}, // for AEAD cipher
	{HMAC_NONE,         NULL,                            NULL,          0,  0},
};


char* get_ssh2_mac_name(const struct SSH2Mac *mac)
{
	if (mac) {
		return mac->name;
	}
	else {
		return "unknown";
	}
}

char* get_ssh2_mac_name_by_id(const SSH2MacId id)
{
	return get_ssh2_mac_name(get_ssh2_mac(id));
}

const EVP_MD* get_ssh2_mac_EVP_MD(const struct SSH2Mac *mac)
{
	if (mac) {
		return mac->evp_md();
	}
	else {
		return EVP_md_null();
	}
}

const struct SSH2Mac *get_ssh2_mac(SSH2MacId id)
{
	const struct SSH2Mac *ptr = ssh2_macs;

	while (ptr->name != NULL) {
		if (ptr->id == id) {
			return ptr;
		}
		ptr++;
	}

	return NULL;
}

int get_ssh2_mac_truncatebits(const struct SSH2Mac *mac)
{
	if (mac) {
		return mac->truncatebits;
	}
	else {
		return 0;
	}
}

int get_ssh2_mac_etm(const struct SSH2Mac *mac)
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
#if defined(LIBRESSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x30000000UL
		HMAC_RIPEMD160_EtM,
		HMAC_RIPEMD160,
#endif
		HMAC_MD5_EtM,
		HMAC_MD5,
		HMAC_NONE,
		HMAC_SHA1_96_EtM,
		HMAC_MD5_96_EtM,
		HMAC_SHA1_96,
		HMAC_MD5_96,
#if !defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000UL
		0, 0, // Dummy for HMAC_RIPEMD160_EtM, HMAC_RIPEMD160
#endif
		0, // Dummy for HMAC_SHA2_512_96,
		0, // Dummy for HMAC_SHA2_256_96,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

const struct SSH2Mac *choose_SSH2_mac_algorithm(char *server_proposal, char *my_proposal)
{
	char str_hmac[64];
	const struct SSH2Mac *ptr = ssh2_macs;

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

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
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
