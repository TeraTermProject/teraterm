/*
 * (C) 2025- TeraTerm Project
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

#include <windows.h>
#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <wchar.h>

#include "i18n.h"
#include "tt_res.h"
#include "ttlib.h"
#include "ttlib_types.h"
#include "dlglib.h"
#include "tttypes.h"		// for WM_USER_DLGHELP2
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "history_store.h"
#include "compat_win.h"

#include "recvfiledlg.h"

typedef struct {
	recvfiledlgdata *create_param;
	// work
	UINT MsgDlgHelp;
} RecvFileDlgWork_t;

static HistoryStore *hs;

static BOOL SelectFile(HWND hDlgWnd, const recvfiledlgdata *data, const wchar_t *filename_ini, wchar_t **filename)
{
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", "FILEDLG_TRANS_TITLE_RECVFILE", L"Receive file", data->UILanguageFileW, &uimsg);
	wchar_t *title;
	aswprintf(&title, L"Tera Term: %s", uimsg);
	free(uimsg);
	uimsg = NULL;

	wchar_t *filterW = GetCommonDialogFilterWW(data->filerecv_filter, data->UILanguageFileW);

	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner = hDlgWnd;
	ofn.lpstrFilter = filterW;
	ofn.nFilterIndex = 0;
	ofn.lpstrTitle = title;
	ofn.lpstrFile = filename_ini;				// 初期ファイル名
	ofn.lpstrInitialDir = data->initial_dir;	// 初期フォルダ
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	BOOL Ok = TTGetSaveFileNameW(&ofn, filename);

	free(filterW);
	free(title);

	return Ok;
}

static INT_PTR CALLBACK RecvFileDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "FILEDLG_TRANS_TITLE_RECVFILE" },
		{ IDC_RECVFILE_FILENAME_TITLE, "DLG_RECVFILE_FILENAME_TITLE" },
		{ IDC_RECVFILE_CHECK_BINARY, "DLG_RECVFILE_BINARY" },
		{ IDC_RECVFILE_AUTOSTOP_LABEL, "DLG_RECVFILE_AUTOSTOP_LABEL" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDHELP, "BTN_HELP" },
	};
	RecvFileDlgWork_t *work = (RecvFileDlgWork_t *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
	recvfiledlgdata *data = work != NULL ? work->create_param : NULL;

	switch (msg) {
		case WM_INITDIALOG: {
			work = (RecvFileDlgWork_t *)calloc(1, sizeof(*work));
			SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)work);
			data = (recvfiledlgdata *)lp;
			work->create_param = data;
			work->MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);
			::DragAcceptFiles(hDlgWnd, TRUE);
			SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), data->UILanguageFileW);
			SendMessage(GetDlgItem(hDlgWnd, IDC_RECVFILE_CHECK_BINARY), BM_SETCHECK, BST_CHECKED, 0);	// binaryのチェックボックスをチェック
			EnableWindow(GetDlgItem(hDlgWnd, IDC_RECVFILE_CHECK_BINARY),FALSE);						  	// binaryのチェックボックスは無効 (TODO:textモードは未実装)
			SendDlgItemMessage(hDlgWnd, IDC_RECVFILE_AUTOSTOP_EDIT, EM_LIMITTEXT, 8, 0);
			SetDlgNum(hDlgWnd, IDC_RECVFILE_AUTOSTOP_EDIT, data->autostop_sec);
			CenterWindow(hDlgWnd, GetParent(hDlgWnd));

			if (data->initial_file != NULL) {
				HistoryStoreAddTop(hs, data->initial_file, FALSE);
			}
			HistoryStoreSetControl(hs, hDlgWnd, IDC_RECVFILE_FILENAME_EDIT);
			return TRUE;
		}
		case WM_DESTROY: {
			free(work);
			SetWindowLongPtr(hDlgWnd, DWLP_USER, 0);
			return FALSE;
		}
		case WM_COMMAND:
			switch (wp) {
				case IDOK | (BN_CLICKED << 16): {
					wchar_t *filename;
					hGetWindowTextW(GetDlgItem(hDlgWnd, IDC_RECVFILE_FILENAME_EDIT), &filename);
					HANDLE hFile = CreateFileW(filename,
											   GENERIC_WRITE, FILE_SHARE_READ, NULL,
											   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE) {
						static const TTMessageBoxInfoW mbinfo = {
							"Tera Term",
							"MSG_TT_ERROR", L"Tera Term: Error",
							"MSG_CANTOPEN_FILE_ERROR", L"Cannot open file",
							MB_TASKMODAL | MB_ICONEXCLAMATION };
						TTMessageBoxW(hDlgWnd, &mbinfo, data->UILanguageFileW);
						free(filename);
						PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDC_RECVFILE_FILENAME_EDIT), TRUE);
						return TRUE;
					}
					CloseHandle(hFile);
					HistoryStoreAddTop(hs, filename, FALSE);
					data->filename = filename;
					data->binary = IsDlgButtonChecked(hDlgWnd, IDC_RECVFILE_CHECK_BINARY) == BST_CHECKED ? TRUE : FALSE;
					data->autostop_sec = GetDlgItemInt(hDlgWnd, IDC_RECVFILE_AUTOSTOP_EDIT, NULL, FALSE);
					TTEndDialog(hDlgWnd, IDOK);
					return TRUE;
				}

				case IDHELP | (BN_CLICKED << 16):
					PostMessage(GetParent(hDlgWnd), WM_USER_DLGHELP2, HlpMenuFileRecvfile, 0);
					return TRUE;

				case IDCANCEL | (BN_CLICKED << 16):
					data->filename = NULL;
					TTEndDialog(hDlgWnd, IDCANCEL);
					return TRUE;

				case IDC_RECVFILE_FILENAME_BUTTON | (BN_CLICKED << 16): {
					wchar_t *filename_ini;
					hGetDlgItemTextW(hDlgWnd, IDC_RECVFILE_FILENAME_EDIT, &filename_ini);
					wchar_t *filename;
					BOOL Ok = SelectFile(hDlgWnd,data, filename_ini, &filename);
					free(filename_ini);
					if (Ok) {
						SetDlgItemTextW(hDlgWnd, IDC_RECVFILE_FILENAME_EDIT, filename);
						PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDOK), TRUE);
						free(filename);
					}
					return TRUE;
				}

				default:
					return FALSE;
			}
			break;

		case WM_DROPFILES: {
			// 複数ドロップされても最初の1つだけを扱う
			HDROP hDrop = (HDROP)wp;
			const UINT len = DragQueryFileW(hDrop, 0, NULL, 0);
			if (len == 0) {
				DragFinish(hDrop);
				return TRUE;
			}
			wchar_t *filename = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
			DragQueryFileW(hDrop, 0, filename, len + 1);
			filename[len] = '\0';
			SetDlgItemTextW(hDlgWnd, IDC_RECVFILE_FILENAME_EDIT, filename);
			SendDlgItemMessage(hDlgWnd, IDC_RECVFILE_FILENAME_EDIT, EM_SETSEL, len, len);
			PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDOK), TRUE);

			free(filename);
			DragFinish(hDrop);
			return TRUE;
		}
		default:
			if (work != NULL && msg == work->MsgDlgHelp) {
				// コモンダイアログでヘルプボタンが押された
				PostMessage(GetParent(hDlgWnd), WM_USER_DLGHELP2, HlpMenuFileRecvfile, 0);
				return TRUE;
			}
			return FALSE;
	}
}

INT_PTR recvfiledlg(HINSTANCE hInstance, HWND hWndParent, recvfiledlgdata *data)
{
	if (hs == NULL) {
		hs = HistoryStoreCreate(20);
	}

	BOOL skip_dialog = data->skip_dialog;
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) {
		// CTRL が押されていた時、逆の動作となる
		skip_dialog = !skip_dialog;
	}

	if (!skip_dialog) {
		INT_PTR ret;
		ret = TTDialogBoxParam(hInstance, MAKEINTRESOURCEW(IDD_RECVFILEDLG), hWndParent, RecvFileDlgProc, (LPARAM)data);
		return ret;
	}
	else {
		wchar_t *filename_ini = data->filename;
		wchar_t *filename;
		BOOL Ok = SelectFile(hWndParent, data, filename_ini, &filename);
		if (Ok) {
			data->filename = filename;
			return (INT_PTR)IDOK;
		}
		else {
			data->filename = NULL;
			return (INT_PTR)IDCANCEL;
		}
	}
}

void recvfiledlgUnInit(void)
{
	if (hs != NULL) {
		HistoryStoreDestroy(hs);
		hs = NULL;
	}
}
