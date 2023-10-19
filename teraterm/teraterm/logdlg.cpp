/*
 * (C) 2023- TeraTerm Project
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

#include <stdio.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ftdlg.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"

#include "filesys_log_res.h"
#include "filesys_log.h"

#include "logdlg.h"

#define TitLog      L"Log"

typedef struct {
	FLogDlgInfo_t *info;
	// work
	BOOL file_exist;
	int current_bom; // 存在するファイルのエンコーディング（ファイルのBOMから判定）
	UINT_PTR timer;
	BOOL enable_timer;
	WNDPROC proc;
	TTTSet *pts;
	TComVar *pcv;
} LogDlgWork_t;

/**
 *	ダイアログの内容を ts に書き戻し
 *
 *	TODO
 *		ダイアログで設定した値は一時的なもので
 *		設定を上書きするのは良くないのではないだろうか?
 */
static void SetLogFlags(HWND Dialog, TTTSet *pts)
{
	WORD BinFlag, val;

	GetRB(Dialog, &BinFlag, IDC_FOPTBIN, IDC_FOPTBIN);
	pts->LogBinary = BinFlag;

	GetRB(Dialog, &val, IDC_APPEND, IDC_APPEND);
	pts->Append = val;

	if (!BinFlag) {
		GetRB(Dialog, &val, IDC_PLAINTEXT, IDC_PLAINTEXT);
		pts->LogTypePlainText = val;

		GetRB(Dialog, &val, IDC_TIMESTAMP, IDC_TIMESTAMP);
		pts->LogTimestamp = val;
	}

	GetRB(Dialog, &val, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
	pts->LogHideDialog = val;

	GetRB(Dialog, &val, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
	pts->LogAllBuffIncludedInFirst = val;

	pts->LogTimestampType = (WORD)(GetCurSel(Dialog, IDC_TIMESTAMPTYPE) - 1);
}

/**
 *	ログファイルチェック
 *
 *	@param[in]	filename
 *	@param[out]	exist	TURE/FALSE
 *	@param[out]	bom		0	no BOM (or file not exist)
 *						1	UTF-8
 *						2	UTF-16LE
 *						3	UTF-16BE
 */
static void CheckLogFile(const wchar_t *filename, BOOL *exist, int *bom)
{
	*exist = FALSE;
	*bom = 0;

	// ファイルが存在する?
	DWORD logdir = GetFileAttributesW(filename);
	if ((logdir != INVALID_FILE_ATTRIBUTES) && ((logdir & FILE_ATTRIBUTE_DIRECTORY) == 0)) {
		// ファイルがあった
		*exist = TRUE;

		// BOM有り/無しチェック
		FILE *fp;
		errno_t e = _wfopen_s(&fp, filename, L"rb");
		if (e == 0 && fp != NULL) {
			unsigned char tmp[4];
			size_t l = fread(tmp, 1, sizeof(tmp), fp);
			fclose(fp);
			if (l < 2) {
				*bom = 0;
			} else if (l >= 2 && tmp[0] == 0xff && tmp[1] == 0xfe) {
				// UTF-16LE
				*bom = 2;
			} else if (l >= 2 && tmp[0] == 0xfe && tmp[1] == 0xff) {
				// UTF-16BE
				*bom = 3;
			} else if (l >= 3 && tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf) {
				// UTF-8
				*bom = 1;
			} else {
				*bom = 0;
			}
		}
	}
}

static void CheckLogFile(const wchar_t *filename, LogDlgWork_t *work)
{
	BOOL exist;
	int bom;
	CheckLogFile(filename, &exist, &bom);
	work->file_exist = exist;
	work->current_bom = bom;
}

/**
 * ラジオボタン、ファイルの状態からコントロールをEnable/Disableする
 */
static void ArrangeControls(HWND Dialog, LogDlgWork_t *work)
{
	WORD Append, LogBinary;

	GetRB(Dialog, &Append, IDC_APPEND, IDC_APPEND);
	GetRB(Dialog, &LogBinary, IDC_FOPTBIN, IDC_FOPTBIN);

	// Append ラジオボタン
	if (work->file_exist) {
		// 指定されたファイルが存在する場合は Enable
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), TRUE);

		if (Append > 0) {
			CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		}
		else {
			CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
		}
	}
	else {
		// 指定されたファイルが存在しない場合は Disable
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), FALSE);

		// ファイルがない -> 新規
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
	}

	// BOM, Encoding
	if (!LogBinary && !Append) {
		// Text かつ New/Overwrite の場合に Enable
		EnableWindow(GetDlgItem(Dialog, IDC_BOM), TRUE);
		EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), TRUE);
	}
	else {
		// そうでない場合に Disable
		//   BOM はファイルの先頭から書き込むときしか意味がない
		//   Encoding は追記でも意味があるが、既存ファイルのエンコーディングを
		//   強制的にダイアログに反映するので、ユーザによる指定はさせない
		EnableWindow(GetDlgItem(Dialog, IDC_BOM), FALSE);
		EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), FALSE);
	}

	// Plain Text, Timestamp, Timestamp 種別
	if (LogBinary) {
		// Binary の場合は Disable
		CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, IDC_FOPTBIN);

		DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_PLAINTEXT);
		DisableDlgItem(Dialog, IDC_TIMESTAMP, IDC_TIMESTAMP);
		DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
	}
	else {
		// Text の場合は Enable
		CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, IDC_FOPTTEXT);

		EnableDlgItem(Dialog, IDC_PLAINTEXT, IDC_PLAINTEXT);
		EnableDlgItem(Dialog, IDC_TIMESTAMP, IDC_TIMESTAMP);

		// Timestamp 種別
		if (IsDlgButtonChecked(Dialog, IDC_TIMESTAMP) == BST_UNCHECKED) {
			// Timestamp=off の場合は Disable
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
		} else {
			// Timestamp=on の場合は Enable
			EnableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
		}
	}

	if (work->file_exist && Append) {
		// 既存ファイルのエンコーディングを反映する
		int bom = work->current_bom;
		int cur =
			bom == 1 ? 0 :
			bom == 2 ? 1 :
			bom == 3 ? 2 : 0;
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, cur, 0);
	}
}

static LRESULT CALLBACK FNameEditProc(HWND dlg, UINT msg,
									  WPARAM wParam, LPARAM lParam)
{
	LogDlgWork_t *work = (LogDlgWork_t *)GetWindowLongPtr(dlg, GWLP_USERDATA);
	switch (msg) {
	case WM_KEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_KILLFOCUS:
		work->enable_timer = FALSE;
		break;
	}
	return CallWindowProcW(work->proc, dlg, msg, wParam, lParam);
}

static INT_PTR CALLBACK LogFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_TABSHEET_TITLE_LOG" },
		{ IDC_SENDFILE_FILENAME_TITLE, "DLG_FOPT_FILENAME_TITLE" },
		{ IDC_FOPTTEXT, "DLG_FOPT_TEXT" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
		{ IDC_BOM, "DLG_FOPT_BOM" },
		{ IDC_APPEND, "DLG_FOPT_APPEND" },
		{ IDC_PLAINTEXT, "DLG_FOPT_PLAIN" },
		{ IDC_HIDEDIALOG, "DLG_FOPT_HIDEDIALOG" },
		{ IDC_ALLBUFF_INFIRST, "DLG_FOPT_ALLBUFFINFIRST" },
		{ IDC_TIMESTAMP, "DLG_FOPT_TIMESTAMP" },
		{ IDC_NEW_OVERWRITE, "DLG_FOPT_NEW_OVERWRITE" },
		{ IDC_APPEND_GROUP, "DLG_FOPT_APPEND_LABEL" },
		{ IDC_BINARY_GROUP, "DLG_FOPT_BINARY_LABEL" },
	};
	static const I18nTextInfo timestamp_list[] = {
		{ "DLG_FOPT_TIMESTAMP_LOCAL", L"Local Time" },
		{ "DLG_FOPT_TIMESTAMP_UTC", L"UTC" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_LOGGING", L"Elapsed Time (Logging)" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_CONNECTION", L"Elapsed Time (Connection)" },
	};
	LogDlgWork_t *work = (LogDlgWork_t *)GetWindowLongPtr(Dialog, DWLP_USER);

	if (Message == RegisterWindowMessage(HELPMSGSTRING)) {
		// コモンダイアログからのヘルプメッセージを付け替える
		Message = WM_COMMAND;
		wParam = IDHELP;
	}
	switch (Message) {
	case WM_INITDIALOG: {
		work = (LogDlgWork_t *)lParam;
		TTTSet *pts = work->pts;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)work);
		::DragAcceptFiles(Dialog, TRUE);

		SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), pts->UILanguageFileW);
		SetI18nListW("Tera Term", Dialog, IDC_TIMESTAMPTYPE, timestamp_list, _countof(timestamp_list),
		             pts->UILanguageFileW, 0);

		SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-8");
		SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-16LE");
		SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-16BE");
		SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);

		// ファイル名を設定する
		//   ファイルのチェック、コントロールの設定も行われる
		//		WM_COMMAND, EN_CHANGE が発生する
		wchar_t *fname = FLogGetLogFilename(work->info->filename);
		SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
		free(fname);
		HWND file_edit = GetDlgItem(Dialog, IDC_FOPT_FILENAME_EDIT);
		SetWindowLongPtr(file_edit, GWLP_USERDATA, (LONG_PTR)work);
		work->proc = (WNDPROC)SetWindowLongPtrW(file_edit, GWLP_WNDPROC, (LONG_PTR)FNameEditProc);

		// timestamp
		CheckDlgButton(Dialog, IDC_TIMESTAMP, pts->LogTimestamp == 0 ? BST_UNCHECKED : BST_CHECKED);

		// timestamp 種別
		int tstype = pts->LogTimestampType == TIMESTAMP_LOCAL ? 0 :
		             pts->LogTimestampType == TIMESTAMP_UTC ? 1 :
		             pts->LogTimestampType == TIMESTAMP_ELAPSED_LOGSTART ? 2 :
		             pts->LogTimestampType == TIMESTAMP_ELAPSED_CONNECTED ? 3 : 0;
		SendDlgItemMessageA(Dialog, IDC_TIMESTAMPTYPE, CB_SETCURSEL, tstype, 0);
		EnableWindow(GetDlgItem(Dialog, IDC_TIMESTAMPTYPE), pts->LogTimestamp == 0 ? FALSE : TRUE);

		// plain text
		CheckDlgButton(Dialog, IDC_PLAINTEXT, pts->LogTypePlainText == 0 ? BST_UNCHECKED : BST_CHECKED);

		// Hide dialog チェックボックス
		if (pts->LogHideDialog) {
			SetRB(Dialog, 1, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
		}

		// Include screen buffer チェックボックス
		if (pts->LogAllBuffIncludedInFirst) {
			SetRB(Dialog, 1, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
		}

		CenterWindow(Dialog, GetParent(Dialog));

		SetFocus(GetDlgItem(Dialog, IDC_FOPT_FILENAME_EDIT));

		work->enable_timer = TRUE;
		work->timer = SetTimer(Dialog, 0, 1000, NULL);

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			wchar_t *filename;
			hGetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, &filename);
			work->info->filename = filename;
			work->info->append = IsDlgButtonChecked(Dialog, IDC_APPEND) == BST_CHECKED;
			work->info->bom = IsDlgButtonChecked(Dialog, IDC_BOM) == BST_CHECKED;
			work->info->code = (LogCode_t)SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_GETCURSEL, 0, 0);
			SetLogFlags(Dialog, work->pts);
			EndDialog(Dialog, IDOK);
			break;
		}
		case IDCANCEL:
			EndDialog(Dialog, IDCANCEL);
			break;
		case IDHELP:
			OpenHelpCV(work->pcv, HH_HELP_CONTEXT, HlpFileLog);
			break;
		case IDC_FOPT_FILENAME_BUTTON: {
			/* save current dir */
			const wchar_t *UILanguageFile = work->pts->UILanguageFileW;

			wchar_t *fname_ini;
			hGetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, &fname_ini);

			const wchar_t* simple_log_filter = L"*.txt;*.log";
			wchar_t *FNFilter = GetCommonDialogFilterWW(simple_log_filter, UILanguageFile);

			wchar_t *caption;
			wchar_t *uimsg;
			GetI18nStrWW("Tera Term", "FILEDLG_TRANS_TITLE_LOG",
						 TitLog, UILanguageFile, &uimsg);
			aswprintf(&caption, L"Tera Term: %s", uimsg);
			free(uimsg);

			TTOPENFILENAMEW ofn = {};
			//ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			ofn.Flags |= OFN_EXPLORER | OFN_ENABLESIZING;
			ofn.Flags |= OFN_SHOWHELP;
			ofn.hwndOwner = Dialog;
			ofn.lpstrFilter = FNFilter;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = fname_ini;
			ofn.lpstrTitle = caption;
			ofn.lpstrInitialDir = work->pts->LogDefaultPathW;
			wchar_t *fname;
			BOOL Ok = TTGetSaveFileNameW(&ofn, &fname);
			if (Ok) {
				SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
				free(fname);
			}
			free(caption);
			free(FNFilter);
			free(fname_ini);

			break;
		}
		case IDC_NEW_OVERWRITE:
		case IDC_APPEND:
		case IDC_FOPTTEXT:
		case IDC_FOPTBIN:
		case IDC_TIMESTAMP:
			ArrangeControls(Dialog, work);
			break;
		case IDC_FOPT_FILENAME_EDIT:
			if (HIWORD(wParam) == EN_CHANGE){
				wchar_t *filename;
				hGetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, &filename);
				CheckLogFile(filename, work);
				free(filename);
				ArrangeControls(Dialog, work);
			}
			break;
		}
		break;
	case WM_DROPFILES: {
		// 複数ドロップされても最初の1つだけを扱う
		HDROP hDrop = (HDROP)wParam;
		const UINT len = DragQueryFileW(hDrop, 0, NULL, 0);
		if (len == 0) {
			DragFinish(hDrop);
			return TRUE;
		}
		wchar_t *filename = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
		DragQueryFileW(hDrop, 0, filename, len + 1);
		filename[len] = '\0';
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename);
		SendDlgItemMessage(Dialog, IDC_FOPT_FILENAME_EDIT, EM_SETSEL, len, len);
		free(filename);
		DragFinish(hDrop);
		return TRUE;
	}
	case WM_TIMER: {
		if (!work->enable_timer) {
			KillTimer(Dialog, work->timer);
			work->timer = 0;
			break;
		}
		wchar_t *fname = FLogGetLogFilename(work->info->filename);
		SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
		SendDlgItemMessageW(Dialog, IDC_FOPT_FILENAME_EDIT, EM_SETSEL, 0, -1);
		free(fname);
		work->timer = SetTimer(Dialog, 0, 1000, NULL);
		break;
	}
	case WM_DESTROY:
		if (work->timer != 0) {
			KillTimer(Dialog, work->timer);
		}
		break;
	}
	return FALSE;
}

/**
 *	ログダイアログを開く
 *	@param[in,out]	info.filename	ファイル名初期値
 *									OK時、ファイル名、不要になったらfree()すること
 *	@retval	TRUE	[ok] が押された
 *	@retval	FALSE	キャンセルされた
 */
BOOL FLogOpenDialog(HINSTANCE hInst_, HWND hWnd, FLogDlgInfo_t *info)
{
	LogDlgWork_t *work = (LogDlgWork_t *)calloc(sizeof(LogDlgWork_t), 1);
	work->info = info;
	work->pts = info->pts;
	work->pcv = info->pcv;
	INT_PTR ret = TTDialogBoxParam(
		hInst_, MAKEINTRESOURCE(IDD_LOGDLG),
		hWnd, LogFnHook, (LPARAM)work);
	free(work);
	return ret == IDOK ? TRUE : FALSE;
}
