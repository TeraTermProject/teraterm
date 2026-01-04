/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2022- TeraTerm Project
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

#include "ttcommon.h"
#include "ttdialog.h"
#include "commlib.h"
#include "ttlib.h"
#include "dlglib.h"

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <assert.h>

#include <shlobj.h>
#include <windows.h>
#include <wchar.h>
#include <htmlhelp.h>

#include "compat_win.h"
#include "codeconv.h"
#include "asprintf.h"
#include "win32helper.h"
#include "tipwin2.h"
#include "dlglib.h"
#include "helpid.h"
#include "vtdisp.h"
#include "tmfc.h"
#include "tmfc_propdlg.h"

#include "theme.h"
#include "themedlg_res.h"
#include "themedlg.h"

typedef struct ColorSampleDataSt ColorSampleData_t;

//////////////////////////////////////////////////////////////////////////////
typedef struct ColorSampleDataSt {
	int dummy;
	WNDPROC OrigProc;
	HWND hWnd;
	COLORREF color;
} ColorSampleData_t;

static void DispColorSample(HWND hWnd, COLORREF color)
{
	RECT rect;
	HDC hDC = GetDC(hWnd);
	HBRUSH hBrush = CreateSolidBrush(color);
	GetClientRect(hWnd, &rect);
	FillRect(hDC, &rect, hBrush);
	DeleteObject(hBrush);
	ReleaseDC(hWnd, hDC);
}

static LRESULT CALLBACK ColorSampleProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ColorSampleData_t *self = (ColorSampleData_t *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	LRESULT result;

	result = CallWindowProcW(self->OrigProc, hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_PAINT: {
		DispColorSample(hWnd, self->color);
		result = TRUE;
		break;
	}
	case WM_NCDESTROY:
		free(self);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
		break;
	default:
		break;
	}
	return result;
}

/**
 *	ダイアログ上のフレームをカラーサンプルにする
 *	ColorSampleSetColor()で色を設定する
 *	ダイアログが閉じるときに自動的に破棄される
 *
 */
ColorSampleData_t *ColorSampleInit(HWND hDlg, int ID, COLORREF color)
{
	ColorSampleData_t *self = (ColorSampleData_t *)calloc(1, sizeof(ColorSampleData_t));
	HWND hWnd = GetDlgItem(hDlg, ID);
	self->color = color;
	self->hWnd = hWnd;
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
	self->OrigProc = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
	SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)ColorSampleProc);
	return self;
}

/**
 *	色を設定する
 */
void ColorSampleSetColor(ColorSampleData_t *cs, COLORREF color)
{
	ColorSampleData_t *self = cs;
	self->color = color;
	InvalidateRect(self->hWnd, NULL, FALSE);
}

//////////////////////////////////////////////////////////////////////////////

typedef struct {
	// 共通
	HINSTANCE hInst;
	TComVar *pcv;
	TTTSet *pts;
	HWND hVTWin;
	// file tab
	struct {
		int dummy;
	} FileTab;
	// bg theme tab
	struct {
		ColorSampleData_t *cs_bg;
		ColorSampleData_t *cs_splane;
		BGTheme bg_theme;
	} BGTab;
	// color theme tab
	struct {
		TipWin2 *tipwin;
		TColorTheme color_theme;
	} ColorTab;
	struct {
		BGTheme bg_theme;
		TColorTheme color_theme;
	} Backup;
} ThemeDlgData;

static void SetWindowTextColor(HWND hWnd, COLORREF color)
{
	char str[32];
	sprintf_s(str, "%02x%02x%02x", GetRValue(color), GetGValue(color), GetBValue(color));
	SetWindowTextA(hWnd, str);
}

static void SetDlgItemTextColor(HWND hDlg, int ID, COLORREF color)
{
	SetWindowTextColor(GetDlgItem(hDlg, ID), color);
}

static COLORREF GetWindowTextColor(HWND hWnd)
{
	char str[32];
	unsigned int r, g, b;
	char elem[3];

	GetWindowTextA(hWnd, str, _countof(str));

	memcpy(elem, &str[0], 2);
	elem[2] = 0;
	r = 0;
	sscanf_s(elem, "%x", &r);

	memcpy(elem, &str[2], 2);
	elem[2] = 0;
	g = 0;
	sscanf_s(elem, "%x", &g);

	memcpy(elem, &str[4], 2);
	elem[2] = 0;
	b = 0;
	sscanf_s(elem, "%x", &b);

	return RGB(r, g, b);
}

static COLORREF GetDlgItemTextColor(HWND hDlg, int ID)
{
	return GetWindowTextColor(GetDlgItem(hDlg, ID));
}

static void ResetControls(HWND hWnd, ThemeDlgData* dlg_data)
{
	const BGTheme *bg_theme = &dlg_data->BGTab.bg_theme;

	// bg image
	SendDlgItemMessageA(hWnd, IDC_BGIMG_CHECK, BM_SETCHECK, bg_theme->BGDest.enable, 0);
	SetDlgItemTextW(hWnd, IDC_BGIMG_EDIT, bg_theme->BGDest.file);
	{
		LRESULT count = SendDlgItemMessageA(hWnd, IDC_BGIMG_COMBO, CB_GETCOUNT, 0, 0);
		int sel = 0;
		int i;
		for (i = 0; i < count; i++) {
			BG_PATTERN pattern = (BG_PATTERN)SendDlgItemMessageW(hWnd, IDC_BGIMG_COMBO, CB_GETITEMDATA, i, 0);
			if (pattern == bg_theme->BGDest.pattern) {
				sel = i;
				break;
			}
		}
		SendDlgItemMessageA(hWnd, IDC_BGIMG_COMBO, CB_SETCURSEL, sel, 0);
	}
	SendDlgItemMessageA(hWnd, IDC_BGIMG_ALPHA_SLIDER, TBM_SETPOS, TRUE, bg_theme->BGDest.alpha);
	ColorSampleSetColor(dlg_data->BGTab.cs_bg, bg_theme->BGDest.color);

	// wall paper
	SendDlgItemMessageA(hWnd, IDC_WALLPAPER_CHECK, BM_SETCHECK, bg_theme->BGSrc1.enable, 0);

	// simple color plane
	SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_CHECK, BM_SETCHECK, bg_theme->BGSrc2.enable, 0);
	SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA_SLIDER, TBM_SETPOS, TRUE, bg_theme->BGSrc2.alpha);
	ColorSampleSetColor(dlg_data->BGTab.cs_splane, bg_theme->BGSrc2.color);
}

static void ControlEnable(HWND hWnd)
{
	static const int scp_controls[] = {
		IDC_SIMPLE_COLOR_PLANE_COLOR_TITLE,
		IDC_SIMPLE_COLOR_PLANE_SAMPLE,
		IDC_SIMPLE_COLOR_PLANE_BUTTON,
		IDC_SIMPLE_COLOR_PLANE_ALPHA_TITLE,
		IDC_SIMPLE_COLOR_PLANE_ALPHA_SLIDER,
	};
	static const int bg_controls[] = {
		IDC_BGIMG_FILE_TITLE,
		IDC_BGIMG_EDIT,
		IDC_BGIMG_BUTTON,
		IDC_BGIMG_PATTERN_TITLE,
		IDC_BGIMG_COMBO,
		IDC_BGIMG_COLOR_TITLE,
		IDC_BGIMG_COLOR_SAMPLE,
		IDC_BGIMG_COLOR_BUTTON,
		IDC_BGIMG_ALPHA_TITLE,
		IDC_BGIMG_ALPHA_SLIDER
	};
	int i;
	BOOL enable;

	BOOL scp_enable = (BOOL)SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_CHECK, BM_GETCHECK, 0, 0);
	BOOL bg_enable = (BOOL)SendDlgItemMessageA(hWnd, IDC_BGIMG_CHECK, BM_GETCHECK, 0, 0);
	BOOL wp_enable = (BOOL)SendDlgItemMessageA(hWnd, IDC_WALLPAPER_CHECK, BM_GETCHECK, 0, 0);

	// Simple color plane
	enable = scp_enable;
	for (i = 0; i < _countof(scp_controls); i++) {
		EnableWindow(GetDlgItem(hWnd, scp_controls[i]), enable);
	}
	enable = scp_enable && (bg_enable || wp_enable) ? TRUE : FALSE;
	EnableWindow(GetDlgItem(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA_TITLE), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA_SLIDER), enable);

	// BG image
	enable = bg_enable;
	for (i = 0; i < _countof(bg_controls); i++) {
		EnableWindow(GetDlgItem(hWnd, bg_controls[i]), enable);
	}
}

static void ReadFromDialog(HWND hWnd, BGTheme* bg_theme)
{
	LRESULT checked;
	LRESULT index;

	// bg_image
	checked = SendDlgItemMessageA(hWnd, IDC_BGIMG_CHECK, BM_GETCHECK, 0, 0);
	bg_theme->BGDest.enable = checked & BST_CHECKED;
	GetDlgItemTextW(hWnd, IDC_BGIMG_EDIT, bg_theme->BGDest.file, _countof(bg_theme->BGDest.file));
	index = SendDlgItemMessageA(hWnd, IDC_BGIMG_COMBO, CB_GETCURSEL, 0, 0);
	bg_theme->BGDest.pattern = (BG_PATTERN)SendDlgItemMessageA(hWnd, IDC_BGIMG_COMBO, CB_GETITEMDATA, index, 0);
	bg_theme->BGDest.alpha = (BYTE)SendDlgItemMessageA(hWnd, IDC_BGIMG_ALPHA_SLIDER, TBM_GETPOS, 0, 0);

	// wall paper
	checked = SendDlgItemMessageA(hWnd, IDC_WALLPAPER_CHECK, BM_GETCHECK, 0, 0);
	bg_theme->BGSrc1.enable = checked & BST_CHECKED;

	// simple color plane
	checked = SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_CHECK, BM_GETCHECK, 0, 0);
	bg_theme->BGSrc2.enable = checked & BST_CHECKED;
	bg_theme->BGSrc2.alpha = (BYTE)SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA_SLIDER, TBM_GETPOS, 0, 0);
}

static BOOL ChooseColor(HWND hWnd, COLORREF *color_ptr)
{
	static COLORREF CustColors[16];
	CHOOSECOLORA cc = {};
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hWnd;
	cc.rgbResult = *color_ptr;
	cc.lpCustColors = CustColors;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (ChooseColorA(&cc)) {
		*color_ptr = cc.rgbResult;
		return TRUE;
	}
	return FALSE;
}

static INT_PTR CALLBACK BGThemeProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	ThemeDlgData* dlg_data = (ThemeDlgData*)GetWindowLongPtr(hWnd, DWLP_USER);

	switch (msg) {
		case WM_INITDIALOG: {
			static const DlgTextInfo TextInfos[] = {
				{ IDC_BGIMG_CHECK, "DLG_THEME_BG_IMAGE" },
			};
			dlg_data = (ThemeDlgData*)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);

			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), dlg_data->pts->UILanguageFileW);

			SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
			SendDlgItemMessageA(hWnd, IDC_BGIMG_ALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 255));

			for (int i = 0;; i++) {
				LRESULT index;
				const BG_PATTERN_ST *st = ThemeBGPatternList(i);
				if (st == NULL) {
					break;
				}
				index = SendDlgItemMessageW(hWnd, IDC_BGIMG_COMBO, CB_ADDSTRING, 0, (LPARAM)st->str);
				SendDlgItemMessageW(hWnd, IDC_BGIMG_COMBO, CB_SETITEMDATA, index, st->id);
			}

			dlg_data->BGTab.cs_bg =
				ColorSampleInit(hWnd, IDC_BGIMG_COLOR_SAMPLE, dlg_data->BGTab.bg_theme.BGDest.color);
			dlg_data->BGTab.cs_splane =
				ColorSampleInit(hWnd, IDC_SIMPLE_COLOR_PLANE_SAMPLE, dlg_data->BGTab.bg_theme.BGSrc2.color);
			ResetControls(hWnd, dlg_data);

			return TRUE;
			break;
		}
		case WM_COMMAND: {
			switch (wp) {
			case IDC_BGIMG_BUTTON | (BN_CLICKED << 16): {
				// 画像ファイル選択
				wchar_t *bg_file;
				hGetDlgItemTextW(hWnd, IDC_BGIMG_EDIT, &bg_file);

				TTOPENFILENAMEW ofn = {};
				ofn.hwndOwner   = hWnd;
				ofn.lpstrFile   = bg_file;
				//ofn.lpstrFilter = "";
				//ofn.nFilterIndex = 1;
				ofn.hInstance = dlg_data->hInst;
				ofn.lpstrDefExt = L"jpg";
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
				ofn.lpstrTitle = L"select bg image file";

				wchar_t *filename;
				BOOL r = TTGetOpenFileNameW(&ofn, &filename);
				free(bg_file);
				if (r) {
					SetDlgItemTextW(hWnd, IDC_BGIMG_EDIT, filename);
					free(filename);
				}
				break;
			}
			case IDC_BGIMG_COLOR_BUTTON | (BN_CLICKED << 16):
				if (ChooseColor(hWnd, &dlg_data->BGTab.bg_theme.BGDest.color)) {
					ColorSampleSetColor(dlg_data->BGTab.cs_bg,
										dlg_data->BGTab.bg_theme.BGDest.color);
				}
				break;
			case IDC_SIMPLE_COLOR_PLANE_BUTTON | (BN_CLICKED << 16):
				if (ChooseColor(hWnd, &dlg_data->BGTab.bg_theme.BGSrc2.color)) {
					ColorSampleSetColor(dlg_data->BGTab.cs_splane,
										dlg_data->BGTab.bg_theme.BGSrc2.color);
				}
				break;
			case IDC_SIMPLE_COLOR_PLANE_CHECK | (BN_CLICKED << 16):
			case IDC_BGIMG_CHECK | (BN_CLICKED << 16):
			case IDC_WALLPAPER_CHECK | (BN_CLICKED << 16): {
				ControlEnable(hWnd);
				break;
			}
			default:
				break;
			}
			break;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
			case PSN_APPLY: {
				break;
			}
			case PSN_HELP:
				OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupThemeEditor);
				break;
			case PSN_KILLACTIVE: {
				ReadFromDialog(hWnd, &dlg_data->BGTab.bg_theme);
				break;
			}
			case PSN_SETACTIVE: {
				ResetControls(hWnd, dlg_data);
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			return FALSE;
	}
	return FALSE;
}

static UINT CALLBACK BGCallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	UINT ret_val = 0;
	(void)hwnd;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE:
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

static HPROPSHEETPAGE ThemeEditorCreate(ThemeDlgData *dlg_data)
{
	const int id = IDD_TABSHEET_BG_THEME_EDITOR;
	HINSTANCE inst = dlg_data->hInst;

	wchar_t *title;
	GetI18nStrWW("Tera Term", "DLG_THEME_BG_TITLE",
				 L"Background", dlg_data->pts->UILanguageFileW, &title);

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = BGCallBack;
	psp.pszTitle = title;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));

	psp.pfnDlgProc = BGThemeProc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(title);
	return hpsp;
}

//////////////////////////////////////////////////////////////////////////////

static struct {
	const wchar_t *name;
	COLORREF color;
} list[] = {
	{ L"VTColor Fore", RGB(0,0,0) },
	{ L"VTColor Back", RGB(0,0,0) },
	{ L"VTBoldColor Fore", RGB(0,0,0) },
	{ L"VTBoldColor Back", RGB(0,0,0) },
	{ L"VTUnderlineColor Fore", RGB(0,0,0) },
	{ L"VTUnderlineColor Back", RGB(0,0,0) },
	{ L"VTBlinkColor Fore", RGB(0,0,0) },
	{ L"VTBlinkColor Back", RGB(0,0,0) },
	{ L"VTReverseColor Fore", RGB(0,0,0) },
	{ L"VTReverseColor Back", RGB(0,0,0) },
	{ L"URLColor Fore", RGB(0,0,0) },
	{ L"URLColor Back", RGB(0,0,0) },
	{ L"ANSI  0 / Black", RGB(0,0,0) },
	{ L"ANSI  1 / Red", RGB(197,15,31) },
	{ L"ANSI  2 / Green", RGB(19, 161, 14) },
	{ L"ANSI  3 / Yellow", RGB(193, 156, 0) },
	{ L"ANSI  4 / Blue", RGB(0, 55, 218) },
	{ L"ANSI  5 / Magenta", RGB(136, 23, 152) },
	{ L"ANSI  6 / Cyan", RGB(58, 150, 221) },
	{ L"ANSI  7 / White", RGB(204, 204, 204) },
 	{ L"ANSI  8 / Bright Black (Gray)", RGB(118, 118, 118) },
 	{ L"ANSI  9 / Bright Red", RGB(231, 72, 86) },
 	{ L"ANSI 10 / Bright Green", RGB(22, 198, 12) },
 	{ L"ANSI 11 / Bright Yellow", RGB(249, 241, 165) },
 	{ L"ANSI 12 / Bright Blue", RGB(59, 120, 255) },
 	{ L"ANSI 13 / Bright Magenta", RGB(180, 0, 158) },
 	{ L"ANSI 14 / Bright Cyan", RGB(97, 214, 214) },
 	{ L"ANSI 15 / Bright White", RGB(242, 242, 242) },
};

static void SetColor(const TColorTheme *data)
{
	list[0].color = data->vt.fg;
	list[1].color = data->vt.bg;
	list[2].color = data->bold.fg;
	list[3].color = data->bold.bg;
	list[4].color = data->underline.fg;
	list[5].color = data->underline.bg;
	list[6].color = data->blink.fg;
	list[7].color = data->blink.bg;
	list[8].color = data->reverse.fg;
	list[9].color = data->reverse.bg;
	list[10].color = data->url.fg;
	list[11].color = data->url.bg;
	for (int i = 0; i < 16; i++) {
		list[12+i].color = data->ansicolor.color[i];
	}
}

static void RestoreColor(TColorTheme *data)
{
	data->vt.fg = list[0].color;
	data->vt.bg = list[1].color;
	data->bold.fg = list[2].color;
	data->bold.bg = list[3].color;
	data->underline.fg = list[4].color;
	data->underline.bg = list[5].color;
	data->blink.fg = list[6].color;
	data->blink.bg = list[7].color;
	data->reverse.fg = list[8].color;
	data->reverse.bg = list[9].color;
	data->url.fg = list[10].color;
	data->url.bg = list[11].color;
	for (int i = 0; i < 16; i++) {
		data->ansicolor.color[i] = list[12+i].color;
	}
}

static void SetColorListCtrlValue(HWND hWndList, int y)
{
	LVITEMW item;

	const COLORREF c = list[y].color;
	const int r = GetRValue(c);
	const int g = GetGValue(c);
	const int b = GetBValue(c);

	wchar_t color_str[64];
	swprintf_s(color_str, L"#%02x%02x%02x", r,g,b);
	item.mask = LVIF_TEXT;
	item.iItem = y;
	item.iSubItem = 2;
	item.pszText = color_str;
	SendMessage(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

	swprintf_s(color_str, L"%d,%d,%d", r,g,b);
	item.iSubItem = 3;
	item.pszText = color_str;
	SendMessage(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);
}

static void SetColorListCtrl(HWND hWnd)
{
	HWND hWndList = GetDlgItem(hWnd, IDC_COLOR_LIST);
	ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	SendMessage(hWndList, LVM_DELETEALLITEMS, 0, 0);

	for (int y = 0; y< _countof(list); y++) {
		LVITEMW item;
		item.mask = LVIF_TEXT;
		item.iItem = y;
		item.iSubItem = 0;
		item.pszText = (LPWSTR)list[y].name;
		SendMessage(hWndList, LVM_INSERTITEMW, 0, (LPARAM)&item);

		item.iSubItem = 1;
		item.pszText = NULL;
		SendMessage(hWndList, LVM_SETITEMW, 0, (LPARAM)&item);

		SetColorListCtrlValue(hWndList, y);
	}

	// 幅を調整
	for (int i = 0; i < 4; i++) {
		ListView_SetColumnWidth(hWndList, i, i == 1 ? LVSCW_AUTOSIZE_USEHEADER : LVSCW_AUTOSIZE);
	}
}

static INT_PTR CALLBACK ColorThemeProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{IDC_COLOR_DEFAULT_BUTTON, "BTN_DEFAULT"},
	};
	ThemeDlgData *dlg_data = (ThemeDlgData *)GetWindowLongPtr(hWnd, DWLP_USER);
	TTTSet *ts = dlg_data == NULL ? NULL : dlg_data->pts;

	switch (msg) {
	case WM_INITDIALOG: {
		dlg_data = (ThemeDlgData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
		ts = dlg_data->pts;
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);
		SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

		dlg_data->ColorTab.tipwin = TipWin2Create(NULL, hWnd);
		TipWin2SetTextW(dlg_data->ColorTab.tipwin, IDC_COLOR_LIST, L"Double click to open color picker");

		{
			HWND hWndList = GetDlgItem(hWnd, IDC_COLOR_LIST);
			LV_COLUMNA lvcol;
			lvcol.mask = LVCF_TEXT | LVCF_SUBITEM;
			lvcol.pszText = (LPSTR)"name";
			lvcol.iSubItem = 0;
			//ListView_InsertColumn(hWndList, 0, &lvcol);
			SendMessage(hWndList, LVM_INSERTCOLUMNA, 0, (LPARAM)&lvcol);

			lvcol.pszText = (LPSTR)"color";
			lvcol.iSubItem = 1;
			//ListView_InsertColumn(hWndList, 1, &lvcol);
			SendMessage(hWndList, LVM_INSERTCOLUMNA, 1, (LPARAM)&lvcol);

			lvcol.pszText = (LPSTR)"value(#RRGGBB)";
			lvcol.iSubItem = 2;
			//ListView_InsertColumn(hWndList, 1, &lvcol);
			SendMessage(hWndList, LVM_INSERTCOLUMNA, 2, (LPARAM)&lvcol);

			lvcol.pszText = (LPSTR)"value(R, G, B)";
			lvcol.iSubItem = 3;
			//ListView_InsertColumn(hWndList, 1, &lvcol);
			SendMessage(hWndList, LVM_INSERTCOLUMNA, 3, (LPARAM)&lvcol);
		}

		SetColor(&dlg_data->ColorTab.color_theme);
		SetColorListCtrl(hWnd);
		break;
	}
	case WM_COMMAND: {
		switch (wp) {
		case IDC_COLOR_DEFAULT_BUTTON | (BN_CLICKED << 16): {
			// デフォルト
			ThemeGetColorDefault(&dlg_data->ColorTab.color_theme);
			SetColor(&dlg_data->ColorTab.color_theme);
			SetColorListCtrl(hWnd);
			break;
		}
		default:
			break;
		}
		break;
	}
	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lp;
		switch (nmhdr->idFrom) {
		case IDC_COLOR_LIST: {
			NMLISTVIEW *nmlist = (NMLISTVIEW *)lp;
			switch (nmlist->hdr.code) {
			case NM_CUSTOMDRAW: {
				NMLVCUSTOMDRAW *lpLvCustomDraw = (LPNMLVCUSTOMDRAW)lp;
				switch (lpLvCustomDraw->nmcd.dwDrawStage) {
				case CDDS_PREPAINT: {
					// go CDDS_ITEMPREPAINT stage
					SetWindowLongW(hWnd, DWLP_MSGRESULT, (LONG)CDRF_NOTIFYITEMDRAW);
					return TRUE;
				}
				case CDDS_ITEMPREPAINT: {
					// go CDDS_SUBITEM stage
					SetWindowLongW(hWnd, DWLP_MSGRESULT, (LONG)CDRF_NOTIFYSUBITEMDRAW);
					return TRUE;
				}
				case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
				case CDDS_SUBITEM: {
					// 行と列指定
					// if (lpLvCustomDraw->nmcd.dwItemSpec == 1 && lpLvCustomDraw->iSubItem == 1) {
					if (lpLvCustomDraw->iSubItem == 1) {
						lpLvCustomDraw->clrText = RGB(0x80, 0x80, 0x80);
						lpLvCustomDraw->clrTextBk = list[lpLvCustomDraw->nmcd.dwItemSpec].color;
					}
					else
					{
						// 色を変更しないセルのデフォルト色を指定
						// これを指定しない場合、色を変更した列以降は、同じ色になってしまう

						// 文字のデフォルト色
						lpLvCustomDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);

						// 背景のデフォルト色
						lpLvCustomDraw->clrTextBk = GetSysColor(COLOR_WINDOW);
					}
					SetWindowLongW(hWnd, DWLP_MSGRESULT, (LONG)CDRF_NEWFONT);
					return TRUE;
				}
				default:
					SetWindowLongW(hWnd, DWLP_MSGRESULT, (LONG)CDRF_DODEFAULT);
					return TRUE;
				}
				break;
			}
//			case NM_CLICK:
			case NM_DBLCLK:
			case NM_RCLICK: {
				static COLORREF CustColors[16];		// TODO 一時的な色保存必要?
				int i = nmlist->iItem;
				CHOOSECOLORA cc = {};
				cc.lStructSize = sizeof(cc);
				cc.hwndOwner = hWnd;
				cc.rgbResult = list[i].color;
				cc.lpCustColors = CustColors;
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;
				if (ChooseColorA(&cc)) {
					list[i].color = cc.rgbResult;
					SetColorListCtrlValue(nmlist->hdr.hwndFrom, i);
					InvalidateRect(nmlist->hdr.hwndFrom, NULL, TRUE);
				}
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		switch (nmhdr->code) {
		case PSN_APPLY: {
			break;
		}
		case PSN_HELP:
			OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupThemeEditor);
			break;
		case PSN_KILLACTIVE: {
			RestoreColor(&dlg_data->ColorTab.color_theme);
			break;
		}
		case PSN_SETACTIVE: {
			SetColor(&dlg_data->ColorTab.color_theme);
			SetColorListCtrl(hWnd);
			break;
		}
		case TTN_POP:
			// 1回だけ表示するため、閉じたら削除する
			TipWin2SetTextW(dlg_data->ColorTab.tipwin, IDC_COLOR_LIST, NULL);
			break;
		default:
			break;
		}
		break;
	}
	case WM_DESTROY:
		TipWin2Destroy(dlg_data->ColorTab.tipwin);
		dlg_data->ColorTab.tipwin = NULL;
		break;

	default:
		return FALSE;
	}
	return FALSE;
}

static UINT CALLBACK ColorCallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	(void)hwnd;
	UINT ret_val = 0;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE:
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

static HPROPSHEETPAGE ColorThemeEditorCreate(ThemeDlgData *dlg_data)
{
	const int id = IDD_TABSHEET_COLOR_THEME_EDITOR;
	HINSTANCE inst = dlg_data->hInst;

	wchar_t *title;
	GetI18nStrWW("Tera Term", "DLG_THEME_COLOR_TITLE",
				 L"Text color", dlg_data->pts->UILanguageFileW, &title);

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = ColorCallBack;
	psp.pszTitle = title;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if 1
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
#endif

	psp.pfnDlgProc = ColorThemeProc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(title);
	return hpsp;
}

//////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK FileProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_FILE_LOAD_BUTTON, "DLG_THEME_PREVIEW_FILE_LOAD" },
		{ IDC_FILE_SAVE_BUTTON, "DLG_THEME_PREVIEW_FILE_SAVE" },
	};
	ThemeDlgData *dlg_data = (ThemeDlgData *)GetWindowLongPtr(hWnd, DWLP_USER);
	TTTSet *ts = dlg_data == NULL ? NULL : dlg_data->pts;

	switch (msg) {
	case WM_INITDIALOG: {
		dlg_data = (ThemeDlgData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
		ts = dlg_data->pts;
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);
		SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), dlg_data->pts->UILanguageFileW);

		EnableWindow(GetDlgItem(hWnd, IDC_FILE_SAVE_BUTTON), FALSE);
		return TRUE;
		break;
	}
	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lp;
		switch (nmhdr->code) {
		case PSN_APPLY: {
			// OK
#if 0
			TipWin2Destroy(dlg_data->tipwin);
			dlg_data->tipwin = NULL;
#endif
			break;
		}
		case PSN_HELP:
			OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupThemeEditor);
			break;
		default:
			break;
		}
		break;
	}
	case WM_COMMAND: {
		switch (wp) {
		case IDC_FILE_UNDO_BUTTON | (BN_CLICKED << 16): {
			// undo,元に戻す
			dlg_data->BGTab.bg_theme = dlg_data->Backup.bg_theme;
			dlg_data->ColorTab.color_theme = dlg_data->Backup.color_theme;
			goto set;
			break;
		}
		case IDC_FILE_PREVIEW_BUTTON | (BN_CLICKED << 16): {
			set:
			// preview
			ThemeSetBG(vt_src , & dlg_data->BGTab.bg_theme);
			ThemeSetColor(vt_src, &dlg_data->ColorTab.color_theme);
			BGSetupPrimary(vt_src, TRUE);
			InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
			break;
		}
		case IDC_FILE_LOAD_BUTTON | (BN_CLICKED << 16): {
			// load
			// テーマファイル読み込み
			OPENFILENAMEW ofn = {};
			wchar_t theme_file[MAX_PATH];

			if (ts->EtermLookfeel.BGThemeFileW != NULL) {
				wcscpy_s(theme_file, _countof(theme_file), ts->EtermLookfeel.BGThemeFileW);
			}
			else {
				theme_file[0] = 0;
			}

			ofn.lStructSize = get_OPENFILENAME_SIZEW();
			ofn.hwndOwner   = hWnd;
			ofn.lpstrFile   = theme_file;
			ofn.nMaxFile    = _countof(theme_file);
			ofn.nFilterIndex = 1;
			ofn.hInstance = dlg_data->hInst;
			ofn.lpstrDefExt = L"ini";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			ofn.lpstrTitle = L"select theme file";

			if (GetOpenFileNameW(&ofn)) {
				ThemeLoad(theme_file, &dlg_data->BGTab.bg_theme, &dlg_data->ColorTab.color_theme);

				static const TTMessageBoxInfoW info = {
					"Tera Term",
					NULL, L"Info",
					NULL, L"Preview?",
					MB_YESNO | MB_ICONWARNING
				};
				if (TTMessageBoxW(hWnd, &info, ts->UILanguageFileW) == IDYES) {
					ThemeSetColor(vt_src, &dlg_data->ColorTab.color_theme);
					ThemeSetBG(vt_src, &dlg_data->BGTab.bg_theme);

					BGSetupPrimary(vt_src, TRUE);
					InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
				}
			}

			break;
		}
		case IDC_FILE_SAVE_BUTTON | (BN_CLICKED << 16): {
			// save
			// テーマファイル書き出し
			wchar_t theme_file[MAX_PATH];
			OPENFILENAMEW ofn = {};

			theme_file[0] = 0;

			ofn.lStructSize = get_OPENFILENAME_SIZEW();
			ofn.hwndOwner   = hWnd;
			ofn.lpstrFile   = theme_file;
			ofn.nMaxFile    = _countof(theme_file);
			//ofn.lpstrFilter = "";
			ofn.nFilterIndex = 1;
			ofn.hInstance = dlg_data->hInst;
			ofn.lpstrDefExt = L"ini";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
			ofn.lpstrTitle = L"save theme file";

			if (GetSaveFileNameW(&ofn)) {
				LRESULT checked = SendDlgItemMessageA(hWnd, IDC_FILE_SAVE_BG_CHECK, BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) {
					ThemeSaveBG(&dlg_data->BGTab.bg_theme, theme_file);
				}
				checked = SendDlgItemMessageA(hWnd, IDC_FILE_SAVE_COLOR_CHECK, BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) {
					ThemeSaveColor(&dlg_data->ColorTab.color_theme, theme_file);
				}
			}
			break;
		}
		case IDC_FILE_SAVE_BG_CHECK:
		case IDC_FILE_SAVE_COLOR_CHECK: {
			LRESULT bg = SendDlgItemMessageA(hWnd, IDC_FILE_SAVE_BG_CHECK, BM_GETCHECK, 0, 0);
			LRESULT color = SendDlgItemMessageA(hWnd, IDC_FILE_SAVE_COLOR_CHECK, BM_GETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_FILE_SAVE_BUTTON),
						 ((bg & BST_CHECKED) || (color & BST_CHECKED)) ? TRUE : FALSE);
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		return FALSE;
	}
	return FALSE;
}

static UINT CALLBACK FileCallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	UINT ret_val = 0;
	(void)hwnd;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE:
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

static HPROPSHEETPAGE ThemeEditorFile(ThemeDlgData* dlg_data)
{
	const int id = IDD_TABSHEET_THEME_FILE;

	HINSTANCE inst = dlg_data->hInst;

	wchar_t *title;
	GetI18nStrWW("Tera Term", "DLG_THEME_PREVIEW_FILE_TITLE",
				 L"Preview/File", dlg_data->pts->UILanguageFileW, &title);

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = FileCallBack;
	psp.pszTitle = title;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if 1
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
#endif

	psp.pfnDlgProc = FileProc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(title);
	return hpsp;
}

//////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK BGAlphaProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_THEME_BG_ALPHA_TITLE"},
		{ IDC_TEXT_BACK_ALPHA_TITLE, "DLG_THEME_BG_ALPHA_TEXT_BACK_ALPHA_TITLE" },
		{ IDC_REVERSE_TEXT_BACK_ALPHA_TITLE, "DLG_THEME_BG_ALPHA_REVERSE_TEXT_BACK_ALPHA_TITLE" },
		{ IDC_OTHER_TEXT_BACK_ALPHA_TITLE, "DLG_THEME_BG_ALPHA_OTHER_BACK_TITLE" },
	};
	ThemeDlgData *dlg_data = (ThemeDlgData *)GetWindowLongPtr(hWnd, DWLP_USER);

	switch (msg) {
	case WM_INITDIALOG: {
		dlg_data = (ThemeDlgData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);
		SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), dlg_data->pts->UILanguageFileW);

		SendDlgItemMessageA(hWnd, IDC_REVERSE_TEXT_ALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
		SendDlgItemMessageA(hWnd, IDC_TEXT_ALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
		SendDlgItemMessageA(hWnd, IDC_BACK_ALPHA_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 255));

		break;
	}
	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lp;
		switch (nmhdr->code) {
		case PSN_APPLY: {
			break;
		}
		case PSN_HELP:
			OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupThemeEditor);
			break;
		case PSN_KILLACTIVE: {
			BGTheme *bg_theme = &dlg_data->BGTab.bg_theme;
			bg_theme->BGReverseTextAlpha = (BYTE)SendDlgItemMessageA(hWnd, IDC_REVERSE_TEXT_ALPHA_SLIDER, TBM_GETPOS, 0, 0);
			bg_theme->TextBackAlpha = (BYTE)SendDlgItemMessageA(hWnd, IDC_TEXT_ALPHA_SLIDER, TBM_GETPOS, 0, 0);
			bg_theme->BackAlpha = (BYTE)SendDlgItemMessageA(hWnd, IDC_BACK_ALPHA_SLIDER, TBM_GETPOS, 0, 0);
			break;
		}
		case PSN_SETACTIVE: {
			BGTheme *bg_theme = &dlg_data->BGTab.bg_theme;
			SendDlgItemMessageA(hWnd, IDC_REVERSE_TEXT_ALPHA_SLIDER, TBM_SETPOS, TRUE, bg_theme->BGReverseTextAlpha);
			SendDlgItemMessageA(hWnd, IDC_TEXT_ALPHA_SLIDER, TBM_SETPOS, TRUE, bg_theme->TextBackAlpha);
			SendDlgItemMessageA(hWnd, IDC_BACK_ALPHA_SLIDER, TBM_SETPOS, TRUE, bg_theme->BackAlpha);
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		return FALSE;
	}
	return FALSE;
}

static UINT CALLBACK BGAlphaCallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	(void)hwnd;
	UINT ret_val = 0;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE:
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

static HPROPSHEETPAGE BGAlphaCreate(ThemeDlgData *dlg_data)
{
	const int id = IDD_TABSHEET_BG_THEME_ALPHA_EDITOR;
	HINSTANCE inst = dlg_data->hInst;

	wchar_t *title;
	GetI18nStrWW("Tera Term", "DLG_THEME_BG_ALPHA_TITLE",
				 L"Background image alpha", dlg_data->pts->UILanguageFileW, &title);

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = BGAlphaCallBack;
	psp.pszTitle = title;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if 1
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
#endif

	psp.pfnDlgProc = BGAlphaProc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(title);
	return hpsp;
}

//////////////////////////////////////////////////////////////////////////////

// Theme dialog
class CThemeDlg: public TTCPropSheetDlg
{
public:
	CThemeDlg(HINSTANCE hInstance, HWND hParentWnd, ThemeDlgData *dlg_data):
		TTCPropSheetDlg(hInstance, hParentWnd, dlg_data->pts->UILanguageFileW)
	{
		HPROPSHEETPAGE page;
		page = ThemeEditorFile(dlg_data);
		AddPage(page);
		page = ThemeEditorCreate(dlg_data);
		AddPage(page);
		page = BGAlphaCreate(dlg_data);
		AddPage(page);
		page = ColorThemeEditorCreate(dlg_data);
		AddPage(page);

		SetCaption(L"Theme Editor");
	}
	~CThemeDlg() {
		;
	}

private:
	;
};

void ThemeDialog(HINSTANCE hInst, HWND hWnd, TComVar *pcv)
{
	ThemeDlgData *dlg_data = (ThemeDlgData*)calloc(1, sizeof(*dlg_data));
	dlg_data->hInst = hInst;
	dlg_data->pcv = pcv;
	dlg_data->pts = pcv->ts;
	dlg_data->hVTWin = pcv->HWin;
	ThemeGetBG(vt_src, &dlg_data->BGTab.bg_theme);
	dlg_data->Backup.bg_theme = dlg_data->BGTab.bg_theme;
	ThemeGetColor(vt_src, &dlg_data->ColorTab.color_theme);
	dlg_data->Backup.color_theme = dlg_data->ColorTab.color_theme;

	CThemeDlg dlg(hInst, hWnd, dlg_data);
	INT_PTR r = dlg.DoModal();
	if (r == 0) {
		// cancel時、バックアップ内容に戻す
		ThemeSetBG(vt_src, &dlg_data->Backup.bg_theme);
		ThemeSetColor(vt_src, &dlg_data->Backup.color_theme);
		BGSetupPrimary(vt_src, TRUE);
		InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
	}
	else if (r >= 1) {
		// okなど(Changes were saved by the user)
		ThemeSetBG(vt_src, &dlg_data->BGTab.bg_theme);
		ThemeSetColor(vt_src, &dlg_data->ColorTab.color_theme);
		BGSetupPrimary(vt_src, TRUE);
		InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
	}

	free(dlg_data);
}
