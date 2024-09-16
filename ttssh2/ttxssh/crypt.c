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

#define WINDOWS

#include "ttxssh.h"
#include "util.h"

#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/err.h>
#include <openssl/des.h>
#include <openssl/hmac.h> // for SSH2(yutaka)
#include <openssl/dsa.h>
#include "cipher.h"
#include "ssh.h"

#define do_crc(buf, len) (~(uint32)crc32(0xFFFFFFFF, (buf), (len)))
#define get_uint32(buf) get_uint32_MSBfirst((buf))

#define DEATTACK_OK		0
#define DEATTACK_DETECTED	1

/*
 * $Id: crypt.c,v 1.28 2008-11-23 07:44:05 doda Exp $ Cryptographic attack
 * detector for ssh - source code (C)1998 CORE-SDI, Buenos Aires Argentina
 * Ariel Futoransky(futo@core-sdi.com) <http://www.core-sdi.com>
 */

/* SSH Constants */
#define SSH_BLOCKSIZE 8

/* Hashing constants */
#define HASH_MINSIZE     8*2048
#define HASH_ENTRYSIZE   4
#define HASH_FACTOR(x)   ((x)*3/2)
#define HASH_UNUSEDCHAR  0xff
#define HASH_UNUSED      0xffffffff
#define HASH_IV          0xfffffffe

#define HASH_MINBLOCKS  (7*SSH_BLOCKSIZE)

/* Hash function (Input keys are cipher results) */
#define HASH(x) get_uint32(x)

#define CMP(a,b) memcmp(a, b, SSH_BLOCKSIZE)

static unsigned char *encbuff = NULL;
static unsigned int encbufflen = 0;

static void crc_update(uint32 *a, uint32 b)
{
	b ^= *a;
	*a = do_crc((unsigned char *) &b, sizeof(b));
}

/* check_crc
   detects if a block is used in a particular pattern
*/

static int check_crc(unsigned char *S, unsigned char *buf,
                     uint32 len, unsigned char *IV)
{
	uint32 crc;
	unsigned char *c;

	crc = 0;
	if (IV && !CMP(S, IV)) {
		crc_update(&crc, 1);
		crc_update(&crc, 0);
	}
	for (c = buf; c < buf + len; c += SSH_BLOCKSIZE) {
		if (!CMP(S, c)) {
			crc_update(&crc, 1);
			crc_update(&crc, 0);
		} else {
			crc_update(&crc, 0);
			crc_update(&crc, 0);
		}
	}

	return crc == 0;
}


/*
detect_attack
Detects a crc32 compensation attack on a packet
*/
static int detect_attack(CRYPTDetectAttack *statics,
                         unsigned char *buf, uint32 len,
                         unsigned char *IV)
{
	uint32 *h = statics->h;
	uint32 n = statics->n;
	uint32 i, j;
	uint32 l;
	unsigned char *c;
	unsigned char *d;

	for (l = n; l < HASH_FACTOR(len / SSH_BLOCKSIZE); l = l << 2) {
	}

	if (h == NULL) {
		n = l;
		h = (uint32 *) malloc(n * HASH_ENTRYSIZE);
	} else {
		if (l > n) {
			n = l;
			h = (uint32 *) realloc(h, n * HASH_ENTRYSIZE);
		}
	}

	statics->h = h;
	statics->n = n;

	if (len <= HASH_MINBLOCKS) {
		for (c = buf; c < buf + len; c += SSH_BLOCKSIZE) {
			if (IV && (!CMP(c, IV))) {
				if ((check_crc(c, buf, len, IV)))
					return DEATTACK_DETECTED;
				else
					break;
			}
			for (d = buf; d < c; d += SSH_BLOCKSIZE) {
				if (!CMP(c, d)) {
					if ((check_crc(c, buf, len, IV)))
						return DEATTACK_DETECTED;
					else
						break;
				}
			}
		}
		return (DEATTACK_OK);
	}
	memset(h, HASH_UNUSEDCHAR, n * HASH_ENTRYSIZE);

	if (IV) {
		h[HASH(IV) & (n - 1)] = HASH_IV;
	}

	for (c = buf, j = 0; c < (buf + len); c += SSH_BLOCKSIZE, j++) {
		for (i = HASH(c) & (n - 1); h[i] != HASH_UNUSED;
			 i = (i + 1) & (n - 1)) {
			if (h[i] == HASH_IV) {
				if (!CMP(c, IV)) {
					if (check_crc(c, buf, len, IV))
						return DEATTACK_DETECTED;
					else
						break;
				}
			} else if (!CMP(c, buf + h[i] * SSH_BLOCKSIZE)) {
				if (check_crc(c, buf, len, IV))
					return DEATTACK_DETECTED;
				else
					break;
			}
		}
		h[i] = j;
	}

	return DEATTACK_OK;
}

BOOL CRYPT_detect_attack(PTInstVar pvar, unsigned char *buf, int bytes)
{
	if (SSHv1(pvar) && pvar->crypt_state.sender_cipher != SSH_CIPHER_NONE) {
		return detect_attack(&pvar->crypt_state.detect_attack_statics, buf, bytes, NULL) == DEATTACK_DETECTED;
	} else {
		return FALSE;
	}
}

BOOL CRYPT_encrypt_aead(PTInstVar pvar, unsigned char *data, unsigned int bytes, unsigned int aadlen, unsigned int authlen)
{
	unsigned char *newbuff = NULL;
	unsigned int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;
	unsigned char lastiv[1];
	char tmp[80];
	struct sshcipher_ctx *cc = pvar->cc[MODE_OUT];
	unsigned int newbuff_len = bytes;

	if (bytes == 0)
		return TRUE;

	if (bytes % block_size) {
		UTIL_get_lang_msg("MSG_ENCRYPT_ERROR1", pvar, "%s encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
		            get_cipher_name(pvar->crypt_state.sender_cipher),
		            bytes, block_size);
		notify_fatal_error(pvar, tmp, TRUE);
		return FALSE;
	}

	if (cc->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
		// chacha20-poly1305 �ł� aadlen ���Í����̑Ώ�
		//   aadlen �� bytes �͕ʁX�ɈÍ��������
		// chachapoly_crypt �̒��ŔF�؃f�[�^(AEAD tag)�����������
		newbuff_len += aadlen + authlen;
	}
	if (newbuff_len > encbufflen) {
		if ((newbuff = realloc(encbuff, newbuff_len)) == NULL)
			goto err;
		encbuff = newbuff;
		encbufflen = newbuff_len;
	}

	if (cc->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
		if (chachapoly_crypt(cc->cp_ctx, pvar->ssh_state.sender_sequence_number,
		                     encbuff, data, bytes, aadlen, authlen, 1) != 0) {
			goto err;
		}
		memcpy(data, encbuff, aadlen + bytes + authlen);
		return TRUE;
	}

	if (!EVP_CIPHER_CTX_ctrl(cc->evp, EVP_CTRL_GCM_IV_GEN, 1, lastiv))
		goto err;

	if (aadlen && !EVP_Cipher(cc->evp, NULL, data, aadlen) < 0)
		goto err;

	// AES-GCM �ł� aadlen ���Í������Ȃ��̂ŁA���̐悾���Í�������
	if (EVP_Cipher(cc->evp, encbuff, data+aadlen, bytes) < 0)
		goto err;

	memcpy(data+aadlen, encbuff, bytes);

	if (EVP_Cipher(cc->evp, NULL, NULL, 0) < 0)
		goto err;

	if (!EVP_CIPHER_CTX_ctrl(cc->evp, EVP_CTRL_GCM_GET_TAG, authlen, data+aadlen+bytes))
		goto err;

	return TRUE;

err:
	UTIL_get_lang_msg("MSG_ENCRYPT_ERROR2", pvar, "%s encrypt error(2)");
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
	            get_cipher_name(pvar->crypt_state.sender_cipher));
	notify_fatal_error(pvar, tmp, TRUE);
	return FALSE;
}

BOOL CRYPT_decrypt_aead(PTInstVar pvar, unsigned char *data, unsigned int bytes, unsigned int aadlen, unsigned int authlen)
{
	unsigned char *newbuff = NULL;
	unsigned int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;
	unsigned char lastiv[1];
	char tmp[80];
	struct sshcipher_ctx *cc = pvar->cc[MODE_IN];
	unsigned int newbuff_len = bytes;

	if (bytes == 0)
		return TRUE;

	if (bytes % block_size) {
		UTIL_get_lang_msg("MSG_DECRYPT_ERROR1", pvar, "%s decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
		            get_cipher_name(pvar->crypt_state.receiver_cipher),
		            bytes, block_size);
		notify_fatal_error(pvar, tmp, TRUE);
		return FALSE;
	}

	if (cc->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
		// chacha20-poly1305 �ł� aadlen ���Í�������Ă���
		newbuff_len += aadlen;
	}
	if (newbuff_len > encbufflen) {
		if ((newbuff = realloc(encbuff, newbuff_len)) == NULL)
			goto err;
		encbuff = newbuff;
		encbufflen = newbuff_len;
	}

	if (cc->cipher->id == SSH2_CIPHER_CHACHAPOLY) {
		if (chachapoly_crypt(cc->cp_ctx, pvar->ssh_state.receiver_sequence_number,
		                     encbuff, data, bytes, aadlen, authlen, 0) != 0) {
			goto err;
		}
		memcpy(data, encbuff, aadlen + bytes);
		return TRUE;
	}

	if (!EVP_CIPHER_CTX_ctrl(cc->evp, EVP_CTRL_GCM_IV_GEN, 1, lastiv))
		goto err;

	if (!EVP_CIPHER_CTX_ctrl(cc->evp, EVP_CTRL_GCM_SET_TAG, authlen, data+aadlen+bytes))
		goto err;

	if (aadlen && !EVP_Cipher(cc->evp, NULL, data, aadlen) < 0)
		goto err;

	// AES-GCM �ł� aadlen ���Í������Ȃ��̂ŁA���̐悾����������
	if (EVP_Cipher(cc->evp, encbuff, data+aadlen, bytes) < 0)
		goto err;

	memcpy(data+aadlen, encbuff, bytes);

	if (EVP_Cipher(cc->evp, NULL, NULL, 0) < 0)
		goto err;

	return TRUE;

err:
	UTIL_get_lang_msg("MSG_DECRYPT_ERROR2", pvar, "%s decrypt error(2)");
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
	            get_cipher_name(pvar->crypt_state.receiver_cipher));
	notify_fatal_error(pvar, tmp, TRUE);
	return FALSE;
}

static void no_encrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
}

static void crypt_SSH2_encrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	unsigned char *newbuff;
	int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;
	char tmp[80];

	if (bytes == 0)
		return;

	if (bytes % block_size) {
		UTIL_get_lang_msg("MSG_ENCRYPT_ERROR1", pvar,
		                  "%s encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
		            get_cipher_name(pvar->crypt_state.sender_cipher),
		            bytes, block_size);
		notify_fatal_error(pvar, tmp, TRUE);
		return;
	}

	if (bytes > encbufflen) {
		if ((newbuff = realloc(encbuff, bytes)) == NULL)
			return;
		encbuff = newbuff;
		encbufflen = bytes;
	}

	if (EVP_Cipher(pvar->cc[MODE_OUT]->evp, encbuff, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_ENCRYPT_ERROR2", pvar, "%s encrypt error(2)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
		            get_cipher_name(pvar->crypt_state.sender_cipher));
		notify_fatal_error(pvar, tmp, TRUE);
	} else {
		memcpy(buf, encbuff, bytes);
	}
}

static void crypt_SSH2_decrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	unsigned char *newbuff;
	int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;
	char tmp[80];

	if (bytes == 0)
		return;

	if (bytes % block_size) {
		UTIL_get_lang_msg("MSG_DECRYPT_ERROR1", pvar,
		                  "%s decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
		            get_cipher_name(pvar->crypt_state.receiver_cipher),
		            bytes, block_size);
		notify_fatal_error(pvar, tmp, TRUE);
		return;
	}

	if (bytes > encbufflen) {
		if ((newbuff = malloc(bytes)) == NULL)
			return;
		encbuff = newbuff;
		encbufflen = bytes;
	}

	if (EVP_Cipher(pvar->cc[MODE_IN]->evp, encbuff, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_DECRYPT_ERROR2", pvar, "%s decrypt error(2)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->UIMsg,
		            get_cipher_name(pvar->crypt_state.receiver_cipher));
		notify_fatal_error(pvar, tmp, TRUE);
	} else {
		memcpy(buf, encbuff, bytes);
	}
}

static void c3DES_encrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	Cipher3DESState *encryptstate = &pvar->crypt_state.enc.c3DES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k1, &encryptstate->ivec1, DES_ENCRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k2, &encryptstate->ivec2, DES_DECRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k3, &encryptstate->ivec3, DES_ENCRYPT);
}

static void c3DES_decrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	Cipher3DESState *decryptstate = &pvar->crypt_state.dec.c3DES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k3, &decryptstate->ivec3, DES_DECRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k2, &decryptstate->ivec2, DES_ENCRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k1, &decryptstate->ivec1, DES_DECRYPT);
}

static void cDES_encrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	CipherDESState *encryptstate = &pvar->crypt_state.enc.cDES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k, &encryptstate->ivec, DES_ENCRYPT);
}

static void cDES_decrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	CipherDESState *decryptstate = &pvar->crypt_state.dec.cDES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k, &decryptstate->ivec, DES_DECRYPT);
}

static void flip_endianness(unsigned char *cbuf, unsigned int bytes)
{
	uint32 *buf = (uint32 *) cbuf;
	int count = bytes / 4;

	while (count > 0) {
		uint32 w = *buf;

		*buf = ((w << 24) & 0xFF000000) | ((w << 8) & 0x00FF0000)
		     | ((w >> 8) & 0x0000FF00) | ((w >> 24) & 0x000000FF);
		count--;
		buf++;
	}
}

static void cBlowfish_encrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	CipherBlowfishState *encryptstate =
		&pvar->crypt_state.enc.cBlowfish;

	flip_endianness(buf, bytes);
	BF_cbc_encrypt(buf, buf, bytes, &encryptstate->k, encryptstate->ivec,
	               BF_ENCRYPT);
	flip_endianness(buf, bytes);
}

static void cBlowfish_decrypt(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	CipherBlowfishState *decryptstate =
		&pvar->crypt_state.dec.cBlowfish;

	flip_endianness(buf, bytes);
	BF_cbc_encrypt(buf, buf, bytes, &decryptstate->k, decryptstate->ivec,
	               BF_DECRYPT);
	flip_endianness(buf, bytes);
}

void CRYPT_set_random_data(PTInstVar pvar, unsigned char *buf, unsigned int bytes)
{
	int ret;

	// OpenSSL 1.1.1���g�����ꍇ�AWindowsMe�ł� RAND_bytes() �̌Ăяo���ŗ�����B
	logprintf(LOG_LEVEL_VERBOSE, "%s: RAND_bytes call", __FUNCTION__);
	ret = RAND_bytes(buf, bytes);
	if (ret < 0) {
		logprintf(LOG_LEVEL_ERROR, "%s: RAND_bytes error(%d)", __FUNCTION__, ret);
	}
}

void CRYPT_initialize_random_numbers(PTInstVar pvar)
{
	// �Ă΂Ȃ��Ă��悢�炵��
	// http://www.mail-archive.com/openssl-users@openssl.org/msg60484.html
	//RAND_screen();
}

static BIGNUM *get_bignum(unsigned char *bytes)
{
	int bits = get_ushort16_MSBfirst(bytes);

	return BN_bin2bn(bytes + 2, (bits + 7) / 8, NULL);
}

// make_key()�� fingerprint �����ł����p����̂ŁAstatic���폜�B(2006.3.27 yutaka)
RSA *make_key(PTInstVar pvar,
              int bits, unsigned char *exp,
              unsigned char *mod)
{
	RSA *key = RSA_new();
	BIGNUM *e = NULL, *n = NULL;

	if (key != NULL) {
		// OpenSSL 1.1.0�ł�RSA�\���̂̃����o�[�ɒ��ڃA�N�Z�X�ł��Ȃ����߁A
		// RSA_set0_key�֐��Őݒ肷��K�v������B
		e = get_bignum(exp);
		n = get_bignum(mod);
		RSA_set0_key(key, n, e, NULL);
	}

	if (key == NULL || e == NULL || n == NULL) {
		UTIL_get_lang_msg("MSG_RSAKEY_SETUP_ERROR", pvar,
		                  "Error setting up RSA keys");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);

		if (key != NULL) {
			if (e != NULL) {
				BN_free(e);
			}
			if (n != NULL) {
				BN_free(n);
			}
			RSA_free(key);
		}

		return NULL;
	} else {
		return key;
	}
}

void CRYPT_set_server_cookie(PTInstVar pvar, unsigned char *cookie)
{
	if (SSHv1(pvar)) {
		memcpy(pvar->crypt_state.server_cookie, cookie, SSH_COOKIE_LENGTH);
	} else {
		memcpy(pvar->crypt_state.server_cookie, cookie,
		       SSH2_COOKIE_LENGTH);
	}
}

void CRYPT_set_client_cookie(PTInstVar pvar, unsigned char *cookie)
{
	if (SSHv2(pvar)) {
		memcpy(pvar->crypt_state.client_cookie, cookie,
		       SSH2_COOKIE_LENGTH);
	}
}

BOOL CRYPT_set_server_RSA_key(PTInstVar pvar,
                              int bits, unsigned char *exp,
                              unsigned char *mod)
{
	pvar->crypt_state.server_key.RSA_key = make_key(pvar, bits, exp, mod);

	return pvar->crypt_state.server_key.RSA_key != NULL;
}

BOOL CRYPT_set_host_RSA_key(PTInstVar pvar,
                            int bits, unsigned char *exp,
                            unsigned char *mod)
{
	pvar->crypt_state.host_key.RSA_key = make_key(pvar, bits, exp, mod);

	return pvar->crypt_state.host_key.RSA_key != NULL;
}

BOOL CRYPT_set_supported_ciphers(PTInstVar pvar, int sender_ciphers,
                                 int receiver_ciphers)
{
	int cipher_mask;

	if (SSHv2(pvar)) {
		return TRUE;
	}

	cipher_mask = (1 << SSH_CIPHER_DES)
	            | (1 << SSH_CIPHER_3DES)
	            | (1 << SSH_CIPHER_BLOWFISH);

	sender_ciphers &= cipher_mask;
	receiver_ciphers &= cipher_mask;
	pvar->crypt_state.supported_sender_ciphers = sender_ciphers;
	pvar->crypt_state.supported_receiver_ciphers = receiver_ciphers;

	if (sender_ciphers == 0) {
		UTIL_get_lang_msg("MSG_UNAVAILABLE_CIPHERS_ERROR", pvar,
		                  "The server does not support any of the TTSSH encryption algorithms.\n"
		                  "A secure connection cannot be made in the TTSSH-to-server direction.\n"
		                  "The connection will be closed.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return FALSE;
	} else if (receiver_ciphers == 0) {
		UTIL_get_lang_msg("MSG_UNAVAILABLE_CIPHERS_ERROR", pvar,
		                  "The server does not support any of the TTSSH encryption algorithms.\n"
		                  "A secure connection cannot be made in the TTSSH-to-server direction.\n"
		                  "The connection will be closed.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return FALSE;
	} else {
		return TRUE;
	}
}

unsigned int CRYPT_get_decryption_block_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 8;
	} else {
		// �p�P�b�g��M���ɂ����镜���A���S���Y���̃u���b�N�T�C�Y (2004.11.7 yutaka)
		// cf. 3DES=8, AES128=16
		return (pvar->ssh2_keys[MODE_IN].enc.block_size);
	}
}

unsigned int CRYPT_get_encryption_block_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 8;
	} else {
		// �p�P�b�g���M���ɂ�����Í��A���S���Y���̃u���b�N�T�C�Y (2004.11.7 yutaka)
		// cf. 3DES=8, AES128=16
		return (pvar->ssh2_keys[MODE_OUT].enc.block_size);
	}
}

unsigned int CRYPT_get_receiver_MAC_size(PTInstVar pvar)
{
	struct Mac *mac;

	if (SSHv1(pvar)) {
		return 0;

	} else { // for SSH2(yutaka)
		mac = &pvar->ssh2_keys[MODE_IN].mac;
		if (mac == NULL || mac->enabled == 0)
			return 0;

		return (pvar->ssh2_keys[MODE_IN].mac.mac_len);
	}

}

// HMAC�̌���
// ���{�֐��� SSH2 �ł̂ݎg�p�����B
// (2004.12.17 yutaka)
BOOL CRYPT_verify_receiver_MAC(PTInstVar pvar, uint32 sequence_number,
                               char *data, int len, char *MAC)
{
	HMAC_CTX *c = NULL;
	unsigned char m[EVP_MAX_MD_SIZE];
	unsigned char b[4];
	struct Mac *mac;

	mac = &pvar->ssh2_keys[MODE_IN].mac;

	// HMAC���܂��L���łȂ��ꍇ�́A����OK�Ƃ��ĕԂ��B
	if (mac == NULL || mac->enabled == 0)
		return TRUE;

	if (mac->key == NULL) {
		logprintf(LOG_LEVEL_VERBOSE, "HMAC key is NULL(seq %lu len %d)", sequence_number, len);
		goto error;
	}

	if ((u_int)mac->mac_len > sizeof(m)) {
		logprintf(LOG_LEVEL_VERBOSE, "HMAC len(%d) is larger than %d bytes(seq %lu len %d)",
		          mac->mac_len, (int)sizeof(m), sequence_number, len);
		goto error;
	}

	c = HMAC_CTX_new();
	if (c == NULL)
		goto error;

	HMAC_Init_ex(c, mac->key, mac->key_len, mac->md, NULL);
	set_uint32_MSBfirst(b, sequence_number);
	HMAC_Update(c, b, sizeof(b));
	HMAC_Update(c, data, len);
	HMAC_Final(c, m, NULL);
	// HMAC_cleanup()��OpenSSL 1.1.0�ō폜����AHMAC_CTX_free()�ɏW�񂳂ꂽ�B

	if (memcmp(m, MAC, mac->mac_len)) {
		logprintf(LOG_LEVEL_VERBOSE, "HMAC key is not matched(seq %lu len %d)", sequence_number, len);
		logprintf_hexdump(LOG_LEVEL_VERBOSE, m, mac->mac_len, "m:");
		logprintf_hexdump(LOG_LEVEL_VERBOSE, MAC, mac->mac_len, "MAC:");
		goto error;
	}

	HMAC_CTX_free(c);

	return TRUE;

error:
	if (c)
		HMAC_CTX_free(c);

	return FALSE;
}

unsigned int CRYPT_get_sender_MAC_size(PTInstVar pvar)
{
	struct Mac *mac;

	if (SSHv2(pvar)) {	// for SSH2(yutaka)
		mac = &pvar->ssh2_keys[MODE_OUT].mac;
		if (mac == NULL || mac->enabled == 0)
			return 0;

		return (mac->mac_len);
	}

	return 0;
}

// for SSH2
BOOL CRYPT_build_sender_MAC(PTInstVar pvar, uint32 sequence_number,
                            char *data, int len, char *MAC)
{
	HMAC_CTX *c = NULL;
	static u_char m[EVP_MAX_MD_SIZE];
	u_char b[4];
	struct Mac *mac;

	if (SSHv2(pvar)) { // for SSH2(yutaka)
		mac = &pvar->ssh2_keys[MODE_OUT].mac;
		if (mac == NULL || mac->enabled == 0)
			return FALSE;

		c = HMAC_CTX_new();
		if (c == NULL)
			return FALSE;

		HMAC_Init_ex(c, mac->key, mac->key_len, mac->md, NULL);
		set_uint32_MSBfirst(b, sequence_number);
		HMAC_Update(c, b, sizeof(b));
		HMAC_Update(c, data, len);
		HMAC_Final(c, m, NULL);
		// HMAC_cleanup()��OpenSSL 1.1.0�ō폜����AHMAC_CTX_free()�ɏW�񂳂ꂽ�B

		// 20�o�C�g�������R�s�[
		memcpy(MAC, m, pvar->ssh2_keys[MODE_OUT].mac.mac_len);
	//	memcpy(MAC, m, sizeof(m));

		HMAC_CTX_free(c);

		return TRUE;
	}

	return TRUE;

}

static int choose_cipher(PTInstVar pvar, int supported)
{
	int i;

	for (i = 0; pvar->session_settings.CipherOrder[i] != 0; i++) {
		int cipher = pvar->session_settings.CipherOrder[i] - '0';

		if (cipher == SSH_CIPHER_NONE) {
			break;
		} else if ((supported & (1 << cipher)) != 0) {
			return cipher;
		}
	}

	return SSH_CIPHER_NONE;
}

BOOL CRYPT_choose_ciphers(PTInstVar pvar)
{
	pvar->crypt_state.sender_cipher =
		choose_cipher(pvar, pvar->crypt_state.supported_sender_ciphers);
	pvar->crypt_state.receiver_cipher =
		choose_cipher(pvar, pvar->crypt_state.supported_receiver_ciphers);

	if (pvar->crypt_state.sender_cipher == SSH_CIPHER_NONE
		|| pvar->crypt_state.receiver_cipher == SSH_CIPHER_NONE) {
		UTIL_get_lang_msg("MSG_CIPHER_NONE_ERROR", pvar,
		                  "All the encryption algorithms that this program and the server both understand have been disabled.\n"
		                  "To communicate with this server, you will have to enable some more ciphers\n"
		                  "in the TTSSH Setup dialog box when you run Tera Term again.\n"
		                  "This connection will now close.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return FALSE;
	} else {
		return TRUE;
	}
}

unsigned int CRYPT_get_encrypted_session_key_len(PTInstVar pvar)
{
	int server_key_bits;
	int host_key_bits;
	int server_key_bytes;
	int host_key_bytes;
	BIGNUM *n;

	// OpenSSL 1.1.0�ł�RSA�\���̂̃����o�[�ɒ��ڃA�N�Z�X�ł��Ȃ����߁A
	// RSA_get0_key�֐��Ŏ擾����K�v������B
	RSA_get0_key(pvar->crypt_state.server_key.RSA_key, &n, NULL, NULL);
	server_key_bits = BN_num_bits(n);

	RSA_get0_key(pvar->crypt_state.host_key.RSA_key, &n, NULL, NULL);
	host_key_bits = BN_num_bits(n);

	server_key_bytes = (server_key_bits + 7) / 8;
	host_key_bytes = (host_key_bits + 7) / 8;

	if (server_key_bits < host_key_bits) {
		return host_key_bytes;
	} else {
		return server_key_bytes;
	}
}

int CRYPT_choose_session_key(PTInstVar pvar,
                             unsigned char *encrypted_key_buf)
{
	int server_key_bits;
	int host_key_bits;
	int server_key_bytes;
	int host_key_bytes;
	int encrypted_key_bytes;
	int bit_delta;
	BIGNUM *server_n, *host_n;

	// OpenSSL 1.1.0�ł�RSA�\���̂̃����o�[�ɒ��ڃA�N�Z�X�ł��Ȃ����߁A
	// RSA_get0_key�֐��Ŏ擾����K�v������B
	RSA_get0_key(pvar->crypt_state.server_key.RSA_key, &server_n, NULL, NULL);
	server_key_bits = BN_num_bits(server_n);

	RSA_get0_key(pvar->crypt_state.host_key.RSA_key, &host_n, NULL, NULL);
	host_key_bits = BN_num_bits(host_n);

	server_key_bytes = (server_key_bits + 7) / 8;
	host_key_bytes = (host_key_bits + 7) / 8;

	if (server_key_bits < host_key_bits) {
		encrypted_key_bytes = host_key_bytes;
		bit_delta = host_key_bits - server_key_bits;
	} else {
		encrypted_key_bytes = server_key_bytes;
		bit_delta = server_key_bits - host_key_bits;
	}

	if (bit_delta < 128 || server_key_bits < 512 || host_key_bits < 512) {
		UTIL_get_lang_msg("MSG_RSAKEY_TOOWEAK_ERROR", pvar,
		                  "Server RSA keys are too weak. A secure connection cannot be established.");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return 0;
	} else {
		/* following Goldberg's code, I'm using MD5(servkey->n || hostkey->n || cookie)
		   for the session ID, rather than the one specified in the RFC */
		int session_buf_len = server_key_bytes + host_key_bytes + 8;
		char *session_buf = (char *) malloc(session_buf_len);
		char session_id[16];
		int i;

		BN_bn2bin(host_n, session_buf);
		BN_bn2bin(server_n,
		          session_buf + host_key_bytes);
		memcpy(session_buf + server_key_bytes + host_key_bytes,
		       pvar->crypt_state.server_cookie, 8);
		MD5(session_buf, session_buf_len, session_id);

		free(session_buf);

		RAND_bytes(pvar->crypt_state.sender_cipher_key,
		           SSH_SESSION_KEY_LENGTH);
		memcpy(pvar->crypt_state.receiver_cipher_key,
		       pvar->crypt_state.sender_cipher_key,
		       SSH_SESSION_KEY_LENGTH);

		memcpy(encrypted_key_buf + encrypted_key_bytes -
		       SSH_SESSION_KEY_LENGTH, pvar->crypt_state.sender_cipher_key,
		       SSH_SESSION_KEY_LENGTH);
		for (i = 0; i < sizeof(session_id); i++) {
			encrypted_key_buf[encrypted_key_bytes -
			                  SSH_SESSION_KEY_LENGTH + i]
				^= session_id[i];
		}

		if (host_key_bits > server_key_bits) {
			if (RSA_public_encrypt(SSH_SESSION_KEY_LENGTH,
			                       encrypted_key_buf +
			                       encrypted_key_bytes -
			                       SSH_SESSION_KEY_LENGTH,
			                       encrypted_key_buf +
			                       encrypted_key_bytes - server_key_bytes,
			                       pvar->crypt_state.server_key.RSA_key,
			                       RSA_PKCS1_PADDING) < 0)
				return 0;

			if (RSA_public_encrypt(server_key_bytes,
			                       encrypted_key_buf +
			                       encrypted_key_bytes - server_key_bytes,
			                       encrypted_key_buf,
			                       pvar->crypt_state.host_key.RSA_key,
			                       RSA_PKCS1_PADDING) < 0)
				return 0;
		} else {
			if (RSA_public_encrypt(SSH_SESSION_KEY_LENGTH,
			                       encrypted_key_buf +
			                       encrypted_key_bytes -
			                       SSH_SESSION_KEY_LENGTH,
			                       encrypted_key_buf +
			                       encrypted_key_bytes - host_key_bytes,
			                       pvar->crypt_state.host_key.RSA_key,
			                       RSA_PKCS1_PADDING) < 0)
				return 0;

			if (RSA_public_encrypt(host_key_bytes,
			                       encrypted_key_buf +
			                       encrypted_key_bytes - host_key_bytes,
			                       encrypted_key_buf,
			                       pvar->crypt_state.server_key.RSA_key,
			                       RSA_PKCS1_PADDING) < 0)
				return 0;
		}
	}

	return 1;
}

int CRYPT_generate_RSA_challenge_response(PTInstVar pvar,
                                          unsigned char *challenge,
                                          int challenge_len,
                                          unsigned char *response)
{
	int server_key_bits;
	int host_key_bits;
	int server_key_bytes;
	int host_key_bytes;
	int session_buf_len;
	char *session_buf;
	char decrypted_challenge[48];
	int decrypted_challenge_len;
	BIGNUM *server_n, *host_n;

	// OpenSSL 1.1.0�ł�RSA�\���̂̃����o�[�ɒ��ڃA�N�Z�X�ł��Ȃ����߁A
	// RSA_get0_key�֐��Ŏ擾����K�v������B
	RSA_get0_key(pvar->crypt_state.server_key.RSA_key, &server_n, NULL, NULL);
	server_key_bits = BN_num_bits(server_n);

	RSA_get0_key(pvar->crypt_state.host_key.RSA_key, &host_n, NULL, NULL);
	host_key_bits = BN_num_bits(host_n);

	server_key_bytes = (server_key_bits + 7) / 8;
	host_key_bytes = (host_key_bits + 7) / 8;
	session_buf_len = server_key_bytes + host_key_bytes + 8;
	session_buf = (char *) malloc(session_buf_len);

	decrypted_challenge_len =
		RSA_private_decrypt(challenge_len, challenge, challenge,
		                    AUTH_get_cur_cred(pvar)->key_pair->rsa,
		                    RSA_PKCS1_PADDING);
	if (decrypted_challenge_len < 0) {
		free(session_buf);
		return 0;
	}
	if (decrypted_challenge_len >= SSH_RSA_CHALLENGE_LENGTH) {
		memcpy(decrypted_challenge,
		       challenge + decrypted_challenge_len -
		       SSH_RSA_CHALLENGE_LENGTH, SSH_RSA_CHALLENGE_LENGTH);
	} else {
		SecureZeroMemory(decrypted_challenge,
		                 SSH_RSA_CHALLENGE_LENGTH - decrypted_challenge_len);
		memcpy(decrypted_challenge + SSH_RSA_CHALLENGE_LENGTH -
		       decrypted_challenge_len, challenge,
		       decrypted_challenge_len);
	}

	BN_bn2bin(host_n, session_buf);
	BN_bn2bin(server_n,
	          session_buf + host_key_bytes);
	memcpy(session_buf + server_key_bytes + host_key_bytes,
	       pvar->crypt_state.server_cookie, 8);
	MD5(session_buf, session_buf_len, decrypted_challenge + 32);

	free(session_buf);

	MD5(decrypted_challenge, 48, response);

	return 1;
}

static void c3DES_init(char *session_key, Cipher3DESState *state)
{
	DES_set_key((const_DES_cblock *) session_key, &state->k1);
	DES_set_key((const_DES_cblock *) (session_key + 8), &state->k2);
	DES_set_key((const_DES_cblock *) (session_key + 16), &state->k3);
	SecureZeroMemory(state->ivec1, 8);
	SecureZeroMemory(state->ivec2, 8);
	SecureZeroMemory(state->ivec3, 8);
}

static void cDES_init(char *session_key, CipherDESState *state)
{
	DES_set_key((const_DES_cblock *) session_key, &state->k);
	SecureZeroMemory(state->ivec, 8);
}

static void cBlowfish_init(char *session_key,
                           CipherBlowfishState *state)
{
	BF_set_key(&state->k, 32, session_key);
	SecureZeroMemory(state->ivec, 8);
}

BOOL CRYPT_start_encryption(PTInstVar pvar, int sender_flag, int receiver_flag)
{
	struct Enc *enc;
	char *encryption_key = pvar->crypt_state.sender_cipher_key;
	char *decryption_key = pvar->crypt_state.receiver_cipher_key;
	const struct ssh2cipher *cipher;
	BOOL isOK = TRUE;

	if (sender_flag) {
		if (SSHv1(pvar)) {
			switch (pvar->crypt_state.sender_cipher) {
			case SSH_CIPHER_3DES:
				c3DES_init(encryption_key, &pvar->crypt_state.enc.c3DES);
				pvar->crypt_state.encrypt = c3DES_encrypt;
				break;

			case SSH_CIPHER_DES:
				cDES_init(encryption_key, &pvar->crypt_state.enc.cDES);
				pvar->crypt_state.encrypt = cDES_encrypt;
				break;

			case SSH_CIPHER_BLOWFISH:
				cBlowfish_init(encryption_key, &pvar->crypt_state.enc.cBlowfish);
				pvar->crypt_state.encrypt = cBlowfish_encrypt;
				break;

			case SSH_CIPHER_NONE:
			case SSH_CIPHER_IDEA:
			case SSH_CIPHER_TSS:
			case SSH_CIPHER_RC4:
				isOK = FALSE;
				break;
			}
		}
		else {
			// SSH2
			cipher = pvar->ciphers[MODE_OUT];
			if (cipher) {
				pvar->crypt_state.sender_cipher = get_cipher_id(pvar->ciphers[MODE_OUT]);
				enc = &pvar->ssh2_keys[MODE_OUT].enc;
				cipher_init_SSH2(&pvar->cc[MODE_OUT], cipher,
				                 enc->key, enc->key_len,
				                 enc->iv, enc->iv_len,
				                 CIPHER_ENCRYPT,
				                 pvar);
				pvar->crypt_state.encrypt = crypt_SSH2_encrypt;
			}
			else {
				pvar->crypt_state.sender_cipher = SSH_CIPHER_NONE;
				isOK = FALSE;
			}
		}
	}

	if (receiver_flag) {
		if (SSHv1(pvar)) {
			switch (pvar->crypt_state.receiver_cipher) {
			case SSH_CIPHER_3DES:
				c3DES_init(decryption_key, &pvar->crypt_state.dec.c3DES);
				pvar->crypt_state.decrypt = c3DES_decrypt;
				break;

			case SSH_CIPHER_DES:
				cDES_init(decryption_key, &pvar->crypt_state.dec.cDES);
				pvar->crypt_state.decrypt = cDES_decrypt;
				break;

			case SSH_CIPHER_BLOWFISH:
				cBlowfish_init(decryption_key, &pvar->crypt_state.dec.cBlowfish);
				pvar->crypt_state.decrypt = cBlowfish_decrypt;
				break;

			case SSH_CIPHER_NONE:
			case SSH_CIPHER_IDEA:
			case SSH_CIPHER_TSS:
			case SSH_CIPHER_RC4:
				isOK = FALSE;
				break;
			}
		}
		else {
			// SSH2
			cipher = pvar->ciphers[MODE_IN];
			if (cipher) {
				pvar->crypt_state.receiver_cipher = get_cipher_id(pvar->ciphers[MODE_IN]);
				enc = &pvar->ssh2_keys[MODE_IN].enc;
				cipher_init_SSH2(&pvar->cc[MODE_IN], cipher,
				                 enc->key, enc->key_len,
				                 enc->iv, enc->iv_len,
				                 CIPHER_DECRYPT,
				                 pvar);
				pvar->crypt_state.decrypt = crypt_SSH2_decrypt;
			}
			else {
				pvar->crypt_state.receiver_cipher = SSH_CIPHER_NONE;
				isOK = FALSE;
			}
		}
	}

	if (!isOK) {
		UTIL_get_lang_msg("MSG_CIPHER_NOTSELECTED_ERROR", pvar,
		                  "No cipher selected!");
		notify_fatal_error(pvar, pvar->UIMsg, TRUE);
		return FALSE;
	} else {
		SecureZeroMemory(encryption_key, CRYPT_KEY_LENGTH);
		SecureZeroMemory(decryption_key, CRYPT_KEY_LENGTH);
		return TRUE;
	}
}

void CRYPT_init(PTInstVar pvar)
{
	pvar->crypt_state.encrypt = no_encrypt;
	pvar->crypt_state.decrypt = no_encrypt;
	pvar->crypt_state.sender_cipher = SSH_CIPHER_NONE;
	pvar->crypt_state.receiver_cipher = SSH_CIPHER_NONE;
	pvar->crypt_state.server_key.RSA_key = NULL;
	pvar->crypt_state.host_key.RSA_key = NULL;

	pvar->crypt_state.detect_attack_statics.h = NULL;
	pvar->crypt_state.detect_attack_statics.n =
		HASH_MINSIZE / HASH_ENTRYSIZE;
}

void CRYPT_get_cipher_info(PTInstVar pvar, char *dest, int len)
{
	UTIL_get_lang_msgU8("DLG_ABOUT_CIPHER_INFO", pvar,
						"%s to server, %s from server");
	_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
	          get_cipher_name(pvar->crypt_state.sender_cipher),
	          get_cipher_name(pvar->crypt_state.receiver_cipher));
}

void CRYPT_get_server_key_info(PTInstVar pvar, char *dest, int len)
{
	BIGNUM *server_n, *host_n;

	// OpenSSL 1.1.0�ł�RSA�\���̂̃����o�[�ɒ��ڃA�N�Z�X�ł��Ȃ����߁A
	// RSA_get0_key�֐��Ŏ擾����K�v������B

	if (SSHv1(pvar)) {
		if (pvar->crypt_state.server_key.RSA_key == NULL
		 || pvar->crypt_state.host_key.RSA_key == NULL) {
			UTIL_get_lang_msgU8("DLG_ABOUT_KEY_NONE", pvar, "None");
			strncpy_s(dest, len, pvar->UIMsg, _TRUNCATE);
		} else {
			RSA_get0_key(pvar->crypt_state.server_key.RSA_key, &server_n, NULL, NULL);
			RSA_get0_key(pvar->crypt_state.host_key.RSA_key, &host_n, NULL, NULL);

			UTIL_get_lang_msgU8("DLG_ABOUT_KEY_INFO", pvar,
								"%d-bit server key, %d-bit host key");
			_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
			            BN_num_bits(server_n),
			            BN_num_bits(host_n));
		}
	} else { // SSH2
			UTIL_get_lang_msgU8("DLG_ABOUT_KEY_INFO2", pvar,
								"%d-bit client key, %d-bit server key");
			_snprintf_s(dest, len, _TRUNCATE, pvar->UIMsg,
			            pvar->client_key_bits,
			            pvar->server_key_bits);
	}
}

static void destroy_public_key(CRYPTPublicKey * key)
{
	if (key->RSA_key != NULL) {
		RSA_free(key->RSA_key);
		key->RSA_key = NULL;
	}
}

void CRYPT_free_public_key(CRYPTPublicKey * key)
{
	destroy_public_key(key);
	free(key);
}

void CRYPT_end(PTInstVar pvar)
{
	free(encbuff);
	encbuff = NULL;
	encbufflen = 0;

	destroy_public_key(&pvar->crypt_state.host_key);
	destroy_public_key(&pvar->crypt_state.server_key);

	if (pvar->crypt_state.detect_attack_statics.h != NULL) {
		SecureZeroMemory(pvar->crypt_state.detect_attack_statics.h,
		                 pvar->crypt_state.detect_attack_statics.n * HASH_ENTRYSIZE);
		free(pvar->crypt_state.detect_attack_statics.h);
	}

	SecureZeroMemory(pvar->crypt_state.sender_cipher_key,
	                 sizeof(pvar->crypt_state.sender_cipher_key));
	SecureZeroMemory(pvar->crypt_state.receiver_cipher_key,
	                 sizeof(pvar->crypt_state.receiver_cipher_key));
	SecureZeroMemory(pvar->crypt_state.server_cookie,
	                 sizeof(pvar->crypt_state.server_cookie));
	SecureZeroMemory(pvar->crypt_state.client_cookie,
	                 sizeof(pvar->crypt_state.client_cookie));
	SecureZeroMemory(&pvar->crypt_state.enc, sizeof(pvar->crypt_state.enc));
	SecureZeroMemory(&pvar->crypt_state.dec, sizeof(pvar->crypt_state.dec));
}

int CRYPT_passphrase_decrypt(int cipher, char *passphrase,
                             char *buf, int bytes)
{
	unsigned char passphrase_key[16];

	MD5(passphrase, strlen(passphrase), passphrase_key);

	switch (cipher) {
	case SSH_CIPHER_3DES:{
			Cipher3DESState state;

			DES_set_key((const_DES_cblock *) passphrase_key,
			            &state.k1);
			DES_set_key((const_DES_cblock *) (passphrase_key + 8),
			            &state.k2);
			DES_set_key((const_DES_cblock *) passphrase_key,
			            &state.k3);
			SecureZeroMemory(state.ivec1, 8);
			SecureZeroMemory(state.ivec2, 8);
			SecureZeroMemory(state.ivec3, 8);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k3, &state.ivec3, DES_DECRYPT);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k2, &state.ivec2, DES_ENCRYPT);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k1, &state.ivec1, DES_DECRYPT);
			break;
		}

	case SSH_CIPHER_DES:{
			CipherDESState state;

			cDES_init(passphrase_key, &state);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k, &state.ivec, DES_DECRYPT);
			break;
		}

	case SSH_CIPHER_BLOWFISH:{
			CipherBlowfishState state;

			BF_set_key(&state.k, 16, passphrase_key);
			SecureZeroMemory(state.ivec, 8);
			flip_endianness(buf, bytes);
			BF_cbc_encrypt(buf, buf, bytes, &state.k, state.ivec,
			               BF_DECRYPT);
			flip_endianness(buf, bytes);
			break;
		}

	case SSH_CIPHER_NONE:
		break;

	default:
		SecureZeroMemory(passphrase_key, sizeof(passphrase_key));
		return 0;
	}

	SecureZeroMemory(passphrase_key, sizeof(passphrase_key));
	return 1;
}
