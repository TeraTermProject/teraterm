/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004- TeraTerm Project
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
#include "ssh.h"
#include "ssherr.h"
#include "cipher.h"
#include "kex.h"

#include <openssl/evp.h>

#include "codeconv.h"

// from cipher-3des.c
extern const EVP_CIPHER* evp_ssh1_3des(void);

static const struct ssh2cipher ssh2_ciphers[] = {
	{SSH2_CIPHER_3DES_CBC,        "3des-cbc",         8, 24,    0, 0, 0, EVP_des_ede3_cbc},     // RFC4253
	{SSH2_CIPHER_AES128_CBC,      "aes128-cbc",      16, 16,    0, 0, 0, EVP_aes_128_cbc},      // RFC4253
	{SSH2_CIPHER_AES192_CBC,      "aes192-cbc",      16, 24,    0, 0, 0, EVP_aes_192_cbc},      // RFC4253
	{SSH2_CIPHER_AES256_CBC,      "aes256-cbc",      16, 32,    0, 0, 0, EVP_aes_256_cbc},      // RFC4253
	{SSH2_CIPHER_BLOWFISH_CBC,    "blowfish-cbc",     8, 16,    0, 0, 0, EVP_bf_cbc},           // RFC4253
	{SSH2_CIPHER_AES128_CTR,      "aes128-ctr",      16, 16,    0, 0, 0, EVP_aes_128_ctr},      // RFC4344
	{SSH2_CIPHER_AES192_CTR,      "aes192-ctr",      16, 24,    0, 0, 0, EVP_aes_192_ctr},      // RFC4344
	{SSH2_CIPHER_AES256_CTR,      "aes256-ctr",      16, 32,    0, 0, 0, EVP_aes_256_ctr},      // RFC4344
	{SSH2_CIPHER_ARCFOUR,         "arcfour",          8, 16,    0, 0, 0, EVP_rc4},              // RFC4253
	{SSH2_CIPHER_ARCFOUR128,      "arcfour128",       8, 16, 1536, 0, 0, EVP_rc4},              // RFC4345
	{SSH2_CIPHER_ARCFOUR256,      "arcfour256",       8, 32, 1536, 0, 0, EVP_rc4},              // RFC4345
	{SSH2_CIPHER_CAST128_CBC,     "cast128-cbc",      8, 16,    0, 0, 0, EVP_cast5_cbc},        // RFC4253
	{SSH2_CIPHER_3DES_CTR,        "3des-ctr",         8, 24,    0, 0, 0, evp_des3_ctr},         // RFC4344
	{SSH2_CIPHER_BLOWFISH_CTR,    "blowfish-ctr",     8, 32,    0, 0, 0, evp_bf_ctr},           // RFC4344
	{SSH2_CIPHER_CAST128_CTR,     "cast128-ctr",      8, 16,    0, 0, 0, evp_cast5_ctr},        // RFC4344
	{SSH2_CIPHER_CAMELLIA128_CBC, "camellia128-cbc", 16, 16,    0, 0, 0, EVP_camellia_128_cbc}, // draft-kanno-secsh-camellia-02
	{SSH2_CIPHER_CAMELLIA192_CBC, "camellia192-cbc", 16, 24,    0, 0, 0, EVP_camellia_192_cbc}, // draft-kanno-secsh-camellia-02
	{SSH2_CIPHER_CAMELLIA256_CBC, "camellia256-cbc", 16, 32,    0, 0, 0, EVP_camellia_256_cbc}, // draft-kanno-secsh-camellia-02
	{SSH2_CIPHER_CAMELLIA128_CTR, "camellia128-ctr", 16, 16,    0, 0, 0, EVP_camellia_128_ctr}, // draft-kanno-secsh-camellia-02
	{SSH2_CIPHER_CAMELLIA192_CTR, "camellia192-ctr", 16, 24,    0, 0, 0, EVP_camellia_192_ctr}, // draft-kanno-secsh-camellia-02
	{SSH2_CIPHER_CAMELLIA256_CTR, "camellia256-ctr", 16, 32,    0, 0, 0, EVP_camellia_256_ctr}, // draft-kanno-secsh-camellia-02
#ifdef WITH_CAMELLIA_PRIVATE
	{SSH2_CIPHER_CAMELLIA128_CBC, "camellia128-cbc@openssh.org", 16, 16, 0,  0,  0, EVP_camellia_128_cbc},
	{SSH2_CIPHER_CAMELLIA192_CBC, "camellia192-cbc@openssh.org", 16, 24, 0,  0,  0, EVP_camellia_192_cbc},
	{SSH2_CIPHER_CAMELLIA256_CBC, "camellia256-cbc@openssh.org", 16, 32, 0,  0,  0, EVP_camellia_256_cbc},
	{SSH2_CIPHER_CAMELLIA128_CTR, "camellia128-ctr@openssh.org", 16, 16, 0,  0,  0, evp_camellia_128_ctr},
	{SSH2_CIPHER_CAMELLIA192_CTR, "camellia192-ctr@openssh.org", 16, 24, 0,  0,  0, evp_camellia_128_ctr},
	{SSH2_CIPHER_CAMELLIA256_CTR, "camellia256-ctr@openssh.org", 16, 32, 0,  0,  0, evp_camellia_128_ctr},
#endif // WITH_CAMELLIA_PRIVATE
	{SSH2_CIPHER_AES128_GCM,      "aes128-gcm@openssh.com",      16, 16, 0, 12, 16, EVP_aes_128_gcm}, // not RFC5647, PROTOCOL of OpenSSH
	{SSH2_CIPHER_AES256_GCM,      "aes256-gcm@openssh.com",      16, 32, 0, 12, 16, EVP_aes_256_gcm}, // not RFC5647, PROTOCOL of OpenSSH
	{SSH2_CIPHER_CHACHAPOLY,      "chacha20-poly1305@openssh.com",  8, 64, 0, 0, 16, EVP_enc_null},
	{SSH_CIPHER_NONE,             "none",             8,  0,    0, 0, 0, EVP_enc_null},         // for no passphrase key file
	{SSH_CIPHER_3DES,             "3des",             8, 16,    0, 0, 0, evp_ssh1_3des},        // for RSA1 key file
};


int get_cipher_id(const struct ssh2cipher *cipher)
{
	if (cipher) {
		return cipher->id;
	}
	else {
		return 0;
	}
}

u_int get_cipher_block_size(const struct ssh2cipher *cipher)
{
	u_int blocksize = 0;
	
	if (cipher) {
		blocksize = cipher->block_size;
	}

	return max(blocksize, 8);
}

u_int get_cipher_key_len(const struct ssh2cipher *cipher)
{
	if (cipher) {
		return cipher->key_len;
	}
	else {
		return 0;
	}
}

u_int get_cipher_discard_len(const struct ssh2cipher *cipher)
{
	if (cipher) {
		return cipher->discard_len;
	}
	else {
		return 0;
	}
}

u_int get_cipher_iv_len(const struct ssh2cipher *cipher)
{
	if (cipher) {
		if (cipher->iv_len != 0 || cipher->id == SSH2_CIPHER_CHACHAPOLY) {
			return cipher->iv_len;
		}
		else {
			return cipher->block_size;
		}
	}
	else {
		return 8; // block_size
	}
}

u_int get_cipher_auth_len(const struct ssh2cipher *cipher)
{
	if (cipher) {
		return cipher->auth_len;
	}
	else {
		return 0;
	}
}

const EVP_CIPHER *get_cipher_EVP_CIPHER(const struct ssh2cipher *cipher)
{
	if (cipher) {
		return cipher->func();
	}
	else {
		return EVP_enc_null();
	}
}

char *get_cipher_string(const struct ssh2cipher *cipher)
{
	if (cipher) {
		return cipher->name;
	}
	else {
		return "unknown";
	}
}

// 暗号アルゴリズム名から検索する。
const struct ssh2cipher *get_cipher_by_name(char *name)
{
	const struct ssh2cipher *ptr = ssh2_ciphers;

	if (name == NULL || name[0] == '\0')
		return NULL;

	while (ptr->name != NULL) {
		if (strcmp(ptr->name, name) == 0) {
			return ptr;
		}
		ptr++;
	}

	// not found.
	return NULL;
}

// 表示名
char *get_cipher_name(int cipher_id)
{
	switch (cipher_id) {
	case SSH_CIPHER_NONE:
		return "None";
	case SSH_CIPHER_3DES:
		return "3DES (168 key bits)";
	case SSH_CIPHER_DES:
		return "DES (56 key bits)";
	case SSH_CIPHER_BLOWFISH:
		return "Blowfish (256 key bits)";

	// SSH2 
	case SSH2_CIPHER_3DES_CBC:
		return "3des-cbc";
	case SSH2_CIPHER_AES128_CBC:
		return "aes128-cbc";
	case SSH2_CIPHER_AES192_CBC:
		return "aes192-cbc";
	case SSH2_CIPHER_AES256_CBC:
		return "aes256-cbc";
	case SSH2_CIPHER_BLOWFISH_CBC:
		return "blowfish-cbc";
	case SSH2_CIPHER_AES128_CTR:
		return "aes128-ctr";
	case SSH2_CIPHER_AES192_CTR:
		return "aes192-ctr";
	case SSH2_CIPHER_AES256_CTR:
		return "aes256-ctr";
	case SSH2_CIPHER_ARCFOUR:
		return "arcfour";
	case SSH2_CIPHER_ARCFOUR128:
		return "arcfour128";
	case SSH2_CIPHER_ARCFOUR256:
		return "arcfour256";
	case SSH2_CIPHER_CAST128_CBC:
		return "cast-128-cbc";
	case SSH2_CIPHER_3DES_CTR:
		return "3des-ctr";
	case SSH2_CIPHER_BLOWFISH_CTR:
		return "blowfish-ctr";
	case SSH2_CIPHER_CAST128_CTR:
		return "cast-128-ctr";
	case SSH2_CIPHER_CAMELLIA128_CBC:
		return "camellia128-cbc";
	case SSH2_CIPHER_CAMELLIA192_CBC:
		return "camellia192-cbc";
	case SSH2_CIPHER_CAMELLIA256_CBC:
		return "camellia256-cbc";
	case SSH2_CIPHER_CAMELLIA128_CTR:
		return "camellia128-ctr";
	case SSH2_CIPHER_CAMELLIA192_CTR:
		return "camellia192-ctr";
	case SSH2_CIPHER_CAMELLIA256_CTR:
		return "camellia256-ctr";
	case SSH2_CIPHER_AES128_GCM:
		return "aes128-gcm@openssh.com";
	case SSH2_CIPHER_AES256_GCM:
		return "aes256-gcm@openssh.com";
	case SSH2_CIPHER_CHACHAPOLY:
		return "chacha20-poly1305@openssh.com(SSH2)";

	default:
		return "Unknown";
	}
}

// リストボックス表示名
wchar_t *get_listbox_cipher_nameW(int cipher_id, PTInstVar pvar)
{
	typedef struct {
		int no;
		const char *nameA;
	} list_t;
	static const list_t list[] = {
		{ SSH_CIPHER_3DES, "3DES(SSH1)" },
		{ SSH_CIPHER_DES, "DES(SSH1)" },
		{ SSH_CIPHER_BLOWFISH, "Blowfish(SSH1)" },
		{ SSH2_CIPHER_AES128_CBC, "aes128-cbc(SSH2)" },
		{ SSH2_CIPHER_AES192_CBC, "aes192-cbc(SSH2)" },
		{ SSH2_CIPHER_AES256_CBC, "aes256-cbc(SSH2)" },
		{ SSH2_CIPHER_3DES_CBC, "3des-cbc(SSH2)" },
		{ SSH2_CIPHER_BLOWFISH_CBC, "blowfish-cbc(SSH2)" },
		{ SSH2_CIPHER_AES128_CTR, "aes128-ctr(SSH2)" },
		{ SSH2_CIPHER_AES192_CTR, "aes192-ctr(SSH2)" },
		{ SSH2_CIPHER_AES256_CTR, "aes256-ctr(SSH2)" },
		{ SSH2_CIPHER_ARCFOUR, "arcfour(SSH2)" },
		{ SSH2_CIPHER_ARCFOUR128, "arcfour128(SSH2)" },
		{ SSH2_CIPHER_ARCFOUR256, "arcfour256(SSH2)" },
		{ SSH2_CIPHER_CAST128_CBC, "cast128-cbc(SSH2)" },
		{ SSH2_CIPHER_3DES_CTR, "3des-ctr(SSH2)" },
		{ SSH2_CIPHER_BLOWFISH_CTR, "blowfish-ctr(SSH2)" },
		{ SSH2_CIPHER_CAST128_CTR, "cast128-ctr(SSH2)" },
		{ SSH2_CIPHER_CAMELLIA128_CBC, "camellia128-cbc(SSH2)" },
		{ SSH2_CIPHER_CAMELLIA192_CBC, "camellia192-cbc(SSH2)" },
		{ SSH2_CIPHER_CAMELLIA256_CBC, "camellia256-cbc(SSH2)" },
		{ SSH2_CIPHER_CAMELLIA128_CTR, "camellia128-ctr(SSH2)" },
		{ SSH2_CIPHER_CAMELLIA192_CTR, "camellia192-ctr(SSH2)" },
		{ SSH2_CIPHER_CAMELLIA256_CTR, "camellia256-ctr(SSH2)" },
		{ SSH2_CIPHER_AES128_GCM, "aes128-gcm@openssh.com(SSH2)" },
		{ SSH2_CIPHER_AES256_GCM, "aes256-gcm@openssh.com(SSH2)" },
		{ SSH2_CIPHER_CHACHAPOLY, "chacha20-poly1305@openssh.com(SSH2)" },
	};
	int i;
	const list_t *p = list;

	if (cipher_id == SSH_CIPHER_NONE) {
		wchar_t uimsg[MAX_UIMSG];
		UTIL_get_lang_msgW("DLG_SSHSETUP_CIPHER_BORDER", pvar,
						   L"<ciphers below this line are disabled>", uimsg);
		return _wcsdup(uimsg);
	}
	for (i = 0; i < _countof(list); p++,i++) {
		if (p->no == cipher_id) {
			return ToWcharA(p->nameA);
		}
	}
	return NULL;
}

/*
 * Remove unsupported cipher or duplicated cipher.
 * Add unspecified ciphers at the end of list.
 */
void normalize_cipher_order(char *buf)
{
	/* SSH_CIPHER_NONE means that all ciphers below that one are disabled.
	   We *never* allow no encryption. */
	static char default_strings[] = {
		SSH2_CIPHER_AES256_GCM,
		SSH2_CIPHER_CAMELLIA256_CTR,
		SSH2_CIPHER_CHACHAPOLY,
		SSH2_CIPHER_AES256_CTR,
		SSH2_CIPHER_CAMELLIA256_CBC,
		SSH2_CIPHER_AES256_CBC,
		SSH2_CIPHER_CAMELLIA192_CTR,
		SSH2_CIPHER_AES192_CTR,
		SSH2_CIPHER_CAMELLIA192_CBC,
		SSH2_CIPHER_AES192_CBC,
		SSH2_CIPHER_AES128_GCM,
		SSH2_CIPHER_CAMELLIA128_CTR,
		SSH2_CIPHER_AES128_CTR,
		SSH2_CIPHER_CAMELLIA128_CBC,
		SSH2_CIPHER_AES128_CBC,
#if defined(LIBRESSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x30000000UL
		SSH2_CIPHER_3DES_CTR,
#endif
		SSH2_CIPHER_3DES_CBC,
#if defined(LIBRESSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x30000000UL
		SSH2_CIPHER_BLOWFISH_CTR,
		SSH2_CIPHER_BLOWFISH_CBC,
		SSH2_CIPHER_CAST128_CTR,
		SSH2_CIPHER_CAST128_CBC,
#endif
		SSH_CIPHER_3DES,
		SSH_CIPHER_NONE,
#if defined(LIBRESSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x30000000UL
		SSH2_CIPHER_ARCFOUR256,
		SSH2_CIPHER_ARCFOUR128,
		SSH2_CIPHER_ARCFOUR,
#endif
		SSH_CIPHER_BLOWFISH,
		SSH_CIPHER_DES,
#if !defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000UL
		0, 0, 0, // Dummy for SSH2_CIPHER_3DES_CTR, SSH2_CIPHER_BLOWFISH_CTR, SSH2_CIPHER_BLOWFISH_CBC,
		0, 0,    // Dummy for SSH2_CIPHER_CAST128_CTR, SSH2_CIPHER_CAST128_CBC
		0, 0, 0, // Dummy for SSH2_CIPHER_ARCFOUR256, SSH2_CIPHER_ARCFOUR128, SSH2_CIPHER_ARCFOUR
#endif
		0, 0, 0 // Dummy for SSH_CIPHER_IDEA, SSH_CIPHER_TSS, SSH_CIPHER_RC4
	};

	normalize_generic_order(buf, default_strings, NUM_ELEM(default_strings));
}

const struct ssh2cipher *choose_SSH2_cipher_algorithm(char *server_proposal, char *my_proposal)
{
	char str_cipher[32];
	const struct ssh2cipher *ptr = ssh2_ciphers;

	choose_SSH2_proposal(server_proposal, my_proposal, str_cipher, sizeof(str_cipher));
	return get_cipher_by_name(str_cipher);
}

void SSH2_update_cipher_myproposal(PTInstVar pvar)
{
	static char buf[512]; // TODO: malloc()にすべき
	int cipher;
	int len, i;
	char *c_str;

	// 通信中には呼ばれないはずだが、念のため。(2006.6.26 maya)
	if (pvar->socket != INVALID_SOCKET) {
		return;
	}

	// 暗号アルゴリズム優先順位に応じて、myproposal[]を書き換える。(2004.11.6 yutaka)
	buf[0] = '\0';
	for (i = 0 ; pvar->settings.CipherOrder[i] != 0 ; i++) {
		cipher = pvar->settings.CipherOrder[i] - '0';
		if (cipher == 0) // disabled line
			break;
		switch (cipher) {
			case SSH2_CIPHER_3DES_CBC:
				c_str = "3des-cbc,";
				break;
			case SSH2_CIPHER_3DES_CTR:
				c_str = "3des-ctr,";
				break;
			case SSH2_CIPHER_BLOWFISH_CBC:
				c_str = "blowfish-cbc,";
				break;
			case SSH2_CIPHER_BLOWFISH_CTR:
				c_str = "blowfish-ctr,";
				break;
			case SSH2_CIPHER_AES128_CBC:
				c_str = "aes128-cbc,";
				break;
			case SSH2_CIPHER_AES192_CBC:
				c_str = "aes192-cbc,";
				break;
			case SSH2_CIPHER_AES256_CBC:
				c_str = "aes256-cbc,";
				break;
			case SSH2_CIPHER_AES128_CTR:
				c_str = "aes128-ctr,";
				break;
			case SSH2_CIPHER_AES192_CTR:
				c_str = "aes192-ctr,";
				break;
			case SSH2_CIPHER_AES256_CTR:
				c_str = "aes256-ctr,";
				break;
			case SSH2_CIPHER_ARCFOUR:
				c_str = "arcfour,";
				break;
			case SSH2_CIPHER_ARCFOUR128:
				c_str = "arcfour128,";
				break;
			case SSH2_CIPHER_ARCFOUR256:
				c_str = "arcfour256,";
				break;
			case SSH2_CIPHER_CAST128_CBC:
				c_str = "cast128-cbc,";
				break;
			case SSH2_CIPHER_CAST128_CTR:
				c_str = "cast128-ctr,";
				break;
#ifdef WITH_CAMELLIA_PRIVATE
			case SSH2_CIPHER_CAMELLIA128_CBC:
				c_str = "camellia128-cbc,camellia128-cbc@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CBC:
				c_str = "camellia192-cbc,camellia192-cbc@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CBC:
				c_str = "camellia256-cbc,camellia256-cbc@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA128_CTR:
				c_str = "camellia128-ctr,camellia128-ctr@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CTR:
				c_str = "camellia192-ctr,camellia192-ctr@openssh.org,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CTR:
				c_str = "camellia256-ctr,camellia256-ctr@openssh.org,";
				break;
#endif // WITH_CAMELLIA_PRIVATE
			case SSH2_CIPHER_CAMELLIA128_CBC:
				c_str = "camellia128-cbc,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CBC:
				c_str = "camellia192-cbc,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CBC:
				c_str = "camellia256-cbc,";
				break;
			case SSH2_CIPHER_CAMELLIA128_CTR:
				c_str = "camellia128-ctr,";
				break;
			case SSH2_CIPHER_CAMELLIA192_CTR:
				c_str = "camellia192-ctr,";
				break;
			case SSH2_CIPHER_CAMELLIA256_CTR:
				c_str = "camellia256-ctr,";
				break;
			case SSH2_CIPHER_AES128_GCM:
				c_str = "aes128-gcm@openssh.com,";
				break;
			case SSH2_CIPHER_AES256_GCM:
				c_str = "aes256-gcm@openssh.com,";
				break;
			case SSH2_CIPHER_CHACHAPOLY:
				c_str = "chacha20-poly1305@openssh.com,";
				break;
			default:
				continue;
		}
		strncat_s(buf, sizeof(buf), c_str, _TRUNCATE);
	}
	len = strlen(buf);
	if (len > 0)
		buf[len - 1] = '\0';  // get rid of comma
	myproposal[PROPOSAL_ENC_ALGS_CTOS] = buf;  // Client To Server
	myproposal[PROPOSAL_ENC_ALGS_STOC] = buf;  // Server To Client
}


//
// SSH2用アルゴリズムの初期化
//
int cipher_init_SSH2(
	struct sshcipher_ctx **ccp, const struct ssh2cipher *cipher,
	const u_char *key, u_int keylen,
	const u_char *iv, u_int ivlen,
	int do_encrypt,
	PTInstVar pvar)
{
	struct sshcipher_ctx *cc = NULL;
	int ret = SSH_ERR_INTERNAL_ERROR;
	const EVP_CIPHER *type;
	int klen;
	unsigned char *junk = NULL, *discard = NULL;
	char tmp[80];

	*ccp = NULL;
	if ((cc = calloc(sizeof(*cc), 1)) == NULL) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 1);
		notify_fatal_error(pvar, tmp, TRUE);
		return SSH_ERR_ALLOC_FAIL;
	}

	if (keylen < cipher->key_len) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 2);
		notify_fatal_error(pvar, tmp, TRUE);
		ret = SSH_ERR_INVALID_ARGUMENT;
		goto out;
	}
	if (iv != NULL && ivlen < get_cipher_iv_len(cipher)) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 3);
		notify_fatal_error(pvar, tmp, TRUE);
		ret = SSH_ERR_INVALID_ARGUMENT;
		goto out;
	}

	cc->cipher = cipher;
	if (cipher->id == SSH2_CIPHER_CHACHAPOLY) {
		cc->cp_ctx = chachapoly_new(key, keylen);
		ret = cc->cp_ctx != NULL ? 0 : SSH_ERR_INVALID_ARGUMENT;
		if (ret == SSH_ERR_INVALID_ARGUMENT) {
			UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 4);
			notify_fatal_error(pvar, tmp, TRUE);
		}
		goto out;
	}
	type = (*cipher->func)();
	if ((cc->evp = EVP_CIPHER_CTX_new()) == NULL) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 5);
		notify_fatal_error(pvar, tmp, TRUE);
		ret = SSH_ERR_ALLOC_FAIL;
		goto out;
	}
	if (EVP_CipherInit(cc->evp, type, NULL, (u_char *)iv, (do_encrypt == CIPHER_ENCRYPT)) == 0) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 6);
		notify_fatal_error(pvar, tmp, TRUE);
		ret = SSH_ERR_LIBCRYPTO_ERROR;
		goto out;
	}
	if (get_cipher_auth_len(cipher) &&
	    !EVP_CIPHER_CTX_ctrl(cc->evp, EVP_CTRL_GCM_SET_IV_FIXED, -1, (u_char *)iv)) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 7);
		notify_fatal_error(pvar, tmp, TRUE);
		ret = SSH_ERR_LIBCRYPTO_ERROR;
		goto out;
	}
	klen = EVP_CIPHER_CTX_key_length(cc->evp);
	if (klen > 0 && keylen != (u_int)klen) {
		if (EVP_CIPHER_CTX_set_key_length(cc->evp, keylen) == 0) {
			UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 8);
			notify_fatal_error(pvar, tmp, TRUE);
			ret = SSH_ERR_LIBCRYPTO_ERROR;
			goto out;
		}
	}
	if (EVP_CipherInit(cc->evp, NULL, (u_char *)key, NULL, -1) == 0) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 9);
		notify_fatal_error(pvar, tmp, TRUE);
		ret = SSH_ERR_LIBCRYPTO_ERROR;
		goto out;
	}

	if (cipher->discard_len > 0) {
		junk = malloc(cipher->discard_len);
		discard = malloc(cipher->discard_len);
		if (junk == NULL || discard == NULL ||
		    EVP_Cipher(cc->evp, discard, junk, cipher->discard_len) == 0) {
			UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar, "Cipher initialize error(%d)");
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 10);
			notify_fatal_error(pvar, tmp, TRUE);
		}
		else {
			SecureZeroMemory(discard, cipher->discard_len);
		}
		free(junk);
		free(discard);
	}
	ret = 0;

out:
	if (ret == 0) {
		*ccp = cc;
	}
	else {
		if (cc != NULL) {
			EVP_CIPHER_CTX_free(cc->evp);
			SecureZeroMemory(cc, sizeof(*cc));
		}
	}
	return ret;
}

//
// SSH2用アルゴリズムの破棄
///
void cipher_free_SSH2(struct sshcipher_ctx *cc)
{
	if (cc == NULL)
		return;
	if (cc->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
		chachapoly_free(cc->cp_ctx);
		cc->cp_ctx = NULL;
	}
	EVP_CIPHER_CTX_free(cc->evp);
	cc->evp = NULL;
	SecureZeroMemory(cc, sizeof(*cc));
}
