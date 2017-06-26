/*
Copyright (c) 1998-2001, Robert O'Callahan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

The name of Robert O'Callahan may not be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"
#include "keyfiles.h"
#include "key.h"

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

	if (BN_cmp(key->p, key->q) < 0) {
		BIGNUM *tmp = key->p;

		key->p = key->q;
		key->q = tmp;
	}

	if (r != NULL && ctx != NULL) {
		key->dmp1 = BN_new();
		key->dmq1 = BN_new();
		key->iqmp = BN_mod_inverse(NULL, key->q, key->p, ctx);

		if (key->dmp1 != NULL && key->dmq1 != NULL && key->iqmp != NULL) {
			OK = BN_sub(r, key->p, BN_value_one())
			  && BN_mod(key->dmp1, key->d, r, ctx)
			  && BN_sub(r, key->q, BN_value_one())
			  && BN_mod(key->dmq1, key->d, r, ctx);
		}
	}

	BN_free(r);
	BN_CTX_free(ctx);

	return OK;
}

static RSA *read_RSA_private_key(PTInstVar pvar,
                                 char * relative_name,
                                 char * passphrase,
                                 BOOL * invalid_passphrase,
                                 BOOL is_auto_login)
{
	char filename[2048];
	int fd;
	unsigned int length, amount_read;
	unsigned char *keyfile_data;
	unsigned int index;
	int cipher;
	RSA *key;
	unsigned int E_index, N_index, D_index, U_index, P_index, Q_index = 0;

	*invalid_passphrase = FALSE;

	get_teraterm_dir_relative_name(filename, sizeof(filename),
	                               relative_name);

	fd = _open(filename, _O_RDONLY | _O_SEQUENTIAL | _O_BINARY);
	if (fd == -1) {
		if (errno == ENOENT) {
			UTIL_get_lang_msg("MSG_KEYFILES_READ_ENOENT_ERROR", pvar,
			                  "An error occurred while trying to read the key file.\n"
			                  "The specified filename does not exist.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		} else {
			UTIL_get_lang_msg("MSG_KEYFILES_READ_ERROR", pvar,
			                  "An error occurred while trying to read the key file.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			_close(fd);
			return NULL;
		}
	} else {
		UTIL_get_lang_msg("MSG_KEYFILES_READ_ERROR", pvar,
		                  "An error occurred while trying to read the key file.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		free(keyfile_data);
		return NULL;
	}

	if (strcmp(keyfile_data, ID_string) != 0) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_NOTCONTAIN_ERROR", pvar,
		                  "The specified key file does not contain an SSH private key.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		free(keyfile_data);
		return NULL;
	}

	index = NUM_ELEM(ID_string);

	if (length < index + 9) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	N_index = index;
	index += (get_ushort16_MSBfirst(keyfile_data + index) + 7) / 8 + 2;
	if (length < index + 2) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	E_index = index;
	index += (get_ushort16_MSBfirst(keyfile_data + index) + 7) / 8 + 2;
	/* skip comment */
	if (length < index + 4) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	index += get_uint32_MSBfirst(keyfile_data + index) + 4;

	if (length < index + 6) {
		UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_TRUNCATE_ERROR", pvar,
		                  "The specified key file has been truncated and does not contain a valid private key.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		free(keyfile_data);
		return NULL;
	}
	if (cipher != SSH_CIPHER_NONE) {
		if ((length - index) % 8 != 0) {
			UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_LENGTH_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the passphrase.\n"
			                  "The file does not have the correct length.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
			free(keyfile_data);
			return NULL;
		}
		if (!CRYPT_passphrase_decrypt
			(cipher, passphrase, keyfile_data + index, length - index)) {
			UTIL_get_lang_msg("MSG_KEYFILES_PRIVATEKEY_NOCIPHER_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the passphrase.\n"
			                  "The cipher type used to encrypt the file is not supported by TTSSH for this purpose.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		}
		else {
			UTIL_get_lang_msg("MSG_KEYFILES_PASSPHRASE_ERROR", pvar,
			                  "The specified key file cannot be decrypted using the passphrase.\n"
			                  "The passphrase is incorrect.");
			notify_nonfatal_error(pvar, pvar->ts->UIMsg);
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
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);
		SecureZeroMemory(keyfile_data, length);
		free(keyfile_data);
		return NULL;
	}

	key = RSA_new();
	key->n = get_bignum(keyfile_data + N_index);
	key->e = get_bignum(keyfile_data + E_index);
	key->d = get_bignum(keyfile_data + D_index);
	key->p = get_bignum(keyfile_data + P_index);
	key->q = get_bignum(keyfile_data + Q_index);

	if (!normalize_key(key)) {
		UTIL_get_lang_msg("MSG_KEYFILES_CRYPTOLIB_ERROR", pvar,
		                  "Error in cryptography library.\n"
		                  "Perhaps the stored key is invalid.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);

		RSA_free(key);
		key = NULL;
	}

	SecureZeroMemory(keyfile_data, length);
	free(keyfile_data);
	return key;
}

Key *KEYFILES_read_private_key(PTInstVar pvar,
                               char * relative_name,
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
	SSHCipher ciphernameval;
	size_t authlen;
	EVP_CIPHER_CTX cipher_ctx;

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
			logprintf(LOG_LEVEL_WARNING, __FUNCTION__ ": key file too large.");
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
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": missing begin marker");
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
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": no end marker");
		goto error;
	}

	// ファイルのスキャンが終わったので、base64 decodeする。
	len = buffer_len(encoded);
	if ((cp = buffer_append_space(copy_consumed, len)) == NULL) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": buffer_append_space");
		goto error;
	}
	if ((dlen = b64decode(cp, len, buffer_ptr(encoded))) < 0) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": base64 decode failed");
		goto error;
	}
	if ((unsigned int)dlen > len) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": crazy base64 length %d > %u", dlen, len);
		goto error;
	}

	buffer_consume_end(copy_consumed, len - dlen);
	if (buffer_remain_len(copy_consumed) < sizeof(AUTH_MAGIC) ||
	    memcmp(buffer_tail_ptr(copy_consumed), AUTH_MAGIC, sizeof(AUTH_MAGIC))) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": bad magic");
		goto error;
	}
	buffer_consume(copy_consumed, sizeof(AUTH_MAGIC));

	/*
	 * デコードしたデータを解析する。
	 */
	// 暗号化アルゴリズムの名前
	ciphername = buffer_get_string_msg(copy_consumed, NULL);
	ciphernameval = get_cipher_by_name(ciphername);
	if (ciphernameval == SSH_CIPHER_NONE && strcmp(ciphername, "none") != 0) {
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
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": unknown kdf name");
		goto error;
	}
	if (!strcmp(kdfname, "none") && strcmp(ciphername, "none") != 0) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ "%s: cipher %s requires kdf", ciphername);
		goto error;
	}

	/* kdf options */
	kdfp = buffer_get_string_msg(copy_consumed, &klen);
	if (kdfp == NULL) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": kdf options not set");
		goto error;
	}
	if (klen > 0) {
		if ((cp = buffer_append_space(kdf, klen)) == NULL) {
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": kdf alloc failed");
			goto error;
		}
		memcpy(cp, kdfp, klen);
	}

	/* number of keys */
	if (buffer_get_int_ret(&nkeys, copy_consumed) < 0) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": key counter missing");
		goto error;
	}
	if (nkeys != 1) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": only one key supported");
		goto error;
	}

	/* pubkey */
	cp = buffer_get_string_msg(copy_consumed, &len);
	if (cp == NULL) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": pubkey not found");
		goto error;
	}
	free(cp); /* XXX check pubkey against decrypted private key */

	/* size of encrypted key blob */
	len = buffer_get_int(copy_consumed);
	blocksize = get_cipher_block_size(ciphernameval);
	authlen = 0;  // TODO: とりあえず固定化
	if (len < blocksize) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": encrypted data too small");
		goto error;
	}
	if (len % blocksize) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": length not multiple of blocksize");
		goto error;
	}

	/* setup key */
	keylen = get_cipher_key_len(ciphernameval);
	ivlen = blocksize;
	key = calloc(1, keylen + ivlen);
	if (!strcmp(kdfname, KDFNAME)) {
		salt = buffer_get_string_msg(kdf, &slen);
		if (salt == NULL) {
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": salt not set");
			goto error;
		}
		rounds = buffer_get_int(kdf);
		// TODO: error check
		if (bcrypt_pbkdf(passphrase, strlen(passphrase), salt, slen,
		    key, keylen + ivlen, rounds) < 0) {
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": bcrypt_pbkdf failed");
			goto error;
		}
	}

	// 復号化
	cp = buffer_append_space(b, len);
	cipher_init_SSH2(&cipher_ctx, key, keylen, key + keylen, ivlen, CIPHER_DECRYPT, 
		get_cipher_EVP_CIPHER(ciphernameval), 0, pvar);
	if (EVP_Cipher(&cipher_ctx, cp, buffer_tail_ptr(copy_consumed), len) == 0) {
		cipher_cleanup_SSH2(&cipher_ctx);
		goto error;
	}
	cipher_cleanup_SSH2(&cipher_ctx);
	buffer_consume(copy_consumed, len);

	if (buffer_remain_len(copy_consumed) != 0) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": key blob has trailing data (len = %u)",
			buffer_remain_len(copy_consumed));
		goto error;
	}

	/* check bytes */
	if (buffer_get_int_ret(&check1, b) < 0 ||
	    buffer_get_int_ret(&check2, b) < 0) {
		logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": check bytes missing");
		goto error;
	}
	if (check1 != check2) {
		logprintf(LOG_LEVEL_VERBOSE, __FUNCTION__ ": decrypt failed: 0x%08x != 0x%08x",
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
			logprintf(LOG_LEVEL_ERROR, __FUNCTION__ ": bad padding");
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

	switch (pk->type) {
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
 * Private-Hash: Base16... (PuTTY-User-Key-File-1) ???
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
	Key *result = NULL;
	EVP_PKEY *pk = NULL;
	unsigned long err = 0;
	int i, len, len2;
	char *encname = NULL, *comment = NULL, *private_mac = NULL;
	buffer_t *pubkey = NULL, *prikey = NULL;

	result = (Key *)malloc(sizeof(Key));
	ZeroMemory(result, sizeof(Key)); 
	result->type = KEY_NONE;
	result->rsa = NULL;
	result->dsa = NULL;
	result->ecdsa = NULL;

	pubkey = buffer_init();
	prikey = buffer_init();

	// parse keyfile & decode blob
	{
	char line[200], buf[100];
	BIO *bmem, *b64, *chain;
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp(line, "PuTTY-User-Key-File-2: ", strlen("PuTTY-User-Key-File-2: ")) == 0) {
			if (strncmp(line + strlen("PuTTY-User-Key-File-2: "), "ssh-dss", strlen("ssh-dss")) == 0) {
				result->type = KEY_DSA;
			}
			else if (strncmp(line + strlen("PuTTY-User-Key-File-2: "), "ssh-rsa", strlen("ssh-rsa")) == 0) {
				result->type = KEY_RSA;
			}
			else if (strncmp(line + strlen("PuTTY-User-Key-File-2: "), "ecdsa-sha2-nistp256", strlen("ecdsa-sha2-nistp256")) == 0) {
				result->type = KEY_ECDSA256;
			}
			else if (strncmp(line + strlen("PuTTY-User-Key-File-2: "), "ecdsa-sha2-nistp384", strlen("ecdsa-sha2-nistp384")) == 0) {
				result->type = KEY_ECDSA384;
			}
			else if (strncmp(line + strlen("PuTTY-User-Key-File-2: "), "ecdsa-sha2-nistp521", strlen("ecdsa-sha2-nistp521")) == 0) {
				result->type = KEY_ECDSA521;
			}
			else if (strncmp(line + strlen("PuTTY-User-Key-File-2: "), "ssh-ed25519", strlen("ssh-ed25519")) == 0) {
				result->type = KEY_ED25519;
			}
			else {
				strncpy_s(errmsg, errmsg_len, "not a PuTTY SSH-2 private key", _TRUNCATE);
				goto error;
			}
		}
		else if (strncmp(line, "Encryption: ", strlen("Encryption: ")) == 0) {
			len = strlen(line + strlen("Encryption: "));
			encname = (char *)malloc(len); // trim \n
			strncpy_s(encname, len, line + strlen("Encryption: "), _TRUNCATE);
			if (strcmp(encname, "aes256-cbc") == 0) {
				// NOP
			}
			else if (strcmp(encname, "none") == 0) {
				// NOP
			}
			else {
				strncpy_s(errmsg, errmsg_len, "unknown encryption type", _TRUNCATE);
				goto error;
			}
		}
		else if (strncmp(line, "Comment: ", strlen("Comment: ")) == 0) {
			len = strlen(line + strlen("Comment: "));
			comment = (char *)malloc(len); // trim \n
			strncpy_s(comment, len, line + strlen("Comment: "), _TRUNCATE);
		}
		else if (strncmp(line, "Private-MAC: ", strlen("Private-MAC: ")) == 0) {
			len = strlen(line + strlen("Private-MAC: "));
			private_mac = (char *)malloc(len); // trim \n
			strncpy_s(private_mac, len, line + strlen("Private-MAC: "), _TRUNCATE);
		}
		else if (strncmp(line, "Private-HASH: ", strlen("Private-HASH: ")) == 0) {
			strncpy_s(errmsg, errmsg_len, "not a PuTTY SSH-2 private key", _TRUNCATE);
			goto error;
		}
		else if (strncmp(line, "Public-Lines: ", strlen("Public-Lines: ")) == 0) {
			len = atoi(line + strlen("Public-Lines: "));
			b64 = BIO_new(BIO_f_base64());
			bmem = BIO_new(BIO_s_mem());
			for (i=0; i<len && fgets(line, sizeof(line), fp)!=NULL; i++) {
				BIO_write(bmem, line, strlen(line));
			}
			BIO_flush(bmem);
			chain = BIO_push(b64, bmem);
			BIO_set_mem_eof_return(chain, 0);
			while ((len2 = BIO_read(chain, buf, sizeof(buf))) > 0) {
				buffer_append(pubkey, buf, len2);
			}
			BIO_free_all(chain);
		}
		else if (strncmp(line, "Private-Lines: ", strlen("Private-Lines: ")) == 0) {
			len = atoi(line + strlen("Private-Lines: "));
			b64 = BIO_new(BIO_f_base64());
			bmem = BIO_new(BIO_s_mem());
			for (i=0; i<len && fgets(line, sizeof(line), fp)!=NULL; i++) {
				BIO_write(bmem, line, strlen(line));
			}
			BIO_flush(bmem);
			chain = BIO_push(b64, bmem);
			BIO_set_mem_eof_return(chain, 0);
			while ((len2 = BIO_read(chain, buf, sizeof(buf))) > 0) {
				buffer_append(prikey, buf, len2);
			}
			BIO_free_all(chain);
		}
		else {
			strncpy_s(errmsg, errmsg_len, "not a PuTTY SSH-2 private key", _TRUNCATE);
			goto error;
		}
	}
	}

	if (result->type == KEY_NONE || strlen(encname) == 0 || buffer_len(pubkey) == 0 || buffer_len(prikey) == 0) {
		strncpy_s(errmsg, errmsg_len, "key file format error", _TRUNCATE);
		goto error;
	}

	// decrypt prikey with aes256-cbc
	if (strcmp(encname, "aes256-cbc") == 0) {
		const EVP_MD *md = EVP_sha1();
		EVP_MD_CTX ctx;
		unsigned char key[40], iv[32];
		EVP_CIPHER_CTX cipher_ctx;
		char *decrypted = NULL;

		EVP_DigestInit(&ctx, md);
		EVP_DigestUpdate(&ctx, "\0\0\0\0", 4);
		EVP_DigestUpdate(&ctx, passphrase, strlen(passphrase));
		EVP_DigestFinal(&ctx, key, &len);

		EVP_DigestInit(&ctx, md);
		EVP_DigestUpdate(&ctx, "\0\0\0\1", 4);
		EVP_DigestUpdate(&ctx, passphrase, strlen(passphrase));
		EVP_DigestFinal(&ctx, key + 20, &len);

		memset(iv, 0, sizeof(iv));

		// decrypt
		cipher_init_SSH2(&cipher_ctx, key, 32, iv, 16, CIPHER_DECRYPT, EVP_aes_256_cbc(), 0, pvar);
		len = buffer_len(prikey);
		decrypted = (char *)malloc(len);
		if (EVP_Cipher(&cipher_ctx, decrypted, prikey->buf, len) == 0) {
			strncpy_s(errmsg, errmsg_len, "Key decrypt error", _TRUNCATE);
			free(decrypted);
			cipher_cleanup_SSH2(&cipher_ctx);
			goto error;
		}
		buffer_clear(prikey);
		buffer_append(prikey, decrypted, len);
		free(decrypted);
		cipher_cleanup_SSH2(&cipher_ctx);
	}

	// verity MAC
	{
	char realmac[41];
	unsigned char binary[20];
	buffer_t *macdata;

	macdata = buffer_init();

	len = strlen(get_ssh_keytype_name(result->type));
	buffer_put_int(macdata, len);
	buffer_append(macdata, get_ssh_keytype_name(result->type), len);
	len = strlen(encname);
	buffer_put_int(macdata, len);
	buffer_append(macdata, encname, len);
	len = strlen(comment);
	buffer_put_int(macdata, len);
	buffer_append(macdata, comment, len);
	buffer_put_int(macdata, pubkey->len);
	buffer_append(macdata, pubkey->buf, pubkey->len);
	buffer_put_int(macdata, prikey->len);
	buffer_append(macdata, prikey->buf, prikey->len);
	
	if (private_mac != NULL) {
		unsigned char mackey[20];
		char header[] = "putty-private-key-file-mac-key";
		const EVP_MD *md = EVP_sha1();
		EVP_MD_CTX ctx;

		EVP_DigestInit(&ctx, md);
		EVP_DigestUpdate(&ctx, header, sizeof(header)-1);
		len = strlen(passphrase);
		if (strcmp(encname, "aes256-cbc") == 0 && len > 0) {
			EVP_DigestUpdate(&ctx, passphrase, len);
		}
		EVP_DigestFinal(&ctx, mackey, &len);

		//hmac_sha1_simple(mackey, sizeof(mackey), macdata->buf, macdata->len, binary);
		{
		EVP_MD_CTX ctx[2];
		unsigned char intermediate[20];
		unsigned char foo[64];
		int i;

		memset(foo, 0x36, sizeof(foo));
		for (i = 0; i < sizeof(mackey) && i < sizeof(foo); i++) {
			foo[i] ^= mackey[i];
		}
		EVP_DigestInit(&ctx[0], md);
		EVP_DigestUpdate(&ctx[0], foo, sizeof(foo));

		memset(foo, 0x5C, sizeof(foo));
		for (i = 0; i < sizeof(mackey) && i < sizeof(foo); i++) {
			foo[i] ^= mackey[i];
		}
		EVP_DigestInit(&ctx[1], md);
		EVP_DigestUpdate(&ctx[1], foo, sizeof(foo));

		memset(foo, 0, sizeof(foo));

		EVP_DigestUpdate(&ctx[0], macdata->buf, macdata->len);
		EVP_DigestFinal(&ctx[0], intermediate, &len);

		EVP_DigestUpdate(&ctx[1], intermediate, sizeof(intermediate));
		EVP_DigestFinal(&ctx[1], binary, &len);
		}

		memset(mackey, 0, sizeof(mackey));
		
	}
	else {
		strncpy_s(errmsg, errmsg_len, "key file format error", _TRUNCATE);
		buffer_free(macdata);
		goto error;
	}

	buffer_free(macdata);

	for (i=0; i<20; i++) {
		sprintf(realmac + 2*i, "%02x", binary[i]);
	}

	if (strcmp(private_mac, realmac) != 0) {
		if (strcmp(encname, "aes256-cbc") == 0) {
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
		pub = pubkey->buf;
		pri = prikey->buf;
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
		result->rsa->e = BN_new();
		result->rsa->n = BN_new();
		result->rsa->d = BN_new();
		result->rsa->p = BN_new();
		result->rsa->q = BN_new();
		result->rsa->iqmp = BN_new();
		if (result->rsa->e == NULL ||
		    result->rsa->n == NULL ||
		    result->rsa->d == NULL ||
		    result->rsa->p == NULL ||
		    result->rsa->q == NULL ||
		    result->rsa->iqmp == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		buffer_get_bignum2(&pub, result->rsa->e);
		buffer_get_bignum2(&pub, result->rsa->n);

		buffer_get_bignum2(&pri, result->rsa->d);
		buffer_get_bignum2(&pri, result->rsa->p);
		buffer_get_bignum2(&pri, result->rsa->q);
		buffer_get_bignum2(&pri, result->rsa->iqmp);

		break;
	}
	case KEY_DSA:
	{
		char *pubkey_type, *pub, *pri;
		pub = pubkey->buf;
		pri = prikey->buf;
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
		result->dsa->p = BN_new();
		result->dsa->q = BN_new();
		result->dsa->g = BN_new();
		result->dsa->pub_key = BN_new();
		result->dsa->priv_key = BN_new();
		if (result->dsa->p == NULL ||
		    result->dsa->q == NULL ||
		    result->dsa->g == NULL ||
		    result->dsa->pub_key == NULL ||
		    result->dsa->priv_key == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		buffer_get_bignum2(&pub, result->dsa->p);
		buffer_get_bignum2(&pub, result->dsa->q);
		buffer_get_bignum2(&pub, result->dsa->g);
		buffer_get_bignum2(&pub, result->dsa->pub_key);

		buffer_get_bignum2(&pri, result->dsa->priv_key);

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

		pub = pubkey->buf;
		pri = prikey->buf;
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
		pub = pubkey->buf;
		pri = prikey->buf;
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

	fclose(fp);

	if (encname != NULL)
		free(encname);

	if (comment != NULL)
		free(comment);

	if (pubkey != NULL)
		buffer_free(pubkey);

	if (prikey != NULL)
		buffer_free(prikey);

	if (private_mac != NULL)
		free(private_mac);

	return (result);

error:
	if (result != NULL)
		key_free(result);

	if (fp != NULL)
		fclose(fp);

	if (encname != NULL)
		free(encname);

	if (comment != NULL)
		free(comment);

	if (pubkey != NULL)
		buffer_free(pubkey);

	if (prikey != NULL)
		buffer_free(prikey);

	if (private_mac != NULL)
		free(private_mac);

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

	if (blob->len < 8) {
		strncpy_s(errmsg, errmsg_len, "key body not present", _TRUNCATE);
		goto error;
	}
	i = buffer_get_int(blob);
	if (i != 0x3f6ff9eb) {
		strncpy_s(errmsg, errmsg_len, "magic number error", _TRUNCATE);
		goto error;
	}
	len = buffer_get_int(blob);
	if (len == 0 || len > blob->len) {
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
		EVP_CIPHER_CTX cipher_ctx;
		char *decrypted = NULL;

		MD5_Init(&md);
		MD5_Update(&md, passphrase, strlen(passphrase));
		MD5_Final(key, &md);

		MD5_Init(&md);
		MD5_Update(&md, passphrase, strlen(passphrase));
		MD5_Update(&md, key, 16);
		MD5_Final(key + 16, &md);

		memset(iv, 0, sizeof(iv));

		// decrypt
		cipher_init_SSH2(&cipher_ctx, key, 24, iv, 8, CIPHER_DECRYPT, EVP_des_ede3_cbc(), 0, pvar);
		decrypted = (char *)malloc(len);
		if (EVP_Cipher(&cipher_ctx, decrypted, blob->buf + blob->offset, len) == 0) {
			strncpy_s(errmsg, errmsg_len, "Key decrypt error", _TRUNCATE);
			cipher_cleanup_SSH2(&cipher_ctx);
			goto error;
		}
		buffer_append(blob2, decrypted, len);
		free(decrypted);
		cipher_cleanup_SSH2(&cipher_ctx);

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
		result->rsa = RSA_new();
		if (result->rsa == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}
		result->rsa->e = BN_new();
		result->rsa->n = BN_new();
		result->rsa->d = BN_new();
		result->rsa->p = BN_new();
		result->rsa->q = BN_new();
		result->rsa->iqmp = BN_new();
		if (result->rsa->e == NULL ||
		    result->rsa->n == NULL ||
		    result->rsa->d == NULL ||
		    result->rsa->p == NULL ||
		    result->rsa->q == NULL ||
		    result->rsa->iqmp == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		buffer_get_bignum_SECSH(blob2, result->rsa->e);
		buffer_get_bignum_SECSH(blob2, result->rsa->d);
		buffer_get_bignum_SECSH(blob2, result->rsa->n);
		buffer_get_bignum_SECSH(blob2, result->rsa->iqmp);
		buffer_get_bignum_SECSH(blob2, result->rsa->p);
		buffer_get_bignum_SECSH(blob2, result->rsa->q);

		break;
	}
	case KEY_DSA:
	{
		int param;

		result->dsa = DSA_new();
		if (result->dsa == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}
		result->dsa->p = BN_new();
		result->dsa->q = BN_new();
		result->dsa->g = BN_new();
		result->dsa->pub_key = BN_new();
		result->dsa->priv_key = BN_new();
		if (result->dsa->p == NULL ||
		    result->dsa->q == NULL ||
		    result->dsa->g == NULL ||
		    result->dsa->pub_key == NULL ||
		    result->dsa->priv_key == NULL) {
			strncpy_s(errmsg, errmsg_len, "key init error", _TRUNCATE);
			goto error;
		}

		param = buffer_get_int(blob2);
		if (param != 0) {
			strncpy_s(errmsg, errmsg_len, "predefined DSA parameters not supported", _TRUNCATE);
			goto error;
		}
		buffer_get_bignum_SECSH(blob2, result->dsa->p);
		buffer_get_bignum_SECSH(blob2, result->dsa->g);
		buffer_get_bignum_SECSH(blob2, result->dsa->q);
		buffer_get_bignum_SECSH(blob2, result->dsa->pub_key);
		buffer_get_bignum_SECSH(blob2, result->dsa->priv_key);

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

ssh2_keyfile_type get_ssh2_keytype(char *relative_name,
                                   FILE **fp,
                                   char *errmsg,
                                   int errmsg_len) {
	ssh2_keyfile_type ret = SSH2_KEYFILE_TYPE_NONE;
	char filename[2048];
	char line[200];
	int i;

	// 相対パスを絶対パスへ変換する。こうすることにより、「ドットで始まる」ディレクトリに
	// あるファイルを読み込むことができる。(2005.2.7 yutaka)
	get_teraterm_dir_relative_name(filename, sizeof(filename),
	                               relative_name);

	*fp = fopen(filename, "r");
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
