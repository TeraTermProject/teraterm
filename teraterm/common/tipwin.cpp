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
 * Copyright (C) 2008-2019 TeraTerm Project
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

#ifdef _WIN64
        typedef LONG_PTR WINDOW_LONG_PTR;
#else
        typedef LONG WINDOW_LONG_PTR;
#endif

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

VOID CTipWin::CalcStrRect(VOID)
{
	HDC hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, tWin->tip_font);
	tWin->str_rect.top = 0;
	tWin->str_rect.left = 0;
	DrawText(hdc, tWin->str, tWin->str_len,
			 &tWin->str_rect, DT_LEFT|DT_CALCRECT);
	DeleteDC(hdc);
}

LRESULT CALLBACK CTipWin::WndProc(HWND hWnd, UINT nMsg,
                                       WPARAM wParam, LPARAM lParam)
{
	CTipWin* self = (CTipWin *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (nMsg) {
		case WM_CREATE: {
			CREATESTRUCTA *create_st = (CREATESTRUCTA *)lParam;
			self = (CTipWin *)create_st->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
			self->tWin->tip_wnd = hWnd;
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

				SelectObject(hdc, self->tWin->tip_font);
				SelectObject(hdc, GetStockObject(BLACK_PEN));

				hbr = CreateSolidBrush(self->tWin->tip_bg);
				holdbr = SelectObject(hdc, hbr);

				GetClientRect(hWnd, &cr);
				Rectangle(hdc, cr.left, cr.top, cr.right, cr.bottom);

				SetTextColor(hdc, self->tWin->tip_text);
				SetBkColor(hdc, self->tWin->tip_bg);

				{
					RECT rect = self->tWin->str_rect;
					rect.left = rect.left + FRAME_WIDTH;
					rect.right = rect.right + FRAME_WIDTH;
					rect.top = rect.top + FRAME_WIDTH;
					rect.bottom = rect.bottom + FRAME_WIDTH;
					DrawText(hdc, self->tWin->str, self->tWin->str_len, &rect, DT_LEFT);
				}

				SelectObject(hdc, holdbr);
				DeleteObject(hbr);

				EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_DESTROY:
			if(self->IsExists()) {
				DeleteObject(self->tWin->tip_font);
				self->tWin->tip_font = NULL;
			}
			break;

		case WM_SETTEXT:
			{
				LPCTSTR str = (LPCTSTR) lParam;
				const int str_width = self->tWin->str_rect.right - self->tWin->str_rect.left;
				const int str_height = self->tWin->str_rect.bottom - self->tWin->str_rect.top;

				free((void *)(self->tWin->str));
				self->tWin->str_len = _tcslen(str);
				self->tWin->str = _tcsdup(str);
				self->CalcStrRect();

				SetWindowPos(hWnd, NULL,
							 0, 0,
							 str_width + FRAME_WIDTH * 2, str_height + FRAME_WIDTH * 2,
				             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
				InvalidateRect(hWnd, NULL, FALSE);

			}
			break;

		case WM_NCDESTROY:
			if (self->IsExists() && self->tWin->auto_destroy) {
				free((void *)self->tWin->str);
				free(self->tWin);
				/*
				 * use-after-freeによりTera Termの動作が不安定となる問題を修正した。
				 *
				 * WinMainで CVTWindow クラスのコンストラクタで、アロケートした
				 * TipWinメンバーを、ここのタイミングで解放していたため。
				 * 正しくは CVTWindow クラスのデストラクタで解放する。
				 */
			}
			break;
		case WM_TIMER:
			KillTimer(hWnd, self->timerid);
			self->timerid = NULL;
			self->SetVisible(FALSE);
			break;
		default:
			break;
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

CTipWin::CTipWin(HWND src)
{
	Create(src, 0, 0, "");
	SetVisible(FALSE);
}

CTipWin::CTipWin(HWND src, int cx, int cy, const TCHAR *str)
{
	Create(src, cx, cy, str);
	SetVisible(TRUE);
}

CTipWin::~CTipWin()
{
	Destroy();
}

ATOM CTipWin::tip_class;

VOID CTipWin::Create(HWND src, int cx, int cy, const TCHAR *str)
{
	const HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(src, GWLP_HINSTANCE);
	LOGFONTA logfont;
	const UINT uDpi = GetMonitorDpiFromWindow(src);

	if (!tip_class) {
		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = _T("TipWinClass");

		tip_class = RegisterClass(&wc);
	}

	tWin = (TipWin *)malloc(sizeof(TipWin));
	if (tWin == NULL) return;
	tWin->str_len = _tcslen(str);
	tWin->str = _tcsdup(str);
	tWin->px = cx;
	tWin->py = cy;
	tWin->tip_bg = GetSysColor(COLOR_INFOBK);
	tWin->tip_text = GetSysColor(COLOR_INFOTEXT);
	GetMessageboxFont(&logfont);
	logfont.lfWidth = MulDiv(logfont.lfWidth, uDpi, 96);
	logfont.lfHeight = MulDiv(logfont.lfHeight, uDpi, 96);
	tWin->tip_font = CreateFontIndirect(&logfont);
	CalcStrRect();

	const int str_width = tWin->str_rect.right - tWin->str_rect.left;
	const int str_height = tWin->str_rect.bottom - tWin->str_rect.top;
	tWin->tip_wnd =
		CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
					   MAKEINTRESOURCE(tip_class),
					   str, WS_POPUP,
					   cx, cy,
					   str_width + FRAME_WIDTH * 2, str_height + FRAME_WIDTH * 2,
					   src, NULL, hInst, this);

	/*
	 * WindowsMe(9x)では、SSH認証のダイアログの表示では NULL が返ってくるため、
	 * アサーションをしないようにした。Tera Termのリサイズでは NULL ではないが、
	 * ツールチップが描画されない。
	 */

	tWin->hParentWnd = src;
	tWin->auto_destroy = TRUE;

	pts.x = cx;
	pts.y = cy;
	timerid = NULL;
}

VOID CTipWin::GetTextWidthHeight(HWND src, const TCHAR *str, int *width, int *height)
{
	TipWinGetTextWidthHeight(src, str, width, height);
}

POINT CTipWin::GetPos(void)
{
	return pts;
}

VOID CTipWin::SetPos(int x, int y)
{
	if(IsExists()) {
		pts.x = x;
		pts.y = y;
		SetWindowPos(tWin->tip_wnd, 0, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

	}
}

VOID CTipWin::SetText(TCHAR *str)
{
	if(IsExists()) {
		SetWindowText(tWin->tip_wnd, str);
		// ツールチップのテキストとウィンドウの描画順の関係でテキストを2度描画してツールチップウィンドウをリサイズする
		SetWindowText(tWin->tip_wnd, str);
	}
}

VOID CTipWin::Destroy(void)
{
	if(IsExists()) {
		DestroyWindow(tWin->tip_wnd);
	}
}

VOID CTipWin::SetHideTimer(int ms)
{
	if(IsExists()) {
		HWND hWnd = tWin->tip_wnd;
		timerid = SetTimer(hWnd, NULL, ms, NULL);
	}
}

BOOL CTipWin::IsExists(void)
{
	return (tWin != NULL);
}

VOID CTipWin::SetVisible(BOOL bVisible)
{
	int visible = (bVisible) ? SW_SHOWNOACTIVATE : SW_HIDE;
	if(IsExists()) {
		HWND hWnd = tWin->tip_wnd;
		if(IsWindowVisible(hWnd) != bVisible) {
			ShowWindow(hWnd, visible);
		}
	}
}

BOOL CTipWin::IsVisible(void)
{
	if(IsExists()) {
		HWND hWnd = tWin->tip_wnd;
		return (BOOL)IsWindowVisible(hWnd);
	}
	return FALSE;
}

TipWin* TipWinCreate(HWND src, int cx, int cy, const TCHAR *str)
{
	CTipWin* tipwin = new CTipWin(src, cx, cy, str);
	return (TipWin*)tipwin;
}

/*
 * 文字列をツールチップに描画した時の横と縦のサイズを取得する。
 *
 * [in]
 *   src
 *   str
 * [out]
 *   width
 *   height
 */
void TipWinGetTextWidthHeight(HWND src, const TCHAR *str, int *width, int *height)
{
	LOGFONTA logfont;
	HFONT tip_font;
	UINT uDpi;
	HDC hdc;
	RECT str_rect;
	size_t str_len;

	/* 文字列の長さを計算する */
	str_len = _tcslen(str);
	
	/* DPIを取得する */
	uDpi = GetMonitorDpiFromWindow(src);

	/* フォントを生成する */
	GetMessageboxFont(&logfont);
	logfont.lfWidth = MulDiv(logfont.lfWidth, uDpi, 96);
	logfont.lfHeight = MulDiv(logfont.lfHeight, uDpi, 96);
	tip_font = CreateFontIndirect(&logfont);

	/* 文字列を描画してサイズを求める */
	hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, tip_font);
	str_rect.top = 0;
	str_rect.left = 0;
	DrawText(hdc, str, str_len, &str_rect, DT_LEFT|DT_CALCRECT);
	*width = str_rect.right - str_rect.left;
	*height = str_rect.bottom - str_rect.top;
	DeleteDC(hdc);

	/* フォントを破棄する */
	DeleteObject(tip_font);
}

void TipWinGetPos(TipWin *tWin, int *x, int *y)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	POINT pt = tipwin->GetPos();
	*x = (int)pt.x;
	*y = (int)pt.y;
}

void TipWinSetPos(TipWin *tWin, int x, int y)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	tipwin->SetPos(x, y);
}

void TipWinSetText(TipWin* tWin, TCHAR *str)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	tipwin->SetText(str);
}

void TipWinSetHideTimer(TipWin *tWin, int ms)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	tipwin->SetHideTimer(ms);
}

void TipWinDestroy(TipWin* tWin)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	tipwin->Destroy();
}

int TipWinIsExists(TipWin *tWin)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	return (int)tipwin->IsExists();
}

void TipWinSetVisible(TipWin* tWin, int bVisible)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	tipwin->SetVisible(bVisible);
}

int TipWinIsVisible(TipWin* tWin)
{
	CTipWin* tipwin = (CTipWin*) tWin;
	return (int)tipwin->IsVisible();
}
