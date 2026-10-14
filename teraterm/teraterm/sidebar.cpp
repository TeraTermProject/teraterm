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


/*
 *	Sidebar of saved devices
 *
 *	header ("Saved devices" + add button), a search box and a tree:
 *	sites are the folders, devices the leaves. Double click or Enter opens
 *	the device in a new tab.
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "teraterm.h"
#include "tttypes.h"
#include "tt_res.h"
#include "compat_win.h"
#include "darkmode.h"
#include "devices.h"
#include "frame.h"
#include "sidebar.h"
#include "dbglog.h"

#define SIDEBAR_CLASS_NAME L"TeraTermSidebar"
#define DEFAULT_WIDTH 250		// 96 dpi pixels
#define MIN_WIDTH 160
#define MAX_WIDTH 600
#define ID_SEARCH 100
#define ID_TREE 101
#define WM_APP_CONNECT (WM_APP + 1)	// wParam = device index

// context menu
#define IDM_CONNECT 1
#define IDM_EDIT 2
#define IDM_DUPLICATE 3
#define IDM_DELETE 4
#define IDM_ADD 5

static HWND sidebar_wnd;
static HWND search_wnd;
static HWND tree_wnd;
static HWND frame_wnd;
static HINSTANCE sidebar_inst;
static HFONT ui_font;
static HFONT header_font;
static UINT font_dpi;
static HBRUSH bg_brush;
static HBRUSH edit_brush;
static BOOL visible = TRUE;
static int width96 = DEFAULT_WIDTH;
static BOOL hot_add;
static BOOL tracking_mouse;
static BOOL dragging;

// sites the user collapsed (kept while Tera Term runs)
static wchar_t **collapsed;
static int collapsed_count;

typedef HRESULT (WINAPI *SetWindowTheme_t)(HWND, LPCWSTR, LPCWSTR);
static SetWindowTheme_t pSetTheme;

typedef struct {
	COLORREF bg, text, text_quiet, edit_bg, border, hot, accent;
} SidebarColors;

static void GetColors(SidebarColors *c)
{
	if (DarkModeIsEnabled()) {
		c->bg = RGB(32, 32, 32);
		c->text = RGB(230, 230, 230);
		c->text_quiet = RGB(150, 150, 150);
		c->edit_bg = RGB(45, 45, 45);
		c->border = RGB(60, 60, 60);
		c->hot = RGB(60, 60, 60);
		c->accent = RGB(64, 156, 255);
	}
	else {
		c->bg = RGB(243, 243, 243);
		c->text = RGB(28, 28, 28);
		c->text_quiet = RGB(100, 100, 100);
		c->edit_bg = RGB(255, 255, 255);
		c->border = RGB(210, 210, 210);
		c->hot = RGB(222, 222, 222);
		c->accent = RGB(0, 120, 215);
	}
}

static UINT GetDpi(void)
{
	HWND w = sidebar_wnd != NULL ? sidebar_wnd : frame_wnd;
	if (pGetDpiForWindow != NULL && w != NULL) {
		return pGetDpiForWindow(w);
	}
	return 96;
}

static int Scale(int v)
{
	return MulDiv(v, (int)GetDpi(), 96);
}

static int HeaderHeight(void) { return Scale(40); }
static int SearchHeight(void) { return Scale(28); }
static int Margin(void) { return Scale(10); }
static int GripWidth(void) { return Scale(5); }

static void UpdateFonts(void)
{
	const UINT dpi = GetDpi();
	if (ui_font != NULL && font_dpi == dpi) {
		return;
	}
	if (ui_font != NULL) DeleteObject(ui_font);
	if (header_font != NULL) DeleteObject(header_font);
	NONCLIENTMETRICSW ncm = {};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	LOGFONTW lf = ncm.lfMessageFont;
	lf.lfHeight = -MulDiv(9, (int)dpi, 72);
	ui_font = CreateFontIndirectW(&lf);
	lf.lfHeight = -MulDiv(8, (int)dpi, 72);
	lf.lfWeight = FW_SEMIBOLD;
	header_font = CreateFontIndirectW(&lf);
	font_dpi = dpi;
}

static void UpdateBrushes(void)
{
	SidebarColors c;
	GetColors(&c);
	if (bg_brush != NULL) DeleteObject(bg_brush);
	if (edit_brush != NULL) DeleteObject(edit_brush);
	bg_brush = CreateSolidBrush(c.bg);
	edit_brush = CreateSolidBrush(c.edit_bg);
}

static void ApplyTreeTheme(void)
{
	SidebarColors c;
	GetColors(&c);
	if (pSetTheme != NULL) {
		pSetTheme(tree_wnd, DarkModeIsEnabled() ? L"DarkMode_Explorer" : L"Explorer", NULL);
	}
	TreeView_SetBkColor(tree_wnd, c.bg);
	TreeView_SetTextColor(tree_wnd, c.text);
	TreeView_SetLineColor(tree_wnd, c.border);
	TreeView_SetItemHeight(tree_wnd, Scale(26));
	TreeView_SetIndent(tree_wnd, Scale(14));
}

/*
 *	geometry
 */
static void GetAddRect(RECT *r)
{
	RECT rc;
	GetClientRect(sidebar_wnd, &rc);
	const int size = Scale(24);
	r->right = rc.right - GripWidth() - Scale(6);
	r->left = r->right - size;
	r->top = (HeaderHeight() - size) / 2;
	r->bottom = r->top + size;
}

static void GetSearchBox(RECT *r)
{
	RECT rc;
	GetClientRect(sidebar_wnd, &rc);
	r->left = Margin();
	r->right = rc.right - GripWidth() - Margin() + Scale(4);
	r->top = HeaderHeight();
	r->bottom = r->top + SearchHeight();
}

static void Layout(void)
{
	if (sidebar_wnd == NULL) {
		return;
	}
	RECT rc;
	GetClientRect(sidebar_wnd, &rc);
	RECT box;
	GetSearchBox(&box);
	// the edit sits inside a painted box, vertically centered
	HDC dc = GetDC(search_wnd);
	HGDIOBJ old = SelectObject(dc, ui_font);
	TEXTMETRICW tm;
	GetTextMetricsW(dc, &tm);
	SelectObject(dc, old);
	ReleaseDC(search_wnd, dc);
	const int eh = tm.tmHeight + Scale(2);
	SetWindowPos(search_wnd, NULL, box.left + Scale(8), (box.top + box.bottom - eh) / 2,
				 box.right - box.left - Scale(16), eh, SWP_NOZORDER | SWP_NOACTIVATE);
	const int ty = box.bottom + Scale(8);
	SetWindowPos(tree_wnd, NULL, Scale(4), ty, rc.right - GripWidth() - Scale(4), rc.bottom - ty,
				 SWP_NOZORDER | SWP_NOACTIVATE);
}

/*
 *	tree content
 */
static BOOL ContainsI(const wchar_t *text, const wchar_t *word)
{
	if (word[0] == 0) {
		return TRUE;
	}
	const size_t n = wcslen(word);
	for (const wchar_t *p = text; *p != 0; p++) {
		if (_wcsnicmp(p, word, n) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL Matches(const Device *d, const wchar_t *filter)
{
	return ContainsI(d->name, filter) || ContainsI(d->host, filter) ||
		   ContainsI(d->site, filter) || ContainsI(d->user, filter);
}

static BOOL IsCollapsed(const wchar_t *site)
{
	for (int i = 0; i < collapsed_count; i++) {
		if (wcscmp(collapsed[i], site) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static void SetCollapsed(const wchar_t *site, BOOL collapse)
{
	for (int i = 0; i < collapsed_count; i++) {
		if (wcscmp(collapsed[i], site) == 0) {
			if (!collapse) {
				free(collapsed[i]);
				collapsed[i] = collapsed[--collapsed_count];
			}
			return;
		}
	}
	if (collapse) {
		collapsed = (wchar_t **)realloc(collapsed, sizeof(wchar_t *) * (collapsed_count + 1));
		collapsed[collapsed_count++] = _wcsdup(site);
	}
}

static int CompareDevices(const void *a, const void *b)
{
	const Device *da = DevicesGet(*(const int *)a);
	const Device *db = DevicesGet(*(const int *)b);
	// devices without a site go last
	if ((da->site[0] == 0) != (db->site[0] == 0)) {
		return da->site[0] == 0 ? 1 : -1;
	}
	int r = _wcsicmp(da->site, db->site);
	if (r != 0) {
		return r;
	}
	return _wcsicmp(da->name, db->name);
}

static void DeviceLabel(const Device *d, wchar_t *buf, size_t count)
{
	if (d->protocol == DEVICE_SERIAL) {
		swprintf_s(buf, count, L"%s   COM%d", d->name, d->com_port);
	}
	else {
		swprintf_s(buf, count, L"%s   %s", d->name, d->host);
	}
}

static HTREEITEM InsertItem(HTREEITEM parent, const wchar_t *text, LPARAM data, BOOL bold)
{
	TVINSERTSTRUCTW ins = {};
	ins.hParent = parent;
	ins.hInsertAfter = TVI_LAST;
	ins.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	ins.item.pszText = (LPWSTR)text;
	ins.item.lParam = data;
	ins.item.stateMask = TVIS_BOLD;
	ins.item.state = bold ? TVIS_BOLD : 0;
	return (HTREEITEM)SendMessageW(tree_wnd, TVM_INSERTITEMW, 0, (LPARAM)&ins);
}

// item data: device index + 1, 0 = site, -1 = hint
static void Rebuild(const wchar_t *select_id)
{
	wchar_t filter[128];
	GetWindowTextW(search_wnd, filter, _countof(filter));

	SendMessageW(tree_wnd, WM_SETREDRAW, FALSE, 0);
	TreeView_DeleteAllItems(tree_wnd);

	const int count = DevicesCount();
	if (count == 0) {
		InsertItem(TVI_ROOT, L"No saved devices yet", -1, FALSE);
		InsertItem(TVI_ROOT, L"Click + above to add a switch", -1, FALSE);
		SendMessageW(tree_wnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(tree_wnd, NULL, TRUE);
		return;
	}

	int *order = (int *)malloc(sizeof(int) * count);
	int shown = 0;
	for (int i = 0; i < count; i++) {
		if (Matches(DevicesGet(i), filter)) {
			order[shown++] = i;
		}
	}
	qsort(order, shown, sizeof(int), CompareDevices);

	HTREEITEM site_item = NULL;
	const wchar_t *site_name = NULL;
	HTREEITEM select_item = NULL;
	for (int k = 0; k < shown; k++) {
		const Device *d = DevicesGet(order[k]);
		HTREEITEM parent = TVI_ROOT;
		if (d->site[0] != 0) {
			if (site_name == NULL || _wcsicmp(site_name, d->site) != 0) {
				int n = 0;
				for (int j = k; j < shown && _wcsicmp(DevicesGet(order[j])->site, d->site) == 0; j++) {
					n++;
				}
				wchar_t label[160];
				swprintf_s(label, _countof(label), L"%s  (%d)", d->site, n);
				site_item = InsertItem(TVI_ROOT, label, 0, TRUE);
				site_name = d->site;
			}
			parent = site_item;
		}
		wchar_t label[400];
		DeviceLabel(d, label, _countof(label));
		HTREEITEM item = InsertItem(parent, label, order[k] + 1, FALSE);
		if (select_id != NULL && wcscmp(d->id, select_id) == 0) {
			select_item = item;
		}
	}
	if (shown == 0) {
		InsertItem(TVI_ROOT, L"No matching devices", -1, FALSE);
	}

	// expand sites (all of them while searching)
	for (HTREEITEM it = TreeView_GetRoot(tree_wnd); it != NULL; it = TreeView_GetNextSibling(tree_wnd, it)) {
		TVITEMW tv = {};
		tv.mask = TVIF_PARAM;
		tv.hItem = it;
		TreeView_GetItem(tree_wnd, &tv);
		if (tv.lParam != 0) {
			continue;
		}
		HTREEITEM child = TreeView_GetChild(tree_wnd, it);
		tv.lParam = 0;
		tv.hItem = child;
		TreeView_GetItem(tree_wnd, &tv);
		const Device *d = DevicesGet((int)tv.lParam - 1);
		if (filter[0] != 0 || d == NULL || !IsCollapsed(d->site)) {
			TreeView_Expand(tree_wnd, it, TVE_EXPAND);
		}
	}
	free(order);

	if (select_item != NULL) {
		TreeView_EnsureVisible(tree_wnd, select_item);
		TreeView_SelectItem(tree_wnd, select_item);
	}
	SendMessageW(tree_wnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(tree_wnd, NULL, TRUE);
}

static LPARAM ItemData(HTREEITEM item)
{
	if (item == NULL) {
		return -1;
	}
	TVITEMW tv = {};
	tv.mask = TVIF_PARAM;
	tv.hItem = item;
	TreeView_GetItem(tree_wnd, &tv);
	return tv.lParam;
}

static int SelectedDevice(void)
{
	const LPARAM data = ItemData(TreeView_GetSelection(tree_wnd));
	return data > 0 ? (int)data - 1 : -1;
}

static int FirstShownDevice(void)
{
	for (HTREEITEM it = TreeView_GetRoot(tree_wnd); it != NULL; it = TreeView_GetNextSibling(tree_wnd, it)) {
		LPARAM data = ItemData(it);
		if (data > 0) {
			return (int)data - 1;
		}
		if (data == 0) {
			HTREEITEM child = TreeView_GetChild(tree_wnd, it);
			data = ItemData(child);
			if (data > 0) {
				return (int)data - 1;
			}
		}
	}
	return -1;
}

/*
 *	actions
 */
static void Connect(int index)
{
	const Device *d = DevicesGet(index);
	if (d == NULL) {
		return;
	}
	wchar_t password[256];
	password[0] = 0;
	if (d->protocol == DEVICE_SSH && d->user[0] != 0) {
		DevicePasswordGet(d, password, _countof(password));
	}
	wchar_t *cmd = DeviceBuildCommand(d, password);
	SecureZeroMemory(password, sizeof(password));
	DbgLog("sidebar: open saved device (protocol %d)", d->protocol);
	FrameConnect(cmd);
	DeviceFreeCommand(cmd);
}

/*
 *	device dialog
 */
typedef struct {
	Device dev;
	BOOL is_new;
	BOOL had_password;
} DeviceDlgData;

static const DWORD bauds[] = { 9600, 19200, 38400, 57600, 115200 };

static void ShowConnectionFields(HWND dlg)
{
	const int protocol = (int)SendDlgItemMessageW(dlg, IDC_DEVICE_PROTOCOL, CB_GETCURSEL, 0, 0);
	const BOOL serial = protocol == DEVICE_SERIAL;
	const BOOL ssh = protocol == DEVICE_SSH;
	static const int tcp_ids[] = { IDC_DEVICE_HOST_LABEL, IDC_DEVICE_HOST, IDC_DEVICE_PORT_LABEL, IDC_DEVICE_PORT };
	static const int com_ids[] = { IDC_DEVICE_COM_LABEL, IDC_DEVICE_COM, IDC_DEVICE_BAUD_LABEL, IDC_DEVICE_BAUD };
	static const int auth_ids[] = { IDC_DEVICE_USER_LABEL, IDC_DEVICE_USER, IDC_DEVICE_PASS_LABEL, IDC_DEVICE_PASS, IDC_DEVICE_SAVEPASS };
	for (int id : tcp_ids) ShowWindow(GetDlgItem(dlg, id), serial ? SW_HIDE : SW_SHOW);
	for (int id : com_ids) ShowWindow(GetDlgItem(dlg, id), serial ? SW_SHOW : SW_HIDE);
	for (int id : auth_ids) EnableWindow(GetDlgItem(dlg, id), ssh);
	SetDlgItemTextW(dlg, IDC_DEVICE_HINT,
					ssh ? L"Leave the password empty to be asked for it each time you connect."
						: L"Telnet and serial consoles log in by hand for now; the username and password are used for SSH.");
}

static void FillComPorts(HWND dlg, int current)
{
	HWND combo = GetDlgItem(dlg, IDC_DEVICE_COM);
	int select = -1;
	for (int n = 1; n <= 64; n++) {
		wchar_t name[16], target[256];
		swprintf_s(name, _countof(name), L"COM%d", n);
		const BOOL exists = QueryDosDeviceW(name, target, _countof(target)) != 0;
		if (!exists && n != current) {
			continue;
		}
		const int i = (int)SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)name);
		SendMessageW(combo, CB_SETITEMDATA, i, n);
		if (n == current) {
			select = i;
		}
	}
	if (SendMessageW(combo, CB_GETCOUNT, 0, 0) == 0) {
		const int i = (int)SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"COM1");
		SendMessageW(combo, CB_SETITEMDATA, i, 1);
	}
	SendMessageW(combo, CB_SETCURSEL, select >= 0 ? select : 0, 0);
}

static INT_PTR CALLBACK DeviceDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
	DeviceDlgData *data = (DeviceDlgData *)GetWindowLongPtrW(dlg, DWLP_USER);
	switch (msg) {
	case WM_INITDIALOG: {
		data = (DeviceDlgData *)lp;
		SetWindowLongPtrW(dlg, DWLP_USER, (LONG_PTR)data);
		const Device *d = &data->dev;
		SetWindowTextW(dlg, data->is_new ? L"Add saved device" : L"Edit saved device");
		SetDlgItemTextW(dlg, IDC_DEVICE_NAME, d->name);

		// sites already in use
		HWND site = GetDlgItem(dlg, IDC_DEVICE_SITE);
		for (int i = 0; i < DevicesCount(); i++) {
			const wchar_t *s = DevicesGet(i)->site;
			if (s[0] != 0 && SendMessageW(site, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)s) == CB_ERR) {
				SendMessageW(site, CB_ADDSTRING, 0, (LPARAM)s);
			}
		}
		SetWindowTextW(site, d->site);

		HWND proto = GetDlgItem(dlg, IDC_DEVICE_PROTOCOL);
		SendMessageW(proto, CB_ADDSTRING, 0, (LPARAM)L"SSH");
		SendMessageW(proto, CB_ADDSTRING, 0, (LPARAM)L"Telnet");
		SendMessageW(proto, CB_ADDSTRING, 0, (LPARAM)L"Serial console");
		SendMessageW(proto, CB_SETCURSEL, d->protocol, 0);

		SetDlgItemTextW(dlg, IDC_DEVICE_HOST, d->host);
		SetDlgItemInt(dlg, IDC_DEVICE_PORT, d->port, FALSE);
		FillComPorts(dlg, d->com_port);
		HWND baud = GetDlgItem(dlg, IDC_DEVICE_BAUD);
		for (DWORD b : bauds) {
			wchar_t buf[16];
			swprintf_s(buf, _countof(buf), L"%lu", (unsigned long)b);
			SendMessageW(baud, CB_ADDSTRING, 0, (LPARAM)buf);
		}
		SetDlgItemInt(dlg, IDC_DEVICE_BAUD, d->baud, FALSE);
		SetDlgItemTextW(dlg, IDC_DEVICE_USER, d->user);
		SendDlgItemMessageW(dlg, IDC_DEVICE_PASS, EM_SETCUEBANNER, TRUE,
							(LPARAM)(data->had_password ? L"Saved, leave empty to keep it" : L""));
		CheckDlgButton(dlg, IDC_DEVICE_SAVEPASS, (data->is_new || data->had_password) ? BST_CHECKED : BST_UNCHECKED);
		ShowConnectionFields(dlg);
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case IDC_DEVICE_PROTOCOL:
			if (HIWORD(wp) == CBN_SELCHANGE) {
				// switch the default port with the protocol
				const int protocol = (int)SendDlgItemMessageW(dlg, IDC_DEVICE_PROTOCOL, CB_GETCURSEL, 0, 0);
				const UINT port = GetDlgItemInt(dlg, IDC_DEVICE_PORT, NULL, FALSE);
				if (protocol == DEVICE_TELNET && port == 22) SetDlgItemInt(dlg, IDC_DEVICE_PORT, 23, FALSE);
				if (protocol == DEVICE_SSH && port == 23) SetDlgItemInt(dlg, IDC_DEVICE_PORT, 22, FALSE);
				ShowConnectionFields(dlg);
			}
			return TRUE;
		case IDOK: {
			Device *d = &data->dev;
			GetDlgItemTextW(dlg, IDC_DEVICE_NAME, d->name, _countof(d->name));
			GetDlgItemTextW(dlg, IDC_DEVICE_SITE, d->site, _countof(d->site));
			d->protocol = (int)SendDlgItemMessageW(dlg, IDC_DEVICE_PROTOCOL, CB_GETCURSEL, 0, 0);
			GetDlgItemTextW(dlg, IDC_DEVICE_HOST, d->host, _countof(d->host));
			d->port = (int)GetDlgItemInt(dlg, IDC_DEVICE_PORT, NULL, FALSE);
			const int com = (int)SendDlgItemMessageW(dlg, IDC_DEVICE_COM, CB_GETCURSEL, 0, 0);
			d->com_port = com >= 0 ? (int)SendDlgItemMessageW(dlg, IDC_DEVICE_COM, CB_GETITEMDATA, com, 0) : 1;
			d->baud = GetDlgItemInt(dlg, IDC_DEVICE_BAUD, NULL, FALSE);
			GetDlgItemTextW(dlg, IDC_DEVICE_USER, d->user, _countof(d->user));
			// trim the host, a pasted address often has spaces
			wchar_t *h = d->host;
			while (*h == L' ' || *h == L'\t') h++;
			memmove(d->host, h, (wcslen(h) + 1) * sizeof(wchar_t));
			for (size_t n = wcslen(d->host); n > 0 && (d->host[n - 1] == L' ' || d->host[n - 1] == L'\t'); n--) {
				d->host[n - 1] = 0;
			}

			if (d->name[0] == 0) {
				if (d->protocol == DEVICE_SERIAL) {
					swprintf_s(d->name, _countof(d->name), L"COM%d", d->com_port);
				}
				else {
					wcsncpy_s(d->name, _countof(d->name), d->host, _TRUNCATE);
				}
			}
			if (d->protocol != DEVICE_SERIAL && d->host[0] == 0) {
				MessageBoxW(dlg, L"Enter the switch's host name or IP address.", L"Tera Term", MB_OK | MB_ICONINFORMATION);
				SetFocus(GetDlgItem(dlg, IDC_DEVICE_HOST));
				return TRUE;
			}
			if (d->port <= 0 || d->port > 65535) {
				d->port = d->protocol == DEVICE_TELNET ? 23 : 22;
			}
			if (d->baud == 0) {
				d->baud = 9600;
			}
			EndDialog(dlg, IDOK);
			return TRUE;
		}
		case IDCANCEL:
			EndDialog(dlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// the password is read from the dialog before it closes, see EditDevice()
static wchar_t dlg_password[256];
static BOOL dlg_save_password;

static INT_PTR CALLBACK DeviceDlgProcOuter(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_COMMAND && LOWORD(wp) == IDOK) {
		GetDlgItemTextW(dlg, IDC_DEVICE_PASS, dlg_password, _countof(dlg_password));
		dlg_save_password = IsDlgButtonChecked(dlg, IDC_DEVICE_SAVEPASS) == BST_CHECKED;
	}
	return DeviceDlgProc(dlg, msg, wp, lp);
}

/**
 *	@param	index	device to edit, -1 = new device
 *	@param	copy	new device as a copy of index
 */
static void EditDevice(int index, BOOL copy, const wchar_t *site)
{
	DeviceDlgData data = {};
	// a copy, the list is reallocated when a device is added
	Device orig;
	const BOOL have_src = DevicesGet(index) != NULL;
	if (have_src) {
		orig = *DevicesGet(index);
	}
	const Device *src = have_src ? &orig : NULL;
	if (src != NULL) {
		data.dev = *src;
		data.is_new = copy;
		data.had_password = !copy && DeviceHasPassword(src);
		if (copy) {
			wchar_t name[128];
			swprintf_s(name, _countof(name), L"%s copy", src->name);
			wcsncpy_s(data.dev.name, _countof(data.dev.name), name, _TRUNCATE);
		}
	}
	else {
		DeviceInit(&data.dev, DEVICE_SSH);
		data.is_new = TRUE;
		if (site != NULL) {
			wcsncpy_s(data.dev.site, _countof(data.dev.site), site, _TRUNCATE);
		}
	}

	dlg_password[0] = 0;
	const INT_PTR r = DialogBoxParamW(sidebar_inst, MAKEINTRESOURCEW(IDD_DEVICE_DIALOG), frame_wnd,
									  DeviceDlgProcOuter, (LPARAM)&data);
	if (r != IDOK) {
		SecureZeroMemory(dlg_password, sizeof(dlg_password));
		return;
	}

	int saved;
	if (data.is_new) {
		saved = DevicesAdd(&data.dev);
	}
	else {
		DevicesUpdate(index, &data.dev);
		saved = index;
	}
	const Device *d = DevicesGet(saved);
	if (d->protocol != DEVICE_SSH || !dlg_save_password) {
		DevicePasswordSet(d, NULL);
	}
	else if (dlg_password[0] != 0) {
		if (!DevicePasswordSet(d, dlg_password)) {
			MessageBoxW(frame_wnd, L"The password could not be saved in Windows Credential Manager.",
						L"Tera Term", MB_OK | MB_ICONWARNING);
		}
	}
	else if (data.is_new && copy && src != NULL) {
		// a copy keeps the original's password
		wchar_t pw[256];
		if (DevicePasswordGet(src, pw, _countof(pw))) {
			DevicePasswordSet(d, pw);
		}
		SecureZeroMemory(pw, sizeof(pw));
	}
	SecureZeroMemory(dlg_password, sizeof(dlg_password));
	DbgLog("sidebar: device %s", data.is_new ? "added" : "changed");
	Rebuild(d->id);
}

static void DeleteDevice(int index)
{
	const Device *d = DevicesGet(index);
	if (d == NULL) {
		return;
	}
	wchar_t msg[300];
	swprintf_s(msg, _countof(msg), L"Delete %s from saved devices?\nIts saved password is removed too.", d->name);
	if (MessageBoxW(frame_wnd, msg, L"Tera Term", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
		return;
	}
	DevicesDelete(index);
	DbgLog("sidebar: device deleted");
	Rebuild(NULL);
}

static void SiteOfItem(HTREEITEM item, wchar_t *site, size_t count)
{
	site[0] = 0;
	const LPARAM data = ItemData(item);
	if (data > 0) {
		wcsncpy_s(site, count, DevicesGet((int)data - 1)->site, _TRUNCATE);
	}
	else if (data == 0) {
		const LPARAM child = ItemData(TreeView_GetChild(tree_wnd, item));
		if (child > 0) {
			wcsncpy_s(site, count, DevicesGet((int)child - 1)->site, _TRUNCATE);
		}
	}
}

static void ContextMenu(POINT screen)
{
	TVHITTESTINFO hit = {};
	hit.pt = screen;
	ScreenToClient(tree_wnd, &hit.pt);
	HTREEITEM item = TreeView_HitTest(tree_wnd, &hit);
	if (item != NULL) {
		TreeView_SelectItem(tree_wnd, item);
	}
	const LPARAM data = ItemData(item);
	wchar_t site[128];
	SiteOfItem(item, site, _countof(site));

	HMENU menu = CreatePopupMenu();
	if (data > 0) {
		AppendMenuW(menu, MF_STRING, IDM_CONNECT, L"Connect");
		SetMenuDefaultItem(menu, IDM_CONNECT, FALSE);
		AppendMenuW(menu, MF_STRING, IDM_EDIT, L"Edit...");
		AppendMenuW(menu, MF_STRING, IDM_DUPLICATE, L"Duplicate...");
		AppendMenuW(menu, MF_STRING, IDM_DELETE, L"Delete");
		AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	}
	wchar_t add[200];
	if (site[0] != 0) {
		swprintf_s(add, _countof(add), L"Add device to %s...", site);
	}
	else {
		wcscpy_s(add, _countof(add), L"Add device...");
	}
	AppendMenuW(menu, MF_STRING, IDM_ADD, add);

	const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen.x, screen.y, 0, sidebar_wnd, NULL);
	DestroyMenu(menu);
	const int index = data > 0 ? (int)data - 1 : -1;
	switch (cmd) {
	case IDM_CONNECT: Connect(index); break;
	case IDM_EDIT: EditDevice(index, FALSE, NULL); break;
	case IDM_DUPLICATE: EditDevice(index, TRUE, NULL); break;
	case IDM_DELETE: DeleteDevice(index); break;
	case IDM_ADD: EditDevice(-1, FALSE, site[0] != 0 ? site : NULL); break;
	}
}

/*
 *	search box: Enter opens the first match, Down moves to the list, Esc clears
 */
static LRESULT CALLBACK SearchProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR ref)
{
	(void)id; (void)ref;
	switch (msg) {
	case WM_KEYDOWN:
		if (wp == VK_RETURN) {
			const int index = FirstShownDevice();
			if (index >= 0) {
				PostMessageW(sidebar_wnd, WM_APP_CONNECT, index, 0);
			}
			return 0;
		}
		if (wp == VK_DOWN) {
			SetFocus(tree_wnd);
			HTREEITEM first = TreeView_GetRoot(tree_wnd);
			if (first != NULL && TreeView_GetSelection(tree_wnd) == NULL) {
				TreeView_SelectItem(tree_wnd, first);
			}
			return 0;
		}
		if (wp == VK_ESCAPE) {
			SetWindowTextW(hWnd, L"");
			return 0;
		}
		break;
	case WM_CHAR:
		if (wp == L'\r' || wp == 0x1b) {
			return 0;	// no beep
		}
		break;
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	}
	return DefSubclassProc(hWnd, msg, wp, lp);
}

/*
 *	painting
 */
static void FillSolid(HDC dc, const RECT *r, COLORREF c)
{
	SetBkColor(dc, c);
	ExtTextOutW(dc, 0, 0, ETO_OPAQUE, r, NULL, 0, NULL);
}

static void Paint(HDC dc)
{
	RECT rc;
	GetClientRect(sidebar_wnd, &rc);
	SidebarColors c;
	GetColors(&c);
	FillSolid(dc, &rc, c.bg);

	// right edge line
	RECT edge = rc;
	edge.left = edge.right - Scale(1);
	FillSolid(dc, &edge, c.border);

	SetBkMode(dc, TRANSPARENT);
	HGDIOBJ old_font = SelectObject(dc, header_font);
	SetTextColor(dc, c.text_quiet);
	RECT title = rc;
	title.left = Margin() + Scale(2);
	title.bottom = HeaderHeight();
	DrawTextW(dc, L"SAVED DEVICES", -1, &title, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);

	// add button
	RECT add;
	GetAddRect(&add);
	if (hot_add) {
		HBRUSH hb = CreateSolidBrush(c.hot);
		HGDIOBJ ob = SelectObject(dc, hb);
		HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
		RoundRect(dc, add.left, add.top, add.right + 1, add.bottom + 1, Scale(6), Scale(6));
		SelectObject(dc, op);
		SelectObject(dc, ob);
		DeleteObject(hb);
	}
	HPEN pen = CreatePen(PS_SOLID, Scale(1) < 1 ? 1 : Scale(1), c.text);
	HGDIOBJ old_pen = SelectObject(dc, pen);
	const int cx = (add.left + add.right) / 2, cy = (add.top + add.bottom) / 2, len = Scale(6);
	MoveToEx(dc, cx - len, cy, NULL);
	LineTo(dc, cx + len + 1, cy);
	MoveToEx(dc, cx, cy - len, NULL);
	LineTo(dc, cx, cy + len + 1);
	SelectObject(dc, old_pen);
	DeleteObject(pen);

	// search box
	RECT box;
	GetSearchBox(&box);
	HBRUSH eb = CreateSolidBrush(c.edit_bg);
	HPEN bp = CreatePen(PS_SOLID, 1, GetFocus() == search_wnd ? c.accent : c.border);
	HGDIOBJ ob = SelectObject(dc, eb);
	HGDIOBJ op = SelectObject(dc, bp);
	RoundRect(dc, box.left, box.top, box.right, box.bottom, Scale(8), Scale(8));
	SelectObject(dc, op);
	SelectObject(dc, ob);
	DeleteObject(bp);
	DeleteObject(eb);
	SelectObject(dc, old_font);
}

/*
 *	window procedure
 */
static BOOL OnGrip(int x)
{
	RECT rc;
	GetClientRect(sidebar_wnd, &rc);
	return x >= rc.right - GripWidth();
}

static LRESULT CALLBACK SidebarProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		Paint(dc);
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	case WM_SIZE:
		Layout();
		InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case WM_CTLCOLOREDIT: {
		SidebarColors c;
		GetColors(&c);
		SetTextColor((HDC)wp, c.text);
		SetBkColor((HDC)wp, c.edit_bg);
		return (LRESULT)edit_brush;
	}
	case WM_SETCURSOR:
		if (LOWORD(lp) == HTCLIENT) {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			if ((HWND)wp == hWnd && OnGrip(pt.x)) {
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
				return TRUE;
			}
		}
		break;
	case WM_MOUSEMOVE: {
		const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
		if (dragging) {
			int w = MulDiv(x, 96, (int)GetDpi());
			if (w < MIN_WIDTH) w = MIN_WIDTH;
			if (w > MAX_WIDTH) w = MAX_WIDTH;
			if (w != width96) {
				width96 = w;
				FrameLayout();
			}
			return 0;
		}
		if (!tracking_mouse) {
			TRACKMOUSEEVENT tme = {};
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);
			tracking_mouse = TRUE;
		}
		RECT add;
		GetAddRect(&add);
		POINT pt = { x, y };
		const BOOL hot = PtInRect(&add, pt);
		if (hot != hot_add) {
			hot_add = hot;
			InvalidateRect(hWnd, &add, FALSE);
		}
		return 0;
	}
	case WM_MOUSELEAVE:
		tracking_mouse = FALSE;
		if (hot_add) {
			hot_add = FALSE;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		return 0;
	case WM_LBUTTONDOWN: {
		POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
		if (OnGrip(pt.x)) {
			dragging = TRUE;
			SetCapture(hWnd);
			return 0;
		}
		RECT add;
		GetAddRect(&add);
		if (PtInRect(&add, pt)) {
			EditDevice(-1, FALSE, NULL);
		}
		return 0;
	}
	case WM_LBUTTONUP:
		if (dragging) {
			dragging = FALSE;
			ReleaseCapture();
			DevicesSetSidebarWidth(width96);
		}
		return 0;
	case WM_CAPTURECHANGED:
		dragging = FALSE;
		return 0;
	case WM_COMMAND:
		if (LOWORD(wp) == ID_SEARCH) {
			if (HIWORD(wp) == EN_CHANGE) {
				Rebuild(NULL);
			}
			else if (HIWORD(wp) == EN_SETFOCUS || HIWORD(wp) == EN_KILLFOCUS) {
				RECT box;
				GetSearchBox(&box);
				InvalidateRect(hWnd, &box, FALSE);
			}
		}
		return 0;
	case WM_NOTIFY: {
		NMHDR *nm = (NMHDR *)lp;
		if (nm->hwndFrom != tree_wnd) {
			break;
		}
		switch (nm->code) {
		case NM_DBLCLK: {
			const int index = SelectedDevice();
			if (index >= 0) {
				// after the tree is done with the click, so the new tab keeps the focus
				PostMessageW(hWnd, WM_APP_CONNECT, index, 0);
				return TRUE;	// no expand / collapse
			}
			break;
		}
		case NM_RCLICK: {
			POINT pt;
			GetCursorPos(&pt);
			ContextMenu(pt);
			return TRUE;
		}
		case TVN_KEYDOWN: {
			const NMTVKEYDOWN *kd = (const NMTVKEYDOWN *)lp;
			const int index = SelectedDevice();
			if (kd->wVKey == VK_RETURN && index >= 0) {
				PostMessageW(hWnd, WM_APP_CONNECT, index, 0);
				return 1;
			}
			if (kd->wVKey == VK_DELETE && index >= 0) {
				DeleteDevice(index);
				return 1;
			}
			if (kd->wVKey == VK_F2 && index >= 0) {
				EditDevice(index, FALSE, NULL);
				return 1;
			}
			if (kd->wVKey == VK_APPS) {
				HTREEITEM sel = TreeView_GetSelection(tree_wnd);
				RECT r;
				POINT pt = { 0, 0 };
				if (sel != NULL && TreeView_GetItemRect(tree_wnd, sel, &r, TRUE)) {
					pt.x = r.left;
					pt.y = r.bottom;
				}
				ClientToScreen(tree_wnd, &pt);
				ContextMenu(pt);
				return 1;
			}
			break;
		}
		case TVN_ITEMEXPANDEDW: {
			const NMTREEVIEWW *tv = (const NMTREEVIEWW *)lp;
			wchar_t site[128];
			SiteOfItem(tv->itemNew.hItem, site, _countof(site));
			wchar_t filter[8];
			GetWindowTextW(search_wnd, filter, _countof(filter));
			if (site[0] != 0 && filter[0] == 0) {
				SetCollapsed(site, tv->action == TVE_COLLAPSE);
			}
			break;
		}
		}
		break;
	}
	case WM_APP_CONNECT:
		Connect((int)wp);
		return 0;
	case WM_CONTEXTMENU:
		if ((HWND)wp == hWnd) {
			POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
			ContextMenu(pt);
			return 0;
		}
		break;
	}
	return DefWindowProcW(hWnd, msg, wp, lp);
}

/*
 *	public
 */
HWND SidebarCreate(HWND frame, HINSTANCE inst)
{
	frame_wnd = frame;
	sidebar_inst = inst;
	visible = DevicesGetSidebarVisible();
	width96 = DevicesGetSidebarWidth();
	if (width96 < MIN_WIDTH || width96 > MAX_WIDTH) {
		width96 = DEFAULT_WIDTH;
	}

	HMODULE ux = LoadLibraryW(L"uxtheme.dll");
	if (ux != NULL) {
		pSetTheme = (SetWindowTheme_t)GetProcAddress(ux, "SetWindowTheme");
	}

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = SidebarProc;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = SIDEBAR_CLASS_NAME;
	RegisterClassExW(&wc);

	sidebar_wnd = CreateWindowExW(0, SIDEBAR_CLASS_NAME, L"", WS_CHILD | WS_CLIPCHILDREN | (visible ? WS_VISIBLE : 0),
								  0, 0, 0, 0, frame, NULL, inst, NULL);
	if (sidebar_wnd == NULL) {
		return NULL;
	}
	UpdateFonts();
	UpdateBrushes();

	search_wnd = CreateWindowExW(0, WC_EDITW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
								 0, 0, 0, 0, sidebar_wnd, (HMENU)ID_SEARCH, inst, NULL);
	SendMessageW(search_wnd, WM_SETFONT, (WPARAM)ui_font, FALSE);
	SendMessageW(search_wnd, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search devices");
	SetWindowSubclass(search_wnd, SearchProc, 1, 0);

	tree_wnd = CreateWindowExW(0, WC_TREEVIEWW, L"",
							   WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_SHOWSELALWAYS |
							   TVS_FULLROWSELECT | TVS_TRACKSELECT | TVS_NOHSCROLL | TVS_INFOTIP,
							   0, 0, 0, 0, sidebar_wnd, (HMENU)ID_TREE, inst, NULL);
	SendMessageW(tree_wnd, WM_SETFONT, (WPARAM)ui_font, FALSE);
	TreeView_SetExtendedStyle(tree_wnd, TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS,
							  TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS);
	ApplyTreeTheme();
	Rebuild(NULL);
	DbgLog("sidebar: created, %d saved devices", DevicesCount());
	return sidebar_wnd;
}

HWND SidebarGetHWND(void)
{
	return sidebar_wnd;
}

BOOL SidebarIsVisible(void)
{
	return sidebar_wnd != NULL && visible;
}

void SidebarSetVisible(BOOL show)
{
	if (sidebar_wnd == NULL || show == visible) {
		return;
	}
	visible = show;
	ShowWindow(sidebar_wnd, show ? SW_SHOW : SW_HIDE);
	DevicesSetSidebarVisible(show);
	FrameLayout();
	if (show) {
		SetFocus(search_wnd);
	}
}

int SidebarWidth(void)
{
	if (!SidebarIsVisible()) {
		return 0;
	}
	return Scale(width96);
}

void SidebarDarkModeChanged(void)
{
	if (sidebar_wnd == NULL) {
		return;
	}
	UpdateBrushes();
	ApplyTreeTheme();
	RedrawWindow(sidebar_wnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void SidebarDpiChanged(void)
{
	if (sidebar_wnd == NULL) {
		return;
	}
	UpdateFonts();
	SendMessageW(search_wnd, WM_SETFONT, (WPARAM)ui_font, TRUE);
	SendMessageW(tree_wnd, WM_SETFONT, (WPARAM)ui_font, TRUE);
	ApplyTreeTheme();
	Layout();
	InvalidateRect(sidebar_wnd, NULL, FALSE);
}

void SidebarAddDevice(void)
{
	if (sidebar_wnd == NULL) {
		return;
	}
	if (!visible) {
		SidebarSetVisible(TRUE);
	}
	EditDevice(-1, FALSE, NULL);
}

BOOL SidebarOwnsWindow(HWND hWnd)
{
	return sidebar_wnd != NULL && (hWnd == sidebar_wnd || IsChild(sidebar_wnd, hWnd));
}
