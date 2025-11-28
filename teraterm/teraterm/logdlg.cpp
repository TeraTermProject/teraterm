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
#include "ttlib_types.h"
#include "dlglib.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "history_store.h"
#include "compat_win.h"

#include "filesys_log_res.h"
#include "filesys_log.h"

#include "logdlg.h"

#define ID_EVENT	0

typedef struct {
	FLogDlgInfo_t *info;
	// work
	bool file_exist;
	int current_bom; // 存在するファイルのエンコーディング（ファイルのBOMから判定）
	bool available_timer;
	bool enable_timer;
	bool on_initdialog;
	HWND file_edit;
	WNDPROC proc;
	TTTSet *pts;
	TComVar *pcv;
} LogDlgWork_t;

static HistoryStore *hs;

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
 *	@param[out]	exist	ture/false
 *	@param[out]	bom		0	no BOM (or file not exist or too short)
 *						1	UTF-8
 *						2	UTF-16LE
 *						3	UTF-16BE
 */
static void CheckLogFile(const wchar_t *filename, bool *exist, int *bom)
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
	bool exist;
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
	// Append ラジオボタン
	if (work->file_exist) {
		// 指定されたファイルが存在する場合は Enable
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), TRUE);
	}
	else {
		// 指定されたファイルが存在しない場合は Disable
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), FALSE);
	}

	const bool log_binary = IsDlgButtonChecked(Dialog, IDC_FOPTBIN) == BST_CHECKED;
	const bool new_overwrite = IsDlgButtonChecked(Dialog, IDC_NEW_OVERWRITE) == BST_CHECKED;

	// BOM, Encoding
	if (!log_binary && new_overwrite) {
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
	if (log_binary) {
		// Binary の場合は Disable
		DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_PLAINTEXT);
		DisableDlgItem(Dialog, IDC_TIMESTAMP, IDC_TIMESTAMP);
		DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
	}
	else {
		// Text の場合は Enable
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

	if (work->file_exist && !new_overwrite) {
		// 既存ファイルのエンコーディングを反映する
		int bom = work->current_bom;
		int cur =
			bom == 1 ? 0 :
			bom == 2 ? 1 :
			bom == 3 ? 2 : 0;
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, cur, 0);
	}
}

/**
 * ログファイルをチェックして、
 * ラジオボタン、ファイルの状態からコントロールをEnable/Disableする
 */
static void ArrangeControlsFilename(HWND Dialog, LogDlgWork_t *work, const wchar_t *filename)
{
	const bool file_exist_prev = work->file_exist;
	CheckLogFile(filename, work);
	if (work->on_initdialog || file_exist_prev != work->file_exist) {
		if (work->file_exist) {
			// ファイルが存在する、設定に合わせて新規(上書き)/追記を選択する
			CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND,
							 work->pts->Append == 0 ? IDC_NEW_OVERWRITE : IDC_APPEND);
		}
		else {
			// ファイルが存在しない、新規を選択する
			CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
		}
		ArrangeControls(Dialog, work);
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
		work->enable_timer = false;
		break;
	}
	return CallWindowProcW(work->proc, dlg, msg, wParam, lParam);
}

static INT_PTR CALLBACK LogFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_FOPT_TITLE" },
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
		work->on_initdialog = true;
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

		// new(overwrite)/append radio button
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND,
						 pts->Append == 0 ? IDC_NEW_OVERWRITE : IDC_APPEND);

		// timestamp
		CheckDlgButton(Dialog, IDC_TIMESTAMP, pts->LogTimestamp == 0 ? BST_UNCHECKED : BST_CHECKED);

		// timestamp 種別
		int tstype = pts->LogTimestampType == TIMESTAMP_LOCAL ? 0 :
		             pts->LogTimestampType == TIMESTAMP_UTC ? 1 :
		             pts->LogTimestampType == TIMESTAMP_ELAPSED_LOGSTART ? 2 :
		             pts->LogTimestampType == TIMESTAMP_ELAPSED_CONNECTED ? 3 : 0;
		SendDlgItemMessageA(Dialog, IDC_TIMESTAMPTYPE, CB_SETCURSEL, tstype, 0);

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

		// text/binary radio button
		CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, pts->LogBinary == 0 ? IDC_FOPTTEXT : IDC_FOPTBIN);

		// ファイル名を設定する
		//   ファイルのチェック、コントロールの設定も行われる
		//		WM_COMMAND, EN_CHANGE が発生する
		wchar_t *fname = FLogGetLogFilename(work->info->filename);
		SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
		ArrangeControlsFilename(Dialog, work, fname);
		free(fname);

		// ドロップダウンのエディットコントロールのウィンドウハンドルを取得
		COMBOBOXINFO cbi;
		cbi.cbSize = sizeof(COMBOBOXINFO);
		_GetComboBoxInfo(GetDlgItem(Dialog, IDC_FOPT_FILENAME_EDIT), &cbi);
		work->file_edit = cbi.hwndItem;

		// サブクラス化
		SetWindowLongPtrW(work->file_edit, GWLP_USERDATA, (LONG_PTR)work);
		work->proc = (WNDPROC)SetWindowLongPtrW(work->file_edit, GWLP_WNDPROC, (LONG_PTR)FNameEditProc);

		HistoryStoreSetControl(hs, Dialog, IDC_FOPT_FILENAME_EDIT);
		ExpandCBWidth(Dialog, IDC_FOPT_FILENAME_EDIT);

		CenterWindow(Dialog, GetParent(Dialog));

		work->enable_timer = true;
		work->available_timer = true;
		SetTimer(Dialog, ID_EVENT, 1000, NULL);

		work->on_initdialog = false;

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

			wchar_t *uimsg;
			GetI18nStrWW("Tera Term", "DLG_FOPT_TITLE", L"Log", UILanguageFile, &uimsg);
			wchar_t *caption;
			aswprintf(&caption, L"Tera Term: %s", uimsg);
			free(uimsg);
			wchar_t *logdir = GetTermLogDir(work->pts);

			TTOPENFILENAMEW ofn = {};
			//ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			ofn.Flags |= OFN_EXPLORER | OFN_ENABLESIZING;
			ofn.Flags |= OFN_SHOWHELP;
			ofn.hwndOwner = Dialog;
			ofn.lpstrFilter = FNFilter;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = fname_ini;
			ofn.lpstrTitle = caption;
			ofn.lpstrInitialDir = logdir;
			wchar_t *fname;
			BOOL Ok = TTGetSaveFileNameW(&ofn, &fname);
			if (Ok) {
				SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
				ArrangeControlsFilename(Dialog, work, fname);
				free(fname);
			}
			free(logdir);
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
			switch (HIWORD(wParam)) {
			case CBN_EDITCHANGE: {
				wchar_t *filename;
				hGetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, &filename);
				ArrangeControlsFilename(Dialog, work, filename);
				free(filename);
				break;
			}
			case CBN_SELCHANGE: {
				int cursor = (int)SendDlgItemMessageA(Dialog, IDC_FOPT_FILENAME_EDIT, CB_GETCURSEL, 0, 0);
				wchar_t *filename;
				hGetDlgItemCBTextW(Dialog, IDC_FOPT_FILENAME_EDIT, cursor, &filename);
				ArrangeControlsFilename(Dialog, work, filename);
				free(filename);
				break;
			}
			case CBN_DROPDOWN: {
				work->enable_timer = false;
				break;
			}
			}
			break;
		}
		break;
	case WM_DROPFILES: {
		// 複数ドロップされても最初の1つだけを扱う
		HDROP hDrop = (HDROP)wParam;
		wchar_t *filename;
		hDragQueryFileW(hDrop, 0, &filename);
		DragFinish(hDrop);
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename);
		SendDlgItemMessageW(Dialog, IDC_FOPT_FILENAME_EDIT, EM_SETSEL, 0, -1);
		ArrangeControlsFilename(Dialog, work, filename);
		free(filename);
		return TRUE;
	}
	case WM_TIMER: {
		if (!work->enable_timer) {
			KillTimer(Dialog, ID_EVENT);
			work->available_timer = false;
			break;
		}
		wchar_t *fname = FLogGetLogFilename(work->info->filename);
		SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
		SendMessageW(work->file_edit, EM_SETSEL, 0, -1);
		ArrangeControlsFilename(Dialog, work, fname);
		free(fname);
		break;
	}
	case WM_DESTROY:
		if (work->available_timer) {
			KillTimer(Dialog, ID_EVENT);
			work->available_timer = false;
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
	if (hs == NULL) {
		hs = HistoryStoreCreate(20);
	}

	LogDlgWork_t *work = (LogDlgWork_t *)calloc(1, sizeof(LogDlgWork_t));
	work->info = info;
	work->pts = info->pts;
	work->pcv = info->pcv;
	INT_PTR ret = TTDialogBoxParam(
		hInst_, MAKEINTRESOURCEW(IDD_LOGDLG),
		hWnd, LogFnHook, (LPARAM)work);
	free(work);
	if (ret == IDOK) {
		HistoryStoreAddTop(hs, info->filename, FALSE);
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void FLogOpenDialogUnInit(void)
{
	if (hs != NULL) {
		HistoryStoreDestroy(hs);
		hs = NULL;
	}
}
