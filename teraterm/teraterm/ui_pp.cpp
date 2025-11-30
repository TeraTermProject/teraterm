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

/* ui property page */

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "dlglib.h"
#include "compat_win.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "tipwin2.h"
#include "tttext.h"
#include "tslib.h"
#include "ttcommdlg.h"

#include "ui_pp.h"
#include "ui_pp_res.h"

typedef struct {
	wchar_t* fullname;
	wchar_t* filename;
	wchar_t* language;
	wchar_t* date;
	wchar_t* contributor;
} LangInfo;

static const wchar_t* get_lang_folder()
{
	return (IsWindowsNTKernel()) ? L"lang_utf16le" : L"lang";
}

static void LangFree(LangInfo* infos, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++) {
		LangInfo* p = infos + i;
		free(p->filename);
		free(p->fullname);
		free(p->language);
		free(p->date);
		free(p->contributor);
	}
	free(infos);
}

/**
 *	ファイル名をリストする(ファイル名のみ)
 *	infosに追加してreturnする
 */
static LangInfo* LangAppendFileList(const wchar_t* folder, LangInfo* infos, size_t* infos_size)
{
	wchar_t* fullpath;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	size_t count = *infos_size;

	aswprintf(&fullpath, L"%s\\*.lng", folder);
	hFind = FindFirstFileW(fullpath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				LangInfo* p = (LangInfo*)realloc(infos, sizeof(LangInfo) * (count + 1));
				if (p != NULL) {
					infos = p;
					p = infos + count;
					count++;
					memset(p, 0, sizeof(*p));
					p->filename = _wcsdup(fd.cFileName);
					aswprintf(&p->fullname, L"%s\\%s", folder, fd.cFileName);
				}
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	free(fullpath);

	*infos_size = count;
	return infos;
}

/**
 *	lngファイルの Info セクションを読み込む
 */
static void LangRead(LangInfo* infos, size_t infos_size)
{
	size_t i;

	for (i = 0; i < infos_size; i++) {
		LangInfo* p = infos + i;
		const wchar_t* lng = p->fullname;
		wchar_t* s;
		hGetPrivateProfileStringW(L"Info", L"language", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->language = _wcsdup(p->filename);
		}
		else {
			p->language = s;
		}
		hGetPrivateProfileStringW(L"Info", L"date", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->date = _wcsdup(L"-");
		}
		else {
			p->date = s;
		}
		hGetPrivateProfileStringW(L"Info", L"contributor", NULL, lng, &s);
		if (s[0] == 0) {
			free(s);
			p->contributor = _wcsdup(L"-");
		}
		else {
			p->contributor = s;
		}
	}
}

static wchar_t *LangInfoText(const LangInfo *p)
{
	wchar_t *info;
	aswprintf(&info,
			  L"language\r\n"
			  L"  %s\r\n"
			  L"filename\r\n"
			  L"  %s\r\n"
			  L"date\r\n"
			  L"  %s\r\n"
			  L"contributor\r\n"
			  L"  %s",
			  p->language, p->filename, p->date, p->contributor);
	return info;
}

struct UIPPData {
	TTTSet *pts;
	const wchar_t *UILanguageFileW;
	DLGTEMPLATE *dlg_templ;
	TComVar *pcv;
	LangInfo* lng_infos;
	size_t lng_size;
	size_t selected_lang;	// 選ばれていたlngファイル番号
	TipWin2 *tipwin2;
	LOGFONTW DlgFont;
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
	cf.lpLogFont = &dlg_data->DlgFont;
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
	ArrangeControlsForChooseFont(hWnd, &data->DlgFont, IDC_LIST_HIDDEN_FONTS_DLG, IDC_LIST_PRO_FONTS_DLG, mode);
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_GENUILANG_LABEL, "DLG_GEN_LANG_UI" },
		{ IDC_DLGFONT, "DLG_TAB_FONT_DLGFONT"},
		{ IDC_DLGFONT_CHOOSE, "DLG_TAB_FONT_BTN_SELECT" },
		{ IDC_DLGFONT_DEFAULT, "DLG_TAB_FONT_BTN_DEFAULT" },
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

			// UI Language, 読み込み
			LangInfo* infos = NULL;
			size_t infos_size = 0;
			wchar_t* folder;
			aswprintf(&folder, L"%s\\%s", pts->ExeDirW, get_lang_folder());
			infos = LangAppendFileList(folder, infos, &infos_size);
			free(folder);
			if (wcscmp(pts->ExeDirW, pts->HomeDirW) != 0) {
				aswprintf(&folder, L"%s\\%s", pts->HomeDirW, get_lang_folder());
				infos = LangAppendFileList(folder, infos, &infos_size);
				free(folder);
			}
			LangRead(infos, infos_size);
			data->lng_infos = infos;
			data->lng_size = infos_size;

			// UI Language用 tipwin
			data->tipwin2 = TipWin2Create(data->hInst, hWnd);

			// UI Language, 選択
			data->selected_lang = 0;
			for (size_t i = 0; i < infos_size; i++) {
				const LangInfo* p = infos + i;
				SendDlgItemMessageW(hWnd, IDC_GENUILANG, CB_ADDSTRING, 0, (LPARAM)p->language);
				if (wcscmp(p->fullname, pts->UILanguageFileW) == 0) {
					data->selected_lang = i;
					wchar_t *info_text = LangInfoText(p);
					TipWin2SetTextW(data->tipwin2, IDC_GENUILANG, info_text);
					free(info_text);
				}
			}
			SendDlgItemMessageW(hWnd, IDC_GENUILANG, CB_SETCURSEL, data->selected_lang, 0);
			ExpandCBWidth(hWnd, IDC_GENUILANG);

			TSGetLogFont(GetParent(hWnd), pts, 2, 0, &data->DlgFont);
			SetFontStringW(hWnd, IDC_DLGFONT_EDIT, &data->DlgFont);

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

			LangFree(data->lng_infos, data->lng_size);
			data->lng_infos = NULL;
			data->lng_size = 0;

			TipWin2Destroy(data->tipwin2);
			data->tipwin2 = NULL;

			break;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					UIPPData *dlg_data = (UIPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);
					TTTSet *pts = dlg_data->pts;
					LRESULT w = SendDlgItemMessageA(hWnd, IDC_GENUILANG, CB_GETCURSEL, 0, 0);
					if (w != (LRESULT)dlg_data->selected_lang) {
						const LangInfo* p = dlg_data->lng_infos + w;
						free(pts->UILanguageFileW);
						pts->UILanguageFileW = _wcsdup(p->fullname);

						// タイトルの更新を行う。(2014.2.23 yutaka)
						PostMessage(dlg_data->hVTWin, WM_USER_CHANGETITLE, 0, 0);
					}

					TSSetLogFont(GetParent(hWnd), &dlg_data->DlgFont, 2, 0, pts);

					// TTXKanjiMenu は Language を見てメニューを表示するので、変更の可能性がある
					// OK 押下時にメニュー再描画のメッセージを飛ばすようにした。 (2007.7.14 maya)
					// 言語ファイルの変更時にメニューの再描画が必要 (2012.5.5 maya)
					PostMessage(dlg_data->hVTWin, WM_USER_CHANGEMENU, 0, 0);

					break;
				}
				case PSN_HELP: {
					HWND vtwin = GetParent(hWnd);
					vtwin = GetParent(vtwin);
					PostMessageA(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalUI, 0);
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
				case IDC_GENUILANG | (CBN_SELCHANGE << 16): {
					size_t ui_sel = (size_t)SendDlgItemMessageA(hWnd, IDC_GENUILANG, CB_GETCURSEL, 0, 0);
					wchar_t *info_text = LangInfoText(data->lng_infos + ui_sel);
					TipWin2SetTextW(data->tipwin2, IDC_GENUILANG, info_text);
					free(info_text);
					break;
				}
				case IDC_DLGFONT_CHOOSE | (BN_CLICKED << 16):
					if (ChooseDlgFont(hWnd, data) != FALSE) {
						SetFontStringW(hWnd, IDC_DLGFONT_EDIT, &data->DlgFont);
						ArrangeControls(hWnd, data, ACFCF_CONTINUE);
					}
					break;

				case IDC_DLGFONT_DEFAULT | (BN_CLICKED << 16):
					GetMessageboxFontW(&data->DlgFont);
					GetFontPitchAndFamily(hWnd, &data->DlgFont);
					SetFontStringW(hWnd, IDC_DLGFONT_EDIT, &data->DlgFont);
					ArrangeControls(hWnd, data, ACFCF_CONTINUE);
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

HPROPSHEETPAGE UIPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// 注 common/tt_res.h と ui_pp_res.h で値を一致させること
	int id = IDD_TABSHEET_UI;

	UIPPData *dlg_data = (UIPPData *)calloc(1, sizeof(UIPPData));
	dlg_data->UILanguageFileW = pts->UILanguageFileW;
	dlg_data->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t* UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_UI",
		         L"UI", pts->UILanguageFileW, &UIMsg);
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
