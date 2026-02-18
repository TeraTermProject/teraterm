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
#include "compat_win.h"

#include "resize_helper.h"

typedef struct Controls_st {
	UINT id;
	int type;
	ResizeHelperAnchor anchor;
	RECT rect;			// 初期化時の初期クライアント座標
} Controls;

typedef struct ReiseDlgHelper_st {
	// window info
	HWND hWnd;					// 制御するダイアログのハンドル
	UINT dpi;					// 現在のDPI
	LONG c_width;				// クライアントエリアのサイズ
	LONG c_height;
	RECT last_rect;				// ウィンドウ領域
	// resize infos
	Controls *control_list;
	int control_count;
	LONG init_width;			// 初期化時のウィンドウサイズ
	LONG init_height;
	LONG init_c_width;			// 初期化時のクライアントエリア
	LONG init_c_height;
	UINT init_dpi;				// 初期化時のDPI
	// sizebox
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

// サイズボックスの位置とサイズ設定
static void SizeBoxSetPos(ReiseDlgHelper_t *h)
{
	RECT rect;
	GetClientRect(h->hWnd, &rect);
	int x = rect.right - h->sizebox_width;
	int y = rect.bottom - h->sizebox_height;
	SetWindowPos(h->sizebox_hWnd, NULL, x, y, h->sizebox_width, h->sizebox_height,
				 SWP_NOZORDER | SWP_SHOWWINDOW);
}

// サイズボックスのサイズ取得
static void SizeBoxUpdateSize(ReiseDlgHelper_t *h)
{
	if (pGetSystemMetricsForDpi != NULL) {
		h->sizebox_width = pGetSystemMetricsForDpi(SM_CXVSCROLL, h->dpi);
		h->sizebox_height = pGetSystemMetricsForDpi(SM_CYHSCROLL, h->dpi);
	} else {
		h->sizebox_width = GetSystemMetrics(SM_CXVSCROLL);
		h->sizebox_height = GetSystemMetrics(SM_CYHSCROLL);
	}
}

// サイズボックスを作成
static void SizeBoxCreate(ReiseDlgHelper_t *h)
{
	SizeBoxUpdateSize(h);

	HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(h->hWnd, GWLP_HINSTANCE);
	h->sizebox_hWnd =
		CreateWindowExW(
			0, L"SCROLLBAR", NULL,
			WS_CHILD | WS_VISIBLE | SBS_SIZEBOX,		// SBS_SIZEBOX = SBS_SIZEGRIP
			10, 10, h->sizebox_width, h->sizebox_height,
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
	(void)wp;

	assert(h != NULL);

	h->c_width = LOWORD(lp);
	h->c_height = HIWORD(lp);

	if (h->sizebox_hWnd != NULL) {
		SizeBoxSetPos(h);
	}
	ArrangeControls(h);

	// ウィンドウの位置を保存
	RECT rect;
	GetWindowRect(h->hWnd, &rect);
	h->last_rect = rect;
}

/**
 *	WM_GETMINMAXINFO を処理
 *
 *		ウィンドウを小さくなりすぎないようにする
 *
 *		このメッセージで処理すると
 *		「ドラッグ中にウィンドウの内容を表示する=OFF」のときも
 *		自然な枠線表示が行える。
 *
 *		DPIが異なるモニタをまたいで、DPIが変化したとき、
 *		WM_DPICHANGEDの前に
 *		WM_GETMINMAXINFOとWM_SIZEが発生する
 *		このときウィンドウのDPIの扱いをどうするかがむずかしい
 *
 *		そこでWM_WINDOWPOSCHANGINGをつかって小さくなりすぎないようにした
 */
#if 0
void ReiseDlgHelper_WM_GETMINMAXINFO(ReiseDlgHelper_t *h, LPARAM lp)
{
	assert(h != NULL);

	// 初期サイズを最小サイズとする
	// 現在のDPIに合わせてスケーリング
	LPMINMAXINFO pmmi = (LPMINMAXINFO)lp;
	pmmi->ptMinTrackSize.x = MulDiv(h->init_width, h->dpi, h->init_dpi);
	pmmi->ptMinTrackSize.y = MulDiv(h->init_height, h->dpi, h->init_dpi);
}
#endif

void ReiseDlgHelper_WM_WINDOWPOSCHANGING(ReiseDlgHelper_t *h, WPARAM wp, LPARAM lp)
{
	WINDOWPOS* winpos = reinterpret_cast<WINDOWPOS*>(lp);
	(void)wp;

	// DPI に応じて最小サイズを計算
	int minW = MulDiv(h->init_width, h->dpi, h->init_dpi);
	int minH = MulDiv(h->init_height, h->dpi, h->init_dpi);

	// 最小サイズを強制
	if (winpos->cx < minW) {
		int cx_old = winpos->cx;
		winpos->cx = minW;
		if ((winpos->flags & SWP_NOMOVE) == 0) {
			if (h->last_rect.left == winpos->x) {
				// 左端が等しい
				if (h->last_rect.right == winpos->x + winpos->cx) {
					// 左端と右端が等しい(最小サイズをさらに小さくしようとしている)
					// そのまま
				} else {
					// 左端だけが等しい(リサイズ、右端を左に寄せた)
					winpos->x = h->last_rect.left;	// 左に寄せる
				}
			} else if (h->last_rect.right == winpos->x + cx_old) {
				// 右端が等しい(リサイズ、左端を右に寄せた)
				winpos->x = h->last_rect.right - winpos->cx;	// 右に寄せる
			} else {
				// 左端も右端も異なる(移動してきた)
				// そのまま
			}
		}
	}
	if (winpos->cy < minH) {
		int cy_old = winpos->cy;
		winpos->cy = minH;
		if ((winpos->flags & SWP_NOMOVE) == 0) {
			if (h->last_rect.top == winpos->y) {
				// 上端が等しい
				if (h->last_rect.top == winpos->y + winpos->cy) {
					// 上端と下端が等しい(最小サイズをさらに小さくしようとしている)
					// そのまま
				} else {
					// 上端だけが等しい(リサイズ、下端を上に寄せた)
					winpos->y = h->last_rect.top;	// 上に寄せる
				}
			}
			else if (h->last_rect.bottom == winpos->y + cy_old) {
				// 下端が等しい(リサイズ、上端を下に寄せた)
				winpos->y = h->last_rect.bottom - winpos->cy;	// 下に寄せる
			} else {
				// 上端も下端も異なる(移動してきた)
				// そのまま
			}
		}
	}
}

BOOL ReiseDlgHelper_WM_GETDPISCALEDSIZE(ReiseDlgHelper_t *h, WPARAM wp, LPARAM lp)
{
	assert(h != NULL);
	UINT new_dpi = HIWORD(wp);

	RECT client_rect;
	GetClientRect(h->hWnd, &client_rect);
	int tmpScreenWidth = client_rect.right - client_rect.left;
	int tmpScreenHeight = client_rect.bottom - client_rect.top;

	// Client Areaのサイズからウィンドウサイズを算出
	if (pAdjustWindowRectExForDpi != NULL || pAdjustWindowRectEx != NULL) {
		const DWORD Style = (DWORD)::GetWindowLongPtr(h->hWnd, GWL_STYLE);
		const DWORD ExStyle = (DWORD)::GetWindowLongPtr(h->hWnd, GWL_EXSTYLE);
		const BOOL bMenu = FALSE; // メニューは無しの想定
		if (pGetSystemMetricsForDpi != NULL) {
			// スクロールバーが表示されている場合は、
			// スクリーンサイズ(クライアントエリアのサイズ)に追加する
			int min_pos;
			int max_pos;
			GetScrollRange(h->hWnd, SB_VERT, &min_pos, &max_pos);
			if (min_pos != max_pos) {
				tmpScreenWidth += pGetSystemMetricsForDpi(SM_CXVSCROLL, new_dpi);
			}
			GetScrollRange(h->hWnd, SB_HORZ, &min_pos, &max_pos);
			if (min_pos != max_pos) {
				tmpScreenHeight += pGetSystemMetricsForDpi(SM_CXHSCROLL, new_dpi);
			}
		}
		RECT Rect = {0, 0, tmpScreenWidth, tmpScreenHeight};
		if (pAdjustWindowRectExForDpi != NULL) {
			// Windows 10, version 1607+
			pAdjustWindowRectExForDpi(&Rect, Style, bMenu, ExStyle, new_dpi);
		}
		else {
			// Windows 2000+
			pAdjustWindowRectEx(&Rect, Style, bMenu, ExStyle);
		}
		SIZE *sz = (SIZE *)lp;
		sz->cx = Rect.right - Rect.left;
		sz->cy = Rect.bottom - Rect.top;
		return TRUE;
	}
	return FALSE;
}

/**
 *	DPIが変化した
 */
void ReiseDlgHelper_WM_DPICHANGED(ReiseDlgHelper_t *h, WPARAM wp, LPARAM lp)
{
	assert(h != NULL);
	assert(LOWORD(wp) == HIWORD(wp));	// X DPIとY DPI、同じ値になる
	h->dpi = LOWORD(wp);

	if (h->sizebox_hWnd != NULL) {
		// サイズボックスのサイズを更新
		SizeBoxUpdateSize(h);
	}

	// SetWindowPos() を行って再度 WM_SIZE を発生、コントロールの調整を行う
	const RECT *rect = (RECT *)lp;		// 推奨されるサイズと位置
	SetWindowPos(h->hWnd,
				 NULL,
				 rect->left,
				 rect->top,
				 rect->right - rect->left,
				 rect->bottom - rect->top,
				 SWP_NOZORDER | SWP_NOACTIVATE);
}

void ReiseDlgHelper_WM_MOVE(ReiseDlgHelper_t *h, WPARAM wp, LPARAM lp)
{
	assert(h != NULL);
	(void)lp;
	(void)wp;
	RECT rect;
	GetWindowRect(h->hWnd, &rect);
	h->last_rect = rect;
}

ReiseDlgHelper_t *ReiseDlgHelperInit(HWND dlg, BOOL size_box, const ResizeHelperInfo *infos, size_t info_count)
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

BOOL ResizeDlgHelperProc(ReiseDlgHelper_t *h, HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (h == NULL) {
		// WM_INITDIALOG 前、WM_DESTROY後にもメッセージがくる
		// resize helperが使用されていないときは何もしない
		return FALSE;
	}
	assert(h->hWnd == hWnd);
	(void)hWnd;

	switch (msg) {
#if 0
	case WM_GETMINMAXINFO:
		ReiseDlgHelper_WM_GETMINMAXINFO(h, lp);
		return FALSE;
#endif
	case WM_SIZE:
		ReiseDlgHelper_WM_SIZE(h, wp, lp);
		return FALSE;

	case WM_GETDPISCALEDSIZE:
		return ReiseDlgHelper_WM_GETDPISCALEDSIZE(h, wp, lp);

	case WM_DPICHANGED:
		ReiseDlgHelper_WM_DPICHANGED(h, wp, lp);
		return FALSE;

	case WM_WINDOWPOSCHANGING:
		ReiseDlgHelper_WM_WINDOWPOSCHANGING(h, wp, lp);
		return FALSE;

	case WM_MOVE:
		ReiseDlgHelper_WM_MOVE(h, wp,lp);
		return FALSE;
	}
	return FALSE;
}
