/*
 * (C) 2019- TeraTerm Project
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
#include "tt_res.h"
#include "compat_win.h"
#include "win32helper.h"

#include "clipboarddlg.h"

typedef struct {
	clipboarddlgdata *data;
	int init_width;
	int init_height;
	HWND hStatus;
	int ok2right;
	int edit2ok;
	int edit2bottom;
} DlgPrivateData;

static void TTGetCaretPos(HWND hDlgWnd, POINT *p)
{
	if (ActiveWin == IdVT) { // VT Window
		/*
		 * Caret off 時に GetCaretPos() で正確な場所が取れないので、
		 * vtdisp.c 内部で管理している値から計算する
		 */
		int x, y;
		DispConvScreenToWin(CursorX, CursorY, &x, &y);
		p->x = x;
		p->y = y;
	}
	else if (!GetCaretPos(p)) { // Tek Window
		/*
		 * Tek Window は内部管理の値を取るのが面倒なので GetCaretPos() を使う
		 * GetCaretPos() がエラーになった場合は念のため 0, 0 を入れておく
		 */
		p->x = 0;
		p->y = 0;
	}

	// x, y の両方が 0 の時は親ウィンドウの中央に移動させられるので、
	// それを防ぐ為に x を 1 にする
	if (p->x == 0 && p->y == 0) {
		p->x = 1;
	}

	ClientToScreen(GetParent(hDlgWnd), p);
}

static INT_PTR CALLBACK OnClipboardDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_CLIPBOARD_TITLE" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDOK, "BTN_OK" },
	};
	RECT rc_dlg;
	RECT rc_edit, rc_ok, rc_cancel;
	DlgPrivateData *pdata = (DlgPrivateData *)GetWindowLongPtr(hDlgWnd, DWLP_USER);

	switch (msg) {
		case WM_INITDIALOG:
			pdata = (DlgPrivateData *)lp;
			SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)pdata);
			SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), pdata->data->UILanguageFileW);

			SetDlgItemTextW(hDlgWnd, IDC_EDIT, pdata->data->strW_ptr);

			// リサイズアイコンを右下に表示させたいので、ステータスバーを付ける。
			InitCommonControls();
			pdata->hStatus = CreateStatusWindow(
				WS_CHILD | WS_VISIBLE |
				CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hDlgWnd, 1);

			// ダイアログのサイズ(初期値)
			GetWindowRect(hDlgWnd, &rc_dlg);
			pdata->init_width = rc_dlg.right-rc_dlg.left;
			pdata->init_height = rc_dlg.bottom-rc_dlg.top;

			// 現在サイズから必要な値を計算
			GetClientRect(hDlgWnd,                                 &rc_dlg);
			GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
			GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);

			POINT p;
			p.x = rc_dlg.right;
			p.y = rc_dlg.bottom;
			ClientToScreen(hDlgWnd, &p);
			pdata->ok2right = p.x - rc_ok.left;
			pdata->edit2bottom = p.y - rc_edit.bottom;
			pdata->edit2ok = rc_ok.left - rc_edit.right;

			// サイズを復元
			SetWindowPos(hDlgWnd, NULL, 0, 0,
			             pdata->data->PasteDialogSize.cx, pdata->data->PasteDialogSize.cy,
			             SWP_NOZORDER | SWP_NOMOVE);

			// 位置移動
			POINT CaretPos;
			TTGetCaretPos(hDlgWnd, &CaretPos);
			SetWindowPos(hDlgWnd, NULL, CaretPos.x, CaretPos.y,
			             0, 0, SWP_NOSIZE | SWP_NOZORDER);

			// 画面からはみ出さないよう移動
			MoveWindowToDisplayPoint(hDlgWnd, &CaretPos);
			//MoveWindowToDisplay(hDlgWnd);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
				{
					INT_PTR result = IDCANCEL;

					wchar_t *strW;
					DWORD error = hGetDlgItemTextW(hDlgWnd, IDC_EDIT, &strW);
					if (error == NO_ERROR) {
						pdata->data->strW_edited_ptr = strW;
						result = IDOK;
					}

					TTEndDialog(hDlgWnd, result);
				}
					break;

				case IDCANCEL:
					TTEndDialog(hDlgWnd, IDCANCEL);
					break;

				default:
					return FALSE;
			}
			return TRUE;

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
				             dlg_w - pdata->ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// CANCEL
				p.x = rc_cancel.left;
				p.y = rc_cancel.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDCANCEL), 0,
				             dlg_w - pdata->ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// EDIT
				p.x = rc_edit.left;
				p.y = rc_edit.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_EDIT), 0,
				             0, 0, dlg_w - p.x - pdata->edit2ok - pdata->ok2right, dlg_h - p.y - pdata->edit2bottom,
				             SWP_NOMOVE | SWP_NOZORDER);

				// サイズを保存
				GetWindowRect(hDlgWnd, &rc_dlg);
				pdata->data->PasteDialogSize.cx = rc_dlg.right - rc_dlg.left;
				pdata->data->PasteDialogSize.cy = rc_dlg.bottom - rc_dlg.top;

				// status bar
				SendMessage(pdata->hStatus , msg , wp , lp);
			}
			return TRUE;

		case WM_GETMINMAXINFO:
			{
				// ダイアログの初期サイズより小さくできないようにする
				LPMINMAXINFO lpmmi = (LPMINMAXINFO)lp;
				lpmmi->ptMinTrackSize.x = pdata->init_width;
				lpmmi->ptMinTrackSize.y = pdata->init_height;
			}
			return FALSE;

		case WM_DESTROY:
			DestroyWindow(pdata->hStatus);
			return 0;

		default:
			return FALSE;
	}
}

INT_PTR clipboarddlg(
	HINSTANCE hInstance,
	HWND hWndParent,
	clipboarddlgdata *data)
{
	DlgPrivateData *pdata = (DlgPrivateData * )calloc(1, sizeof(DlgPrivateData));
	INT_PTR ret;
	pdata->data = data;
	ret = TTDialogBoxParam(hInstance, MAKEINTRESOURCEW(IDD_CLIPBOARD_DIALOG),
						   hWndParent, OnClipboardDlgProc, (LPARAM)pdata);
	free(pdata);
	return ret;
}
