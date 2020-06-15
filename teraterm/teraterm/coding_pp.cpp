/*
 * (C) 2020 TeraTerm Project
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

/* coding property page */

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "coding_pp_res.h"
#include "dlglib.h"
#include "compat_win.h"
#include "setting.h"
#include "layer_for_unicode.h"
#include "helpid.h"

#include "coding_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

static const char *KanjiInList[] = {"^[$@","^[$B",NULL};
static const char *KanjiOutList[] = {"^[(B","^[(J",NULL};
static const char *KanjiOutList2[] = {"^[(B","^[(J","^[(H",NULL};
static const char *CellWidthList[] = { "1 Cell", "2 Cell", NULL };

static const struct {
	int lang;
	int coding;
	const char *CodeName;
} KanjiList[] = {
	{ IdUtf8,		IdUTF8,				"UTF-8" },
	{ IdEnglish,	IdCodeEnglish,		"English" },
	{ IdJapanese,	IdSJIS,				"Japanese/SJIS (CP932)" },
	{ IdJapanese,	IdEUC,				"Japanese/EUC" },
	{ IdJapanese,	IdJIS,				"Japanese/JIS" },
	{ IdJapanese,	IdUTF8,				"Japanese/UTF-8" },
	{ IdRussian,	IdWindows,			"Russian/Windows (CP1251)" },
	{ IdRussian,	IdKOI8,				"Russian/KOI8-R" },
	{ IdRussian,	Id866,				"Russian/CP 866" },
	{ IdRussian,	IdISO,				"Russian/ISO 8859-5" },
	{ IdKorean,		IdKoreanCP51949,	"Korean/KS5601 (CP51949)" },
	{ IdKorean,		IdUTF8,				"Korean/UTF-8" },
	{ IdChinese,	IdCnGB2312,			"China/GB2312 (CP936)" },
	{ IdChinese,	IdCnBig5,			"China/Big5 (CP950)" },
	{ IdChinese,	IdUTF8,				"China/UTF-8" },
};

struct CodingPPData {
	TTTSet *pts;
	const char *UILanguageFile;
	DLGTEMPLATE *dlg_templ;
};

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{0, "DLG_GEN_TITLE"},
		{IDC_GENLANGLABEL, "DLG_GEN_LANG"},
		//		{ IDC_TERMKANJILABEL, "DLG_TERM_KANJI" },
		{IDC_TERMKANJILABEL, "DLG_TERMK_KANJI"},
		{IDC_TERMKANA, "DLG_TERM_KANA"},
		{IDC_TERMKANJISENDLABEL, "DLG_TERMK_KANJISEND"},
		//		{ IDC_TERMKANASEND, "DLG_TERM_KANASEND" },
		{IDC_TERMKANASEND, "DLG_TERM_KANASEND"},
		{IDC_TERMKINTEXT, "DLG_TERM_KIN"},
		{IDC_TERMKOUTTEXT, "DLG_TERM_KOUT"},
	};
	CodingPPData *DlgData = (CodingPPData *)GetWindowLongPtr(hWnd, DWLP_USER);

	switch (msg) {
		case WM_INITDIALOG: {
			DlgData = (CodingPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			const TTTSet *ts = DlgData->pts;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)DlgData);
			SetDlgTexts(hWnd, TextInfos, _countof(TextInfos), DlgData->pts->UILanguageFile);

			int recv_index = 0;
			int send_index = 0;
			for (int i = 0; i < _countof(KanjiList); i++) {
				int id = KanjiList[i].lang * 100 + KanjiList[i].coding;
				int index =
					(int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_ADDSTRING, 0, (LPARAM)KanjiList[i].CodeName);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETITEMDATA, index, id);
				if (ts->Language == KanjiList[i].lang && ts->KanjiCode == KanjiList[i].coding) {
					recv_index = i;
				}

				index =
					(int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_ADDSTRING, 0, (LPARAM)KanjiList[i].CodeName);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETITEMDATA, index, id);
				if (ts->Language == KanjiList[i].lang && ts->KanjiCodeSend == KanjiList[i].coding) {
					send_index = i;
				}
			}
			ExpandCBWidth(hWnd, IDC_TERMKANJI);
			ExpandCBWidth(hWnd, IDC_TERMKANJISEND);
			SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETCURSEL, recv_index, 0);
			SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, send_index, 0);

			if (recv_index == send_index) {
				CheckDlgButton(hWnd, IDC_USE_SAME_CODE, BST_CHECKED);
				EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), FALSE);
			}

			if (ts->KanjiCode != IdJIS) {
				DisableDlgItem(hWnd, IDC_TERMKANA, IDC_TERMKANA);
			}
			if (ts->KanjiCodeSend != IdJIS) {
				DisableDlgItem(hWnd, IDC_TERMKANASEND, IDC_TERMKOUT);
			}

			SetRB(hWnd, ts->JIS7Katakana, IDC_TERMKANA, IDC_TERMKANA);
			SetRB(hWnd, ts->JIS7KatakanaSend, IDC_TERMKANASEND, IDC_TERMKANASEND);

			SetDropDownList(hWnd, IDC_TERMKIN, KanjiInList, ts->KanjiIn);
			if ((ts->TermFlag & TF_ALLOWWRONGSEQUENCE) != 0) {
				SetDropDownList(hWnd, IDC_TERMKOUT, KanjiOutList2, ts->KanjiOut);
			}
			else {
				SetDropDownList(hWnd, IDC_TERMKOUT, KanjiOutList, ts->KanjiOut);
			}

			// characters as wide
			SetDropDownList(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CellWidthList, ts->UnicodeAmbiguousWidth == 1 ? 1 : 2);
			CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, ts->UnicodeEmojiOverride ? BST_CHECKED : BST_UNCHECKED);
			SetDropDownList(hWnd, IDC_EMOJI_WIDTH_COMBO, CellWidthList, ts->UnicodeEmojiWidth == 1 ? 1 : 2);
			EnableWindow(GetDlgItem(hWnd, IDC_EMOJI_WIDTH_COMBO), ts->UnicodeEmojiOverride);

			return TRUE;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					TTTSet *ts = DlgData->pts;

					ts->JIS7KatakanaSend = 0;
					ts->JIS7Katakana = 0;
					ts->KanjiIn = 0;
					ts->KanjiOut = 0;
					ts->RussHost = 0;
					ts->RussClient = 0;
					ts->RussFont = 0;

					// 受信コード
					int curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					int id = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);
					int lang = id / 100;
					int coding = id % 100;
					ts->Language = lang;
					ts->KanjiCode = coding;
					if (coding == IdUTF8 && (lang == IdJapanese || lang == IdKorean || lang == IdChinese || lang == IdUtf8)) {
						;
					}
					else if (lang == IdJapanese && (coding == IdSJIS || coding == IdEUC || coding == IdJIS)) {
						if (coding == IdJIS) {
							ts->JIS7Katakana = (IsDlgButtonChecked(hWnd, IDC_TERMKANA) == BST_CHECKED);
							ts->JIS7KatakanaSend = (IsDlgButtonChecked(hWnd, IDC_TERMKANASEND) == BST_CHECKED);
							WORD w = (WORD)GetCurSel(hWnd, IDC_TERMKIN);
							if (w > 0) {
								ts->KanjiIn = w;
							}
							w = (WORD)GetCurSel(hWnd, IDC_TERMKOUT);
							if (w > 0) {
								ts->KanjiOut = w;
							}
						}
					}
					else if (lang == IdRussian &&
							 (coding == IdWindows || coding == IdKOI8 || coding == Id866 || coding == IdISO)) {
						ts->RussHost = coding;
					}
					else if (lang == IdKorean && coding == IdKoreanCP51949) {
						;
					}
					else if (lang == IdChinese && (coding == IdCnGB2312 || coding == IdCnBig5)) {
						;
					}
					else if (lang == IdEnglish && coding == IdCodeEnglish) {
						;
					}
					else {
						assert(FALSE);
					}

					// 送信コード
					curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					id = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);
					coding = id % 100;
					ts->KanjiCodeSend = coding;

					// characters as wide
					ts->UnicodeAmbiguousWidth = (BYTE)GetCurSel(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO);
					ts->UnicodeEmojiOverride = (BYTE)IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK);
					ts->UnicodeEmojiWidth = (BYTE)GetCurSel(hWnd, IDC_EMOJI_WIDTH_COMBO);

					break;
				}
				case PSN_HELP: {
					HWND vtwin = GetParent(hWnd);
					vtwin = GetParent(vtwin);
					PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalCoding, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_COMMAND: {
			switch (wp) {
				case IDC_TERMKANJI | (CBN_SELCHANGE << 16): {
					// 受信コード
					int curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					int id = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);
					if (id == IdJapanese * 100 + IdJIS) {
						EnableDlgItem(hWnd, IDC_TERMKANA, IDC_TERMKANA);
					}
					else {
						DisableDlgItem(hWnd, IDC_TERMKANA, IDC_TERMKANA);
					}
					if ((id / 100) == IdChinese || (id / 100) == IdJapanese || (id / 100) == IdKorean) {
						// CJK
						SendDlgItemMessage(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CB_SETCURSEL, 1, 0);
						CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_CHECKED);
						EnableWindow(GetDlgItem(hWnd, IDC_EMOJI_WIDTH_COMBO), TRUE);
						SendDlgItemMessage(hWnd, IDC_EMOJI_WIDTH_COMBO, CB_SETCURSEL, 1, 0);
					}
					else {
						CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_UNCHECKED);
						SendDlgItemMessage(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CB_SETCURSEL, 0, 0);
						EnableWindow(GetDlgItem(hWnd, IDC_EMOJI_WIDTH_COMBO), FALSE);
					}

					if (IsDlgButtonChecked(hWnd, IDC_USE_SAME_CODE) == BST_CHECKED) {
						// 送信コードを同じ値にする
						SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, curPos, 0);
						goto kanji_send_selchange;
					}
					break;
				}
				case IDC_TERMKANJISEND | (CBN_SELCHANGE << 16): {
				kanji_send_selchange:
					// 送信コード
					int curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					int id = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);
					if (id == IdJapanese * 100 + IdJIS) {
						EnableDlgItem(hWnd, IDC_TERMKANASEND, IDC_TERMKOUT);
					}
					else {
						DisableDlgItem(hWnd, IDC_TERMKANASEND, IDC_TERMKOUT);
					}
					break;
				}
				case IDC_USE_SAME_CODE | (BN_CLICKED << 16): {
					BOOL enable = (IsDlgButtonChecked(hWnd, IDC_USE_SAME_CODE) == BST_CHECKED) ? FALSE : TRUE;
					EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), enable);
					break;
				}
				case IDC_EMOJI_WIDTH_CHECK | (BN_CLICKED << 16): {
					BOOL enable = (IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_EMOJI_WIDTH_COMBO), enable);
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_DESTROY: {
			free(DlgData->dlg_templ);
			free(DlgData);
			SetWindowLongPtr(hWnd, DWLP_USER, NULL);
			break;
		}
		default:
			return FALSE;
	}
	return FALSE;
}

HPROPSHEETPAGE CodingPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// 注 common/tt_res.h と coding_pp_res.h で値を一致させること
	int id = IDD_TABSHEET_CODING;

	CodingPPData *Param = (CodingPPData *)calloc(sizeof(CodingPPData), 1);
	Param->UILanguageFile = pts->UILanguageFile;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = inst;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));
	psp.pResource = Param->dlg_templ;
#endif
	psp.pszTitle = L"coding";		// TODO lng ファイルに入れる
	psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = _CreatePropertySheetPageW(&psp);
	return hpsp;
}
