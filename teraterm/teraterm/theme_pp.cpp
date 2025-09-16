/*
 * Copyright (C) 2024- TeraTerm Project
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

#include "theme_pp.h"

#include <stdio.h>
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <shlobj.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttwinman.h"	// for cv
#include "dlglib.h"
#include "ttlib.h"
#include "helpid.h"
#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "themedlg.h"
#include "theme.h"
#include "tt_res.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

struct ThemePPData {
	HINSTANCE hInst;
	const wchar_t *UILanguageFileW;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
};

static void ArrangeControls(HWND hWnd)
{
	static const int controls[] = {
		IDC_CHECK_FAST_SIZE_MOVE,
		IDC_THEME_FILE,
		IDC_THEME_EDIT,
		IDC_THEME_BUTTON,
		IDC_SPIPATH_EDIT,
		IDC_SPIPATH_BUTTON,
	};

	const BOOL enable = IsDlgButtonChecked(hWnd, IDC_THEME_ENABLE) == BST_CHECKED ? TRUE : FALSE;
	for (int i = 0 ; i < _countof(controls); i++) {
		int id = controls[i];
		EnableWindow(GetDlgItem(hWnd, id), enable);
	}
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_CHECK_FAST_SIZE_MOVE, "DLG_TAB_VISUAL_FAST_SIZE_MOVE" },
	};
	ThemePPData *dlg_data = (ThemePPData *)GetWindowLongPtr(hWnd, DWLP_USER);
	TTTSet *ts = dlg_data == NULL ? NULL : dlg_data->pts;

	switch (msg) {
		case WM_INITDIALOG: {
			dlg_data = (ThemePPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			ts = dlg_data->pts;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);
			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), dlg_data->pts->UILanguageFileW);

			const static I18nTextInfo theme_select[] = {
				{ "DLG_TAB_VISUAL_THEME_STARTUP_NO_USE", L"no use" },
				{ "DLG_TAB_VISUAL_THEME_STARTUP_FIXED_THEME", L"fixed theme file" },
				{ "DLG_TAB_VISUAL_THEME_STARTUP_RANDOM_THEME", L"random theme file" },
			};

			CheckDlgButton(hWnd, IDC_THEME_ENABLE, ThemeIsEnabled() ? BST_CHECKED : BST_UNCHECKED);

			int sel = ts->EtermLookfeel.BGEnable;
			if (sel < 0) sel = 0;
			if (sel > 2) sel = 2;
			SetI18nListW("Tera Term", hWnd, IDC_THEME_FILE, theme_select, _countof(theme_select),
						 ts->UILanguageFileW, sel);
			BOOL enable = (sel == 1) ? TRUE : FALSE;
			EnableDlgItem(hWnd, IDC_THEME_EDIT, enable);
			EnableDlgItem(hWnd, IDC_THEME_BUTTON, enable);

			SetDlgItemTextW(hWnd, IDC_THEME_EDIT, ts->EtermLookfeel.BGThemeFileW);
			CheckDlgButton(hWnd, IDC_CHECK_FAST_SIZE_MOVE, ts->EtermLookfeel.BGFastSizeMove != 0);
			SetDlgItemTextW(hWnd, IDC_SPIPATH_EDIT, ts->EtermLookfeel.BGSPIPathW);

			ArrangeControls(hWnd);
			break;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					int r = GetCurSel(hWnd, IDC_THEME_FILE);
					switch (r) {
					default:
						assert(FALSE);
						// fall through
					case 1:
						ts->EtermLookfeel.BGEnable = 0;
						break;
					case 2: {
						// テーマファイル指定
						ts->EtermLookfeel.BGEnable = 1;

						wchar_t* theme_file;
						hGetDlgItemTextW(hWnd, IDC_THEME_EDIT, &theme_file);

						if (ts->EtermLookfeel.BGThemeFileW != NULL) {
							free(ts->EtermLookfeel.BGThemeFileW);
						}
						ts->EtermLookfeel.BGThemeFileW = theme_file;
						break;
					}
					case 3: {
						// ランダムテーマ
						ts->EtermLookfeel.BGEnable = 2;
						if (ts->EtermLookfeel.BGThemeFileW != NULL) {
							free(ts->EtermLookfeel.BGThemeFileW);
						}
						ts->EtermLookfeel.BGThemeFileW = NULL;
						break;
					}
					}

					if ((IsDlgButtonChecked(hWnd, IDC_THEME_ENABLE) == BST_CHECKED) && ThemeIsEnabled() == FALSE) {
						// テーマをenableにする
						ThemeEnable(TRUE);
					}
					else if ((IsDlgButtonChecked(hWnd, IDC_THEME_ENABLE) == BST_UNCHECKED) &&
							 ThemeIsEnabled() == TRUE) {
						// テーマをdisableにする
						ThemeEnable(FALSE);
					}

					ts->EtermLookfeel.BGFastSizeMove =
						IsDlgButtonChecked(hWnd, IDC_CHECK_FAST_SIZE_MOVE) == BST_CHECKED;

					wchar_t *spi_path;
					hGetDlgItemTextW(hWnd, IDC_SPIPATH_EDIT, &spi_path);
					free(ts->EtermLookfeel.BGSPIPathW);
					ts->EtermLookfeel.BGSPIPathW = spi_path;

					break;
				}
				case PSN_HELP: {
					HWND vtwin = GetParent(hWnd);
					vtwin = GetParent(vtwin);
					PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalTheme, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_COMMAND: {
			switch (wp) {
			case IDC_THEME_ENABLE | (BN_CLICKED << 16): {
				ArrangeControls(hWnd);
				break;
			}
			case IDC_THEME_EDITOR_BUTTON | (BN_CLICKED << 16): {
				ThemeDialog(dlg_data->hInst, hWnd, &cv);
				break;
			}
			case IDC_THEME_FILE | (CBN_SELCHANGE << 16): {
				int r = GetCurSel(hWnd, IDC_THEME_FILE);
				// 固定のとき、ファイル名を入力できるようにする
				BOOL enable = (r == 2) ? TRUE : FALSE;
				EnableWindow(GetDlgItem(hWnd, IDC_THEME_EDIT), enable);
				EnableWindow(GetDlgItem(hWnd, IDC_THEME_BUTTON), enable);
				break;
			}
			case IDC_THEME_BUTTON | (BN_CLICKED << 16): {
				// テーマファイルを選択する
				wchar_t *theme_file;
				hGetDlgItemTextW(hWnd, IDC_THEME_EDIT, &theme_file);

				wchar_t *theme_dir;
				aswprintf(&theme_dir, L"%s\\theme", ts->HomeDirW);

				TTOPENFILENAMEW ofn = {};
				ofn.hwndOwner = hWnd;
				ofn.lpstrFilter = L"Theme Files(*.ini)\0*.ini\0All Files(*.*)\0*.*\0";
				ofn.lpstrTitle = L"select theme file";
				ofn.lpstrFile = theme_file;
				ofn.lpstrInitialDir = theme_dir;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				wchar_t *filename;
				BOOL ok = TTGetOpenFileNameW(&ofn, &filename);
				if (ok) {
					SetDlgItemTextW(hWnd, IDC_THEME_EDIT, filename);
					free(filename);
				}
				free(theme_dir);
				free(theme_file);
				return TRUE;
			}
			case IDC_SPIPATH_BUTTON | (BN_CLICKED << 16): {
				wchar_t *def;
				hGetDlgItemTextW(hWnd, IDC_SPIPATH_EDIT, &def);
				if (GetFileAttributesW(def) == INVALID_FILE_ATTRIBUTES) {
					// フォルダが存在しない(入力途中?,TT4から移行?)
					static const TTMessageBoxInfoW info = {
						"Tera Term",
						"MSG_TT_NOTICE", L"Tera Term: Notice",
						NULL, L"'%s' not exist\nUse home folder",
						MB_OK };
					TTMessageBoxW(hWnd, &info, ts->UILanguageFileW, def);
					free(def);
					def = _wcsdup(ts->HomeDirW);
				}

				TTBROWSEINFOW bi = {};
				bi.hwndOwner = hWnd;
				bi.lpszTitle = L"Select Susie Plugin path";
				bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_NEWDIALOGSTYLE;

				wchar_t *folder;
				if (TTSHBrowseForFolderW(&bi, def, &folder)) {
					SetDlgItemTextW(hWnd, IDC_SPIPATH_EDIT, folder);
					free(folder);
				}
				free(def);
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

HPROPSHEETPAGE ThemePageCreate(HINSTANCE inst, TTTSet *pts)
{
	const int id = IDD_TABSHEET_THEME;

	ThemePPData *Param = (ThemePPData *)calloc(1, sizeof(ThemePPData));
	Param->hInst = inst;
	Param->UILanguageFileW = pts->UILanguageFileW;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t* UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_THEME",
		         L"Theme", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
