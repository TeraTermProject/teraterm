/*
 * (C) 2020- TeraTerm Project
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

/* font property page */

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "font_pp_res.h"
#include "dlglib.h"
#include "setting.h"
#include "vtdisp.h"		// for DispSetupFontDlg()
#include "buffer.h"
#include "compat_win.h"	// for CF_INACTIVEFONTS
#include "helpid.h"
#include "codeconv.h"

#include "font_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

struct FontPPData {
	HINSTANCE hInst;
	const wchar_t *UILanguageFileW;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
//	LOGFONTA VTFont;
	LOGFONTW DlgFont;
};

static void GetDlgLogFont(HWND hWnd, const TTTSet *ts, LOGFONTW *logfont)
{
	memset(logfont, 0, sizeof(*logfont));
	if (ts->DialogFontNameW == NULL || ts->DialogFontNameW[0] == 0) {
		// フォントが設定されていなかったらOSのフォントを使用する
		GetMessageboxFontW(logfont);
	}
	else {
		wcsncpy_s(logfont->lfFaceName, _countof(logfont->lfFaceName), ts->DialogFontNameW,  _TRUNCATE);
		logfont->lfHeight = -GetFontPixelFromPoint(hWnd, ts->DialogFontPoint);
		logfont->lfCharSet = ts->DialogFontCharSet;
		logfont->lfWeight = FW_NORMAL;
		logfont->lfOutPrecision = OUT_DEFAULT_PRECIS;
		logfont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		logfont->lfQuality = DEFAULT_QUALITY;
		logfont->lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
	}
}

static void SetDlgLogFont(HWND hWnd, const LOGFONTW *logfont, TTTSet *ts)
{
	wcsncpy_s(ts->DialogFontNameW, _countof(ts->DialogFontNameW), logfont->lfFaceName, _TRUNCATE);
	ts->DialogFontPoint = GetFontPointFromPixel(hWnd, -logfont->lfHeight);
	ts->DialogFontCharSet = logfont->lfCharSet;
}

static UINT_PTR CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_INITDIALOG) {
		FontPPData *dlg_data = (FontPPData *)(((CHOOSEFONTA *)lParam)->lCustData);
		wchar_t *uimsg;
		static const wchar_t def[] = L"\"Font style\" selection here won't affect actual font appearance.";
		GetI18nStrWW("Tera Term", "DLG_CHOOSEFONT_STC6", def, dlg_data->UILanguageFileW, &uimsg);
		SetDlgItemTextW(Dialog, stc6, uimsg);
		free(uimsg);

		SetFocus(GetDlgItem(Dialog,cmb1));

		CenterWindow(Dialog, GetParent(Dialog));
	}
	return FALSE;
}

static BOOL ChooseDlgFont(HWND hWnd, FontPPData *dlg_data)
{
	const TTTSet *ts = dlg_data->pts;

	// ダイアログ表示
	CHOOSEFONTW cf = {};
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &dlg_data->DlgFont;
	cf.Flags =
		CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
		CF_SHOWHELP | CF_NOVERTFONTS |
		CF_ENABLEHOOK;
	if (IsWindows7OrLater() && ts->ListHiddenFonts) {
		cf.Flags |= CF_INACTIVEFONTS;
	}
	cf.lpfnHook = &TFontHook;
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = dlg_data->hInst;
	cf.lCustData = (LPARAM)dlg_data;
	BOOL result = ChooseFontW(&cf);
	return result;
}

static void EnableCodePage(HWND hWnd, BOOL enable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_VTFONT_CODEPAGE_LABEL), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_VTFONT_CODEPAGE_EDIT), enable);
}

static void SetFontString(HWND hWnd, int item, const LOGFONTW *logfont)
{
	// https://docs.microsoft.com/en-us/windows/win32/api/dimm/ns-dimm-logfonta
	// http://www.coara.or.jp/~tkuri/D/015.htm#D2002-09-14
	wchar_t b[128];
	swprintf_s(b, L"%s (%d,%d) %d", logfont->lfFaceName, logfont->lfWidth, logfont->lfHeight, logfont->lfCharSet);
	SetDlgItemTextW(hWnd, item, b);
}

static void SetVTFontString(HWND hWnd, int item, const TTTSet *ts)
{
	LOGFONTW logfont = {};
	logfont.lfWidth = ts->VTFontSize.x;
	logfont.lfHeight = ts->VTFontSize.y;
	ACPToWideChar_t(ts->VTFont, logfont.lfFaceName, _countof(logfont.lfFaceName));
	logfont.lfCharSet = ts->VTFontCharSet;
	SetFontString(hWnd, item, &logfont);
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_VTWINFONT, "DLG_TAB_FONT_VTWINFONT" },
		{ IDC_VTFONT_CHOOSE, "DLG_TAB_FONT_BTN_SELECT" },
		{ IDC_VTFONT_TITLE, "DLG_TAB_FONT_VTFONT_TITLE" },
		{ IDC_VTFONT_CODEPAGE_LABEL, "DLG_TAB_FONT_CODEPAGE_LABEL" },
		{ IDC_FONT_QUALITY_LABEL, "DLG_TAB_VISUAL_FONT_QUALITY" },
		{ IDC_DLGFONT, "DLG_TAB_FONT_DLGFONT"},
		{ IDC_DLGFONT_CHOOSE, "DLG_TAB_FONT_BTN_SELECT" },
		{ IDC_DLGFONT_DEFAULT, "DLG_TAB_FONT_BTN_DEFAULT" },
		{ IDC_LIST_HIDDEN_FONTS, "DLG_TAB_GENERAL_LIST_HIDDEN_FONTS" },
		{ IDC_LIST_PRO_FONTS, "DLG_TAB_FONT_LIST_PRO_FONTS" },
		{ IDC_CHARACTER_SPACE_TITLE, "DLG_TAB_FONT_CHARACTER_SPACE" },
	};
	FontPPData *dlg_data = (FontPPData *)GetWindowLongPtr(hWnd, DWLP_USER);
	TTTSet *ts = dlg_data == NULL ? NULL : dlg_data->pts;

	switch (msg) {
		case WM_INITDIALOG: {
			dlg_data = (FontPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			ts = dlg_data->pts;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);
			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), dlg_data->pts->UILanguageFileW);

			GetDlgLogFont(GetParent(hWnd), ts, &dlg_data->DlgFont);

			SetVTFontString(hWnd, IDC_VTFONT_EDIT, ts);

			CheckDlgButton(hWnd,
						   UnicodeDebugParam.UseUnicodeApi ? IDC_VTFONT_UNICODE : IDC_VTFONT_ANSI,
						   BST_CHECKED);
			SetDlgItemInt(hWnd, IDC_VTFONT_CODEPAGE_EDIT, UnicodeDebugParam.CodePageForANSIDraw, FALSE);
			EnableCodePage(hWnd, UnicodeDebugParam.UseUnicodeApi ? FALSE : TRUE);

			CheckDlgButton(hWnd, IDC_LIST_HIDDEN_FONTS, ts->ListHiddenFonts);
			EnableWindow(GetDlgItem(hWnd, IDC_LIST_HIDDEN_FONTS), IsWindows7OrLater() ? TRUE : FALSE);

			SetDlgItemInt(hWnd, IDC_SPACE_RIGHT, ts->FontDW, FALSE);
			SetDlgItemInt(hWnd, IDC_SPACE_LEFT, ts->FontDH, FALSE);
			SetDlgItemInt(hWnd, IDC_SPACE_TOP, ts->FontDX, FALSE);
			SetDlgItemInt(hWnd, IDC_SPACE_BOTTOM, ts->FontDY, FALSE);

			const static I18nTextInfo visual_font_quality[] = {
				{ "DLG_TAB_VISUAL_FONT_QUALITY_DEFAULT", L"Default" },
				{ "DLG_TAB_VISUAL_FONT_QUALITY_NONANTIALIASED", L"Non-Antialiased" },
				{ "DLG_TAB_VISUAL_FONT_QUALITY_ANTIALIASED", L"Antialiased" },
				{ "DLG_TAB_VISUAL_FONT_QUALITY_CLEARTYPE", L"ClearType" },
			};
			SetI18nListW("Tera Term", hWnd, IDC_FONT_QUALITY, visual_font_quality, _countof(visual_font_quality),
						 ts->UILanguageFileW, 0);
			int cur =
				ts->FontQuality == DEFAULT_QUALITY ? 0 :
				ts->FontQuality == NONANTIALIASED_QUALITY ? 1 :
				ts->FontQuality == ANTIALIASED_QUALITY ? 2 :
				/*ts->FontQuality == CLEARTYPE_QUALITY ? */ 3;
			SendDlgItemMessage(hWnd, IDC_FONT_QUALITY, CB_SETCURSEL, cur, 0);

			SetFontString(hWnd, IDC_DLGFONT_EDIT, &dlg_data->DlgFont);

			break;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					UnicodeDebugParam.UseUnicodeApi =
						IsDlgButtonChecked(hWnd, IDC_VTFONT_UNICODE) == BST_CHECKED;
					UnicodeDebugParam.CodePageForANSIDraw =
						GetDlgItemInt(hWnd, IDC_VTFONT_CODEPAGE_EDIT, NULL, FALSE);
					// ANSI表示用のコードページを設定する
					BuffSetDispCodePage(UnicodeDebugParam.CodePageForANSIDraw);
					ts->ListHiddenFonts = IsDlgButtonChecked(hWnd, IDC_LIST_HIDDEN_FONTS) == BST_CHECKED;

					SetDlgLogFont(GetParent(hWnd), &dlg_data->DlgFont, ts);

					// font quality
					int cur = (int)SendDlgItemMessageA(hWnd, IDC_FONT_QUALITY, CB_GETCURSEL, 0, 0);
					ts->FontQuality =
						cur == 0 ? DEFAULT_QUALITY :
						cur == 1 ? NONANTIALIASED_QUALITY :
						cur == 2 ? ANTIALIASED_QUALITY :
						CLEARTYPE_QUALITY;

					break;
				}
				case PSN_HELP: {
					HWND vtwin = GetParent(hWnd);
					vtwin = GetParent(vtwin);
					PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalFont, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_COMMAND: {
			switch (wp) {
			case IDC_VTFONT_ANSI | (BN_CLICKED << 16):
			case IDC_VTFONT_UNICODE | (BN_CLICKED << 16): {
				BOOL enable = (wp & 0xffff) == IDC_VTFONT_ANSI ? TRUE : FALSE;
				EnableCodePage(hWnd, enable);
				break;
			}
			case IDC_VTFONT_CHOOSE | (BN_CLICKED << 16): {
				DispSetupFontDlg(hWnd);
				SetDlgItemInt(hWnd, IDC_VTFONT_CODEPAGE_EDIT, UnicodeDebugParam.CodePageForANSIDraw, FALSE);
				SetVTFontString(hWnd, IDC_VTFONT_EDIT, ts);
				break;
			}
			case IDC_DLGFONT_CHOOSE | (BN_CLICKED << 16):
				if (ChooseDlgFont(hWnd, dlg_data) != FALSE) {
					SetFontString(hWnd, IDC_DLGFONT_EDIT, &dlg_data->DlgFont);
				}
				break;

			case IDC_DLGFONT_DEFAULT | (BN_CLICKED << 16): {
				GetMessageboxFontW(&dlg_data->DlgFont);
				SetFontString(hWnd, IDC_DLGFONT_EDIT, &dlg_data->DlgFont);
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

static UINT CALLBACK CallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
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
		free((void *)ppsp->lParam);
		ppsp->lParam = NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE FontPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// 注 common/tt_res.h と font_pp_res.h で値を一致させること
	const int id = IDD_TABSHEET_FONT;

	FontPPData *Param = (FontPPData *)calloc(sizeof(FontPPData), 1);
	Param->hInst = inst;
	Param->UILanguageFileW = pts->UILanguageFileW;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_FONT",
				 L"Font", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
