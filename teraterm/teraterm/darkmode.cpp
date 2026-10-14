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
#include <commctrl.h>
#include <wchar.h>

#include "compat_win.h"
#include "darkmode.h"

// colors
#define DM_COLOR_BG			RGB(32, 32, 32)
#define DM_COLOR_CTRL_BG	RGB(43, 43, 43)
#define DM_COLOR_HOT		RGB(65, 65, 65)
#define DM_COLOR_TEXT		RGB(230, 230, 230)
#define DM_COLOR_GRAY		RGB(128, 128, 128)

// undocumented menu bar messages
#define WM_UAHDRAWMENU		0x0091
#define WM_UAHDRAWMENUITEM	0x0092

typedef union tagUAHMENUITEMMETRICS {
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizeBar[2];
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizePopup[4];
} UAHMENUITEMMETRICS;

typedef struct tagUAHMENUPOPUPMETRICS {
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2;
} UAHMENUPOPUPMETRICS;

typedef struct tagUAHMENU {
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags;
} UAHMENU;

typedef struct tagUAHMENUITEM {
	int iPosition;
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct tagUAHDRAWMENUITEM {
	DRAWITEMSTRUCT dis;
	UAHMENU um;
	UAHMENUITEM umi;
} UAHDRAWMENUITEM;

// uxtheme.dll ordinals
enum PreferredAppMode {
	AppModeDefault = 0,
	AppModeAllowDark = 1,
	AppModeForceDark = 2,
	AppModeForceLight = 3,
};
typedef PreferredAppMode (WINAPI *SetPreferredAppMode_t)(PreferredAppMode mode);	// 135 (1903-)
typedef BOOL (WINAPI *AllowDarkModeForApp_t)(BOOL allow);							// 135 (1809)
typedef BOOL (WINAPI *AllowDarkModeForWindow_t)(HWND hWnd, BOOL allow);				// 133
typedef void (WINAPI *FlushMenuThemes_t)(void);										// 136
typedef void (WINAPI *RefreshImmersiveColorPolicyState_t)(void);					// 104
typedef HRESULT (WINAPI *SetWindowTheme_t)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

static BOOL initialized;
static BOOL supported;
static BOOL enabled;
static DWORD build_number;
static HHOOK cbt_hook;
static HBRUSH brush_bg;
static HBRUSH brush_ctrl_bg;
static HBRUSH brush_hot;

static SetPreferredAppMode_t pSetPreferredAppMode;
static AllowDarkModeForApp_t pAllowDarkModeForApp;
static AllowDarkModeForWindow_t pAllowDarkModeForWindow;
static FlushMenuThemes_t pFlushMenuThemes;
static RefreshImmersiveColorPolicyState_t pRefreshImmersiveColorPolicyState;
static SetWindowTheme_t pSetWindowTheme;

static const UINT_PTR DIALOG_SUBCLASS_ID = 0x54544d44;	// 'TTMD'

static DWORD GetBuildNumber(void)
{
	typedef void (WINAPI *RtlGetNtVersionNumbers_t)(LPDWORD major, LPDWORD minor, LPDWORD build);
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll == NULL) {
		return 0;
	}
	RtlGetNtVersionNumbers_t pRtlGetNtVersionNumbers =
		(RtlGetNtVersionNumbers_t)GetProcAddress(ntdll, "RtlGetNtVersionNumbers");
	if (pRtlGetNtVersionNumbers == NULL) {
		return 0;
	}
	DWORD major, minor, build;
	pRtlGetNtVersionNumbers(&major, &minor, &build);
	if (major < 10) {
		return 0;
	}
	return build & ~0xF0000000;
}

static void LoadAPIs(void)
{
	if (initialized) {
		return;
	}
	initialized = TRUE;

	brush_bg = CreateSolidBrush(DM_COLOR_BG);
	brush_ctrl_bg = CreateSolidBrush(DM_COLOR_CTRL_BG);
	brush_hot = CreateSolidBrush(DM_COLOR_HOT);

	build_number = GetBuildNumber();
	if (build_number < 17763) {
		// Windows 10 1809 or later is required
		return;
	}
	HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (uxtheme == NULL) {
		return;
	}
	pSetWindowTheme = (SetWindowTheme_t)GetProcAddress(uxtheme, "SetWindowTheme");
	pAllowDarkModeForWindow = (AllowDarkModeForWindow_t)GetProcAddress(uxtheme, MAKEINTRESOURCEA(133));
	pFlushMenuThemes = (FlushMenuThemes_t)GetProcAddress(uxtheme, MAKEINTRESOURCEA(136));
	pRefreshImmersiveColorPolicyState =
		(RefreshImmersiveColorPolicyState_t)GetProcAddress(uxtheme, MAKEINTRESOURCEA(104));
	FARPROC ord135 = GetProcAddress(uxtheme, MAKEINTRESOURCEA(135));
	if (build_number < 18362) {
		pAllowDarkModeForApp = (AllowDarkModeForApp_t)ord135;
	}
	else {
		pSetPreferredAppMode = (SetPreferredAppMode_t)ord135;
	}
	supported = pSetWindowTheme != NULL && pAllowDarkModeForWindow != NULL &&
		(pSetPreferredAppMode != NULL || pAllowDarkModeForApp != NULL);
}

static void SetAppMode(BOOL dark)
{
	if (pSetPreferredAppMode != NULL) {
		pSetPreferredAppMode(dark ? AppModeForceDark : AppModeForceLight);
	}
	else if (pAllowDarkModeForApp != NULL) {
		pAllowDarkModeForApp(dark);
	}
	if (pRefreshImmersiveColorPolicyState != NULL) {
		pRefreshImmersiveColorPolicyState();
	}
	if (pFlushMenuThemes != NULL) {
		pFlushMenuThemes();
	}
}

static void SetTitleBarDark(HWND hWnd, BOOL dark)
{
	if (pDwmSetWindowAttribute == NULL || build_number < 17763) {
		return;
	}
	// DWMWA_USE_IMMERSIVE_DARK_MODE is 20 (19 before Windows 10 20H1)
	const DWORD attr = (build_number >= 18985) ? 20 : 19;
	BOOL value = dark;
	pDwmSetWindowAttribute(hWnd, attr, &value, sizeof(value));
}

static void SetTheme(HWND hWnd, LPCWSTR app_name)
{
	pAllowDarkModeForWindow(hWnd, enabled);
	pSetWindowTheme(hWnd, enabled ? app_name : NULL, NULL);
}

/**
 *	Apply dark (or light) theme to one control in a dialog
 */
static BOOL CALLBACK ApplyToChild(HWND hWnd, LPARAM lParam)
{
	(void)lParam;
	wchar_t class_name[64];
	if (GetClassNameW(hWnd, class_name, _countof(class_name)) == 0) {
		return TRUE;
	}

	if (_wcsicmp(class_name, WC_BUTTONW) == 0) {
		const LONG type = GetWindowLongW(hWnd, GWL_STYLE) & BS_TYPEMASK;
		if (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX ||
			type == BS_3STATE || type == BS_AUTO3STATE ||
			type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON ||
			type == BS_GROUPBOX) {
			// themed check boxes ignore WM_CTLCOLORBTN text color, use classic style
			if (enabled) {
				pSetWindowTheme(hWnd, L"", L"");
			}
			else {
				pSetWindowTheme(hWnd, NULL, NULL);
			}
		}
		else {
			SetTheme(hWnd, L"DarkMode_Explorer");
		}
	}
	else if (_wcsicmp(class_name, WC_EDITW) == 0 ||
			 _wcsicmp(class_name, WC_COMBOBOXW) == 0 ||
			 _wcsicmp(class_name, L"ComboBoxEx32") == 0) {
		SetTheme(hWnd, L"DarkMode_CFD");
	}
	else if (_wcsicmp(class_name, WC_LISTVIEWW) == 0) {
		SetTheme(hWnd, L"DarkMode_Explorer");
		ListView_SetBkColor(hWnd, enabled ? DM_COLOR_CTRL_BG : GetSysColor(COLOR_WINDOW));
		ListView_SetTextBkColor(hWnd, enabled ? DM_COLOR_CTRL_BG : GetSysColor(COLOR_WINDOW));
		ListView_SetTextColor(hWnd, enabled ? DM_COLOR_TEXT : GetSysColor(COLOR_WINDOWTEXT));
		HWND header = ListView_GetHeader(hWnd);
		if (header != NULL) {
			SetTheme(header, L"DarkMode_ItemsView");
		}
	}
	else if (_wcsicmp(class_name, WC_TREEVIEWW) == 0) {
		SetTheme(hWnd, L"DarkMode_Explorer");
		TreeView_SetBkColor(hWnd, enabled ? DM_COLOR_CTRL_BG : (COLORREF)-1);
		TreeView_SetTextColor(hWnd, enabled ? DM_COLOR_TEXT : (COLORREF)-1);
	}
	else if (_wcsicmp(class_name, WC_LISTBOXW) == 0 ||
			 _wcsicmp(class_name, WC_SCROLLBARW) == 0) {
		SetTheme(hWnd, L"DarkMode_Explorer");
	}
	InvalidateRect(hWnd, NULL, TRUE);
	return TRUE;
}

/**
 *	Common file dialogs and other shell dialogs draw themselves, leave them alone
 */
static BOOL CALLBACK FindShellChild(HWND hWnd, LPARAM lParam)
{
	wchar_t class_name[64];
	if (GetClassNameW(hWnd, class_name, _countof(class_name)) != 0) {
		if (wcscmp(class_name, L"DirectUIHWND") == 0 ||
			wcscmp(class_name, L"SHELLDLL_DefView") == 0 ||
			wcscmp(class_name, L"DUIViewWndClassName") == 0) {
			*(BOOL *)lParam = TRUE;
			return FALSE;
		}
	}
	return TRUE;
}

static LRESULT CALLBACK DialogSubclassProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
										   UINT_PTR id, DWORD_PTR ref)
{
	(void)ref;
	switch (msg) {
	case WM_INITDIALOG: {
		LRESULT r = DefSubclassProc(hWnd, msg, wp, lp);
		BOOL shell = FALSE;
		EnumChildWindows(hWnd, FindShellChild, (LPARAM)&shell);
		if (shell) {
			RemoveWindowSubclass(hWnd, DialogSubclassProc, id);
			return r;
		}
		if (enabled) {
			if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD) == 0) {
				SetTitleBarDark(hWnd, TRUE);
			}
			pAllowDarkModeForWindow(hWnd, TRUE);
			EnumChildWindows(hWnd, ApplyToChild, 0);
		}
		return r;
	}
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
		if (enabled) {
			HDC hdc = (HDC)wp;
			SetTextColor(hdc, DM_COLOR_TEXT);
			SetBkColor(hdc, DM_COLOR_BG);
			return (LRESULT)brush_bg;
		}
		break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		if (enabled) {
			HDC hdc = (HDC)wp;
			SetTextColor(hdc, DM_COLOR_TEXT);
			SetBkColor(hdc, DM_COLOR_CTRL_BG);
			return (LRESULT)brush_ctrl_bg;
		}
		break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, DialogSubclassProc, id);
		break;
	}
	return DefSubclassProc(hWnd, msg, wp, lp);
}

/**
 *	Hook window creation on the UI thread to theme every dialog,
 *	including dialogs in TTXs and other Tera Term DLLs
 */
static LRESULT CALLBACK CbtProc(int code, WPARAM wp, LPARAM lp)
{
	if (code == HCBT_CREATEWND && enabled) {
		HWND hWnd = (HWND)wp;
		const CBT_CREATEWNDW *cw = (const CBT_CREATEWNDW *)lp;
		LPCWSTR cls = cw->lpcs->lpszClass;
		BOOL is_dialog;
		if (IS_INTRESOURCE(cls)) {
			is_dialog = (LOWORD((ULONG_PTR)cls) == 0x8002);	// WC_DIALOG
		}
		else {
			is_dialog = (wcscmp(cls, L"#32770") == 0);
		}
		if (is_dialog) {
			HMODULE comdlg = GetModuleHandleW(L"comdlg32.dll");
			HMODULE shell = GetModuleHandleW(L"shell32.dll");
			HINSTANCE inst = cw->lpcs->hInstance;
			if (inst == NULL || (inst != comdlg && inst != shell)) {
				SetWindowSubclass(hWnd, DialogSubclassProc, DIALOG_SUBCLASS_ID, 0);
			}
		}
	}
	return CallNextHookEx(cbt_hook, code, wp, lp);
}

/**
 *	Call once at startup, before the main window is created
 */
void DarkModeInit(BOOL enable)
{
	LoadAPIs();
	if (!supported) {
		return;
	}
	if (cbt_hook == NULL) {
		cbt_hook = SetWindowsHookExW(WH_CBT, CbtProc, NULL, GetCurrentThreadId());
	}
	enabled = enable;
	if (enabled) {
		SetAppMode(TRUE);
	}
}

/**
 *	Turn dark mode on or off while running
 *	Top level windows have to be updated with DarkModeApplyToWindow()
 */
void DarkModeSetEnabled(BOOL enable)
{
	LoadAPIs();
	if (!supported) {
		return;
	}
	enabled = enable;
	SetAppMode(enabled);
}

BOOL DarkModeIsEnabled(void)
{
	return supported && enabled;
}

BOOL DarkModeIsSupported(void)
{
	LoadAPIs();
	return supported;
}

/**
 *	Apply the current mode to a top level window (VT / TEK window)
 *	title bar, scroll bars and menu bar
 */
void DarkModeApplyToWindow(HWND hWnd)
{
	if (!supported) {
		return;
	}
	pAllowDarkModeForWindow(hWnd, enabled);
	SetTitleBarDark(hWnd, enabled);
	// scroll bars of the window
	pSetWindowTheme(hWnd, enabled ? L"DarkMode_Explorer" : NULL, NULL);
	DrawMenuBar(hWnd);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
}

static void DrawMenuBarBottomLine(HWND hWnd)
{
	MENUBARINFO mbi = {};
	mbi.cbSize = sizeof(mbi);
	if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi)) {
		return;
	}
	RECT client;
	GetClientRect(hWnd, &client);
	MapWindowPoints(hWnd, NULL, (POINT *)&client, 2);
	RECT window;
	GetWindowRect(hWnd, &window);
	OffsetRect(&client, -window.left, -window.top);

	RECT line = client;
	line.bottom = line.top;
	line.top--;
	HDC hdc = GetWindowDC(hWnd);
	FillRect(hdc, &line, brush_bg);
	ReleaseDC(hWnd, hdc);
}

/**
 *	Draw the menu bar in dark colors
 *	Call from the window procedure of a window with a menu bar
 *
 *	@retval	TRUE	message was handled, return *result
 */
BOOL DarkModeHandleNCMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
	if (!supported || !enabled) {
		return FALSE;
	}

	switch (msg) {
	case WM_UAHDRAWMENU: {
		const UAHMENU *um = (const UAHMENU *)lp;
		MENUBARINFO mbi = {};
		mbi.cbSize = sizeof(mbi);
		GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);
		RECT window;
		GetWindowRect(hWnd, &window);
		RECT rc = mbi.rcBar;
		OffsetRect(&rc, -window.left, -window.top);
		FillRect(um->hdc, &rc, brush_bg);
		*result = TRUE;
		return TRUE;
	}
	case WM_UAHDRAWMENUITEM: {
		const UAHDRAWMENUITEM *di = (const UAHDRAWMENUITEM *)lp;
		wchar_t text[256] = {};
		MENUITEMINFOW mii = {};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = text;
		mii.cch = _countof(text) - 1;
		GetMenuItemInfoW(di->um.hmenu, di->umi.iPosition, TRUE, &mii);

		const UINT state = di->dis.itemState;
		HBRUSH bg = brush_bg;
		COLORREF fg = DM_COLOR_TEXT;
		if (state & (ODS_HOTLIGHT | ODS_SELECTED)) {
			bg = brush_hot;
		}
		if (state & (ODS_GRAYED | ODS_DISABLED | ODS_INACTIVE)) {
			fg = DM_COLOR_GRAY;
		}
		UINT flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
		if (state & ODS_NOACCEL) {
			flags |= DT_HIDEPREFIX;
		}

		RECT rc = di->dis.rcItem;
		FillRect(di->um.hdc, &rc, bg);
		const int old_mode = SetBkMode(di->um.hdc, TRANSPARENT);
		const COLORREF old_color = SetTextColor(di->um.hdc, fg);
		DrawTextW(di->um.hdc, text, -1, &rc, flags);
		SetTextColor(di->um.hdc, old_color);
		SetBkMode(di->um.hdc, old_mode);
		*result = TRUE;
		return TRUE;
	}
	case WM_NCPAINT:
	case WM_NCACTIVATE:
		*result = DefWindowProcW(hWnd, msg, wp, lp);
		DrawMenuBarBottomLine(hWnd);
		return TRUE;
	}
	return FALSE;
}
