/*
 * Copyright (C) 2022- TeraTerm Project
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
 *	common contol の TOOLTIPS_CLASS を使用したツールチップ
 */
#include <windows.h>
#include <commctrl.h>

#include "tipwin2.h"

typedef struct tagTipWinData {
	HWND hDlg;
	HWND hTip;
} TipWin2;

TipWin2 *TipWin2Create(HINSTANCE hInstance, HWND hDlg)
{
	HINSTANCE hInst = hInstance;
	if (hInstance == NULL) {
		hInst = (HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE);
	}

	HWND hTip = CreateWindowExW(NULL, TOOLTIPS_CLASSW, NULL,
								WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
								CW_USEDEFAULT, CW_USEDEFAULT,
								CW_USEDEFAULT, CW_USEDEFAULT,
								hDlg, NULL,
								hInst, NULL);
	if (hTip == NULL) {
		return NULL;
	}
	SendMessageW(hTip, TTM_SETMAXTIPWIDTH, 0, INT_MAX);

	TipWin2 *tWin = (TipWin2 *)calloc(sizeof(TipWin2), 1);
	if (tWin == NULL) {
		return NULL;
	}
	tWin->hTip = hTip;
	tWin->hDlg = hDlg;

	return tWin;
}

void TipWin2Destroy(TipWin2 *tWin)
{
	DestroyWindow(tWin->hTip);
	tWin->hTip = NULL;
	free(tWin);
}

/**
 * @brief ツールチップを登録する
 * @param tWin
 * @param id ダイアログのコントロールID
 * @param text ツールチップ
 */
void TipWin2SetTextW(TipWin2 *tWin, int id, const wchar_t *text)
{
	TOOLINFOW toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = tWin->hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;	// TTF_IDISHWND があればrectは参照されない
	toolInfo.uId = (UINT_PTR)GetDlgItem(tWin->hDlg, id);
	toolInfo.lpszText = (LPWSTR)text;	// text は SendMessage() 時に存在すれば良い
	SendMessageW(tWin->hTip, TTM_ADDTOOLW, 0, (LPARAM)&toolInfo);
}
