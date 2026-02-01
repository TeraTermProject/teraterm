/*
 * Copyright (C) 2024- TeraTerm Project
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

/* log property page */

#include <stdio.h>
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttwinman.h"	// for ts
#include "dlglib.h"
#include "helpid.h"
#include "tipwin.h"
#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "filesys_log.h"	// for FLogGetLogFilenameBase()

#include "log_pp.h"

CLogPropPageDlg::CLogPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CLogPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_Log",
				 L"Log", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
	m_TipWin = new CTipWin(inst);
}

CLogPropPageDlg::~CLogPropPageDlg()
{
	free((void *)m_psp.pszTitle);
	m_TipWin->Destroy();
	delete m_TipWin;
	m_TipWin = NULL;
}

// CLogPropPageDlg メッセージ ハンドラ

#define LOG_ROTATE_SIZETYPE_NUM 3
static const char *LogRotateSizeType[] = {
	"Byte", "KB", "MB"
};

static const char *GetLogRotateSizeType(int val)
{
	if (val >= LOG_ROTATE_SIZETYPE_NUM)
		val = 0;

	return LogRotateSizeType[val];
}

void CLogPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_VIEWLOG_LABEL, "DLG_TAB_LOG_EDITOR" },
		{ IDC_VIEWLOG_EXE_LABEL, "DLG_TAB_LOG_EDITOR_EXE" },
		{ IDC_VIEWLOG_ARG_LABEL, "DLG_TAB_LOG_EDITOR_ARG" },
		{ IDC_DEFAULTNAME_LABEL, "DLG_TAB_LOG_FILENAME" },
		{ IDC_DEFAULTPATH_LABEL, "DLG_TAB_LOG_FILEPATH" },
		{ IDC_AUTOSTART, "DLG_TAB_LOG_AUTOSTART" },
		// Log rotate
		{ IDC_LOG_ROTATE, "DLG_TAB_LOG_ROTATE" },
		{ IDC_ROTATE_SIZE_TEXT, "DLG_TAB_LOG_ROTATE_SIZE_TEXT" },
		{ IDC_ROTATE_STEP_TEXT, "DLG_TAB_LOG_ROTATESTEP" },
		// Log options
		// FIXME: メッセージカタログは既存のログオプションのものを流用したが、アクセラレータキーが重複するかもしれない。
		{ IDC_LOG_OPTION_GROUP, "DLG_FOPT" },
		{ IDC_OPT_BINARY, "DLG_FOPT_BINARY" },
		{ IDC_OPT_APPEND, "DLG_FOPT_APPEND" },
		{ IDC_OPT_PLAINTEXT, "DLG_FOPT_PLAIN" },
		{ IDC_OPT_HIDEDLG, "DLG_FOPT_HIDEDIALOG" },
		{ IDC_OPT_INCBUF, "DLG_FOPT_ALLBUFFINFIRST" },
		{ IDC_OPT_TIMESTAMP, "DLG_FOPT_TIMESTAMP" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	const static I18nTextInfo fopt_timestamp[] = {
		{ "DLG_FOPT_TIMESTAMP_LOCAL", L"Local Time" },
		{ "DLG_FOPT_TIMESTAMP_UTC", L"UTC" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_LOGGING", L"Elapsed Time (Logging)" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_CONNECTION", L"Elapsed Time (Connection)" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_OPT_TIMESTAMP_TYPE, fopt_timestamp, _countof(fopt_timestamp),
				 ts.UILanguageFileW, 0);

	// Viewlog Editor path
	SetDlgItemTextW(IDC_VIEWLOG_EDITOR_EXE, ts.ViewlogEditorW);
	SetDlgItemTextW(IDC_VIEWLOG_EDITOR_ARG, ts.ViewlogEditorArg);

	// Log Default File Name
	static const wchar_t *logfile_patterns[] = {
		L"teraterm.log",
		L"%H%M%S.log",
		L"%y%m%d%H%M%S.log",
		L"%Y%m%d_%H%M%S.log",
		L"%y%m%d_%H%M%S.log",
		L"teraterm_%y%m%d%H%M%S.log",
		L"%y%m%d_%H%M%S_&h.log",
		L"%Y%m%d_%H%M%S_%z.log",
		L"%y%m%d_%H%M%S_%z.log",
		L"%Y%m%dT%H%M%S%z.log",
	};
	for (int i = 0; i < _countof(logfile_patterns); i++) {
		const wchar_t *pattern = logfile_patterns[i];
		SendDlgItemMessageW(IDC_DEFAULTNAME_EDITOR, CB_ADDSTRING, 0, (LPARAM)pattern);
	}
	ExpandCBWidth(m_hWnd, IDC_DEFAULTNAME_EDITOR);
	SetDlgItemTextW(IDC_DEFAULTNAME_EDITOR, ts.LogDefaultNameW);

	// Log Default File Path (2007.5.30 maya)
	SetDlgItemTextW(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPathW);

	/* Auto start logging (2007.5.31 maya) */
	SetCheck(IDC_AUTOSTART, ts.LogAutoStart);

	// Log rotate
	SetCheck(IDC_LOG_ROTATE, ts.LogRotate != ROTATE_NONE);

	for (int i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
		SendDlgItemMessageA(IDC_ROTATE_SIZE_TYPE, CB_ADDSTRING, 0, (LPARAM)LogRotateSizeType[i]);
	}
	int TmpLogRotateSize = ts.LogRotateSize;
	for (int i = 0 ; i < ts.LogRotateSizeType ; i++)
		TmpLogRotateSize /= 1024;
	SetDlgItemInt(IDC_ROTATE_SIZE, TmpLogRotateSize, FALSE);
	SendDlgItemMessageA(IDC_ROTATE_SIZE_TYPE, CB_SELECTSTRING, -1, (LPARAM)GetLogRotateSizeType(ts.LogRotateSizeType));
	SetDlgItemInt(IDC_ROTATE_STEP, ts.LogRotateStep, FALSE);
	if (ts.LogRotate == ROTATE_NONE) {
		EnableDlgItem(IDC_ROTATE_SIZE_TEXT, FALSE);
		EnableDlgItem(IDC_ROTATE_SIZE, FALSE);
		EnableDlgItem(IDC_ROTATE_SIZE_TYPE, FALSE);
		EnableDlgItem(IDC_ROTATE_STEP_TEXT, FALSE);
		EnableDlgItem(IDC_ROTATE_STEP, FALSE);
	} else {
		EnableDlgItem(IDC_ROTATE_SIZE_TEXT, TRUE);
		EnableDlgItem(IDC_ROTATE_SIZE, TRUE);
		EnableDlgItem(IDC_ROTATE_SIZE_TYPE, TRUE);
		EnableDlgItem(IDC_ROTATE_STEP_TEXT, TRUE);
		EnableDlgItem(IDC_ROTATE_STEP, TRUE);
	}

	// Log options
	SetCheck(IDC_OPT_BINARY, ts.LogBinary != 0);
	if (ts.LogBinary) {
		EnableDlgItem(IDC_OPT_PLAINTEXT, FALSE);
		EnableDlgItem(IDC_OPT_TIMESTAMP, FALSE);
	} else {
		EnableDlgItem(IDC_OPT_PLAINTEXT, TRUE);
		EnableDlgItem(IDC_OPT_TIMESTAMP, TRUE);
	}
	SetCheck(IDC_OPT_APPEND, ts.Append != 0);
	SetCheck(IDC_OPT_PLAINTEXT, ts.LogTypePlainText != 0);
	SetCheck(IDC_OPT_HIDEDLG, ts.LogHideDialog != 0);
	SetCheck(IDC_OPT_INCBUF, ts.LogAllBuffIncludedInFirst != 0);
	SetCheck(IDC_OPT_TIMESTAMP, ts.LogTimestamp != 0);

	SetCurSel(IDC_OPT_TIMESTAMP_TYPE, ts.LogTimestampType);
	if (ts.LogBinary || !ts.LogTimestamp) {
		EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
	}
	else {
		EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
	}
/*
	switch (ts.LogTimestampType) {
		case CSF_CBRW:
			cmb->SetCurSel(3);
			break;
		case CSF_CBREAD:
			cmb->SetCurSel(2);
			break;
		case CSF_CBWRITE:
			cmb->SetCurSel(1);
			break;
		default: // off
			cmb->SetCurSel(0);
			break;
	}
*/
	m_TipWin->Create(m_hWnd);

	PostMessage(m_hWnd, WM_NEXTDLGCTL,
				(WPARAM)GetDlgItem(IDC_VIEWLOG_EDITOR_EXE), TRUE);
}

wchar_t *CLogPropPageDlg::MakePreviewStr(const wchar_t *format, const wchar_t *UILanguageFile)
{
	wchar_t *str = FLogGetLogFilenameBase(format);

	wchar_t *message = NULL;
	if (isInvalidStrftimeCharW(format)) {
		wchar_t *msg;
		GetI18nStrWW("Tera Term", "MSG_LOGFILE_INVALID_CHAR_ERROR",
					 L"Invalid character is included in log file name.",
					 UILanguageFile, &msg);
		awcscats(&message, L"\r\n", msg, L"(strftime)", NULL);
		free(msg);
	}

	if (isInvalidFileNameCharW(format)) {
		wchar_t *msg;
		GetI18nStrWW("Tera Term", "MSG_LOGFILE_INVALID_CHAR_ERROR",
					 L"Invalid character is included in log file name.",
					 UILanguageFile, &msg);
		awcscats(&message, L"\r\n", msg, L"(char)", NULL);
		free(msg);
	}

	if (message != NULL) {
		awcscat(&str, message);
		free(message);
	}

	return str;
}

BOOL CLogPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_VIEWLOG_PATH | (BN_CLICKED << 16):
			{
				wchar_t *editor;
				hGetDlgItemTextW(m_hWnd, IDC_VIEWLOG_EDITOR_EXE, &editor);

				TTOPENFILENAMEW ofn = {};
				ofn.hwndOwner = m_hWnd;
				ofn.lpstrFilter = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_LOGVIEW_APP_FILTER", L"exe(*.exe)\\0*.exe\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFileW);
				ofn.lpstrFile = editor;
				ofn.lpstrTitle = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_LOGVIEW_APP_TITLE", L"Choose a executing file with launching logging file", ts.UILanguageFileW);
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				wchar_t *filew;
				BOOL ok = TTGetOpenFileNameW(&ofn, &filew);
				if (ok) {
					SetDlgItemTextW(IDC_VIEWLOG_EDITOR_EXE, filew);
					free(filew);
				}
				free((void *)ofn.lpstrFilter);
				free((void *)ofn.lpstrTitle);
				free(editor);
			}
			return TRUE;

		case IDC_DEFAULTPATH_PUSH | (BN_CLICKED << 16):
			// ログディレクトリの選択ダイアログ
			{
				wchar_t *title = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_LOGDIR_TITLE", L"Select log folder", ts.UILanguageFileW);
				wchar_t *default_path;
				hGetDlgItemTextW(m_hWnd, IDC_DEFAULTPATH_EDITOR, &default_path);
				if (default_path[0] == 0) {
					free(default_path);
					default_path = _wcsdup(ts.LogDirW);
				}
				wchar_t *new_path;
				if (doSelectFolderW(GetSafeHwnd(), default_path, title, &new_path)) {
					SetDlgItemTextW(IDC_DEFAULTPATH_EDITOR, new_path);
					free(new_path);
				}
				free(default_path);
				free(title);
			}

			return TRUE;

		case IDC_LOG_ROTATE | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_LOG_ROTATE)) {
					EnableDlgItem(IDC_ROTATE_SIZE_TEXT, TRUE);
					EnableDlgItem(IDC_ROTATE_SIZE, TRUE);
					EnableDlgItem(IDC_ROTATE_SIZE_TYPE, TRUE);
					EnableDlgItem(IDC_ROTATE_STEP_TEXT, TRUE);
					EnableDlgItem(IDC_ROTATE_STEP, TRUE);
				} else {
					EnableDlgItem(IDC_ROTATE_SIZE_TEXT, FALSE);
					EnableDlgItem(IDC_ROTATE_SIZE, FALSE);
					EnableDlgItem(IDC_ROTATE_SIZE_TYPE, FALSE);
					EnableDlgItem(IDC_ROTATE_STEP_TEXT, FALSE);
					EnableDlgItem(IDC_ROTATE_STEP, FALSE);
				}

			}
			return TRUE;

		case IDC_OPT_BINARY | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_OPT_BINARY)) {
					EnableDlgItem(IDC_OPT_PLAINTEXT, FALSE);
					EnableDlgItem(IDC_OPT_TIMESTAMP, FALSE);
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
				} else {
					EnableDlgItem(IDC_OPT_PLAINTEXT, TRUE);
					EnableDlgItem(IDC_OPT_TIMESTAMP, TRUE);

					if (GetCheck(IDC_OPT_TIMESTAMP)) {
						EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
					}
				}
			}
			return TRUE;

		case IDC_OPT_TIMESTAMP | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_OPT_TIMESTAMP)) {
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
				} else {
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
				}
			}
			return TRUE;

		case IDC_DEFAULTNAME_EDITOR | (CBN_SELCHANGE << 16) :
		case IDC_DEFAULTNAME_EDITOR | (CBN_EDITCHANGE << 16):
		case IDC_DEFAULTNAME_EDITOR | (CBN_CLOSEUP << 16):
		case IDC_DEFAULTNAME_EDITOR | (CBN_SETFOCUS << 16): {
			wchar_t *format = NULL;
			if (wParam == (IDC_DEFAULTNAME_EDITOR | (CBN_SELCHANGE << 16))) {
				LRESULT r = SendDlgItemMessageW(IDC_DEFAULTNAME_EDITOR, CB_GETCURSEL, 0, 0);
				if (r != CB_ERR) {
					hGetDlgItemCBTextW(m_hWnd, IDC_DEFAULTNAME_EDITOR, r, &format);
				}
			}
			if (format == NULL) {
				hGetDlgItemTextW(m_hWnd, IDC_DEFAULTNAME_EDITOR, &format);
			}
			wchar_t *preview = MakePreviewStr(format, ts.UILanguageFileW);
			if (preview[0] != 0 && wcscmp(format, preview) != 0) {
				RECT rc;
				::GetWindowRect(::GetDlgItem(m_hWnd, IDC_DEFAULTNAME_EDITOR), &rc);
				m_TipWin->SetText(preview);
				m_TipWin->SetPos(rc.left, rc.bottom);
				m_TipWin->SetHideTimer(5 * 1000);  // 表示時間
				if (!m_TipWin->IsVisible()) {
					m_TipWin->SetVisible(TRUE);
				}
			}
			else {
				m_TipWin->SetVisible(FALSE);
			}
			free(preview);
			free(format);
			return TRUE;
		}

		case IDC_DEFAULTNAME_EDITOR | (CBN_KILLFOCUS << 16): {
			if (m_TipWin->IsVisible()) {
				m_TipWin->SetVisible(FALSE);
			}
			return TRUE;
		}
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CLogPropPageDlg::OnOKLogFilename()
{
	wchar_t *def_name;
	time_t time_local;
	struct tm tm_local;

	hGetDlgItemTextW(m_hWnd, IDC_DEFAULTNAME_EDITOR, &def_name);
	if (isInvalidStrftimeCharW(def_name)) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_LOGFILE_INVALID_CHAR_ERROR", L"Invalid character is included in log file name.",
			MB_ICONEXCLAMATION };
		TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW);
		return;
	}

	// 現在時刻を取得
	time(&time_local);
	localtime_s(&tm_local, & time_local);
	// 時刻文字列に変換
	wchar_t buf2[MAX_PATH];
	if (wcslen(def_name) != 0 && wcsftime(buf2, _countof(buf2), def_name, &tm_local) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_LOGFILE_TOOLONG_ERROR", L"The log file name is too long.",
			MB_ICONEXCLAMATION };
		TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW);
		free(def_name);
		return;
	}

	wchar_t *buf3 = replaceInvalidFileNameCharW(buf2, '_');

	if (isInvalidFileNameCharW(buf3)) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_LOGFILE_INVALID_CHAR_ERROR", L"Invalid character is included in log file name.",
			MB_ICONEXCLAMATION };
		TTMessageBoxW(m_hWnd, &info, ts.UILanguageFileW);
		free(def_name);
		free(buf3);
		return;
	}

	free(ts.LogDefaultNameW);
	ts.LogDefaultNameW = def_name;
	free(buf3);
}

void CLogPropPageDlg::OnOK()
{
	// Viewlog Editor path
	free(ts.ViewlogEditorW);
	hGetDlgItemTextW(m_hWnd, IDC_VIEWLOG_EDITOR_EXE, &ts.ViewlogEditorW);
	free(ts.ViewlogEditorArg);
	hGetDlgItemTextW(m_hWnd, IDC_VIEWLOG_EDITOR_ARG, &ts.ViewlogEditorArg);

	// Log Default File Name
	OnOKLogFilename();

	// Log Default File Path (2007.5.30 maya)
	free(ts.LogDefaultPathW);
	hGetDlgItemTextW(m_hWnd, IDC_DEFAULTPATH_EDITOR, &ts.LogDefaultPathW);

	/* Auto start logging (2007.5.31 maya) */
	ts.LogAutoStart = GetCheck(IDC_AUTOSTART);

	/* Log Rotate */
	if (GetCheck(IDC_LOG_ROTATE)) {  /* on */
		char buf[80];
		ts.LogRotate = ROTATE_SIZE;
		GetDlgItemTextA(IDC_ROTATE_SIZE_TYPE, buf, _countof(buf));
		ts.LogRotateSizeType = 0;
		for (int i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
			if (strcmp(buf, LogRotateSizeType[i]) == 0) {
				ts.LogRotateSizeType = i;
				break;
			}
		}
		ts.LogRotateSize = GetDlgItemInt(IDC_ROTATE_SIZE);
		for (int i = 0 ; i < ts.LogRotateSizeType ; i++)
			ts.LogRotateSize *= 1024;

		ts.LogRotateStep = GetDlgItemInt(IDC_ROTATE_STEP);

	} else { /* off */
		ts.LogRotate = ROTATE_NONE;
		/* 残りのメンバーは意図的に設定を残す。*/
	}

	// Log Options
	if (GetCheck(IDC_OPT_BINARY)) {
		ts.LogBinary = 1;
	}
	else {
		ts.LogBinary = 0;
	}

	if (GetCheck(IDC_OPT_APPEND)) {
		ts.Append = 1;
	}
	else {
		ts.Append = 0;
	}

	if (GetCheck(IDC_OPT_PLAINTEXT)) {
		ts.LogTypePlainText = 1;
	}
	else {
		ts.LogTypePlainText = 0;
	}

	if (GetCheck(IDC_OPT_HIDEDLG)) {
		ts.LogHideDialog = 1;
	}
	else {
		ts.LogHideDialog = 0;
	}

	if (GetCheck(IDC_OPT_INCBUF)) {
		ts.LogAllBuffIncludedInFirst = 1;
	}
	else {
		ts.LogAllBuffIncludedInFirst = 0;
	}

	if (GetCheck(IDC_OPT_TIMESTAMP)) {
		ts.LogTimestamp = 1;
	}
	else {
		ts.LogTimestamp = 0;
	}

	ts.LogTimestampType = GetCurSel(IDC_OPT_TIMESTAMP_TYPE);
}

void CLogPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalLog, 0);
}
