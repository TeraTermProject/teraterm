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
#include <assert.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "tttypes.h"
#include "protodlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "ttlib.h"
#include "ttlib_types.h"
#include "dlglib.h"
#include "codeconv.h"
#include "asprintf.h"

#include "filesys_log_res.h"

#include "filesys.h"
#include "filesys_proto.h"
#include "tt_res.h"
#include "filesys_win32.h"

#include "kermit.h"
#include "xmodem.h"
#include "ymodem.h"
#include "zmodem.h"
#include "bplus.h"
#include "quickvan.h"

#define OpKmtRcv   3
#define OpKmtGet   4
#define OpKmtSend  5
#define OpKmtFin   6
#define OpXRcv     7
#define OpXSend    8
#define OpZRcv     9
#define OpZSend    10
#define OpBPRcv    11
#define OpBPSend   12
#define OpQVRcv    13
#define OpQVSend   14
#define OpYRcv     15
#define OpYSend    16

#define PROTO_KMT 1
#define PROTO_XM  2
#define PROTO_ZM  3
#define PROTO_BP  4
#define PROTO_QV  5
#define PROTO_YM  6

#define TitKmtRcv   L"Kermit Receive"
#define TitKmtGet   L"Kermit Get"
#define TitKmtSend  L"Kermit Send"
#define TitKmtFin   L"Kermit Finish"
#define TitXRcv     L"XMODEM Receive"
#define TitXSend    L"XMODEM Send"
#define TitYRcv     L"YMODEM Receive"
#define TitYSend    L"YMODEM Send"
#define TitZRcv     L"ZMODEM Receive"
#define TitZSend    L"ZMODEM Send"
#define TitQVRcv    L"Quick-VAN Receive"
#define TitQVSend   L"Quick-VAN Send"

static PFileVarProto FileVar = NULL;
static int ProtoId;
static PProtoDlg PtDlg = NULL;
static BOOL cv_ProtoFlag = FALSE;

static void _SetDlgTime(TFileVarProto *fv, DWORD elapsed, int bytes)
{
	SetDlgTime(fv->HWin, IDC_PROTOELAPSEDTIME, elapsed, bytes);
}

static void _SetDlgPacketNum(struct FileVarProto *fv, LONG Num)
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

static void _SetDlgProtoFileName(struct FileVarProto *fv, const char *filename)
{
	if (filename == NULL) {
		SetDlgItemTextA(fv->HWin, IDC_PROTOFNAME, "");
		return;
	}
	// �t�@�C����(�Ō�̃p�X�Z�p���[�^�������\��)
	const char *s = filename;
	const char *p = strrchr(filename, '\\');
	if (p == NULL) {
		p = strrchr(filename, '/');
	}
	if (p != NULL) {
		s = p + 1;
	}
	assert(fv->HWin != NULL);
	SetDlgItemTextW(fv->HWin, IDC_PROTOFNAME, wc::fromUtf8(s));
}

static void _InitDlgProgress(struct FileVarProto *fv, int *CurProgStat)
{
	InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, CurProgStat);
}

/**
 *	�t�@�C�������擾
 *
 *	@return		�t�@�C���� UTF-8
 *				NULL�̂Ƃ����̃t�@�C���͂Ȃ�
 *				�s�v�ɂȂ����� free() ���邱��
 */
static char *GetNextFname(PFileVarProto fv)
{
	wchar_t *f = fv->FileNames[fv->FNCount];
	if (f == NULL) {
		/* no more file name */
		return NULL;
	}
	fv->FNCount++;
	char *u8 = ToU8W(f);
	return u8;
}

/**
 *	�_�E�����[�h�p�X���擾
 *
 *	@return		��M�t�H���_(�I�[�Ƀp�X�Z�p���[�^'\\'���t������Ă���)
 *				�s�v�ɂȂ����� free() ���邱��
 */
static char *GetRecievePath(struct FileVarProto *fv)
{
	return ToU8W(fv->RecievePath);
}

static void FTSetTimeOut(PFileVarProto fv, int T)
{
	KillTimer(fv->HMainWin, IdProtoTimer);
	if (T==0) return;
	SetTimer(fv->HMainWin, IdProtoTimer, T*1000, NULL);
}

static void SetDialogCation(struct FileVarProto *fv, const char *key, const wchar_t *default_caption)
{
	const wchar_t *UILanguageFile = ts.UILanguageFileW;
	wchar_t *uimsg;
	wchar_t *caption;
	GetI18nStrWW("Tera Term", key, default_caption, UILanguageFile, &uimsg);
	aswprintf(&caption, L"Tera Term: %s", uimsg);
	free(uimsg);
	free((void *)fv->DlgCaption);
	fv->DlgCaption = caption;
}

static const TInfoOp InfoOp = {
	_InitDlgProgress,
	_SetDlgTime,
	_SetDlgPacketNum,
	_SetDlgByteCount,
	_SetDlgPercent,
	_SetDlgProtoText,
	_SetDlgProtoFileName,
};

static BOOL NewFileVar_(PFileVarProto *pfv)
{
	if (*pfv != NULL) {
		return TRUE;
	}
	TFileVarProto *fv = (TFileVarProto *)malloc(sizeof(TFileVarProto));
	if (fv == NULL)
		return FALSE;
	memset(fv, 0, sizeof(*fv));

	// ��M�t�H���_
	{
		wchar_t *dir = GetDownloadDir(&ts);
		size_t len = wcslen(dir) + 2;
		dir = (wchar_t *)realloc(dir, sizeof(wchar_t) * len);
		AppendSlashW(dir, len);
		fv->RecievePath = dir;
	}

	fv->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
	fv->HMainWin = HVTWin;
	fv->Success = FALSE;
	fv->NoMsg = FALSE;

	fv->file = FilesysCreateWin32();

	fv->GetNextFname = GetNextFname;
	fv->GetRecievePath = GetRecievePath;
	fv->FTSetTimeOut = FTSetTimeOut;
	fv->SetDialogCation = SetDialogCation;

	fv->InfoOp = &InfoOp;

	*pfv = fv;
	return TRUE;
}

static void FreeFileVar_(PFileVarProto *pfv)
{
	PFileVarProto fv = *pfv;
	if (fv == NULL) {
		return;
	}

	if (fv->ProtoOp != NULL) {
		fv->ProtoOp->Destroy(fv);
	}

	if (fv->FileNames != NULL) {
		free(fv->FileNames);
	}
	fv->file->FileSysDestroy(fv->file);
	free(fv->DlgCaption);
	fv->DlgCaption = NULL;
	free(fv);

	*pfv = NULL;
}

static int _ProtoSetOpt(PFileVarProto fv, int request, ...)
{
	va_list ap;
	va_start(ap, request);
	int r = fv->ProtoOp->SetOptV(fv, request, ap);
	va_end(ap);
	return r;
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
	info.UILanguageFileW = ts.UILanguageFileW;
	info.HMainWin = fv->HMainWin;
	pd->Create(hInst, HVTWin, &info);
	fv->HWin = pd->m_hWnd;
	PtDlg = pd;

	BOOL r = fv->ProtoOp->Init(fv, &cv, &ts);
	if (r == FALSE) {
		//fv->Destroy(fv);
		return FALSE;
	}
	SetWindowTextW(fv->HWin, fv->DlgCaption);

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
				CommBinaryOut(&cv,"\015",1);
		}
	}
}

static BOOL ProtoStart(void)
{
	if (cv_ProtoFlag)
		return FALSE;
	if (FileSnedIsSending())
	{	// �t�@�C�����M��
		FreeFileVar_(&FileVar);
		return FALSE;
	}

	NewFileVar_(&FileVar);

	if (FileVar==NULL)
	{
		return FALSE;
	}
	cv_ProtoFlag = TRUE;
	return TRUE;
}

void ProtoEnd(void)
{
	if (! cv_ProtoFlag)
		return;
	cv_ProtoFlag = FALSE;

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

/* �_�C�A���O�𒆉��Ɉړ����� */
static void CenterCommonDialog(HWND hDlg)
{
	/* hDlg�̐e���_�C�A���O�̃E�B���h�E�n���h�� */
	HWND hWndDlgRoot = GetParent(hDlg);
	CenterWindow(hWndDlgRoot, GetParent(hWndDlgRoot));
}

static int get_lang_font(const wchar_t *key, HWND dlg, HFONT *font, const wchar_t *iniFile)
{
	*font = NULL;
	int dpi = GetMonitorDpiFromWindow(dlg);
	LOGFONTW logfont;
	if (GetI18nLogfontW(L"Tera Term", key, &logfont, dpi, iniFile) == FALSE) {
		return FALSE;
	}

	if ((*font = CreateFontIndirectW(&logfont)) == NULL) {
		return FALSE;
	}

	return TRUE;
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
	WORD Hi, Lo;
	LPLONG pl;
	LPOFNOTIFY notify;
	const wchar_t *UILanguageFileW = ts.UILanguageFileW;
	static HFONT DlgXoptFont;

	switch (Message) {
	case WM_INITDIALOG: {
		LPOPENFILENAMEW ofn = (LPOPENFILENAMEW)lParam;

		pl = (LPLONG)ofn->lCustData;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)pl);

		if (get_lang_font(L"DLG_TAHOMA_FONT", Dialog, &DlgXoptFont, UILanguageFileW)) {
			SendDlgItemMessage(Dialog, IDC_XOPT, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTCHECK, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTCRC, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPT1K, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTBIN, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgXoptFont = NULL;
		}

		SetI18nDlgStrsW(Dialog, "Tera Term", text_info, _countof(text_info), UILanguageFileW);

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
	}
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
 *	�_�C�A���O�̃f�t�H���g�t�@�C������Ԃ�
 *		�t�B���^(ts.FileSendFilter)�����C���h�J�[�h�ł͂Ȃ��A
 *		���̃t�@�C�������݂���ꍇ
 *		�f�t�H���g�̃t�@�C�����Ƃ��ĕԂ�
 *
 * @param[in]	path		�t�@�C�������݂��邩���ׂ�p�X
 *							(lpstrInitialDir �ɐݒ肳���p�X)
 * @retval		NULL		�f�t�H���g�t�@�C�����Ȃ�
 * @retval		NULL�ȊO	�f�t�H���g�t�@�C��(�s�v�ɂȂ�����free()���邱��)
 */
static wchar_t *GetCommonDialogDefaultFilenameW(const wchar_t *path)
{
	const char *FileSendFilterA = ts.FileSendFilter;
	if (strlen(FileSendFilterA) == 0) {
		return NULL;
	}

	// �t�B���^�����C���h�J�[�h�ł͂Ȃ��A���̃t�@�C�������݂���ꍇ
	// ���炩���߃f�t�H���g�̃t�@�C���������Ă��� (2008.5.18 maya)
	wchar_t *filename = NULL;
	if (!isInvalidFileNameChar(FileSendFilterA)) {
		wchar_t file[MAX_PATH];
		wcsncpy_s(file, _countof(file), path, _TRUNCATE);
		AppendSlashW(file, _countof(file));
		wchar_t *FileSendFilterW = ToWcharA(FileSendFilterA);
		wcsncat_s(file, _countof(file), FileSendFilterW, _TRUNCATE);
		DWORD attr = GetFileAttributesW(file);
		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// �t�@�C�������݂���
			filename = _wcsdup(file);
		}
		free(FileSendFilterW);
	}

	return filename;
}

wchar_t **MakeStrArrayFromArray(wchar_t **strs)
{
	// ���𐔂���
	size_t strs_count = 0;
	size_t strs_len = 0;
	while(1) {
		const wchar_t *f = strs[strs_count];
		if (f == NULL) {
			break;
		}
		strs_count++;
		size_t len = wcslen(f) + 1;	// len = 1 when "\0"
		strs_len += len;
	}

	// 1�̈�ɕۑ�
	size_t ptrs_len = sizeof(char *) * (strs_count + 1);
	char *pool = (char *)malloc(ptrs_len + strs_len * sizeof(wchar_t));
	wchar_t **ptrs = (wchar_t **)pool;
	wchar_t *strpool = (wchar_t *)(pool + ptrs_len);
	for (int i = 0 ; i < strs_count; i++) {
		size_t len = wcslen(strs[i]) + 1;
		len = len * sizeof(wchar_t);
		memcpy(strpool, strs[i], len);
		ptrs[i] = strpool;
		strpool += len;
	}
	ptrs[strs_count] = NULL;
	return ptrs;
}

wchar_t **MakeStrArrayFromStr(const wchar_t *str)
{
	const wchar_t *strs[2];
	strs[0] = str;
	strs[1] = NULL;
	wchar_t **ret = MakeStrArrayFromArray((wchar_t **)strs);
	return ret;
}

wchar_t **MakeFileArrayMultiSelect(const wchar_t *lpstrFile)
{
	// ���𐔂���
	size_t file_count = 0;
	const wchar_t *p = lpstrFile;
	const wchar_t *path = p;
	size_t len = wcslen(p);
	p += len + 1;
	while(1) {
		len = wcslen(p);
		if (len == 0) {
			break;
		}
		p += len + 1;
		file_count++;
	}

	if (file_count == 0) {
		// 1�����I������Ă���
		return MakeStrArrayFromStr(lpstrFile);
	}

	// �p�X + �t�@�C���� �ꗗ�쐬
	size_t ptr_len = sizeof(wchar_t *) * (file_count + 1);
	wchar_t **filenames = (wchar_t **)malloc(ptr_len);
	len = wcslen(path);
	p = path + (len + 1);
	size_t filelen_sum = 0;
	for (int i = 0 ; i < file_count; i++) {
		size_t filelen = aswprintf(&filenames[i], L"%s\\%s", path, p);
		filelen_sum += filelen + 1;
		len = wcslen(p);
		p += len + 1;
	}
	filenames[file_count] = NULL;

	wchar_t **ret = MakeStrArrayFromArray(filenames);

	for (int i = 0 ; i < file_count; i++) {
		free(filenames[i]);
	}

	return ret;
}

static wchar_t **_GetXFname(HWND HWin, BOOL Receive, const wchar_t *caption, LPLONG Option)
{
	wchar_t *CurDir = GetDownloadDir(&ts);
	wchar_t *filterW = ToWcharA(ts.FileSendFilter);
	wchar_t *FNFilter = GetCommonDialogFilterWW(!Receive ? filterW : NULL, ts.UILanguageFileW);
	free(filterW);

	wchar_t FullName[MAX_PATH];
	FullName[0] = 0;
	if (!Receive) {
		wchar_t *default_filename = GetCommonDialogDefaultFilenameW(CurDir);
		if (default_filename != NULL) {
			wcsncpy_s(FullName, _countof(FullName), default_filename, _TRUNCATE);
			free(default_filename);
		}
	}

	OPENFILENAMEW ofn = {};
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
	ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_XOPT);
	ofn.hInstance = hInst;

	/* save current dir */
	wchar_t TempDir[MAX_PATH];
	GetCurrentDirectoryW(_countof(TempDir), TempDir);
	BOOL Ok;
	if (!Receive)
	{
		Ok = GetOpenFileNameW(&ofn);
	}
	else {
		Ok = GetSaveFileNameW(&ofn);
	}
	free(FNFilter);
	free(CurDir);
	SetCurrentDirectoryW(TempDir);

	wchar_t **ret = NULL;
	if (Ok) {
		if (Receive)
			*Option = opt;
		else
			*Option = MAKELONG(LOWORD(*Option),HIWORD(opt));

		ret = MakeStrArrayFromStr(FullName);
	}

	return ret;
}

/**
 *	OnIdle()#teraterm.cpp����R�[�������
 *		cv_ProtoFlag �� 0 �ȊO�̂Ƃ�
 *	@retval		0		continue
 *				1/2		ActiveWin(�O���[�o���ϐ�)�̒l(IdVT=1/IdTek=2)
 *				�� ���̂Ƃ���̂Ă��Ă���
 */
int ProtoDlgParse(void)
{
	int P;

	P = ActiveWin;
	if (PtDlg==NULL)
		return P;

	CommReceive(&cv); //�_�C�A���O�\�����Ɏ�M�����f�[�^�������ł���悤�ɓǂݎ����s�킹��

	PFileVarProto fv = FileVar;
	if (fv->ProtoOp->Parse(fv, &cv))
		P = 0; /* continue */
	else {
		CommSend(&cv);
		ProtoEnd();
	}
	return P;
}

void ProtoDlgTimeOut(void)
{
	if (PtDlg!=NULL) {
		PFileVarProto fv = FileVar;
		fv->ProtoOp->TimeOutProc(fv, &cv);
	}
}

void ProtoDlgCancel(void)
{
	if (PtDlg!=NULL) {
		PFileVarProto fv = FileVar;
		fv->ProtoOp->Cancel(fv, &cv);
		ProtoEnd();
	}
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
	wchar_t TempFull[MAX_PATH];
	const wchar_t *UILanguageFileW = ts.UILanguageFileW;

	switch (Message) {
	case WM_INITDIALOG:
		fv = (PFileVarProto)lParam;
		SetWindowLongPtr(Dialog, DWLP_USER, lParam);
		SendDlgItemMessage(Dialog, IDC_GETFN, EM_LIMITTEXT, sizeof(TempFull)-1,0);
		SetI18nDlgStrsW(Dialog, "Tera Term", text_info, _countof(text_info), UILanguageFileW);
		return TRUE;

	case WM_COMMAND:
		fv = (PFileVarProto)GetWindowLongPtr(Dialog,DWLP_USER);
		switch (LOWORD(wParam)) {
		case IDOK:
			if (fv!=NULL) {
				GetDlgItemTextW(Dialog, IDC_GETFN, TempFull, _countof(TempFull));
				if (wcslen(TempFull)==0) {
					fv->FileNames = NULL;
					return TRUE;
				}
				fv->FileNames = MakeStrArrayFromStr(TempFull);
			}
			EndDialog(Dialog, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(Dialog, 0);
			return TRUE;
		case IDC_GETFNHELP:
			if (fv!=NULL) {
				// �Ăяo�������w���vID����������
				PostMessage(fv->HMainWin,WM_USER_DLGHELP2,0,0);
			}
			break;
		}
	}
	return FALSE;
}

static BOOL _GetGetFname(HWND HWin, PFileVarProto fv, PTTSet ts)
{
	SetDialogFont(ts->DialogFontNameW, ts->DialogFontPoint, ts->DialogFontCharSet,
				  ts->UILanguageFileW, "Tera Term", "DLG_SYSTEM_FONT");
	return (BOOL)TTDialogBoxParam(hInst,
								  MAKEINTRESOURCEW(IDD_GETFNDLG),
								  HWin, GetFnDlg, (LPARAM)fv);
}

static UINT_PTR CALLBACK TransFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ IDC_FOPT, "DLG_FOPT" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
	};
	LPWORD pw;
	LPOFNOTIFY notify;
	static HFONT DlgFoptFont;

	switch (Message) {
	case WM_INITDIALOG: {
		const wchar_t *UILanguageFileW = ts.UILanguageFileW;
		LPOPENFILENAMEW ofn = (LPOPENFILENAMEW)lParam;
		pw = (LPWORD)ofn->lCustData;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)pw);

		if (get_lang_font(L"DLG_TAHOMA_FONT", Dialog, &DlgFoptFont, UILanguageFileW)) {
			SendDlgItemMessage(Dialog, IDC_FOPT, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_FOPTBIN, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_FOPTAPPEND, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_PLAINTEXT, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_TIMESTAMP, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgFoptFont = NULL;
		}

		SetI18nDlgStrsW(Dialog, "Tera Term", text_info, _countof(text_info), UILanguageFileW);

		SetRB(Dialog,*pw & 1,IDC_FOPTBIN,IDC_FOPTBIN);

		CenterCommonDialog(Dialog);

		return TRUE;
	}
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


static wchar_t **_GetMultiFname(HWND hWnd, WORD FuncId, const wchar_t *caption, LPWORD Option)
{
#define FnStrMemSize 4096
	wchar_t TempDir[MAX_PATH];
	const wchar_t *UILanguageFileW = ts.UILanguageFileW;
	wchar_t *CurDir = GetDownloadDir(&ts);

	/* save current dir */
	GetCurrentDirectoryW(_countof(TempDir), TempDir);

	wchar_t *FnStrMem = (wchar_t *)malloc(FnStrMemSize * sizeof(wchar_t));
	if (FnStrMem == NULL) {
		MessageBeep(0);
		return FALSE;
	}
	FnStrMem[0] = 0;

	wchar_t *filterW = ToWcharA(ts.FileSendFilter);
	wchar_t *FNFilter = GetCommonDialogFilterWW(filterW, UILanguageFileW);
	free(filterW);

	wchar_t *default_filename = GetCommonDialogDefaultFilenameW(CurDir);
	if (default_filename != NULL) {
		wcsncpy_s(FnStrMem, FnStrMemSize, default_filename, _TRUNCATE);
		free(default_filename);
	}

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = hWnd;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FnStrMem;
	ofn.nMaxFile = FnStrMemSize;
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
		ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_FOPT);
	} else if (FuncId==GMF_Y) {
		// TODO: YMODEM

	}

	ofn.hInstance = hInst;

	BOOL Ok = GetOpenFileNameW(&ofn);
	free(FNFilter);

	wchar_t **ret = NULL;
	if (Ok) {
		// multiple selection
		ret = MakeFileArrayMultiSelect(FnStrMem);
	}
	free(FnStrMem);
	free(CurDir);

	/* restore dir */
	SetCurrentDirectoryW(TempDir);

	return ret;
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
 *	Kermit ���M
 *
 *	@param[in]	filename			��M�t�@�C����(NULL�̂Ƃ��A�_�C�A���O�őI������)
 */
BOOL KermitStartSend(const wchar_t *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;
	TFileVarProto *fv = FileVar;

	FileVar->OpId = OpKmtSend;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_KMTSEND", TitKmtSend);

	if (filename == NULL) {
		WORD w = 0;
		wchar_t **filenames = _GetMultiFname(fv->HMainWin, GMF_KERMIT, fv->DlgCaption, &w);
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
	KermitStart(IdKmtSend);

	return TRUE;
}

/**
 *	Kermit ��M
 */
BOOL KermitGet(const wchar_t *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpKmtSend;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_KMTGET", TitKmtGet);

	if (filename == NULL) {
		if (! _GetGetFname(FileVar->HMainWin,FileVar, &ts) || FileVar->FileNames == NULL) {
			FreeFileVar_(&FileVar);
			return FALSE;
		}
	}
	else {
		FileVar->FileNames = MakeStrArrayFromStr(filename);
		FileVar->NoMsg = TRUE;
	}
	KermitStart(IdKmtGet);

	return TRUE;
}

/**
 *	Kermit ��M
 */
BOOL KermitStartRecive(BOOL macro)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpKmtRcv;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_KMTRCV", TitKmtRcv);

	if (macro) {
		// �}�N������
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

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_KMTFIN", TitKmtFin);

	if (macro) {
		FileVar->NoMsg = TRUE;
	}
	KermitStart(IdKmtFinish);

	return TRUE;
}

/**
 *	XMODEM��M
 *
 *	@param[in]	filename			��M�t�@�C����(NULL�̂Ƃ��A�_�C�A���O�őI������)
 *	@param[in]	ParamBinaryFlag
 *	@param[in]	ParamXmodemOpt
 */
BOOL XMODEMStartReceive(const wchar_t *filename, WORD ParamBinaryFlag, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpXRcv;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_XRCV", TitXRcv);

	if (filename == NULL) {
		LONG Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		wchar_t **filenames = _GetXFname(HVTWin, TRUE, fv->DlgCaption, &Option);
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

	if (! ProtoStart()) {
		return FALSE;
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	int mode = IdXReceive;
	if (! OpenProtoDlg(FileVar,PROTO_XM,mode, ts.XmodemOpt,ts.XmodemBin)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

/**
 *	XMODEM���M
 *
 *	@param[in]	filename			���M�t�@�C����(NULL�̂Ƃ��A�_�C�A���O�őI������)
 *	@param[in]	ParamXmodemOpt
 */
BOOL XMODEMStartSend(const wchar_t *filename, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpXSend;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_XSEND", TitXSend);

	if (filename == NULL) {
		LONG Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		wchar_t **filenames = _GetXFname(HVTWin, FALSE, fv->DlgCaption, &Option);
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

	if (! ProtoStart()) {
		return FALSE;
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	int mode = IdXSend;
	if (! OpenProtoDlg(FileVar,PROTO_XM,mode, ts.XmodemOpt,ts.XmodemBin)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

/**
 *	YMODEM��M
 *
 *	@param[in]	macro	TURE�̂Ƃ��}�N������Ă΂ꂽ
 */
BOOL YMODEMStartReceive(BOOL macro)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	TFileVarProto *fv = FileVar;

	if (macro) {
		FileVar->NoMsg = TRUE;
	}

	if (! ProtoStart())
		return FALSE;

	FileVar->OpId = OpYRcv;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_YRCV", TitYRcv);

	// �t�@�C���]�����̃I�v�V������"Yopt1K"�Ɍ��ߑł��B
	WORD Opt = Yopt1K;
//	_SetFileVar(FileVar);

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_YM,IdYReceive,Opt,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

/**
 *	YMODEM���M
 *
 *	@param[in]	filename			���M�t�@�C����(NULL�̂Ƃ��A�_�C�A���O�őI������)
 */
BOOL YMODEMStartSend(const wchar_t *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	if (! ProtoStart())
		return FALSE;

	TFileVarProto *fv = FileVar;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_YSEND", TitYSend);

	// �t�@�C���]�����̃I�v�V������"Yopt1K"�Ɍ��ߑł��B
	// TODO: "Yopt1K", "YoptG", "YoptSingle"����ʂ������Ȃ�΁AIDD_FOPT���g������K�v����B
	WORD Opt = Yopt1K;
	FileVar->OpId = OpYSend;
	if (filename == NULL) {
		wchar_t **filenames = _GetMultiFname(fv->HMainWin, GMF_Y, fv->DlgCaption, &Opt);
		if (filenames == NULL) {
			ProtoEnd();
			return FALSE;
		}
		//ts.XmodemBin = Opt;
		fv->FileNames = filenames;
	}
	else {
		fv->FileNames = MakeStrArrayFromStr(filename);
		FileVar->NoMsg = TRUE;
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_YM, IdYSend,Opt,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

/**
 *	ZMODEM��M
 *
 *	@param[in]	macro		TURE�̂Ƃ��}�N������Ă΂ꂽ
 *	@param[in]	autostart	TURE�̂Ƃ������X�^�[�g
 */
BOOL ZMODEMStartReceive(BOOL macro, BOOL autostart)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	if (macro) {
		FileVar->NoMsg = TRUE;
	}
	int mode = autostart ? IdZAutoR : IdZReceive;

	if (! ProtoStart())
		return FALSE;

	TFileVarProto *fv = FileVar;

	/* IdZReceive or IdZAutoR */
	FileVar->OpId = OpZRcv;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_ZRCV", TitZRcv);

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	WORD Opt = 0;
	if (! OpenProtoDlg(FileVar,PROTO_ZM,mode,Opt,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

/**
 *	ZMODEM���M
 *
 *	@param[in]	filename			���M�t�@�C����(NULL�̂Ƃ��A�_�C�A���O�őI������)
 *	@param[in]	ParamBinaryFlag		binary mode
 *	@param[in]	autostart			TURE�̂Ƃ������X�^�[�g
 */
BOOL ZMODEMStartSend(const wchar_t *filename, WORD ParamBinaryFlag, BOOL autostart)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	int mode = autostart ? IdZAutoS : IdZSend;

	if (! ProtoStart())
		return FALSE;

	TFileVarProto *fv = FileVar;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_ZSEND", TitZSend);

	WORD Opt = ts.XmodemBin;
	FileVar->OpId = OpZSend;
	if (filename == NULL) {
		wchar_t **filenames = _GetMultiFname(fv->HMainWin, GMF_Z, fv->DlgCaption, &Opt);
		if (filenames == NULL) {
			if (mode == IdZAutoS) {
				CommRawOut(&cv, "\030\030\030\030\030\030\030\030\b\b\b\b\b\b\b\b\b\b", 18);
			}
			ProtoEnd();
			return FALSE;
		}
		fv->FileNames = filenames;
		ts.XmodemBin = Opt;
	}
	else {
		fv->FileNames = MakeStrArrayFromStr(filename);
		ts.XmodemBin = ParamBinaryFlag;
		FileVar->NoMsg = TRUE;
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_ZM,mode,Opt,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

static wchar_t **_GetTransFname(HWND hWnd, const wchar_t *DlgCaption)
{
	wchar_t TempDir[MAX_PATH];
	wchar_t FileName[MAX_PATH];
	const wchar_t *UILanguageFileW = ts.UILanguageFileW;
	wchar_t *CurDir = GetDownloadDir(&ts);

	/* save current dir */
	GetCurrentDirectoryW(_countof(TempDir), TempDir);

	wchar_t *filterW = ToWcharA(ts.FileSendFilter);
	wchar_t *FNFilter = GetCommonDialogFilterWW(filterW, UILanguageFileW);
	free(filterW);

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = hWnd;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = _countof(FileName);
	ofn.lpstrInitialDir = CurDir;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHOWHELP;
	ofn.lpstrTitle = DlgCaption;
	ofn.hInstance = hInst;

	BOOL Ok = GetOpenFileNameW(&ofn);
	free(FNFilter);
	free(CurDir);

	wchar_t **ret = NULL;
	if (Ok) {
		ret = MakeStrArrayFromStr(FileName);
	}
	/* restore dir */
	SetCurrentDirectoryW(TempDir);
	return ret;
}

BOOL BPStartSend(const wchar_t *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	TFileVarProto *fv = FileVar;
	FileVar->OpId = OpBPSend;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_BPSEND", TitBPSend);

	if (! ProtoStart())
		return FALSE;

	if (filename == NULL) {
		wchar_t **filenames = _GetTransFname(fv->HMainWin, FileVar->DlgCaption);
		if (filenames == NULL) {
			ProtoEnd();
			return FALSE;
		}
		fv->FileNames = filenames;
	}
	else {
		fv->FileNames = MakeStrArrayFromStr(filename);
		fv->NoMsg = TRUE;
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_BP, IdBPSend,0,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

BOOL BPStartReceive(BOOL macro, BOOL autostart)
{
	if (FileVar != NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	TFileVarProto *fv = FileVar;
	int mode = autostart ? IdBPAuto : IdBPReceive;

	if (macro) {
		FileVar->NoMsg = TRUE;
	}

	if (! ProtoStart())
		return FALSE;

	/* IdBPReceive or IdBPAuto */
	FileVar->OpId = OpBPRcv;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_BPRCV", TitBPRcv);

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_BP,mode,0,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

BOOL QVStartReceive(BOOL macro)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	if (macro) {
		FileVar->NoMsg = TRUE;
	}
	int mode = IdQVReceive;

	if (! ProtoStart())
		return FALSE;

	TFileVarProto *fv = FileVar;

	FileVar->OpId = OpQVRcv;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_QVRCV", TitQVRcv);

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_QV,mode,0,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

BOOL QVStartSend(const wchar_t *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	TFileVarProto *fv = FileVar;

	if (! ProtoStart())
		return FALSE;

	FileVar->OpId = OpQVSend;

	SetDialogCation(fv, "FILEDLG_TRANS_TITLE_QVSEND", TitQVSend);

	if (filename == NULL) {
		WORD Opt;
		wchar_t **filenames = _GetMultiFname(fv->HMainWin, GMF_QV, fv->DlgCaption, &Opt);
		if (filenames == NULL) {
			ProtoEnd();
			return FALSE;
		}
		fv->FileNames = filenames;
	}
	else {
		fv->FileNames = MakeStrArrayFromStr(filename);
		FileVar->NoMsg = TRUE;
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_QV,IdQVSend,0,0)) {
		ProtoEnd();
		return FALSE;
	}

	return TRUE;
}

BOOL IsFileVarNULL()
{
	return FileVar == NULL;
}

BOOL ProtoGetProtoFlag(void)
{
	return cv_ProtoFlag;
}
