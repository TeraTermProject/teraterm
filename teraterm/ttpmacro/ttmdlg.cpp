/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* TTMACRO.EXE, dialog boxes */

#include <windows.h>
#include <direct.h>
#include <commdlg.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "compat_win.h"
#include "teraterm.h"
#include "ttm_res.h"
#include "ttlib.h"
#include "ttmdef.h"
#include "errdlg.h"
#include "inpdlg.h"
#include "msgdlg.h"
#include "statdlg.h"
#include "ListDlg.h"
#include "ttmlib.h"
#include "ttmdlg.h"
#include "ttmacro.h"
#include "ttmparse.h"
#include "ttmdde.h"

#include "ttmdlg.h"

wchar_t *HomeDirW;
wchar_t FileName[MAX_PATH];
wchar_t TopicName[11];
wchar_t ShortName[MAX_PATH];
wchar_t **Params = NULL;
int ParamCnt;
int ParamsSize;
BOOL SleepFlag;
BOOL IOption;
BOOL VOption;

// (x,y) = (CW_USEDEFAULT, CW_USEDEFAULT)のときセンターに表示
static int DlgPosX = CW_USEDEFAULT;
static int DlgPosY = CW_USEDEFAULT;
static int DlgPosition = 0;
static int DlgOffsetX = 0;
static int DlgOffsetY = 0;

static CStatDlg *StatDlg = NULL;

void ParseParam(void)
{
	wchar_t *Param, **ptmp;
	wchar_t Temp[MaxStrLen];
	wchar_t *start, *cur, *next;

	// Get command line parameters
	FileName[0] = 0;
	TopicName[0] = 0;
	SleepFlag = FALSE;
	IOption = FALSE;
	VOption = FALSE;
	Param = GetCommandLineW();

	ParamsSize = 50;
	Params = (wchar_t **)malloc(sizeof(wchar_t *) * ParamsSize);
	if (Params) {
		Params[0] = _wcsdup(Param);
		Params[1] = NULL;
	}

	// the first term shuld be executable filename of TTMACRO
	start = GetParam(Temp, _countof(Temp), Param);
	ParamCnt = 0;

	for (cur = start; next = GetParam(Temp, sizeof(Temp), cur); cur = next) {
		DequoteParam(Temp, _countof(Temp), Temp);
		if (ParamCnt == 0) {
			if (_wcsnicmp(Temp,L"/D=",3)==0) { // DDE option
				wcsncpy_s(TopicName, _countof(TopicName), &Temp[3], _TRUNCATE);
				continue;
			}
			else if (_wcsicmp(Temp, L"/I")==0) {
				IOption = TRUE;
				continue;
			}
			else if (_wcsicmp(Temp, L"/S")==0) {
				SleepFlag = TRUE;
				continue;
			}
			else if (_wcsicmp(Temp, L"/V")==0) {
				VOption = TRUE;
				continue;
			}
		}

		if (++ParamCnt == 1) {
			wcsncpy_s(FileName, _countof(FileName), Temp, _TRUNCATE);
			if (Params == NULL) {
				break;
			}
		}
		else {
			if (ParamsSize <= ParamCnt) {
				ParamsSize += 10;
				ptmp = (wchar_t **)realloc(Params, sizeof(wchar_t*) * ParamsSize);
				if (ptmp == NULL) {
					ParamCnt--;
					break;
				}
				Params = ptmp;
			}
			Params[ParamCnt] = _wcsdup(Temp);
		}
	}
}

BOOL GetFileName(HWND HWin, wchar_t **fname)
{
	wchar_t *FNFilter;
	wchar_t *title;
	TTOPENFILENAMEW FNameRec = {};

	GetI18nStrWW("Tera Term", "FILEDLG_OPEN_MACRO_FILTER", L"Macro files (*.ttl)\\0*.ttl\\0\\0", UILanguageFileW, &FNFilter);
	GetI18nStrWW("Tera Term", "FILEDLG_OPEN_MACRO_TITLE", L"MACRO: Open macro", UILanguageFileW, &title);

	FNameRec.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	FNameRec.hwndOwner = HWin;
	FNameRec.lpstrFilter = FNFilter;
	FNameRec.nFilterIndex = 1;
	FNameRec.lpstrFile = FileName;
	// 以前読み込んだ .ttl ファイルのパスを記憶できるように、初期ディレクトリを固定にしない。
	// (2008.4.7 yutaka)
#if 0
	FNameRec.lpstrInitialDir = HomeDirW;
#endif
	FNameRec.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	FNameRec.lpstrDefExt = L"TTL";
	FNameRec.lpstrTitle = title;
	wchar_t *fn;
	BOOL r = TTGetOpenFileNameW(&FNameRec, &fn);
	free(FNFilter);
	free(title);
	if (r == FALSE) {
		*fname = NULL;
		return FALSE;
	}
	*fname = fn;
	return TRUE;
}

void SetDlgPos(int x, int y, int position, int offset_x, int offset_y)
{
	DlgPosX = x;
	DlgPosY = y;
	DlgPosition = position;
	DlgOffsetX = offset_x;
	DlgOffsetY = offset_y;
	if (StatDlg!=NULL) { // update status box position
		StatDlg->Update(NULL,NULL,DlgPosX,DlgPosY);
	}
}

/*
  SetDlgPosEX()

   setdlgpos のパラメータ指定のうち、下記のNo.3と4を処理する。
   No.1とNo.2は macrodlgbase.h の SetDlgPos() で処理する。

   setdlgpos [<x> <y> [<position> [<offset x> <offset y>]]]
   1. setdlgpos
   2. setdlgpos <x> <y>
   3. setdlgpos <x> <y> <position>
   4. setdlgpos <x> <y> <position> <offset x> <offset y>

*/
int SetDlgPosEX(HWND hWnd, int width, int height, int *PosX, int *PosY) {
	char Str[MaxStrLen];
	int showflag;
	int w_x, w_y, w_width, w_height;
	int c_x, c_y, c_width, c_height;
	RECT rcDesktop;
	int new_x = 0, new_y = 0, position;
	POINT pt;

	if (hWnd == NULL || DlgPosition == 0) {
		return -1;		// 固定位置
	}

	if (Linked) {
		if (GetTTParam(CmdGetTTPos, Str, sizeof(Str)) != 0) {
			return -1;
		}
		if (sscanf_s(Str, "%d %d %d %d %d %d %d %d %d", &showflag,
					 &w_x, &w_y, &w_width, &w_height,
					 &c_x, &c_y, &c_width, &c_height) != 9) {
			return -1;
		}
	}

	if (Linked && showflag != 1 && showflag != 3) {
		pt.x = w_x + w_width  / 2;
		pt.y = w_y + w_height / 2;
	} else {
		pt.x = 0;
		pt.y = 0;
	}
	GetDesktopRectFromPoint(&pt, &rcDesktop);

	if (DlgPosition >= 1 && DlgPosition <= 5) {					// ディスプレイ基準
		c_x = rcDesktop.left;
		c_y = rcDesktop.top;
		c_width  = rcDesktop.right - rcDesktop.left;
		c_height = rcDesktop.bottom - rcDesktop.top;
		position = DlgPosition;
	} else if (DlgPosition >= 6 && DlgPosition <= 10) {			// vt window基準
		if (! Linked || showflag == 1 || showflag == 3) {
			return -1; // 非リンク、1:最小化、3:非表示 の場合は固定位置
		}
		position = DlgPosition - 5;
	} else {
		return -1;
	}

	switch (position) {
	case 1: // 左上隅
		new_x = c_x;
		new_y = c_y;
		break;
	case 2: // 右上隅
		new_x = c_x + c_width - width;
		new_y = c_y;
		break;
	case 3: // 左下隅
		new_x = c_x;
		new_y = c_y + c_height - height;
		break;
	case 4: // 右下隅
		new_x = c_x + c_width  - width;
		new_y = c_y + c_height - height;
		break;
	case 5: // 中央
		new_x = c_x + c_width  / 2 - width  / 2;
		new_y = c_y + c_height / 2 - height / 2;
		break;
	}
	new_x += DlgOffsetX;
	new_y += DlgOffsetY;

	// デスクトップからはみ出さないよう調整
	if (new_x + width > rcDesktop.right) {
		new_x = rcDesktop.right - width;
	}
	if (new_x < rcDesktop.left){
		new_x = rcDesktop.left;
	}
	if (new_y + height > rcDesktop.bottom) {
		new_y = rcDesktop.bottom - height;
	}
	if (new_y < rcDesktop.top) {
		new_y = rcDesktop.top;
	}
	*PosX = new_x;
	*PosY = new_y;

	SetWindowPos(hWnd, HWND_TOP, new_x, new_y, width, height, 0);

	return 0;
}

int OpenInpDlg(wchar_t *Buff, const wchar_t *Text, const wchar_t *Caption,
                const wchar_t *Default, BOOL Paswd)
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	CInpDlg InpDlg(Buff,Text,Caption,Default,Paswd,DlgPosX,DlgPosY);
	return InpDlg.DoModal(hInst, hWndParent);
}

int OpenErrDlg(const char *Msg, const char *Line, int lineno, int start, int end, const char *FileName)
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	CErrDlg ErrDlg(Msg,Line,DlgPosX,DlgPosY, lineno, start, end, FileName);
	return ErrDlg.DoModal(hInst, hWndParent);
}

int OpenMsgDlg(const wchar_t *Text, const wchar_t *Caption, BOOL YesNo)
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	CMsgDlg MsgDlg(Text,Caption,YesNo,DlgPosX,DlgPosY);
	return MsgDlg.DoModal(hInst, hWndParent);
}

void OpenStatDlg(const wchar_t *Text, const wchar_t *Caption)
{
	if (StatDlg==NULL) {
		HINSTANCE hInst = GetInstance();
		StatDlg = new CStatDlg();
		StatDlg->Create(hInst,Text,Caption,DlgPosX,DlgPosY);
	}
	else {// if status box already exists,
		// update text and caption only.
		StatDlg->Update(Text,Caption,32767,0);
	}
}

void CloseStatDlg()
{
	if (StatDlg==NULL) {
		return;
	}
	assert(_CrtCheckMemory());
	StatDlg->DestroyWindow();
	assert(_CrtCheckMemory());
	StatDlg = NULL;
}

void BringupStatDlg()
{
	if (StatDlg==NULL) {
		return;
	}
	StatDlg->Bringup();
}

/**
 * @retval 0以上	選択項目
 * @retval -1		cancelボタン
 * @retval -2		closeボタン
 */
int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected, int ext, int DlgWidth, int DlgHeight)
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY, ext, DlgWidth, DlgHeight);
	INT_PTR r = ListDlg.DoModal(hInst, hWndParent);
	if (r == IDOK) {
		return ListDlg.m_SelectItem;
	}
	return r == IDCANCEL ? -1 : -2;
}
