// Import from PuTTY 0.60 windows/sizetip.c
/*
 * PuTTY is copyright 1997-2004 Simon Tatham.
 *
 * Portions copyright Robert de Bath, Joris van Rantwijk, Delian
 * Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,
 * Justin Bradford, Ben Harris, Malcolm Smith, Ahmad Khalifa, Markus
 * Kuhn, Colin Watson, Christopher Staite, and CORE SDI S.A.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright (C) 2008-2018 TeraTerm Project
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
/* based on windows/sizetip.c from PuTTY 0.60 */

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>

#include "ttlib.h"		// for GetMessageboxFont()

#include "tipwin.h"

#define	FRAME_WIDTH	6

static ATOM tip_class = 0;

typedef struct tagTipWinData {
	HFONT tip_font;
	COLORREF tip_bg;
	COLORREF tip_text;
	HWND tip_wnd;
	HWND hParentWnd;
	int tip_enabled;
	const TCHAR *str;
	size_t str_len;
	RECT str_rect;
	RECT rect;
	int px;
	int py;
	BOOL auto_destroy;
} TipWin;

/**
 *	point を
 *	スクリーンからはみ出している場合、入るように補正する
 *	NearestMonitor が TRUE のとき、最も近いモニタ
 *	FALSEのとき、マウスのあるモニタに移動させる
 *	ディスプレイの端から FrameWidth(pixel) より離れるようにする
 */
static void FixPosFromFrame(POINT *point, int FrameWidth, BOOL NearestMonitor)
{
	if (HasMultiMonitorSupport()) {
		// マルチモニタがサポートされている場合
		HMONITOR hm;
		MONITORINFO mi;
		int ix, iy;

		// 元の座標を保存しておく
		ix = point->x;
		iy = point->y;

		hm = MonitorFromPoint(*point, MONITOR_DEFAULTTONULL);
		if (hm == NULL) {
			if (NearestMonitor) {
				// 最も近いモニタに表示する
				hm = MonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
			} else {
				// スクリーンからはみ出している場合はマウスのあるモニタに表示する
				GetCursorPos(point);
				hm = MonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
			}
		}

		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hm, &mi);
		if (ix < mi.rcMonitor.left + FrameWidth) {
			ix = mi.rcMonitor.left + FrameWidth;
		}
		if (iy < mi.rcMonitor.top + FrameWidth) {
			iy = mi.rcMonitor.top + FrameWidth;
		}
		
		point->x = ix;
		point->y = iy;
	}
	else
	{
		// マルチモニタがサポートされていない場合
		if (point->x < FrameWidth) {
			point->x = FrameWidth;
		}
		if (point->y < FrameWidth) {
			point->y = FrameWidth;
		}
	}
}

static void CalcStrRect(TipWin *pTipWin)
{
	HDC hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, pTipWin->tip_font);
	pTipWin->str_rect.top = 0;
	pTipWin->str_rect.left = 0;
	DrawText(hdc, pTipWin->str, pTipWin->str_len,
			 &pTipWin->str_rect, DT_LEFT|DT_CALCRECT);
	DeleteDC(hdc);
}

static LRESULT CALLBACK SizeTipWndProc(HWND hWnd, UINT nMsg,
                                       WPARAM wParam, LPARAM lParam)
{
	TipWin *pTipWin = (TipWin *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (nMsg) {
		case WM_CREATE: {
			CREATESTRUCTA *create_st = (CREATESTRUCTA *)lParam;
			pTipWin = (TipWin *)create_st->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pTipWin);
			pTipWin->tip_wnd = hWnd;
			break;
		}

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			{
				HBRUSH hbr;
				HGDIOBJ holdbr;
				RECT cr;
				HDC hdc;

				PAINTSTRUCT ps;
				hdc = BeginPaint(hWnd, &ps);

				SelectObject(hdc, pTipWin->tip_font);
				SelectObject(hdc, GetStockObject(BLACK_PEN));

				hbr = CreateSolidBrush(pTipWin->tip_bg);
				holdbr = SelectObject(hdc, hbr);

				GetClientRect(hWnd, &cr);
				Rectangle(hdc, cr.left, cr.top, cr.right, cr.bottom);

				SetTextColor(hdc, pTipWin->tip_text);
				SetBkColor(hdc, pTipWin->tip_bg);

				{
					RECT rect = pTipWin->str_rect;
					rect.left = rect.left + FRAME_WIDTH;
					rect.right = rect.right + FRAME_WIDTH;
					rect.top = rect.top + FRAME_WIDTH;
					rect.bottom = rect.bottom + FRAME_WIDTH;
					DrawText(hdc, pTipWin->str, pTipWin->str_len, &rect, DT_LEFT);
				}

				SelectObject(hdc, holdbr);
				DeleteObject(hbr);

				EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_DESTROY:
			DeleteObject(pTipWin->tip_font);
			pTipWin->tip_font = NULL;
			break;

		case WM_SETTEXT:
			{
				LPCTSTR str = (LPCTSTR) lParam;
				const int str_width = pTipWin->str_rect.right - pTipWin->str_rect.left;
				const int str_height = pTipWin->str_rect.bottom - pTipWin->str_rect.top;

				free((void *)(pTipWin->str));
				pTipWin->str_len = _tcslen(str);
				pTipWin->str = _tcsdup(str);
				CalcStrRect(pTipWin);

				SetWindowPos(hWnd, NULL,
							 0, 0,
							 str_width + FRAME_WIDTH * 2, str_height + FRAME_WIDTH * 2,
				             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
				InvalidateRect(hWnd, NULL, FALSE);

			}
			break;

		case WM_NCDESTROY:
			if (pTipWin->auto_destroy) {
				free((void *)pTipWin->str);
				free(pTipWin);
			}
			break;
		default:
			break;
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

static void register_class(HINSTANCE hInst)
{
	if (!tip_class) {
		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = SizeTipWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = _T("SizeTipClass");

		tip_class = RegisterClass(&wc);
	}
}

/* Create the tip window */
static void create_tipwin(TipWin *pTipWin, HINSTANCE hInst, int cx, int cy)
{
	const TCHAR *str = pTipWin->str;
	HWND hParnetWnd = pTipWin->hParentWnd;
	const int str_width = pTipWin->str_rect.right - pTipWin->str_rect.left;
	const int str_height = pTipWin->str_rect.bottom - pTipWin->str_rect.top;
	pTipWin->tip_wnd =
		CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
					   MAKEINTRESOURCE(tip_class),
					   str, WS_POPUP,
					   cx, cy,
					   str_width + FRAME_WIDTH * 2, str_height + FRAME_WIDTH * 2,
					   hParnetWnd, NULL, hInst, pTipWin);
	assert(pTipWin->tip_wnd != NULL);
}

TipWin *TipWinCreate(HWND src, int cx, int cy, const TCHAR *str, BOOL resizing_tips)
{
	TipWin *pTipWin;
	const HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(src, GWLP_HINSTANCE);
	LOGFONTA logfont;
	const UINT uDpi = GetMonitorDpiFromWindow(src);
	int height;
	POINT point;

	register_class(hInst);
	pTipWin = (TipWin *)malloc(sizeof(TipWin));
	if (pTipWin == NULL) return NULL;
	pTipWin->str_len = _tcslen(str);
	pTipWin->str = _tcsdup(str);
	pTipWin->px = cx;
	pTipWin->py = cy;
	pTipWin->tip_bg = GetSysColor(COLOR_INFOBK);
	pTipWin->tip_text = GetSysColor(COLOR_INFOTEXT);
	GetMessageboxFont(&logfont);
	logfont.lfWidth = MulDiv(logfont.lfWidth, uDpi, 96);
	logfont.lfHeight = MulDiv(logfont.lfHeight, uDpi, 96);
	pTipWin->tip_font = CreateFontIndirect(&logfont);
	CalcStrRect(pTipWin);
	pTipWin->hParentWnd = src;

	// 文字列の高さから表示位置を調整する。
	if (resizing_tips) {
		height = pTipWin->str_rect.bottom - pTipWin->str_rect.top;
		point.x = cx;
		point.y = cy - (height + FRAME_WIDTH * 2);
		FixPosFromFrame(&point, 16, FALSE);
		cx = point.x;
		cy = point.y;
	}

	create_tipwin(pTipWin, hInst, cx, cy);

	pTipWin->hParentWnd = src;
	pTipWin->auto_destroy = TRUE;
	ShowWindow(pTipWin->tip_wnd, SW_SHOWNOACTIVATE);
	return pTipWin;
}

void TipWinSetPos(int x, int y)
{
}

void TipWinSetText(TipWin *tWin, TCHAR *text)
{
	if (tWin != NULL) {
		HWND tip_wnd = tWin->tip_wnd;
		SetWindowText(tip_wnd, text);
	}
}

void TipWinDestroy(TipWin *tWin)
{
	if (tWin != NULL) {
		HWND tip_wnd = tWin->tip_wnd;
		DestroyWindow(tip_wnd);
	}
}

