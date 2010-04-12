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



static void crc_update(uint32 FAR * a, uint32 b)
{
	b ^= *a;
	*a = do_crc((unsigned char FAR *) &b, sizeof(b));
}

/* check_crc
   detects if a block is used in a particular pattern
*/

static int check_crc(unsigned char FAR * S, unsigned char FAR * buf,
                     uint32 len, unsigned char FAR * IV)
{
	uint32 crc;
	unsigned char FAR *c;

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
static int detect_attack(CRYPTDetectAttack FAR * statics,
                         unsigned char FAR * buf, uint32 len,
                         unsigned char *FAR IV)
{
	uint32 FAR *h = statics->h;
	uint32 n = statics->n;
	uint32 i, j;
	uint32 l;
	unsigned char FAR *c;
	unsigned char FAR *d;

	for (l = n; l < HASH_FACTOR(len / SSH_BLOCKSIZE); l = l << 2) {
	}

	if (h == NULL) {
		n = l;
		h = (uint32 FAR *) malloc(n * HASH_ENTRYSIZE);
	} else {
		if (l > n) {
			n = l;
			h = (uint32 FAR *) realloc(h, n * HASH_ENTRYSIZE);
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

BOOL CRYPT_detect_attack(PTInstVar pvar, unsigned char FAR * buf,
                         int bytes)
{
	if (SSHv1(pvar)) {
		switch (pvar->crypt_state.sender_cipher) {
		case SSH_CIPHER_NONE:
			return FALSE;
		case SSH_CIPHER_IDEA:
			return detect_attack(&pvar->crypt_state.detect_attack_statics,
			                     buf, bytes,
			                     pvar->crypt_state.dec.cIDEA.ivec) ==
			       DEATTACK_DETECTED;
		default:
			return detect_attack(&pvar->crypt_state.detect_attack_statics,
			                     buf, bytes, NULL) == DEATTACK_DETECTED;
		}
	} else {
		return FALSE;
	}
}

static void no_encrypt(PTInstVar pvar, unsigned char FAR * buf, int bytes)
{
}


// for SSH2(yutaka)
// 事前に設定する鍵長が違うだけなので、AES192, AES256 でも
// cAES128_encrypt/cAES128_decrypt を使用できる (2007.10.16 maya)
static void cAES128_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                            int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_AES128_ENCRYPT_ERROR1", pvar,
		                  "AES128/192/256 encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
		            pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_OUT], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_AES128_ENCRYPT_ERROR2", pvar,
		                  "AES128/192/256 encrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		//unsigned char key[AES128_KEYLEN], iv[AES128_IVLEN];
		//memcpy(key, pvar->ssh2_keys[MODE_OUT].enc.key, AES128_KEYLEN);
		// IVはDES関数内で更新されるため、ローカルにコピーしてから使う。
		//memcpy(iv, pvar->ssh2_keys[MODE_OUT].enc.iv, AES128_IVLEN);

		//debug_print(50, key, 24);
		//debug_print(51, iv, 8);
		//debug_print(52, buf, bytes);
		//debug_print(53, newbuf, bytes);

		memcpy(buf, newbuf, bytes);
	}

error:
	free(newbuf);
}

static void cAES128_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                            int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_AES128_DECRYPT_ERROR1", pvar,
		                  "AES128/192/256 decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_IN], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_AES128_DECRYPT_ERROR2", pvar,
		                  "AES128/192/256 decrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		//unsigned char key[AES128_KEYLEN], iv[AES128_IVLEN];
		//memcpy(key, pvar->ssh2_keys[MODE_IN].enc.key, AES128_KEYLEN);
		// IVはDES関数内で更新されるため、ローカルにコピーしてから使う。
		//memcpy(iv, pvar->ssh2_keys[MODE_IN].enc.iv, AES128_IVLEN);

		//debug_print(70, key, AES128_KEYLEN);
		//debug_print(71, iv, AES128_IVLEN);
		//debug_print(72, buf, bytes);
		//debug_print(73, newbuf, bytes);

		memcpy(buf, newbuf, bytes);
	}

error:
	free(newbuf);
}



// for SSH2(yutaka)
static void c3DES_encrypt2(PTInstVar pvar, unsigned char FAR * buf,
                              int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_3DESCBC_ENCRYPT_ERROR1", pvar,
		                  "3DES-CBC encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
		            pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_OUT], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_3DESCBC_ENCRYPT_ERROR2", pvar,
		                  "3DES-CBC encrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		//unsigned char key[24], iv[8];
		//memcpy(key, pvar->ssh2_keys[MODE_OUT].enc.key, 24);
		// IVはDES関数内で更新されるため、ローカルにコピーしてから使う。
		//memcpy(iv, pvar->ssh2_keys[MODE_OUT].enc.iv, 8);
		
		//debug_print(50, key, 24);
		//debug_print(51, iv, 8);
		//debug_print(52, buf, bytes);
		//debug_print(53, newbuf, bytes);

		memcpy(buf, newbuf, bytes);
	}

error:
	free(newbuf);
}

static void c3DES_decrypt2(PTInstVar pvar, unsigned char FAR * buf,
                              int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_3DESCBC_DECRYPT_ERROR1", pvar,
		                  "3DES-CBC decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_IN], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_3DESCBC_DECRYPT_ERROR2", pvar,
		                  "3DES-CBC decrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		//unsigned char key[24], iv[8];
		//memcpy(key, pvar->ssh2_keys[MODE_IN].enc.key, 24);
		// IVはDES関数内で更新されるため、ローカルにコピーしてから使う。
		//memcpy(iv, pvar->ssh2_keys[MODE_IN].enc.iv, 8);
		
		//debug_print(70, key, 24);
		//debug_print(71, iv, 8);
		//debug_print(72, buf, bytes);
		//debug_print(73, newbuf, bytes);

		memcpy(buf, newbuf, bytes);
	}

error:
	free(newbuf);
}


static void cBlowfish_encrypt2(PTInstVar pvar, unsigned char FAR * buf,
                               int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_BLOWFISH_ENCRYPT_ERROR1", pvar,
		                  "Blowfish encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
		            pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_OUT], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_BLOWFISH_ENCRYPT_ERROR2", pvar,
		                  "Blowfish encrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		memcpy(buf, newbuf, bytes);

	}

error:
	free(newbuf);
}

static void cBlowfish_decrypt2(PTInstVar pvar, unsigned char FAR * buf,
                               int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_BLOWFISH_DECRYPT_ERROR1", pvar,
		                  "Blowfish decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_IN], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_BLOWFISH_DECRYPT_ERROR2", pvar,
		                  "Blowfish decrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		memcpy(buf, newbuf, bytes);

	}

error:
	free(newbuf);
}

static void cArcfour_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                               int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_ARCFOUR_ENCRYPT_ERROR1", pvar,
		                  "Arcfour encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
		            pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_OUT], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_ARCFOUR_ENCRYPT_ERROR2", pvar,
		                  "Arcfour encrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		memcpy(buf, newbuf, bytes);

	}

error:
	free(newbuf);
}

static void cArcfour_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                               int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_ARCFOUR_DECRYPT_ERROR1", pvar,
		                  "Arcfour decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_IN], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_ARCFOUR_DECRYPT_ERROR2", pvar,
		                  "Arcfour decrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		memcpy(buf, newbuf, bytes);

	}

error:
	free(newbuf);
}

static void cCast128_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                             int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_OUT].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_CAST128_ENCRYPT_ERROR1", pvar,
		                  "CAST128 encrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
		            pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_OUT], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_CAST128_ENCRYPT_ERROR2", pvar,
		                  "CAST128 encrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		memcpy(buf, newbuf, bytes);

	}

error:
	free(newbuf);
}

static void cCast128_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                             int bytes)
{
	unsigned char *newbuf = malloc(bytes);
	int block_size = pvar->ssh2_keys[MODE_IN].enc.block_size;

	// 事前復号化により、全ペイロードが復号化されている場合は、0バイトになる。(2004.11.7 yutaka)
	if (bytes == 0)
		goto error;

	if (newbuf == NULL)
		return;

	if (bytes % block_size) {
		char tmp[80];
		UTIL_get_lang_msg("MSG_CAST128_DECRYPT_ERROR1", pvar,
		                  "CAST128 decrypt error(1): bytes %d (%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, bytes, block_size);
		notify_fatal_error(pvar, tmp);
		goto error;
	}

	if (EVP_Cipher(&pvar->evpcip[MODE_IN], newbuf, buf, bytes) == 0) {
		UTIL_get_lang_msg("MSG_CAST128_DECRYPT_ERROR2", pvar,
		                  "CAST128 decrypt error(2)");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		goto error;

	} else {
		memcpy(buf, newbuf, bytes);

	}

error:
	free(newbuf);
}



static void c3DES_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                          int bytes)
{
	Cipher3DESState FAR *encryptstate = &pvar->crypt_state.enc.c3DES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k1, &encryptstate->ivec1, DES_ENCRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k2, &encryptstate->ivec2, DES_DECRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k3, &encryptstate->ivec3, DES_ENCRYPT);
}

static void c3DES_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                          int bytes)
{
	Cipher3DESState FAR *decryptstate = &pvar->crypt_state.dec.c3DES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k3, &decryptstate->ivec3, DES_DECRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k2, &decryptstate->ivec2, DES_ENCRYPT);
	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k1, &decryptstate->ivec1, DES_DECRYPT);
}

static void cDES_encrypt(PTInstVar pvar, unsigned char FAR * buf,
						 int bytes)
{
	CipherDESState FAR *encryptstate = &pvar->crypt_state.enc.cDES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &encryptstate->k, &encryptstate->ivec, DES_ENCRYPT);
}

static void cDES_decrypt(PTInstVar pvar, unsigned char FAR * buf,
						 int bytes)
{
	CipherDESState FAR *decryptstate = &pvar->crypt_state.dec.cDES;

	DES_ncbc_encrypt(buf, buf, bytes,
	                 &decryptstate->k, &decryptstate->ivec, DES_DECRYPT);
}

static void cIDEA_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                          int bytes)
{
	CipherIDEAState FAR *encryptstate = &pvar->crypt_state.enc.cIDEA;
	int num = 0;

	idea_cfb64_encrypt(buf, buf, bytes, &encryptstate->k,
	                   encryptstate->ivec, &num, IDEA_ENCRYPT);
}

static void cIDEA_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                          int bytes)
{
	CipherIDEAState FAR *decryptstate = &pvar->crypt_state.dec.cIDEA;
	int num = 0;

	idea_cfb64_encrypt(buf, buf, bytes, &decryptstate->k,
	                   decryptstate->ivec, &num, IDEA_DECRYPT);
}

static void flip_endianness(unsigned char FAR * cbuf, int bytes)
{
	uint32 FAR *buf = (uint32 FAR *) cbuf;
	int count = bytes / 4;

	while (count > 0) {
		uint32 w = *buf;

		*buf = ((w << 24) & 0xFF000000) | ((w << 8) & 0x00FF0000)
		     | ((w >> 8) & 0x0000FF00) | ((w >> 24) & 0x000000FF);
		count--;
		buf++;
	}
}

static void cBlowfish_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                              int bytes)
{
	CipherBlowfishState FAR *encryptstate =
		&pvar->crypt_state.enc.cBlowfish;

	flip_endianness(buf, bytes);
	BF_cbc_encrypt(buf, buf, bytes, &encryptstate->k, encryptstate->ivec,
	               BF_ENCRYPT);
	flip_endianness(buf, bytes);
}

static void cBlowfish_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                              int bytes)
{
	CipherBlowfishState FAR *decryptstate =
		&pvar->crypt_state.dec.cBlowfish;

	flip_endianness(buf, bytes);
	BF_cbc_encrypt(buf, buf, bytes, &decryptstate->k, decryptstate->ivec,
	               BF_DECRYPT);
	flip_endianness(buf, bytes);
}

static void cRC4_encrypt(PTInstVar pvar, unsigned char FAR * buf,
                         int bytes)
{
	CipherRC4State FAR *encryptstate = &pvar->crypt_state.enc.cRC4;
	int num = 0;

	RC4(&encryptstate->k, bytes, buf, buf);
}

static void cRC4_decrypt(PTInstVar pvar, unsigned char FAR * buf,
                         int bytes)
{
	CipherRC4State FAR *decryptstate = &pvar->crypt_state.dec.cRC4;
	int num = 0;

	RC4(&decryptstate->k, bytes, buf, buf);
}

void CRYPT_set_random_data(PTInstVar pvar, unsigned char FAR * buf,
                           int bytes)
{
	RAND_bytes(buf, bytes);
}

void CRYPT_initialize_random_numbers(PTInstVar pvar)
{
	RAND_screen();
}

static BIGNUM FAR *get_bignum(unsigned char FAR * bytes)
{
	int bits = get_ushort16_MSBfirst(bytes);

	return BN_bin2bn(bytes + 2, (bits + 7) / 8, NULL);
}

// make_key()を fingerprint 生成でも利用するので、staticを削除。(2006.3.27 yutaka)
RSA FAR *make_key(PTInstVar pvar,
                  int bits, unsigned char FAR * exp,
                  unsigned char FAR * mod)
{
	RSA FAR *key = RSA_new();

	if (key != NULL) {
		key->e = get_bignum(exp);
		key->n = get_bignum(mod);
	}

	if (key == NULL || key->e == NULL || key->n == NULL) {
		UTIL_get_lang_msg("MSG_RSAKEY_SETUP_ERROR", pvar,
		                  "Error setting up RSA keys");
		notify_fatal_error(pvar, pvar->ts->UIMsg);

		if (key != NULL) {
			if (key->e != NULL) {
				BN_free(key->e);
			}
			if (key->n != NULL) {
				BN_free(key->n);
			}
			RSA_free(key);
		}

		return NULL;
	} else {
		return key;
	}
}

void CRYPT_set_server_cookie(PTInstVar pvar, unsigned char FAR * cookie)
{
	if (SSHv1(pvar)) {
		memcpy(pvar->crypt_state.server_cookie, cookie, SSH_COOKIE_LENGTH);
	} else {
		memcpy(pvar->crypt_state.server_cookie, cookie,
		       SSH2_COOKIE_LENGTH);
	}
}

void CRYPT_set_client_cookie(PTInstVar pvar, unsigned char FAR * cookie)
{
	if (SSHv2(pvar)) {
		memcpy(pvar->crypt_state.client_cookie, cookie,
		       SSH2_COOKIE_LENGTH);
	}
}

BOOL CRYPT_set_server_RSA_key(PTInstVar pvar,
                              int bits, unsigned char FAR * exp,
                              unsigned char FAR * mod)
{
	pvar->crypt_state.server_key.RSA_key = make_key(pvar, bits, exp, mod);

	return pvar->crypt_state.server_key.RSA_key != NULL;
}

BOOL CRYPT_set_host_RSA_key(PTInstVar pvar,
                            int bits, unsigned char FAR * exp,
                            unsigned char FAR * mod)
{
	pvar->crypt_state.host_key.RSA_key = make_key(pvar, bits, exp, mod);

	return pvar->crypt_state.host_key.RSA_key != NULL;
}

BOOL CRYPT_set_supported_ciphers(PTInstVar pvar, int sender_ciphers,
                                 int receiver_ciphers)
{
	int cipher_mask;

	if (SSHv1(pvar)) {
		cipher_mask = (1 << SSH_CIPHER_DES)
		            | (1 << SSH_CIPHER_3DES)
		            | (1 << SSH_CIPHER_BLOWFISH);

	} else { // for SSH2(yutaka)
		// SSH2がサポートするデータ通信用アルゴリズム（公開鍵交換用とは別）
		cipher_mask = (1 << SSH2_CIPHER_3DES_CBC)
		            | (1 << SSH2_CIPHER_AES128_CBC)
		            | (1 << SSH2_CIPHER_AES192_CBC)
		            | (1 << SSH2_CIPHER_AES256_CBC)
		            | (1 << SSH2_CIPHER_BLOWFISH_CBC)
		            | (1 << SSH2_CIPHER_AES128_CTR)
		            | (1 << SSH2_CIPHER_AES192_CTR)
		            | (1 << SSH2_CIPHER_AES256_CTR)
		            | (1 << SSH2_CIPHER_ARCFOUR)
		            | (1 << SSH2_CIPHER_ARCFOUR128)
		            | (1 << SSH2_CIPHER_ARCFOUR256)
		            | (1 << SSH2_CIPHER_CAST128_CBC)
		            | (1 << SSH2_CIPHER_3DES_CTR)
		            | (1 << SSH2_CIPHER_BLOWFISH_CTR)
		            | (1 << SSH2_CIPHER_CAST128_CTR);
	}

	sender_ciphers &= cipher_mask;
	receiver_ciphers &= cipher_mask;
	pvar->crypt_state.supported_sender_ciphers = sender_ciphers;
	pvar->crypt_state.supported_receiver_ciphers = receiver_ciphers;

	if (sender_ciphers == 0) {
		UTIL_get_lang_msg("MSG_UNAVAILABLE_CIPHERS_ERROR", pvar,
		                  "The server does not support any of the TTSSH encryption algorithms.\n"
		                  "A secure connection cannot be made in the TTSSH-to-server direction.\n"
		                  "The connection will be closed.");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		return FALSE;
	} else if (receiver_ciphers == 0) {
		UTIL_get_lang_msg("MSG_UNAVAILABLE_CIPHERS_ERROR", pvar,
		                  "The server does not support any of the TTSSH encryption algorithms.\n"
		                  "A secure connection cannot be made in the TTSSH-to-server direction.\n"
		                  "The connection will be closed.");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		return FALSE;
	} else {
		return TRUE;
	}
}

int CRYPT_get_decryption_block_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 8;
	} else {
		// パケット受信時における復号アルゴリズムのブロックサイズ (2004.11.7 yutaka)
		// cf. 3DES=8, AES128=16
		return (pvar->ssh2_keys[MODE_IN].enc.block_size);
	}
}

int CRYPT_get_encryption_block_size(PTInstVar pvar)
{
	if (SSHv1(pvar)) {
		return 8;
	} else {
		// パケット送信時における暗号アルゴリズムのブロックサイズ (2004.11.7 yutaka)
		// cf. 3DES=8, AES128=16
		return (pvar->ssh2_keys[MODE_OUT].enc.block_size);
	}
}

int CRYPT_get_receiver_MAC_size(PTInstVar pvar)
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

// HMACの検証 
// ※本関数は SSH2 でのみ使用される。
// (2004.12.17 yutaka)
BOOL CRYPT_verify_receiver_MAC(PTInstVar pvar, uint32 sequence_number,
                               char FAR * data, int len, char FAR * MAC)
{
	HMAC_CTX c;
	unsigned char m[EVP_MAX_MD_SIZE];
	unsigned char b[4];
	struct Mac *mac;

	mac = &pvar->ssh2_keys[MODE_IN].mac;

	// HMACがまだ有効でない場合は、検証OKとして返す。
	if (mac == NULL || mac->enabled == 0) 
		return TRUE;

	if (mac->key == NULL)
		goto error;

	if ((u_int)mac->mac_len > sizeof(m))
		goto error;

	HMAC_Init(&c, mac->key, mac->key_len, mac->md);
	set_uint32_MSBfirst(b, sequence_number);
	HMAC_Update(&c, b, sizeof(b));
	HMAC_Update(&c, data, len);
	HMAC_Final(&c, m, NULL);
	HMAC_cleanup(&c);

	if (memcmp(m, MAC, mac->mac_len)) {
		goto error;
	}

	return TRUE;

error:
	return FALSE;
}

int CRYPT_get_sender_MAC_size(PTInstVar pvar)
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
                            char FAR * data, int len, char FAR * MAC)
{
	HMAC_CTX c;
	static u_char m[EVP_MAX_MD_SIZE];
	u_char b[4];
	struct Mac *mac;

	if (SSHv2(pvar)) { // for SSH2(yutaka)
		mac = &pvar->ssh2_keys[MODE_OUT].mac;
		if (mac == NULL || mac->enabled == 0) 
			return FALSE;

		HMAC_Init(&c, mac->key, mac->key_len, mac->md);
		set_uint32_MSBfirst(b, sequence_number);
		HMAC_Update(&c, b, sizeof(b));
		HMAC_Update(&c, data, len);
		HMAC_Final(&c, m, NULL);
		HMAC_cleanup(&c);

		// 20バイト分だけコピー
		memcpy(MAC, m, pvar->ssh2_keys[MODE_OUT].mac.mac_len);
	//	memcpy(MAC, m, sizeof(m));

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
	if (SSHv1(pvar)) {
		pvar->crypt_state.sender_cipher = choose_cipher(pvar,
		                                                pvar->crypt_state.
		                                                supported_sender_ciphers);
		pvar->crypt_state.receiver_cipher =
			choose_cipher(pvar, pvar->crypt_state.supported_receiver_ciphers);

	} else { // SSH2(yutaka)
		pvar->crypt_state.sender_cipher = pvar->ctos_cipher;
		pvar->crypt_state.receiver_cipher =pvar->stoc_cipher;

	}

	if (pvar->crypt_state.sender_cipher == SSH_CIPHER_NONE
		|| pvar->crypt_state.receiver_cipher == SSH_CIPHER_NONE) {
		UTIL_get_lang_msg("MSG_CHIPHER_NONE_ERROR", pvar,
		                  "All the encryption algorithms that this program and the server both understand have been disabled.\n"
		                  "To communicate with this server, you will have to enable some more ciphers\n"
		                  "in the TTSSH Setup dialog box when you run Tera Term again.\n"
		                  "This connection will now close.");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		return FALSE;
	} else {
		return TRUE;
	}
}

int CRYPT_get_encrypted_session_key_len(PTInstVar pvar)
{
	int server_key_bits =
		BN_num_bits(pvar->crypt_state.server_key.RSA_key->n);
	int host_key_bits = BN_num_bits(pvar->crypt_state.host_key.RSA_key->n);
	int server_key_bytes = (server_key_bits + 7) / 8;
	int host_key_bytes = (host_key_bits + 7) / 8;

	if (server_key_bits < host_key_bits) {
		return host_key_bytes;
	} else {
		return server_key_bytes;
	}
}

int CRYPT_choose_session_key(PTInstVar pvar,
                             unsigned char FAR * encrypted_key_buf)
{
	int server_key_bits =
		BN_num_bits(pvar->crypt_state.server_key.RSA_key->n);
	int host_key_bits = BN_num_bits(pvar->crypt_state.host_key.RSA_key->n);
	int server_key_bytes = (server_key_bits + 7) / 8;
	int host_key_bytes = (host_key_bits + 7) / 8;
	int encrypted_key_bytes;
	int bit_delta;

	if (server_key_bits < host_key_bits) {
		encrypted_key_bytes = host_key_bytes;
		bit_delta = host_key_bits - server_key_bits;
	} else {
		encrypted_key_bytes = server_key_bytes;
		bit_delta = server_key_bits - host_key_bits;
	}

	if (bit_delta < 128 || server_key_bits < 512 || host_key_bits < 512) {
		UTIL_get_lang_msg("MSG_RASKEY_TOOWEAK_ERROR", pvar,
		                  "Server RSA keys are too weak. A secure connection cannot be established.");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		return 0;
	} else {
		/* following Goldberg's code, I'm using MD5(servkey->n || hostkey->n || cookie)
		   for the session ID, rather than the one specified in the RFC */
		int session_buf_len = server_key_bytes + host_key_bytes + 8;
		char FAR *session_buf = (char FAR *) malloc(session_buf_len);
		char session_id[16];
		int i;

		BN_bn2bin(pvar->crypt_state.host_key.RSA_key->n, session_buf);
		BN_bn2bin(pvar->crypt_state.server_key.RSA_key->n,
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
                                          unsigned char FAR * challenge,
                                          int challenge_len,
                                          unsigned char FAR * response)
{
	int server_key_bits =
		BN_num_bits(pvar->crypt_state.server_key.RSA_key->n);
	int host_key_bits = BN_num_bits(pvar->crypt_state.host_key.RSA_key->n);
	int server_key_bytes = (server_key_bits + 7) / 8;
	int host_key_bytes = (host_key_bits + 7) / 8;
	int session_buf_len = server_key_bytes + host_key_bytes + 8;
	char FAR *session_buf = (char FAR *) malloc(session_buf_len);
	char decrypted_challenge[48];
	int decrypted_challenge_len;

	decrypted_challenge_len =
		RSA_private_decrypt(challenge_len, challenge, challenge,
		                    AUTH_get_cur_cred(pvar)->key_pair->RSA_key,
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
		memset(decrypted_challenge, 0,
		       SSH_RSA_CHALLENGE_LENGTH - decrypted_challenge_len);
		memcpy(decrypted_challenge + SSH_RSA_CHALLENGE_LENGTH -
		       decrypted_challenge_len, challenge,
		       decrypted_challenge_len);
	}

	BN_bn2bin(pvar->crypt_state.host_key.RSA_key->n, session_buf);
	BN_bn2bin(pvar->crypt_state.server_key.RSA_key->n,
	          session_buf + host_key_bytes);
	memcpy(session_buf + server_key_bytes + host_key_bytes,
	       pvar->crypt_state.server_cookie, 8);
	MD5(session_buf, session_buf_len, decrypted_challenge + 32);

	free(session_buf);

	MD5(decrypted_challenge, 48, response);

	return 1;
}

static void c3DES_init(char FAR * session_key, Cipher3DESState FAR * state)
{
	DES_set_key((const_DES_cblock FAR *) session_key, &state->k1);
	DES_set_key((const_DES_cblock FAR *) (session_key + 8), &state->k2);
	DES_set_key((const_DES_cblock FAR *) (session_key + 16), &state->k3);
	memset(state->ivec1, 0, 8);
	memset(state->ivec2, 0, 8);
	memset(state->ivec3, 0, 8);
}

static void cDES_init(char FAR * session_key, CipherDESState FAR * state)
{
	DES_set_key((const_des_cblock FAR *) session_key, &state->k);
	memset(state->ivec, 0, 8);
}

static void cIDEA_init(char FAR * session_key, CipherIDEAState FAR * state)
{
	idea_set_encrypt_key(session_key, &state->k);
	memset(state->ivec, 0, 8);
}

static void cBlowfish_init(char FAR * session_key,
                           CipherBlowfishState FAR * state)
{
	BF_set_key(&state->k, 32, session_key);
	memset(state->ivec, 0, 8);
}


//
// SSH2用アルゴリズムの初期化
//
// for SSH2(yutaka)
//
void cipher_init_SSH2(EVP_CIPHER_CTX *evp,
                      const u_char *key, u_int keylen,
                      const u_char *iv, u_int ivlen,
                      int encrypt,
                      const EVP_CIPHER *type,
                      int discard_len,
                      PTInstVar pvar)
{
	int klen;
	char tmp[80];
	unsigned char *junk = NULL, *discard = NULL;

	EVP_CIPHER_CTX_init(evp);
	if (EVP_CipherInit(evp, type, NULL, (u_char *)iv, (encrypt == CIPHER_ENCRYPT)) == 0) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar,
		                  "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 1);
		notify_fatal_error(pvar, tmp);
		return;
	}

	klen = EVP_CIPHER_CTX_key_length(evp);
	if (klen > 0 && keylen != klen) {
		if (EVP_CIPHER_CTX_set_key_length(evp, keylen) == 0) {
			UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar,
			                  "Cipher initialize error(%d)");
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 2);
			notify_fatal_error(pvar, tmp);
			return;
		}
	}
	if (EVP_CipherInit(evp, NULL, (u_char *)key, NULL, -1) == 0) {
		UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar,
		                  "Cipher initialize error(%d)");
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, pvar->ts->UIMsg, 3);
		notify_fatal_error(pvar, tmp);
		return;
	}

	if (discard_len > 0) {
		junk = malloc(discard_len);
		discard = malloc(discard_len);
		if (junk == NULL || discard == NULL ||
		    EVP_Cipher(evp, discard, junk, discard_len) == 0) {
			UTIL_get_lang_msg("MSG_CIPHER_INIT_ERROR", pvar,
			                  "Cipher initialize error(%d)");
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
			            pvar->ts->UIMsg, 3);
			notify_fatal_error(pvar, tmp);
		}
		else {
			memset(discard, 0, discard_len);
		}
		free(junk);
		free(discard);
	}
}


BOOL CRYPT_start_encryption(PTInstVar pvar, int sender_flag, int receiver_flag)
{
	char FAR *encryption_key = pvar->crypt_state.sender_cipher_key;
	char FAR *decryption_key = pvar->crypt_state.receiver_cipher_key;
	BOOL isOK = TRUE;

	if (sender_flag) {
		switch (pvar->crypt_state.sender_cipher) {
			// for SSH2(yutaka)
		case SSH2_CIPHER_3DES_CBC:
		case SSH2_CIPHER_3DES_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_OUT].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_OUT],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher),
				                 CIPHER_ENCRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.sender_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.sender_cipher),
				                 pvar);

				//debug_print(10, enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher));
				//debug_print(11, enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher));

				pvar->crypt_state.encrypt = c3DES_encrypt2;
				break;
			}

			// for SSH2(yutaka)
		case SSH2_CIPHER_AES128_CBC:
		case SSH2_CIPHER_AES192_CBC:
		case SSH2_CIPHER_AES256_CBC:
		case SSH2_CIPHER_AES128_CTR:
		case SSH2_CIPHER_AES192_CTR:
		case SSH2_CIPHER_AES256_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_OUT].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_OUT],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher),
				                 CIPHER_ENCRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.sender_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.sender_cipher),
				                 pvar);

				//debug_print(10, enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher));
				//debug_print(11, enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher));

				pvar->crypt_state.encrypt = cAES128_encrypt;
				break;
			}

		case SSH2_CIPHER_BLOWFISH_CBC:
		case SSH2_CIPHER_BLOWFISH_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_OUT].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_OUT],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher),
				                 CIPHER_ENCRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.sender_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.sender_cipher),
				                 pvar);

				//debug_print(10, enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher));
				//debug_print(11, enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher));

				pvar->crypt_state.encrypt = cBlowfish_encrypt2;
				break;
			}

		case SSH2_CIPHER_ARCFOUR:
		case SSH2_CIPHER_ARCFOUR128:
		case SSH2_CIPHER_ARCFOUR256:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_OUT].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_OUT],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher),
				                 CIPHER_ENCRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.sender_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.sender_cipher),
				                 pvar);
				//debug_print(10, enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher));
				//debug_print(11, enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher));

				pvar->crypt_state.encrypt = cArcfour_encrypt;
				break;
			}

		case SSH2_CIPHER_CAST128_CBC:
		case SSH2_CIPHER_CAST128_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_OUT].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_OUT],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher),
				                 CIPHER_ENCRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.sender_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.sender_cipher),
				                 pvar);
				//debug_print(10, enc->key, get_cipher_key_len(pvar->crypt_state.sender_cipher));
				//debug_print(11, enc->iv, get_cipher_block_size(pvar->crypt_state.sender_cipher));

				pvar->crypt_state.encrypt = cCast128_encrypt;
				break;
			}

		case SSH_CIPHER_3DES:{
				c3DES_init(encryption_key, &pvar->crypt_state.enc.c3DES);
				pvar->crypt_state.encrypt = c3DES_encrypt;
				break;
			}
		case SSH_CIPHER_IDEA:{
				cIDEA_init(encryption_key, &pvar->crypt_state.enc.cIDEA);
				pvar->crypt_state.encrypt = cIDEA_encrypt;
				break;
			}
		case SSH_CIPHER_DES:{
				cDES_init(encryption_key, &pvar->crypt_state.enc.cDES);
				pvar->crypt_state.encrypt = cDES_encrypt;
				break;
			}
		case SSH_CIPHER_RC4:{
				RC4_set_key(&pvar->crypt_state.enc.cRC4.k, 16,
							encryption_key + 16);
				pvar->crypt_state.encrypt = cRC4_encrypt;
				break;
			}
		case SSH_CIPHER_BLOWFISH:{
				cBlowfish_init(encryption_key,
				               &pvar->crypt_state.enc.cBlowfish);
				pvar->crypt_state.encrypt = cBlowfish_encrypt;
				break;
			}
		default:
			isOK = FALSE;
		}
	}


	if (receiver_flag) {
		switch (pvar->crypt_state.receiver_cipher) {
			// for SSH2(yutaka)
		case SSH2_CIPHER_3DES_CBC:
		case SSH2_CIPHER_3DES_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_IN].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_IN],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher),
				                 CIPHER_DECRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.receiver_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.receiver_cipher),
				                 pvar);

				//debug_print(12, enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher));
				//debug_print(13, enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher));

				pvar->crypt_state.decrypt = c3DES_decrypt2;
				break;
			}

			// for SSH2(yutaka)
		case SSH2_CIPHER_AES128_CBC:
		case SSH2_CIPHER_AES192_CBC:
		case SSH2_CIPHER_AES256_CBC:
		case SSH2_CIPHER_AES128_CTR:
		case SSH2_CIPHER_AES192_CTR:
		case SSH2_CIPHER_AES256_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_IN].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_IN],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher),
				                 CIPHER_DECRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.receiver_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.receiver_cipher),
				                 pvar);

				//debug_print(12, enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher));
				//debug_print(13, enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher));

				pvar->crypt_state.decrypt = cAES128_decrypt;
				break;
			}

		case SSH2_CIPHER_BLOWFISH_CBC:
		case SSH2_CIPHER_BLOWFISH_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_IN].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_IN],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher),
				                 CIPHER_DECRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.receiver_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.receiver_cipher),
				                 pvar);

				//debug_print(12, enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher));
				//debug_print(13, enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher));

				pvar->crypt_state.decrypt = cBlowfish_decrypt2;
				break;
			}

		case SSH2_CIPHER_ARCFOUR:
		case SSH2_CIPHER_ARCFOUR128:
		case SSH2_CIPHER_ARCFOUR256:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_IN].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_IN],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher),
				                 CIPHER_DECRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.receiver_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.receiver_cipher),
				                 pvar);

				//debug_print(12, enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher));
				//debug_print(13, enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher));

				pvar->crypt_state.decrypt = cArcfour_decrypt;
				break;
			}

		case SSH2_CIPHER_CAST128_CBC:
		case SSH2_CIPHER_CAST128_CTR:
			{
				struct Enc *enc;

				enc = &pvar->ssh2_keys[MODE_IN].enc;
				cipher_init_SSH2(&pvar->evpcip[MODE_IN],
				                 enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher),
				                 enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher),
				                 CIPHER_DECRYPT,
				                 get_cipher_EVP_CIPHER(pvar->crypt_state.receiver_cipher),
				                 get_cipher_discard_len(pvar->crypt_state.receiver_cipher),
				                 pvar);

				//debug_print(12, enc->key, get_cipher_key_len(pvar->crypt_state.receiver_cipher));
				//debug_print(13, enc->iv, get_cipher_block_size(pvar->crypt_state.receiver_cipher));

				pvar->crypt_state.decrypt = cCast128_decrypt;
				break;
			}

		case SSH_CIPHER_3DES:{
				c3DES_init(decryption_key, &pvar->crypt_state.dec.c3DES);
				pvar->crypt_state.decrypt = c3DES_decrypt;
				break;
			}
		case SSH_CIPHER_IDEA:{
				cIDEA_init(decryption_key, &pvar->crypt_state.dec.cIDEA);
				pvar->crypt_state.decrypt = cIDEA_decrypt;
				break;
			}
		case SSH_CIPHER_DES:{
				cDES_init(decryption_key, &pvar->crypt_state.dec.cDES);
				pvar->crypt_state.decrypt = cDES_decrypt;
				break;
			}
		case SSH_CIPHER_RC4:{
				RC4_set_key(&pvar->crypt_state.dec.cRC4.k, 16, decryption_key);
				pvar->crypt_state.decrypt = cRC4_decrypt;
				break;
			}
		case SSH_CIPHER_BLOWFISH:{
				cBlowfish_init(decryption_key,
				               &pvar->crypt_state.dec.cBlowfish);
				pvar->crypt_state.decrypt = cBlowfish_decrypt;
				break;
			}
		default:
			isOK = FALSE;
		}
	}


	if (!isOK) {
		UTIL_get_lang_msg("MSG_CHPHER_NOTSELECTED_ERROR", pvar,
		                  "No cipher selected!");
		notify_fatal_error(pvar, pvar->ts->UIMsg);
		return FALSE;
	} else {
		memset(encryption_key, 0, CRYPT_KEY_LENGTH);
		memset(decryption_key, 0, CRYPT_KEY_LENGTH);
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

static char FAR *get_cipher_name(int cipher)
{
	switch (cipher) {
	case SSH_CIPHER_NONE:
		return "None";
	case SSH_CIPHER_3DES:
		return "3DES (168 key bits)";
	case SSH_CIPHER_DES:
		return "DES (56 key bits)";
	case SSH_CIPHER_IDEA:
		return "IDEA (128 key bits)";
	case SSH_CIPHER_RC4:
		return "RC4 (128 key bits)";
	case SSH_CIPHER_BLOWFISH:
		return "Blowfish (256 key bits)";

	// SSH2 
	case SSH2_CIPHER_3DES_CBC:
		return "3DES-CBC";
	case SSH2_CIPHER_AES128_CBC:
		return "AES128-CBC";
	case SSH2_CIPHER_AES192_CBC:
		return "AES192-CBC";
	case SSH2_CIPHER_AES256_CBC:
		return "AES256-CBC";
	case SSH2_CIPHER_BLOWFISH_CBC:
		return "Blowfish-CBC";
	case SSH2_CIPHER_AES128_CTR:
		return "AES128-CTR";
	case SSH2_CIPHER_AES192_CTR:
		return "AES192-CTR";
	case SSH2_CIPHER_AES256_CTR:
		return "AES256-CTR";
	case SSH2_CIPHER_ARCFOUR:
		return "Arcfour";
	case SSH2_CIPHER_ARCFOUR128:
		return "Arcfour128";
	case SSH2_CIPHER_ARCFOUR256:
		return "Arcfour256";
	case SSH2_CIPHER_CAST128_CBC:
		return "CAST-128-CBC";
	case SSH2_CIPHER_3DES_CTR:
		return "3DES-CTR";
	case SSH2_CIPHER_BLOWFISH_CTR:
		return "Blowfish-CTR";
	case SSH2_CIPHER_CAST128_CTR:
		return "CAST-128-CTR";

	default:
		return "Unknown";
	}
}

void CRYPT_get_cipher_info(PTInstVar pvar, char FAR * dest, int len)
{
	UTIL_get_lang_msg("DLG_ABOUT_CIPHER_INFO", pvar,
	                  "%s to server, %s from server");
	_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
	          get_cipher_name(pvar->crypt_state.sender_cipher),
	          get_cipher_name(pvar->crypt_state.receiver_cipher));
}

void CRYPT_get_server_key_info(PTInstVar pvar, char FAR * dest, int len)
{
	if (SSHv1(pvar)) {
		if (pvar->crypt_state.server_key.RSA_key == NULL
		 || pvar->crypt_state.host_key.RSA_key == NULL) {
			UTIL_get_lang_msg("DLG_ABOUT_KEY_NONE", pvar, "None");
			strncpy_s(dest, len, pvar->ts->UIMsg, _TRUNCATE);
		} else {
			UTIL_get_lang_msg("DLG_ABOUT_KEY_INFO", pvar,
			                  "%d-bit server key, %d-bit host key");
			_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
			            BN_num_bits(pvar->crypt_state.server_key.RSA_key->n),
			            BN_num_bits(pvar->crypt_state.host_key.RSA_key->n));
		}
	} else { // SSH2
			UTIL_get_lang_msg("DLG_ABOUT_KEY_INFO", pvar,
			                  "%d-bit server key, %d-bit host key");
			_snprintf_s(dest, len, _TRUNCATE, pvar->ts->UIMsg,
			            pvar->server_key_bits,
			            pvar->client_key_bits);
	}
}

static void destroy_public_key(CRYPTPublicKey FAR * key)
{
	if (key->RSA_key != NULL) {
		RSA_free(key->RSA_key);
		key->RSA_key = NULL;
	}
}

void CRYPT_free_public_key(CRYPTPublicKey FAR * key)
{
	destroy_public_key(key);
	free(key);
}

void CRYPT_end(PTInstVar pvar)
{
	destroy_public_key(&pvar->crypt_state.host_key);
	destroy_public_key(&pvar->crypt_state.server_key);

	if (pvar->crypt_state.detect_attack_statics.h != NULL) {
		memset(pvar->crypt_state.detect_attack_statics.h, 0,
		       pvar->crypt_state.detect_attack_statics.n * HASH_ENTRYSIZE);
		free(pvar->crypt_state.detect_attack_statics.h);
	}

	memset(pvar->crypt_state.sender_cipher_key, 0,
	       sizeof(pvar->crypt_state.sender_cipher_key));
	memset(pvar->crypt_state.receiver_cipher_key, 0,
	       sizeof(pvar->crypt_state.receiver_cipher_key));
	memset(pvar->crypt_state.server_cookie, 0,
	       sizeof(pvar->crypt_state.server_cookie));
	memset(pvar->crypt_state.client_cookie, 0,
	       sizeof(pvar->crypt_state.client_cookie));
	memset(&pvar->crypt_state.enc, 0, sizeof(pvar->crypt_state.enc));
	memset(&pvar->crypt_state.dec, 0, sizeof(pvar->crypt_state.dec));
}

int CRYPT_passphrase_decrypt(int cipher, char FAR * passphrase,
                             char FAR * buf, int bytes)
{
	unsigned char passphrase_key[16];

	MD5(passphrase, strlen(passphrase), passphrase_key);

	switch (cipher) {
	case SSH_CIPHER_3DES:{
			Cipher3DESState state;

			DES_set_key((const_DES_cblock FAR *) passphrase_key,
			            &state.k1);
			DES_set_key((const_DES_cblock FAR *) (passphrase_key + 8),
			            &state.k2);
			DES_set_key((const_DES_cblock FAR *) passphrase_key,
			            &state.k3);
			memset(state.ivec1, 0, 8);
			memset(state.ivec2, 0, 8);
			memset(state.ivec3, 0, 8);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k3, &state.ivec3, DES_DECRYPT);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k2, &state.ivec2, DES_ENCRYPT);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k1, &state.ivec1, DES_DECRYPT);
			break;
		}

	case SSH_CIPHER_IDEA:{
			CipherIDEAState state;
			int num = 0;

			cIDEA_init(passphrase_key, &state);
			idea_cfb64_encrypt(buf, buf, bytes, &state.k, state.ivec,
			                   &num, IDEA_DECRYPT);
			break;
		}

	case SSH_CIPHER_DES:{
			CipherDESState state;

			cDES_init(passphrase_key, &state);
			DES_ncbc_encrypt(buf, buf, bytes,
			                 &state.k, &state.ivec, DES_DECRYPT);
			break;
		}

	case SSH_CIPHER_RC4:{
			CipherRC4State state;
			int num = 0;

			RC4_set_key(&state.k, 16, passphrase_key);
			RC4(&state.k, bytes, buf, buf);
			break;
		}

	case SSH_CIPHER_BLOWFISH:{
			CipherBlowfishState state;

			BF_set_key(&state.k, 16, passphrase_key);
			memset(state.ivec, 0, 8);
			flip_endianness(buf, bytes);
			BF_cbc_encrypt(buf, buf, bytes, &state.k, state.ivec,
						   BF_DECRYPT);
			flip_endianness(buf, bytes);
			break;
		}

	case SSH_CIPHER_NONE:
		break;

	default:
		memset(passphrase_key, 0, sizeof(passphrase_key));
		return 0;
	}

	memset(passphrase_key, 0, sizeof(passphrase_key));
	return 1;
}

void CRYPT_free_key_pair(CRYPTKeyPair FAR * key_pair)
{
	if (key_pair->RSA_key != NULL)
		RSA_free(key_pair->RSA_key);

	if (key_pair->DSA_key != NULL)
		DSA_free(key_pair->DSA_key);

	free(key_pair);
}
