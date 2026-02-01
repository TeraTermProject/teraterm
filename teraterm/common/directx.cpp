/*
 * Copyright (C) 2025- TeraTerm Project
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

#include <dwrite.h>
#include <assert.h>

#include "compat_dwrite.h"

#include "directx.h"

static IDWriteFactory* pDWriteFactory = NULL;

/**
 *	Direct Xの初期化
 */
BOOL DXInit(void)
{
	if (pDWriteCreateFactory == NULL) {
		// Direct Write が使えない
		// Windows 7未満
		return FALSE;
	}
	assert(pDWriteFactory == NULL);

	HRESULT hr = pDWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&pDWriteFactory));
	if (SUCCEEDED(hr)) {
		return TRUE;
	}
	pDWriteFactory = NULL;
	return FALSE;
}

/**
 *	Direct Xの終了
 */
void DXUninit(void)
{
	if (pDWriteFactory != NULL) {
		pDWriteFactory->Release();
		pDWriteFactory = NULL;
	}
}

/**
 *	logfontからfont family名を取得する
 *
 *	@param	logfont
 *	@param	font_family		フォントファミリ名、不要になったら free() すること
 *	@retval	FALSE			取得失敗/Windows 7未満/未初期化
 */
BOOL DXGetFontFamilyName(const LOGFONTW *logfont, wchar_t **font_family)
{
	if (pDWriteFactory == NULL) {
		return FALSE;
	}

	assert(logfont != NULL);
	assert(font_family != NULL);

	BOOL result = FALSE;
	IDWriteGdiInterop *pGdiInterop = NULL;
	IDWriteFont *pWriteFont = NULL;
	IDWriteFontFamily *pFontFamily = NULL;
	IDWriteLocalizedStrings *pFamilyName = NULL;
	*font_family = NULL;

	do {
		HRESULT hr = pDWriteFactory->GetGdiInterop(&pGdiInterop);
		if (!SUCCEEDED(hr)) break;

		hr = pGdiInterop->CreateFontFromLOGFONT(logfont, &pWriteFont);
		if (!SUCCEEDED(hr)) break;

		hr = pWriteFont->GetFontFamily(&pFontFamily);
		if (!SUCCEEDED(hr)) break;

		hr = pFontFamily->GetFamilyNames(&pFamilyName);
		if (!SUCCEEDED(hr)) break;

		// FontFamily名を取得
		BOOL exists = FALSE;
		UINT32 index = 0;
		hr = pFamilyName->FindLocaleName(L"en-us", &index, &exists);
		if (!SUCCEEDED(hr) || !exists) break;
		UINT32 length = 0;
		hr = pFamilyName->GetStringLength(index, &length);
		if (!SUCCEEDED(hr)) break;
		length += 1;	// add 1 for null terminator.
		wchar_t *name = (wchar_t *)malloc(sizeof(wchar_t) * length);
		if (name == NULL) break;
		hr = pFamilyName->GetString(index, name, length);
		if (!SUCCEEDED(hr)) {
			free(name);
			break;
		}
		*font_family = name;

		result = TRUE;
	} while(0);

	if (pFamilyName) pFamilyName->Release();
	if (pFontFamily) pFontFamily->Release();
	if (pWriteFont) pWriteFont->Release();
	if (pGdiInterop) pGdiInterop->Release();

	return result;
}
