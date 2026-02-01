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
 * Copyright (C) 2008- TeraTerm Project
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
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <wchar.h>
#include <assert.h>

#include "ttlib.h"		// for GetMessageboxFont()
#include "codeconv.h"

#include "tipwin.h"

#define TIP_WIN_FRAME_WIDTH 6
#define TipWinClassName L"TeraTermTipWinClass"

class CTipWinImpl
{
public:
	CTipWinImpl(HINSTANCE hInstance);
	~CTipWinImpl();
	VOID Create(HWND pHwnd);
	VOID Destroy();
	VOID SetText(const char *str);
	VOID SetText(const wchar_t *str);
	POINT GetPos();
	VOID SetPos(int x, int y);
	VOID SetHideTimer(int ms);
	BOOL IsExists();
	VOID SetVisible(BOOL bVisible);
	BOOL IsVisible();
	void GetWindowSize(int *width, int *height);
	void GetFrameSize(int *width);
private:
	HINSTANCE hInstance;
	wchar_t class_name[32];
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
	VOID CalcStrRect();
	void CalcWindowRect();
	ATOM RegisterClass();
	BOOL UnregisterClass();
	BOOL IsClassRegistered();
	int win_width_;
	int win_height_;
	HFONT tip_font;
	COLORREF tip_bg;
	COLORREF tip_text;
	HWND tip_wnd;
	const wchar_t *str_;
	size_t str_len;
	RECT str_rect;
	int px;
	int py;
};

VOID CTipWinImpl::CalcStrRect()
{
	HDC hdc = GetDC(tip_wnd);
	SelectObject(hdc, tip_font);
	str_rect.top = 0;
	str_rect.left = 0;
	DrawTextW(hdc, str_, (int)str_len, &str_rect, DT_LEFT | DT_CALCRECT | DT_NOPREFIX);
	DeleteDC(hdc);
}

void CTipWinImpl::CalcWindowRect()
{
	// ウィンドウのサイズは文字領域+左右(上下)のフレーム
	const int str_width = str_rect.right - str_rect.left;
	const int str_height = str_rect.bottom - str_rect.top;
	const int win_width = str_width + TIP_WIN_FRAME_WIDTH * 2;
	const int win_height = str_height + TIP_WIN_FRAME_WIDTH * 2;
	win_width_ = win_width;
	win_height_ = win_height;
}

LRESULT CALLBACK CTipWinImpl::WndProc(HWND hWnd, UINT nMsg,
                                       WPARAM wParam, LPARAM lParam)
{
	CTipWinImpl *self = (CTipWinImpl *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (nMsg) {
		case WM_CREATE: {
			CREATESTRUCTA *create_st = (CREATESTRUCTA *)lParam;
			self = (CTipWinImpl *)create_st->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
			self->tip_wnd = hWnd;
			break;
		}

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT: {
			SetWindowPos(self->tip_wnd, NULL, self->px, self->py, self->win_width_, self->win_height_,
						 SWP_NOZORDER | SWP_NOACTIVATE);
			HBRUSH hbr;
			HGDIOBJ holdbr;
			RECT cr;
			HDC hdc;

			PAINTSTRUCT ps;
			hdc = BeginPaint(hWnd, &ps);

			SelectObject(hdc, self->tip_font);
			SelectObject(hdc, GetStockObject(BLACK_PEN));

			hbr = CreateSolidBrush(self->tip_bg);
			holdbr = SelectObject(hdc, hbr);

			GetClientRect(hWnd, &cr);
			Rectangle(hdc, cr.left, cr.top, cr.right, cr.bottom);

			SetTextColor(hdc, self->tip_text);
			SetBkColor(hdc, self->tip_bg);

			RECT rect = self->str_rect;
			rect.left = rect.left + TIP_WIN_FRAME_WIDTH;
			rect.right = rect.right + TIP_WIN_FRAME_WIDTH;
			rect.top = rect.top + TIP_WIN_FRAME_WIDTH;
			rect.bottom = rect.bottom + TIP_WIN_FRAME_WIDTH;
			DrawTextW(hdc, self->str_, (int)self->str_len, &rect, DT_LEFT | DT_NOPREFIX);

			SelectObject(hdc, holdbr);
			DeleteObject(hbr);

			EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_TIMER:
			KillTimer(hWnd, 0);
			self->SetVisible(FALSE);
			break;

		default:
			break;
	}

	return DefWindowProcW(hWnd, nMsg, wParam, lParam);
}

CTipWinImpl::CTipWinImpl(HINSTANCE hInstance) : hInstance(hInstance)
{
	str_ = NULL;
	class_name[0] = 0;
}

CTipWinImpl::~CTipWinImpl()
{
	if(IsExists()) {
		Destroy();
	}
	free((void *)str_);
	str_ = NULL;
}

ATOM CTipWinImpl::RegisterClass()
{
	WNDCLASSW wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = class_name;
	return RegisterClassW(&wc);
}

VOID CTipWinImpl::Create(HWND pHwnd)
{
	LOGFONTA logfont;
	const UINT uDpi = GetMonitorDpiFromWindow(pHwnd);

	if(hInstance == NULL) {
		hInstance = (HINSTANCE)GetWindowLongPtr(pHwnd, GWLP_HINSTANCE);
	}
	if (class_name[0] == 0) {
		_snwprintf_s(class_name, _countof(class_name), _TRUNCATE, L"%s_%p", TipWinClassName, hInstance);
	}
	RegisterClass();
	str_len = 0;
	str_ = (wchar_t *)malloc(sizeof(wchar_t));
	memset((void *)str_, 0, sizeof(wchar_t));
	px = 0;
	py = 0;
	tip_bg = GetSysColor(COLOR_INFOBK);
	tip_text = GetSysColor(COLOR_INFOTEXT);
	GetMessageboxFont(&logfont);
	logfont.lfWidth = MulDiv(logfont.lfWidth, uDpi, 96);
	logfont.lfHeight = MulDiv(logfont.lfHeight, uDpi, 96);
	tip_font = CreateFontIndirect(&logfont);
	tip_wnd =
		CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
						class_name,
						NULL, WS_POPUP,
						0, 0,
						1, 1,
						pHwnd, NULL, hInstance, this);
}

VOID CTipWinImpl::Destroy()
{
	if(IsExists()) {
		// フォントの破棄
		DeleteObject(tip_font);
		tip_font = NULL;
		// ウィンドウの破棄
		SetWindowLongPtr(tip_wnd, GWLP_USERDATA, NULL);
		DestroyWindow(tip_wnd);
		tip_wnd = NULL;
	}
}

POINT CTipWinImpl::GetPos(void)
{
	POINT pts;
	pts.x = px;
	pts.y = py;
	return pts;
}

VOID CTipWinImpl::SetPos(int x, int y)
{
	if(IsExists()) {
		px = x;
		py = y;
		SetWindowPos(tip_wnd, 0, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	}
}

VOID CTipWinImpl::SetText(const wchar_t *str)
{
	if(!IsExists()) {
		return;
	}

	str_len = wcslen(str);
	if (str != NULL) {
		free((void *)str_);
	}
	str_ = _wcsdup(str);
	CalcStrRect();
	CalcWindowRect();

	InvalidateRect(tip_wnd, NULL, FALSE);
}

VOID CTipWinImpl::SetText(const char *str)
{
	wchar_t *strW = ToWcharA(str);
	SetText(strW);
	free(strW);
}

VOID CTipWinImpl::SetHideTimer(int ms)
{
	if(IsExists()) {
		SetTimer(tip_wnd, 0, ms, NULL);
	}
}

BOOL CTipWinImpl::IsExists(void)
{
	if(tip_wnd == NULL)
		return FALSE;
	return TRUE;
}

VOID CTipWinImpl::SetVisible(BOOL bVisible)
{
	int visible = (bVisible) ? SW_SHOWNOACTIVATE : SW_HIDE;
	if(IsExists()) {
		HWND hWnd = tip_wnd;
		if(IsWindowVisible(hWnd) != bVisible) {
			ShowWindow(hWnd, visible);
		}
	}
}

BOOL CTipWinImpl::IsVisible(void)
{
	if(IsExists()) {
		HWND hWnd = tip_wnd;
		return (BOOL)IsWindowVisible(hWnd);
	}
	return FALSE;
}

void CTipWinImpl::GetWindowSize(int *width, int *height)
{
	if (width != NULL) {
		*width = win_width_;
	}
	if (height != NULL) {
		*height = win_height_;
	}
}

void CTipWinImpl::GetFrameSize(int *width)
{
	*width = TIP_WIN_FRAME_WIDTH;
}

/**
 *	C++ Interface
 */

CTipWin::CTipWin(HINSTANCE hInstance)
{
	pimpl_ = new CTipWinImpl(hInstance);
}

CTipWin::~CTipWin()
{
	delete pimpl_;
	pimpl_ = NULL;
}

void CTipWin::Create(HWND pHwnd)
{
	pimpl_->Create(pHwnd);
}

VOID CTipWin::Destroy()
{
	pimpl_->Destroy();
}

VOID CTipWin::SetText(const char *str)
{
	pimpl_->SetText(str);
}

VOID CTipWin::SetText(const wchar_t *str)
{
	pimpl_->SetText(str);
}

POINT CTipWin::GetPos()
{
	return pimpl_->GetPos();
}

VOID CTipWin::SetPos(int x, int y)
{
	pimpl_->SetPos(x, y);
}

VOID CTipWin::SetHideTimer(int ms)
{
	pimpl_->SetHideTimer(ms);
}

BOOL CTipWin::IsExists()
{
	return pimpl_->IsExists();
}

VOID CTipWin::SetVisible(BOOL bVisible)
{
	pimpl_->SetVisible(bVisible);
}

BOOL CTipWin::IsVisible()
{
	return pimpl_->IsVisible();
}

void CTipWin::GetWindowSize(int *width, int *height)
{
	pimpl_->GetWindowSize(width, height);
}

void CTipWin::GetFrameSize(int *width)
{
	pimpl_->GetFrameSize(width);
}

/**
 *	C Interface
 */
typedef struct tagTipWin {
	CTipWin* ctipwin;
} TipWin;

TipWin *TipWinCreate(HINSTANCE hInstance, HWND src)
{
	TipWin *tipwin = (TipWin *)malloc(sizeof(TipWin));
	CTipWin* ctipwin = new CTipWin(hInstance);
	ctipwin->Create(src);
	tipwin->ctipwin = ctipwin;
	return (TipWin*)tipwin;
}

TipWin *TipWinCreateW(HINSTANCE hInstance, HWND src, int cx, int cy, const wchar_t *str)
{
	TipWin *tipwin = TipWinCreate(hInstance, src);
	CTipWin *ctipwin = tipwin->ctipwin;
	ctipwin->SetText(str);
	ctipwin->SetPos(cx, cy);
	ctipwin->SetVisible(TRUE);
	return (TipWin*)tipwin;
}

TipWin *TipWinCreateA(HINSTANCE hInstance, HWND src, int cx, int cy, const char *str)
{
	wchar_t *strW = ToWcharA(str);
	TipWin *tipwin = TipWinCreateW(hInstance, src, cx, cy, strW);
	free(strW);
	return tipwin;
}

void TipWinGetPos(TipWin *tWin, int *x, int *y)
{
	CTipWin *ctipwin = tWin->ctipwin;
	POINT pt = ctipwin->GetPos();
	*x = (int)pt.x;
	*y = (int)pt.y;
}

void TipWinSetPos(TipWin *tWin, int x, int y)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->SetPos(x, y);
}

/**
 *	文字をセットする
 *		文字をセットすると tipwinのサイズが取得(TipWinGetWindowSize())できるようになる
 */
void TipWinSetTextW(TipWin* tWin, const wchar_t *str)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->SetText(str);
}

void TipWinSetTextA(TipWin* tWin, const char *str)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->SetText(str);
}

void TipWinSetHideTimer(TipWin *tWin, int ms)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->SetHideTimer(ms);
}

void TipWinDestroy(TipWin* tWin)
{
	CTipWin *ctipwin = tWin->ctipwin;
	delete(ctipwin);
	tWin->ctipwin = NULL;
	free(tWin);
}

int TipWinIsExists(TipWin *tWin)
{
	CTipWin *ctipwin = tWin->ctipwin;
	return (int)ctipwin->IsExists();
}

void TipWinSetVisible(TipWin* tWin, int bVisible)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->SetVisible(bVisible);
}

int TipWinIsVisible(TipWin* tWin)
{
	CTipWin *ctipwin = tWin->ctipwin;
	return (int)ctipwin->IsVisible();
}

void TipWinGetWindowSize(TipWin* tWin, int *width, int *height)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->GetWindowSize(width, height);
}

void TipWinGetFrameSize(TipWin* tWin, int *width)
{
	CTipWin *ctipwin = tWin->ctipwin;
	ctipwin->GetFrameSize(width);
}
