/*
 * (C) 2020 TeraTerm Project
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
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "protodlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "ttlib.h"
#include "dlglib.h"
#include "vtterm.h"
#include "ftlib.h"
#include "buffer.h"
#include "helpid.h"
#include "layer_for_unicode.h"
#include "codeconv.h"
#include "asprintf.h"

#include "filesys_log_res.h"

#include "filesys.h"
#include "filesys_proto.h"
#include "ttfile_proto.h"
#include "tt_res.h"
#include "filesys_win32.h"

#include "kermit.h"
#include "xmodem.h"
#include "ymodem.h"
#include "zmodem.h"
#include "bplus.h"
#include "quickvan.h"

static PFileVarProto FileVar = NULL;
static int ProtoId;

extern BOOL FSend;

static PProtoDlg PtDlg = NULL;

static void _SetDlgTime(TFileVarProto *fv, DWORD elapsed, int bytes)
{
	SetDlgTime(fv->HWin, IDC_PROTOELAPSEDTIME, fv->StartTime, fv->ByteCount);
}

static void _SetDlgPaketNum(struct FileVarProto *fv, LONG Num)
{
	SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, Num);
}

static void _SetDlgByteCount(struct FileVarProto *fv, LONG Num)
{
	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, Num);
}

static void _SetDlgPercent(struct FileVarProto *fv, LONG a, LONG b, int *p)
{
	SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS, a, b, p);
}

static void _SetDlgProtoText(struct FileVarProto *fv, const char *text)
{
	SetDlgItemText(fv->HWin, IDC_PROTOPROT, text);
}

static void _SetDlgProtoFileName(struct FileVarProto *fv, const char *text)
{
	// ファイル名(最後のパスセパレータから後ろを表示)
	const char *s = text;
	const char *p = strrchr(text, '\\');
	if (p == NULL) {
		p = strrchr(text, '/');
	}
	if (p != NULL) {
		s = p + 1;
	}
	assert(fv->HWin != NULL);
	SetDlgItemText(fv->HWin, IDC_PROTOFNAME, s);
}

static void _InitDlgProgress(struct FileVarProto *fv, int *CurProgStat)
{
	InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, CurProgStat);
}

static BOOL NewFileVar_(PFileVarProto *pfv)
{
	if (*pfv != NULL) {
		return TRUE;
	}
	TFileVarProto *fv = (TFileVarProto *)malloc(sizeof(TFileVarProto));
	if (fv == NULL)
		return FALSE;
	memset(fv, 0, sizeof(*fv));

	// 受信フォルダ
	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, _countof(FileDirExpanded));
	AppendSlash(FileDirExpanded, _countof(FileDirExpanded));
	fv->RecievePath = _strdup(FileDirExpanded);

	// 受信フォルダを fv->FullName に設定しておく
	// fv->FullName[fv->DirLen] からファイル名を設定するとフルパスになる
	strncpy_s(fv->FullName, sizeof(fv->FullName), FileDirExpanded, _TRUNCATE);
	fv->DirLen = strlen(fv->FullName);

	fv->FileOpen = FALSE;
	fv->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
	fv->HMainWin = HVTWin;
	fv->Success = FALSE;
	fv->NoMsg = FALSE;
	fv->HideDialog = FALSE;

	FilesysCreate(fv);

	fv->InitDlgProgress = _InitDlgProgress;
	fv->SetDlgTime = _SetDlgTime;
	fv->SetDlgPaketNum = _SetDlgPaketNum;
	fv->SetDlgByteCount = _SetDlgByteCount;
	fv->SetDlgPercent = _SetDlgPercent;
	fv->SetDlgProtoText = _SetDlgProtoText;
	fv->SetDlgProtoFileName = _SetDlgProtoFileName;

	*pfv = fv;
	return TRUE;
}

static void FreeFileVar_(PFileVarProto *pfv)
{
	PFileVarProto fv = *pfv;
	if (fv == NULL) {
		return;
	}

	if (fv->Destroy != NULL) {
		fv->Destroy(fv);
	}

	if (fv->FileOpen) {
		fv->Close(fv);
		fv->FileOpen = FALSE;
	}
#if 0
	if (fv->FnStrMem != NULL) {
		free(fv->FnStrMem);
	}
	if (fv->FnStrMemHandle != 0)
	{
		GlobalUnlock(fv->FnStrMemHandle);
		GlobalFree(fv->FnStrMemHandle);
	}
#endif
	if (fv->FileNames != NULL) {
		free(fv->FileNames);
	}
	fv->FileSysDestroy(fv);
	free(fv);

	*pfv = NULL;
}

static BOOL OpenProtoDlg(PFileVarProto fv, int IdProto, int Mode, WORD Opt1, WORD Opt2)
{
	PProtoDlg pd;

	ProtoId = IdProto;

	switch (ProtoId) {
		case PROTO_KMT:
			KmtCreate(fv);
			break;
		case PROTO_XM:
			XCreate(fv);
			break;
		case PROTO_YM:
			YCreate(fv);
			break;
		case PROTO_ZM:
			ZCreate(fv);
			break;
		case PROTO_BP:
			BPCreate(fv);
			break;
		case PROTO_QV:
			QVCreate(fv);
			break;
		default:
			assert(FALSE);
			return FALSE;
			break;
	}

	switch (ProtoId) {
		case PROTO_KMT:
			_ProtoSetOpt(fv, KMT_MODE, Mode);
			break;
		case PROTO_XM:
			_ProtoSetOpt(fv, XMODEM_MODE, Mode);
			_ProtoSetOpt(fv, XMODEM_OPT, Opt1);
			_ProtoSetOpt(fv, XMODEM_TEXT_FLAG, 1 - (Opt2 & 1));
			break;
		case PROTO_YM:
			_ProtoSetOpt(fv, YMODEM_MODE, Mode);
			_ProtoSetOpt(fv, YMODEM_OPT, Opt1);
			break;
		case PROTO_ZM:
			_ProtoSetOpt(fv, ZMODEM_MODE, Mode);
			_ProtoSetOpt(fv, ZMODEM_BINFLAG, (Opt1 & 1) != 0);
			break;
		case PROTO_BP:
			_ProtoSetOpt(fv, BPLUS_MODE, Mode);
			break;
		case PROTO_QV:
			_ProtoSetOpt(fv, QUICKVAN_MODE, Mode);
			break;
	}

	pd = new CProtoDlg();
	if (pd==NULL)
	{
		return FALSE;
	}
	CProtoDlgInfo info;
	info.UILanguageFile = ts.UILanguageFile;
	info.HMainWin = fv->HMainWin;
	pd->Create(hInst, HVTWin, &info);
	fv->HWin = pd->m_hWnd;

	BOOL r = fv->Init(fv, &cv, &ts);
	if (r == FALSE) {
		fv->Destroy(fv);
		return FALSE;
	}
	SetWindowText(fv->HWin, fv->DlgCaption);

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
		{	// Quick-VAN special code
			//if ((ProtoId==PROTO_QV) &&
			//    (((PQVVar)ProtoVar)->QVMode==IdQVSend))
			if (FileVar->OpId == OpQVSend)
				CommTextOut(&cv,"\015",1);
		}
	}
}

static BOOL ProtoStart(void)
{
	if (cv.ProtoFlag)
		return FALSE;
	if (FSend)
	{
		FreeFileVar_(&FileVar);
		return FALSE;
	}

	NewFileVar_(&FileVar);

	if (FileVar==NULL)
	{
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

	FreeFileVar_(&FileVar);
}

/* ダイアログを中央に移動する */
static void CenterCommonDialog(HWND hDlg)
{
	/* hDlgの親がダイアログのウィンドウハンドル */
	HWND hWndDlgRoot = GetParent(hDlg);
	CenterWindow(hWndDlgRoot, GetParent(hWndDlgRoot));
}

static HFONT DlgXoptFont;

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
	const char *UILanguageFile = ts.UILanguageFile;

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

/**
 *	ダイアログのデフォルトファイル名を返す
 *		フィルタ(ts.FileSendFilter)がワイルドカードではなく、
 *		そのファイルが存在する場合
 *		デフォルトのファイル名として返す
 *
 * @param[in]	path		ファイルが存在するか調べるパス
 *							(lpstrInitialDir に設定されるパス)
 * @retval		NULL		デフォルトファイル名なし
 * @retval		NULL以外	デフォルトファイル(不要になったらfree()すること)
 */
static wchar_t *GetCommonDialogDefaultFilenameW(const wchar_t *path)
{
	const char *FileSendFilterA = ts.FileSendFilter;
	if (strlen(FileSendFilterA) == 0) {
		return NULL;
	}

	// フィルタがワイルドカードではなく、そのファイルが存在する場合
	// あらかじめデフォルトのファイル名を入れておく (2008.5.18 maya)
	wchar_t *filename = NULL;
	if (!isInvalidFileNameChar(FileSendFilterA)) {
		wchar_t file[MAX_PATH];
		wcsncpy_s(file, _countof(file), path, _TRUNCATE);
		AppendSlashW(file, _countof(file));
		wchar_t *FileSendFilterW = ToWcharA(FileSendFilterA);
		wcsncat_s(file, _countof(file), FileSendFilterW, _TRUNCATE);
		DWORD attr = _GetFileAttributesW(file);
		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// ファイルが存在する
			filename = _wcsdup(file);
		}
		free(FileSendFilterW);
	}

	return filename;
}

static char *GetCommonDialogDefaultFilenameA(const char *path)
{
	wchar_t *pathW = ToWcharA(path);
	wchar_t *fileW = GetCommonDialogDefaultFilenameW(pathW);
	char *fileA = ToCharW(fileW);
	free(pathW);
	free(fileW);
	return fileA;
}

char **MakeStrArrayFromArray(char **strs)
{
	// 数を数える
	size_t strs_count = 0;
	size_t strs_len = 0;
	while(1) {
		const char *f = strs[strs_count];
		if (f == NULL) {
			break;
		}
		strs_count++;
		size_t len = strlen(f) + 1;	// len = 1 when "\0"
		strs_len += len;
	}

	// 1領域に保存
	size_t ptrs_len = sizeof(char *) * (strs_count + 1);
	char *pool = (char *)malloc(ptrs_len + strs_len);
	char **ptrs = (char **)pool;
	char *strpool = pool + ptrs_len;
	for (int i = 0 ; i < strs_count; i++) {
		size_t len = strlen(strs[i]) + 1;
		memcpy(strpool, strs[i], len);
		ptrs[i] = strpool;
		strpool += len;
	}
	ptrs[strs_count] = NULL;
	return ptrs;
}

char **MakeStrArrayFromStr(const char *str)
{
	const char *strs[2];
	strs[0] = str;
	strs[1] = NULL;
	char **ret = MakeStrArrayFromArray((char **)strs);
	return ret;
}

char **MakeFileArrayMultiSelect(const char *lpstrFile)
{
	// 数を数える
	size_t file_count = 0;
	const char *p = lpstrFile;
	const char *path = p;
	size_t len = strlen(p);
	p += len + 1;
	while(1) {
		len = strlen(p);
		if (len == 0) {
			break;
		}
		p += len + 1;
		file_count++;
	}

	if (file_count == 0) {
		// 1つだけ選択されていた
		return MakeStrArrayFromStr(lpstrFile);
	}

	// パス + ファイル名 一覧作成
	size_t ptr_len = sizeof(char *) * (file_count + 1);
	char **filenames = (char **)malloc(ptr_len);
	len = strlen(path);
	p = path + (len + 1);
	size_t filelen_sum = 0;
	for (int i = 0 ; i < file_count; i++) {
		size_t filelen = asprintf(&filenames[i], "%s\\%s", path, p);
		filelen_sum += filelen + 1;
		len = strlen(p);
		p += len + 1;
	}
	filenames[file_count] = NULL;

	char **ret = MakeStrArrayFromArray(filenames);

	for (int i = 0 ; i < file_count; i++) {
		free(filenames[i]);
	}

	return ret;
}

static char **_GetXFname(HWND HWin, BOOL Receive, const char *caption, LPLONG Option)
{
	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	PCHAR CurDir = FileDirExpanded;

	char *FNFilter = GetCommonDialogFilterA(!Receive ? ts.FileSendFilter : NULL, ts.UILanguageFile);

	char FullName[MAX_PATH];
	FullName[0] = 0;
	if (!Receive) {
		char *default_filename = GetCommonDialogDefaultFilenameA(CurDir);
		if (default_filename != NULL) {
			strncpy_s(FullName, _countof(FullName), default_filename, _TRUNCATE);
			free(default_filename);
		}
	}

	OPENFILENAME ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = HWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FullName;
	ofn.nMaxFile = _countof(FullName);
	ofn.lpstrInitialDir = CurDir;
	LONG opt = *Option;
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
	ofn.lpstrTitle = caption;
	ofn.lpfnHook = XFnHook;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_XOPT);
	ofn.hInstance = hInst;

	/* save current dir */
	wchar_t TempDir[MAX_PATH];
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);
	BOOL Ok;
	if (!Receive)
	{
		Ok = GetOpenFileName(&ofn);
	}
	else {
		Ok = GetSaveFileName(&ofn);
	}
	free(FNFilter);
	_SetCurrentDirectoryW(TempDir);

	char **ret = NULL;
	if (Ok) {
		//fv->DirLen = ofn.nFileOffset;
		//fv->FnPtr = ofn.nFileOffset;

		if (Receive)
			*Option = opt;
		else
			*Option = MAKELONG(LOWORD(*Option),HIWORD(opt));

		ret = MakeStrArrayFromStr(FullName);
	}

	return ret;
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

	if (_ProtoParse(ProtoId,FileVar,&cv))
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
		_ProtoTimeOutProc(ProtoId,FileVar,&cv);
}

void ProtoDlgCancel(void)
{
	if ((PtDlg!=NULL) &&
	    _ProtoCancel(ProtoId,FileVar,&cv))
		ProtoEnd();
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
	PFileVarProto fv;
	char TempFull[MAX_PATH];
	int i, j;
	const char *UILanguageFile = ts.UILanguageFile;

	switch (Message) {
	case WM_INITDIALOG:
		fv = (PFileVarProto)lParam;
		SetWindowLongPtr(Dialog, DWLP_USER, lParam);
		SendDlgItemMessage(Dialog, IDC_GETFN, EM_LIMITTEXT, sizeof(TempFull)-1,0);
		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);
		return TRUE;

	case WM_COMMAND:
		fv = (PFileVarProto)GetWindowLongPtr(Dialog,DWLP_USER);
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
			return TRUE;
		case IDCANCEL:
			EndDialog(Dialog, 0);
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

static BOOL _GetGetFname(HWND HWin, PFileVarProto fv, PTTSet ts)
{
	SetDialogFont(ts->DialogFontName, ts->DialogFontPoint, ts->DialogFontCharSet,
				  ts->UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	return (BOOL)TTDialogBoxParam(hInst,
								  MAKEINTRESOURCE(IDD_GETFNDLG),
								  HWin, GetFnDlg, (LPARAM)fv);
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

/* GetMultiFname function id */
#define GMF_KERMIT 0 /* Kermit Send */
#define GMF_Z  1     /* ZMODEM Send */
#define GMF_QV 2     /* Quick-VAN Send */
#define GMF_Y  3     /* YMODEM Send */


static char **_GetMultiFname(HWND hWnd, WORD FuncId, const char *caption, LPWORD Option)
{
#define FnStrMemSize 4096
	wchar_t TempDir[MAX_PATH];
	const char *FileSendFilter = ts.FileSendFilter;
	const char *UILanguageFile = ts.UILanguageFile;

	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	PCHAR CurDir = FileDirExpanded;

	/* save current dir */
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);

	char *FnStrMem = (char *)malloc(FnStrMemSize);
	if (FnStrMem == NULL) {
		MessageBeep(0);
		return FALSE;
	}
	FnStrMem[0] = 0;

	char *FNFilter = GetCommonDialogFilterA(FileSendFilter, UILanguageFile);

	char *default_filename = GetCommonDialogDefaultFilenameA(CurDir);
	if (default_filename != NULL) {
		strncpy_s(FnStrMem, FnStrMemSize / sizeof(char), default_filename, _TRUNCATE);
		free(default_filename);
	}

	OPENFILENAME ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = hWnd;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FnStrMem;
	ofn.nMaxFile = FnStrMemSize / sizeof(char);
	ofn.lpstrTitle= caption;
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

	BOOL Ok = GetOpenFileName(&ofn);
	free(FNFilter);

	char **ret = NULL;
	if (Ok) {
		// multiple selection
		ret = MakeFileArrayMultiSelect(FnStrMem);
	}
	free(FnStrMem);

	/* restore dir */
	_SetCurrentDirectoryW(TempDir);

	return ret;
}

static void _SetFileVar(PFileVarProto fv)
{
	int i;
	char c;

	GetFileNamePos(fv->FullName,&(fv->DirLen),&i);
	c = fv->FullName[fv->DirLen];
	if (c=='\\'||c=='/') fv->DirLen++;
}

static void KermitStart(int mode)
{
	if (! ProtoStart())
		return;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_KMT,mode,0,0))
		ProtoEnd();
}

/**
 *	Kermit 送信
 *
 *	@param[in]	filename			受信ファイル名(NULLのとき、ダイアログで選択する)
 */
BOOL KermitStartSend(const char *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;
	TFileVarProto *fv = FileVar;

	FileVar->OpId = OpKmtSend;

	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	get_lang_msg("FILEDLG_TRANS_TITLE_KMTSEND", uimsg, sizeof(uimsg), TitKmtSend, UILanguageFile);
	strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

	if (filename == NULL) {
		WORD w = 0;
		char **filenames = _GetMultiFname(fv->HMainWin, GMF_KERMIT, fv->DlgCaption, &w);
		if (filenames == NULL) {
			FreeFileVar_(&FileVar);
			return FALSE;
		}
		fv->FileNames = filenames;
	}
	else {
		fv->FileNames = MakeStrArrayFromStr(filename);
		FileVar->NoMsg = TRUE;
	}
//	GetNextFname(fv);
	KermitStart(IdKmtSend);

	return TRUE;
}

/**
 *	Kermit 受信
 */
BOOL KermitGet(const char *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpKmtSend;

	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	get_lang_msg("FILEDLG_TRANS_TITLE_KMTGET", uimsg, sizeof(uimsg), TitKmtGet, UILanguageFile);
	strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

	if (filename == NULL) {
		if (! _GetGetFname(FileVar->HMainWin,FileVar, &ts) || (strlen(FileVar->FullName)==0)) {
			FreeFileVar_(&FileVar);
			return FALSE;
		}
	}
	else {
		FileVar->DirLen = 0;
		strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
		FileVar->NumFname = 1;
		FileVar->NoMsg = TRUE;
		_SetFileVar(FileVar);
	}
	KermitStart(IdKmtGet);

	return TRUE;
}

/**
 *	Kermit 受信
 */
BOOL KermitStartRecive(BOOL macro)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpKmtRcv;

	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	get_lang_msg("FILEDLG_TRANS_TITLE_KMTRCV", uimsg, sizeof(uimsg), TitKmtRcv, UILanguageFile);
	strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

	if (macro) {
		// マクロから
		FileVar->NoMsg = TRUE;
	}
	KermitStart(IdKmtReceive);

	return TRUE;
}

/**
 *	Kermit finish
 */
BOOL KermitFinish(BOOL macro)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;

	FileVar->OpId = OpKmtFin;

	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	get_lang_msg("FILEDLG_TRANS_TITLE_KMTFIN", uimsg, sizeof(uimsg), TitKmtFin, UILanguageFile);
	strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

	if (macro) {
		FileVar->NoMsg = TRUE;
	}
	KermitStart(IdKmtFinish);

	return TRUE;
}

static void XMODEMStart(int mode)
{
	if (! ProtoStart())
		return;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_XM,mode, ts.XmodemOpt,ts.XmodemBin)) {
		ProtoEnd();
	}
}

/**
 *	XMODEM受信
 *
 *	@param[in]	filename			受信ファイル名(NULLのとき、ダイアログで選択する)
 *	@param[in]	ParamBinaryFlag
 *	@param[in]	ParamXmodemOpt
 */
BOOL XMODEMStartReceive(const char *filename, WORD ParamBinaryFlag, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpXRcv;

	const char *UILanguageFile = ts.UILanguageFile;
	char uimsg[MAX_UIMSG];
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	get_lang_msg("FILEDLG_TRANS_TITLE_XRCV", uimsg, sizeof(uimsg), TitXRcv, UILanguageFile);
	strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

	if (filename == NULL) {
		LONG Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		char **filenames = _GetXFname(HVTWin, TRUE, fv->DlgCaption, &Option);
		if (filenames == NULL) {
			FreeFileVar_(&FileVar);
			return FALSE;
		}
		fv->FileNames = filenames;
		int tmp = HIWORD(Option);
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
		fv->FileNames = MakeStrArrayFromStr(filename);
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
	}
	GetNextFname(fv);
	XMODEMStart(IdXReceive);
	fv->SetDlgProtoFileName(fv, fv->FullName);

	return TRUE;
}

/**
 *	XMODEM送信
 *
 *	@param[in]	filename			送信ファイル名(NULLのとき、ダイアログで選択する)
 *	@param[in]	ParamXmodemOpt
 */
BOOL XMODEMStartSend(const char *filename, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpXSend;

	const char *UILanguageFile = ts.UILanguageFile;
	char uimsg[MAX_UIMSG];
	strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
	get_lang_msg("FILEDLG_TRANS_TITLE_XSEND", uimsg, sizeof(uimsg), TitXSend, UILanguageFile);
	strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

	if (filename == NULL) {
		LONG Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		char **filenames = _GetXFname(HVTWin, FALSE, fv->DlgCaption, &Option);
		if (filenames == NULL) {
			FreeFileVar_(&FileVar);
			return FALSE;
		}
		fv->FileNames = filenames;
		int tmp = HIWORD(Option);
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
	else {
		fv->FileNames = MakeStrArrayFromStr(filename);
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
	}
	GetNextFname(fv);
	XMODEMStart(IdXSend);
	fv->SetDlgProtoFileName(fv, fv->FullName);

	return TRUE;
}

void YMODEMStart(int mode)
{
	WORD Opt;
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdYSend)
	{
		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_YSEND", uimsg, sizeof(uimsg), TitYSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		// ファイル転送時のオプションは"Yopt1K"に決め打ち。
		// TODO: "Yopt1K", "YoptG", "YoptSingle"を区別したいならば、IDD_FOPTを拡張する必要あり。
		Opt = Yopt1K;
		FileVar->OpId = OpYSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! _GetMultiFname(fv->HMainWin, GMF_Y, fv->DlgCaption, &Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			//ts.XmodemBin = Opt;
		}
		else
		_SetFileVar(FileVar);
	}
	else {
		FileVar->OpId = OpYRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_YRCV", uimsg, sizeof(uimsg), TitYRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		// ファイル転送時のオプションは"Yopt1K"に決め打ち。
		Opt = Yopt1K;
		_SetFileVar(FileVar);
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
	if (!NewFileVar_(&FileVar)) {
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
	if (!NewFileVar_(&FileVar)) {
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
	WORD Opt = 0; // TODO 使っていない
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode == IdZSend || mode == IdZAutoS)
	{
		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_ZSEND", uimsg, sizeof(uimsg), TitZSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		Opt = ts.XmodemBin;
		FileVar->OpId = OpZSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char **filenames = _GetMultiFname(fv->HMainWin, GMF_Z, fv->DlgCaption, &Opt);
			if (filenames == NUL) {
				if (mode == IdZAutoS) {
					CommRawOut(&cv, "\030\030\030\030\030\030\030\030\b\b\b\b\b\b\b\b\b\b", 18);
				}
				ProtoEnd();
				return;
			}
			fv->FileNames = filenames;
			GetNextFname(fv);
			ts.XmodemBin = Opt;
		}
		else
		_SetFileVar(FileVar);
	}
	else {
		/* IdZReceive or IdZAutoR */
		FileVar->OpId = OpZRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_ZRCV", uimsg, sizeof(uimsg), TitZRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

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
	if (!NewFileVar_(&FileVar)) {
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
	if (!NewFileVar_(&FileVar)) {
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

static BOOL _GetTransFname(PFileVarProto fv, const char *DlgCaption)
{
	wchar_t TempDir[MAX_PATH];
	char FileName[MAX_PATH];
	const char *UILanguageFile = ts.UILanguageFile;

	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	PCHAR CurDir = FileDirExpanded;

	/* save current dir */
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);

	char *FNFilter = GetCommonDialogFilterA(ts.FileSendFilter, UILanguageFile);

	ExtractFileName(fv->FullName, FileName ,sizeof(FileName));
	strncpy_s(fv->FullName, sizeof(fv->FullName), FileName, _TRUNCATE);

	OPENFILENAME ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = fv->HMainWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FullName;
	ofn.nMaxFile = sizeof(fv->FullName);
	ofn.lpstrInitialDir = CurDir;

	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	ofn.Flags |= OFN_SHOWHELP;

	ofn.lpstrTitle = DlgCaption;

	ofn.hInstance = hInst;

	BOOL Ok = GetOpenFileName(&ofn);
	free(FNFilter);

	if (Ok) {
		fv->DirLen = ofn.nFileOffset;
	}
	/* restore dir */
	_SetCurrentDirectoryW(TempDir);
	return Ok;
}

void BPStart(int mode)
{
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdBPSend)
	{
		FileVar->OpId = OpBPSend;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_BPSEND", uimsg, sizeof(uimsg), TitBPSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			FileVar->FullName[0] = 0;
			if (! _GetTransFname(FileVar, FileVar->DlgCaption))
			{
				ProtoEnd();
				return;
			}
		}
		else
			_SetFileVar(FileVar);
	}
	else {
		/* IdBPReceive or IdBPAuto */
		FileVar->OpId = OpBPRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_BPRCV", uimsg, sizeof(uimsg), TitBPRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

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
	if (!NewFileVar_(&FileVar)) {
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
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	BPStart(IdBPReceive);

	return TRUE;
}

void QVStart(int mode)
{
	WORD W;
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdQVSend)
	{
		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_QVSEND", uimsg, sizeof(uimsg), TitQVSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		FileVar->OpId = OpQVSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! _GetMultiFname(fv->HMainWin, GMF_QV, fv->DlgCaption, &W) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
		}
		else
			_SetFileVar(FileVar);
	}
	else {
		FileVar->OpId = OpQVRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_QVRCV", uimsg, sizeof(uimsg), TitQVRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

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
	if (!NewFileVar_(&FileVar)) {
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
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	QVStart(IdQVSend);

	return TRUE;
}

BOOL IsFileVarNULL()
{
	return FileVar == NULL;
}
