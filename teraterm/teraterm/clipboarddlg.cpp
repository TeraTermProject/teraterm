/*
 * (C) 2019-2020 TeraTerm Project
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

#include "teraterm.h"
#include "tttypes.h"
#include "vtdisp.h"
#include "vtterm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "ttwinman.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"
#include "layer_for_unicode.h"
#include "tt_res.h"
#include "clipboarddlg.h"
#include "compat_win.h"

static INT_PTR CALLBACK OnClipboardDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_CLIPBOARD_TITLE" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDOK, "BTN_OK" },
	};
	POINT p;
	RECT rc_dsk, rc_dlg;
	int dlg_height, dlg_width;
	static int ok2right, edit2ok, edit2bottom;
	RECT rc_edit, rc_ok, rc_cancel;
	// for status bar
	static HWND hStatus = NULL;
	static int init_width, init_height;
	clipboarddlgdata *data = (clipboarddlgdata *)GetWindowLongPtr(hDlgWnd, DWLP_USER);

	switch (msg) {
		case WM_INITDIALOG:
			data = (clipboarddlgdata *)lp;
			SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)data);
			SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), data->UILanguageFile);

			if (data->strW_ptr != NULL) {
				_SetDlgItemTextW(hDlgWnd, IDC_EDIT, data->strW_ptr);
			} else {
				SetDlgItemTextA(hDlgWnd, IDC_EDIT, data->strA_ptr);
			}

			if (ActiveWin == IdVT) { // VT Window
				/*
				 * Caret off 時に GetCaretPos() で正確な場所が取れないので、
				 * vtdisp.c 内部で管理している値から計算する
				 */
				int x, y;
				DispConvScreenToWin(CursorX, CursorY, &x, &y);
				p.x = x;
				p.y = y;
			}
			else if (!GetCaretPos(&p)) { // Tek Window
				/*
				 * Tek Window は内部管理の値を取るのが面倒なので GetCaretPos() を使う
				 * GetCaretPos() がエラーになった場合は念のため 0, 0 を入れておく
				 */
				p.x = 0;
				p.y = 0;
			}

			// x, y の両方が 0 の時は親ウィンドウの中央に移動させられるので、
			// それを防ぐ為に x を 1 にする
			if (p.x == 0 && p.y == 0) {
				p.x = 1;
			}

			ClientToScreen(GetParent(hDlgWnd), &p);

			// キャレットが画面からはみ出しているときに貼り付けをすると
			// 確認ウインドウが見えるところに表示されないことがある。
			// ウインドウからはみ出した場合に調節する (2008.4.24 maya)
			if (pMonitorFromPoint == NULL) {
				// NT4.0, 95 はマルチモニタAPIに非対応
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc_dsk, 0);
			}
			else {
				HMONITOR hm;
				POINT pt;
				MONITORINFO mi;

				pt.x = p.x;
				pt.y = p.y;
				hm = pMonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

				mi.cbSize = sizeof(MONITORINFO);
				pGetMonitorInfoA(hm, &mi);
				rc_dsk = mi.rcWork;
			}
			GetWindowRect(hDlgWnd, &rc_dlg);
			dlg_height = rc_dlg.bottom-rc_dlg.top;
			dlg_width  = rc_dlg.right-rc_dlg.left;
			if (p.y < rc_dsk.top) {
				p.y = rc_dsk.top;
			}
			else if (p.y + dlg_height > rc_dsk.bottom) {
				p.y = rc_dsk.bottom - dlg_height;
			}
			if (p.x < rc_dsk.left) {
				p.x = rc_dsk.left;
			}
			else if (p.x + dlg_width > rc_dsk.right) {
				p.x = rc_dsk.right - dlg_width;
			}

			SetWindowPos(hDlgWnd, NULL, p.x, p.y,
			             0, 0, SWP_NOSIZE | SWP_NOZORDER);

			// ダイアログの初期サイズを保存
			GetWindowRect(hDlgWnd, &rc_dlg);
			init_width = rc_dlg.right - rc_dlg.left;
			init_height = rc_dlg.bottom - rc_dlg.top;

			// 現在サイズから必要な値を計算
			GetClientRect(hDlgWnd,                                 &rc_dlg);
			GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
			GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);

			p.x = rc_dlg.right;
			p.y = rc_dlg.bottom;
			ClientToScreen(hDlgWnd, &p);
			ok2right = p.x - rc_ok.left;
			edit2bottom = p.y - rc_edit.bottom;
			edit2ok = rc_ok.left - rc_edit.right;

			// サイズを復元
			SetWindowPos(hDlgWnd, NULL, 0, 0,
			             ts.PasteDialogSize.cx, ts.PasteDialogSize.cy,
			             SWP_NOZORDER | SWP_NOMOVE);

			// リサイズアイコンを右下に表示させたいので、ステータスバーを付ける。
			InitCommonControls();
			hStatus = CreateStatusWindow(
				WS_CHILD | WS_VISIBLE |
				CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hDlgWnd, 1);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
				{
					INT_PTR result = IDCANCEL;

					size_t len = _SendDlgItemMessageW(hDlgWnd, IDC_EDIT, WM_GETTEXTLENGTH, 0, 0);
					len++; // for '\0'
					wchar_t *strW = (wchar_t *)malloc(sizeof(wchar_t) * len);
					if (strW != NULL) {
						_GetDlgItemTextW(hDlgWnd, IDC_EDIT, strW, (int)len);
						strW[len - 1] = '\0';
						result = IDOK;
					}
					data->strW_edited_ptr = strW;

					DestroyWindow(hStatus);
					TTEndDialog(hDlgWnd, result);
				}
					break;

				case IDCANCEL:
					DestroyWindow(hStatus);
					TTEndDialog(hDlgWnd, IDCANCEL);
					break;

				default:
					return FALSE;
			}

		case WM_SIZE:
			{
				// 再配置
				int dlg_w, dlg_h;

				GetClientRect(hDlgWnd,                                 &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
				GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);
				GetWindowRect(GetDlgItem(hDlgWnd, IDCANCEL),           &rc_cancel);

				// OK
				p.x = rc_ok.left;
				p.y = rc_ok.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDOK), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// CANCEL
				p.x = rc_cancel.left;
				p.y = rc_cancel.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDCANCEL), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// EDIT
				p.x = rc_edit.left;
				p.y = rc_edit.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_EDIT), 0,
				             0, 0, dlg_w - p.x - edit2ok - ok2right, dlg_h - p.y - edit2bottom,
				             SWP_NOMOVE | SWP_NOZORDER);

				// サイズを保存
				GetWindowRect(hDlgWnd, &rc_dlg);
				ts.PasteDialogSize.cx = rc_dlg.right - rc_dlg.left;
				ts.PasteDialogSize.cy = rc_dlg.bottom - rc_dlg.top;

				// status bar
				SendMessage(hStatus , msg , wp , lp);
			}
			return TRUE;

		case WM_GETMINMAXINFO:
			{
				// ダイアログの初期サイズより小さくできないようにする
				LPMINMAXINFO lpmmi;
				lpmmi = (LPMINMAXINFO)lp;
				lpmmi->ptMinTrackSize.x = init_width;
				lpmmi->ptMinTrackSize.y = init_height;
			}
			return FALSE;

		default:
			return FALSE;
	}
}

INT_PTR clipboarddlg(
	HINSTANCE hInstance,
	HWND hWndParent,
	clipboarddlgdata *data)
{
	INT_PTR ret;
	ret = TTDialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CLIPBOARD_DIALOG),
						   hWndParent, OnClipboardDlgProc, (LPARAM)data);
	return ret;
}
