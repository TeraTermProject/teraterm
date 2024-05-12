/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2009- TeraTerm Project
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

/* TTMACRO.EXE, password encryption */

#include "teraterm.h"
#include <stdlib.h>
#include <string.h>

#define LIBRESSL_DISABLE_OVERRIDE_WINCRYPT_DEFINES_WARNING

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "ttmdef.h"

#define PWD_MAX_LEN 300
#define PWD_CIPHER EVP_aes_256_ctr()
#define ENC_IN_LEN 508
#define PBKDF2_DIGEST EVP_sha256()
#define PBKDF2_ITER_DEFAULT 10000
#define PBKDF2_IKLEN 32
#define PBKDF2_IVLEN 16

static int EncDecPWD(PCHAR InStr, PCHAR OutStr, const char *EncryptStr, int encode)
{
	static const char magic[] = "Salted__";
	int magic_len = sizeof(magic) - 1;
	unsigned char salt[PKCS5_SALT_LEN];
	BIO *b64 = NULL, *mem = NULL, *bio = NULL, *benc = NULL;
	int in_len;
	int ret = -1;
	
	if ((b64 = BIO_new(BIO_f_base64())) == NULL ||
		(mem = BIO_new(BIO_s_mem())) == NULL ||
		(bio = BIO_push(b64, mem)) == NULL) {
		goto end;
	}
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	in_len = strlen(InStr);
	
	if (encode) {
		if (in_len > PWD_MAX_LEN ||
			BIO_write(bio, magic, magic_len) != magic_len ||
			RAND_bytes(salt, PKCS5_SALT_LEN) <= 0 ||
			BIO_write(bio, (char *)salt, PKCS5_SALT_LEN) != PKCS5_SALT_LEN) {
			goto end;
		}
	} else {
		char mbuf[sizeof(magic) - 1];
		if (in_len != ENC_IN_LEN ||
			BIO_write(mem, InStr, in_len) != in_len ||
			BIO_flush(mem) != 1 ||
			BIO_read(bio, mbuf, magic_len) != magic_len ||
			memcmp(mbuf, magic, magic_len) != 0 ||
			BIO_read(bio, salt, PKCS5_SALT_LEN) != PKCS5_SALT_LEN) {
			goto end;
		}
	}
	
	unsigned char tmpkeyiv[EVP_MAX_KEY_LENGTH + EVP_MAX_IV_LENGTH];
	unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
	int iklen = PBKDF2_IKLEN;
	int ivlen = PBKDF2_IVLEN;
	if (PKCS5_PBKDF2_HMAC(EncryptStr, strlen(EncryptStr),
						  (const unsigned char *)&salt, PKCS5_SALT_LEN,
						  PBKDF2_ITER_DEFAULT, (EVP_MD *)PBKDF2_DIGEST,
						  iklen + ivlen, tmpkeyiv) != 1) {
		goto end;
	}
	memcpy(key, tmpkeyiv, iklen);
	memcpy(iv, tmpkeyiv + iklen, ivlen);
	
	EVP_CIPHER_CTX *ctx;
	if ((benc = BIO_new(BIO_f_cipher())) == NULL ||
		BIO_get_cipher_ctx(benc, &ctx) != 1 ||
		EVP_CipherInit_ex(ctx, PWD_CIPHER, NULL, key, iv, encode) != 1 ||
		(bio = BIO_push(benc, bio))  == NULL) {
		goto end;
	}

	unsigned char hash[MaxStrLen + PKCS5_SALT_LEN];
	unsigned char hbuf[SHA512_DIGEST_LENGTH];
	int enc_len;
	enc_len = strlen(EncryptStr);
	memcpy(hash, EncryptStr, enc_len);
	memcpy(hash + enc_len, salt, PKCS5_SALT_LEN);
	SHA512(hash, enc_len + PKCS5_SALT_LEN, hbuf);
	
	if (encode) {
		unsigned char tmpbuf[MaxStrLen];
		int len;
		memcpy(tmpbuf, InStr, in_len);
		memset(tmpbuf + in_len, 0x00, PWD_MAX_LEN - in_len);
		tmpbuf[PWD_MAX_LEN] = 0;
		if (BIO_write(bio, tmpbuf, PWD_MAX_LEN + 1) != PWD_MAX_LEN + 1 ||
			BIO_write(bio, hbuf, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH ||
			BIO_flush(bio) != 1 ||
			(len = BIO_read(mem, OutStr, MaxStrLen)) <= 0) {
			goto end;
		}
		OutStr[len] = 0;
		ret = 0;
	} else {
		char tmpbuf[MaxStrLen];
		unsigned char hbuf_in[SHA512_DIGEST_LENGTH];
		if (BIO_read(bio, tmpbuf, PWD_MAX_LEN + 1) != PWD_MAX_LEN + 1 ||
			BIO_read(bio, hbuf_in, SHA512_DIGEST_LENGTH) != SHA512_DIGEST_LENGTH ||
			memcmp(hbuf, hbuf_in, SHA512_DIGEST_LENGTH) != 0) {
			goto end;
		}
		memcpy(OutStr, tmpbuf, PWD_MAX_LEN + 1);
		ret = 0;
	}

 end:
	if (ret != 0) {
		OutStr[0] = 0;
	}
	BIO_free(benc);
	BIO_free(mem);
	BIO_free(b64);

	return ret;
}

BOOL EncSeparate(const char *Str, int *i, LPBYTE b)
{
	int cptr, bptr;
	unsigned int d;

	cptr = *i / 8;
	if (Str[cptr]==0) {
		return FALSE;
	}
	bptr = *i % 8;
	d = ((BYTE)Str[cptr] << 8) |
		 (BYTE)Str[cptr+1];
	*b = (BYTE)((d >> (10-bptr)) & 0x3f);

	*i = *i + 6;
	return TRUE;
}

BYTE EncCharacterize(BYTE c, LPBYTE b)
{
	BYTE d;

	d = c + *b;
	if (d > (BYTE)0x7e) {
		d = d - (BYTE)0x5e;
	}
	if (*b<0x30) {
		*b = 0x30;
	}
	else if (*b<0x40) {
		*b = 0x40;
	}
	else if (*b<0x50) {
		*b = 0x50;
	}
	else if (*b<0x60) {
		*b = 0x60;
	}
	else if (*b<0x70) {
		*b = 0x70;
	}
	else {
		*b = 0x21;
	}

	return d;
}

int Encrypt(const char *InStr, PCHAR OutStr, PCHAR EncryptStr)
{

	int i, j;
	BYTE b, r, r2;

	OutStr[0] = 0;
	if (InStr[0]==0) {
		return 0;
	}

	if (EncryptStr[0] != 0) {
		if (EncDecPWD((PCHAR)InStr, OutStr, EncryptStr, 1) != 0) {
			return -1;
		}
	} else {
		srand(LOWORD(GetTickCount()));
		r = (BYTE)(rand() & 0x3f);
		r2 = (~r) & 0x3f;
		OutStr[0] = r;
		i = 0;
		j = 1;
		while (EncSeparate(InStr,&i,&b)) {
			r = (BYTE)(rand() & 0x3f);
			OutStr[j++] = (b + r) & 0x3f;
			OutStr[j++] = r;
		}
		OutStr[j++] = r2;
		OutStr[j] = 0;
		i = 0;
		b = 0x21;
		while (i < j) {
			OutStr[i] = EncCharacterize(OutStr[i],&b);
			i++;
		}
	}

	return 0;
}

void DecCombine(PCHAR Str, int *i, BYTE b)
{
	int cptr, bptr;
	unsigned int d;

	cptr = *i / 8;
	bptr = *i % 8;
	if (bptr==0) {
		Str[cptr] = 0;
	}

	d = ((BYTE)Str[cptr] << 8) |
	    (b << (10 - bptr));

	Str[cptr] = (BYTE)(d >> 8);
	Str[cptr+1] = (BYTE)(d & 0xff);
	*i = *i + 6;
	return;
}

BYTE DecCharacter(BYTE c, LPBYTE b)
{
	BYTE d;

	if (c < *b) {
		d = (BYTE)0x5e + c - *b;
	}
	else {
		d = c - *b;
	}
	d = d & 0x3f;

	if (*b<0x30) {
		*b = 0x30;
	}
	else if (*b<0x40) {
		*b = 0x40;
	}
	else if (*b<0x50) {
		*b = 0x50;
	}
	else if (*b<0x60) {
		*b = 0x60;
	}
	else if (*b<0x70) {
		*b = 0x70;
	}
	else {
		*b = 0x21;
	}

	return d;
}

int Decrypt(PCHAR InStr, PCHAR OutStr, PCHAR EncryptStr)
{
	int i, j, k;
	BYTE b;
	char Temp[512];

	OutStr[0] = 0;
	j = strlen(InStr);
	if (j==0) {
		return 0;
	}

	if (EncryptStr[0] != 0) {
		if (EncDecPWD((PCHAR)InStr, OutStr, EncryptStr, 0) != 0) {
			return -1;
		}
	} else {
		b = 0x21;
		for (i=0 ; i < j ; i++) {
			Temp[i] = DecCharacter(InStr[i],&b);
		}
		if ((Temp[0] ^ Temp[j-1]) != (BYTE)0x3f) {
			return 0;
		}
		i = 1;
		k = 0;
		while (i < j-2) {
			Temp[i] = ((BYTE)Temp[i] - (BYTE)Temp[i+1]) & (BYTE)0x3f;
			DecCombine(OutStr,&k,Temp[i]);
			i = i + 2;
		}
	}

	return 0;
}
