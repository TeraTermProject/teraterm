/*
 * (C) 2026- TeraTerm Project
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
#include <windowsx.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttwinman.h"
#include "ttsetup.h"
#include "compat_win.h"
#include "tt_res.h"
#include "vtwin.h"
#include "darkmode.h"
#include "session.h"
#include "frame.h"
#include "vtdisp.h"
#include "dbglog.h"
#include "devices.h"
#include "sidebar.h"

#define FRAME_CLASS_NAME	L"TeraTermTabFrame"
#define WM_APP_REMOVETAB	(WM_APP + 1)
#define WM_APP_NEWTAB		(WM_APP + 2)
#define STATUS_TIMER_ID		1

#define TAB_MAX_WIDTH		220
#define TAB_MIN_WIDTH		90

typedef struct {
	CVTWindow *vt;
	HWND hWnd;
	TTSession *session;
	wchar_t *title;
	BOOL closing;
	BOOL was_connected;	// connected at some point (not an empty tab)
} Tab;

static HWND frame_wnd;
static HINSTANCE frame_inst;
static Tab *tabs;
static int tab_count;
static int active_tab = -1;
static int hot_tab = -1;		// tab under the mouse
static BOOL hot_close;			// mouse is on the close button of hot_tab
static BOOL hot_plus;			// mouse is on the "+" button
static BOOL tracking_mouse;
static BOOL laying_out;			// frame is resizing the VT window
static HFONT tab_font;
static UINT tab_font_dpi;

static UINT GetFrameDpi(void)
{
	if (pGetDpiForWindow != NULL && frame_wnd != NULL) {
		return pGetDpiForWindow(frame_wnd);
	}
	return 96;
}

static int Scale(int v)
{
	return MulDiv(v, (int)GetFrameDpi(), 96);
}

static int TabBarHeight(void)
{
	return Scale(32);
}

static int StatusBarHeight(void)
{
	return Scale(24);
}

// left edge of the tab strip, the content and the status bar (right of the sidebar)
static int BarLeft(void)
{
	return SidebarWidth();
}

static HFONT GetTabFont(void)
{
	const UINT dpi = GetFrameDpi();
	if (tab_font != NULL && tab_font_dpi == dpi) {
		return tab_font;
	}
	if (tab_font != NULL) {
		DeleteObject(tab_font);
	}
	NONCLIENTMETRICSW ncm = {};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	LOGFONTW lf = ncm.lfMessageFont;
	lf.lfHeight = -MulDiv(9, (int)dpi, 72);
	tab_font = CreateFontIndirectW(&lf);
	tab_font_dpi = dpi;
	return tab_font;
}

/*
 *	tab strip geometry
 */
static int TabWidth(void)
{
	RECT rc;
	GetClientRect(frame_wnd, &rc);
	const int plus = Scale(32);
	int w = tab_count > 0 ? (rc.right - BarLeft() - plus) / tab_count : TAB_MAX_WIDTH;
	if (w > Scale(TAB_MAX_WIDTH)) w = Scale(TAB_MAX_WIDTH);
	if (w < Scale(TAB_MIN_WIDTH)) w = Scale(TAB_MIN_WIDTH);
	return w;
}

static void GetTabRect(int index, RECT *r)
{
	const int w = TabWidth();
	r->left = BarLeft() + index * w;
	r->right = r->left + w;
	r->top = 0;
	r->bottom = TabBarHeight();
}

static void GetCloseRect(const RECT *tab, RECT *r)
{
	const int size = Scale(16);
	r->right = tab->right - Scale(8);
	r->left = r->right - size;
	r->top = (tab->top + tab->bottom - size) / 2;
	r->bottom = r->top + size;
}

static void GetPlusRect(RECT *r)
{
	const int w = TabWidth();
	r->left = BarLeft() + tab_count * w + Scale(4);
	r->right = r->left + Scale(26);
	r->top = Scale(3);
	r->bottom = TabBarHeight() - Scale(3);
}

/**
 *	hit test of the tab strip
 *	@retval	-1		no tab
 */
static int HitTest(POINT pt, BOOL *on_close, BOOL *on_plus)
{
	*on_close = FALSE;
	*on_plus = FALSE;
	if (pt.y < 0 || pt.y >= TabBarHeight()) {
		return -1;
	}
	RECT r;
	GetPlusRect(&r);
	if (PtInRect(&r, pt)) {
		*on_plus = TRUE;
		return -1;
	}
	for (int i = 0; i < tab_count; i++) {
		GetTabRect(i, &r);
		if (PtInRect(&r, pt)) {
			RECT c;
			GetCloseRect(&r, &c);
			*on_close = PtInRect(&c, pt);
			return i;
		}
	}
	return -1;
}

static void InvalidateTabBar(void)
{
	RECT rc;
	GetClientRect(frame_wnd, &rc);
	rc.bottom = TabBarHeight();
	InvalidateRect(frame_wnd, &rc, FALSE);
}

/*
 *	painting
 */
typedef struct {
	COLORREF bar, tab, tab_hot, tab_active, text, text_inactive, accent, close_hot;
} TabColors;

static void GetColors(TabColors *c)
{
	if (DarkModeIsEnabled()) {
		c->bar = RGB(24, 24, 24);
		c->tab = RGB(38, 38, 38);
		c->tab_hot = RGB(52, 52, 52);
		c->tab_active = RGB(30, 30, 30);
		c->text = RGB(235, 235, 235);
		c->text_inactive = RGB(160, 160, 160);
		c->accent = RGB(64, 156, 255);
		c->close_hot = RGB(80, 80, 80);
	}
	else {
		c->bar = RGB(222, 222, 222);
		c->tab = RGB(234, 234, 234);
		c->tab_hot = RGB(244, 244, 244);
		c->tab_active = RGB(255, 255, 255);
		c->text = RGB(20, 20, 20);
		c->text_inactive = RGB(90, 90, 90);
		c->accent = RGB(0, 120, 215);
		c->close_hot = RGB(210, 210, 210);
	}
}

static void FillSolid(HDC dc, const RECT *r, COLORREF c)
{
	SetBkColor(dc, c);
	ExtTextOutW(dc, 0, 0, ETO_OPAQUE, r, NULL, 0, NULL);
}

static void DrawX(HDC dc, const RECT *r, COLORREF c, int pad)
{
	HPEN pen = CreatePen(PS_SOLID, Scale(1) < 1 ? 1 : Scale(1), c);
	HGDIOBJ old = SelectObject(dc, pen);
	MoveToEx(dc, r->left + pad, r->top + pad, NULL);
	LineTo(dc, r->right - pad, r->bottom - pad);
	MoveToEx(dc, r->right - pad - 1, r->top + pad, NULL);
	LineTo(dc, r->left + pad - 1, r->bottom - pad);
	SelectObject(dc, old);
	DeleteObject(pen);
}

static void DrawPlus(HDC dc, const RECT *r, COLORREF c)
{
	HPEN pen = CreatePen(PS_SOLID, Scale(1) < 1 ? 1 : Scale(1), c);
	HGDIOBJ old = SelectObject(dc, pen);
	const int cx = (r->left + r->right) / 2;
	const int cy = (r->top + r->bottom) / 2;
	const int len = Scale(6);
	MoveToEx(dc, cx - len, cy, NULL);
	LineTo(dc, cx + len + 1, cy);
	MoveToEx(dc, cx, cy - len, NULL);
	LineTo(dc, cx, cy + len + 1);
	SelectObject(dc, old);
	DeleteObject(pen);
}

static void PaintTabBar(HDC dc_out)
{
	RECT client;
	GetClientRect(frame_wnd, &client);
	RECT bar = client;
	bar.bottom = TabBarHeight();

	// double buffering
	HDC dc = CreateCompatibleDC(dc_out);
	HBITMAP bmp = CreateCompatibleBitmap(dc_out, bar.right, bar.bottom);
	HGDIOBJ old_bmp = SelectObject(dc, bmp);
	HGDIOBJ old_font = SelectObject(dc, GetTabFont());
	SetBkMode(dc, TRANSPARENT);

	TabColors c;
	GetColors(&c);
	FillSolid(dc, &bar, c.bar);

	for (int i = 0; i < tab_count; i++) {
		RECT r;
		GetTabRect(i, &r);
		const BOOL active = (i == active_tab);
		RECT body = r;
		body.right -= Scale(1);		// separator
		FillSolid(dc, &body, active ? c.tab_active : (i == hot_tab ? c.tab_hot : c.tab));
		if (active) {
			RECT accent = body;
			accent.bottom = accent.top + Scale(2);
			FillSolid(dc, &accent, c.accent);
		}

		// connection state dot
		const TComVar *cv = SessionGetCV(tabs[i].session);
		const int dot = Scale(8);
		RECT d;
		d.left = r.left + Scale(10);
		d.top = (r.top + r.bottom - dot) / 2;
		d.right = d.left + dot;
		d.bottom = d.top + dot;
		HBRUSH brush = CreateSolidBrush(cv->Ready ? RGB(46, 204, 113) : RGB(128, 128, 128));
		HGDIOBJ old_brush = SelectObject(dc, brush);
		HGDIOBJ old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
		Ellipse(dc, d.left, d.top, d.right + 1, d.bottom + 1);
		SelectObject(dc, old_pen);
		SelectObject(dc, old_brush);
		DeleteObject(brush);

		// title
		RECT close;
		GetCloseRect(&r, &close);
		RECT text = r;
		text.left = d.right + Scale(8);
		text.right = close.left - Scale(4);
		SetTextColor(dc, active ? c.text : c.text_inactive);
		const wchar_t *title = tabs[i].title != NULL ? tabs[i].title : L"Tera Term";
		DrawTextW(dc, title, -1, &text, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

		// close button
		if (active || i == hot_tab) {
			if (i == hot_tab && hot_close) {
				FillSolid(dc, &close, c.close_hot);
			}
			DrawX(dc, &close, active ? c.text : c.text_inactive, Scale(4));
		}
	}

	RECT plus;
	GetPlusRect(&plus);
	if (hot_plus) {
		FillSolid(dc, &plus, c.close_hot);
	}
	DrawPlus(dc, &plus, c.text_inactive);

	BitBlt(dc_out, 0, 0, bar.right, bar.bottom, dc, 0, 0, SRCCOPY);
	SelectObject(dc, old_font);
	SelectObject(dc, old_bmp);
	DeleteObject(bmp);
	DeleteDC(dc);
}

/*
 *	status bar: connection of the active tab
 */
static void GetStatusRect(RECT *r)
{
	GetClientRect(frame_wnd, r);
	r->left = BarLeft();
	r->top = r->bottom - StatusBarHeight();
}

static void InvalidateStatusBar(void)
{
	RECT r;
	GetStatusRect(&r);
	InvalidateRect(frame_wnd, &r, FALSE);
}

static void StatusText(wchar_t *left, size_t left_count, wchar_t *right, size_t right_count, BOOL *connected)
{
	left[0] = 0;
	right[0] = 0;
	*connected = FALSE;
	if (active_tab < 0) {
		return;
	}
	const TTTSet *ts = SessionGetTS(tabs[active_tab].session);
	const TComVar *cv = SessionGetCV(tabs[active_tab].session);
	*connected = cv->Open && cv->Ready;
	if (!cv->Open) {
		wcscpy_s(left, left_count, tabs[active_tab].was_connected ? L"Disconnected" : L"Not connected");
	}
	else if (cv->PortType == IdSerial) {
		swprintf_s(left, left_count, L"Serial   COM%d   %lu baud", ts->ComPort, (unsigned long)ts->Baud);
	}
	else if (cv->PortType == IdTCPIP) {
		const wchar_t *proto = cv->isSSH ? L"SSH" : (ts->Telnet ? L"Telnet" : L"TCP");
		wchar_t host[256];
		if (ts->HostNameW != NULL && ts->HostNameW[0] != 0) {
			wcsncpy_s(host, _countof(host), ts->HostNameW, _TRUNCATE);
		}
		else {
			MultiByteToWideChar(CP_ACP, 0, ts->HostName, -1, host, _countof(host));
			host[_countof(host) - 1] = 0;
		}
		swprintf_s(left, left_count, L"%s   %s:%d%s", proto, host, ts->TCPPort, cv->Ready ? L"" : L"   connecting...");
	}
	else if (cv->PortType == IdFile) {
		wcscpy_s(left, left_count, L"Replaying log");
	}
	else {
		wcscpy_s(left, left_count, L"Named pipe");
	}
	swprintf_s(right, right_count, L"%d x %d", ts->TerminalWidth, ts->TerminalHeight);
}

static void PaintStatusBar(HDC dc_out)
{
	RECT r;
	GetStatusRect(&r);
	if (r.right <= r.left) {
		return;
	}
	const int w = r.right - r.left, h = r.bottom - r.top;
	HDC dc = CreateCompatibleDC(dc_out);
	HBITMAP bmp = CreateCompatibleBitmap(dc_out, w, h);
	HGDIOBJ old_bmp = SelectObject(dc, bmp);
	HGDIOBJ old_font = SelectObject(dc, GetTabFont());
	SetBkMode(dc, TRANSPARENT);

	TabColors c;
	GetColors(&c);
	RECT all = { 0, 0, w, h };
	FillSolid(dc, &all, c.bar);
	RECT line = { 0, 0, w, Scale(1) };
	FillSolid(dc, &line, c.tab);

	wchar_t left[400], right[64];
	BOOL connected;
	StatusText(left, _countof(left), right, _countof(right), &connected);

	const int dot = Scale(8);
	RECT d;
	d.left = Scale(10);
	d.top = (h - dot) / 2;
	d.right = d.left + dot;
	d.bottom = d.top + dot;
	HBRUSH brush = CreateSolidBrush(connected ? RGB(46, 204, 113) : RGB(128, 128, 128));
	HGDIOBJ old_brush = SelectObject(dc, brush);
	HGDIOBJ old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
	Ellipse(dc, d.left, d.top, d.right + 1, d.bottom + 1);
	SelectObject(dc, old_pen);
	SelectObject(dc, old_brush);
	DeleteObject(brush);

	SetTextColor(dc, c.text_inactive);
	RECT tr = { Scale(10) + dot + Scale(8), 0, w - Scale(10), h };
	DrawTextW(dc, right, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);
	SIZE rs;
	GetTextExtentPoint32W(dc, right, (int)wcslen(right), &rs);
	tr.right -= rs.cx + Scale(16);
	SetTextColor(dc, c.text);
	DrawTextW(dc, left, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

	BitBlt(dc_out, r.left, r.top, w, h, dc, 0, 0, SRCCOPY);
	SelectObject(dc, old_font);
	SelectObject(dc, old_bmp);
	DeleteObject(bmp);
	DeleteDC(dc);
}

/*
 *	layout
 */
void FrameGetContentRect(RECT *rc)
{
	GetClientRect(frame_wnd, rc);
	rc->left = BarLeft();
	rc->top = TabBarHeight();
	rc->bottom -= StatusBarHeight();
	if (rc->right < rc->left) {
		rc->right = rc->left;
	}
	if (rc->bottom < rc->top) {
		rc->bottom = rc->top;
	}
}

static void LayoutActive(void)
{
	if (active_tab < 0) {
		return;
	}
	RECT rc;
	FrameGetContentRect(&rc);
	laying_out = TRUE;
	SetWindowPos(tabs[active_tab].hWnd, NULL, rc.left, rc.top,
				 rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
	laying_out = FALSE;
}

/**
 *	place the sidebar and the active tab (window resized, sidebar shown,
 *	hidden or resized)
 */
void FrameLayout(void)
{
	if (frame_wnd == NULL) {
		return;
	}
	HWND sidebar = SidebarGetHWND();
	if (sidebar != NULL && SidebarIsVisible()) {
		RECT rc;
		GetClientRect(frame_wnd, &rc);
		SetWindowPos(sidebar, NULL, 0, 0, SidebarWidth(), rc.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
	}
	LayoutActive();
	InvalidateRect(frame_wnd, NULL, TRUE);
}

/**
 *	VT window changed its own size (font change, terminal size change)
 *	resize the frame so it fits
 */
void FrameChildSizeChanged(HWND vt)
{
	if (laying_out || active_tab < 0 || tabs[active_tab].hWnd != vt) {
		return;
	}
	if (IsZoomed(frame_wnd) || IsIconic(frame_wnd)) {
		LayoutActive();
		return;
	}
	RECT child;
	GetWindowRect(vt, &child);
	RECT content;
	FrameGetContentRect(&content);
	const int cw = child.right - child.left;
	const int ch = child.bottom - child.top;
	if (cw == content.right - content.left && ch == content.bottom - content.top) {
		return;
	}
	RECT win;
	GetWindowRect(frame_wnd, &win);
	RECT client;
	GetClientRect(frame_wnd, &client);
	const int dw = (win.right - win.left) - client.right;
	const int dh = (win.bottom - win.top) - client.bottom;
	SetWindowPos(frame_wnd, NULL, 0, 0, cw + BarLeft() + dw, ch + TabBarHeight() + StatusBarHeight() + dh,
				 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/*
 *	tabs
 */
static void CloseTab(int index);

static int FindTab(HWND vt)
{
	for (int i = 0; i < tab_count; i++) {
		if (tabs[i].hWnd == vt) {
			return i;
		}
	}
	return -1;
}

static void UpdateCaption(void)
{
	if (active_tab < 0) {
		return;
	}
	const wchar_t *title = tabs[active_tab].title;
	SetWindowTextW(frame_wnd, title != NULL ? title : L"Tera Term");
}

static void UpdateTitle(int index)
{
	wchar_t buf[512];
	GetWindowTextW(tabs[index].hWnd, buf, _countof(buf));
	free(tabs[index].title);
	tabs[index].title = _wcsdup(buf);
}

static void SelectTab(int index)
{
	if (index < 0 || index >= tab_count) {
		return;
	}
	if (index == active_tab) {
		SetFocus(tabs[index].hWnd);
		return;
	}
	const int prev = active_tab;
	active_tab = index;

	if (prev >= 0 && prev < tab_count) {
		tabs[prev].vt->OnFrameActivate(WA_INACTIVE);
	}
	SessionSwitch(tabs[index].session);
	LayoutActive();
	ShowWindow(tabs[index].hWnd, SW_SHOW);
	if (prev >= 0 && prev < tab_count) {
		ShowWindow(tabs[prev].hWnd, SW_HIDE);
	}
	tabs[index].vt->OnFrameActivate(GetActiveWindow() == frame_wnd ? WA_ACTIVE : WA_INACTIVE);
	HMENU menu = tabs[index].vt->GetMainMenu();
	if (GetMenu(frame_wnd) != menu) {
		SetMenu(frame_wnd, menu);
	}
	SetFocus(tabs[index].hWnd);
	UpdateCaption();
	InvalidateTabBar();
	InvalidateStatusBar();
}

void FrameAddTab(CVTWindow *vt, TTSession *session)
{
	tabs = (Tab *)realloc(tabs, sizeof(Tab) * (tab_count + 1));
	Tab *t = &tabs[tab_count];
	t->vt = vt;
	t->hWnd = vt->m_hWnd;
	t->session = session;
	t->title = NULL;
	t->closing = FALSE;
	t->was_connected = FALSE;
	tab_count++;
	UpdateTitle(tab_count - 1);
	DbgLog("tab: %d added (session %p), %d tabs", tab_count - 1, session, tab_count);

	if (tab_count == 1) {
		// dark mode is known after the settings were read (by the first tab)
		if (DarkModeIsEnabled()) {
			DarkModeApplyToWindow(frame_wnd);
		}
		// saved devices live next to TERATERM.INI
		DevicesLoad(SessionGetTS(session)->HomeDirW);
		SidebarCreate(frame_wnd, frame_inst);
		// first tab, the frame wraps the terminal size from the settings
		RECT child;
		GetWindowRect(t->hWnd, &child);
		RECT win, client;
		GetWindowRect(frame_wnd, &win);
		GetClientRect(frame_wnd, &client);
		const int w = (child.right - child.left) + BarLeft() + (win.right - win.left) - client.right;
		const int h = (child.bottom - child.top) + TabBarHeight() + StatusBarHeight() +
					  (win.bottom - win.top) - client.bottom;
		UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
		int x = 0, y = 0;
		const TTTSet *ts = SessionGetTS(session);
		if (ts->VTPos.x == CW_USEDEFAULT) {
			flags |= SWP_NOMOVE;
		}
		else {
			x = ts->VTPos.x;
			y = ts->VTPos.y;
		}
		SetWindowPos(frame_wnd, NULL, x, y, w, h, flags);
		SelectTab(0);
		FrameLayout();
		if (ts->nCmdShow != SW_HIDE && ts->HideWindow == 0) {
			ShowWindow(frame_wnd, ts->Minimize > 0 ? SW_SHOWMINIMIZED : ts->nCmdShow);
			UpdateWindow(frame_wnd);
		}
		return;
	}
	SelectTab(tab_count - 1);
}

CVTWindow *FrameGetActiveVT(void)
{
	return active_tab >= 0 ? tabs[active_tab].vt : NULL;
}

HWND FrameGetActiveVTHWND(void)
{
	return active_tab >= 0 ? tabs[active_tab].hWnd : NULL;
}

/**
 *	Open a new tab
 *	@param	command_line	same as Tera Term command line, NULL = new connection dialog
 */
void FrameNewTab(const wchar_t *command_line)
{
	TTSession *s = SessionCreate();
	DbgLog("tab: new tab, session %p, %s", s, command_line != NULL ? "with connection" : "new connection dialog");
	SessionSwitch(s);
	g_ts->TeraTermInstance = frame_inst;
	g_ts->nCmdShow = SW_SHOW;
	CVTWindow *vt = new CVTWindow(frame_inst, command_line != NULL ? command_line : L"ttermpro");
	if (vt->m_hWnd == NULL) {
		DbgLog("tab: creating the window failed (session %p)", s);
		delete vt;
		pVTWin = NULL;
		// back to the tab that was active
		if (active_tab >= 0) {
			SessionSwitch(tabs[active_tab].session);
		}
		SessionDestroy(s);
		return;
	}
	pVTWin = vt;
	// the tab fills the frame, the terminal size (columns, lines) follows
	AdjustSize = FALSE;
	FrameAddTab(vt, s);
}

/**
 *	Open a new tab later, from the frame's message loop
 *	(can be called from a VT window procedure)
 */
void FramePostNewTab(const wchar_t *command_line)
{
	wchar_t *cmd = command_line != NULL ? _wcsdup(command_line) : NULL;
	if (!PostMessageW(frame_wnd, WM_APP_NEWTAB, 0, (LPARAM)cmd)) {
		free(cmd);
	}
}

/**
 *	Open a connection from the sidebar
 *	An empty tab (never connected, e.g. the start-up tab) is replaced by it.
 */
void FrameConnect(const wchar_t *command_line)
{
	HWND empty = NULL;
	if (active_tab >= 0 && !tabs[active_tab].closing) {
		const TComVar *cv = SessionGetCV(tabs[active_tab].session);
		if (cv->Open) {
			tabs[active_tab].was_connected = TRUE;
		}
		else if (!tabs[active_tab].was_connected) {
			empty = tabs[active_tab].hWnd;
		}
	}
	const int before = tab_count;
	FrameNewTab(command_line);
	if (empty != NULL && tab_count > before) {
		CloseTab(FindTab(empty));
	}
}

/**
 *	VT window rebuilt its menu
 */
void FrameChildMenuChanged(HWND vt, HMENU menu)
{
	const int i = FindTab(vt);
	if (i < 0 || i != active_tab) {
		return;
	}
	SetMenu(frame_wnd, menu);
	DrawMenuBar(frame_wnd);
}

void FrameChildTitleChanged(HWND vt)
{
	const int i = FindTab(vt);
	if (i < 0) {
		return;
	}
	UpdateTitle(i);
	if (SessionGetCV(tabs[i].session)->Open) {
		tabs[i].was_connected = TRUE;
	}
	if (i == active_tab) {
		UpdateCaption();
	}
	InvalidateTabBar();
	InvalidateStatusBar();
}

/**
 *	called from WM_DESTROY of a VT window (its session is active)
 *	the tab is removed later, outside of the window procedure
 */
void FrameChildDestroyed(HWND vt)
{
	const int i = FindTab(vt);
	if (i < 0) {
		return;
	}
	tabs[i].closing = TRUE;
	DbgLog("tab: %d closing (session %p)", i, tabs[i].session);
	PostMessageW(frame_wnd, WM_APP_REMOVETAB, (WPARAM)vt, 0);
}

static void RemoveTab(HWND vt)
{
	const int i = FindTab(vt);
	if (i < 0) {
		return;
	}
	Tab t = tabs[i];
	memmove(&tabs[i], &tabs[i + 1], sizeof(Tab) * (tab_count - i - 1));
	tab_count--;
	DbgLog("tab: %d removed (session %p), %d tabs left", i, t.session, tab_count);

	if (tab_count == 0) {
		// last tab, quit (the session stays for the clean up in WinMain)
		active_tab = -1;
		SessionSwitch(t.session);
		delete t.vt;
		free(t.title);
		DestroyWindow(frame_wnd);
		return;
	}

	// select another tab first, so that the closed session is not active
	if (active_tab == i) {
		active_tab = -1;
		SelectTab(i < tab_count ? i : tab_count - 1);
	}
	else if (active_tab > i) {
		active_tab--;
	}

	TTSession *prev = SessionEnter(t.session);
	delete t.vt;
	TTSetUnInit(g_ts);
	SessionLeave(prev);
	SessionDestroy(t.session);
	free(t.title);
	InvalidateTabBar();
}

static void CloseTab(int index)
{
	if (index < 0 || index >= tab_count || tabs[index].closing) {
		return;
	}
	SendMessageW(tabs[index].hWnd, WM_CLOSE, 0, 0);
}

static void CloseAll(void)
{
	// close from the last tab, a tab may cancel (disconnect confirmation)
	for (int i = tab_count - 1; i >= 0; i--) {
		if (i >= tab_count) {
			continue;
		}
		HWND vt = tabs[i].hWnd;
		SendMessageW(vt, WM_CLOSE, 0, 0);
		if (IsWindow(vt)) {
			// canceled
			return;
		}
	}
}

/*
 *	mouse on the tab strip
 */
static void OnMouseMove(POINT pt)
{
	if (!tracking_mouse) {
		TRACKMOUSEEVENT tme = {};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = frame_wnd;
		TrackMouseEvent(&tme);
		tracking_mouse = TRUE;
	}
	BOOL on_close, on_plus;
	const int hot = HitTest(pt, &on_close, &on_plus);
	if (hot != hot_tab || on_close != hot_close || on_plus != hot_plus) {
		hot_tab = hot;
		hot_close = on_close;
		hot_plus = on_plus;
		InvalidateTabBar();
	}
}

static void OnMouseLeave(void)
{
	tracking_mouse = FALSE;
	if (hot_tab != -1 || hot_plus) {
		hot_tab = -1;
		hot_close = FALSE;
		hot_plus = FALSE;
		InvalidateTabBar();
	}
}

static void OnLButtonDown(POINT pt)
{
	BOOL on_close, on_plus;
	const int i = HitTest(pt, &on_close, &on_plus);
	if (on_plus) {
		FrameNewTab(NULL);
	}
	else if (i >= 0 && on_close) {
		CloseTab(i);
	}
	else if (i >= 0) {
		SelectTab(i);
	}
}

static void OnMButtonUp(POINT pt)
{
	BOOL on_close, on_plus;
	const int i = HitTest(pt, &on_close, &on_plus);
	if (i >= 0) {
		CloseTab(i);
	}
}

static void OnLButtonDblClk(POINT pt)
{
	BOOL on_close, on_plus;
	const int i = HitTest(pt, &on_close, &on_plus);
	if (i < 0 && !on_plus && pt.y < TabBarHeight()) {
		FrameNewTab(NULL);
	}
}

/*
 *	commands
 */
static BOOL OnFrameCommand(WORD id)
{
	switch (id) {
	case ID_FILE_NEWTAB:
		FrameNewTab(NULL);
		return TRUE;
	case ID_FILE_CLOSETAB:
		CloseTab(active_tab);
		return TRUE;
	case ID_TAB_NEXT:
		if (tab_count > 1) {
			SelectTab((active_tab + 1) % tab_count);
		}
		return TRUE;
	case ID_TAB_PREV:
		if (tab_count > 1) {
			SelectTab((active_tab + tab_count - 1) % tab_count);
		}
		return TRUE;
	case ID_FILE_SIDEBAR:
		SidebarSetVisible(!SidebarIsVisible());
		if (!SidebarIsVisible() && active_tab >= 0) {
			SetFocus(tabs[active_tab].hWnd);
		}
		return TRUE;
	case ID_FILE_ADDDEVICE:
		SidebarAddDevice();
		return TRUE;
	}
	return FALSE;
}

static void OnDpiChanged(WPARAM wp, LPARAM lp)
{
	const RECT *r = (const RECT *)lp;
	const UINT dpi = LOWORD(wp);
	SidebarDpiChanged();
	SetWindowPos(frame_wnd, NULL, r->left, r->top, r->right - r->left, r->bottom - r->top,
				 SWP_NOZORDER | SWP_NOACTIVATE);
	FrameLayout();
	for (int i = 0; i < tab_count; i++) {
		if (i != active_tab) {
			// same size as the active tab, so the terminal size is right when selected
			RECT rc;
			FrameGetContentRect(&rc);
			laying_out = TRUE;
			SetWindowPos(tabs[i].hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						 SWP_NOZORDER | SWP_NOACTIVATE);
			laying_out = FALSE;
		}
		tabs[i].vt->OnFrameDpiChanged(dpi);
	}
	InvalidateRect(frame_wnd, NULL, TRUE);
}

static LRESULT CALLBACK FrameProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT result;
	if (DarkModeHandleNCMessage(hWnd, msg, wp, lp, &result)) {
		return result;
	}

	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		PaintTabBar(dc);
		PaintStatusBar(dc);
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND: {
		// content area behind the VT window (visible when the VT window is smaller)
		RECT rc;
		FrameGetContentRect(&rc);
		TabColors c;
		GetColors(&c);
		FillSolid((HDC)wp, &rc, c.tab_active);
		return 1;
	}
	case WM_SIZE:
		FrameLayout();
		return 0;
	case WM_TIMER:
		if (wp == STATUS_TIMER_ID) {
			// connection state and terminal size of the tabs
			for (int i = 0; i < tab_count; i++) {
				if (SessionGetCV(tabs[i].session)->Open) {
					tabs[i].was_connected = TRUE;
				}
			}
			InvalidateTabBar();
			InvalidateStatusBar();
		}
		return 0;
	case WM_MOUSEMOVE: {
		POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
		OnMouseMove(pt);
		return 0;
	}
	case WM_MOUSELEAVE:
		OnMouseLeave();
		return 0;
	case WM_LBUTTONDOWN: {
		POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
		OnLButtonDown(pt);
		return 0;
	}
	case WM_LBUTTONDBLCLK: {
		POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
		OnLButtonDblClk(pt);
		return 0;
	}
	case WM_MBUTTONUP: {
		POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
		OnMButtonUp(pt);
		return 0;
	}
	case WM_COMMAND:
		if (OnFrameCommand(LOWORD(wp))) {
			return 0;
		}
		// menu of the active tab
		if (active_tab >= 0) {
			return SendMessageW(tabs[active_tab].hWnd, msg, wp, lp);
		}
		return 0;
	case WM_INITMENUPOPUP:
		if (active_tab >= 0) {
			LRESULT r = SendMessageW(tabs[active_tab].hWnd, msg, wp, lp);
			CheckMenuItem((HMENU)wp, ID_FILE_SIDEBAR, MF_BYCOMMAND | (SidebarIsVisible() ? MF_CHECKED : MF_UNCHECKED));
			return r;
		}
		break;
	case WM_ENTERMENULOOP:
	case WM_EXITMENULOOP:
	case WM_MENUSELECT:
		if (active_tab >= 0) {
			return SendMessageW(tabs[active_tab].hWnd, msg, wp, lp);
		}
		break;
	case WM_ACTIVATE:
		if (active_tab >= 0) {
			tabs[active_tab].vt->OnFrameActivate(LOWORD(wp));
			if (LOWORD(wp) != WA_INACTIVE) {
				SetFocus(tabs[active_tab].hWnd);
			}
		}
		return 0;
	case WM_SETFOCUS:
		if (active_tab >= 0) {
			SetFocus(tabs[active_tab].hWnd);
		}
		return 0;
	case WM_DPICHANGED:
		OnDpiChanged(wp, lp);
		return 0;
	case WM_USER_IDLETIMER:
		// only wakes up the message loop
		return 0;
	case WM_APP_REMOVETAB:
		RemoveTab((HWND)wp);
		return 0;
	case WM_APP_NEWTAB: {
		wchar_t *cmd = (wchar_t *)lp;
		FrameNewTab(cmd);
		free(cmd);
		return 0;
	}
	case WM_CLOSE:
		CloseAll();
		return 0;
	case WM_DESTROY:
		KillTimer(hWnd, STATUS_TIMER_ID);
		DbgLog("frame: closed");
		frame_wnd = NULL;
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wp, lp);
}

/**
 *	Create the main window (hidden)
 *	The window position and size are set when the first tab is added
 */
HWND FrameCreate(HINSTANCE hInstance)
{
	frame_inst = hInstance;

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = FrameProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_VT));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = FRAME_CLASS_NAME;
	RegisterClassExW(&wc);

	int x = CW_USEDEFAULT;
	int y = CW_USEDEFAULT;
	if (g_ts->VTPos.x != CW_USEDEFAULT) {
		x = g_ts->VTPos.x;
		y = g_ts->VTPos.y;
	}
	frame_wnd = CreateWindowExW(0, FRAME_CLASS_NAME, L"Tera Term",
								WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
								x, y, 800, 500, NULL, NULL, hInstance, NULL);
	if (frame_wnd != NULL && DarkModeIsEnabled()) {
		DarkModeApplyToWindow(frame_wnd);
	}
	if (frame_wnd != NULL) {
		SetTimer(frame_wnd, STATUS_TIMER_ID, 1000, NULL);
	}
	return frame_wnd;
}

/**
 *	dark mode was turned on / off (DarkModeSetEnabled() is already called)
 */
void FrameDarkModeChanged(void)
{
	const BOOL dark = DarkModeIsEnabled();
	DarkModeApplyToWindow(frame_wnd);
	for (int i = 0; i < tab_count; i++) {
		tabs[i].vt->ApplyDarkMode(dark);
	}
	SidebarDarkModeChanged();
	DrawMenuBar(frame_wnd);
	RedrawWindow(frame_wnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
}

int FrameTabCount(void)
{
	return tab_count;
}

HWND FrameGetHWND(void)
{
	return frame_wnd;
}
