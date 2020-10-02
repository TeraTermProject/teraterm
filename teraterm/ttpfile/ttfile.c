/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2019 TeraTerm Project
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

/* TTFILE.DLL, file transfer, VT window printing */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <direct.h>
#include <commdlg.h>
#include <string.h>

#include "ttlib.h"
#include "ftlib.h"
#include "dlglib.h"
#include "kermit.h"
#include "xmodem.h"
#include "ymodem.h"
#include "zmodem.h"
#include "bplus.h"
#include "quickvan.h"
// resource IDs
#include "file_res.h"

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <assert.h>

#include "compat_w95.h"

static HANDLE hInst;

static HFONT DlgFoptFont;
static HFONT DlgXoptFont;
static HFONT DlgGetfnFont;

char UILanguageFile[MAX_PATH];
char FileSendFilter[128];

BOOL PASCAL GetSetupFname(HWND HWin, WORD FuncId, PTTSet ts)
{
	int i, j;
	OPENFILENAME ofn;
	char uimsg[MAX_UIMSG];

	//  char FNameFilter[HostNameMaxLength + 1]; // 81(yutaka)
	char FNameFilter[81]; // 81(yutaka)
	char TempDir[MAXPATHLEN];
	char Dir[MAXPATHLEN];
	char Name[MAX_PATH];
	BOOL Ok;

	/* save current dir */
	_getcwd(TempDir,sizeof(TempDir));

	/* File name filter */
	memset(FNameFilter, 0, sizeof(FNameFilter));
	if (FuncId==GSF_LOADKEY) {
		get_lang_msg("FILEDLG_KEYBOARD_FILTER", uimsg, sizeof(uimsg), "keyboard setup files (*.cnf)\\0*.cnf\\0\\0", UILanguageFile);
		memcpy(FNameFilter, uimsg, sizeof(FNameFilter));
	}
	else {
		get_lang_msg("FILEDLG_SETUP_FILTER", uimsg, sizeof(uimsg), "setup files (*.ini)\\0*.ini\\0\\0", UILanguageFile);
		memcpy(FNameFilter, uimsg, sizeof(FNameFilter));
	}

	/* OPENFILENAME record */
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = HWin;
	ofn.lpstrFile   = Name;
	ofn.nMaxFile    = sizeof(Name);
	ofn.lpstrFilter = FNameFilter;
	ofn.nFilterIndex = 1;
	ofn.hInstance = hInst;

	if (FuncId==GSF_LOADKEY) {
		ofn.lpstrDefExt = "cnf";
		GetFileNamePos(ts->KeyCnfFN,&i,&j);
		strncpy_s(Name, sizeof(Name),&(ts->KeyCnfFN[j]), _TRUNCATE);
		memcpy(Dir,ts->KeyCnfFN,i);
		Dir[i] = 0;

		if ((strlen(Name)==0) || (_stricmp(Name,"KEYBOARD.CNF")==0))
			strncpy_s(Name, sizeof(Name),"KEYBOARD.CNF", _TRUNCATE);
	}
	else {
		ofn.lpstrDefExt = "ini";
		GetFileNamePos(ts->SetupFName,&i,&j);
		strncpy_s(Name, sizeof(Name),&(ts->SetupFName[j]), _TRUNCATE);
		memcpy(Dir,ts->SetupFName,i);
		Dir[i] = 0;

		if ((strlen(Name)==0) || (_stricmp(Name,"TERATERM.INI")==0))
			strncpy_s(Name, sizeof(Name),"TERATERM.INI", _TRUNCATE);
	}

	if (strlen(Dir)==0)
		strncpy_s(Dir, sizeof(Dir),ts->HomeDir, _TRUNCATE);

	_chdir(Dir);

	switch (FuncId) {
	case GSF_SAVE:
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_SHOWHELP;
		// 初期ファイルディレクトリをプログラム本体がある箇所に固定する (2005.1.6 yutaka)
		// 読み込まれたteraterm.iniがあるディレクトリに固定する。
		// これにより、/F= で指定された位置に保存されるようになる。(2005.1.26 yutaka)
		// Windows Vista ではファイル名まで指定すると NULL と同じ挙動をするようなので、
		// ファイル名を含まない形でディレクトリを指定するようにした。(2006.9.16 maya)
//		ofn.lpstrInitialDir = __argv[0];
//		ofn.lpstrInitialDir = ts->SetupFName;
		ofn.lpstrInitialDir = Dir;
		get_lang_msg("FILEDLG_SAVE_SETUP_TITLE", uimsg, sizeof(uimsg), "Tera Term: Save setup", UILanguageFile);
		ofn.lpstrTitle = uimsg;
		Ok = GetSaveFileName(&ofn);
		if (Ok)
			strncpy_s(ts->SetupFName, sizeof(ts->SetupFName),Name, _TRUNCATE);
		break;
	case GSF_RESTORE:
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
		get_lang_msg("FILEDLG_RESTORE_SETUP_TITLE", uimsg, sizeof(uimsg), "Tera Term: Restore setup", UILanguageFile);
		ofn.lpstrTitle = uimsg;
		Ok = GetOpenFileName(&ofn);
		if (Ok)
			strncpy_s(ts->SetupFName, sizeof(ts->SetupFName),Name, _TRUNCATE);
		break;
	case GSF_LOADKEY:
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
		get_lang_msg("FILEDLG_LOAD_KEYMAP_TITLE", uimsg, sizeof(uimsg), "Tera Term: Load key map", UILanguageFile);
		ofn.lpstrTitle = uimsg;
		Ok = GetOpenFileName(&ofn);
		if (Ok)
			strncpy_s(ts->KeyCnfFN, sizeof(ts->KeyCnfFN),Name, _TRUNCATE);
		break;
	default:
		assert(FALSE);
		Ok = FALSE;
		break;
	}

#if defined(_DEBUG)
	if (!Ok) {
		DWORD Err = GetLastError();
		DWORD DlgErr = CommDlgExtendedError();
		assert(Err == 0 && DlgErr == 0);
	}
#endif

	/* restore dir */
	_chdir(TempDir);

	return Ok;
}

/* ダイアログを中央に移動する */
static void CenterCommonDialog(HWND hDlg)
{
	/* hDlgの親がダイアログのウィンドウハンドル */
	HWND hWndDlgRoot = GetParent(hDlg);
	CenterWindow(hWndDlgRoot, GetParent(hWndDlgRoot));
}

static UINT_PTR CALLBACK TransFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam);

BOOL WINAPI GetTransFname(PFileVar fv, PCHAR CurDir, WORD FuncId, LPLONG Option)
{
	char uimsg[MAX_UIMSG];
	char *FNFilter;
	OPENFILENAME ofn;
	WORD optw = 0;
	char TempDir[MAXPATHLEN];
	BOOL Ok;
	char FileName[MAX_PATH];

	/* save current dir */
	_getcwd(TempDir,sizeof(TempDir));

	memset(&ofn, 0, sizeof(OPENFILENAME));

	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	switch (FuncId) {
	case GTF_SEND:
		get_lang_msg("FILEDLG_TRANS_TITLE_SENDFILE", uimsg, sizeof(uimsg), TitSendFile, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case GTF_BP:
		get_lang_msg("FILEDLG_TRANS_TITLE_BPSEND", uimsg, sizeof(uimsg), TitBPSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	default:
		return FALSE;
	}

	FNFilter = GetCommonDialogFilterA(FileSendFilter, UILanguageFile);

	ExtractFileName(fv->FullName, FileName ,sizeof(FileName));
	strncpy_s(fv->FullName, sizeof(fv->FullName), FileName, _TRUNCATE);
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = fv->HMainWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FullName;
	ofn.nMaxFile = sizeof(fv->FullName);
	ofn.lpstrInitialDir = CurDir;

	switch (FuncId) {
	case GTF_SEND:
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FOPT);

		ofn.lpfnHook = (LPOFNHOOKPROC)(&TransFnHook);
		optw = (WORD)*Option;
		ofn.lCustData = (LPARAM)&optw;
		break;
	case GTF_BP:
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		break;
	}

	ofn.Flags |= OFN_SHOWHELP;

	ofn.lpstrTitle = fv->DlgCaption;

	ofn.hInstance = hInst;

	Ok = GetOpenFileName(&ofn);
	free(FNFilter);

	if (Ok) {
		*Option = (long)optw;

		fv->DirLen = ofn.nFileOffset;

		if (CurDir!=NULL) {
			memcpy(CurDir,fv->FullName,fv->DirLen-1);
			CurDir[fv->DirLen-1] = 0;
		}
	}
	/* restore dir */
	_chdir(TempDir);
	return Ok;
}

static UINT_PTR CALLBACK TransFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ IDC_FOPT, "DLG_FOPT" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
	};
	LPOPENFILENAME ofn;
	LPWORD pw;
	LPOFNOTIFY notify;
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
	case WM_INITDIALOG:
		ofn = (LPOPENFILENAME)lParam;
		pw = (LPWORD)ofn->lCustData;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)pw);

		font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (get_lang_font("DLG_TAHOMA_FONT", Dialog, &logfont, &DlgFoptFont, UILanguageFile)) {
			SendDlgItemMessage(Dialog, IDC_FOPT, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_FOPTBIN, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_FOPTAPPEND, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_PLAINTEXT, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_TIMESTAMP, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgFoptFont = NULL;
		}

		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);

		SetRB(Dialog,*pw & 1,IDC_FOPTBIN,IDC_FOPTBIN);

		CenterCommonDialog(Dialog);

		return TRUE;
	case WM_COMMAND: // for old style dialog
		switch (LOWORD(wParam)) {
		case IDOK:
			pw = (LPWORD)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pw!=NULL)
				GetRB(Dialog,pw,IDC_FOPTBIN,IDC_FOPTBIN);
			if (DlgFoptFont != NULL) {
				DeleteObject(DlgFoptFont);
			}
			break;
		case IDCANCEL:
			if (DlgFoptFont != NULL) {
				DeleteObject(DlgFoptFont);
			}
			break;
		}
		break;
	case WM_NOTIFY: // for Explorer-style dialog
		notify = (LPOFNOTIFY)lParam;
		switch (notify->hdr.code) {
		case CDN_FILEOK:
			pw = (LPWORD)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pw!=NULL)
				GetRB(Dialog,pw,IDC_FOPTBIN,IDC_FOPTBIN);
			if (DlgFoptFont != NULL) {
				DeleteObject(DlgFoptFont);
			}
			break;
		}
		break;
	}
	return FALSE;
}

BOOL WINAPI GetMultiFname(PFileVar fv, PCHAR CurDir, WORD FuncId, LPWORD Option)
{
	int i, len;
	char uimsg[MAX_UIMSG];
	char *FNFilter;
	OPENFILENAME ofn;
	char TempDir[MAXPATHLEN];
	BOOL Ok;
	char defaultFName[MAX_PATH];

	/* save current dir */
	_getcwd(TempDir,sizeof(TempDir));

	fv->NumFname = 0;

	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	switch (FuncId) {
	case GMF_KERMIT:
		get_lang_msg("FILEDLG_TRANS_TITLE_KMTSEND", uimsg, sizeof(uimsg), TitKmtSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case GMF_Z:
		get_lang_msg("FILEDLG_TRANS_TITLE_ZSEND", uimsg, sizeof(uimsg), TitZSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case GMF_QV:
		get_lang_msg("FILEDLG_TRANS_TITLE_QVSEND", uimsg, sizeof(uimsg), TitQVSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case GMF_Y:
		get_lang_msg("FILEDLG_TRANS_TITLE_YSEND", uimsg, sizeof(uimsg), TitYSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	default:
		return FALSE;
	}

	/* moemory should be zero-initialized */
	fv->FnStrMemHandle = GlobalAlloc(GHND, FnStrMemSize);
	if (fv->FnStrMemHandle == NULL) {
		MessageBeep(0);
		return FALSE;
	}
	else {
		fv->FnStrMem = GlobalLock(fv->FnStrMemHandle);
		if (fv->FnStrMem == NULL) {
			GlobalFree(fv->FnStrMemHandle);
			fv->FnStrMemHandle = 0;
			MessageBeep(0);
			return FALSE;
		}
	}

	FNFilter = GetCommonDialogFilterA(FileSendFilter, UILanguageFile);

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = fv->HMainWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FnStrMem;
	ofn.nMaxFile = FnStrMemSize;
	ofn.lpstrTitle= fv->DlgCaption;
	ofn.lpstrInitialDir = CurDir;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	ofn.Flags |= OFN_SHOWHELP;
	ofn.lCustData = 0;
	if (FuncId==GMF_Z) {
		ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
		ofn.lCustData = (LPARAM)Option;
		ofn.lpfnHook = TransFnHook;
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FOPT);
	} else if (FuncId==GMF_Y) {
		// TODO: YMODEM

	}

	ofn.hInstance = hInst;

	// フィルタがワイルドカードではなく、そのファイルが存在する場合
	// あらかじめデフォルトのファイル名を入れておく (2008.5.18 maya)
	if (strlen(FileSendFilter) > 0 && !isInvalidFileNameChar(FileSendFilter)) {
		char file[MAX_PATH];
		strncpy_s(file, sizeof(file), CurDir, _TRUNCATE);
		AppendSlash(file, sizeof(file));
		strncat_s(file, sizeof(file), FileSendFilter, _TRUNCATE);
		if (_access(file, 0) == 0) {
			strncpy_s(defaultFName, sizeof(defaultFName), FileSendFilter, _TRUNCATE);
			ofn.lpstrFile = defaultFName;
		}
	}

	Ok = GetOpenFileName(&ofn);
	free(FNFilter);

	if (Ok) {
		/* count number of file names */
		len = strlen(fv->FnStrMem);
		i = 0;
		while (len>0) {
			i = i + len + 1;
			fv->NumFname++;
			len = strlen(&fv->FnStrMem[i]);
		}

		fv->NumFname--;

		if (fv->NumFname<1) { // single selection
			fv->NumFname = 1;
			fv->DirLen = ofn.nFileOffset;
			strncpy_s(fv->FullName, sizeof(fv->FullName),fv->FnStrMem, _TRUNCATE);
			fv->FnPtr = 0;
		}
		else { // multiple selection
			strncpy_s(fv->FullName, sizeof(fv->FullName),fv->FnStrMem, _TRUNCATE);
			AppendSlash(fv->FullName,sizeof(fv->FullName));
			fv->DirLen = strlen(fv->FullName);
			fv->FnPtr = strlen(fv->FnStrMem)+1;
		}

		memcpy(CurDir,fv->FullName,fv->DirLen);
		CurDir[fv->DirLen] = 0;
		if ((fv->DirLen>3) &&
		    (CurDir[fv->DirLen-1]=='\\'))
			CurDir[fv->DirLen-1] = 0;

		fv->FNCount = 0;
	}

	GlobalUnlock(fv->FnStrMemHandle);
	if (! Ok) {
		GlobalFree(fv->FnStrMemHandle);
		fv->FnStrMemHandle = NULL;
	}

	/* restore dir */
	_chdir(TempDir);

	return Ok;
}

static INT_PTR CALLBACK GetFnDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_GETFN_TITLE" },
		{ IDC_FILENAME, "DLG_GETFN_FILENAME" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_GETFNHELP, "BTN_HELP" },
	};
	PFileVar fv;
	char TempFull[MAX_PATH];
	int i, j;
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
	case WM_INITDIALOG:
		fv = (PFileVar)lParam;
		SetWindowLongPtr(Dialog, DWLP_USER, lParam);
		SendDlgItemMessage(Dialog, IDC_GETFN, EM_LIMITTEXT, sizeof(TempFull)-1,0);

		font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgGetfnFont, UILanguageFile)) {
			SendDlgItemMessage(Dialog, IDC_FILENAME, WM_SETFONT, (WPARAM)DlgGetfnFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_GETFN, WM_SETFONT, (WPARAM)DlgGetfnFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgGetfnFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgGetfnFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_GETFNHELP, WM_SETFONT, (WPARAM)DlgGetfnFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgGetfnFont = NULL;
		}

		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);

		return TRUE;

	case WM_COMMAND:
		fv = (PFileVar)GetWindowLongPtr(Dialog,DWLP_USER);
		switch (LOWORD(wParam)) {
		case IDOK:
			if (fv!=NULL) {
				GetDlgItemText(Dialog, IDC_GETFN, TempFull, sizeof(TempFull));
				if (strlen(TempFull)==0) return TRUE;
				GetFileNamePos(TempFull,&i,&j);
				FitFileName(&(TempFull[j]),sizeof(TempFull) - j, NULL);
				strncat_s(fv->FullName,sizeof(fv->FullName),&(TempFull[j]),_TRUNCATE);
			}
			EndDialog(Dialog, 1);
			if (DlgGetfnFont != NULL) {
				DeleteObject(DlgGetfnFont);
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(Dialog, 0);
			if (DlgGetfnFont != NULL) {
				DeleteObject(DlgGetfnFont);
			}
			return TRUE;
		case IDC_GETFNHELP:
			if (fv!=NULL) {
				// 呼び出し元がヘルプIDを準備する
				PostMessage(fv->HMainWin,WM_USER_DLGHELP2,0,0);
			}
			break;
		}
	}
	return FALSE;
}

BOOL WINAPI GetGetFname(HWND HWin, PFileVar fv)
{
	return (BOOL)DialogBoxParam(hInst,
	                            MAKEINTRESOURCE(IDD_GETFNDLG),
	                            HWin, GetFnDlg, (LPARAM)fv);
}

void WINAPI SetFileVar(PFileVar fv)
{
	int i;
	char uimsg[MAX_UIMSG];
	char c;

	GetFileNamePos(fv->FullName,&(fv->DirLen),&i);
	c = fv->FullName[fv->DirLen];
	if (c=='\\'||c=='/') fv->DirLen++;
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	switch (fv->OpId) {
	case OpLog:
		get_lang_msg("FILEDLG_TRANS_TITLE_LOG", uimsg, sizeof(uimsg), TitLog, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpSendFile:
		get_lang_msg("FILEDLG_TRANS_TITLE_SENDFILE", uimsg, sizeof(uimsg), TitSendFile, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpKmtRcv:
		get_lang_msg("FILEDLG_TRANS_TITLE_KMTRCV", uimsg, sizeof(uimsg), TitKmtRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpKmtGet:
		get_lang_msg("FILEDLG_TRANS_TITLE_KMTGET", uimsg, sizeof(uimsg), TitKmtGet, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpKmtSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_KMTSEND", uimsg, sizeof(uimsg), TitKmtSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpKmtFin:
		get_lang_msg("FILEDLG_TRANS_TITLE_KMTFIN", uimsg, sizeof(uimsg), TitKmtFin, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpXRcv:
		get_lang_msg("FILEDLG_TRANS_TITLE_XRCV", uimsg, sizeof(uimsg), TitXRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpXSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_XSEND", uimsg, sizeof(uimsg), TitXSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpYRcv:
		get_lang_msg("FILEDLG_TRANS_TITLE_YRCV", uimsg, sizeof(uimsg), TitYRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpYSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_YSEND", uimsg, sizeof(uimsg), TitYSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpZRcv:
		get_lang_msg("FILEDLG_TRANS_TITLE_ZRCV", uimsg, sizeof(uimsg), TitZRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpZSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_ZSEND", uimsg, sizeof(uimsg), TitZSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpBPRcv:
		get_lang_msg("FILEDLG_TRANS_TITLE_BPRCV", uimsg, sizeof(uimsg), TitBPRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpBPSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_BPSEND", uimsg, sizeof(uimsg), TitBPSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpQVRcv:
		get_lang_msg("FILEDLG_TRANS_TITLE_QVRCV", uimsg, sizeof(uimsg), TitQVRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	case OpQVSend:
		get_lang_msg("FILEDLG_TRANS_TITLE_QVSEND", uimsg, sizeof(uimsg), TitQVSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
		break;
	}
}

/* Hook function for XMODEM file name dialog box */
static UINT_PTR CALLBACK XFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ IDC_XOPT, "DLG_XOPT" },
		{ IDC_XOPTCHECK, "DLG_XOPT_CHECKSUM" },
		{ IDC_XOPTCRC, "DLG_XOPT_CRC" },
		{ IDC_XOPT1K, "DLG_XOPT_1K" },
		{ IDC_XOPTBIN, "DLG_XOPT_BINARY" },
	};
	LPOPENFILENAME ofn;
	WORD Hi, Lo;
	LPLONG pl;
	LPOFNOTIFY notify;
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
	case WM_INITDIALOG:
		ofn = (LPOPENFILENAME)lParam;
		pl = (LPLONG)ofn->lCustData;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)pl);

		font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (get_lang_font("DLG_TAHOMA_FONT", Dialog, &logfont, &DlgXoptFont, UILanguageFile)) {
			SendDlgItemMessage(Dialog, IDC_XOPT, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTCHECK, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTCRC, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPT1K, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTBIN, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgXoptFont = NULL;
		}

		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);

		if (LOWORD(*pl)==0xFFFF) { // Send
			ShowDlgItem(Dialog, IDC_XOPT1K, IDC_XOPT1K);
			Hi = 0;
			if (HIWORD(*pl) == Xopt1kCRC || HIWORD(*pl) == Xopt1kCksum) {
				Hi = 1;
			}
			SetRB(Dialog, Hi, IDC_XOPT1K, IDC_XOPT1K);
		}
		else { // Recv
			ShowDlgItem(Dialog, IDC_XOPTCHECK, IDC_XOPTCRC);
			Hi = HIWORD(*pl);
			if (Hi == Xopt1kCRC) {
				Hi = XoptCRC;
			}
			else if (Hi == Xopt1kCksum) {
				Hi = XoptCheck;
			}
			SetRB(Dialog, Hi, IDC_XOPTCHECK, IDC_XOPTCRC);

			ShowDlgItem(Dialog,IDC_XOPTBIN,IDC_XOPTBIN);
			SetRB(Dialog,LOWORD(*pl),IDC_XOPTBIN,IDC_XOPTBIN);
		}
		CenterCommonDialog(Dialog);
		return TRUE;
	case WM_COMMAND: // for old style dialog
		switch (LOWORD(wParam)) {
		case IDOK:
			pl = (LPLONG)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pl!=NULL)
			{
				if (LOWORD(*pl)==0xFFFF) { // Send
					Lo = 0xFFFF;

					GetRB(Dialog, &Hi, IDC_XOPT1K, IDC_XOPT1K);
					if (Hi > 0) { // force CRC if 1K
						Hi = Xopt1kCRC;
					}
					else {
						Hi = XoptCRC;
					}
				}
				else { // Recv
					GetRB(Dialog, &Lo, IDC_XOPTBIN, IDC_XOPTBIN);
					GetRB(Dialog, &Hi, IDC_XOPTCHECK, IDC_XOPTCRC);
				}
				*pl = MAKELONG(Lo,Hi);
			}
			if (DlgXoptFont != NULL) {
				DeleteObject(DlgXoptFont);
			}
			break;
		case IDCANCEL:
			if (DlgXoptFont != NULL) {
				DeleteObject(DlgXoptFont);
			}
			break;
		}
		break;
	case WM_NOTIFY:	// for Explorer-style dialog
		notify = (LPOFNOTIFY)lParam;
		switch (notify->hdr.code) {
		case CDN_FILEOK:
			pl = (LPLONG)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pl!=NULL) {
				if (LOWORD(*pl) == 0xFFFF) { // Send
					Lo = 0xFFFF;

					GetRB(Dialog, &Hi, IDC_XOPT1K, IDC_XOPT1K);
					if (Hi > 0) { // force CRC if 1K
						Hi = Xopt1kCRC;
					}
					else {
						Hi = XoptCRC;
					}
				}
				else { // Recv
					GetRB(Dialog, &Lo, IDC_XOPTBIN, IDC_XOPTBIN);
					GetRB(Dialog, &Hi, IDC_XOPTCHECK, IDC_XOPTCRC);
				}
				*pl = MAKELONG(Lo, Hi);
			}
			if (DlgXoptFont != NULL) {
				DeleteObject(DlgXoptFont);
			}
			break;
		}
		break;
	}
	return FALSE;
}

BOOL WINAPI GetXFname(HWND HWin, BOOL Receive, LPLONG Option, PFileVar fv, PCHAR CurDir)
{
	char uimsg[MAX_UIMSG];
	char *FNFilter;
	OPENFILENAME ofn;
	LONG opt;
	char TempDir[MAXPATHLEN];
	BOOL Ok;

	/* save current dir */
	_getcwd(TempDir,sizeof(TempDir));

	fv->FullName[0] = 0;
	memset(&ofn, 0, sizeof(OPENFILENAME));

	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	if (Receive) {
		get_lang_msg("FILEDLG_TRANS_TITLE_XRCV", uimsg, sizeof(uimsg), TitXRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}
	else {
		get_lang_msg("FILEDLG_TRANS_TITLE_XSEND", uimsg, sizeof(uimsg), TitXSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

	// フィルタがワイルドカードではなく、そのファイルが存在する場合
	// あらかじめデフォルトのファイル名を入れておく (2008.5.18 maya)
	if (!Receive) {
		if (strlen(FileSendFilter) > 0) {
			if (!isInvalidFileNameChar(FileSendFilter)) {
				char file[MAX_PATH];
				strncpy_s(file, sizeof(file), CurDir, _TRUNCATE);
				AppendSlash(file, sizeof(file));
				strncat_s(file, sizeof(file), FileSendFilter, _TRUNCATE);
				if (_access(file, 0) == 0) {
					strncpy_s(fv->FullName, sizeof(fv->FullName), FileSendFilter, _TRUNCATE);
				}
			}
		}
	}

	FNFilter = GetCommonDialogFilterA(Receive == TRUE ? NULL : FileSendFilter, UILanguageFile);

	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = HWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FullName;
	ofn.nMaxFile = sizeof(fv->FullName);
	ofn.lpstrInitialDir = CurDir;
	opt = *Option;
	if (! Receive)
	{
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		opt = opt | 0xFFFF;
	}
	else {
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	}
	ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
	ofn.Flags |= OFN_SHOWHELP;
	ofn.lCustData = (LPARAM)&opt;
	ofn.lpstrTitle = fv->DlgCaption;
	ofn.lpfnHook = XFnHook;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_XOPT);
	ofn.hInstance = hInst;

	if (!Receive)
	{
		Ok = GetOpenFileName(&ofn);
	}
	else {
		Ok = GetSaveFileName(&ofn);
	}
	free(FNFilter);

	if (Ok) {
		fv->DirLen = ofn.nFileOffset;
		fv->FnPtr = ofn.nFileOffset;
		memcpy(CurDir,fv->FullName,fv->DirLen-1);
		CurDir[fv->DirLen-1] = 0;

		if (Receive)
			*Option = opt;
		else
			*Option = MAKELONG(LOWORD(*Option),HIWORD(opt));
	}

	/* restore dir */
	_chdir(TempDir);

	return Ok;
}

void WINAPI ProtoInit(int Proto, PFileVar fv, PCHAR pv, PComVar cv, PTTSet ts)
{
	switch (Proto) {
	case PROTO_KMT:
		KmtInit(fv,(PKmtVar)pv,cv,ts);
		break;
	case PROTO_XM:
		XInit(fv,(PXVar)pv,cv,ts);
		break;
	case PROTO_YM:
		YInit(fv,(PYVar)pv,cv,ts);
		break;
	case PROTO_ZM:
		ZInit(fv,(PZVar)pv,cv,ts);
		break;
	case PROTO_BP:
		BPInit(fv,(PBPVar)pv,cv,ts);
		break;
	case PROTO_QV:
		QVInit(fv,(PQVVar)pv,cv,ts);
		break;
	}
}

BOOL WINAPI ProtoParse(int Proto, PFileVar fv, PCHAR pv, PComVar cv)
{
	BOOL Ok;

	Ok = FALSE;
	switch (Proto) {
	case PROTO_KMT:
		Ok = KmtReadPacket(fv,(PKmtVar)pv,cv);
		break;
	case PROTO_XM:
		switch (((PXVar)pv)->XMode) {
		case IdXReceive:
			Ok = XReadPacket(fv,(PXVar)pv,cv);
			break;
		case IdXSend:
			Ok = XSendPacket(fv,(PXVar)pv,cv);
			break;
		}
		break;
	case PROTO_YM:
		switch (((PYVar)pv)->YMode) {
		case IdYReceive:
			Ok = YReadPacket(fv,(PYVar)pv,cv);
			break;
		case IdYSend:
			Ok = YSendPacket(fv,(PYVar)pv,cv);
			break;
		}
		break;
	case PROTO_ZM:
		Ok = ZParse(fv,(PZVar)pv,cv);
		break;
	case PROTO_BP:
		Ok = BPParse(fv,(PBPVar)pv,cv);
		break;
	case PROTO_QV:
		switch (((PQVVar)pv)->QVMode) {
		case IdQVReceive:
			Ok = QVReadPacket(fv,(PQVVar)pv,cv);
			break;
		case IdQVSend:
			Ok = QVSendPacket(fv,(PQVVar)pv,cv);
			break;
		}
		break;
	}
	return Ok;
}

void WINAPI ProtoTimeOutProc(int Proto, PFileVar fv, PCHAR pv, PComVar cv)
{
	switch (Proto) {
	case PROTO_KMT:
		KmtTimeOutProc(fv,(PKmtVar)pv,cv);
		break;
	case PROTO_XM:
		XTimeOutProc(fv,(PXVar)pv,cv);
		break;
	case PROTO_YM:
		YTimeOutProc(fv,(PYVar)pv,cv);
		break;
	case PROTO_ZM:
		ZTimeOutProc(fv,(PZVar)pv,cv);
		break;
	case PROTO_BP:
		BPTimeOutProc(fv,(PBPVar)pv,cv);
		break;
	case PROTO_QV:
		QVTimeOutProc(fv,(PQVVar)pv,cv);
		break;
	}
}

BOOL WINAPI ProtoCancel(int Proto, PFileVar fv, PCHAR pv, PComVar cv)
{
	switch (Proto) {
	case PROTO_KMT:
		KmtCancel(fv,(PKmtVar)pv,cv);
		break;
	case PROTO_XM:
		XCancel(fv,(PXVar)pv,cv);
		break;
	case PROTO_YM:
		YCancel(fv, (PYVar)pv,cv);
		break;
	case PROTO_ZM:
		ZCancel((PZVar)pv);
		break;
	case PROTO_BP:
		if (((PBPVar)pv)->BPState != BP_Failure) {
			BPCancel((PBPVar)pv);
			return FALSE;
		}
		break;
	case PROTO_QV:
		QVCancel(fv,(PQVVar)pv,cv);
		break;
	}
	return TRUE;
}

void WINAPI TTFILESetUILanguageFile(char *file)
{
	strncpy_s(UILanguageFile, sizeof(UILanguageFile), file, _TRUNCATE);
}

void WINAPI TTFILESetFileSendFilter(char *file)
{
	strncpy_s(FileSendFilter, sizeof(FileSendFilter), file, _TRUNCATE);
}

BOOL WINAPI DllMain(HANDLE hInstance,
                    ULONG ul_reason_for_call,
                    LPVOID lpReserved)
{
	hInst = hInstance;
	switch( ul_reason_for_call ) {
	case DLL_THREAD_ATTACH:
		/* do thread initialization */
		break;
	case DLL_THREAD_DETACH:
		/* do thread cleanup */
		break;
	case DLL_PROCESS_ATTACH:
		/* do process initialization */
		DoCover_IsDebuggerPresent();
		break;
	case DLL_PROCESS_DETACH:
		/* do process cleanup */
		break;
	}
	return TRUE;
}
