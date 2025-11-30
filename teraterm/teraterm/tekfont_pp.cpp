/*
 * (C) 2025- TeraTerm Project
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

/* tekfont property page */

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "dlglib.h"
#include "compat_win.h"
#include "helpid.h"
#include "tttext.h"
#include "codeconv.h"
#include "tslib.h"
#include "ttcommdlg.h"
#include "asprintf.h"
#include "win32helper.h"

#include "tekfont_pp.h"
#include "tekfont_pp_res.h"

struct UIPPData {
	TTTSet *pts;
	const wchar_t *UILanguageFileW;
	DLGTEMPLATE *dlg_templ;
	TComVar *pcv;
	LOGFONTW tek_font;
	HWND hVTWin;
	HINSTANCE hInst;
};

static UINT_PTR CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM /*wParam*/, LPARAM lParam)
{
	if (Message == WM_INITDIALOG) {
		UIPPData *dlg_data = (UIPPData *)(((CHOOSEFONTW *)lParam)->lCustData);
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

static BOOL ChooseDlgFont(HWND hWnd, UIPPData *dlg_data)
{
	// ダイアログ表示
	CHOOSEFONTW cf = {};
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &dlg_data->tek_font;
	cf.Flags =
		CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
		//CF_SHOWHELP |
		CF_NOVERTFONTS |
		CF_ENABLEHOOK;
	if (IsWindows7OrLater() && (IsDlgButtonChecked(hWnd, IDC_LIST_HIDDEN_FONTS_DLG) == BST_CHECKED)) {
		cf.Flags |= CF_INACTIVEFONTS;
	}
	if (IsDlgButtonChecked(hWnd, IDC_LIST_PRO_FONTS_DLG) != BST_CHECKED) {
		cf.Flags |= CF_FIXEDPITCHONLY;
	}
	cf.lpfnHook = TFontHook;
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = dlg_data->hInst;
	cf.lCustData = (LPARAM)dlg_data;
	BOOL result = ChooseFontW(&cf);
	return result;
}

static void ArrangeControls(HWND hWnd, UIPPData *data,ACFCF_MODE mode)
{
	ArrangeControlsForChooseFont(hWnd, &data->tek_font, IDC_LIST_HIDDEN_FONTS_DLG, IDC_LIST_PRO_FONTS_DLG, mode);
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_DLGFONT, "DLG_TAB_FONT_TEKFONT"},
		{ IDC_DLGFONT_CHOOSE, "DLG_TAB_FONT_BTN_SELECT" },
		{ IDC_LIST_PRO_FONTS_DLG, "DLG_TAB_FONT_LIST_PRO_FONTS_DLG" },
		{ IDC_LIST_HIDDEN_FONTS_DLG, "DLG_TAB_GENERAL_LIST_HIDDEN_FONTS" },
		{ IDC_FONT_FOLDER, "DLG_TAB_FONT_FOLDER" },
	};

	switch (msg) {
		case WM_INITDIALOG: {
			UIPPData *data = (UIPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			TTTSet *pts = data->pts;

			SetWindowLongPtrW(hWnd, DWLP_USER, (LONG_PTR)data);

			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), data->pts->UILanguageFileW);

			SetFontStringW(hWnd, IDC_DLGFONT_EDIT, &data->tek_font);

			ArrangeControls(hWnd, data, ACFCF_INIT_DIALOG);

			wchar_t *font_folder;
			HRESULT r = _SHGetKnownFolderPath(FOLDERID_Fonts, KF_FLAG_DEFAULT, NULL, &font_folder);
			if (r == S_OK) {
				wchar_t *text;
				hGetDlgItemTextW(hWnd, IDC_FONT_FOLDER, &text);
				wchar_t *new_text;
				aswprintf(&new_text, text, font_folder);
				TTTextMenu(hWnd, IDC_FONT_FOLDER, new_text, NULL, 0);
				free(text);
				free(new_text);
			}

			return TRUE;
		}
		case WM_DESTROY: {
			UIPPData *data = (UIPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);

			break;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					UIPPData *dlg_data = (UIPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);
					TTTSet *pts = dlg_data->pts;

					TSSetLogFont(hWnd, &dlg_data->tek_font, 1, 0, pts);

					break;
				}
				case PSN_HELP: {
					HWND vtwin = GetParent(hWnd);
					vtwin = GetParent(vtwin);
					PostMessageA(vtwin, WM_USER_DLGHELP2, HlpTEKSetupFont, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_COMMAND: {
			UIPPData *data = (UIPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);
			switch (wp) {
				case IDC_DLGFONT_CHOOSE | (BN_CLICKED << 16):
					if (ChooseDlgFont(hWnd, data) != FALSE) {
						SetFontStringW(hWnd, IDC_DLGFONT_EDIT, &data->tek_font);
						ArrangeControls(hWnd, data, ACFCF_CONTINUE);
					}
					break;

				case IDC_FONT_FOLDER:
					OpenFontFolder();
					break;

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
		ppsp->lParam = 0;
		break;
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE TEKFontPageCreate(HINSTANCE inst, HWND hWnd, TTTSet *pts)
{
	// 注 common/tt_res.h と ui_pp_res.h で値を一致させること
	int id = IDD_TABSHEET_TEKFONT;

	UIPPData *dlg_data = (UIPPData *)calloc(1, sizeof(UIPPData));
	dlg_data->UILanguageFileW = pts->UILanguageFileW;
	dlg_data->pts = pts;

	TSGetLogFont(hWnd, pts, 1, 0, &dlg_data->tek_font);

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t* UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_FONT_TEK",
		         L"Font(TEK)", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
	psp.dwFlags |= PSP_DLGINDIRECT;
	dlg_data->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
	psp.pResource = dlg_data->dlg_templ;

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)dlg_data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
