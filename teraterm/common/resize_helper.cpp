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
#include <stdlib.h>
#include <assert.h>

#include "resize_helper.h"

typedef struct Controls_st
{
	UINT id;
	int type;
	int anchor;
} Controls;

typedef struct ReiseDlgHelper_st
{
	Controls *control_list;
	int control_count;
	HWND hWnd;
	LONG init_width;
	LONG init_height;
	LONG window_width;
	LONG window_height;
	HWND hWndSizeBox;
} ReiseDlgHelper_t;

static void SetSizeBoxPos(ReiseDlgHelper_t *h)
{
	RECT rect;
	GetClientRect(h->hWnd, &rect);
	int width = GetSystemMetrics(SM_CXVSCROLL);
	int height = GetSystemMetrics(SM_CYHSCROLL);
	int x = rect.right - width;
	int y = rect.bottom - height;
	SetWindowPos(h->hWndSizeBox, NULL,
				 x, y, width, height,
				 SWP_NOZORDER|SWP_SHOWWINDOW);
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
	h->window_width = h->init_width;
	h->window_height = h->init_height;
	h->hWnd = dlg;
	h->control_list = NULL;
	h->control_count = 0;

	if (size_box) {
		HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(dlg, GWLP_HINSTANCE);
		// サイズボックスを作成
		// SBS_SIZEBOX = SBS_SIZEGRIP
		h->hWndSizeBox =
			CreateWindowExW(
				0, L"SCROLLBAR", NULL,
				WS_CHILD | WS_VISIBLE | SBS_SIZEBOX | WS_DISABLED,
				10, 10, 10, 10,
				dlg, NULL, hInstance, NULL);
		SetSizeBoxPos(h);
	}
	return h;
}

void ReiseDlgHelperDelete(ReiseDlgHelper_t *h)
{
	if (h != NULL) {
		if (h->hWndSizeBox != NULL) {
			DestroyWindow(h->hWndSizeBox);
		}
		free(h->control_list);
		h->control_list = NULL;
		free(h);
	}
}

void ReiseDlgHelperAdd(ReiseDlgHelper_t *h, UINT id, ResizeHelperAnchor anchor)
{
	Controls *p;
	assert(h != NULL);
	Controls *ary = (Controls *)realloc(h->control_list, sizeof(Controls) * (h->control_count + 1));
	if (ary == NULL) {
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
	h->control_list = ary;
	p = &ary[h->control_count];
	h->control_count++;
	p->id = id;
	p->anchor = anchor;
}

void ReiseDlgHelper_WM_SIZE(ReiseDlgHelper_t *h)
{
	int new_width;
	int new_height;
	int delta_width;
	int delta_height;
	RECT rect;
	assert(h != NULL);
	GetWindowRect(h->hWnd, &rect);
	new_width = rect.right - rect.left;
	new_height = rect.bottom - rect.top;
	delta_width = new_width - h->window_width;
	delta_height = new_height - h->window_height;

	const Controls *p = h->control_list;
	int n = h->control_count;
	while(n-- != 0) {
		HWND item = GetDlgItem(h->hWnd, p->id);
		POINT lt;
		int width;
		int height;

		GetWindowRect(item, &rect);
		lt.x = rect.left;
		lt.y = rect.top;
		ScreenToClient(h->hWnd, &lt);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;

		if ((p->anchor & RESIZE_HELPER_ANCHOR_NONE_H) != 0) {
			// 1/2して加えるので奇数サイズのリサイズが発生すると
			// 少しづつ位置がずれる
			lt.x += delta_width / 2;
		} else if ((p->anchor & RESIZE_HELPER_ANCHOR_LR) == RESIZE_HELPER_ANCHOR_LR) {
			width += delta_width;
		} else if (p->anchor & RESIZE_HELPER_ANCHOR_RIGHT) {
			lt.x += delta_width;
		}
		if ((p->anchor & RESIZE_HELPER_ANCHOR_NONE_V) != 0) {
			// 1/2して加えるので奇数サイズのリサイズが発生すると
			// 少しづつ位置がずれる
			lt.y += delta_height / 2;
		} else if ((p->anchor & RESIZE_HELPER_ANCHOR_TB) == RESIZE_HELPER_ANCHOR_TB) {
			height += delta_height;
		} else if (p->anchor & RESIZE_HELPER_ANCHOR_BOTTOM) {
			lt.y += delta_height;
		}
		MoveWindow(item, lt.x, lt.y, width, height, FALSE);

		p++;
	}
	h->window_height = new_height;
	h->window_width = new_width;

	if (h->hWndSizeBox != NULL) {
		SetSizeBoxPos(h);
	}

	// 移動しているときにコントロールが重なると表示が乱れることがある
	// 全体を再描画する
	InvalidateRect(h->hWnd, NULL, TRUE);
}

/**
 *	WM_GETMINMAXINFO を処理
 */
INT_PTR ReiseDlgHelper_WM_GETMINMAXINFO(ReiseDlgHelper_t *h, LPARAM lp)
{
	assert(h != NULL);
	// 初期サイズを最小サイズとする
	LPMINMAXINFO pmmi = (LPMINMAXINFO)lp;
	pmmi->ptMinTrackSize.x = h->init_width;
	pmmi->ptMinTrackSize.y = h->init_height;
	return 0;
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
