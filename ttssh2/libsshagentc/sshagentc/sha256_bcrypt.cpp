/*
 * Copyright (C) 2023- TeraTerm Project
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

#include <stdlib.h>
#include <windows.h>

#include "sha256.h"

static BOOL CalcHash(const void *data_ptr, size_t data_size, ALG_ID alg, void **hash_value, size_t *hash_len)
{
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	BOOL r;

	*hash_value = 0;
	*hash_len = 0;

	//DWORD dwProvType = PROV_RSA_FULL;
	DWORD dwProvType = PROV_RSA_AES;
	r = CryptAcquireContextA(&hProv, NULL, NULL, dwProvType, CRYPT_VERIFYCONTEXT);
	if (r == FALSE) {
		return FALSE;
	}

	r = CryptCreateHash(hProv, alg, 0, 0, &hHash);
	if (r == FALSE) {
	error_release:
		CryptReleaseContext(hProv, 0);
		return FALSE;
	}

	r = CryptHashData(hHash, (BYTE *)data_ptr, (DWORD)data_size, 0);
	if (r == FALSE) {
		goto error_release;
	}

	DWORD len;
	CryptGetHashParam(hHash, HP_HASHVAL, NULL, &len, 0);
	BYTE *ptr = (BYTE *)malloc(len);
	CryptGetHashParam(hHash, HP_HASHVAL, ptr, &len, 0);

	*hash_value = ptr;
	*hash_len = len;

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	return TRUE;
}

BOOL sha256(const void *data_ptr, size_t data_size, uint8_t buf[32])
{
	const size_t sha256_len = 32;
	void *hash_value;
	size_t hash_len;
	ALG_ID alg = CALG_SHA_256;
	BOOL r = CalcHash(data_ptr, data_size, alg, &hash_value, &hash_len);
	if (r == FALSE || hash_len != sha256_len) {
		memset(buf, 0, sha256_len);
		return FALSE;
	}
	memcpy(buf, hash_value, sha256_len);
	free(hash_value);
	return TRUE;
}
