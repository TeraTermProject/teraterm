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
#include "inifile_com.h"
#include "helpid.h"
#include "vtdisp.h"
#include "tmfc.h"
#include "tmfc_propdlg.h"

#include "bg_theme.h"
#include "themedlg_res.h"
#include "themedlg.h"

typedef struct {
	// 共通
	HINSTANCE hInst;
	TComVar *pcv;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
	TipWin2 *tipwin;
	HWND hVTWin;
	// file tab
	struct {
		DLGTEMPLATE *dlg_templ;
	} FileTab;
	// bg theme tab
	BGTheme bg_theme;
	struct {
		DLGTEMPLATE *dlg_templ;
		TipWin2 *tipwin;
		BGTheme bg_theme;
	} BGTab;
	// color theme tab
	struct {
		TColorTheme color_theme;
	} color_tab;
	struct {
		BGTheme bg_theme;
		TColorTheme color_theme;
	} backup;
} ThemeDlgData;

static void SetWindowTextColor(HWND hWnd, COLORREF color)
{
	char str[32];
	sprintf(str, "%02x%02x%02x", GetRValue(color), GetGValue(color), GetBValue(color));
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
	sscanf(elem, "%x", &r);

	memcpy(elem, &str[2], 2);
	elem[2] = 0;
	g = 0;
	sscanf(elem, "%x", &g);

	memcpy(elem, &str[4], 2);
	elem[2] = 0;
	b = 0;
	sscanf(elem, "%x", &b);

	return RGB(r, g, b);
}

static COLORREF GetDlgItemTextColor(HWND hDlg, int ID)
{
	return GetWindowTextColor(GetDlgItem(hDlg, ID));
}

static void ResetControls(HWND hWnd, const BGTheme *bg_theme)
{
	SendDlgItemMessage(hWnd, IDC_BGIMG_CHECK, BM_SETCHECK, (bg_theme->BGDest.type == BG_PICTURE) ? TRUE : FALSE, 0);
	SetDlgItemTextA(hWnd, IDC_BGIMG_EDIT, bg_theme->BGDest.file);
	SetDlgItemTextColor(hWnd, IDC_BGIMG_COLOR_EDIT, bg_theme->BGDest.color);
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
		SendDlgItemMessage(hWnd, IDC_BGIMG_COMBO, CB_SETCURSEL, sel, 0);
	}

	SendDlgItemMessage(hWnd, IDC_WALLPAPER_CHECK, BM_SETCHECK, bg_theme->BGSrc1.alpha != 0, 0);
	SetDlgItemInt(hWnd, IDC_WALLPAPER_ALPHA_EDIT, bg_theme->BGSrc1.alpha, FALSE);

	SendDlgItemMessage(hWnd, IDC_SIMPLE_COLOR_PLANE_CHECK, BM_SETCHECK, bg_theme->BGSrc2.alpha != 0, 0);
	SetDlgItemInt(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA, bg_theme->BGSrc2.alpha, FALSE);
	SetDlgItemTextColor(hWnd, IDC_SIMPLE_COLOR_PLANE_COLOR, bg_theme->BGSrc2.color);
}

static void ReadFromDialog(HWND hWnd, BGTheme* bg_theme)
{
	LRESULT checked;
	LRESULT index;
	checked = SendDlgItemMessageA(hWnd, IDC_BGIMG_CHECK, BM_GETCHECK, 0, 0);
	bg_theme->BGDest.type = checked & BST_CHECKED ? BG_PICTURE : BG_NONE;
	GetDlgItemTextA(hWnd, IDC_BGIMG_EDIT, bg_theme->BGDest.file, sizeof(bg_theme->BGDest.file));
	bg_theme->BGDest.color = GetDlgItemTextColor(hWnd, IDC_BGIMG_COLOR_EDIT);
	index = SendDlgItemMessage(hWnd, IDC_BGIMG_COMBO, CB_GETCURSEL, 0, 0);
	bg_theme->BGDest.pattern = (BG_PATTERN)SendDlgItemMessage(hWnd, IDC_BGIMG_COMBO, CB_GETITEMDATA, index, 0);

	checked = SendDlgItemMessageA(hWnd, IDC_WALLPAPER_CHECK, BM_GETCHECK, 0, 0);
	if (checked & BST_CHECKED) {
		bg_theme->BGSrc1.type = BG_WALLPAPER;
		bg_theme->BGSrc1.alpha = GetDlgItemInt(hWnd, IDC_WALLPAPER_ALPHA_EDIT, NULL, FALSE);
	} else {
		bg_theme->BGSrc1.type = BG_NONE;
		bg_theme->BGSrc1.alpha = 0;
	}

	checked = SendDlgItemMessageA(hWnd, IDC_SIMPLE_COLOR_PLANE_CHECK, BM_GETCHECK, 0, 0);
	if (checked & BST_CHECKED) {
		bg_theme->BGSrc2.type = BG_COLOR;
		bg_theme->BGSrc2.alpha = GetDlgItemInt(hWnd, IDC_SIMPLE_COLOR_PLANE_ALPHA, NULL, FALSE);
	} else {
		bg_theme->BGSrc2.type = BG_NONE;
		bg_theme->BGSrc2.alpha = 0;
	}
	bg_theme->BGSrc2.color = GetDlgItemTextColor(hWnd, IDC_SIMPLE_COLOR_PLANE_COLOR);
}

static INT_PTR CALLBACK BGThemeProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	ThemeDlgData* dlg_data = (ThemeDlgData*)GetWindowLongPtr(hWnd, DWLP_USER);

	switch (msg) {
		case WM_INITDIALOG: {
			int i;
			dlg_data = (ThemeDlgData*)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);

			SetDlgItemTextW(hWnd, IDC_BG_THEME_HELP,
							L"Mix order:\n"
							L"  base\n"
							L"    v\n"
							L"  Background Image\n"
							L"    v\n"
							L"  wallpaper\n"
							L"    v\n"
							L"  simple color plane"
				);

			for (i = 0;; i++) {
				LRESULT index;
				const BG_PATTERN_ST *st = GetBGPatternList(i);
				if (st == NULL) {
					break;
				}
				index = SendDlgItemMessageA(hWnd, IDC_BGIMG_COMBO, CB_ADDSTRING, 0, (LPARAM)st->str);
				SendDlgItemMessageW(hWnd, IDC_BGIMG_COMBO, CB_SETITEMDATA, index, st->id);
			}

			ResetControls(hWnd, &dlg_data->bg_theme);
			return TRUE;
			break;
		}
		case WM_COMMAND: {
			switch (wp) {
			case IDC_BGIMG_BUTTON | (BN_CLICKED << 16): {
				// 画像ファイル選択
				OPENFILENAMEW ofn = {};
				wchar_t bg_file[MAX_PATH];
				wchar_t *bg_file_in;

				hGetDlgItemTextW(hWnd, IDC_BGIMG_EDIT, &bg_file_in);
				wcscpy_s(bg_file, _countof(bg_file), bg_file_in);
				free(bg_file_in);

				ofn.lStructSize = get_OPENFILENAME_SIZEW();
				ofn.hwndOwner   = hWnd;
				ofn.lpstrFile   = bg_file;
				ofn.nMaxFile    = _countof(bg_file);
				//ofn.lpstrFilter = "";
				ofn.nFilterIndex = 1;
				ofn.hInstance = dlg_data->hInst;
				ofn.lpstrDefExt = L"jpg";
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
				ofn.lpstrTitle = L"select bg image file";

				if (GetOpenFileNameW(&ofn)) {
					SetDlgItemTextW(hWnd, IDC_BGIMG_EDIT, bg_file);
				}
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
				OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupAdditionalTheme);
				break;
			case PSN_KILLACTIVE: {
				ReadFromDialog(hWnd, &dlg_data->bg_theme);
				break;
			}
			case PSN_SETACTIVE: {
				ResetControls(hWnd, &dlg_data->bg_theme);
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

static HPROPSHEETPAGE ThemeEditorCreate2(ThemeDlgData *dlg_data)
{
	const int id = IDD_TABSHEET_THEME_EDITOR;
	HINSTANCE inst = dlg_data->hInst;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = BGCallBack;
	psp.pszTitle = L"Background(BG)";		// TODO lng ファイルに入れる
	psp.pszTemplate = MAKEINTRESOURCEW(id);
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));

	psp.pfnDlgProc = BGThemeProc;
	psp.lParam = (LPARAM)dlg_data;

	return CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
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
	list[4].color = data->blink.fg;
	list[5].color = data->blink.bg;
	list[6].color = data->reverse.fg;
	list[7].color = data->reverse.bg;
	list[8].color = data->url.fg;
	list[9].color = data->url.bg;
	for (int i = 0; i < 16; i++) {
		list[10+i].color = data->ansicolor.color[i];
	}
}

static void RestoreColor(TColorTheme *data)
{
	data->vt.fg = list[0].color;
	data->vt.bg = list[1].color;
	data->bold.fg = list[2].color;
	data->bold.bg = list[3].color;
	data->blink.fg = list[4].color;
	data->blink.bg = list[5].color;
	data->reverse.fg = list[6].color;
	data->reverse.bg = list[7].color;
	data->url.fg = list[8].color;
	data->url.bg = list[9].color;
	for (int i = 0; i < 16; i++) {
		data->ansicolor.color[i] = list[10+i].color;
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
	swprintf_s(color_str, L"0x%02x%02x%02x", r,g,b);
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
		{0, "DLG_GEN_TITLE"},
	};
	ThemeDlgData *dlg_data = (ThemeDlgData *)GetWindowLongPtr(hWnd, DWLP_USER);

	switch (msg) {
	case WM_INITDIALOG: {
		dlg_data = (ThemeDlgData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);

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

			lvcol.pszText = (LPSTR)"value(RRGGBB)";
			lvcol.iSubItem = 2;
			//ListView_InsertColumn(hWndList, 1, &lvcol);
			SendMessage(hWndList, LVM_INSERTCOLUMNA, 2, (LPARAM)&lvcol);

			lvcol.pszText = (LPSTR)"value(R, G, B)";
			lvcol.iSubItem = 3;
			//ListView_InsertColumn(hWndList, 1, &lvcol);
			SendMessage(hWndList, LVM_INSERTCOLUMNA, 3, (LPARAM)&lvcol);
		}

		SetColor(&dlg_data->color_tab.color_theme);
		SetColorListCtrl(hWnd);
		break;
	}
	case WM_COMMAND: {
		switch (wp) {
		case IDC_COLOR_DEFAULT_BUTTON | (BN_CLICKED << 16): {
			// デフォルト
			BGGetColorDefault(&dlg_data->color_tab.color_theme);
			SetColor(&dlg_data->color_tab.color_theme);
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
				int j = nmlist->iSubItem;
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
			OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupAdditionalTheme);
			break;
		case PSN_KILLACTIVE: {
			RestoreColor(&dlg_data->color_tab.color_theme);
			break;
		}
		case PSN_SETACTIVE: {
			SetColor(&dlg_data->color_tab.color_theme);
			SetColorListCtrl(hWnd);
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

static HPROPSHEETPAGE ColorThemeEditorCreate2(ThemeDlgData *dlg_data)
{
	const int id = IDD_TABSHEET_COLOR_THEME_EDITOR;
	HINSTANCE inst = dlg_data->hInst;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = ColorCallBack;
	psp.pszTitle = L"color";		// TODO lng ファイルに入れる
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if 1
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));
#endif

	psp.pfnDlgProc = ColorThemeProc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	return hpsp;
}

//////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK FileProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{0, "DLG_GEN_TITLE"},
	};
	ThemeDlgData *dlg_data = (ThemeDlgData *)GetWindowLongPtr(hWnd, DWLP_USER);
	TTTSet *ts = dlg_data == NULL ? NULL : dlg_data->pts;

	switch (msg) {
	case WM_INITDIALOG: {
		dlg_data = (ThemeDlgData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
		ts = dlg_data->pts;
		SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);

		dlg_data->tipwin = TipWin2Create(NULL, hWnd);
#if 0
		TipWin2SetTextW(dlg_data->tipwin, IDC_BUTTON1,
						L"テーマフィアルを読み込む\n"
			);
		TipWin2SetTextW(dlg_data->tipwin, IDC_BUTTON3,
						L"現在のダイアログの状態を設定してテーマファイルに書き込む\n"
			);
		TipWin2SetTextW(dlg_data->tipwin, IDC_BUTTON4,
						L"現在のダイアログの状態を設定する\n"
						L"このページの設定は書き出さないと失われる\n"
			);
#endif
		EnableWindow(GetDlgItem(hWnd, IDC_FILE_SAVE_BUTTON), FALSE);
		return TRUE;
		break;
	}
	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lp;
		switch (nmhdr->code) {
		case PSN_APPLY: {
			// OK
			TipWin2Destroy(dlg_data->tipwin);
			dlg_data->tipwin = NULL;
			break;
		}
		case PSN_HELP:
			OpenHelpCV(dlg_data->pcv, HH_HELP_CONTEXT, HlpMenuSetupAdditionalTheme);
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
			dlg_data->bg_theme = dlg_data->backup.bg_theme;
			dlg_data->color_tab.color_theme = dlg_data->backup.color_theme;
			goto set;
			break;
		}
		case IDC_FILE_PREVIEW_BUTTON | (BN_CLICKED << 16): {
			set:
			// preview
			BGSet(&dlg_data->bg_theme);
			BGSetColorData(&dlg_data->color_tab.color_theme);
			BGSetupPrimary(TRUE);
			InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
			break;
		}
		case IDC_FILE_LOAD_BUTTON | (BN_CLICKED << 16): {
			// load
			// テーマファイル読み込み
			OPENFILENAMEW ofn = {};
			wchar_t theme_file[MAX_PATH];

			wcscpy_s(theme_file, _countof(theme_file), ts->EtermLookfeel.BGThemeFileW);

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
				BGLoad(theme_file, &dlg_data->bg_theme, &dlg_data->color_tab.color_theme);

				static const TTMessageBoxInfoW info = {
					"Tera Term",
					NULL, L"Info",
					NULL, L"Preview?",
					MB_YESNO | MB_ICONWARNING
				};
				if (TTMessageBoxW(hWnd, &info, ts->UILanguageFileW) == IDYES) {
					BGSetColorData(&dlg_data->color_tab.color_theme);
					BGSet(&dlg_data->bg_theme);
					//SetColor(&dlg_data->color_tab.color_theme);

					BGSetupPrimary(TRUE);
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
					BGSave(&dlg_data->bg_theme, theme_file);
				}
				checked = SendDlgItemMessageA(hWnd, IDC_FILE_SAVE_COLOR_CHECK, BM_GETCHECK, 0, 0);
				if (checked & BST_CHECKED) {
					BGSaveColor(&dlg_data->color_tab.color_theme, theme_file);
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

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = FileCallBack;
	psp.pszTitle = L"preview/file";		// TODO lng ファイルに入れる
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if 1
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));
#endif

	psp.pfnDlgProc = FileProc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
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
		page = ThemeEditorCreate2(dlg_data);
		AddPage(page);
		page = ColorThemeEditorCreate2(dlg_data);
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
	ThemeDlgData *dlg_data = (ThemeDlgData*)calloc(sizeof(*dlg_data), 1);
	dlg_data->hInst = hInst;
	dlg_data->pcv = pcv;
	dlg_data->pts = pcv->ts;
	dlg_data->hVTWin = pcv->HWin;
	BGGet(&dlg_data->bg_theme);
	dlg_data->backup.bg_theme = dlg_data->bg_theme;
	GetColorData(&dlg_data->color_tab.color_theme);
	dlg_data->backup.color_theme = dlg_data->color_tab.color_theme;

	CThemeDlg dlg(hInst, hWnd, dlg_data);
	INT_PTR r = dlg.DoModal();
	if (r == 0) {
		// cancel時、バックアップ内容に戻す
		BGSet(&dlg_data->backup.bg_theme);
		BGSetColorData(&dlg_data->color_tab.color_theme);
		BGSetupPrimary(TRUE);
		InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
	}
	else if (r >= 1) {
		// okなど(Changes were saved by the user)
		BGSet(&dlg_data->bg_theme);
		BGSetColorData(&dlg_data->color_tab.color_theme);
		BGSetupPrimary(TRUE);
		InvalidateRect(dlg_data->hVTWin, NULL, FALSE);
	}

	free(dlg_data);
}
