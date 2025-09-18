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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"
#include "keyfiles.h"
#include "keyfiles-putty.h"
#include "key.h"
#include "hostkey.h"
#include "argon2.h"

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <openssl/err.h>

#include "cipher.h"

static char ID_string[] = "SSH PRIVATE KEY FILE FORMAT 1.1\n";

typedef struct keyfile_header {
	ssh2_keyfile_type type;
	char *header;
} keyfile_header_t;

static keyfile_header_t keyfile_headers[] = {
	{SSH2_KEYFILE_TYPE_OPENSSH, "-----BEGIN RSA PRIVATE KEY-----"},
	{SSH2_KEYFILE_TYPE_OPENSSH, "-----BEGIN DSA PRIVATE KEY-----"},
	{SSH2_KEYFILE_TYPE_OPENSSH, "-----BEGIN EC PRIVATE KEY-----"},
	{SSH2_KEYFILE_TYPE_OPENSSH, "-----BEGIN ENCRYPTED PRIVATE KEY-----"},
	{SSH2_KEYFILE_TYPE_OPENSSH, "-----BEGIN PRIVATE KEY-----"},
	{SSH2_KEYFILE_TYPE_OPENSSH, "-----BEGIN OPENSSH PRIVATE KEY-----"},
	{SSH2_KEYFILE_TYPE_PUTTY,   "PuTTY-User-Key-File-2"},
	{SSH2_KEYFILE_TYPE_PUTTY,   "PuTTY-User-Key-File-3"},
	{SSH2_KEYFILE_TYPE_SECSH,   "---- BEGIN SSH2 ENCRYPTED PRIVATE KEY ----"},
	{SSH2_KEYFILE_TYPE_NONE,    NULL},
};

static BIGNUM *get_bignum(unsigned char *bytes)
{
	int bits = get_ushort16_MSBfirst(bytes);

	return BN_bin2bn(bytes + 2, (bits + 7) / 8, NULL);
}

/* normalize the RSA key by precomputing various bits of it.
   This code is adapted from libeay's rsa_gen.c
   It's needed to work around "issues" with LIBEAY/RSAREF.
   If this function returns 0, then something went wrong and the
   key must be discarded. */
static BOOL normalize_key(RSA *key)
{
	BOOL OK = FALSE;
	BIGNUM *r = BN_new();
	BN_CTX *ctx = BN_CTX_new();
	BIGNUM *e, *n, *d, *dmp1, *dmq1, *iqmp, *p, *q;

	e = n = d = dmp1 = dmq1 = iqmp = p = q = NULL;

	RSA_get0_key(key, &n, &e, &d);
	RSA_get0_factors(key, &p, &q);
	RSA_get0_crt_params(key, &dmp1, &dmq1, &iqmp);

	if (BN_cmp(p, q) < 0) {
		BN_swap(p, q);
	}

	if (r != NULL && ctx != NULL) {
		dmp1 = BN_new();
		dmq1 = BN_new();
		iqmp = BN_mod_inverse(NULL, q, p, ctx);
		RSA_set0_crt_params(key, dmp1, dmq1, iqmp);

		if (dmp1 != NULL && dmq1 != NULL && iqmp != NULL) {
			OK = BN_sub(r, p, BN_value_one())
			  && BN_mod(dmp1, d, r, ctx)
			  && BN_sub(r, q, BN_value_one())
			  && BN_mod(dmq1, d, r, ctx);
		}
	}

	BN_free(r);
	BN_CTX_free(ctx);

	return OK;
}

static RSA *read_RSA_private_key(PTInstVar pvar,
                                 const wchar_t * relative_name,
                                 char * passphrase,
                                 BOOL * invalid_passphrase,
                                 BOOL is_auto_login)
{
	wchar_t *filename;
	int fd;
	unsigned int length, amount_read;
	unsigned char *keyfile_data;
	unsigned int index;
	int cipher;
	RSA *key;
	unsigned int E_index, N_index, D_index, U_index, P_index, Q_index = 0;
	BIGNUM *e, *n, *d, *p, *q;

	*invalid_passphrase = FALSE;

	filename = get_teraterm_dir_relative_nameW(relative_name);

	fd = _wopen(filename, _O_RDONLY | _O_SEQUENTIAL | _O_BINARY);
	free(filename);
	if (fd == -1) {
		if (errno == ENOENT) {
			UTIL_get_lang_msg("MSG_KEYFILES_READ_ENOENT_ERROR", pvar,
			                  "An error occurred while trying to read the key file.\n"
			                  "The specified filename does not exist.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
		} else {
			UTIL_get_lang_msg("MSG_KEYFILES_READ_ERROR", pvar,
			                  "An error occurred while trying to read the key file.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}
		return NULL;
	}

	length = (int) _lseek(fd, 0, SEEK_END);
	_lseek(fd, 0, SEEK_SET);

	if (length >= 0 && length < 0x7FFFFFFF) {
		keyfile_data = malloc(length + 1);
		if (keyfile_data == NULL) {
			UTIL_get_lang_msg("MSG_KEYFILES_READ_ALLOC_ERROR", pvar,
			                  "Memory ran out while trying to allocate space to read the key file.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
			_close(fd);
			return NULL;
		}
	} else {
		UTIL_get_lang_msg("MSG_KEYFILES_READ_ERROR", pvar,
		                  "An error occurred while trying to read the key file.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		_close(fd);
		return NULL;
	}

	amount_read = _read(fd, keyfile_data, length);
	/* terminate it with 0 so that the strncmp below is guaranteed not to
	   crash */
	keyfile_data[length] = 0;

	_close(fd);

	if (amount_read != length) {
		UTIL_get_lang_msg("MSG_KEYFILES_READ_ERROR", pvar,
		                  "An error occurred while trying to read the key file.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}

	if (strcmp(keyfile_data, ID_string) != 0) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_NOTCONTAIN_ERROR", pvar,
		                  "The specified key file does not contain an SSH private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}

	index = NUM_ELEM(ID_string);

	if (length < index + 9) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}

	cipher = keyfile_data[index];
	/* skip reserved int32, public key bits int32 */
	index += 9;
	/* skip public key e and n mp_ints */
	if (length < index + 2) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	N_index = index;
	index += (get_ushort16_MSBfirst(keyfile_data + index) + 7) / 8 + 2;
	if (length < index + 2) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	E_index = index;
	index += (get_ushort16_MSBfirst(keyfile_data + index) + 7) / 8 + 2;
	/* skip comment */
	if (length < index + 4) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	index += get_uint32_MSBfirst(keyfile_data + index) + 4;

	if (length < index + 6) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	if (cipher != SSH_CIPHER_NONE) {
		if ((length - index) % 8 != 0) {
			UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_LENGTH_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the passphrase.\n"
			                  "The file does not have the correct length.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
			free(keyfile_data);
			return NULL;
		}
		if (!CRYPT_passphrase_decrypt
			(cipher, passphrase, keyfile_data + index, length - index)) {
			UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_NOCIPHER_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the passphrase.\n"
			                  "The cipher type used to encrypt the file is not supported by TTSSH for this purpose.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
			free(keyfile_data);
			return NULL;
		}
	}

	if (keyfile_data[index] != keyfile_data[index + 2]
	 || keyfile_data[index + 1] != keyfile_data[index + 3]) {
		*invalid_passphrase = TRUE;
		if (is_auto_login) {
			UTIL_get_lang_msg("MSG_KEYFILES_PASSPHRASE_EMPTY_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the empty passphrase.\n"
			                  "For auto-login, you must create a key file with no passphrase.\n"
			                  "BEWARE: This means the key can easily be stolen.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}
		else {
			UTIL_get_lang_msg("MSG_KEYFILES_PASSPHRASE_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the passphrase.\n"
			                  "The passphrase is incorrect.");
			notify_nonfatal_error(pvar, pvar->UIMsg);
		}
		SecureZeroMemory(keyfile_data, length);
		free(keyfile_data);
		return NULL;
	}
	index += 4;

	D_index = index;
	if (length >= D_index + 2) {
		U_index =
			D_index + (get_ushort16_MSBfirst(keyfile_data + D_index) +
					   7) / 8 + 2;
		if (length >= U_index + 2) {
			P_index =
				U_index + (get_ushort16_MSBfirst(keyfile_data + U_index) +
						   7) / 8 + 2;
			if (length >= P_index + 2) {
				Q_index =
					P_index +
					(get_ushort16_MSBfirst(keyfile_data + P_index) +
					 7) / 8 + 2;
			}
		}
	}
	if (Q_index == 0
	 || length <
	    Q_index + (get_ushort16_MSBfirst(keyfile_data + Q_index) + 7) / 8 + 2) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		              "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->UIMsg);
		SecureZeroMemory(keyfile_data, length);
		free(keyfile_data);
		return NULL;
	}

	key = RSA_new();
	n = get_bignum(keyfile_data + N_index);
	e = get_bignum(keyfile_data + E_index);
	d = get_bignum(keyfile_data + D_index);
	RSA_set0_key(key, n, e, d);
	p = get_bignum(keyfile_data + P_index);
	q = get_bignum(keyfile_data + Q_index);
	RSA_set0_factors(key, p, q);

	if (!normalize_key(key)) {
		UTIL_get_lang_msg("MSG_KEYFILES_CRYPTOLIB_ERROR", pvar,
		                  "Error in cryptography library.\n"
		                  "Perhaps the stored key is invalid.");
		notify_nonfatal_error(pvar, pvar->UIMsg);

		RSA_free(key);
		key = NULL;
	}

	SecureZeroMemory(keyfile_data, length);
	free(keyfile_data);
	return key;
}

Key *KEYFILES_read_private_key(PTInstVar pvar,
                               const wchar_t * relative_name,
                               char * passphrase,
                               BOOL * invalid_passphrase,
                               BOOL is_auto_login)
{
	RSA *RSA_key = read_RSA_private_key(pvar, relative_name,
	                                    passphrase, invalid_passphrase,
	                                    is_auto_login);

	if (RSA_key == NULL) {
		return FALSE;
	} else {
		Key *result = (Key *) malloc(sizeof(Key));

		// フリーするときに 0 かどうかで判別するため追加。(2004.12.20 yutaka)
		ZeroMemory(result, sizeof(Key));

		result->rsa = RSA_key;
		return result;
	}
}


//
// SSH2
//

// bcrypt KDF 形式で読む
// based on key_parse_private2() @ OpenSSH 6.5
static Key *read_SSH2_private2_key(PTInstVar pvar,
                                   FILE * fp,
                                   char * passphrase,
                                   BOOL * invalid_passphrase,
                                   BOOL is_auto_login,
                                   char *errmsg,
                                   int errmsg_len)
{
	/* (A)
	 * buffer_consume系関数を使う場合は、buffer_lenとbuffer_ptrが使えないので、
	 *   buffer_len -> buffer_remain_len
	 *   buffer_ptr -> buffer_tail_ptr
	 * を代替使用すること。
	 */
	buffer_t *blob = NULL;
	buffer_t *b = NULL;
	buffer_t *kdf = NULL;
	buffer_t *encoded = NULL;
	buffer_t *copy_consumed = NULL;     // (A)
	Key *keyfmt = NULL;
	unsigned char buf[1024];
	unsigned char *cp, last, pad;
	char *ciphername = NULL, *kdfname = NULL, *kdfp = NULL, *key = NULL, *salt = NULL, *comment = NULL;
	unsigned int len, klen, nkeys, blocksize, keylen, ivlen, slen, rounds;
	unsigned int check1, check2, m1len, m2len;
	int dlen, i;
	const struct ssh2cipher *cipher;
	size_t authlen;
	struct sshcipher_ctx *cc = NULL;
	int ret;

	blob = buffer_init();
	b = buffer_init();
	kdf = buffer_init();
	encoded = buffer_init();
	copy_consumed = buffer_init();

	if (blob == NULL || b == NULL || kdf == NULL || encoded == NULL || copy_consumed == NULL)
		goto error;

	// ファイルをすべて読み込む
	for (;;) {
		len = fread(buf, 1, sizeof(buf), fp);
		buffer_append(blob, buf, len);
		if (buffer_len(blob) > MAX_KEY_FILE_SIZE) {
			logprintf(LOG_LEVEL_WARNING, "%s: key file too large.", __FUNCTION__);
			goto error;
		}
		if (len < sizeof(buf))
			break;
	}

	/* base64 decode */
	m1len = sizeof(MARK_BEGIN) - 1;
	m2len = sizeof(MARK_END) - 1;
	cp = buffer_ptr(blob);
	len = buffer_len(blob);
	if (len < m1len || memcmp(cp, MARK_BEGIN, m1len)) {
		logprintf(LOG_LEVEL_VERBOSE, "%s: missing begin marker", __FUNCTION__);
		goto error;
	}
	cp += m1len;
	len -= m1len;
	while (len) {
		if (*cp != '\n' && *cp != '\r')
			buffer_put_char(encoded, *cp);
		last = *cp;
		len--;
		cp++;
		if (last == '\n') {
			if (len >= m2len && !memcmp(cp, MARK_END, m2len)) {
				buffer_put_char(encoded, '\0');
				break;
			}
		}
	}
	if (!len) {
		logprintf(LOG_LEVEL_VERBOSE, "%s: no end marker", __FUNCTION__);
		goto error;
	}

	// ファイルのスキャンが終わったので、base64 decodeする。
	len = buffer_len(encoded);
	if ((cp = buffer_append_space(copy_consumed, len)) == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: buffer_append_space", __FUNCTION__);
		goto error;
	}
	if ((dlen = b64decode(cp, len, buffer_ptr(encoded))) < 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: base64 decode failed", __FUNCTION__);
		goto error;
	}
	if ((unsigned int)dlen > len) {
		logprintf(LOG_LEVEL_ERROR, "%s: crazy base64 length %d > %u", __FUNCTION__, dlen, len);
		goto error;
	}

	buffer_consume_end(copy_consumed, len - dlen);
	if (buffer_remain_len(copy_consumed) < sizeof(AUTH_MAGIC) ||
	    memcmp(buffer_tail_ptr(copy_consumed), AUTH_MAGIC, sizeof(AUTH_MAGIC))) {
		logprintf(LOG_LEVEL_ERROR, "%s: bad magic", __FUNCTION__);
		goto error;
	}
	buffer_consume(copy_consumed, sizeof(AUTH_MAGIC));

	/*
	 * デコードしたデータを解析する。
	 */
	// 暗号化アルゴリズムの名前
	ciphername = buffer_get_string_msg(copy_consumed, NULL);
	cipher = get_cipher_by_name(ciphername);
	if (cipher == NULL && strcmp(ciphername, "none") != 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: unknown cipher name", __FUNCTION__);
		goto error;
	}
	// パスフレーズのチェック。暗号化が none でない場合は空のパスワードを認めない。
	if ((passphrase == NULL || strlen(passphrase) == 0) &&
	    strcmp(ciphername, "none") != 0) {
		/* passphrase required */
		goto error;
	}

	kdfname = buffer_get_string_msg(copy_consumed, NULL);
	if (kdfname == NULL ||
	    (!strcmp(kdfname, "none") && !strcmp(kdfname, KDFNAME))) {
		logprintf(LOG_LEVEL_ERROR, "%s: unknown kdf name", __FUNCTION__ );
		goto error;
	}
	if (!strcmp(kdfname, "none") && strcmp(ciphername, "none") != 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: cipher %s requires kdf", __FUNCTION__, ciphername);
		goto error;
	}

	/* kdf options */
	kdfp = buffer_get_string_msg(copy_consumed, &klen);
	if (kdfp == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: kdf options not set", __FUNCTION__);
		goto error;
	}
	if (klen > 0) {
		if ((cp = buffer_append_space(kdf, klen)) == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: kdf alloc failed", __FUNCTION__);
			goto error;
		}
		memcpy(cp, kdfp, klen);
	}

	/* number of keys */
	if (buffer_get_int_ret(&nkeys, copy_consumed) < 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: key counter missing", __FUNCTION__);
		goto error;
	}
	if (nkeys != 1) {
		logprintf(LOG_LEVEL_ERROR, "%s: only one key supported", __FUNCTION__);
		goto error;
	}

	/* pubkey */
	cp = buffer_get_string_msg(copy_consumed, &len);
	if (cp == NULL) {
		logprintf(LOG_LEVEL_ERROR, "%s: pubkey not found", __FUNCTION__);
		goto error;
	}
	free(cp); /* XXX check pubkey against decrypted private key */

	/* size of encrypted key blob */
	len = buffer_get_int(copy_consumed);
	blocksize = get_cipher_block_size(cipher);
	authlen = 0;  // TODO: とりあえず固定化
	if (len < blocksize) {
		logprintf(LOG_LEVEL_ERROR, "%s: encrypted data too small", __FUNCTION__);
		goto error;
	}
	if (len % blocksize) {
		logprintf(LOG_LEVEL_ERROR, "%s: length not multiple of blocksize", __FUNCTION__);
		goto error;
	}

	/* setup key */
	keylen = get_cipher_key_len(cipher);
	ivlen = blocksize;
	key = calloc(1, keylen + ivlen);
	if (!strcmp(kdfname, KDFNAME)) {
		salt = buffer_get_string_msg(kdf, &slen);
		if (salt == NULL) {
			logprintf(LOG_LEVEL_ERROR, "%s: salt not set", __FUNCTION__);
			goto error;
		}
		rounds = buffer_get_int(kdf);
		// TODO: error check
		if (bcrypt_pbkdf(passphrase, strlen(passphrase), salt, slen,
		    key, keylen + ivlen, rounds) < 0) {
			logprintf(LOG_LEVEL_ERROR, "%s: bcrypt_pbkdf failed", __FUNCTION__);
			goto error;
		}
	}

	// 復号化
	cp = buffer_append_space(b, len);
	cipher_init_SSH2(&cc, cipher, key, keylen, key + keylen, ivlen, CIPHER_DECRYPT, pvar);
	ret = EVP_Cipher(cc->evp, cp, buffer_tail_ptr(copy_consumed), len);
	if (ret == 0) {
		goto error;
	}
	buffer_consume(copy_consumed, len);

	if (buffer_remain_len(copy_consumed) != 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: key blob has trailing data (len = %u)", __FUNCTION__,
			buffer_remain_len(copy_consumed));
		goto error;
	}

	/* check bytes */
	if (buffer_get_int_ret(&check1, b) < 0 ||
	    buffer_get_int_ret(&check2, b) < 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: check bytes missing", __FUNCTION__);
		goto error;
	}
	if (check1 != check2) {
		logprintf(LOG_LEVEL_VERBOSE, "%s: decrypt failed: 0x%08x != 0x%08x", __FUNCTION__,
			check1, check2);
		goto error;
	}

	keyfmt = key_private_deserialize(b);
	if (keyfmt == NULL)
		goto error;

	/* comment */
	comment = buffer_get_string_msg(b, NULL);

	i = 0;
	while (buffer_remain_len(b)) {
		if (buffer_get_char_ret(&pad, b) == -1 ||
		    pad != (++i & 0xff)) {
			logprintf(LOG_LEVEL_ERROR, "%s: bad padding", __FUNCTION__);
			key_free(keyfmt);
			keyfmt = NULL;
			goto error;
		}
	}

	/* success */
	keyfmt->bcrypt_kdf = 1;

error:
	buffer_free(blob);
	buffer_free(b);
	buffer_free(kdf);
	buffer_free(encoded);
	buffer_free(copy_consumed);
	cipher_free_SSH2(cc);

	free(ciphername);
	free(kdfname);
	free(kdfp);
	free(key);
	free(salt);
	free(comment);

	// KDF ではなかった
	if (keyfmt == NULL) {
		fseek(fp, 0, SEEK_SET);

	} else {
		fclose(fp);

	}

	return (keyfmt);
}


// OpenSSL format
Key *read_SSH2_private_key(PTInstVar pvar,
                           FILE * fp,
                           char * passphrase,
                           BOOL * invalid_passphrase,
                           BOOL is_auto_login,
                           char *errmsg,
                           int errmsg_len)
{
	Key *result = NULL;
	EVP_PKEY *pk = NULL;
	unsigned long err = 0;
	int pk_type;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
	//seed_prng();

	result = read_SSH2_private2_key(pvar, fp, passphrase, invalid_passphrase, is_auto_login, errmsg, errmsg_len);
	if (result)
		return (result);

	result = (Key *)malloc(sizeof(Key));
	ZeroMemory(result, sizeof(Key));

	// ファイルからパスフレーズを元に秘密鍵を読み込む。
	pk = PEM_read_PrivateKey(fp, NULL, NULL, passphrase);
	if (pk == NULL) {
		err = ERR_get_error();
		ERR_error_string_n(err, errmsg, errmsg_len);
		*invalid_passphrase = TRUE;
		goto error;
	}

	pk_type = EVP_PKEY_id(pk);
	switch (pk_type) {
	case EVP_PKEY_RSA: // RSA key
		result->type = KEY_RSA;
		result->rsa = EVP_PKEY_get1_RSA(pk);
		result->dsa = NULL;
		result->ecdsa = NULL;

		// RSA目くらましを有効にする（タイミング攻撃からの防御）
		if (RSA_blinding_on(result->rsa, NULL) != 1) {
			err = ERR_get_error();
			ERR_error_string_n(err, errmsg, errmsg_len);
			goto error;
		}
		break;

	case EVP_PKEY_DSA: // DSA key
		result->type = KEY_DSA;
		result->rsa = NULL;
		result->dsa = EVP_PKEY_get1_DSA(pk);
		result->ecdsa = NULL;
		break;

	case EVP_PKEY_EC: // ECDSA key
		result->rsa = NULL;
		result->dsa = NULL;
		result->ecdsa = EVP_PKEY_get1_EC_KEY(pk);
		{
			const EC_GROUP *g = EC_KEY_get0_group(result->ecdsa);
			result->type = nid_to_keytype(EC_GROUP_get_curve_name(g));
			if (result->type == KEY_UNSPEC) {
				goto error;
			}
		}
		break;

	default:
		goto error;
		break;
	}

	if (pk != NULL)
		EVP_PKEY_free(pk);

	fclose(fp);
	return (result);

error:
	if (pk != NULL)
		EVP_PKEY_free(pk);

	if (result != NULL)
		key_free(result);

	if (fp != NULL)
		fclose(fp);

	return (NULL);
}

// PuTTY format
/*
 * PuTTY-User-Key-File-2: ssh-dss
 * Encryption: aes256-cbc
 * Comment: imported-openssh-key
 * Public-Lines: 10
 * Base64...
 * Private-Lines: 1
 * Base64...
 * Private-MAC: Base16...
 *
 * for "ssh-rsa", it will be composed of
 *
 * "Public-Lines: " plus a number N.
 *
 *    string "ssh-rsa"
 *    mpint  exponent
 *    mpint  modulus
 *
 * "Private-Lines: " plus a number N,
 *
 *    mpint  private_exponent
 *    mpint  p                  (the larger of the two primes)
 *    mpint  q                  (the smaller prime)
 *    mpint  iqmp               (the inverse of q modulo p)
 *    data   padding            (to reach a multiple of the cipher block size)
 *
 * for "ssh-dss", it will be composed of
 *
 * "Public-Lines: " plus a number N.
 *
 *    string "ssh-rsa"
 *    mpint p
 *    mpint q
 *    mpint g
 *    mpint y
 *
 * "Private-Lines: " plus a number N,
 *
 *    mpint  x                  (the private key parameter)
 *
 * for "ecdsa-sha2-nistp256" or
 *     "ecdsa-sha2-nistp384" or
 *     "ecdsa-sha2-nistp521", it will be composed of
 *
 * "Public-Lines: " plus a number N.
 *
 *    string  "ecdsa-sha2-[identifier]" ("ecdsa-sha2-nistp256" or
 *                                       "ecdsa-sha2-nistp384" or
 *                                       "ecdsa-sha2-nistp521")
 *    string  [identifier] ("nistp256" or "nistp384" or "nistp521")
 *    string  Q            (EC_POINT)
 *
 * "Private-Lines: " plus a number N,
 *
 *    mpint  n
 *
 * for "ssh-ed25519", it will be composed of
 *
 * "Public-Lines: " plus a number N.
 *
 *    string "ssh-ed25519"
 *    string key
 *
 * "Private-Lines: " plus a number N,
 *
 *    string key
 *
 * "Private-MAC: " plus a hex, HMAC-SHA-1 of:
 *
 *    string name of algorithm ("ssh-dss", "ssh-rsa")
 *    string encryption type
 *    string comment
 *    string public-blob
 *    string private-plaintext
 */
Key *read_SSH2_PuTTY_private_key(PTInstVar pvar,
                                 FILE * fp,
                                 char * passphrase,
                                 BOOL * invalid_passphrase,
                                 BOOL is_auto_login,
                                 char *errmsg,
                                 int errmsg_len)
{
	char header[40], *b = NULL, *encryption = NULL, *comment = NULL, *mac = NULL;
	Key *result = NULL;
	buffer_t *public_blob = NULL, *private_blob = NULL, *cipher_mac_keys_blob = NULL;
	unsigned char *cipherkey = NULL, *cipheriv = NULL, *mackey = NULL;
	unsigned int cipherkey_len, cipheriv_len, mackey_len;
	buffer_t *passphrase_salt = buffer_init();
	const struct ssh2cipher *ciphertype;
	int lines, len;
	ppk_argon2_parameters params;
	unsigned fmt_version = 0;

	result = (Key *)malloc(sizeof(Key));
	ZeroMemory(result, sizeof(Key));
	result->type = KEY_NONE;
	result->rsa = NULL;
	result->dsa = NULL;
	result->ecdsa = NULL;

	// version and algorithm-name
	if (!ppk_read_header(fp, header)) {
		strncpy_s(errmsg, errmsg_len, "no header line found in key file", _TRUNCATE);
		goto error;
	}
	if (0 == strcmp(header, "PuTTY-User-Key-File-3")) {
		fmt_version = 3;
	}
	else if (0 == strcmp(header, "PuTTY-User-Key-File-2")) {
		fmt_version = 2;
	}
	else if (0 == strcmp(header, "PuTTY-User-Key-File-1")) {
		strncpy_s(errmsg, errmsg_len, "PuTTY key format too old", _TRUNCATE);
		goto error;
	}
	else if (0 == strncmp(header, "PuTTY-User-Key-File-", 20)) {
		strncpy_s(errmsg, errmsg_len, "PuTTY key format too new", _TRUNCATE);
		goto error;
	}
	if ((b = ppk_read_body(fp)) == NULL) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if (0 == strcmp(b, "ssh-dss")) {
		result->type = KEY_DSA;
	}
	else if (0 == strcmp(b, "ssh-rsa")) {
		result->type = KEY_RSA;
	}
	else if (0 == strcmp(b, "ecdsa-sha2-nistp256")) {
		result->type = KEY_ECDSA256;
	}
	else if (0 == strcmp(b, "ecdsa-sha2-nistp384")) {
		result->type = KEY_ECDSA384;
	}
	else if (0 == strcmp(b, "ecdsa-sha2-nistp521")) {
		result->type = KEY_ECDSA521;
	}
	else if (0 == strcmp(b, "ssh-ed25519")) {
		result->type = KEY_ED25519;
	}
	else {
		strncpy_s(errmsg, errmsg_len, "unsupported key algorithm", _TRUNCATE);
		free(b);
		goto error;
	}
	free(b);

	// encryption-type
	if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Encryption")) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if ((encryption = ppk_read_body(fp)) == NULL) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if (strcmp(encryption, "aes256-cbc") == 0) {
		ciphertype = get_cipher_by_name(encryption);
	}
	else if (strcmp(encryption, "none") == 0) {
		ciphertype = get_cipher_by_name(encryption);
	}
	else {
		strncpy_s(errmsg, errmsg_len, "unknown encryption type", _TRUNCATE);
		goto error;
	}

	// key-comment-string
	if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Comment")) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if ((comment = ppk_read_body(fp)) == NULL) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}

	// public key
	if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Public-Lines")) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if ((b = ppk_read_body(fp)) == NULL) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		free(b);
		goto error;
	}
	lines = atoi(b);
	free(b);
	public_blob = buffer_init();
	if (!ppk_read_blob(fp, lines, public_blob)) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}

	if (fmt_version >= 3 && ciphertype->key_len != 0) {
		size_t i;

		// argon2-flavour
		if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Key-Derivation")) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			goto error;
		}
		if ((b = ppk_read_body(fp)) == NULL) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			free(b);
			goto error;
		}
		if (!strcmp(b, "Argon2d")) {
			params.type = Argon2_d;
		}
		else if (!strcmp(b, "Argon2i")) {
			params.type = Argon2_i;
		}
		else if (!strcmp(b, "Argon2id")) {
			params.type = Argon2_id;
		}
		else {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			free(b);
			goto error;
		}
		free(b);

		if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Argon2-Memory")) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			goto error;
		}
		if ((b = ppk_read_body(fp)) == NULL) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			free(b);
			goto error;
		}
		if (!str_to_uint32_t(b, &params.argon2_mem)) {
			free(b);
			goto error;
		}
		free(b);

		if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Argon2-Passes")) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			goto error;
		}
		if ((b = ppk_read_body(fp)) == NULL) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			free(b);
			goto error;
		}
		if (!str_to_uint32_t(b, &params.argon2_passes)) {
			free(b);
			goto error;
		}
		free(b);

		if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Argon2-Parallelism")) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			goto error;
		}
		if ((b = ppk_read_body(fp)) == NULL) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			free(b);
			goto error;
		}
		if (!str_to_uint32_t(b, &params.argon2_parallelism)) {
			free(b);
			goto error;
		}
		free(b);

		if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Argon2-Salt")) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			goto error;
		}
		if ((b = ppk_read_body(fp)) == NULL) {
			strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
			free(b);
			goto error;
		}
		for (i = 0; b[i]; i += 2) {
			if (isxdigit((unsigned char)b[i]) && b[i+1] &&
			    isxdigit((unsigned char)b[i+1])) {
				char s[3];
				s[0] = b[i];
				s[1] = b[i+1];
				s[2] = '\0';
				buffer_put_char(passphrase_salt, strtoul(s, NULL, 16));
			}
			else {
				strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
				free(b);
				goto error;
			}
		}
		params.salt = buffer_ptr(passphrase_salt);
		params.saltlen = buffer_len(passphrase_salt);
		free(b);
	}

	// private key
	if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Private-Lines")) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if ((b = ppk_read_body(fp)) == NULL) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		free(b);
		goto error;
	}
	lines = atoi(b);
	free(b);
	private_blob = buffer_init();
	if (!ppk_read_blob(fp, lines, private_blob)) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}

	// hex-mac-data
	if (!ppk_read_header(fp, header) || 0 != strcmp(header, "Private-MAC")) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}
	if ((mac = ppk_read_body(fp)) == NULL) {
		strncpy_s(errmsg, errmsg_len, "file format error", _TRUNCATE);
		goto error;
	}

	fclose(fp);

	if (result->type == KEY_NONE || strlen(encryption) == 0 || buffer_len(public_blob) == 0 || buffer_len(private_blob) == 0) {
		strncpy_s(errmsg, errmsg_len, "key file format error", _TRUNCATE);
		goto error;
	}

	// derive key, iv, mackey
	cipher_mac_keys_blob = buffer_init();
	ssh2_ppk_derive_keys(fmt_version, ciphertype,
	                     passphrase,
	                     cipher_mac_keys_blob,
	                     &cipherkey, &cipherkey_len,
	                     &cipheriv, &cipheriv_len,
	                     &mackey, &mackey_len,
	                     &params);

	// decrypt priate key with aes256-cbc
	if (strcmp(encryption, "aes256-cbc") == 0) {
		struct sshcipher_ctx *cc = NULL;
		char *decrypted = NULL;
		int ret;

		// decrypt
		ciphertype = get_cipher_by_name("aes256-cbc");
		cipher_init_SSH2(&cc, ciphertype, cipherkey, 32, cipheriv, 16, CIPHER_DECRYPT, pvar);
		len = buffer_len(private_blob);
		decrypted = (char *)malloc(len);
		ret = EVP_Cipher(cc->evp, decrypted, buffer_ptr(private_blob), len);
		if (ret == 0) {
			strncpy_s(errmsg, errmsg_len, "Key decrypt error", _TRUNCATE);
			free(decrypted);
			cipher_free_SSH2(cc);
			goto error;
		}
		buffer_clear(private_blob);
		buffer_append(private_blob, decrypted, len);
		free(decrypted);
		cipher_free_SSH2(cc);
	}

	// verity MAC
	{
		unsigned char binary[32];
		char realmac[sizeof(binary) * 2 + 1];
		const EVP_MD *md;
		buffer_t *macdata;
		int i;

		macdata = buffer_init();
		buffer_put_cstring(macdata, get_ssh2_hostkey_type_name(result->type));
		buffer_put_cstring(macdata, encryption);
		buffer_put_cstring(macdata, comment);
		buffer_put_string(macdata, buffer_ptr(public_blob), buffer_len(public_blob));
		buffer_put_string(macdata, buffer_ptr(private_blob), buffer_len(private_blob));

		if (fmt_version == 2) {
			md = EVP_sha1();
		}
		else {
			md = EVP_sha256();
		}
		mac_simple(md, (unsigned char *)mackey, mackey_len, buffer_ptr(macdata), buffer_len(macdata), binary);

		buffer_free(macdata);

		for (i=0; i<EVP_MD_size(md); i++) {
			sprintf(realmac + 2*i, "%02x", binary[i]);
		}

		if (strcmp(mac, realmac) != 0) {
			if (ciphertype->key_len > 0) {
				strncpy_s(errmsg, errmsg_len, "wrong passphrase", _TRUNCATE);
				*invalid_passphrase = TRUE;
				goto error;
			}
			else {
				strncpy_s(errmsg, errmsg_len, "MAC verify failed", _TRUNCATE);
				goto error;
			}
		}
	}

	switch (result->type) {
	case KEY_RSA:
	{
		char *pubkey_type, *pub, *pri;
		BIGNUM *e, *n, *d, *iqmp, *p, *q;

		pub = buffer_ptr(public_blob);
		pri = buffer_ptr(private_blob);
		pubkey_type = buffer_get_string(&pub, NULL);
		if (strcmp(pubkey_type, "ssh-rsa") != 0) {
			strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
			free(pubkey_type);
			goto error;
		}
		free(pubkey_type);

		result->rsa = RSA_new();
		if (result->rsa == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}
		e = BN_new();
		n = BN_new();
		d = BN_new();
		RSA_set0_key(result->rsa, n, e, d);
		p = BN_new();
		q = BN_new();
		RSA_set0_factors(result->rsa, p, q);
		iqmp = BN_new();
		RSA_set0_crt_params(result->rsa, NULL, NULL, iqmp);
		if (e == NULL ||
		    n == NULL ||
		    d == NULL ||
		    p == NULL ||
		    q == NULL ||
		    iqmp == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		buffer_get_bignum2(&pub, e);
		buffer_get_bignum2(&pub, n);

		buffer_get_bignum2(&pri, d);
		buffer_get_bignum2(&pri, p);
		buffer_get_bignum2(&pri, q);
		buffer_get_bignum2(&pri, iqmp);

		break;
	}
	case KEY_DSA:
	{
		char *pubkey_type, *pub, *pri;
		BIGNUM *p, *q, *g, *pub_key, *priv_key;

		pub = buffer_ptr(public_blob);
		pri = buffer_ptr(private_blob);
		pubkey_type = buffer_get_string(&pub, NULL);
		if (strcmp(pubkey_type, "ssh-dss") != 0) {
			strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
			free(pubkey_type);
			goto error;
		}
		free(pubkey_type);

		result->dsa = DSA_new();
		if (result->dsa == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}
		p = BN_new();
		q = BN_new();
		g = BN_new();
		DSA_set0_pqg(result->dsa, p, q, g);
		pub_key = BN_new();
		priv_key = BN_new();
		DSA_set0_key(result->dsa, pub_key, priv_key);
		if (p == NULL ||
		    q == NULL ||
		    g == NULL ||
		    pub_key == NULL ||
		    priv_key == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		buffer_get_bignum2(&pub, p);
		buffer_get_bignum2(&pub, q);
		buffer_get_bignum2(&pub, g);
		buffer_get_bignum2(&pub, pub_key);

		buffer_get_bignum2(&pri, priv_key);

		break;
	}
	case KEY_ECDSA256:
	case KEY_ECDSA384:
	case KEY_ECDSA521:
	{
		char *pubkey_type, *pub, *pri;
		int success = 0;
		unsigned int nid;
		char *curve = NULL;
		ssh_keytype skt;
		BIGNUM *exponent = NULL;
		EC_POINT *q = NULL;

		pub = buffer_ptr(public_blob);
		pri = buffer_ptr(private_blob);
		pubkey_type = buffer_get_string(&pub, NULL);
		if ((result->type == KEY_ECDSA256 && strcmp(pubkey_type, "ecdsa-sha2-nistp256") != 0) ||
		    (result->type == KEY_ECDSA384 && strcmp(pubkey_type, "ecdsa-sha2-nistp384") != 0) ||
		    (result->type == KEY_ECDSA521 && strcmp(pubkey_type, "ecdsa-sha2-nistp521") != 0)) {
			strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
			free(pubkey_type);
			goto error;
		}
		free(pubkey_type);

		nid = keytype_to_cipher_nid(result->type);
		curve = buffer_get_string(&pub, NULL);
		skt = key_curve_name_to_keytype(curve);
		if (nid != keytype_to_cipher_nid(skt))
			goto ecdsa_error;

		if ((result->ecdsa = EC_KEY_new_by_curve_name(nid)) == NULL)
			goto ecdsa_error;
		if ((q = EC_POINT_new(EC_KEY_get0_group(result->ecdsa))) == NULL)
			goto ecdsa_error;
		if ((exponent = BN_new()) == NULL)
			goto ecdsa_error;

		buffer_get_ecpoint(&pub, EC_KEY_get0_group(result->ecdsa), q);
		buffer_get_bignum2(&pri, exponent);
		if (EC_KEY_set_public_key(result->ecdsa, q) != 1)
			goto ecdsa_error;
		if (EC_KEY_set_private_key(result->ecdsa, exponent) != 1)
			goto ecdsa_error;
		if (key_ec_validate_public(EC_KEY_get0_group(result->ecdsa),
		                           EC_KEY_get0_public_key(result->ecdsa)) != 0)
			goto ecdsa_error;
		if (key_ec_validate_private(result->ecdsa) != 0)
			goto ecdsa_error;

		success = 1;

ecdsa_error:
		free(curve);
		if (exponent)
			BN_clear_free(exponent);
		if (q)
			EC_POINT_free(q);
		if (success == 0)
			goto error;

		break;
	}
	case KEY_ED25519:
	{
		char *pubkey_type, *pub, *pri;
		unsigned int pklen, sklen;
		char *sk;
		pub = buffer_ptr(public_blob);
		pri = buffer_ptr(private_blob);
		pubkey_type = buffer_get_string(&pub, NULL);
		if (strcmp(pubkey_type, "ssh-ed25519") != 0) {
			strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
			free(pubkey_type);
			goto error;
		}
		free(pubkey_type);

		result->ed25519_pk = buffer_get_string(&pub, &pklen);
		sk = buffer_get_string(&pri, &sklen);
		result->ed25519_sk = malloc(pklen + sklen + 1);
		memcpy(result->ed25519_sk, sk, sklen);
		memcpy(result->ed25519_sk + sklen, result->ed25519_pk, pklen);
		result->ed25519_sk[sklen + pklen] = '\0';
		free(sk);

		if (pklen != ED25519_PK_SZ)
			goto error;
		if (sklen + pklen != ED25519_SK_SZ)
			goto error;

		break;
	}
	default:
		strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
		goto error;
		break;
	}

	if (encryption != NULL)
		free(encryption);
	if (comment != NULL)
		free(comment);
	if (mac != NULL)
		free(mac);
	if (public_blob != NULL)
		buffer_free(public_blob);
	if (private_blob != NULL)
		buffer_free(private_blob);
	if (cipher_mac_keys_blob != NULL)
		buffer_free(cipher_mac_keys_blob);
	if (passphrase_salt != NULL)
		buffer_free(passphrase_salt);

	return (result);

error:
	if (result != NULL)
		key_free(result);
	if (fp != NULL)
		fclose(fp);
	if (encryption != NULL)
		free(encryption);
	if (comment != NULL)
		free(comment);
	if (mac != NULL)
		free(mac);
	if (public_blob != NULL)
		buffer_free(public_blob);
	if (private_blob != NULL)
		buffer_free(private_blob);
	if (cipher_mac_keys_blob != NULL)
		buffer_free(cipher_mac_keys_blob);
	if (passphrase_salt != NULL)
		buffer_free(passphrase_salt);

	return (NULL);
}

// SECSH(ssh.com) format
/*
 * Code to read ssh.com private keys.
 *
 *  "---- BEGIN SSH2 ENCRYPTED PRIVATE KEY ----"
 *  "Comment:..."
 *  "Base64..."
 *  "---- END SSH2 ENCRYPTED PRIVATE KEY ----"
 *
 * Body of key data
 *
 *  - uint32 0x3f6ff9eb       (magic number)
 *  - uint32 size             (total blob size)
 *  - string key-type         (see below)
 *  - string cipher-type      (tells you if key is encrypted)
 *  - string encrypted-blob
 *
 * The key type strings are ghastly. The RSA key I looked at had a type string of
 *
 *   `if-modn{sign{rsa-pkcs1-sha1},encrypt{rsa-pkcs1v2-oaep}}'
 *   `dl-modp{sign{dsa-nist-sha1},dh{plain}}'
 *   `ec-modp'
 *
 * The encryption. The cipher-type string appears to be either
 *
 *    `none'
 *    `3des-cbc'
 *
 * The payload blob, for an RSA key, contains:
 *  - mpint e
 *  - mpint d
 *  - mpint n  (yes, the public and private stuff is intermixed)
 *  - mpint u  (presumably inverse of p mod q)
 *  - mpint p  (p is the smaller prime)
 *  - mpint q  (q is the larger)
 *
 * For a DSA key, the payload blob contains:
 *  - uint32 0
 *  - mpint p
 *  - mpint g
 *  - mpint q
 *  - mpint y
 *  - mpint x
 *
 * For a ECDSA key, the payload blob contains:
 *  - uint32 1
 *  - string [identifier] ("nistp256" or "nistp384" or "nistp521")
 *  - mpint  n
 */
Key *read_SSH2_SECSH_private_key(PTInstVar pvar,
                                 FILE * fp,
                                 char * passphrase,
                                 BOOL * invalid_passphrase,
                                 BOOL is_auto_login,
                                 char *errmsg,
                                 int errmsg_len)
{
	Key *result = NULL;
	unsigned long err = 0;
	int i, len2;
	unsigned int len;
	int encflag;
	char *encname = NULL;
	buffer_t *blob = NULL, *blob2 = NULL;
	const struct ssh2cipher *cipher = NULL;
	struct sshcipher_ctx *cc = NULL;

	result = (Key *)malloc(sizeof(Key));
	ZeroMemory(result, sizeof(Key));
	result->type = KEY_NONE;
	result->rsa = NULL;
	result->dsa = NULL;
	result->ecdsa = NULL;

	blob = buffer_init();
	blob2 = buffer_init();

	// parse keyfile & decode blob
	{
	char line[200], buf[100];
	BIO *bmem, *b64, *chain;
	int st = 0;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp(line, "---- BEGIN SSH2 ENCRYPTED PRIVATE KEY ----", strlen("---- BEGIN SSH2 ENCRYPTED PRIVATE KEY ----")) == 0) {
			if (st == 0) {
				st = 1;
				continue;
			}
			else {
				break;
			}
		}
		else if (strncmp(line, "---- END SSH2 ENCRYPTED PRIVATE KEY ----", strlen("---- END SSH2 ENCRYPTED PRIVATE KEY ----")) == 0) {
			break;
		}

		if (st == 0) {
			continue;
		}

		if (strchr(line, ':') != NULL) {
			if (st == 1) {
				continue;
			}
			strncpy_s(errmsg, errmsg_len, "header found in body of key data", _TRUNCATE);
			BIO_free_all(bmem);
			goto error;
		}
		else if (st == 1) {
			st = 2;
		}

		BIO_write(bmem, line, strlen(line));
	}

	BIO_flush(bmem);
	chain = BIO_push(b64, bmem);
	BIO_set_mem_eof_return(chain, 0);
	while ((len2 = BIO_read(chain, buf, sizeof(buf))) > 0) {
		buffer_append(blob, buf, len2);
	}
	BIO_free_all(chain);
	buffer_rewind(blob);
	}

	if (buffer_len(blob) < 8) {
		strncpy_s(errmsg, errmsg_len, "key body not present", _TRUNCATE);
		goto error;
	}
	i = buffer_get_int(blob);
	if (i != 0x3f6ff9eb) {
		strncpy_s(errmsg, errmsg_len, "magic number error", _TRUNCATE);
		goto error;
	}
	len = buffer_get_int(blob);
	if (len == 0 || len > buffer_len(blob)) {
		strncpy_s(errmsg, errmsg_len, "body size error", _TRUNCATE);
		goto error;
	}

	len = buffer_get_int(blob);
	if (strncmp(blob->buf + blob->offset, "if-modn{sign{rsa", strlen("if-modn{sign{rsa") - 1) == 0) {
		result->type = KEY_RSA;
	}
	else if (strncmp(blob->buf + blob->offset, "dl-modp{sign{dsa", strlen("dl-modp{sign{dsa") - 1) == 0) {
		result->type = KEY_DSA;
	}
	else if (strncmp(blob->buf + blob->offset, "ec-modp", strlen("ec-modp") - 1) == 0) {
		result->type = KEY_ECDSA256;
	}
	else {
		strncpy_s(errmsg, errmsg_len, "unknown key type", _TRUNCATE);
		goto error;
	}
	buffer_consume(blob, len);

	len = buffer_get_int(blob);
	encname = (char *)malloc(len + 1);
	strncpy_s(encname, len + 1, blob->buf + blob->offset, _TRUNCATE);
	if (strcmp(encname, "3des-cbc") == 0) {
		encflag = 1;
	}
	else if (strcmp(encname, "none") == 0) {
		encflag = 0;
	}
	else {
		strncpy_s(errmsg, errmsg_len, "unknown encryption type", _TRUNCATE);
		goto error;
	}
	buffer_consume(blob, len);

	len = buffer_get_int(blob);
	if (len > (blob->len - blob->offset)) {
		strncpy_s(errmsg, errmsg_len, "body size error", _TRUNCATE);
		goto error;
	}

	// decrypt privkey with 3des-cbc
	if (strcmp(encname, "3des-cbc") == 0) {
		MD5_CTX md;
		unsigned char key[32], iv[16];
		EVP_CIPHER_CTX *cipher_ctx = NULL;
		char *decrypted = NULL;
		int ret;

		cipher_ctx = EVP_CIPHER_CTX_new();
		if (cipher_ctx == NULL) {
			strncpy_s(errmsg, errmsg_len, "Out of memory: EVP_CIPHER_CTX_new()", _TRUNCATE);
			goto error;
		}

		MD5_Init(&md);
		MD5_Update(&md, passphrase, strlen(passphrase));
		MD5_Final(key, &md);

		MD5_Init(&md);
		MD5_Update(&md, passphrase, strlen(passphrase));
		MD5_Update(&md, key, 16);
		MD5_Final(key + 16, &md);

		memset(iv, 0, sizeof(iv));

		// decrypt
		cipher = get_cipher_by_name("3des-cbc");
		cipher_init_SSH2(&cc, cipher, key, 24, iv, 8, CIPHER_DECRYPT, pvar);
		decrypted = (char *)malloc(len);
		ret = EVP_Cipher(cc->evp, decrypted, blob->buf + blob->offset, len);
		if (ret == 0) {
			strncpy_s(errmsg, errmsg_len, "Key decrypt error", _TRUNCATE);
			cipher_free_SSH2(cc);
			goto error;
		}
		buffer_append(blob2, decrypted, len);
		free(decrypted);
		cipher_free_SSH2(cc);

		*invalid_passphrase = TRUE;
	}
	else { // none
		buffer_append(blob2, blob->buf + blob->offset, len);
	}
	buffer_rewind(blob2);

	len = buffer_get_int(blob2);
	if (len <= 0 || len > (blob2->len - blob2->offset)) {
		strncpy_s(errmsg, errmsg_len, "blob size error", _TRUNCATE);
		goto error;
	}

	switch (result->type) {
	case KEY_RSA:
	{
		BIGNUM *e, *n, *d, *iqmp, *p, *q;

		result->rsa = RSA_new();
		if (result->rsa == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}
		e = BN_new();
		n = BN_new();
		d = BN_new();
		RSA_set0_key(result->rsa, n, e, d);
		p = BN_new();
		q = BN_new();
		RSA_set0_factors(result->rsa, p, q);
		iqmp = BN_new();
		RSA_set0_crt_params(result->rsa, NULL, NULL, iqmp);
		if (e == NULL ||
		    n == NULL ||
		    d == NULL ||
		    p == NULL ||
		    q == NULL ||
		    iqmp == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		buffer_get_bignum_SECSH(blob2, e);
		buffer_get_bignum_SECSH(blob2, d);
		buffer_get_bignum_SECSH(blob2, n);
		buffer_get_bignum_SECSH(blob2, iqmp);
		buffer_get_bignum_SECSH(blob2, p);
		buffer_get_bignum_SECSH(blob2, q);

		break;
	}
	case KEY_DSA:
	{
		int param;
		BIGNUM *p, *q, *g, *pub_key, *priv_key;

		result->dsa = DSA_new();
		if (result->dsa == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}
		p = BN_new();
		q = BN_new();
		g = BN_new();
		DSA_set0_pqg(result->dsa, p, q, g);
		pub_key = BN_new();
		priv_key = BN_new();
		DSA_set0_key(result->dsa, pub_key, priv_key);
		if (p == NULL ||
		    q == NULL ||
		    g == NULL ||
		    pub_key == NULL ||
		    priv_key == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		param = buffer_get_int(blob2);
		if (param != 0) {
			strncpy_s(errmsg, errmsg_len, "predefined DSA parameters not supported", _TRUNCATE);
			goto error;
		}
		buffer_get_bignum_SECSH(blob2, p);
		buffer_get_bignum_SECSH(blob2, g);
		buffer_get_bignum_SECSH(blob2, q);
		buffer_get_bignum_SECSH(blob2, pub_key);
		buffer_get_bignum_SECSH(blob2, priv_key);

		break;
	}
	case KEY_ECDSA256:
	{
		unsigned int dummy, nid;
		int success = 0;
		char *curve = NULL;
		BIGNUM *exponent = NULL;
		EC_POINT *q = NULL;
		BN_CTX *ctx = NULL;

		dummy = buffer_get_int(blob2);
		curve = buffer_get_string_msg(blob2, NULL);

		if (strncmp(curve, "nistp256", strlen("nistp256")) == 0) {
			result->type = KEY_ECDSA256;
		}
		else if (strncmp(curve, "nistp384", strlen("nistp384")) == 0) {
			result->type = KEY_ECDSA384;
		}
		else if (strncmp(curve, "nistp521", strlen("nistp521")) == 0) {
			result->type = KEY_ECDSA521;
		}
		else {
			strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
			goto error;
		}

		nid = keytype_to_cipher_nid(result->type);
		if ((result->ecdsa = EC_KEY_new_by_curve_name(nid)) == NULL)
			goto ecdsa_error;
		if ((q = EC_POINT_new(EC_KEY_get0_group(result->ecdsa))) == NULL)
			goto ecdsa_error;
		if ((exponent = BN_new()) == NULL)
			goto ecdsa_error;

		buffer_get_bignum_SECSH(blob2, exponent);
		if (EC_KEY_set_private_key(result->ecdsa, exponent) != 1)
			goto ecdsa_error;
		if (key_ec_validate_private(result->ecdsa) != 0)
			goto ecdsa_error;

		// ファイルには秘密鍵しか格納されていないので公開鍵を計算で求める
		if ((ctx = BN_CTX_new()) == NULL)
			goto ecdsa_error;
		if (!EC_POINT_mul(EC_KEY_get0_group(result->ecdsa), q, exponent, NULL, NULL, ctx)) {
			goto ecdsa_error;
		}
		if (EC_KEY_set_public_key(result->ecdsa, q) != 1)
			goto ecdsa_error;
		if (key_ec_validate_public(EC_KEY_get0_group(result->ecdsa),
		                           EC_KEY_get0_public_key(result->ecdsa)) != 0)
			goto ecdsa_error;

		success = 1;

ecdsa_error:
		free(curve);
		if (exponent)
			BN_clear_free(exponent);
		if (q)
			EC_POINT_free(q);
		if (ctx)
			BN_CTX_free(ctx);
		if (success == 0)
			goto error;

		break;
	}
	default:
		strncpy_s(errmsg, errmsg_len, "key type error", _TRUNCATE);
		goto error;
		break;
	}

	*invalid_passphrase = FALSE;

	fclose(fp);

	if (encname != NULL)
		free(encname);

	if (blob != NULL)
		buffer_free(blob);

	if (blob2 != NULL)
		buffer_free(blob2);

	return (result);

error:
	if (result != NULL)
		key_free(result);

	if (fp != NULL)
		fclose(fp);

	if (encname != NULL)
		free(encname);

	if (blob != NULL)
		buffer_free(blob);

	if (blob2 != NULL)
		buffer_free(blob2);

	return (NULL);
}

ssh2_keyfile_type get_ssh2_keytype(const wchar_t *relative_name,
                                   FILE **fp,
                                   char *errmsg,
                                   int errmsg_len) {
	ssh2_keyfile_type ret = SSH2_KEYFILE_TYPE_NONE;
	wchar_t *filename;
	char line[200];
	int i;

	filename = get_teraterm_dir_relative_nameW(relative_name);

	*fp = _wfopen(filename, L"r");
	free(filename);
	if (*fp == NULL) {
		strncpy_s(errmsg, errmsg_len, strerror(errno), _TRUNCATE);
		return ret;
	}

	while (fgets(line, sizeof(line), *fp) != NULL) {
		for (i=0; keyfile_headers[i].type != SSH2_KEYFILE_TYPE_NONE; i++) {
			if ( strncmp(line, keyfile_headers[i].header, strlen(keyfile_headers[i].header)) == 0) {
				ret = keyfile_headers[i].type;
				break;
			}
		}
		if (ret != SSH2_KEYFILE_TYPE_NONE)
			break;
	}

	if (ret == SSH2_KEYFILE_TYPE_NONE) {
		strncpy_s(errmsg, errmsg_len, "Unknown key file type.", _TRUNCATE);
		fseek(*fp, 0, SEEK_SET);
		return ret;
	}

	fseek(*fp, 0, SEEK_SET);
	return ret;
}
