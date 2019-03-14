/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2019 TeraTerm Project
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

#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <imm.h>

#include "ttime.h"
#include "ttlib.h"

#if 1
#include "teraterm.h"
#include "tttypes.h"
#include "ttwinman.h"
#include "ttcommon.h"
#include "buffer.h"		// for BuffGetCurrentLineData()
#endif

#ifndef _IMM_
  #define _IMM_

  typedef DWORD HIMC;

  typedef struct tagCOMPOSITIONFORM {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
  } COMPOSITIONFORM, *PCOMPOSITIONFORM, NEAR *NPCOMPOSITIONFORM, *LPCOMPOSITIONFORM;
#endif //_IMM_

#define GCS_RESULTSTR 0x0800

typedef LONG (WINAPI *TImmGetCompositionStringA)(HIMC, DWORD, LPVOID, DWORD);
typedef LONG (WINAPI *TImmGetCompositionStringW)(HIMC, DWORD, LPVOID, DWORD);
typedef HIMC (WINAPI *TImmGetContext)(HWND);
typedef BOOL (WINAPI *TImmReleaseContext)(HWND, HIMC);
typedef BOOL (WINAPI *TImmSetCompositionFont)(HIMC, LPLOGFONTA);
typedef BOOL (WINAPI *TImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
typedef BOOL (WINAPI *TImmGetOpenStatus)(HIMC);
typedef BOOL (WINAPI *TImmSetOpenStatus)(HIMC, BOOL);

static TImmGetCompositionStringW PImmGetCompositionStringW;
static TImmGetCompositionStringA PImmGetCompositionStringA;
static TImmGetContext PImmGetContext;
static TImmReleaseContext PImmReleaseContext;
static TImmSetCompositionFont PImmSetCompositionFont;
static TImmSetCompositionWindow PImmSetCompositionWindow;
static TImmGetOpenStatus PImmGetOpenStatus;
static TImmSetOpenStatus PImmSetOpenStatus;


static HANDLE HIMEDLL = NULL;
static LOGFONTA lfIME;

#if 1
static void show_message()
{
  char uimsg[MAX_UIMSG];
    get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg),  "Tera Term: Error", ts.UILanguageFile);
    get_lang_msg("MSG_USE_IME_ERROR", ts.UIMsg, sizeof(ts.UIMsg), "Can't use IME", ts.UILanguageFile);
    MessageBoxA(0,ts.UIMsg,uimsg,MB_ICONEXCLAMATION);
    WritePrivateProfileStringA("Tera Term","IME","off",ts.SetupFName);
    ts.UseIME = 0;
#if 0
    tempts = (PTTSet)malloc(sizeof(TTTSet));
    if (tempts!=NULL)
    {
		GetDefaultSet(tempts);
		tempts->UseIME = 0;
		ChangeDefaultSet(tempts,NULL);
		free(tempts);
    }
#endif
}
#endif

BOOL LoadIME()
{
  BOOL Err;
#if 0
  PTTSet tempts;
#endif
  char imm32_dll[MAX_PATH];

  if (HIMEDLL != NULL) return TRUE;
  GetSystemDirectoryA(imm32_dll, sizeof(imm32_dll));
  strncat_s(imm32_dll, sizeof(imm32_dll), "\\imm32.dll", _TRUNCATE);
  HIMEDLL = LoadLibraryA(imm32_dll);
  if (HIMEDLL == NULL)
  {
	  show_message();
    return FALSE;
  }

  Err = FALSE;

  PImmGetCompositionStringW = (TImmGetCompositionStringW)GetProcAddress(
    HIMEDLL, "ImmGetCompositionStringW");
  if (PImmGetCompositionStringW==NULL) Err = TRUE;

  PImmGetCompositionStringA = (TImmGetCompositionStringA)GetProcAddress(
    HIMEDLL, "ImmGetCompositionStringA");
  if (PImmGetCompositionStringA==NULL) Err = TRUE;

  PImmGetContext = (TImmGetContext)GetProcAddress(
    HIMEDLL, "ImmGetContext");
  if (PImmGetContext==NULL) Err = TRUE;

  PImmReleaseContext = (TImmReleaseContext)GetProcAddress(
    HIMEDLL, "ImmReleaseContext");
  if (PImmReleaseContext==NULL) Err = TRUE;

  PImmSetCompositionFont = (TImmSetCompositionFont)GetProcAddress(
    HIMEDLL, "ImmSetCompositionFontA");
  if (PImmSetCompositionFont==NULL) Err = TRUE;

  PImmSetCompositionWindow = (TImmSetCompositionWindow)GetProcAddress(
    HIMEDLL, "ImmSetCompositionWindow");
  if (PImmSetCompositionWindow==NULL) Err = TRUE;

  PImmGetOpenStatus = (TImmGetOpenStatus)GetProcAddress(
    HIMEDLL, "ImmGetOpenStatus");
  if (PImmGetOpenStatus==NULL) Err = TRUE;

  PImmSetOpenStatus = (TImmSetOpenStatus)GetProcAddress(
    HIMEDLL, "ImmSetOpenStatus");
  if (PImmSetOpenStatus==NULL) Err = TRUE;

  if ( Err )
  {
    FreeLibrary(HIMEDLL);
    HIMEDLL = NULL;
    return FALSE;
  }
  else
    return TRUE;
}

void FreeIME()
{
  HANDLE HTemp;

  if (HIMEDLL==NULL) return;
  HTemp = HIMEDLL;
  HIMEDLL = NULL;

  /* position of conv. window -> default */
  SetConversionWindow(HVTWin,-1,0);
  Sleep(1); // for safety
  FreeLibrary(HTemp);
}

BOOL CanUseIME()
{
  return (HIMEDLL != NULL);
}

void SetConversionWindow(HWND HWin, int X, int Y)
{
  HIMC	hIMC;
  COMPOSITIONFORM cf;

  if (HIMEDLL == NULL) return;
// Adjust the position of conversion window
  hIMC = (*PImmGetContext)(HVTWin);
  if (X>=0)
  {
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = X;
    cf.ptCurrentPos.y = Y;
  }
  else
    cf.dwStyle = CFS_DEFAULT;
  (*PImmSetCompositionWindow)(hIMC,&cf);
  (*PImmReleaseContext)(HVTWin,hIMC);
}

void SetConversionLogFont(HWND HWin, PLOGFONT lf)
{
  HIMC	hIMC;
  if (HIMEDLL == NULL) return;

  memcpy(&lfIME,lf,sizeof(LOGFONT));

  hIMC = (*PImmGetContext)(HVTWin);
  // Set font for the conversion window
  (*PImmSetCompositionFont)(hIMC,&lfIME);
  (*PImmReleaseContext)(HVTWin,hIMC);
}

/*
 *	@param[in,out]	*len	wchar_t文字数
 *	@reterun		変換wchar_t文字列へのポインタ
 *					NULLの場合変換確定していない(またはエラー)
 *					文字列は使用後free()すること
 */
const wchar_t *GetConvString(HWND hWnd, UINT wParam, LPARAM lParam, size_t *len)
{
	wchar_t *lpstr;

	*len = 0;
	if (HIMEDLL==NULL) return NULL;

	if ((lParam & GCS_RESULTSTR) != 0) {
		HIMC hIMC;
		LONG size;

		hIMC = (*PImmGetContext)(hWnd);
		if (hIMC==0) return NULL;

		// Get the size of the result string.
		//		注意 ImmGetCompositionStringW() の戻り値は byte 数
		size = PImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
		if (size <= 0) {
			lpstr = NULL;		// エラー
		} else {
			lpstr = malloc(size + sizeof(WCHAR));
			if (lpstr != NULL)
			{
				size = PImmGetCompositionStringW(hIMC, GCS_RESULTSTR, lpstr, size);
				if (size <= 0) {
					free(lpstr);
					lpstr = NULL;
				} else {
					*len = size/2;
					lpstr[(size/2)] = 0;	// ターミネートする
				}
			}
		}

		(*PImmReleaseContext)(hWnd, hIMC);

	} else {
		lpstr = NULL;
	}

	return lpstr;
}

BOOL GetIMEOpenStatus()
{
	HIMC hIMC;
	BOOL stat;

	if (HIMEDLL==NULL) return FALSE;
	hIMC = (*PImmGetContext)(HVTWin);
	stat = (*PImmGetOpenStatus)(hIMC);
	(*PImmReleaseContext)(HVTWin, hIMC);

	return stat;

}

void SetIMEOpenStatus(BOOL stat) {
	HIMC hIMC;

	if (HIMEDLL==NULL) return;
	hIMC = (*PImmGetContext)(HVTWin);
	(*PImmSetOpenStatus)(hIMC, stat);
	(*PImmReleaseContext)(HVTWin, hIMC);
}

// IMEの前後参照変換機能への対応
// MSからちゃんと仕様が提示されていないので、アドホックにやるしかないらしい。
// cf. http://d.hatena.ne.jp/topiyama/20070703
//     http://ice.hotmint.com/putty/#DOWNLOAD
//     http://27213143.at.webry.info/201202/article_2.html
//     http://webcache.googleusercontent.com/search?q=cache:WzlX3ouMscIJ:anago.2ch.net/test/read.cgi/software/1325573999/82+IMR_DOCUMENTFEED&cd=13&hl=ja&ct=clnk&gl=jp
// (2012.5.9 yutaka)
LRESULT ReplyIMERequestDocumentfeed(LPARAM lParam, int NumOfColumns)
{
	static int complen, newsize;
	static char comp[512];
	int size, ret;
	char buf[512], newbuf[1024];
	HIMC hIMC;

	// "IME=off"の場合は、何もしない。
	size = NumOfColumns + 1;   // カーソルがある行の長さ+null

	if (lParam == 0) {  // 1回目の呼び出し
		// バッファのサイズを返すのみ。
		// ATOK2012では常に complen=0 となる。
		complen = 0;
		memset(comp, 0, sizeof(comp));
		hIMC = PImmGetContext(HVTWin);
		if (hIMC) {
			ret = PImmGetCompositionStringA(hIMC, GCS_COMPSTR, comp, sizeof(comp));
			if (ret == IMM_ERROR_NODATA || ret == IMM_ERROR_GENERAL) {
				memset(comp, 0, sizeof(comp));
			}
			complen = strlen(comp);  // w/o null
			PImmReleaseContext(HVTWin, hIMC);
		}
		newsize = size + complen;  // 変換文字も含めた全体の長さ(including null)

	} else {  // 2回目の呼び出し
		//lParam を RECONVERTSTRING と 文字列格納バッファに使用する
		RECONVERTSTRING *pReconv   = (RECONVERTSTRING*)lParam;
		char*  pszParagraph        = (char*)pReconv + sizeof(RECONVERTSTRING);
		int cx;

		cx = BuffGetCurrentLineData(buf, sizeof(buf));

		// カーソル位置に変換文字列を挿入する。
		memset(newbuf, 0, sizeof(newbuf));
		memcpy(newbuf, buf, cx);
		memcpy(newbuf + cx, comp, complen);
		memcpy(newbuf + cx + complen, buf + cx, size - cx);
		newsize = size + complen;  // 変換文字も含めた全体の長さ(including null)

		pReconv->dwSize            = sizeof(RECONVERTSTRING);
		pReconv->dwVersion         = 0;
		pReconv->dwStrLen          = newsize - 1;
		pReconv->dwStrOffset       = sizeof(RECONVERTSTRING);
		pReconv->dwCompStrLen      = complen;
		pReconv->dwCompStrOffset   = cx;
		pReconv->dwTargetStrLen    = complen;
		pReconv->dwTargetStrOffset = cx;

		memcpy(pszParagraph, newbuf, newsize);
		//OutputDebugPrintf("cx %d buf [%d:%s] -> [%d:%s]\n", cx, size, buf, newsize, newbuf);
	}
	return (sizeof(RECONVERTSTRING) + newsize);
}
