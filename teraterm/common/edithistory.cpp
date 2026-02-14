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

#include <stdio.h>
#include <string.h>
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"		// for MAXHOSTLIST
#include "hostname_rec.h"	// for HostNameMaxLength
#include "ttlib.h"
#include "dlglib.h"
#include "helpid.h"
#include "win32helper.h"
#include "resize_helper.h"
#include "history_store.h"

#include "edithistory_res.h"
#include "edithistory.h"

/**
 *	リストボックスの横スクロール幅を設定する
 */
static void ModifyListboxHScrollWidth(HWND dlg, int dlg_item)
{
	LRESULT i;
	LRESULT item_count;
	int max_width;

	HWND listbox = GetDlgItem(dlg, dlg_item);
	HDC listbox_dc = GetDC(listbox);
	HFONT listbox_font = (HFONT)SendMessageW(listbox, WM_GETFONT, 0, 0);
	HFONT listbox_font_old = (HFONT)SelectObject(listbox_dc, listbox_font);

	// リストボックス内の最大文字列幅取得
	max_width = 0;
	item_count = SendMessageW(listbox, LB_GETCOUNT, 0, 0);
	for (i = 0; i < item_count; i++) {
		SIZE size;
		wchar_t *strW;
		GetDlgItemIndexTextW(dlg, dlg_item, (WPARAM)i, &strW);
		GetTextExtentPoint32W(listbox_dc, strW, (int)wcslen(strW), &size);
		free(strW);
		if (max_width < size.cx) {
			max_width = size.cx;
		}
	}

	// 横幅を設定する
	SendMessageW(listbox, LB_SETHORIZONTALEXTENT, max_width + 5, 0);	// +5 幅に少し余裕を持たせる

	SelectObject(listbox_dc, listbox_font_old);
	ReleaseDC(listbox, listbox_dc);
}

/**
 *	Up/Remove/Downボタンをenable/disableする
 */
static void TCPIPDlgButtons(HWND Dialog)
{
	LRESULT item_count;
	LRESULT cur_pos;
	item_count = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCOUNT, 0, 0);
	cur_pos = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
	if ((cur_pos == LB_ERR) || (item_count == LB_ERR)) {
		DisableDlgItem(Dialog, IDC_TCPIPUP, IDC_TCPIPDOWN);
	}
	else {
		EnableDlgItem(Dialog, IDC_TCPIPREMOVE, IDC_TCPIPREMOVE);
		if (cur_pos == 0) {
			DisableDlgItem(Dialog, IDC_TCPIPUP, IDC_TCPIPUP);
		}
		else {
			EnableDlgItem(Dialog, IDC_TCPIPUP, IDC_TCPIPUP);
		}
		if (cur_pos + 1 >= item_count) {
			DisableDlgItem(Dialog, IDC_TCPIPDOWN, IDC_TCPIPDOWN);
		}
		else {
			EnableDlgItem(Dialog, IDC_TCPIPDOWN, IDC_TCPIPDOWN);
		}
	}
}

typedef struct {
	ReiseDlgHelper_t *resize_helper;
	const EditHistoryDlgData *parent_data;
	HistoryStore *hs;
} DlgData;

static INT_PTR CALLBACK TCPIPDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_TCPIPADD, "DLG_TCPIP_ADD" },
		{ IDC_TCPIPUP, "DLG_TCPIP_UP" },
		{ IDC_TCPIPREMOVE, "DLG_TCPIP_REMOVE" },
		{ IDC_TCPIPDOWN, "DLG_TCPIP_DOWN" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_TCPIPHELP, "BTN_HELP" },
	};
	static const ResizeHelperInfo resize_info[] = {
		{ IDC_TCPIPADD, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPUP, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPREMOVE, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPDOWN, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPHOST, RESIZE_HELPER_ANCHOR_LRT },
		{ IDC_TCPIPLIST, RESIZE_HELPER_ANCHOR_LRTB },
		{ IDOK, (ResizeHelperAnchor)(RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H) },
		{ IDCANCEL, (ResizeHelperAnchor)(RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H) },
		{ IDC_TCPIPHELP, (ResizeHelperAnchor)(RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H) },
	};

	switch (Message) {
		case WM_INITDIALOG: {
			DlgData *data = (DlgData *)lParam;
			SetWindowLongPtrW(Dialog, DWLP_USER, lParam);

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), data->parent_data->UILanguageFileW);
			SetWindowTextW(Dialog, data->parent_data->title);

			SendDlgItemMessageW(Dialog, IDC_TCPIPHOST, EM_LIMITTEXT, HostNameMaxLength - 1, 0);

			HistoryStoreSetControl(data->hs, Dialog, IDC_TCPIPLIST);
			ModifyListboxHScrollWidth(Dialog, IDC_TCPIPLIST);

			/* append a blank item to the bottom */
			SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_ADDSTRING, 0, 0);

			data->resize_helper = ReiseHelperInit(Dialog, TRUE, resize_info, _countof(resize_info));

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					DlgData *data = (DlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
					HistoryStoreGetControl(data->hs, Dialog, IDC_TCPIPLIST);
					EndDialog(Dialog, 1);
					return TRUE;
				}
				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_TCPIPHOST:
					if (HIWORD(wParam) == EN_CHANGE) {
						wchar_t *host;
						hGetDlgItemTextW(Dialog, IDC_TCPIPHOST, &host);
						if (wcslen(host) == 0) {
							DisableDlgItem(Dialog, IDC_TCPIPADD, IDC_TCPIPADD);
						}
						else {
							EnableDlgItem(Dialog, IDC_TCPIPADD, IDC_TCPIPADD);
						}
						free(host);
					}
					break;

				case IDC_TCPIPADD: {
					wchar_t *host;
					hGetDlgItemTextW(Dialog, IDC_TCPIPHOST, &host);
					if (wcslen(host) > 0) {
						LRESULT cur_pos;
						cur_pos = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
						if (cur_pos == LB_ERR) {
							cur_pos = 0;
						}
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_INSERTSTRING, cur_pos, (LPARAM)host);
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos, 0);
						SetDlgItemTextA(Dialog, IDC_TCPIPHOST, NULL);
						TCPIPDlgButtons(Dialog);
						ModifyListboxHScrollWidth(Dialog, IDC_TCPIPLIST);
						SetFocus(GetDlgItem(Dialog, IDC_TCPIPHOST));
					}
					free(host);
					break;
				}
				case IDC_TCPIPLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						TCPIPDlgButtons(Dialog);
					}
					break;

				case IDC_TCPIPUP:
				case IDC_TCPIPDOWN: {
					wchar_t *host;
					LRESULT item_count;
					LRESULT cur_pos;
					item_count = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCOUNT, 0, 0);
					cur_pos = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
					if (item_count == LB_ERR || cur_pos == LB_ERR) {
						return TRUE;
					}
					if (LOWORD(wParam) == IDC_TCPIPDOWN) {
						cur_pos++;
					}
					if ((cur_pos == 0) || (cur_pos >= item_count)) {
						return TRUE;
					}
					GetDlgItemIndexTextW(Dialog, IDC_TCPIPLIST, cur_pos, &host);
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_DELETESTRING, cur_pos, 0);
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_INSERTSTRING, cur_pos - 1, (LPARAM)host);
					free(host);
					if (LOWORD(wParam) == IDC_TCPIPUP) {
						cur_pos--;
					}
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos, 0);
					TCPIPDlgButtons(Dialog);
					SetFocus(GetDlgItem(Dialog, IDC_TCPIPLIST));
					break;
				}

				case IDC_TCPIPREMOVE: {
					LRESULT item_count;
					LRESULT cur_pos;
					wchar_t *host;
					item_count = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCOUNT, 0, 0);
					cur_pos = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
					if ((item_count == LB_ERR) || (cur_pos == LB_ERR)) {
						return TRUE;
					}
					GetDlgItemIndexTextW(Dialog, IDC_TCPIPLIST, cur_pos, &host);
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_DELETESTRING, cur_pos, 0);
					ModifyListboxHScrollWidth(Dialog, IDC_TCPIPLIST);
					if (item_count - 1 == 0) {
						// 0個になった
					}
					else if (cur_pos == item_count - 1) {
						// 一番最後を削除
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos - 1, 0);
					}
					else {
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos, 0);
					}
					SetDlgItemTextW(Dialog, IDC_TCPIPHOST, host);
					TCPIPDlgButtons(Dialog);
					SetFocus(GetDlgItem(Dialog, IDC_TCPIPHOST));
					free(host);
					break;
				}
				case IDC_TCPIPHELP: {
					DlgData *data = (DlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
					PostMessage(data->parent_data->vtwin, WM_USER_DLGHELP2,
								HlpMenuSetupAdditionalTCPIPEditHistory, 0);
					break;
				}
			}
			break;

		case WM_SIZE: {
			DlgData *data = (DlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
			ReiseDlgHelper_WM_SIZE(data->resize_helper, wParam, lParam);
			break;
		}

		case WM_GETMINMAXINFO: {
			DlgData *data = (DlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
			ReiseDlgHelper_WM_GETMINMAXINFO(data->resize_helper, lParam);
			break;
		}

		case WM_DESTROY: {
			DlgData *data = (DlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
			ReiseDlgHelperDelete(data->resize_helper);
			data->resize_helper = NULL;
			break;
		}
	}
	return FALSE;
}

BOOL EditHistoryDlg(HINSTANCE hInstance, HWND WndParent, EditHistoryDlgData *parent_data)
{
	BOOL r;
	DlgData *data = (DlgData *)calloc(1, sizeof(*data));
	if (data == NULL) {
		return FALSE;
	}
	if (hInstance == NULL) {
		hInstance = (HINSTANCE)GetWindowLongPtrW(WndParent, GWLP_HINSTANCE);
	}
	data->parent_data = parent_data;

	// TERATERM.INI の Hosts セクション読み込み
	data->hs = HistoryStoreCreate(MAXHOSTLIST);
	HistoryStoreReadIni(data->hs, parent_data->SetupFNameW, L"Hosts", L"host");

	r= (BOOL)TTDialogBoxParam(hInstance,
							  MAKEINTRESOURCEW(IDD_EDITHISTORYDLG),
							  WndParent, TCPIPDlg, (LPARAM)data);
	if (r == TRUE) {
		// TERATERM.INI へ書き出し
		HistoryStoreSaveIni(data->hs, parent_data->SetupFNameW, L"Hosts", L"host");
	}
	HistoryStoreDestroy(data->hs);
	free(data);
	return r;
}
