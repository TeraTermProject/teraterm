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

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

static char ID_string[] = "SSH PRIVATE KEY FILE FORMAT 1.1\n";

static BIGNUM FAR *get_bignum(unsigned char FAR * bytes)
{
	int bits = get_ushort16_MSBfirst(bytes);

	return BN_bin2bn(bytes + 2, (bits + 7) / 8, NULL);
}

/* normalize the RSA key by precomputing various bits of it.
   This code is adapted from libeay's rsa_gen.c
   It's needed to work around "issues" with LIBEAY/RSAREF.
   If this function returns 0, then something went wrong and the
   key must be discarded. */
static BOOL normalize_key(RSA FAR * key)
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

static RSA FAR *read_RSA_private_key(PTInstVar pvar,
                                     char FAR * relative_name,
                                     char FAR * passphrase,
                                     BOOL FAR * invalid_passphrase,
                                     BOOL is_auto_login)
{
	char filename[2048];
	int fd;
	unsigned int length, amount_read;
	unsigned char *keyfile_data;
	unsigned int index;
	int cipher;
	RSA FAR *key;
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
		memset(keyfile_data, 0, length);
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
		memset(keyfile_data, 0, length);
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
		                  "Error in crytography library.\n"
		                  "Perhaps the stored key is invalid.");
		notify_nonfatal_error(pvar, pvar->ts->UIMsg);

		RSA_free(key);
		key = NULL;
	}

	memset(keyfile_data, 0, length);
	free(keyfile_data);
	return key;
}

CRYPTKeyPair FAR *KEYFILES_read_private_key(PTInstVar pvar,
                                            char FAR * relative_name,
                                            char FAR * passphrase,
                                            BOOL FAR * invalid_passphrase,
                                            BOOL is_auto_login)
{
	RSA FAR *RSA_key = read_RSA_private_key(pvar, relative_name,
	                                        passphrase, invalid_passphrase,
	                                        is_auto_login);

	if (RSA_key == NULL) {
		return FALSE;
	} else {
		CRYPTKeyPair FAR *result =
			(CRYPTKeyPair FAR *) malloc(sizeof(CRYPTKeyPair));

		// フリーするときに 0 かどうかで判別するため追加。(2004.12.20 yutaka)
		ZeroMemory(result, sizeof(CRYPTKeyPair)); 

		result->RSA_key = RSA_key;
		return result;
	}
}


//
// SSH2
//

CRYPTKeyPair *read_SSH2_private_key(PTInstVar pvar,
                                    char FAR * relative_name,
                                    char FAR * passphrase,
                                    BOOL FAR * invalid_passphrase,
                                    BOOL is_auto_login,
                                    char *errmsg,
                                    int errmsg_len)
{
	char filename[2048];
	FILE *fp = NULL;
	CRYPTKeyPair *result = NULL;
	EVP_PKEY *pk = NULL;
	unsigned long err = 0;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
	//seed_prng();

	// 相対パスを絶対パスへ変換する。こうすることにより、「ドットで始まる」ディレクトリに
	// あるファイルを読み込むことができる。(2005.2.7 yutaka)
	get_teraterm_dir_relative_name(filename, sizeof(filename),
	                               relative_name);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		strncpy_s(errmsg, errmsg_len, strerror(errno), _TRUNCATE);
		goto error;
	}

	result = (CRYPTKeyPair *)malloc(sizeof(CRYPTKeyPair));
	ZeroMemory(result, sizeof(CRYPTKeyPair)); 

	// ファイルからパスフレーズを元に秘密鍵を読み込む。
	pk = PEM_read_PrivateKey(fp, NULL, NULL, passphrase);
	if (pk == NULL) {
		err = ERR_get_error();
		ERR_error_string_n(err, errmsg, errmsg_len);
		*invalid_passphrase = TRUE;
		goto error;
	}

	if (pk->type == EVP_PKEY_RSA) { // RSA key
		result->RSA_key = EVP_PKEY_get1_RSA(pk);
		result->DSA_key = NULL;

		// RSA目くらましを有効にする（タイミング攻撃からの防御）
		if (RSA_blinding_on(result->RSA_key, NULL) != 1) {
			err = ERR_get_error();
			ERR_error_string_n(err, errmsg, errmsg_len);
			goto error;
		}

	} else if (pk->type == EVP_PKEY_DSA) { // DSA key
		result->RSA_key = NULL;
		result->DSA_key = EVP_PKEY_get1_DSA(pk);

	} else {
		goto error;
	}

	if (pk != NULL)
		EVP_PKEY_free(pk);

	fclose(fp);
	return (result);

error:
	if (pk != NULL)
		EVP_PKEY_free(pk);

	if (result != NULL)
		CRYPT_free_key_pair(result);

	if (fp != NULL)
		fclose(fp);

	return (NULL);
}
