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

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "ttwinman.h"

#include <windows.h>
#include <stdio.h>

//
// リサイズツールチップ (based on PuTTY sizetip.c)
//
static ATOM tip_class = 0;
static HFONT tip_font;
static COLORREF tip_bg;
static COLORREF tip_text;
static HWND tip_wnd = NULL;
static int tip_enabled = 0;

static LRESULT CALLBACK SizeTipWndProc(HWND hWnd, UINT nMsg,
                                       WPARAM wParam, LPARAM lParam)
{

	switch (nMsg) {
		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			{
				HBRUSH hbr;
				HGDIOBJ holdbr;
				RECT cr;
				int wtlen;
				LPTSTR wt;
				HDC hdc;

				PAINTSTRUCT ps;
				hdc = BeginPaint(hWnd, &ps);

				SelectObject(hdc, tip_font);
				SelectObject(hdc, GetStockObject(BLACK_PEN));

				hbr = CreateSolidBrush(tip_bg);
				holdbr = SelectObject(hdc, hbr);

				GetClientRect(hWnd, &cr);
				Rectangle(hdc, cr.left, cr.top, cr.right, cr.bottom);

				wtlen = GetWindowTextLength(hWnd);
				wt = (LPTSTR) malloc((wtlen + 1) * sizeof(TCHAR));
				GetWindowText(hWnd, wt, wtlen + 1);

				SetTextColor(hdc, tip_text);
				SetBkColor(hdc, tip_bg);

				TextOut(hdc, cr.left + 3, cr.top + 3, wt, wtlen);

				free(wt);

				SelectObject(hdc, holdbr);
				DeleteObject(hbr);

				EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_DESTROY:
			DeleteObject(tip_font);
			tip_font = NULL;
			break;

		case WM_SETTEXT:
			{
				LPCTSTR str = (LPCTSTR) lParam;
				SIZE sz;
				HDC hdc = CreateCompatibleDC(NULL);

				SelectObject(hdc, tip_font);
				GetTextExtentPoint32(hdc, str, strlen(str), &sz);

				SetWindowPos(hWnd, NULL, 0, 0, sz.cx + 6, sz.cy + 6,
				             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
				InvalidateRect(hWnd, NULL, FALSE);

				DeleteDC(hdc);
			}
			break;
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

void UpdateSizeTip(HWND src, int cx, int cy)
{
	TCHAR str[32];

	if (!tip_enabled)
		return;

	if (!tip_wnd) {
		NONCLIENTMETRICS nci;

		/* First make sure the window class is registered */

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
			wc.lpszClassName = "SizeTipClass";

			tip_class = RegisterClass(&wc);
		}
#if 0
		/* Default values based on Windows Standard color scheme */

		tip_font = GetStockObject(SYSTEM_FONT);
		tip_bg = RGB(255, 255, 225);
		tip_text = RGB(0, 0, 0);
#endif

		/* Prepare other GDI objects and drawing info */

		tip_bg = GetSysColor(COLOR_INFOBK);
		tip_text = GetSysColor(COLOR_INFOTEXT);

		memset(&nci, 0, sizeof(NONCLIENTMETRICS));
		nci.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
			sizeof(NONCLIENTMETRICS), &nci, 0);
		tip_font = CreateFontIndirect(&nci.lfStatusFont);
	}

	/* Generate the tip text */

	sprintf(str, "%dx%d", cx, cy);

	if (!tip_wnd) {
		HDC hdc;
		SIZE sz;
		RECT wr;
		int ix, iy;
		HMONITOR hm;

		/* calculate the tip's size */

		hdc = CreateCompatibleDC(NULL);
		GetTextExtentPoint32(hdc, str, strlen(str), &sz);
		DeleteDC(hdc);

		GetWindowRect(src, &wr);

		ix = wr.left;
		iy = wr.top - sz.cy;

		if (HasMultiMonitorSupport()) {
			// マルチモニタがサポートされている場合
			POINT p;
			MONITORINFO mi;

			p.x = ix;
			p.y = iy;

			hm = MonitorFromPoint(p, MONITOR_DEFAULTTONULL);

			if (hm == NULL) {
#if 1
				// ツールチップがスクリーンからはみ出している場合はマウスのあるモニタに表示する
				GetCursorPos(&p);
				hm = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);
#else
				// ツールチップがスクリーンからはみ出している場合は最も近いモニタに表示する
				hm = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);
#endif
			}

			mi.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(hm, &mi);
			if (ix < mi.rcMonitor.left + 16) {
				ix = mi.rcMonitor.left + 16;
			}
			if (iy < mi.rcMonitor.top + 16) {
				iy = mi.rcMonitor.top + 16;
			}
		}
		else {
			// マルチモニタがサポートされていない場合
			if (ix < 16) {
				ix = 16;
			}
			if (iy < 16) {
				iy = 16;
			}
		}

		/* Create the tip window */

		tip_wnd =
			CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
				MAKEINTRESOURCE(tip_class), str, WS_POPUP, ix,
				iy, sz.cx, sz.cy, NULL, NULL, hInst, NULL);

		ShowWindow(tip_wnd, SW_SHOWNOACTIVATE);

	} else {

		/* Tip already exists, just set the text */

		SetWindowText(tip_wnd, str);
	}
}

void EnableSizeTip(int bEnable)
{
	if (tip_wnd && !bEnable) {
		DestroyWindow(tip_wnd);
		tip_wnd = NULL;
	}

	tip_enabled = bEnable;
}
