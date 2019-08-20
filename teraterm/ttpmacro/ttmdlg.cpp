/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
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
#include "ttmacro.h"

#include "ttmdlg.h"

#ifdef _DEBUG
#define malloc(l)     _malloc_dbg((l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, l) _realloc_dbg((p), (l), _NORMAL_BLOCK, __FILE__, __LINE__)
#define calloc(c, s)  _calloc_dbg((c), (s), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)       _free_dbg((p), _NORMAL_BLOCK)
#define strdup(s)	  _strdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#define _strdup(s)	  _strdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

char HomeDir[MAX_PATH];
char FileName[MAX_PATH];
char TopicName[11];
char ShortName[MAX_PATH];
char **Params = NULL;
int ParamCnt;
int ParamsSize;
BOOL SleepFlag;

static int DlgPosX = -10000;
static int DlgPosY = 0;

static CStatDlg *StatDlg = NULL;

void ParseParam(PBOOL IOption, PBOOL VOption)
{
	int dirlen, fnpos;
	char *Param, **ptmp;
	char Temp[MaxStrLen];
	PCHAR start, cur, next;

	// go home directory
	_chdir(HomeDir);

	// Get command line parameters
	FileName[0] = 0;
	TopicName[0] = 0;
	SleepFlag = FALSE;
	*IOption = FALSE;
	*VOption = FALSE;
	Param = GetCommandLine();

	ParamsSize = 50;
	Params = (char **)malloc(sizeof(char*) * ParamsSize);
	if (Params) {
		Params[0] = _strdup(Param);
		Params[1] = NULL;
	}

	// the first term shuld be executable filename of TTMACRO
	start = GetParam(Temp, sizeof(Temp), Param);
	ParamCnt = 0;

	for (cur = start; next = GetParam(Temp, sizeof(Temp), cur); cur = next) {
		DequoteParam(Temp, sizeof(Temp), Temp);
		if (ParamCnt == 0) {
			if (_strnicmp(Temp,"/D=",3)==0) { // DDE option
				strncpy_s(TopicName, sizeof(TopicName), &Temp[3], _TRUNCATE);
				continue;
			}
			else if (_stricmp(Temp, "/I")==0) {
				*IOption = TRUE;
				continue;
			}
			else if (_stricmp(Temp, "/S")==0) {
				SleepFlag = TRUE;
				continue;
			}
			else if (_stricmp(Temp, "/V")==0) {
				*VOption = TRUE;
				continue;
			}
		}

		if (++ParamCnt == 1) {
			strncpy_s(FileName, sizeof(FileName), Temp, _TRUNCATE);
			if (Params == NULL) {
				break;
			}
		}
		else {
			if (ParamsSize <= ParamCnt) {
				ParamsSize += 10;
				ptmp = (char **)realloc(Params, sizeof(char*) * ParamsSize);
				if (ptmp == NULL) {
					ParamCnt--;
					break;
				}
				Params = ptmp;
			}
			Params[ParamCnt] = _strdup(Temp);
		}
	}

	if (FileName[0]=='*') {
		FileName[0] = 0;
	}
	else if (FileName[0]!=0) {
		if (GetFileNamePos(FileName, &dirlen, &fnpos)) {
			FitFileName(&FileName[fnpos], sizeof(FileName) - fnpos, ".TTL");
			strncpy_s(ShortName, sizeof(ShortName), &FileName[fnpos], _TRUNCATE);
			if (dirlen==0) {
				strncpy_s(FileName, sizeof(FileName), HomeDir, _TRUNCATE);
				AppendSlash(FileName, sizeof(FileName));
				strncat_s(FileName, sizeof(FileName), ShortName, _TRUNCATE);
			}

			if (Params) {
				Params[1] = _strdup(ShortName);
			}
		}
		else {
			FileName[0] = 0;
		}
	}
}

BOOL GetFileName(HWND HWin)
{
	char FNFilter[31];
	OPENFILENAME FNameRec;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];

	if (FileName[0]!=0) {
		return FALSE;
	}

	memset(FNFilter, 0, sizeof(FNFilter));
	memset(&FNameRec, 0, sizeof(OPENFILENAME));
	get_lang_msg("FILEDLG_OPEN_MACRO_FILTER", uimsg, sizeof(uimsg), "Macro files (*.ttl)\\0*.ttl\\0\\0", UILanguageFile);
	memcpy(FNFilter, uimsg, sizeof(FNFilter));

	// sizeof(OPENFILENAME) では Windows98/NT で終了してしまうため (2006.8.14 maya)
	FNameRec.lStructSize = get_OPENFILENAME_SIZE();
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
	FNameRec.lpstrDefExt = "TTL";
	get_lang_msg("FILEDLG_OPEN_MACRO_TITLE", uimsg2, sizeof(uimsg2), "MACRO: Open macro", UILanguageFile);
	FNameRec.lpstrTitle = uimsg2;
	if (GetOpenFileName(&FNameRec)) {
		strncpy_s(ShortName, sizeof(ShortName), &(FileName[FNameRec.nFileOffset]), _TRUNCATE);
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

void OpenInpDlg(PCHAR Buff, PCHAR Text, PCHAR Caption,
                PCHAR Default, BOOL Paswd)
{
	CInpDlg InpDlg(Buff,Text,Caption,Default,Paswd,DlgPosX,DlgPosY);
	InpDlg.DoModal();
}

int OpenErrDlg(const char *Msg, PCHAR Line, int lineno, int start, int end, PCHAR FileName)
{
	CErrDlg ErrDlg(Msg,Line,DlgPosX,DlgPosY, lineno, start, end, FileName);
	return ErrDlg.DoModal();
}

int OpenMsgDlg(PCHAR Text, PCHAR Caption, BOOL YesNo)
{
	CMsgDlg MsgDlg(Text,Caption,YesNo,DlgPosX,DlgPosY);
	return MsgDlg.DoModal();
}

void OpenStatDlg(PCHAR Text, PCHAR Caption)
{
	if (StatDlg==NULL) {
		StatDlg = new CStatDlg();
		StatDlg->Create(Text,Caption,DlgPosX,DlgPosY);
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
int OpenListDlg(PCHAR Text, PCHAR Caption, const CHAR **Lists, int Selected)
{
	CListDlg ListDlg(Text, Caption, Lists, Selected, DlgPosX, DlgPosY);
	INT_PTR r = ListDlg.DoModal();
	if (r == IDOK) {
		return ListDlg.m_SelectItem;
	}
	return r == IDCANCEL ? -1 : -2;
}

