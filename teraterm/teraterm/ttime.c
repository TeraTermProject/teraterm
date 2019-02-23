/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
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

#include "teraterm.h"
#include "tttypes.h"
#include <stdlib.h>
#include <string.h>
#include <imm.h>

#include "ttwinman.h"
#include "ttcommon.h"

#include "ttime.h"
#include "ttlib.h"

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

typedef LONG (WINAPI *TImmGetCompositionString)
	(HIMC, DWORD, LPVOID, DWORD);
typedef HIMC (WINAPI *TImmGetContext)(HWND);
typedef BOOL (WINAPI *TImmReleaseContext)(HWND, HIMC);
typedef BOOL (WINAPI *TImmSetCompositionFont)(HIMC, LPLOGFONTA);
typedef BOOL (WINAPI *TImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
typedef BOOL (WINAPI *TImmGetOpenStatus)(HIMC);
typedef BOOL (WINAPI *TImmSetOpenStatus)(HIMC, BOOL);

static TImmGetCompositionString PImmGetCompositionString;
static TImmGetContext PImmGetContext;
static TImmReleaseContext PImmReleaseContext;
static TImmSetCompositionFont PImmSetCompositionFont;
static TImmSetCompositionWindow PImmSetCompositionWindow;
static TImmGetOpenStatus PImmGetOpenStatus;
static TImmSetOpenStatus PImmSetOpenStatus;


static HANDLE HIMEDLL = NULL;
static LOGFONT lfIME;


BOOL LoadIME()
{
  BOOL Err;
#if 0
  PTTSet tempts;
#endif
  char uimsg[MAX_UIMSG];
  char imm32_dll[MAX_PATH];

  if (HIMEDLL != NULL) return TRUE;
  GetSystemDirectory(imm32_dll, sizeof(imm32_dll));
  strncat_s(imm32_dll, sizeof(imm32_dll), "\\imm32.dll", _TRUNCATE);
  HIMEDLL = LoadLibrary(imm32_dll);
  if (HIMEDLL == NULL)
  {
    get_lang_msg("MSG_TT_ERROR", uimsg, sizeof(uimsg),  "Tera Term: Error", ts.UILanguageFile);
    get_lang_msg("MSG_USE_IME_ERROR", ts.UIMsg, sizeof(ts.UIMsg), "Can't use IME", ts.UILanguageFile);
    MessageBox(0,ts.UIMsg,uimsg,MB_ICONEXCLAMATION);
    WritePrivateProfileString("Tera Term","IME","off",ts.SetupFName);
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
    return FALSE;
  }

  Err = FALSE;

  PImmGetCompositionString = (TImmGetCompositionString)GetProcAddress(
    HIMEDLL, "ImmGetCompositionStringA");
  if (PImmGetCompositionString==NULL) Err = TRUE;

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

  // Set font for the conversion window
  (*PImmSetCompositionFont)(hIMC,&lfIME);
  (*PImmReleaseContext)(HVTWin,hIMC);
}

void SetConversionLogFont(PLOGFONT lf)
{
  memcpy(&lfIME,lf,sizeof(LOGFONT));
}

HGLOBAL GetConvString(UINT wParam, LPARAM lParam)
{
	HIMC hIMC;
	HGLOBAL hstr = NULL;
	//LPSTR lpstr;
	wchar_t *lpstr;
	DWORD dwSize;

	if (HIMEDLL==NULL) return NULL;
	hIMC = (*PImmGetContext)(HVTWin);
	if (hIMC==0) return NULL;

	if ((lParam & GCS_RESULTSTR)==0)
		goto skip;

	// Get the size of the result string.
	//dwSize = (*PImmGetCompositionString)(hIMC, GCS_RESULTSTR, NULL, 0);
	dwSize = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
	dwSize += sizeof(WCHAR);
	hstr = GlobalAlloc(GHND,dwSize);
	if (hstr != NULL)
	{
//		lpstr = (LPSTR)GlobalLock(hstr);
		lpstr = GlobalLock(hstr);
		if (lpstr != NULL)
		{
#if 0
			// Get the result strings that is generated by IME into lpstr.
			(*PImmGetCompositionString)
				(hIMC, GCS_RESULTSTR, lpstr, dwSize);
#else
			ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, lpstr, dwSize);
#endif
			GlobalUnlock(hstr);
		}
		else {
			GlobalFree(hstr);
			hstr = NULL;
		}
	}

skip:
	(*PImmReleaseContext)(HVTWin, hIMC);
	return hstr;
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
