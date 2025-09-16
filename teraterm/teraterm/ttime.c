/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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
/* Tera Term */
/* TERATERM.EXE, IME interface */

#include <windows.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdio.h> // for snprintf, snwprintf
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <imm.h>
#include <assert.h>
#include "asprintf.h"
#include "dllutil.h"
#include "ttime.h"

// #define ENABLE_DUMP	1

static LONG (WINAPI *PImmGetCompositionStringA)(HIMC, DWORD, LPVOID, DWORD);
static LONG (WINAPI *PImmGetCompositionStringW)(HIMC, DWORD, LPVOID, DWORD);
static HIMC (WINAPI *PImmGetContext)(HWND);
static BOOL (WINAPI *PImmReleaseContext)(HWND, HIMC);
static BOOL (WINAPI *PImmSetCompositionFontA)(HIMC, LPLOGFONTA);
static BOOL (WINAPI *PImmSetCompositionFontW)(HIMC, LPLOGFONTW);
static BOOL (WINAPI *PImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
static BOOL (WINAPI *PImmGetOpenStatus)(HIMC);
static BOOL (WINAPI *PImmSetOpenStatus)(HIMC, BOOL);

static HANDLE HIMEDLL = NULL;
static LOGFONTA IMELogFontA;
static LOGFONTW IMELogFontW;

static const APIInfo imeapi[] = {
	{ "ImmGetCompositionStringW", (void **)&PImmGetCompositionStringW },
	{ "ImmGetCompositionStringA", (void **)&PImmGetCompositionStringA },
	{ "ImmGetContext", (void **)&PImmGetContext },
	{ "ImmReleaseContext", (void **)&PImmReleaseContext },
	{ "ImmSetCompositionFontA", (void **)&PImmSetCompositionFontA },
	{ "ImmSetCompositionFontW", (void **)&PImmSetCompositionFontW },
	{ "ImmSetCompositionWindow", (void **)&PImmSetCompositionWindow },
	{ "ImmGetOpenStatus", (void **)&PImmGetOpenStatus },
	{ "ImmSetOpenStatus", (void **)&PImmSetOpenStatus },
	{ NULL, NULL },
};

BOOL LoadIME(void)
{
	HANDLE hDll;
	DWORD error;
	if (HIMEDLL != NULL) {
		// 2重初期化?
		return TRUE;
	}
	error = DLLGetApiAddressFromList(L"imm32.dll", DLL_LOAD_LIBRARY_SYSTEM, DLL_ERROR_NOT_EXIST, imeapi, &hDll);
	if (error == NO_ERROR) {
		HIMEDLL = hDll;
	}
	return error != NO_ERROR ? FALSE : TRUE;
}

void FreeIME(HWND hWnd)
{
	HANDLE HTemp;

	if (HIMEDLL == NULL)
		return;
	HTemp = HIMEDLL;
	HIMEDLL = NULL;

	/* position of conv. window -> default */
	SetConversionWindow(hWnd,-1,0);
	Sleep(1); // for safety

	DLLFreeByHandle(HTemp);
	{
		const APIInfo *p = imeapi;
		while(p->ApiName != NULL) {
			*p->func = NULL;
			p++;
		}
	}
}

BOOL CanUseIME(void)
{
  return (HIMEDLL != NULL);
}

void SetConversionWindow(HWND HWnd, int X, int Y)
{
  HIMC	hIMC;
  COMPOSITIONFORM cf;

  if (HIMEDLL == NULL) return;
// Adjust the position of conversion window
  hIMC = (*PImmGetContext)(HWnd);
  if (X>=0)
  {
	cf.dwStyle = CFS_POINT;
	cf.ptCurrentPos.x = X;
	cf.ptCurrentPos.y = Y;
  }
  else
	cf.dwStyle = CFS_DEFAULT;
  (*PImmSetCompositionWindow)(hIMC,&cf);
  (*PImmReleaseContext)(HWnd,hIMC);
}

void ResetConversionLogFont(HWND HWnd)
{
	HIMC	hIMC;
	if (HIMEDLL == NULL) return;

	hIMC = PImmGetContext(HWnd);
	if (hIMC != NULL) {
		BOOL result = FALSE;
		if (PImmSetCompositionFontW != NULL) {
			// ImmSetCompositionFontA()を使用すると
			// 未変換文字列が指定フォントで表示されないことがある
			result = PImmSetCompositionFontW(hIMC, &IMELogFontW);
		}
		if (result == FALSE) {
			// ImmSetCompositionFontW() がエラーを返してきたとき A() でリトライ
			// 9x では W()は存在するがエラーを返してくるようだ
			PImmSetCompositionFontA(hIMC, &IMELogFontA);
		}
		PImmReleaseContext(HWnd,hIMC);
	}
}

static void ConvertLogfontWA(const LOGFONTW *lfW, LOGFONTA *lfA)
{
	LOGFONTA *p = lfA;
	p->lfWeight = lfW->lfWeight;
	p->lfItalic = lfW->lfItalic;
	p->lfUnderline = lfW->lfUnderline;
	p->lfStrikeOut = lfW->lfStrikeOut;
	p->lfWidth = lfW->lfWidth;
	p->lfHeight = lfW->lfHeight;
	p->lfCharSet = lfW->lfCharSet;
	p->lfOutPrecision = lfW->lfOutPrecision;
	p->lfClipPrecision = lfW->lfClipPrecision;
	p->lfQuality = lfW->lfQuality ;
	p->lfPitchAndFamily = lfW->lfPitchAndFamily;
	BOOL use_default_char = 0;
	WideCharToMultiByte(CP_ACP, 0,
						lfW->lfFaceName, _countof(lfW->lfFaceName),
						p->lfFaceName, sizeof(p->lfFaceName),
						NULL, &use_default_char); 
}

void SetConversionLogFont(HWND HWnd, const LOGFONTW *lf)
{
	if (HIMEDLL == NULL) return;

	ConvertLogfontWA(lf, &IMELogFontA);
	if (PImmSetCompositionFontW != NULL) {
		IMELogFontW = *lf;
	}
	ResetConversionLogFont(HWnd);
}

// 内部用
static const char *GetConvStringA_i(HWND hWnd, DWORD index, size_t *len)
{
	HIMC hIMC;
	LONG size;
	char *lpstr;

	hIMC = (*PImmGetContext)(hWnd);
	if (hIMC==0)
		goto error_2;

	// Get the size of the result string.
	//		注意 ImmGetCompositionStringA() の戻り値は byte 数
	size = PImmGetCompositionStringA(hIMC, index, NULL, 0);
	if (size <= 0)
		goto error_1;

	lpstr = malloc(size + sizeof(char));
	if (lpstr == NULL)
		goto error_1;

	size = PImmGetCompositionStringA(hIMC, index, lpstr, size);
	if (size <= 0) {
		free(lpstr);
		goto error_1;
	}

	*len = size;
	lpstr[size] = 0;	// ターミネートする

	(*PImmReleaseContext)(hWnd, hIMC);
	return lpstr;

error_1:
	(*PImmReleaseContext)(hWnd, hIMC);
error_2:
	*len = 0;
	return NULL;
}

// 内部用 wchar_t版
static const wchar_t *GetConvStringW_i(HWND hWnd, DWORD index, size_t *len)
{
	HIMC hIMC;
	LONG size;
	wchar_t *lpstr;

	hIMC = (*PImmGetContext)(hWnd);
	if (hIMC==0)
		goto error_2;

	// Get the size of the result string.
	//		注意 ImmGetCompositionStringW() の戻り値は byte 数
	size = PImmGetCompositionStringW(hIMC, index, NULL, 0);
	if (size <= 0)
		goto error_1;

	lpstr = malloc(size + sizeof(wchar_t));
	if (lpstr == NULL)
		goto error_1;

	size = PImmGetCompositionStringW(hIMC, index, lpstr, size);
	if (size <= 0) {
		free(lpstr);
		goto error_1;
	}

	*len = size/2;
	lpstr[(size/2)] = 0;	// ターミネートする

	(*PImmReleaseContext)(hWnd, hIMC);
	return lpstr;

error_1:
	(*PImmReleaseContext)(hWnd, hIMC);
error_2:
	*len = 0;
	return NULL;
}

/*
 *	@param[out]		*len	wchar_t文字数('\0'は含まない)
 *	@retval			変換wchar_t文字列へのポインタ
 *					NULLの場合変換確定していない(またはエラー)
 *					文字列は使用後free()すること
 */
const wchar_t *GetConvStringW(HWND hWnd, LPARAM lParam, size_t *len)
{
	const wchar_t *lpstr;

	*len = 0;
	if (HIMEDLL==NULL) return NULL;

	if ((lParam & GCS_RESULTSTR) != 0) {
		lpstr = GetConvStringW_i(hWnd, GCS_RESULTSTR, len);
	} else {
		lpstr = NULL;
	}

	return lpstr;
}

/*
 *	@param[out]		*len	文字数('\0'は含まない)
 *	@retval			変換文字列へのポインタ
 *					NULLの場合変換確定していない(またはエラー)
 *					文字列は使用後free()すること
 */
const char *GetConvStringA(HWND hWnd, LPARAM lParam, size_t *len)
{
	const char *lpstr;

	*len = 0;
	if (HIMEDLL==NULL) return NULL;

	if ((lParam & GCS_RESULTSTR) != 0) {
		lpstr = GetConvStringA_i(hWnd, GCS_RESULTSTR, len);
	} else {
		lpstr = NULL;
	}

	return lpstr;
}

BOOL GetIMEOpenStatus(HWND hWnd)
{
	HIMC hIMC;
	BOOL stat;

	if (HIMEDLL==NULL) return FALSE;
	hIMC = (*PImmGetContext)(hWnd);
	stat = (*PImmGetOpenStatus)(hIMC);
	(*PImmReleaseContext)(hWnd, hIMC);

	return stat;
}

void SetIMEOpenStatus(HWND hWnd, BOOL stat)
{
	HIMC hIMC;

	if (HIMEDLL==NULL) return;
	hIMC = (*PImmGetContext)(hWnd);
	(*PImmSetOpenStatus)(hIMC, stat);
	(*PImmReleaseContext)(hWnd, hIMC);
}

#if defined(ENABLE_DUMP)
static void DumpReconvStringSt(RECONVERTSTRING *pReconv, BOOL unicode)
{
	if (unicode) {
		wchar_t *buf;
		aswprintf(&buf,
				  L"Str %d,%d CompStr %d,%d TargeteStr %d,%d '%s'\n",
				  pReconv->dwStrLen,
				  pReconv->dwStrOffset,
				  pReconv->dwCompStrLen,
				  pReconv->dwCompStrOffset,
				  pReconv->dwTargetStrLen,
				  pReconv->dwTargetStrOffset,
				  (wchar_t *)(((char *)pReconv) + pReconv->dwStrOffset)
			);
		_OutputDebugStringW(buf);
		free(buf);
	} else {
		char *buf;
		asprintf(&buf,
				 "Str %d,%d CompStr %d,%d TargeteStr %d,%d '%s'\n",
				 pReconv->dwStrLen,
				 pReconv->dwStrOffset,
				 pReconv->dwCompStrLen,
				 pReconv->dwCompStrOffset,
				 pReconv->dwTargetStrLen,
				 pReconv->dwTargetStrOffset,
				 (((char *)pReconv) + pReconv->dwStrOffset)
			);
		OutputDebugStringA(buf);
		free(buf);
	}
}
#endif

// IMEの前後参照変換機能への対応
// MSからちゃんと仕様が提示されていないので、アドホックにやるしかないらしい。
// cf. http://d.hatena.ne.jp/topiyama/20070703
//	   http://ice.hotmint.com/putty/#DOWNLOAD
//	   http://27213143.at.webry.info/201202/article_2.html
//	   http://webcache.googleusercontent.com/search?q=cache:WzlX3ouMscIJ:anago.2ch.net/test/read.cgi/software/1325573999/82+IMR_DOCUMENTFEED&cd=13&hl=ja&ct=clnk&gl=jp
// (2012.5.9 yutaka)
/**
 * IMEの前後参照変換機能用構造体を作成する
 *		msg == WM_IME_REQUEST,wParam == IMR_DOCUMENTFEED の応答に使う
 *		ANSIかUnicodeウィンドウによって参照文字列を変更すること
 * unicode		TRUEのときunicode
 * str_ptr		参照文字列へのポインタ
 * str_count	参照文字列の文字数(charならbytes数,wchar_tならwchar_t数)
 * cx			カーソル位置(char/wchar_t単位)
 * st_size		生成した構造体のサイズ
 * 戻り値		構造体へのポインタ
 *				不要になったら free()すること
 */
static void *CreateReconvStringSt(HWND hWnd, BOOL unicode,
								  const void *str_ptr, size_t str_count,
								  size_t cx, size_t *st_size_)
{
	static int new_str_len_bytes;
	static int new_buf_len_bytes;
	int new_str_len_count;
	int str_len_bytes;
	size_t str_len_count;
	int cx_bytes;
	RECONVERTSTRING *pReconv;
	const void *comp_ptr;
	size_t complen_count;
	size_t complen_bytes;
	const void *buf;
	size_t st_size;
	char *newbuf;

	buf = str_ptr;
	str_len_count = str_count;

	if(unicode) {
		str_len_bytes = str_len_count * sizeof(wchar_t);
		cx_bytes = cx * sizeof(wchar_t);
	} else {
		str_len_bytes = str_len_count;
		cx_bytes = cx;
	}

	// 変換中候補文字列を取得
	// ATOK2012では常に complen_count=0 となる。
	if (!unicode) {
		const char *comp_strA;
		comp_strA = GetConvStringA_i(hWnd, GCS_COMPSTR, &complen_count);
		complen_bytes = complen_count;
		comp_ptr = comp_strA;
	} else {
		const wchar_t *comp_strW;
		comp_strW = GetConvStringW_i(hWnd,GCS_COMPSTR,	&complen_count);
		complen_bytes = complen_count * sizeof(wchar_t);
		comp_ptr = comp_strW;
	}

	// 変換文字も含めた全体の長さ(including null)
	if (!unicode) {
		new_str_len_bytes = str_len_bytes + complen_bytes;
		new_buf_len_bytes = new_str_len_bytes + 1;
		new_str_len_count = new_str_len_bytes;
	} else {
		new_str_len_bytes = str_len_bytes + complen_bytes;
		new_buf_len_bytes = new_str_len_bytes + sizeof(wchar_t);
		new_str_len_count = new_str_len_bytes / sizeof(wchar_t);
	}

	st_size = sizeof(RECONVERTSTRING) + new_buf_len_bytes;
	pReconv = calloc(1, st_size);

	// Lenは文字数(char/wchar_t単位)
	// Offsetはbyte単位
//	pReconv->dwSize				= sizeof(RECONVERTSTRING);
	pReconv->dwSize				= sizeof(RECONVERTSTRING) + new_buf_len_bytes;
	pReconv->dwVersion			= 0;
	pReconv->dwStrLen			= new_str_len_count;
	pReconv->dwStrOffset		= sizeof(RECONVERTSTRING);
	pReconv->dwCompStrLen		= complen_count;
	pReconv->dwCompStrOffset	= cx_bytes;
	pReconv->dwTargetStrLen		= complen_count;		// = dwCompStrLen
	pReconv->dwTargetStrOffset	= cx_bytes;				// = dwCompStrOffset

	// RECONVERTSTRINGの後ろに
	// 参照文字列をコピー+カーソル位置に変文字列を挿入
	newbuf = (char *)pReconv + sizeof(RECONVERTSTRING);
	if (comp_ptr != NULL) {
		memcpy(newbuf, buf, cx_bytes);
		newbuf += cx_bytes;
		memcpy(newbuf, comp_ptr, complen_bytes);
		newbuf += complen_bytes;
		memcpy(newbuf, (char *)buf + cx_bytes, str_len_bytes - cx_bytes);
		free((void *)comp_ptr);
		comp_ptr = NULL;
	} else {
		memcpy(newbuf, buf, str_len_bytes);
	}
#if defined(ENABLE_DUMP)
	DumpReconvStringSt(pReconv, unicode);
#endif

	*st_size_ = st_size;
	return pReconv;
}

/**
 * IMEの前後参照変換機能用構造体を作成する
 * ANSIウィンドウ用
 *		msg == WM_IME_REQUEST,wParam == IMR_DOCUMENTFEED の応答に使う
 * str_ptr		参照文字列へのポインタ
 * str_count	参照文字列の文字数(char数,bytes数)
 * cx			カーソル位置(char単位)
 * st_size		生成した構造体のサイズ(byte)
 * 戻り値		構造体へのポインタ
 *				不要になったら free()すること
 */
void *CreateReconvStringStA(
	HWND hWnd, const char *str_ptr, size_t str_count,
	size_t cx, size_t *st_size_)
{
	if (HIMEDLL == NULL) return NULL;
	//assert(IsWindowUnicode(hWnd) == FALSE);
	return CreateReconvStringSt(hWnd, FALSE, str_ptr, str_count, cx, st_size_);
}

/**
 * IMEの前後参照変換機能用構造体を作成する
 * unicodeウィンドウ用
 *		msg == WM_IME_REQUEST,wParam == IMR_DOCUMENTFEED の応答に使う
 * str_ptr		参照文字列へのポインタ
 * str_count	参照文字列の文字数(wchar_t数)
 * cx			カーソル位置(wchar_t単位)
 * st_size		生成した構造体のサイズ(byte)
 * 戻り値		構造体へのポインタ
 *				不要になったら free()すること
 */
void *CreateReconvStringStW(
	HWND hWnd, const wchar_t *str_ptr, size_t str_count,
	size_t cx, size_t *st_size_)
{
	if (HIMEDLL == NULL) return NULL;
	//assert(IsWindowUnicode(hWnd) == TRUE);
	return CreateReconvStringSt(hWnd, TRUE, str_ptr, str_count, cx, st_size_);
}

/**
 * IMEが使えるOS?
 */
BOOL IMEEnabled(void)
{
	if (GetSystemMetrics(SM_IMMENABLED) != 0) {
		// Unicode-based IME
		return TRUE;
	}
	if (GetSystemMetrics(SM_DBCSENABLED) != 0) {
		// DBCS
#if 0
		int acp = GetACP();
		if (acp == 932 || acp == 949 || acp = 936 || acp == 950) {
			// CP932	日本語 shift jis
			// CP949	Korean
			// CP936	GB2312
			// CP950	Big5
			return TRUE;
		}
#else
		// DBCS enable なら漢字変換できるはず
		return TRUE;
#endif
	}
	return FALSE;
}
