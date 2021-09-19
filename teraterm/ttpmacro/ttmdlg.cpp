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
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "teraterm.h"
#include "ttm_res.h"
#include "tttypes.h"
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

#include "ttmdlg.h"

char HomeDir[MAX_PATH];
wchar_t *HomeDirW;
wchar_t FileName[MAX_PATH];
wchar_t TopicName[11];
wchar_t ShortName[MAX_PATH];
wchar_t **Params = NULL;
int ParamCnt;
int ParamsSize;
BOOL SleepFlag;

// (x,y) = (CW_USEDEFAULT, CW_USEDEFAULT)のときセンターに表示
static int DlgPosX = CW_USEDEFAULT;
static int DlgPosY = CW_USEDEFAULT;

static CStatDlg *StatDlg = NULL;

void ParseParam(PBOOL IOption, PBOOL VOption)
{
	wchar_t *Param, **ptmp;
	wchar_t Temp[MaxStrLen];
	wchar_t *start, *cur, *next;

	// go home directory
	_chdir(HomeDir);

	// Get command line parameters
	FileName[0] = 0;
	TopicName[0] = 0;
	SleepFlag = FALSE;
	*IOption = FALSE;
	*VOption = FALSE;
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
				*IOption = TRUE;
				continue;
			}
			else if (_wcsicmp(Temp, L"/S")==0) {
				SleepFlag = TRUE;
				continue;
			}
			else if (_wcsicmp(Temp, L"/V")==0) {
				*VOption = TRUE;
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

	if (FileName[0]=='*') {
		FileName[0] = 0;
	}
	else if (FileName[0]!=0) {
		size_t dirlen, fnpos;
		if (GetFileNamePosW(FileName, &dirlen, &fnpos)) {
			FitFileNameW(&FileName[fnpos], _countof(FileName) - fnpos, L".TTL");
			wcsncpy_s(ShortName, _countof(ShortName), &FileName[fnpos], _TRUNCATE);
			if (dirlen==0) {
				wcsncpy_s(FileName, _countof(FileName), HomeDirW, _TRUNCATE);
				AppendSlashW(FileName, _countof(FileName));
				wcsncat_s(FileName, _countof(FileName), ShortName, _TRUNCATE);
			}

			if (Params) {
				Params[1] = _wcsdup(ShortName);
			}
		}
		else {
			FileName[0] = 0;
		}
	}
}

BOOL GetFileName(HWND HWin)
{
	wchar_t FNFilter[31];
	OPENFILENAMEW FNameRec;
	wchar_t uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];

	if (FileName[0]!=0) {
		return FALSE;
	}

	memset(FNFilter, 0, sizeof(FNFilter));
	memset(&FNameRec, 0, sizeof(OPENFILENAME));
	get_lang_msgW("FILEDLG_OPEN_MACRO_FILTER", uimsg, sizeof(uimsg), L"Macro files (*.ttl)\\0*.ttl\\0\\0", UILanguageFile);
	memcpy(FNFilter, uimsg, sizeof(FNFilter));

	// sizeof(OPENFILENAME) では Windows98/NT で終了してしまうため (2006.8.14 maya)
	FNameRec.lStructSize = get_OPENFILENAME_SIZEW();
	FNameRec.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	FNameRec.hwndOwner = HWin;
	FNameRec.lpstrFilter = FNFilter;
	FNameRec.nFilterIndex = 1;
	FNameRec.lpstrFile = FileName;
	FNameRec.nMaxFile = sizeof(FileName);
	// 以前読み込んだ .ttl ファイルのパスを記憶できるように、初期ディレクトリを固定にしない。
	// (2008.4.7 yutaka)
#if 0
	FNameRec.lpstrInitialDir = HomeDir;
#endif
	FNameRec.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	FNameRec.lpstrDefExt = L"TTL";
	get_lang_msgW("FILEDLG_OPEN_MACRO_TITLE", uimsg2, sizeof(uimsg2), L"MACRO: Open macro", UILanguageFile);
	FNameRec.lpstrTitle = uimsg2;
	if (GetOpenFileNameW(&FNameRec)) {
		wcsncpy_s(ShortName, _countof(ShortName), &(FileName[FNameRec.nFileOffset]), _TRUNCATE);
	}
	else {
		FileName[0] = 0;
	}

	if (FileName[0]==0) {
		ShortName[0] = 0;
		return FALSE;
	}
	else {
		return TRUE;
	}
}

void SetDlgPos(int x, int y)
{
	DlgPosX = x;
	DlgPosY = y;
	if (StatDlg!=NULL) { // update status box position
		StatDlg->Update(NULL,NULL,DlgPosX,DlgPosY);
	}
}

void OpenInpDlg(wchar_t *Buff, const wchar_t *Text, const wchar_t *Caption,
                const wchar_t *Default, BOOL Paswd)
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	CInpDlg InpDlg(Buff,Text,Caption,Default,Paswd,DlgPosX,DlgPosY);
	InpDlg.DoModal(hInst, hWndParent);
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
int OpenListDlg(const wchar_t *Text, const wchar_t *Caption, wchar_t **Lists, int Selected)
{
	HINSTANCE hInst = GetInstance();
	HWND hWndParent = GetHWND();
	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY);
	INT_PTR r = ListDlg.DoModal(hInst, hWndParent);
	if (r == IDOK) {
		return ListDlg.m_SelectItem;
	}
	return r == IDCANCEL ? -1 : -2;
}
