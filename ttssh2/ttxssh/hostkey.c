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
#include "hostkey.h"
#include "kex.h"


struct ssh2_host_key_t {
	ssh_keyalgo algo;
	ssh_keytype type;
	int digest_type;
	char *name;
};

static const struct ssh2_host_key_t ssh2_host_key[] = {
	{KEY_ALGO_RSA1,     KEY_RSA1,     NID_sha1,   "ssh-rsa1"},            // for SSH1 only
	{KEY_ALGO_RSA,      KEY_RSA,      NID_sha1,   "ssh-rsa"},             // RFC4253
	{KEY_ALGO_DSA,      KEY_DSA,      NID_sha1,   "ssh-dss"},             // RFC4253
	{KEY_ALGO_ECDSA256, KEY_ECDSA256, NID_sha256, "ecdsa-sha2-nistp256"}, // RFC5656
	{KEY_ALGO_ECDSA384, KEY_ECDSA384, NID_sha384, "ecdsa-sha2-nistp384"}, // RFC5656
	{KEY_ALGO_ECDSA521, KEY_ECDSA521, NID_sha512, "ecdsa-sha2-nistp521"}, // RFC5656
	{KEY_ALGO_ED25519,  KEY_ED25519,  NID_sha512, "ssh-ed25519"},         // RDC8709
	{KEY_ALGO_RSASHA256,KEY_RSA,      NID_sha256, "rsa-sha2-256"},        // RFC8332
	{KEY_ALGO_RSASHA512,KEY_RSA,      NID_sha512, "rsa-sha2-512"},        // RFC8332
	{KEY_ALGO_UNSPEC,   KEY_UNSPEC,   NID_undef,  "ssh-unknown"},
	{KEY_ALGO_NONE,     KEY_NONE,     NID_undef,  NULL},
};

struct ssh_digest_t {
	digest_algorithm id;
	char *name;
};

/* NB. Indexed directly by algorithm number */
static const struct ssh_digest_t ssh_digests[] = {
	{ SSH_DIGEST_MD5,       "MD5" },
	{ SSH_DIGEST_RIPEMD160, "RIPEMD160" },
	{ SSH_DIGEST_SHA1,      "SHA1" },
	{ SSH_DIGEST_SHA256,    "SHA256" },
	{ SSH_DIGEST_SHA384,    "SHA384" },
	{ SSH_DIGEST_SHA512,    "SHA512" },
	{ SSH_DIGEST_MAX,       NULL },
};


ssh_keytype get_hostkey_type_from_name(char *name)
{
	if (strcmp(name, "rsa1") == 0) {
		return KEY_RSA1;
	} else if (strcmp(name, "rsa") == 0) {
		return KEY_RSA;
	} else if (strcmp(name, "dsa") == 0) {
		return KEY_DSA;
	} else if (strcmp(name, "ssh-rsa") == 0) {
		return KEY_RSA;
	} else if (strcmp(name, "ssh-dss") == 0) {
		return KEY_DSA;
	} else if (strcmp(name, "ecdsa-sha2-nistp256") == 0) {
		return KEY_ECDSA256;
	} else if (strcmp(name, "ecdsa-sha2-nistp384") == 0) {
		return KEY_ECDSA384;
	} else if (strcmp(name, "ecdsa-sha2-nistp521") == 0) {
		return KEY_ECDSA521;
	} else if (strcmp(name, "ssh-ed25519") == 0) {
		return KEY_ED25519;
	}
	return KEY_UNSPEC;
}

char* get_ssh2_hostkey_type_name(ssh_keytype type)
{
	const struct ssh2_host_key_t *ptr = ssh2_host_key;

	while (ptr->name != NULL) {
		if (type == ptr->type) {
			return ptr->name;
		}
		ptr++;
	}

	// not found.
	return "ssh-unknown";
}

char *get_ssh2_hostkey_type_name_from_key(Key *key)
{
	return get_ssh2_hostkey_type_name(key->type);
}

char* get_ssh2_keyalgo_name(ssh_keyalgo algo)
{
	const struct ssh2_host_key_t *ptr = ssh2_host_key;

	while (ptr->name != NULL) {
		if (algo == ptr->algo) {
			return ptr->name;
		}
		ptr++;
	}

	// not found.
	return "ssh-unknown";
}

ssh_keyalgo get_ssh2_keyalgo_from_name(const char *name)
{
	const struct ssh2_host_key_t *ptr = ssh2_host_key;

	while (ptr->name != NULL) {
		if (strcmp(name, ptr->name) == 0) {
			return ptr->algo;
		}
		ptr++;
	}

	// not found.
	return KEY_ALGO_UNSPEC;
}

int get_ssh2_key_hashtype(ssh_keyalgo algo)
{
	const struct ssh2_host_key_t *ptr = ssh2_host_key;

	while (ptr->name != NULL) {
		if (algo == ptr->algo) {
			return ptr->digest_type;
		}
		ptr++;
	}

	// not found.
	return NID_sha1;
}

ssh_keytype get_ssh2_keytype_from_keyalgo(ssh_keyalgo algo)
{
	const struct ssh2_host_key_t *ptr = ssh2_host_key;

	while (ptr->name != NULL) {
		if (algo == ptr->algo) {
			return ptr->type;
		}
		ptr++;
	}

	// not found.
	return KEY_UNSPEC;
}

const char* get_ssh2_keytype_name_from_keyalgo(ssh_keyalgo algo)
{
	return get_ssh2_hostkey_type_name(get_ssh2_keytype_from_keyalgo(algo));
}

char* get_digest_algorithm_name(digest_algorithm id)
{
	const struct ssh_digest_t *ptr = ssh_digests;

	while (ptr->name != NULL) {
		if (id == ptr->id) {
			return ptr->name;
		}
		ptr++;
	}

	// not found.
	return "unknown";
}

void normalize_host_key_order(char *buf)
{
	static char default_strings[] = {
		KEY_ALGO_ECDSA256,
		KEY_ALGO_ECDSA384,
		KEY_ALGO_ECDSA521,
		KEY_ALGO_ED25519,
		KEY_ALGO_RSASHA256,
		KEY_ALGO_RSASHA512,
		KEY_ALGO_RSA,
		KEY_ALGO_DSA,
		KEY_ALGO_NONE,
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

ssh_keyalgo choose_SSH2_host_key_algorithm(char *server_proposal, char *my_proposal)
{
	ssh_keytype type = KEY_UNSPEC;
	char str_keytype[20];
	const struct ssh2_host_key_t *ptr = ssh2_host_key;

	choose_SSH2_proposal(server_proposal, my_proposal, str_keytype, sizeof(str_keytype));

	return get_ssh2_keyalgo_from_name(str_keytype);
}

// Host Keyアルゴリズム優先順位に応じて、myproposal[]を書き換える。
// (2011.2.28 yutaka)
void SSH2_update_host_key_myproposal(PTInstVar pvar)
{
	static char buf[256]; // TODO: malloc()にすべき
	int index;
	int len, i;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	buf[0] = '\0';
	for (i = 0 ; pvar->settings.HostKeyOrder[i] != 0 ; i++) {
		index = pvar->settings.HostKeyOrder[i] - '0';
		if (index == KEY_NONE) // disabled line
			break;
		strncat_s(buf, sizeof(buf), get_ssh2_keyalgo_name(index), _TRUNCATE);
		strncat_s(buf, sizeof(buf), ",", _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_SERVER_HOST_KEY_ALGS] = buf; 
}
