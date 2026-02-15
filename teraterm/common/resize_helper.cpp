/*
 * Copyright (C) 2023- TeraTerm Project
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
#include <assert.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "ttlib.h"		// for GetMonitorDpiFromWindow()

#include "resize_helper.h"

typedef struct Controls_st {
	UINT id;
	int type;
	ResizeHelperAnchor anchor;
	RECT rect;			// 初期化時の初期クライアント座標
} Controls;

typedef struct ReiseDlgHelper_st {
	Controls *control_list;
	int control_count;
	HWND hWnd;					// 初期化時の制御するダイアログのハンドル
	LONG init_width;			// 初期化時のウィンドウサイズ
	LONG init_height;
	LONG init_c_width;			// 初期化時のクライアントエリア
	LONG init_c_height;
	UINT init_dpi;				// 初期化時のDPI
	UINT dpi;					// 現在のDPI
	LONG c_width;				// クライアントエリアのサイズ
	LONG c_height;
	HWND sizebox_hWnd;			// サイズボックスのハンドル
	WNDPROC sizebox_prev_proc;	// 元のウィンドウプロシージャ
	LONG sizebox_width;
	LONG sizebox_height;
} ReiseDlgHelper_t;

static LRESULT CALLBACK SizeBoxWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	ReiseDlgHelper_t *h = (ReiseDlgHelper_t *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	switch (msg) {
	case WM_SETCURSOR: {
		HCURSOR hc = LoadCursorA(NULL, IDC_SIZENWSE);
		if (hc != NULL) {
			SetCursor(hc);
			return TRUE;
		}
		break;
	}
	case WM_DESTROY: {
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)h->sizebox_prev_proc);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		h->sizebox_prev_proc = NULL;
		break;
	}
	}
	if (h != NULL && h->sizebox_prev_proc != NULL) {
		return CallWindowProcW(h->sizebox_prev_proc, hwnd, msg, wp, lp);
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

static void SizeBoxSetPos(ReiseDlgHelper_t *h)
{
	RECT rect;
	GetClientRect(h->hWnd, &rect);
	int x = rect.right - h->sizebox_width;
	int y = rect.bottom - h->sizebox_height;
	SetWindowPos(h->sizebox_hWnd, NULL, x, y, h->sizebox_width, h->sizebox_height,
				 SWP_NOZORDER | SWP_SHOWWINDOW);
}

// サイズボックスを作成
static void SizeBoxCreate(ReiseDlgHelper_t *h)
{
	h->sizebox_width = GetSystemMetricsForDpi(SM_CXVSCROLL, h->dpi);
	h->sizebox_height = GetSystemMetricsForDpi(SM_CYHSCROLL, h->dpi);

	HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(h->hWnd, GWLP_HINSTANCE);
	h->sizebox_hWnd =
		CreateWindowExW(
			0, L"SCROLLBAR", NULL,
			WS_CHILD | WS_VISIBLE | SBS_SIZEBOX,		// SBS_SIZEBOX = SBS_SIZEGRIP
			10, 10, 10, 10,
			h->hWnd, NULL, hInstance, NULL);
	if (h->sizebox_hWnd == NULL) {
		return;
	}

	SetWindowLongPtrW(h->sizebox_hWnd, GWLP_USERDATA, (LONG_PTR)h);
	h->sizebox_prev_proc = (WNDPROC)SetWindowLongPtrW(h->sizebox_hWnd, GWLP_WNDPROC, (LONG_PTR)SizeBoxWndProc);

	SizeBoxSetPos(h);
}

static void ArrangeControls(ReiseDlgHelper_t *h)
{
	const LONG current_c_width = h->c_width;
	const LONG current_c_height = h->c_height;

	// クライアントエリアのサイズ
	if (h->init_c_width == 0 || h->init_c_height == 0) {
		return;
	}
	const LONG scaled_init_c_width = MulDiv(h->init_c_width, h->dpi, h->init_dpi);
	const LONG scaled_init_c_height = MulDiv(h->init_c_height, h->dpi, h->init_dpi);

	HDWP hdwp = BeginDeferWindowPos(h->control_count);
	if (hdwp == NULL) {
		return;
	}

	for (int i = 0; i < h->control_count; i++) {
		Controls *p = &h->control_list[i];
		HWND item = GetDlgItem(h->hWnd, p->id);
		if (item == NULL) {
			// コントロールがなくなった?
			continue;
		}

		// 初期コントロール矩形を現在のDPIにスケーリング
		RECT new_rect;
		new_rect.left = MulDiv(p->rect.left, h->dpi, h->init_dpi);
		new_rect.top = MulDiv(p->rect.top, h->dpi, h->init_dpi);
		new_rect.right = MulDiv(p->rect.right, h->dpi, h->init_dpi);
		new_rect.bottom = MulDiv(p->rect.bottom, h->dpi, h->init_dpi);

		LONG new_width = new_rect.right - new_rect.left;
		LONG new_height = new_rect.bottom - new_rect.top;

		// 変更量
		const int delta_width = current_c_width - scaled_init_c_width;
		const int delta_height = current_c_height - scaled_init_c_height;

		if ((p->anchor & RESIZE_HELPER_ANCHOR_NONE_H) != 0) {
			new_rect.left += delta_width / 2;
		} else if ((p->anchor & RESIZE_HELPER_ANCHOR_LR) == RESIZE_HELPER_ANCHOR_LR) {
			new_width += delta_width;
		} else if (p->anchor & RESIZE_HELPER_ANCHOR_RIGHT) {
			new_rect.left += delta_width;
		}

		if ((p->anchor & RESIZE_HELPER_ANCHOR_NONE_V) != 0) {
			new_rect.top += delta_height / 2;
		} else if ((p->anchor & RESIZE_HELPER_ANCHOR_TB) == RESIZE_HELPER_ANCHOR_TB) {
			new_height += delta_height;
		} else if (p->anchor & RESIZE_HELPER_ANCHOR_BOTTOM) {
			new_rect.top += delta_height;
		}

		DeferWindowPos(hdwp, item, NULL, new_rect.left, new_rect.top, new_width, new_height, SWP_NOZORDER | SWP_NOACTIVATE);
	}
	EndDeferWindowPos(hdwp);

	// 移動しているときにコントロールが重なると表示が乱れることがある
	// 全体を再描画する
	InvalidateRect(h->hWnd, NULL, TRUE);
}

ReiseDlgHelper_t *ReiseDlgHelperCreate(HWND dlg, BOOL size_box)
{
	RECT rect;
	ReiseDlgHelper_t *h = (ReiseDlgHelper_t *)calloc(1, sizeof(*h));
	if (h == NULL) {
		return NULL;
	}
	LONG_PTR style = GetWindowLongPtrW(dlg, GWL_STYLE);
	if ((style & WS_THICKFRAME) == 0) {
		// WS_THICKFRAME (=WS_SIZEBOX) スタイルがないと resizeできない
		assert(FALSE);
	}
	GetWindowRect(dlg, &rect);
	h->init_width = rect.right - rect.left;
	h->init_height = rect.bottom - rect.top;
	h->hWnd = dlg;
	h->control_list = NULL;
	h->control_count = 0;

	GetClientRect(dlg, &rect);
	h->init_c_width = rect.right - rect.left;
	h->init_c_height = rect.bottom - rect.top;

	UINT dpi = GetMonitorDpiFromWindow(h->hWnd);
	h->init_dpi = dpi;
	h->dpi = dpi;

	if (size_box) {
		SizeBoxCreate(h);
	}
	return h;
}

void ReiseDlgHelperDelete(ReiseDlgHelper_t *h)
{
	if (h != NULL) {
		if (h->sizebox_hWnd != NULL) {
			DestroyWindow(h->sizebox_hWnd);
		}
		free(h->control_list);
		h->control_list = NULL;
		free(h);
	}
}

void ReiseDlgHelperAdd(ReiseDlgHelper_t *h, UINT id, ResizeHelperAnchor anchor)
{
	assert(h != NULL);
	Controls *p = (Controls *)realloc(h->control_list, sizeof(Controls) * (h->control_count + 1));
	if (p == NULL) {
		return;
	}
	HWND item = GetDlgItem(h->hWnd, id);
	if (item == NULL) {
		assert(item != NULL);	// 存在しないIDが指定されている
		return;
	}
#if _DEBUG
	{
		char ClassName[32];
		GetClassNameA(item, ClassName, _countof(ClassName));
		if (strcmp(ClassName, "ListBox") == 0) {
			// ListBox は LBS_NOINTEGRALHEIGHT 属性が必要
			LONG_PTR r = GetWindowLongPtrW(item, GWL_STYLE);
			assert((r & LBS_NOINTEGRALHEIGHT) != 0);
		}
	}
#endif
	h->control_list = p;
	p = &h->control_list[h->control_count];
	h->control_count++;
	p->id = id;
	p->anchor = anchor;

	// クライアント座標で保存
	RECT item_rect;
	GetWindowRect(item, &item_rect);
	MapWindowPoints(HWND_DESKTOP, h->hWnd, (LPPOINT)&item_rect, 2);
	p->rect = item_rect;
}

void ReiseDlgHelper_WM_SIZE(ReiseDlgHelper_t *h, WPARAM wp, LPARAM lp)
{
	assert(h != NULL);

	if (wp == SIZE_MINIMIZED) {
		return;
	}

	UINT dpi = GetMonitorDpiFromWindow(h->hWnd);
	if (h->dpi != dpi) {
		// DPIが変化したとき、WM_DPICHANGEDが発生する前に WM_SIZE が発生する(Per Monitor v2時?)
		// WM_DPICHANGED前の WM_SIZE は処理せず、WM_DPICHANGED後に行う
		//   * WM_SIZE 前に、WM_GETMINMAXINFO でサイズの上下限チェックが行われる
		return;
	}

	h->c_width = LOWORD(lp);
	h->c_height = HIWORD(lp);

	ArrangeControls(h);
	if (h->sizebox_hWnd != NULL) {
		SizeBoxSetPos(h);
	}
}

/**
 *	WM_GETMINMAXINFO を処理
 */
void ReiseDlgHelper_WM_GETMINMAXINFO(ReiseDlgHelper_t *h, LPARAM lp)
{
	assert(h != NULL);

	// 初期サイズを最小サイズとする
	// 現在のDPIに合わせてスケーリング
	LPMINMAXINFO pmmi = (LPMINMAXINFO)lp;
	pmmi->ptMinTrackSize.x = MulDiv(h->init_width, h->dpi, h->init_dpi);
	pmmi->ptMinTrackSize.y = MulDiv(h->init_height, h->dpi, h->init_dpi);
}

void ReiseDlgHelper_WM_DPICHANGED(ReiseDlgHelper_t *h, WPARAM wp, LPARAM lp)
{
	assert(h != NULL);
	UINT new_dpi = LOWORD(wp);
	//OutputDebugPrintf("WM_DPICHANGED dpi %d->%d\n", h->dpi, new_dpi);
	h->dpi = new_dpi;

	// SetWindowPos() を行って再度 WM_SIZE を発生、コントロールの調整を行う
	const RECT *rect = (RECT *)lp;		// 推奨されるサイズと位置
	SetWindowPos(h->hWnd,
		NULL,
		rect->left,
		rect->top,
		rect->right - rect->left,
		rect->bottom - rect->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	if (h->sizebox_hWnd != NULL) {
		// サイズボックスサイズを更新
		h->sizebox_width = GetSystemMetricsForDpi(SM_CXVSCROLL, h->dpi);
		h->sizebox_height = GetSystemMetricsForDpi(SM_CYHSCROLL, h->dpi);
	}
}

ReiseDlgHelper_t *ReiseHelperInit(HWND dlg, BOOL size_box, const ResizeHelperInfo *infos, size_t info_count)
{
	ReiseDlgHelper_t *h = ReiseDlgHelperCreate(dlg, size_box);
	if (h == NULL) {
		return NULL;
	}

	const ResizeHelperInfo *p = infos;
	for (size_t i = 0; i < info_count; i++) {
		ReiseDlgHelperAdd(h, p->id, p->anchor);
		p++;
	}

	return h;
}

BOOL resizeDlgHelperProc(ReiseDlgHelper_t *h, HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	assert(h != NULL);
	assert(h->hWnd == hWnd);
	(void)hWnd;

	switch (msg) {
	case WM_GETMINMAXINFO:
		ReiseDlgHelper_WM_GETMINMAXINFO(h, lp);
		return FALSE;	// 処理済み

	case WM_SIZE:
		ReiseDlgHelper_WM_SIZE(h, wp, lp);
		return FALSE;

	case WM_DPICHANGED:
		ReiseDlgHelper_WM_DPICHANGED(h, wp, lp);
		return FALSE;
	}
	return FALSE;
}
