/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2018 TeraTerm Project
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

#pragma once

#include <windows.h>
#include "i18n.h"

 /* Routines for dialog boxes */
#ifdef __cplusplus
extern "C" {
#endif

void EnableDlgItem(HWND HDlg, int FirstId, int LastId);
void DisableDlgItem(HWND HDlg, int FirstId, int LastId);
void ShowDlgItem(HWND HDlg, int FirstId, int LastId);
void SetRB(HWND HDlg, int R, int FirstId, int LastId);
void GetRB(HWND HDlg, LPWORD R, int FirstId, int LastId);
void SetDlgNum(HWND HDlg, int id_Item, LONG Num);
void SetDlgPercent(HWND HDlg, int id_Item, int id_Progress, LONG a, LONG b, int *prog);
void SetDlgTime(HWND HDlg, int id_Item, DWORD elapsed, int bytes);
void SetDropDownList(HWND HDlg, int Id_Item, const char *List[], int nsel);
void SetDropDownListW(HWND HDlg, int Id_Item, const wchar_t *List[], int nsel);
LONG GetCurSel(HWND HDlg, int Id_Item);
void InitDlgProgress(HWND HDlg, int id_Progress, int *CurProgStat);
void SetEditboxSubclass(HWND hDlg, int nID, BOOL ComboBox);

#if defined(_UNICODE)
#define SetDropDownListT(p1, p2, p3, p4)	SetDropDownListW(p1, p2, p3, p4)
#else
#define SetDropDownListT(p1, p2, p3, p4)	SetDropDownList(p1, p2, p3, p4)
#endif

void TTSetDlgFontA(const char *face, int height, int charset);
void TTSetDlgFontW(const wchar_t *face, int height, int charset);
const wchar_t *TTGetClassName(const DLGTEMPLATE *DlgTempl);
DLGTEMPLATE *TTGetDlgTemplate(HINSTANCE hInst, LPCTSTR lpTemplateName);
DLGTEMPLATE *TTGetNewDlgTemplate(
	HINSTANCE hInst, const DLGTEMPLATE *src,
	size_t *PrevTemplSize, size_t *NewTemplSize);
BOOL TTEndDialog(HWND hDlgWnd, INT_PTR nResult);
HWND TTCreateDialogIndirectParam(
	HINSTANCE hInstance,
	LPCDLGTEMPLATE lpTemplate,
	HWND hWndParent,
	DLGPROC lpDialogFunc,
	LPARAM lParamInit);
HWND TTCreateDialog(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc);
INT_PTR TTDialogBoxParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc,
	LPARAM lParamInit);
INT_PTR TTDialogBox(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc);
void SetDialogFont(const char *SetupFName, const char *UILanguageFile, const char *Section);
HFONT SetDlgFonts(HWND hDlg, const int nIDDlgItems[], int nIDDlgItemCount,
				  const char *UILanguageFile, PCHAR key);
BOOL IsExistFontA(const char *face, BYTE charset, BOOL strict);
int GetFontPointFromPixel(HWND hWnd, int pixel);
int GetFontPixelFromPoint(HWND hWnd, int point);

#if defined(_UNICODE)
#define TTSetDlgFont(p1,p2,p3)	TTSetDlgFontW(p1,p2,p3)
#else
#define TTSetDlgFont(p1,p2,p3)	TTSetDlgFontA(p1,p2,p3)
#endif

#ifdef __cplusplus
}
#endif
