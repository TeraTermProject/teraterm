/*
 * (C) 2020- TeraTerm Project
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
#include "tipwin2.h"
#include "sendmem.h"
#include "history_store.h"
#include "compat_win.h"

#include "sendfiledlg.h"

typedef struct {
	sendfiledlgdata *create_param;
	// work
	TipWin2 *tip;
	UINT MsgDlgHelp;
} SendFileDlgWork_t;

static HistoryStore *hs;

static void ArrangeControls(HWND hDlgWnd)
{
	LRESULT index = SendDlgItemMessageA(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, CB_GETCURSEL, 0, 0);
	SendMemDelayType sel = (SendMemDelayType)SendDlgItemMessageA(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, CB_GETITEMDATA, (WPARAM)index, 0);

	if (IsDlgButtonChecked(hDlgWnd, IDC_SENDFILE_RADIO_SEQUENTIAL) == BST_CHECKED) {
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTYPE_LABEL), FALSE);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN), FALSE);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_LABEL), FALSE);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN), FALSE);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTIME_LABEL), FALSE);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT), FALSE);
	}
	else {
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTYPE_LABEL), TRUE);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN), TRUE);
		BOOL enable = (sel == SENDMEM_DELAYTYPE_PER_SENDSIZE) ? TRUE : FALSE;
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_LABEL), enable);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN), enable);
		enable = (sel == SENDMEM_DELAYTYPE_NO_DELAY) ? FALSE : TRUE;
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTIME_LABEL), enable);
		EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT), enable);
	}
}

static BOOL SelectFile(HWND hDlgWnd, const sendfiledlgdata *data, const wchar_t *filename_ini, wchar_t **filename)
{
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", "DLG_FILETRANS_TITLE", L"Send file", data->UILanguageFileW, &uimsg);
	wchar_t *title;
	aswprintf(&title, L"Tera Term: %s", uimsg);
	free(uimsg);
	uimsg = NULL;

	wchar_t *filterW = GetCommonDialogFilterWW(data->filesend_filter, data->UILanguageFileW);

	TTOPENFILENAMEW ofn = {};
	ofn.hwndOwner = hDlgWnd;
	ofn.lpstrFilter = filterW;
	ofn.nFilterIndex = 0;
	ofn.lpstrTitle = title;
	ofn.lpstrFile = filename_ini;		// 初期ファイル名
	ofn.lpstrInitialDir = data->initial_dir;	// 初期フォルダ
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_SHOWHELP | OFN_HIDEREADONLY;
	BOOL Ok = TTGetOpenFileNameW(&ofn, filename);

	free(filterW);
	free(title);

	return Ok;
}

static INT_PTR CALLBACK SendFileDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_FILETRANS_TITLE" },
		{ IDC_SENDFILE_FILENAME_TITLE, "DLG_SENDFILE_FILENAME_TITLE" },
		{ IDC_SENDFILE_READING_METHOD_LABEL, "DLG_SENDFILE_READING_METHOD_TITLE" },
		{ IDC_SENDFILE_RADIO_BULK, "DLG_SENDFILE_READING_METHOD_BULK" },
		{ IDC_SENDFILE_RADIO_SEQUENTIAL, "DLG_SENDFILE_READING_METHOD_SEQUENTIAL" },
		{ IDC_SENDFILE_CHECK_BINARY, "DLG_SENDFILE_BINARY" },
		{ IDC_SENDFILE_DELAYTYPE_LABEL, "DLG_SENDFILE_DELAYTYPE_TITLE" },
		{ IDC_SENDFILE_SEND_SIZE_LABEL, "DLG_SENDFILE_SEND_SIZE_TITLE" },
		{ IDC_SENDFILE_DELAYTIME_LABEL, "DLG_SENDFILE_DELAYTIME_TITLE" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDHELP, "BTN_HELP" },
	};
	static const I18nTextInfo delaytype_list[] = {
		{ "DLG_SENDFILE_DELAYTYPE_NO_DELAY", L"no delay", SENDMEM_DELAYTYPE_NO_DELAY },
		{ "DLG_SENDFILE_DELAYTYPE_PER_CHAR", L"per character", SENDMEM_DELAYTYPE_PER_CHAR },
		{ "DLG_SENDFILE_DELAYTYPE_PER_LINE", L"per line", SENDMEM_DELAYTYPE_PER_LINE },
		{ "DLG_SENDFILE_DELAYTYPE_PER_SENDSIZE", L"per sendsize", SENDMEM_DELAYTYPE_PER_SENDSIZE },
	};
	static const size_t send_size_list[] = {16, 256, 4 * 1024};
	SendFileDlgWork_t *work = (SendFileDlgWork_t *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
	sendfiledlgdata *data = work != NULL ? work->create_param : NULL;

	switch (msg) {
		case WM_INITDIALOG: {
			work = (SendFileDlgWork_t *)calloc(1, sizeof(*work));
			SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)work);
			data = (sendfiledlgdata *)lp;
			work->create_param = data;
			work->MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);
			::DragAcceptFiles(hDlgWnd, TRUE);
			SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), data->UILanguageFileW);
			CenterWindow(hDlgWnd, GetParent(hDlgWnd));

			SetI18nListW("Tera Term", hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, delaytype_list, _countof(delaytype_list),
						 data->UILanguageFileW, data->delay_type);

			if (data->initial_file != NULL) {
				HistoryStoreAddTop(hs, data->initial_file, FALSE);
			}

			HistoryStoreSetControl(hs, hDlgWnd, IDC_SENDFILE_FILENAME_EDIT);

			// 送信サイズ
			for (size_t i = 0; i < _countof(send_size_list); i++) {
				char buf[32];
				sprintf(buf, "%lu", (unsigned long)send_size_list[i]);
				SendDlgItemMessageA(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)buf);
			}
			SetDlgNum(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN, (LONG)data->send_size);

			SetDlgNum(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT, data->delay_tick);
			CheckDlgButton(hDlgWnd, IDC_SENDFILE_CHECK_BINARY, data->binary ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlgWnd,
						   data->sequential_read == TRUE ? IDC_SENDFILE_RADIO_SEQUENTIAL : IDC_SENDFILE_RADIO_BULK,
						   BST_CHECKED);

			ArrangeControls(hDlgWnd);

			TipWin2 *tip = TipWin2Create(NULL, hDlgWnd);
			work->tip = tip;
			wchar_t *text;
			GetI18nStrWW("Tera Term", "DLG_SENDFILE_READING_METOHD_TOOLTIP", NULL, data->UILanguageFileW, &text);
			if (text != NULL) {
				TipWin2SetTextW(tip, IDC_SENDFILE_RADIO_SEQUENTIAL, text);
				TipWin2SetTextW(tip, IDC_SENDFILE_RADIO_BULK, text);
				free(text);
			}

			// ドロップダウンのエディットコントロールで数字のみ入力
			{
				COMBOBOXINFO cbi;
				cbi.cbSize = sizeof(COMBOBOXINFO);
				HWND hComboBox = GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN);
				_GetComboBoxInfo(hComboBox, &cbi);
				HWND hEdit = cbi.hwndItem;
				SetWindowLongPtrW(hEdit, GWL_STYLE, GetWindowLongPtrW(hEdit, GWL_STYLE) | ES_NUMBER);
			}

			return TRUE;
		}
		case WM_DESTROY: {
			TipWin2Destroy(work->tip);
			work->tip = NULL;
			free(work);
			SetWindowLongPtr(hDlgWnd, DWLP_USER, 0);
			return FALSE;
		}
		case WM_COMMAND:
			switch (wp) {
				case IDOK | (BN_CLICKED << 16): {
					wchar_t *filename;
					hGetWindowTextW(GetDlgItem(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT), &filename);
					const DWORD attr = GetFileAttributesW(filename);
					if (attr == INVALID_FILE_ATTRIBUTES || attr & FILE_ATTRIBUTE_DIRECTORY) {
						static const TTMessageBoxInfoW mbinfo = {
							"Tera Term",
							"MSG_TT_ERROR", L"Tera Term: Error",
							"MSG_CANTOPEN_FILE_ERROR", L"Cannot open file",
							MB_TASKMODAL | MB_ICONEXCLAMATION };
						TTMessageBoxW(hDlgWnd, &mbinfo, data->UILanguageFileW);

						free(filename);

						PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT),
									TRUE);

						return TRUE;
					}
					HistoryStoreAddTop(hs, filename, FALSE);

					data->filename = filename;
					data->binary = IsDlgButtonChecked(hDlgWnd, IDC_SENDFILE_CHECK_BINARY) == BST_CHECKED ? TRUE : FALSE;
					LRESULT index = SendDlgItemMessageA(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, CB_GETCURSEL, 0, 0);
					data->delay_type = (SendMemDelayType)SendDlgItemMessageA(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, CB_GETITEMDATA, (WPARAM)index, 0);
					data->delay_tick = GetDlgItemInt(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT, NULL, FALSE);
					data->send_size = GetDlgItemInt(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN, NULL, FALSE);
					data->sequential_read = IsDlgButtonChecked(hDlgWnd, IDC_SENDFILE_RADIO_SEQUENTIAL) == BST_CHECKED ? TRUE : FALSE;

					TTEndDialog(hDlgWnd, IDOK);
					return TRUE;
				}

				case IDHELP | (BN_CLICKED << 16):
					PostMessage(GetParent(hDlgWnd), WM_USER_DLGHELP2, HlpMenuFileSendfile, 0);
					return TRUE;

				case IDCANCEL | (BN_CLICKED << 16):
					data->filename = NULL;
					TTEndDialog(hDlgWnd, IDCANCEL);
					return TRUE;

				case IDC_SENDFILE_FILENAME_BUTTON | (BN_CLICKED << 16): {
					wchar_t *filename_ini;
					hGetDlgItemTextW(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, &filename_ini);

					wchar_t *filename;
					BOOL Ok = SelectFile(hDlgWnd,data, filename_ini, &filename);
					free(filename_ini);

					if (Ok) {
						SetDlgItemTextW(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, filename);
						PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDOK), TRUE);
						free(filename);
					}

					return TRUE;
				}

				case IDC_SENDFILE_DELAYTYPE_DROPDOWN | (CBN_SELCHANGE << 16): {
					ArrangeControls(hDlgWnd);
					return TRUE;
				}

				case IDC_SENDFILE_RADIO_BULK | (BN_CLICKED << 16):
				case IDC_SENDFILE_RADIO_SEQUENTIAL | (BN_CLICKED << 16): {
					ArrangeControls(hDlgWnd);
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
			SetDlgItemTextW(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, filename);
			SendDlgItemMessage(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, EM_SETSEL, len, len);
			PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDOK), TRUE);

			free(filename);
			DragFinish(hDrop);
			return TRUE;
		}
		default:
			if (work != NULL && msg == work->MsgDlgHelp) {
				// コモンダイアログでヘルプボタンが押された
				PostMessage(GetParent(hDlgWnd), WM_USER_DLGHELP2, HlpMenuFileSendfile, 0);
				return TRUE;
			}
			return FALSE;
	}
}

INT_PTR sendfiledlg(HINSTANCE hInstance, HWND hWndParent, sendfiledlgdata *data)
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
		ret = TTDialogBoxParam(hInstance, MAKEINTRESOURCEW(IDD_SENDFILEDLG), hWndParent, SendFileDlgProc, (LPARAM)data);
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

void sendfiledlgUnInit(void)
{
	if (hs != NULL) {
		HistoryStoreDestroy(hs);
		hs = NULL;
	}
}
