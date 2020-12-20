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
#include "dlglib.h"
#include "layer_for_unicode.h"
#include "tttypes.h"		// for WM_USER_DLGHELP2
#include "helpid.h"
#include "codeconv.h"
#include "asprintf.h"

#include "sendfiledlg.h"

#define TitSendFile L"Send file"

static INT_PTR CALLBACK SendFileDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{0, "FILEDLG_TRANS_TITLE_SENDFILE"},
		{IDC_SENDFILE_FILENAME_TITLE, "DLG_SENDFILE_FILENAME_TITLE"},
		{IDC_SENDFILE_CHECK_BINARY, "DLG_FOPT_BINARY"},
		{IDC_SENDFILE_DELAYTYPE_LABEL, "DLG_SENDFILE_DELAYTYPE_TITLE"},
		{IDC_SENDFILE_SEND_SIZE_LABEL, "DLG_SENDFILE_SEND_SIZE_TITLE"},
		{IDC_SENDFILE_DELAYTIME_LABEL, "DLG_SENDFILE_DELAYTIME_TITLE"},
		{IDCANCEL, "BTN_CANCEL"},
		{IDOK, "BTN_OK"},
	};
	static const I18nTextInfo delaytype_list[] = {
		{"DLG_SENDFILE_DELAYTYPE_NO_DELAY", L"no delay"},
		{"DLG_SENDFILE_DELAYTYPE_PER_CHAR", L"per charactor"},
		{"DLG_SENDFILE_DELAYTYPE_PER_LINE", L"per line"},
		{"DLG_SENDFILE_DELAYTYPE_PER_SENDSIZE", L"per sendsize"},
	};
	static const int send_size_list[] = {16, 256, 4 * 1024};
	sendfiledlgdata *data = (sendfiledlgdata *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
	int i;

	switch (msg) {
		case WM_INITDIALOG:
			data = (sendfiledlgdata *)lp;
			data->MsgDlgHelp = RegisterWindowMessage(HELPMSGSTRING);
			SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)data);
			::DragAcceptFiles(hDlgWnd, TRUE);
			SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), data->UILanguageFile);
			CenterWindow(hDlgWnd, GetParent(hDlgWnd));

			SetI18nList("TeraTerm", hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, delaytype_list, _countof(delaytype_list),
						data->UILanguageFile, 0);

			for (i = 0; i < _countof(send_size_list); i++) {
				char buf[32];
				sprintf(buf, "%d", send_size_list[i]);
				SendDlgItemMessageA(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)buf);
			}
			SendDlgItemMessage(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN, CB_SETCURSEL, _countof(send_size_list) - 1, 0);

			SetDlgItemTextA(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT, "1");

			EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT), FALSE);

			return TRUE;

		case WM_COMMAND:
			switch (wp) {
				case IDOK | (BN_CLICKED << 16): {
					wchar_t *strW = AllocControlTextW(GetDlgItem(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT));

					const DWORD attr = _GetFileAttributesW(strW);
					if (attr == INVALID_FILE_ATTRIBUTES || attr & FILE_ATTRIBUTE_DIRECTORY) {
						static const TTMessageBoxInfoW mbinfo = {
							"Tera Term",
							"MSG_TT_ERROR", L"Tera Term: Error",
							"MSG_CANTOPEN_FILE_ERROR", L"Cannot open file" };
						TTMessageBoxW(hDlgWnd, &mbinfo, MB_TASKMODAL | MB_ICONEXCLAMATION, data->UILanguageFile);

						free(strW);

						PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT),
									TRUE);

						return TRUE;
					}

					data->filename = strW;
					data->binary =
						SendMessage(GetDlgItem(hDlgWnd, IDC_SENDFILE_CHECK_BINARY), BM_GETCHECK, 0, 0) == BST_CHECKED
							? TRUE
							: FALSE;
					data->delay_type = (int)SendDlgItemMessage(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, CB_GETCURSEL, 0, 0);
					data->delay_tick = GetDlgItemInt(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT, NULL, FALSE);
					data->send_size = GetDlgItemInt(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN, NULL, FALSE);

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
					wchar_t TempDir[MAX_PATH];
					_GetCurrentDirectoryW(_countof(TempDir), TempDir);

					wchar_t *uimsg = TTGetLangStrW("Tera Term", "FILEDLG_TRANS_TITLE_SENDFILE", TitSendFile, data->UILanguageFile);
					wchar_t *title;
					aswprintf(&title, L"Tera Term: %s", uimsg);
					free(uimsg);
					uimsg = NULL;

					wchar_t *filterW = GetCommonDialogFilterW(data->filesend_filter, data->UILanguageFile);
					wchar_t filename[MAX_PATH];
					filename[0] = 0;
					OPENFILENAMEW ofn = {};
					ofn.lStructSize = get_OPENFILENAME_SIZEW();
					ofn.hwndOwner = hDlgWnd;
					ofn.lpstrFile = filename;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrFilter = filterW;
					ofn.nFilterIndex = 0;
					ofn.lpstrTitle = title;
					ofn.Flags = OFN_FILEMUSTEXIST | OFN_SHOWHELP | OFN_HIDEREADONLY;
					BOOL Ok = _GetOpenFileNameW(&ofn);
					free(filterW);
					free(title);

					_SetCurrentDirectoryW(TempDir);

					if (Ok) {
						_SetDlgItemTextW(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, filename);
						PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDOK), TRUE);
					}

					return TRUE;
				}

				case IDC_SENDFILE_DELAYTYPE_DROPDOWN | (CBN_SELCHANGE << 16): {
					int sel = (int)SendDlgItemMessage(hDlgWnd, IDC_SENDFILE_DELAYTYPE_DROPDOWN, CB_GETCURSEL, 0, 0);
					EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_SEND_SIZE_DROPDOWN), sel != 3 ? FALSE : TRUE);
					EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_DELAYTIME_EDIT), sel == 0 ? FALSE : TRUE);
					return TRUE;
				}

				default:
					return FALSE;
			}
			return FALSE;

		case WM_DROPFILES: {
			// 複数ドロップされても最初の1つだけを扱う
			HDROP hDrop = (HDROP)wp;
			const UINT len = _DragQueryFileW(hDrop, 0, NULL, 0);
			if (len == 0) {
				DragFinish(hDrop);
				return TRUE;
			}
			wchar_t *filename = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
			_DragQueryFileW(hDrop, 0, filename, len + 1);
			filename[len] = '\0';
			_SetDlgItemTextW(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, filename);
			SendDlgItemMessage(hDlgWnd, IDC_SENDFILE_FILENAME_EDIT, EM_SETSEL, len, len);
			PostMessage(hDlgWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlgWnd, IDOK), TRUE);

			free(filename);
			DragFinish(hDrop);
			return TRUE;
		}
		default:
			if (data != NULL && msg == data->MsgDlgHelp) {
				// コモンダイアログでヘルプボタンが押された
				PostMessage(GetParent(hDlgWnd), WM_USER_DLGHELP2, HlpMenuFileSendfile, 0);
				return TRUE;
			}
			return FALSE;
	}
}

INT_PTR sendfiledlg(HINSTANCE hInstance, HWND hWndParent, sendfiledlgdata *data)
{
	INT_PTR ret;
	ret = TTDialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SENDFILEDLG), hWndParent, SendFileDlgProc, (LPARAM)data);
	return ret;
}
