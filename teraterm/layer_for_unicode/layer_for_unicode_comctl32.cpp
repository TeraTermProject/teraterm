/*
 * Copyright (C) 2019- TeraTerm Project
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
  * W to A Wrapper
  *
  * API名はW版の頭に '_' を付けたものを使用する
  */

#include <windows.h>
#include <assert.h>

#include "codeconv.h"
#include "compat_win.h"

#include "layer_for_unicode.h"

HPROPSHEETPAGE WINAPI _CreatePropertySheetPageW(LPCPROPSHEETPAGEW_V1 psp)
{
	char *titleA = ToCharW(psp->pszTitle);

	PROPSHEETPAGEA_V1 pspA;
	memset(&pspA, 0, sizeof(pspA));
	pspA.dwSize = sizeof(pspA);
	pspA.dwFlags = psp->dwFlags;
	pspA.hInstance = psp->hInstance;
	pspA.pResource = psp->pResource;
	pspA.pszTitle = titleA;
	pspA.pfnDlgProc = psp->pfnDlgProc;
	pspA.lParam = psp->lParam;

	HPROPSHEETPAGE retval = CreatePropertySheetPageA((LPCPROPSHEETPAGEA)&pspA);

	free(titleA);
	return retval;
}

// リリース用SDKのヘッダに
//	PROPSHEETHEADERW_V1 がないため
//	PROPSHEETHEADERW を使用
//		SDK: Windows Server 2003 R2 Platform SDK
//			 (Microsoft Windows SDK for Windows 7 and .NET Framework 3.5 SP1)
//INT_PTR _PropertySheetW(PROPSHEETHEADERW_V1 *psh)
INT_PTR WINAPI _PropertySheetW(PROPSHEETHEADERW *psh)
{
	char *captionA = ToCharW(psh->pszCaption);

//	PROPSHEETHEADERA_V1 pshA;
	PROPSHEETHEADERA pshA;
	memset(&pshA, 0, sizeof(pshA));
	pshA.dwSize = sizeof(pshA);
	pshA.dwFlags = psh->dwFlags;
	pshA.hwndParent = psh->hwndParent;
	pshA.hInstance = psh->hInstance;
	pshA.pszCaption = captionA;
	pshA.nPages = psh->nPages;
	pshA.phpage = psh->phpage;
	pshA.nStartPage = psh->nStartPage;
	pshA.pfnCallback = psh->pfnCallback;

	INT_PTR retval = PropertySheetA(&pshA);

	free(captionA);
	return retval;
}

static char *ConvertFilter(const wchar_t *filterW)
{
	if (filterW == NULL) {
		return NULL;
	}
	size_t lenW = 0;
	for(;;) {
		if (filterW[lenW] == 0 && filterW[lenW + 1] == 0) {
			lenW++;
			break;
		}
		lenW++;
	}
	lenW++;
	size_t lenA = lenW * 2;		// x2の見込み
	char *filterA = (char *)malloc(lenA);
	::WideCharToMultiByte(CP_ACP, 0, filterW, (int)lenW, filterA, (int)lenA, NULL, NULL);
	return filterA;
}

static BOOL GetOpenSaveFileNameA(BOOL (WINAPI *fn)(LPOPENFILENAMEA ofnA), LPOPENFILENAMEW ofnW)
{
	char fileA[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, ofnW->lpstrFile, -1, fileA, _countof(fileA), NULL,NULL);

	OPENFILENAMEA ofnA;
	memset(&ofnA, 0, sizeof(ofnA));
	ofnA.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
	ofnA.hwndOwner = ofnW->hwndOwner;
	ofnA.lpstrFilter = ConvertFilter(ofnW->lpstrFilter);
	ofnA.nFilterIndex = ofnW->nFilterIndex;
	ofnA.lpstrFile = fileA;
	ofnA.nMaxFile = _countof(fileA);
	ofnA.lpstrTitle = ToCharW(ofnW->lpstrTitle);
	ofnA.Flags = ofnW->Flags;
	ofnA.lCustData = ofnW->lCustData;
	ofnA.lpfnHook = ofnW->lpfnHook;
	ofnA.lpTemplateName = (LPCSTR)ofnW->lpTemplateName;
	ofnA.hInstance = ofnW->hInstance;
	BOOL result = fn(&ofnA);
	if (result) {
		MultiByteToWideChar(CP_ACP, 0, fileA, _countof(fileA), ofnW->lpstrFile, ofnW->nMaxFile);
	}
#if _DEBUG
	else {
		DWORD err = CommDlgExtendedError();
		assert(err == 0);
	}
#endif
	free((void *)ofnA.lpstrFilter);
	free((void *)ofnA.lpstrTitle);
	return result;
}

BOOL WINAPI _GetOpenFileNameW(LPOPENFILENAMEW ofnW)
{
	return GetOpenSaveFileNameA(GetOpenFileNameA, ofnW);
}

BOOL WINAPI _GetSaveFileNameW(LPOPENFILENAMEW ofnW)
{
	return GetOpenSaveFileNameA(GetSaveFileNameA, ofnW);
}
