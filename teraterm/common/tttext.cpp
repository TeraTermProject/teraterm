/*
 * (C) 2024- TeraTerm Project
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

/* Tera Term Text control */
// based on sakura editor 1.5.2.1 # CDlgAbout.cpp

#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "ttlib.h"
#include "win32helper.h"
#include "codeconv.h"

#include "tttext.h"

typedef enum {
	URL,
	MENU,
} Type;

typedef struct TTTextSt {
	HWND hParentWnd;
	WNDPROC proc;
	BOOL mouseover;
	HFONT font;
	HWND hWnd;
	int timer_done;
	wchar_t *text;
	Type type;
	char *url;
	HWND menu_wnd;
	int menu_id;
} TTText;
typedef struct TTTextSt url_subclass_t;

static void FreeSt(TTText *h)
{
	free(h->text);
	h->text = NULL;
	free(h->url);
	h->url = NULL;
}

// static textに割り当てるプロシージャ
static LRESULT CALLBACK UrlWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	url_subclass_t *parent = (url_subclass_t *)GetWindowLongPtrW( hWnd, GWLP_USERDATA );
	HDC hdc;
	POINT pt;
	RECT rc;

	switch (msg) {
#if 0
	case WM_SETCURSOR:
		{
			// カーソル形状変更
			HCURSOR hc;

			hc = (HCURSOR)LoadImage(NULL,
					MAKEINTRESOURCE(IDC_HAND),
					IMAGE_CURSOR,
					0,
					0,
					LR_DEFAULTSIZE | LR_SHARED);
			if (hc != NULL) {
				SetClassLongPtr(hWnd, GCLP_HCURSOR, (LONG_PTR)hc);
			}
			return (LRESULT)0;
		}
#endif

	// ダブルクリック
	//case WM_LBUTTONDBLCLK:

	// シングルクリック
	case WM_LBUTTONDOWN: {
		switch(parent->type) {
		case URL: {
			// kick WWW browser
			ShellExecuteA(NULL, NULL, parent->url, NULL, NULL,SW_SHOWNORMAL);
			break;
		}
		case MENU: {
			HWND menu_wnd = parent->menu_wnd;
			int menu_id = parent->menu_id;
			if (menu_wnd != NULL && menu_id != 0) {
				PostMessageA(menu_wnd, WM_COMMAND, MAKELONG(menu_id, 0), 0);
			}
			break;
		}
		default:
			assert(FALSE);
			break;
		}
		break;
	}

	case WM_MOUSEMOVE:
		{
			BOOL bHilighted;
			pt.x = LOWORD( lParam );
			pt.y = HIWORD( lParam );
			GetClientRect( hWnd, &rc );
			bHilighted = PtInRect( &rc, pt );

			if (parent->mouseover != bHilighted) {
				parent->mouseover = bHilighted;
				InvalidateRect( hWnd, NULL, TRUE );
				if (parent->timer_done == 0) {
					parent->timer_done = 1;
					SetTimer( hWnd, 1, 200, NULL );
				}
			}

		}
		break;

	case WM_TIMER:
		// URLの上にマウスカーソルがあるなら、システムカーソルを変更する。
		if (parent->mouseover) {
#if 1
			HCURSOR hc;
			//SetCapture(hWnd);

			hc = (HCURSOR)LoadImageA(NULL, IDC_HAND,
									 IMAGE_CURSOR, 0, 0,
									 LR_DEFAULTSIZE | LR_SHARED);

			SetSystemCursor(CopyCursor(hc), 32512 /* OCR_NORMAL */);    // 矢印
			SetSystemCursor(CopyCursor(hc), 32513 /* OCR_IBEAM */);     // Iビーム
#endif
		} else {
			//ReleaseCapture();
			// マウスカーソルを元に戻す。
			SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

		}

		// カーソルがウィンドウ外にある場合にも WM_MOUSEMOVE を送る
		GetCursorPos( &pt );
		ScreenToClient( hWnd, &pt );
		GetClientRect( hWnd, &rc );
		if( !PtInRect( &rc, pt ) ) {
			SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELONG( pt.x, pt.y ) );
		}
		break;

	case WM_PAINT:
		{
		// ウィンドウの描画
		PAINTSTRUCT ps;
		HFONT hOldFont;
		TCHAR szText[512];

		hdc = BeginPaint( hWnd, &ps );

		// 現在のクライアント矩形、テキスト、フォントを取得する
		GetClientRect( hWnd, &rc );
		GetWindowText( hWnd, szText, 512 );

		// テキスト描画
		SetBkMode( hdc, TRANSPARENT );
		SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
		if (parent->mouseover) {
			hOldFont = (HFONT)SelectObject(hdc, (HGDIOBJ)parent->font);
		} else {
			HFONT hFont = (HFONT)SendMessage( hWnd, WM_GETFONT, (WPARAM)0, (LPARAM)0 );
			hOldFont = (HFONT)SelectObject(hdc, (HGDIOBJ)hFont);
		}
		TextOut( hdc, 2, 0, szText, lstrlen( szText ) );
		SelectObject( hdc, (HGDIOBJ)hOldFont );

		// フォーカス枠描画
		if( GetFocus() == hWnd )
			DrawFocusRect( hdc, &rc );

		EndPaint( hWnd, &ps );
		return 0;
		}

	case WM_ERASEBKGND:
		hdc = (HDC)wParam;
		GetClientRect( hWnd, &rc );

		// 背景描画
#if 0
		if( parent->mouseover ){
			// ハイライト時背景描画
			SetBkColor( hdc, RGB( 0xff, 0xff, 0 ) );
			ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );
		}else
#endif
		{
			// 親にWM_CTLCOLORSTATICを送って背景ブラシを取得し、背景描画する
			HBRUSH hbr;
			HBRUSH hbrOld;

			hbr = (HBRUSH)SendMessage( GetParent( hWnd ), WM_CTLCOLORSTATIC, wParam, (LPARAM)hWnd );
			hbrOld = (HBRUSH)SelectObject( hdc, hbr );
			FillRect( hdc, &rc, hbr );
			SelectObject( hdc, hbrOld );
		}
		return (LRESULT)1;

	case WM_DESTROY:
		// 後始末
		SetWindowLongPtrW( hWnd, GWLP_WNDPROC, (LONG_PTR)parent->proc );
		if( parent->font != NULL ) {
			DeleteObject( parent->font );
		}

		// マウスカーソルを元に戻す。
		SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

		FreeSt(parent);
		free(parent);

		return (LRESULT)0;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}

	return CallWindowProcW( parent->proc, hWnd, msg, wParam, lParam );
}

// static textにプロシージャを設定し、サブクラス化する。
static void do_subclass_window(HWND hWnd, url_subclass_t *parent)
{
	HFONT hFont;
	LOGFONT lf;
	LONG_PTR style;

	// スタイルを設定
	style = GetWindowLongPtrW(hWnd, GWL_STYLE);
	style = style | (SS_NOTIFY | WS_TABSTOP);
	SetWindowLongPtrW(hWnd, GWL_STYLE, style);

	// 親のプロシージャをサブクラスから参照できるように、ポインタを登録しておく。
	SetWindowLongPtrW( hWnd, GWLP_USERDATA, (LONG_PTR)parent );
	// サブクラスのプロシージャを登録する。
	parent->proc = (WNDPROC)SetWindowLongPtrW( hWnd, GWLP_WNDPROC, (LONG_PTR)UrlWndProc);

	// 下線を付けたフォントを生成, 終了時削除すること
	hFont = (HFONT)SendMessage( hWnd, WM_GETFONT, (WPARAM)0, (LPARAM)0 );
	GetObject( hFont, sizeof(lf), &lf );
	lf.lfUnderline = TRUE;
	parent->font = hFont = CreateFontIndirect( &lf );

	parent->hWnd = hWnd;
	parent->timer_done = 0;
}

// static text のサイズを変更
static void FitControlSize(HWND Dlg, UINT id)
{
	HDC hdc;
	RECT r;
	DWORD dwExt;
	WORD w, h;
	HWND hwnd;
	POINT point;
	wchar_t *text;
	size_t text_len;

	hwnd = GetDlgItem(Dlg, id);
	hdc = GetDC(hwnd);
	SelectObject(hdc, (HFONT)SendMessage(Dlg, WM_GETFONT, 0, 0));
	hGetDlgItemTextW(Dlg, id, &text);
	text_len = wcslen(text);
	dwExt = GetTabbedTextExtentW(hdc, text, (int)text_len, 0, NULL);
	free(text);
	w = LOWORD(dwExt) + 5; // 幅が若干足りないので補正
	h = HIWORD(dwExt);
	GetWindowRect(hwnd, &r);
	point.x = r.left;
	point.y = r.top;
	ScreenToClient(Dlg, &point);
	MoveWindow(hwnd, point.x, point.y, w, h, TRUE);
}

TTText *TTTextURL(HWND hDlg, int id, const wchar_t *text, const char *url)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	url_subclass_t *h = (url_subclass_t *)calloc(1, sizeof(*h));
	if (text == NULL) {
		hGetDlgItemTextW(hDlg, id, &h->text);
	}
	else {
		h->text = _wcsdup(text);
		SetDlgItemTextW(hDlg, id, text);
	}
	if (url == NULL) {
		h->url = ToCharW(h->text);
	} else {
		h->url = _strdup(url);
	}
	FitControlSize(hDlg, id);
	do_subclass_window(hWnd, h);
	h->type = URL;
	return h;
}

TTText *TTTextMenu(HWND hDlg, int id, const wchar_t *text, HWND menu_wnd, int menu_id)
{
	url_subclass_t *h = (url_subclass_t *)calloc(1, sizeof(*h));
	HWND hWnd = GetDlgItem(hDlg, id);
	h->hParentWnd = hDlg;
	if (text == NULL) {
		hGetDlgItemTextW(hDlg, id, &h->text);
	}
	else {
		h->text = _wcsdup(text);
		SetDlgItemTextW(hDlg, id, text);
	}
	FitControlSize(hDlg, id);
	do_subclass_window(hWnd, h);
	h->type = MENU;
	h->menu_wnd = menu_wnd;
	h->menu_id = menu_id;
	return h;
}
