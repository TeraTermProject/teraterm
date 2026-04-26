/*
 * Copyright (C) 2026- TeraTerm Project
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

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <wincrypt.h>
#include <bcrypt.h>

/**
 *	BCryptGenRandom() 互換実装, 存在しない Windows 用
 *
 *	@param[in]		hAlgorithm	未使用
 *	@param[in,out]	pbBuffer
 *	@param[in]		cbBuffer
 *	@param[in]		dwFlags		未使用
 *
 *	@retval	STATUS_SUCCESS				成功
 *	@retval	STATUS_INVALID_PARAMETER	pbBuffer が NULL
 *	@retval	STATUS_UNSUCCESSFUL			CryptoAPI の呼び出しに失敗
 */
extern "C" NTSTATUS WINAPI _BCryptGenRandom(
	BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer,
	ULONG dwFlags)
{
	(void)hAlgorithm;
	(void)dwFlags;
	if (pbBuffer == NULL) {
		return STATUS_INVALID_PARAMETER;
	}

	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContextA(&hProv, NULL, NULL,
							  PROV_RSA_FULL,
							  CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
		return STATUS_UNSUCCESSFUL;
	}
	BOOL ok = CryptGenRandom(hProv, cbBuffer, pbBuffer);
	CryptReleaseContext(hProv, 0);
	if (!ok) {
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}
