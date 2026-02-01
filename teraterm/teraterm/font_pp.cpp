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
#include "vtdisp.h"		// for DispIsResizedFont()
#include "compat_win.h"	// for CF_INACTIVEFONTS
#include "helpid.h"
#include "codeconv.h"
#include "ttdlg.h"	// _ChooseFontDlg()
#include "tipwin2.h"
#include "asprintf.h"
#include "win32helper.h"
#include "tttext.h"
#include "tslib.h"
#include "vtdraw.h"
#include "ttcommdlg.h"

#include "font_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

#define IDC_SPACE_MAXLEN 6 	  // IDC_SPACE(FontDX, FontDW, FontDY, FontDH)の最大桁数
#define IDC_CODEPAGE_MAXLEN 5 // IDC_CODEPAGE(CodePageForANSIDraw)の最大桁数

struct FontPPData {
	HINSTANCE hInst;
	const wchar_t *UILanguageFileW;
	TTTSet *pts;
	DLGTEMPLATE *dlg_templ;
	LOGFONTW VTFont;
	TipWin2 *Tipwin;
};

static void EnableCodePage(HWND hWnd, BOOL enable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_VTFONT_CODEPAGE_LABEL), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_VTFONT_CODEPAGE_EDIT), enable);
}

/**
 *	フォントのCharSet(LOGFONT.charlfCharSet)から
 *	表示に妥当なCodePageを得る
 */
static int GetCodePageFromFontCharSet(BYTE char_set)
{
	static const struct {
		BYTE CharSet;	// LOGFONT.lfCharSet
		int CodePage;
	} table[] = {
		{ SHIFTJIS_CHARSET,  	932 },
		{ HANGUL_CHARSET,		949 },
		{ GB2312_CHARSET,	 	936 },
		{ CHINESEBIG5_CHARSET,	950 },
		{ RUSSIAN_CHARSET,		1251 },
	};
	int i;
	for (i = 0; i < _countof(table); i++) {
		if (table[i].CharSet == char_set) {
			return table[i].CodePage;
		}
	}
	return CP_ACP;
}

static UINT_PTR CALLBACK TVTFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) {
		case WM_INITDIALOG:
		{
			const PTTSet ts = (PTTSet)((CHOOSEFONTA *)lParam)->lCustData;
			wchar_t *uimsg;

			//EnableWindow(GetDlgItem(Dialog, cmb2), FALSE);

			GetI18nStrWW("Tera Term", "DLG_CHOOSEFONT_STC6", L"\"Font style\" selection here won't affect actual font appearance.",
						 ts->UILanguageFileW, &uimsg);
			SetDlgItemTextW(Dialog, stc6, uimsg);
			free(uimsg);

			SetFocus(GetDlgItem(Dialog,cmb1));

			CenterWindow(Dialog, GetParent(Dialog));

			break;
		}
	}
	return FALSE;
}

static BOOL ChooseVTFont(HWND WndParent, FontPPData *dlg_data)
{
	const TTTSet *ts = dlg_data->pts;
	BOOL Ok;
	LOGFONTW VTFontW = dlg_data->VTFont;
	CHOOSEFONTW cf = {};
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = WndParent;
	cf.lpLogFont = &VTFontW;
	cf.Flags =
		CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
		//CF_SHOWHELP |
		CF_NOVERTFONTS |
		CF_ENABLEHOOK;
	if (IsDlgButtonChecked(WndParent, IDC_LIST_HIDDEN_FONTS) == BST_CHECKED) {
		cf.Flags |= CF_INACTIVEFONTS;
	}
	if (IsDlgButtonChecked(WndParent, IDC_LIST_PRO_FONTS_VT) != BST_CHECKED) {
		cf.Flags |= CF_FIXEDPITCHONLY;
	}
	cf.lpfnHook = TVTFontHook;
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = dlg_data->hInst;
	cf.lCustData = (LPARAM)ts;
	Ok = ChooseFontW(&cf);
	if (Ok) {
		dlg_data->VTFont = VTFontW;
	}
	return Ok;
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_VTWINFONT, "DLG_TAB_FONT_VTWINFONT" },
		{ IDC_VTFONT_CHOOSE, "DLG_TAB_FONT_BTN_SELECT" },
		{ IDC_VTFONT_TITLE, "DLG_TAB_FONT_VTFONT_TITLE" },
		{ IDC_VTFONT_CODEPAGE_LABEL, "DLG_TAB_FONT_CODEPAGE_LABEL" },
		{ IDC_LIST_HIDDEN_FONTS, "DLG_TAB_GENERAL_LIST_HIDDEN_FONTS" },
		{ IDC_LIST_PRO_FONTS_VT, "DLG_TAB_FONT_LIST_PRO_FONTS_VT" },
		{ IDC_CHARACTER_SPACE_TITLE, "DLG_TAB_FONT_CHARACTER_SPACE" },
		{ IDC_RESIZED_FONT, "DLG_TAB_FONT_RESIZED_FONT" },
		{ IDC_FONT_FOLDER, "DLG_TAB_FONT_FOLDER" },
	};
	FontPPData *dlg_data = (FontPPData *)GetWindowLongPtr(hWnd, DWLP_USER);
	TTTSet *ts = dlg_data == NULL ? NULL : dlg_data->pts;

	switch (msg) {
		case WM_INITDIALOG: {
			dlg_data = (FontPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			ts = dlg_data->pts;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)dlg_data);
			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), dlg_data->pts->UILanguageFileW);

			TSGetLogFont(hWnd, ts, 0, 0, &dlg_data->VTFont);

			SetFontStringW(hWnd, IDC_VTFONT_EDIT, &dlg_data->VTFont);

			{
				static const wchar_t *unicode = L"Unicode API";
				static const wchar_t *ansi = L"ANSI API";
				wchar_t *s;
				aswprintf(&s, L"Auto (%s)",
						  VTDrawFromID(IdVtDrawAPIAuto) == IdVtDrawAPIUnicode ? unicode : ansi);
				SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_ADDSTRING, 0, (LPARAM)s);
				free(s);
				SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_ADDSTRING, 0, (LPARAM)unicode);
				SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_ADDSTRING, 0, (LPARAM)ansi);
				if (IsWindowsNTKernel() == TRUE) {
					SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_SETCURSEL,
										ts->VTDrawAPI_ini == IdVtDrawAPIAuto ? 0 :
										ts->VTDrawAPI_ini == IdVtDrawAPIUnicode ? 1 : 2, 0);
				} else {
					SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_SETCURSEL, 0, 0);
					EnableWindow(GetDlgItem(hWnd, IDC_VTFONT_COMBO), FALSE);
				}

				wchar_t *fmt;
				hGetDlgItemTextW(hWnd, IDC_VTFONT_CODEPAGE_LABEL, &fmt);
				aswprintf(&s, fmt, GetACP());
				free(fmt);
				SetDlgItemTextW(hWnd, IDC_VTFONT_CODEPAGE_LABEL, s);
				free(s);

				SetDlgItemInt(hWnd, IDC_VTFONT_CODEPAGE_EDIT, ts->VTDrawAnsiCodePage_ini, FALSE);
				EnableCodePage(hWnd, ts->VTDrawAPI == IdVtDrawAPIANSI ? TRUE : FALSE);
				SendDlgItemMessageA(hWnd, IDC_VTFONT_CODEPAGE_EDIT, EM_LIMITTEXT, IDC_CODEPAGE_MAXLEN, 0);
			}

			ArrangeControlsForChooseFont(hWnd, &dlg_data->VTFont, IDC_LIST_HIDDEN_FONTS, IDC_LIST_PRO_FONTS_VT, ACFCF_INIT_VTWIN);

			SetDlgItemInt(hWnd, IDC_SPACE_LEFT, ts->FontDX, TRUE);
			SetDlgItemInt(hWnd, IDC_SPACE_RIGHT, ts->FontDW - ts->FontDX, TRUE);
			SetDlgItemInt(hWnd, IDC_SPACE_TOP, ts->FontDY, TRUE);
			SetDlgItemInt(hWnd, IDC_SPACE_BOTTOM, ts->FontDH - ts->FontDY, TRUE);
			SendDlgItemMessage(hWnd, IDC_SPACE_LEFT,   EM_LIMITTEXT, IDC_SPACE_MAXLEN, 0);
			SendDlgItemMessage(hWnd, IDC_SPACE_RIGHT,  EM_LIMITTEXT, IDC_SPACE_MAXLEN, 0);
			SendDlgItemMessage(hWnd, IDC_SPACE_TOP,    EM_LIMITTEXT, IDC_SPACE_MAXLEN, 0);
			SendDlgItemMessage(hWnd, IDC_SPACE_BOTTOM, EM_LIMITTEXT, IDC_SPACE_MAXLEN, 0);
			dlg_data->Tipwin = TipWin2Create(dlg_data->hInst, hWnd);

			CheckDlgButton(hWnd, IDC_RESIZED_FONT, DispIsResizedFont());

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

			break;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					LRESULT r = SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_GETCURSEL, 0, 0);
					ts->VTDrawAPI_ini = (IdVtDrawAPI)r;
					ts->VTDrawAPI = VTDrawFromID((IdVtDrawAPI)r);
					ts->VTDrawAnsiCodePage_ini =
						GetDlgItemInt(hWnd, IDC_VTFONT_CODEPAGE_EDIT, NULL, FALSE);
					ts->VTDrawAnsiCodePage =
						ts->VTDrawAnsiCodePage_ini == 0 ? GetACP() : ts->VTDrawAnsiCodePage_ini;

					TSSetLogFont(hWnd, &dlg_data->VTFont, 0, 0, ts);

					DispEnableResizedFont(IsDlgButtonChecked(hWnd, IDC_RESIZED_FONT) == BST_CHECKED);

					BOOL parsed;
					int dlgx, dlgw, dlgy, dlgh;
					dlgx = GetDlgItemInt(hWnd, IDC_SPACE_LEFT, &parsed, TRUE);
					if (parsed) {
						dlgw = GetDlgItemInt(hWnd, IDC_SPACE_RIGHT, &parsed, TRUE);
						if (parsed) {
							dlgy = GetDlgItemInt(hWnd, IDC_SPACE_TOP, &parsed, TRUE);
							if (parsed) {
								dlgh = GetDlgItemInt(hWnd, IDC_SPACE_BOTTOM, &parsed, TRUE);
								if (parsed) {
									if (ts->FontDX != dlgx ||
									    ts->FontDW != (dlgw + dlgx) ||
									    ts->FontDY != dlgy ||
									    ts->FontDH != (dlgh + dlgy)) {
										ts->FontDX = dlgx;
										ts->FontDW = dlgw + dlgx;
										ts->FontDY = dlgy;
										ts->FontDH = dlgh + dlgy;
										ChangeFont(vt_src, 0);
										DispChangeWinSize(vt_src, ts->TerminalWidth, ts->TerminalHeight);
									}
								}
							}
						}
					}

					// フォントの設定
					ChangeFont(vt_src, 0);
					HWND vtwin = GetParent(GetParent(hWnd));
					if (::IsZoomed(vtwin)) {
						// 最大化されている場合は、サイズ変更により WM_SIZE を発生させる。
						RECT r;
						::GetWindowRect(vtwin, &r);
						SetWindowPos(vtwin, NULL, 0, 0, r.right - r.left, 0, SWP_NOMOVE);
						SetWindowPos(vtwin, NULL, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE);
					} else {
						DispChangeWinSize(vt_src, WinWidth, WinHeight);
					}
					ChangeCaret(vt_src);

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
			if (HIWORD(wp) == EN_UPDATE) {
				int id = LOWORD(wp);
				if (id == IDC_SPACE_LEFT ||
					id == IDC_SPACE_RIGHT ||
					id == IDC_SPACE_TOP ||
					id == IDC_SPACE_BOTTOM) {
					BOOL parsed;
					GetDlgItemInt(hWnd, id, &parsed, TRUE); // parseできたかチェックする。戻り値は使用しない。
					if (! parsed) {
						HWND hEdit = (HWND)lp;
						if (GetWindowTextLengthW(hEdit) == 0) {
							SetWindowTextW(hEdit, L"0");
							break;
						}
						// ツールチップを表示
						wchar_t *uiTitle, *uiText;
						GetI18nStrWW("Tera Term", "MSG_TOOLTIP_EDITERR_TITLE1", L"Unacceptable Character", ts->UILanguageFileW, &uiTitle);
						GetI18nStrWW("Tera Term", "MSG_TOOLTIP_EDITERR_TEXT1", L"Only positive and negative integers are acceptable.", ts->UILanguageFileW, &uiText);
						TipWin2ShowEdittextErrMsgW(dlg_data->Tipwin, hEdit, id, TTI_ERROR, uiTitle, uiText);
						free(uiTitle);
						free(uiText);

						// 無効な文字を削除
						wchar_t text[IDC_SPACE_MAXLEN + 1];
						GetWindowTextW(hEdit, &text[0], IDC_SPACE_MAXLEN + 1);
						wchar_t *orgp = text, *newp = text;
						while (*orgp) {
							if (newp == text) {
								if (wcsncmp(orgp, L"-", 1) == 0 || iswdigit(*orgp)) {
									*newp++ = *orgp;
								}
							} else if (iswdigit(*orgp)) {
								*newp++ = *orgp;
							}
							orgp++;
						}
						*newp = '\0';
						if (wcscmp(text, L"-") == 0) {
							text[0] = L'0';
							text[1] = '\0';
						}

						// EDITコントロールを更新
						DWORD startPos, endPos;
						SendMessage(hEdit, EM_GETSEL, (WPARAM)(&startPos), (LPARAM)(&endPos));
						SetWindowTextW(hEdit, text);
						startPos--;
						SendMessage(hEdit, EM_SETSEL, startPos, startPos);
					}
				}
				break;
			}

			switch (wp) {
			case IDC_VTFONT_COMBO | (CBN_SELCHANGE << 16): {
				IdVtDrawAPI r = (IdVtDrawAPI)SendDlgItemMessageW(hWnd, IDC_VTFONT_COMBO, CB_GETCURSEL, 0, 0);
				r = VTDrawFromID(r);
				BOOL enable = r == IdVtDrawAPIANSI ? TRUE : FALSE;
				EnableCodePage(hWnd, enable);
				break;
			}
			case IDC_VTFONT_CHOOSE | (BN_CLICKED << 16): {
				if (ChooseVTFont(hWnd, dlg_data)) {
					const int codepage = GetCodePageFromFontCharSet(dlg_data->VTFont.lfCharSet);
					SetDlgItemInt(hWnd, IDC_VTFONT_CODEPAGE_EDIT, codepage, FALSE);

					ArrangeControlsForChooseFont(hWnd, &dlg_data->VTFont, IDC_LIST_HIDDEN_FONTS, IDC_LIST_PRO_FONTS_VT, ACFCF_CONTINUE);
					SetFontStringW(hWnd, IDC_VTFONT_EDIT, &dlg_data->VTFont);
				}
				break;
			}

			case IDC_FONT_FOLDER:
				OpenFontFolder();
				break;

			default:
				break;
			}
			break;
		}

		case WM_DESTROY:
			TipWin2Destroy(dlg_data->Tipwin);
			dlg_data->Tipwin = NULL;
			break;

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

HPROPSHEETPAGE FontPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// 注 common/tt_res.h と font_pp_res.h で値を一致させること
	const int id = IDD_TABSHEET_FONT;

	FontPPData *Param = (FontPPData *)calloc(1, sizeof(FontPPData));
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
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
