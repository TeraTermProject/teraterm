/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

/* TERATERM.EXE, file transfer routines */
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ftdlg.h"
#include "protodlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "ttlib.h"
#include "dlglib.h"
#include "vtterm.h"
#include "win16api.h"
#include "ftlib.h"
#include "buffer.h"
#include "helpid.h"
#include "layer_for_unicode.h"
#include "layer_for_unicode_crt.h"
#include "codeconv.h"

#include "filesys_log_res.h"

#include "filesys.h"

#define FS_BRACKET_NONE  0
#define FS_BRACKET_START 1
#define FS_BRACKET_END   2

PFileVar SendVar = NULL;
//PFileVar FileVar = NULL;
//static PCHAR ProtoVar = NULL;
//static int ProtoId;

static BOOL FileRetrySend, FileRetryEcho, FileCRSend, FileReadEOF, BinaryMode;
static BYTE FileByte;

#define FILE_SEND_BUF_SIZE  8192
struct FileSendHandler {
	CHAR buf[FILE_SEND_BUF_SIZE];
	int pos;
	int end;
};
static struct FileSendHandler FileSendHandler;
static int FileDlgRefresh;

static int FileBracketMode = FS_BRACKET_NONE;
static int FileBracketPtr = 0;
static char BracketStartStr[] = "\033[200~";
static char BracketEndStr[] = "\033[201~";

BOOL FSend = FALSE;

static HMODULE HTTFILE = NULL;
static int TTFILECount = 0;

#if 0
PGetSetupFname GetSetupFname;
PGetTransFname GetTransFname;
PGetMultiFname GetMultiFname;
PGetGetFname GetGetFname;
PSetFileVar SetFileVar;
PGetXFname GetXFname;
#endif
#if 0
PProtoInit ProtoInit;
PProtoParse ProtoParse;
PProtoTimeOutProc ProtoTimeOutProc;
PProtoCancel ProtoCancel;
PTTFILESetUILanguageFile TTFILESetUILanguageFile;
PTTFILESetFileSendFilter TTFILESetFileSendFilter;
#endif

#if 0
#define IdGetSetupFname  1
#define IdGetTransFname  2
#define IdGetMultiFname  3
#define IdGetGetFname	 4
#define IdSetFileVar	 5
#define IdGetXFname	 6
#endif

#define IdProtoInit	 7
#define IdProtoParse	 8
#define IdProtoTimeOutProc 9
#define IdProtoCancel	 10

#define IdTTFILESetUILanguageFile 11
#define IdTTFILESetFileSendFilter 12

#if 0
BOOL LoadTTFILE(void)
{
	BOOL Err;

	if (HTTFILE != NULL)
	{
		TTFILECount++;
		return TRUE;
	}
	else
		TTFILECount = 0;

	HTTFILE = LoadHomeDLL("TTPFILE.DLL");
	if (HTTFILE == NULL)
		return FALSE;

	Err = FALSE;

#if 0
	GetSetupFname = (PGetSetupFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetSetupFname));
	if (GetSetupFname==NULL)
		Err = TRUE;

	GetTransFname = (PGetTransFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetTransFname));
	if (GetTransFname==NULL)
		Err = TRUE;

	GetMultiFname = (PGetMultiFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetMultiFname));
	if (GetMultiFname==NULL)
		Err = TRUE;

	GetGetFname = (PGetGetFname)GetProcAddress(HTTFILE,
	                                           MAKEINTRESOURCE(IdGetGetFname));
	if (GetGetFname==NULL)
		Err = TRUE;

	SetFileVar = (PSetFileVar)GetProcAddress(HTTFILE,
	                                         MAKEINTRESOURCE(IdSetFileVar));
	if (SetFileVar==NULL)
		Err = TRUE;

	GetXFname = (PGetXFname)GetProcAddress(HTTFILE,
	                                       MAKEINTRESOURCE(IdGetXFname));
	if (GetXFname==NULL)
		Err = TRUE;
#endif

	ProtoInit = (PProtoInit)GetProcAddress(HTTFILE,
	                                       MAKEINTRESOURCE(IdProtoInit));
	if (ProtoInit==NULL)
		Err = TRUE;

	ProtoParse = (PProtoParse)GetProcAddress(HTTFILE,
	                                         MAKEINTRESOURCE(IdProtoParse));
	if (ProtoParse==NULL)
		Err = TRUE;

	ProtoTimeOutProc = (PProtoTimeOutProc)GetProcAddress(HTTFILE,
	                                                     MAKEINTRESOURCE(IdProtoTimeOutProc));
	if (ProtoTimeOutProc==NULL)
		Err = TRUE;

	ProtoCancel = (PProtoCancel)GetProcAddress(HTTFILE,
	                                           MAKEINTRESOURCE(IdProtoCancel));
	if (ProtoCancel==NULL)
		Err = TRUE;

	TTFILESetUILanguageFile = (PTTFILESetUILanguageFile)GetProcAddress(HTTFILE,
	                                                                   MAKEINTRESOURCE(IdTTFILESetUILanguageFile));
	if (TTFILESetUILanguageFile==NULL) {
		Err = TRUE;
	}
	else {
		TTFILESetUILanguageFile(ts.UILanguageFile);
	}

	TTFILESetFileSendFilter = (PTTFILESetFileSendFilter)GetProcAddress(HTTFILE,
	                                                                   MAKEINTRESOURCE(IdTTFILESetFileSendFilter));
	if (TTFILESetFileSendFilter==NULL) {
		Err = TRUE;
	}
	else {
		TTFILESetFileSendFilter(ts.FileSendFilter);
	}

	if (Err)
	{
		FreeLibrary(HTTFILE);
		HTTFILE = NULL;
		return FALSE;
	}
	else {
		TTFILECount = 1;
		return TRUE;
	}
}

BOOL FreeTTFILE(void)
{
	if (TTFILECount==0)
		return FALSE;
	TTFILECount--;
	if (TTFILECount>0)
		return TRUE;
	if (HTTFILE!=NULL)
	{
		FreeLibrary(HTTFILE);
		HTTFILE = NULL;
	}
	return TRUE;
}
#endif

static PFileTransDlg SendDlg = NULL;
//static PProtoDlg PtDlg = NULL;

static BOOL OpenFTDlg(PFileVar fv)
{
	PFileTransDlg FTDlg;

	FTDlg = new CFileTransDlg();

	fv->StartTime = 0;
	fv->ProgStat = 0;
	cv.FilePause &= ~fv->OpId;

	if (FTDlg!=NULL)
	{
		CFileTransDlgInfo info;
		info.UILanguageFile = ts.UILanguageFile;
		info.OpId = OpSendFile;
		info.DlgCaption = ToWcharA(fv->DlgCaption);
		info.FileName = NULL;
		info.FullName = ToWcharA(fv->FullName);
		info.HideDialog = ts.LogHideDialog ? TRUE : FALSE;
		info.HMainWin = HVTWin;
		//FTDlg->Create(hInst, HVTWin, fv, &cv, &ts);
		FTDlg->Create(hInst, &info);
		//FTDlg->RefreshNum(fv);
		FTDlg->RefreshNum(0, fv->FileSize, fv->ByteCount);
		free(info.DlgCaption);
		free(info.FullName);
	}

	SendDlg = FTDlg; /* File send */

	fv->StartTime = GetTickCount();

	return (FTDlg!=NULL);
}

static void ShowFTDlg(WORD OpId)
{
	if (SendDlg != NULL) {
		SendDlg->ShowWindow(SW_SHOWNORMAL);
		SetForegroundWindow(SendDlg->GetSafeHwnd());
	}
}

static BOOL NewFileVar(PFileVar *fv)
{
	if ((*fv)==NULL)
	{
		*fv = (PFileVar)malloc(sizeof(TFileVar));
		if ((*fv)!=NULL)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			memset(*fv, 0, sizeof(TFileVar));
			strncpy_s((*fv)->FullName, sizeof((*fv)->FullName), FileDirExpanded, _TRUNCATE);
			AppendSlash((*fv)->FullName,sizeof((*fv)->FullName));
			(*fv)->DirLen = strlen((*fv)->FullName);
			(*fv)->FileOpen = FALSE;
			(*fv)->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
			(*fv)->HMainWin = HVTWin;
			(*fv)->Success = FALSE;
			(*fv)->NoMsg = FALSE;
			(*fv)->HideDialog = FALSE;
		}
	}

	return ((*fv)!=NULL);
}

static void FreeFileVar(PFileVar *fv)
{
	if ((*fv)!=NULL)
	{
		if ((*fv)->FileOpen) CloseHandle((*fv)->FileHandle);
		if ((*fv)->FnStrMemHandle != 0)
		{
			GlobalUnlock((*fv)->FnStrMemHandle);
			GlobalFree((*fv)->FnStrMemHandle);
		}
		free(*fv);
		*fv = NULL;
	}
}

/* ダイアログを中央に移動する */
static void CenterCommonDialog(HWND hDlg)
{
	/* hDlgの親がダイアログのウィンドウハンドル */
	HWND hWndDlgRoot = GetParent(hDlg);
	CenterWindow(hWndDlgRoot, GetParent(hWndDlgRoot));
}

static HFONT DlgFoptFont;
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
	const char *UILanguageFile = ts.UILanguageFile;

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

static BOOL _GetTransFname(PFileVar fv, PCHAR CurDir, WORD FuncId, LPLONG Option)
{
	char uimsg[MAX_UIMSG];
	char *FNFilter;
	OPENFILENAME ofn;
	WORD optw;
	wchar_t TempDir[MAXPATHLEN];
	BOOL Ok;
	char FileName[MAX_PATH];
	const char *UILanguageFile = ts.UILanguageFile;

	/* save current dir */
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);

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

	FNFilter = GetCommonDialogFilterA(ts.FileSendFilter, UILanguageFile);

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
	_SetCurrentDirectoryW(TempDir);
	return Ok;
}

static void _SetFileVar(PFileVar fv)
{
	int i;
	char uimsg[MAX_UIMSG];
	char c;
	const char *UILanguageFile = ts.UILanguageFile;

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

void FileSendStart(void)
{
	LONG Option = 0;

	if (! cv.Ready || FSend) return;
	if (cv.ProtoFlag)
	{
		FreeFileVar(&SendVar);
		return;
	}

#if 0
	if (! LoadTTFILE())
		return;
#endif
	if (! NewFileVar(&SendVar))
	{
#if 0
		FreeTTFILE();
#endif
		return;
	}
	SendVar->OpId = OpSendFile;

	FSend = TRUE;

	if (strlen(&(SendVar->FullName[SendVar->DirLen])) == 0) {
		char FileDirExpanded[MAX_PATH];
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
		if (ts.TransBin)
			Option |= LOGDLG_BINARY;
		SendVar->FullName[0] = 0;
		if (! _GetTransFname(SendVar, FileDirExpanded, GTF_SEND, &Option)) {
			FileTransEnd(OpSendFile);
			return;
		}
		ts.TransBin = CheckFlag(Option, LOGDLG_BINARY);
	}
	else
		_SetFileVar(SendVar);

	SendVar->FileHandle = CreateFile(SendVar->FullName, GENERIC_READ, FILE_SHARE_READ, NULL,
	                                 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SendVar->FileOpen = (SendVar->FileHandle != INVALID_HANDLE_VALUE);
	if (! SendVar->FileOpen)
	{
		FileTransEnd(OpSendFile);
		return;
	}
	SendVar->ByteCount = 0;
	SendVar->FileSize = GetFSize(SendVar->FullName);

	TalkStatus = IdTalkFile;
	FileRetrySend = FALSE;
	FileRetryEcho = FALSE;
	FileCRSend = FALSE;
	FileReadEOF = FALSE;
	FileSendHandler.pos = 0;
	FileSendHandler.end = 0;
	FileDlgRefresh = 0;

	if (BracketedPasteMode()) {
		FileBracketMode = FS_BRACKET_START;
		FileBracketPtr = 0;
		BinaryMode = TRUE;
	}
	else {
		FileBracketMode = FS_BRACKET_NONE;
		BinaryMode = ts.TransBin;
	}

	if (! OpenFTDlg(SendVar))
		FileTransEnd(OpSendFile);
}

BOOL FileSendStart2(const char *filename, int binary)
{
	if (SendVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&SendVar)) {
		return FALSE;
	}

	SendVar->DirLen = 0;
	strncpy_s(SendVar->FullName, sizeof(SendVar->FullName), filename, _TRUNCATE);
	ts.TransBin = binary;
	SendVar->NoMsg = TRUE;
	FileSendStart();

	return TRUE;
}

void FileTransEnd(WORD OpId)
/* OpId = 0: close Log and FileSend
      OpLog: close Log
 OpSendFile: close FileSend */
{
	if ((OpId==0) || (OpId==OpLog)) {
		if (FLogIsOpend()) {
			FLogClose();
		}
	}

	if (((OpId==0) || (OpId==OpSendFile)) && FSend)
	{
		FSend = FALSE;
		TalkStatus = IdTalkKeyb;
		if (SendDlg!=NULL)
		{
			SendDlg->DestroyWindow();
			SendDlg = NULL;
		}
		FreeFileVar(&SendVar);
#if 0
		FreeTTFILE();
#endif
	}

	EndDdeCmnd(0);
}

void FileTransPause(WORD OpId, BOOL Pause)
{
	if (Pause) {
		cv.FilePause |= OpId;
	}
	else {
		cv.FilePause &= ~OpId;
	}
}

int FSOut1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryOut(&cv,(PCHAR)&b,1);
	else if ((b>=0x20) || (b==0x09) || (b==0x0A) || (b==0x0D))
		return CommTextOut(&cv,(PCHAR)&b,1);
	else
		return 1;
}

int FSEcho1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryEcho(&cv,(PCHAR)&b,1);
	else
		return CommTextEcho(&cv,(PCHAR)&b,1);
}

// 以下の時はこちらの関数を使う
// - BinaryMode == true
// - FileBracketMode == false
// - cv.TelFlag == false
// - ts.LocalEcho == 0
static void FileSendBinayBoost(void)
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;

	if ((SendDlg == NULL) ||
		((cv.FilePause & OpSendFile) != 0))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
			FileSendHandler.end - FileSendHandler.pos);
		FileSendHandler.pos += c;
		FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
		if (FileRetrySend)
			return;
	}

	do {
		if (FileSendHandler.pos == FileSendHandler.end) {
			ReadFile(SendVar->FileHandle, &(FileSendHandler.buf[0]), sizeof(FileSendHandler.buf), &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			FileSendHandler.pos = 0;
			FileSendHandler.end = fc;
		} else {
			fc = FileSendHandler.end - FileSendHandler.end;
		}

		if (fc != 0)
		{
			c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
				FileSendHandler.end - FileSendHandler.pos);
			FileSendHandler.pos += c;
			FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
			SendVar->ByteCount = SendVar->ByteCount + c;
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld) {
					//SendDlg->RefreshNum(SendVar);
					SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
				}
				return;
			}
		}
		FileDlgRefresh = SendVar->ByteCount;
		//SendDlg->RefreshNum(SendVar);
		SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
		BCOld = SendVar->ByteCount;
		if (fc != 0)
			return;
	} while (fc != 0);

	FileTransEnd(OpSendFile);
}

void FileSend(void)
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;

	if (cv.PortType == IdSerial && ts.FileSendHighSpeedMode &&
	    BinaryMode && !FileBracketMode && !cv.TelFlag &&
	    (ts.LocalEcho == 0) && (ts.Baud >= 115200)) {
		return FileSendBinayBoost();
	}

	if ((SendDlg==NULL) ||
	    ((cv.FilePause & OpSendFile) !=0))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		FileRetryEcho = (ts.LocalEcho>0);
		c = FSOut1(FileByte);
		FileRetrySend = (c==0);
		if (FileRetrySend)
			return;
	}

	if (FileRetryEcho)
	{
		c = FSEcho1(FileByte);
		FileRetryEcho = (c==0);
		if (FileRetryEcho)
			return;
	}

	do {
		if (FileBracketMode == FS_BRACKET_START) {
			FileByte = BracketStartStr[FileBracketPtr++];
			fc = 1;

			if (FileBracketPtr >= sizeof(BracketStartStr) - 1) {
				FileBracketMode = FS_BRACKET_END;
				FileBracketPtr = 0;
				BinaryMode = ts.TransBin;
			}
		}
		else if (! FileReadEOF) {
			ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			SendVar->ByteCount = SendVar->ByteCount + fc;

			if (FileCRSend && (fc==1) && (FileByte==0x0A)) {
				ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
				fc = LOWORD(read_bytes);
				SendVar->ByteCount = SendVar->ByteCount + fc;
			}
		}
		else {
			fc = 0;
		}

		if (fc == 0 && FileBracketMode == FS_BRACKET_END) {
			FileReadEOF = TRUE;
			FileByte = BracketEndStr[FileBracketPtr++];
			fc = 1;
			BinaryMode = TRUE;

			if (FileBracketPtr >= sizeof(BracketEndStr) - 1) {
				FileBracketMode = FS_BRACKET_NONE;
				FileBracketPtr = 0;
			}
		}


		if (fc!=0)
		{
			c = FSOut1(FileByte);
			FileCRSend = (ts.TransBin==0) && (FileByte==0x0D);
			FileRetrySend = (c==0);
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld) {
					//SendDlg->RefreshNum(SendVar);
					SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
				}
				return;
			}
			if (ts.LocalEcho>0)
			{
				c = FSEcho1(FileByte);
				FileRetryEcho = (c==0);
				if (FileRetryEcho)
					return;
			}
		}
		if ((fc==0) || ((SendVar->ByteCount % 100 == 0) && (FileBracketPtr == 0))) {
			//SendDlg->RefreshNum(SendVar);
			SendDlg->RefreshNum(SendVar->StartTime, SendVar->FileSize, SendVar->ByteCount);
			BCOld = SendVar->ByteCount;
			if (fc!=0)
				return;
		}
	} while (fc!=0);

	FileTransEnd(OpSendFile);
}

#if 0
static BOOL OpenProtoDlg(PFileVar fv, int IdProto, int Mode, WORD Opt1, WORD Opt2)
{
	int vsize;
	PProtoDlg pd;

	ProtoId = IdProto;

	switch (ProtoId) {
		case PROTO_KMT:
			vsize = sizeof(TKmtVar);
			break;
		case PROTO_XM:
			vsize = sizeof(TXVar);
			break;
		case PROTO_YM:
			vsize = sizeof(TYVar);
			break;
		case PROTO_ZM:
			vsize = sizeof(TZVar);
			break;
		case PROTO_BP:
			vsize = sizeof(TBPVar);
			break;
		case PROTO_QV:
			vsize = sizeof(TQVVar);
			break;
		default:
			vsize = 0;
			assert(FALSE);
			break;
	}
	ProtoVar = (PCHAR)malloc(vsize);
	if (ProtoVar==NULL)
		return FALSE;

	switch (ProtoId) {
		case PROTO_KMT:
			((PKmtVar)ProtoVar)->KmtMode = Mode;
			break;
		case PROTO_XM:
			((PXVar)ProtoVar)->XMode = Mode;
			((PXVar)ProtoVar)->XOpt = Opt1;
			((PXVar)ProtoVar)->TextFlag = 1 - (Opt2 & 1);
			break;
		case PROTO_YM:
			((PYVar)ProtoVar)->YMode = Mode;
			((PYVar)ProtoVar)->YOpt = Opt1;
			break;
		case PROTO_ZM:
			((PZVar)ProtoVar)->BinFlag = (Opt1 & 1) != 0;
			((PZVar)ProtoVar)->ZMode = Mode;
			break;
		case PROTO_BP:
			((PBPVar)ProtoVar)->BPMode = Mode;
			break;
		case PROTO_QV:
			((PQVVar)ProtoVar)->QVMode = Mode;
			break;
	}

	pd = new CProtoDlg();
	if (pd==NULL)
	{
		free(ProtoVar);
		ProtoVar = NULL;
		return FALSE;
	}
	CProtoDlgInfo info;
	info.UILanguageFile = ts.UILanguageFile;
	info.HMainWin = fv->HMainWin;
	pd->Create(hInst, HVTWin, &info);
	fv->HWin = pd->m_hWnd;

	(*ProtoInit)(ProtoId,FileVar,ProtoVar,&cv,&ts);

	PtDlg = pd;
	return TRUE;
}

static void CloseProtoDlg(void)
{
	if (PtDlg!=NULL)
	{
		PtDlg->DestroyWindow();
		PtDlg = NULL;

		::KillTimer(FileVar->HMainWin,IdProtoTimer);
		if ((ProtoId==PROTO_QV) &&
		    (((PQVVar)ProtoVar)->QVMode==IdQVSend))
			CommTextOut(&cv,"\015",1);
		if (FileVar->LogFlag)
			CloseHandle(FileVar->LogFile);
		FileVar->LogFile = 0;
		if (ProtoVar!=NULL)
		{
			free(ProtoVar);
			ProtoVar = NULL;
		}
	}
}

static BOOL ProtoStart(void)
{
	if (cv.ProtoFlag)
		return FALSE;
	if (FSend)
	{
		FreeFileVar(&FileVar);
		return FALSE;
	}

	if (! LoadTTFILE())
		return FALSE;
	NewFileVar(&FileVar);

	if (FileVar==NULL)
	{
		FreeTTFILE();
		return FALSE;
	}
	cv.ProtoFlag = TRUE;
	return TRUE;
}

void ProtoEnd(void)
{
	if (! cv.ProtoFlag)
		return;
	cv.ProtoFlag = FALSE;

	/* Enable transmit delay (serial port) */
	cv.DelayFlag = TRUE;
	TalkStatus = IdTalkKeyb;

	CloseProtoDlg();

	if ((FileVar!=NULL) && FileVar->Success)
		EndDdeCmnd(1);
	else
		EndDdeCmnd(0);

	FreeTTFILE();
	FreeFileVar(&FileVar);
}

/**
 *	OnIdle()#teraterm.cppからコールされる
 *		cv.ProtoFlag が 0 以外のとき
 *	@retval		0		continue
 *				1/2		ActiveWin(グローバル変数)の値(IdVT=1/IdTek=2)
 *				注 今のところ捨てられている
 */
int ProtoDlgParse(void)
{
	int P;

	P = ActiveWin;
	if (PtDlg==NULL)
		return P;

	if ((*ProtoParse)(ProtoId,FileVar,ProtoVar,&cv))
		P = 0; /* continue */
	else {
		CommSend(&cv);
		ProtoEnd();
	}
	return P;
}

void ProtoDlgTimeOut(void)
{
	if (PtDlg!=NULL)
		(*ProtoTimeOutProc)(ProtoId,FileVar,ProtoVar,&cv);
}

void ProtoDlgCancel(void)
{
	if ((PtDlg!=NULL) &&
	    (*ProtoCancel)(ProtoId,FileVar,ProtoVar,&cv))
		ProtoEnd();
}

void KermitStart(int mode)
{
	WORD w;

	if (! ProtoStart())
		return;

	switch (mode) {
		case IdKmtSend:
			FileVar->OpId = OpKmtSend;
			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				char FileDirExpanded[MAX_PATH];
				ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
				if (!(*GetMultiFname)(FileVar, FileDirExpanded, GMF_KERMIT, &w) ||
				    (FileVar->NumFname==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				(*SetFileVar)(FileVar);
			break;
		case IdKmtReceive:
			FileVar->OpId = OpKmtRcv;
			break;
		case IdKmtGet:
			FileVar->OpId = OpKmtSend;
			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				if (! (*GetGetFname)(FileVar->HMainWin,FileVar, &ts) ||
				    (strlen(FileVar->FullName)==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				(*SetFileVar)(FileVar);
			break;
		case IdKmtFinish:
			FileVar->OpId = OpKmtFin;
			break;
		default:
			ProtoEnd();
			return;
	}
	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_KMT,mode,0,0))
		ProtoEnd();
}

BOOL KermitStartSend(const char *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtSend);

	return TRUE;
}

BOOL KermitGet(const char *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtGet);

	return TRUE;
}

BOOL KermitStartRecive(void)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtReceive);

	return TRUE;
}

BOOL KermitFinish(void)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtFinish);

	return TRUE;
}

void XMODEMStart(int mode)
{
	LONG Option;
	int tmp;

	if (! ProtoStart())
		return;

	if (mode==IdXReceive)
		FileVar->OpId = OpXRcv;
	else
		FileVar->OpId = OpXSend;

	if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
	{
		char FileDirExpanded[MAX_PATH];
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
		Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		if (! (*GetXFname)(FileVar->HMainWin,
		                   mode==IdXReceive,&Option,FileVar,FileDirExpanded))
		{
			ProtoEnd();
			return;
		}
		tmp = HIWORD(Option);
		if (mode == IdXReceive) {
			if (IsXoptCRC(tmp)) {
				if (IsXopt1k(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCRC;
				}
				else {
					ts.XmodemOpt = XoptCRC;
				}
			}
			else {
				if (IsXopt1k(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCksum;
				}
				else {
					ts.XmodemOpt = XoptCheck;
				}
			}
			ts.XmodemBin = LOWORD(Option);
		}
		else {
			if (IsXopt1k(tmp)) {
				if (IsXoptCRC(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCRC;
				}
				else {
					ts.XmodemOpt = Xopt1kCksum;
				}
			}
			else {
				if (IsXoptCRC(ts.XmodemOpt)) {
					ts.XmodemOpt = XoptCRC;
				}
				else {
					ts.XmodemOpt = XoptCheck;
				}
			}
		}
	}
	else
		(*SetFileVar)(FileVar);

	if (mode==IdXReceive)
		FileVar->FileHandle = _lcreat(FileVar->FullName,0);
	else
		FileVar->FileHandle = _lopen(FileVar->FullName,OF_READ);

	FileVar->FileOpen = FileVar->FileHandle != INVALID_HANDLE_VALUE;
	if (! FileVar->FileOpen)
	{
		ProtoEnd();
		return;
	}
	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_XM,mode,
	                   ts.XmodemOpt,ts.XmodemBin))
		ProtoEnd();
}

BOOL XMODEMStartReceive(const char *fiename, WORD ParamBinaryFlag, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),fiename, _TRUNCATE);
	if (IsXopt1k(ts.XmodemOpt)) {
		if (IsXoptCRC(ParamXmodemOpt)) {
			// CRC
			ts.XmodemOpt = Xopt1kCRC;
		}
		else {	// Checksum
			ts.XmodemOpt = Xopt1kCksum;
		}
	}
	else {
		if (IsXoptCRC(ParamXmodemOpt)) {
			ts.XmodemOpt = XoptCRC;
		}
		else {
			ts.XmodemOpt = XoptCheck;
		}
	}
	ts.XmodemBin = ParamBinaryFlag;
	FileVar->NoMsg = TRUE;
	XMODEMStart(IdXReceive);

	return TRUE;
}

BOOL XMODEMStartSend(const char *fiename, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName), fiename, _TRUNCATE);
	if (IsXoptCRC(ts.XmodemOpt)) {
		if (IsXopt1k(ParamXmodemOpt)) {
			ts.XmodemOpt = Xopt1kCRC;
		}
		else {
			ts.XmodemOpt = XoptCRC;
		}
	}
	else {
		if (IsXopt1k(ParamXmodemOpt)) {
			ts.XmodemOpt = Xopt1kCksum;
		}
		else {
			ts.XmodemOpt = XoptCheck;
		}
	}
	FileVar->NoMsg = TRUE;
	XMODEMStart(IdXSend);

	return TRUE;
}

void YMODEMStart(int mode)
{
	WORD Opt;

	if (! ProtoStart())
		return;

	if (mode==IdYSend)
	{
		char FileDirExpanded[MAX_PATH];
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));

		// ファイル転送時のオプションは"Yopt1K"に決め打ち。
		// TODO: "Yopt1K", "YoptG", "YoptSingle"を区別したいならば、IDD_FOPTを拡張する必要あり。
		Opt = Yopt1K;
		FileVar->OpId = OpYSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,FileDirExpanded,GMF_Y,&Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			//ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else {
		FileVar->OpId = OpYRcv;
		// ファイル転送時のオプションは"Yopt1K"に決め打ち。
		Opt = Yopt1K;
		(*SetFileVar)(FileVar);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_YM,mode,Opt,0))
		ProtoEnd();
}

BOOL YMODEMStartReceive()
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}
	FileVar->NoMsg = TRUE;
	YMODEMStart(IdYReceive);
	return TRUE;
}

BOOL YMODEMStartSend(const char *fiename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),fiename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	YMODEMStart(IdYSend);
	return TRUE;
}

void ZMODEMStart(int mode)
{
	WORD Opt;

	if (! ProtoStart())
		return;

	if (mode == IdZSend || mode == IdZAutoS)
	{
		Opt = ts.XmodemBin;
		FileVar->OpId = OpZSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			if (! (*GetMultiFname)(FileVar,FileDirExpanded,GMF_Z,&Opt) ||
			    (FileVar->NumFname==0))
			{
				if (mode == IdZAutoS) {
					CommRawOut(&cv, "\030\030\030\030\030\030\030\030\b\b\b\b\b\b\b\b\b\b", 18);
				}
				ProtoEnd();
				return;
			}
			ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else /* IdZReceive or IdZAutoR */
		FileVar->OpId = OpZRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_ZM,mode,Opt,0))
		ProtoEnd();
}

BOOL ZMODEMStartReceive(void)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}

	FileVar->NoMsg = TRUE;
	ZMODEMStart(IdZReceive);

	return TRUE;
}

BOOL ZMODEMStartSend(const char *fiename, WORD ParamBinaryFlag)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),fiename, _TRUNCATE);
	FileVar->NumFname = 1;
	ts.XmodemBin = ParamBinaryFlag;
	FileVar->NoMsg = TRUE;

	ZMODEMStart(IdZSend);

	return TRUE;
}

void BPStart(int mode)
{
	LONG Option = 0;

	if (! ProtoStart())
		return;
	if (mode==IdBPSend)
	{
		FileVar->OpId = OpBPSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			FileVar->FullName[0] = 0;
			if (! (*GetTransFname)(FileVar, FileDirExpanded, GTF_BP, &Option))
			{
				ProtoEnd();
				return;
			}
		}
		else
			(*SetFileVar)(FileVar);
	}
	else /* IdBPReceive or IdBPAuto */
		FileVar->OpId = OpBPRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_BP,mode,0,0))
		ProtoEnd();
}

BOOL BPSendStart(const char *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName), filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	BPStart(IdBPSend);

	return TRUE;
}

BOOL BPStartReceive(void)
{
	if (FileVar != NULL)
		return FALSE;
	if (!NewFileVar(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	BPStart(IdBPReceive);

	return TRUE;
}

void QVStart(int mode)
{
	WORD W;

	if (! ProtoStart())
		return;

	if (mode==IdQVSend)
	{
		FileVar->OpId = OpQVSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			if (! (*GetMultiFname)(FileVar,FileDirExpanded,GMF_QV, &W) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
		}
		else
			(*SetFileVar)(FileVar);
	}
	else
		FileVar->OpId = OpQVRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_QV,mode,0,0))
		ProtoEnd();
}

BOOL QVStartReceive(void)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}

	FileVar->NoMsg = TRUE;
	QVStart(IdQVReceive);

	return TRUE;
}

BOOL QVStartSend(const char *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	QVStart(IdQVSend);

	return TRUE;
}
#endif

BOOL IsSendVarNULL()
{
	return SendVar == NULL;
}

#if 0
BOOL IsFileVarNULL()
{
	return FileVar == NULL;
}
#endif
