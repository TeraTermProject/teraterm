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

/* coding property page */

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "tttypes_charset.h"
#include "coding_pp_res.h"
#include "dlglib.h"
#include "compat_win.h"
#include "setting.h"
#include "helpid.h"
#include "ttlib_charset.h"
#include "makeoutputstring.h"
#include "unicode.h"
#include "win32helper.h"
#include "inifile_com.h"

#include "coding_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

static const char *CellWidthList[] = { "1 Cell", "2 Cell", NULL };

struct CodingPPData {
	TTTSet *pts;
	const wchar_t *UILanguageFileW;
	DLGTEMPLATE *dlg_templ;
	OverrideCharWidthInfo unicode_override_charwidth_info;
};

static void EnableWindows(HWND hWnd, const int *list, int count, BOOL enable)
{
	int i;
	for (i = 0; i < count; i++) {
		HWND w = GetDlgItem(hWnd, list[i]);
		EnableWindow(w, enable);
	}
}

static void ArrenageItems(HWND hWnd, CodingPPData *data)
{
	static const int JJISReceiveItems[] = {
		IDC_TERMKINTEXT,
		IDC_TERMKIN,
		IDC_TERMKOUTTEXT,
		IDC_TERMKOUT,
		IDC_TERMKANA,
		IDC_JIS_RECEIVE_TITLE,
	};
	static const int JJISSendItems[] = {
		IDC_JIS_TRANSMIT_TITLE,
		IDC_TERMKANASEND,
	};
	static const int UnicodeItems[] = {
		IDC_AMBIGUOUS_WIDTH_TITLE,
		IDC_AMBIGUOUS_WIDTH_COMBO,
		IDC_EMOJI_WIDTH_CHECK,
		IDC_EMOJI_WIDTH_COMBO,
		IDC_OVERRIDE_CHAR_WIDTH,
		IDC_OVERRIDE_CHAR_WIDTH_COMBO,
	};

	// 受信コード
	LRESULT curPos = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
	IdKanjiCode coding_receive = (IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);

	// 送信コード
	curPos = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
	IdKanjiCode coding_send = (IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);

	bool is_unicode = ((coding_receive == IdUTF8) || (coding_send == IdUTF8));

	// Unicode character width
	if (is_unicode) {
		EnableWindows(hWnd, UnicodeItems, _countof(UnicodeItems), TRUE);
		const OverrideCharWidthInfo *info = &data->unicode_override_charwidth_info;
		if (info->count == 0) {
			EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO), FALSE);
		}
	}
	else {
		EnableWindows(hWnd, UnicodeItems, _countof(UnicodeItems), FALSE);
	}

	if (coding_receive == IdJIS) {
		EnableWindows(hWnd, JJISReceiveItems, _countof(JJISReceiveItems), TRUE);
	}
	else {
		EnableWindows(hWnd, JJISReceiveItems, _countof(JJISReceiveItems), FALSE);
	}

	if (coding_send == IdJIS) {
		EnableWindows(hWnd, JJISSendItems, _countof(JJISSendItems), TRUE);
	}
	else {
		EnableWindows(hWnd, JJISSendItems, _countof(JJISSendItems), FALSE);
	}
}

static INT_PTR CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_GEN_TITLE" },
		// Unicode
		//   override char width
		{ IDC_OVERRIDE_CHAR_WIDTH, "DLG_ENCODING_CHAR_WIDTH_PER_CHAR" },
		// Japanese JIS
		// { IDC_TERMKANJILABEL, "DLG_TERM_KANJI" },
		{ IDC_TERMKANJILABEL, "DLG_TERMK_KANJI" },
		//{ IDC_TERMKANA, "DLG_TERM_KANA" },
		{ IDC_TERMKANJISENDLABEL, "DLG_TERMK_KANJISEND" },
		//{ IDC_TERMKANASEND, "DLG_TERM_KANASEND" },
		{ IDC_TERMKINTEXT, "DLG_TERM_KIN" },
		{ IDC_TERMKOUTTEXT, "DLG_TERM_KOUT" },
		// DEC Special Graphics
		{ IDC_DECSP_UNI2DEC, "DLG_CODING_DECSP_UNICODE_TO_DEC" },
		{ IDC_DECSP_DEC2UNI, "DLG_CODING_DECSP_DEC_TO_UNICODE" },
		{ IDC_DECSP_DO_NOT, "DLG_CODING_DECSP_DO_NOT_MAP" },
		{ IDC_DEC2UNICODE_BOXDRAWING, "DLG_CODING_UNICODE_TO_DEC_BOXDRAWING" },
		{ IDC_DEC2UNICODE_PUNCTUATION, "DLG_CODING_UNICODE_TO_DEC_PUNCTUATION" },
		{ IDC_DEC2UNICODE_MIDDLEDOT, "DLG_CODING_UNICODE_TO_DEC_MIDDLEDOT" },
	};

	switch (msg) {
		case WM_INITDIALOG: {
			CodingPPData *DlgData = (CodingPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			const TTTSet *ts = DlgData->pts;
			SetWindowLongPtrW(hWnd, DWLP_USER, (LONG_PTR)DlgData);
			SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), DlgData->pts->UILanguageFileW);

			int recv_index = 0;
			int send_index = 0;
			int i = 0;
			while(1) {
				const TKanjiList *p = GetKanjiList(i++);
				if (p == NULL) {
					break;
				}
				if (p->CodeStrGUI == NULL) {
					continue;
				}
				LRESULT index = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_ADDSTRING, 0, (LPARAM)p->CodeStrGUI);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETITEMDATA, index, (LPARAM)p->coding);
				if (ts->KanjiCode == p->coding) {
					recv_index = (int)index;
				}

				index = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_ADDSTRING, 0, (LPARAM)p->CodeStrGUI);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETITEMDATA, index, (LPARAM)p->coding);
				if (ts->KanjiCodeSend == p->coding) {
					send_index = (int)index;
				}
			}
			ExpandCBWidth(hWnd, IDC_TERMKANJI);
			ExpandCBWidth(hWnd, IDC_TERMKANJISEND);
			SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_SETCURSEL, recv_index, 0);
			SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, send_index, 0);

			if (recv_index == send_index) {
				CheckDlgButton(hWnd, IDC_USE_DIFFERENT_CODE, BST_UNCHECKED);
				EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), FALSE);
			}
			else {
				CheckDlgButton(hWnd, IDC_USE_DIFFERENT_CODE, BST_CHECKED);
			}

			SetRB(hWnd, ts->JIS7Katakana, IDC_TERMKANA, IDC_TERMKANA);
			SetRB(hWnd, ts->JIS7KatakanaSend, IDC_TERMKANASEND, IDC_TERMKANASEND);

			// Kanji In
			int kanji_in_index = 0;
			for (int i = 0;; i++) {
				const KanjiInOutSt *p = GetKanjiInList(i);
				if (p == NULL) {
					break;
				}
				LRESULT index = SendDlgItemMessageA(hWnd, IDC_TERMKIN, CB_ADDSTRING, 0, (LPARAM)p->menu_str);
				SendDlgItemMessageA(hWnd, IDC_TERMKIN, CB_SETITEMDATA, index, (LPARAM)p->code);
				if (ts->KanjiIn == p->code) {
					kanji_in_index = i;
				}
			}
			ExpandCBWidth(hWnd, IDC_TERMKIN);
			SendDlgItemMessageA(hWnd, IDC_TERMKIN, CB_SETCURSEL, kanji_in_index, 0);

			// Kanji Out
			int kanji_out_index = 0;
			for (int i = 0;; i++) {
				const KanjiInOutSt *p = GetKanjiOutList(i);
				if (p == NULL) {
					break;
				}
				if (p->regular == 0 && ((ts->TermFlag & TF_ALLOWWRONGSEQUENCE) == 0)) {
					continue;
				}
				LRESULT index = SendDlgItemMessageA(hWnd, IDC_TERMKOUT, CB_ADDSTRING, 0, (LPARAM)p->menu_str);
				SendDlgItemMessageA(hWnd, IDC_TERMKOUT, CB_SETITEMDATA, index, (LPARAM)p->code);
				if (ts->KanjiOut == p->code) {
					kanji_out_index = i;
				}
			}
			ExpandCBWidth(hWnd, IDC_TERMKOUT);
			SendDlgItemMessageA(hWnd, IDC_TERMKOUT, CB_SETCURSEL, kanji_out_index, 0);

			// DEC Special Graphics
			CheckDlgButton(hWnd,
						   ts->Dec2Unicode == IdDecSpecialUniToDec ? IDC_DECSP_UNI2DEC:
						   ts->Dec2Unicode == IdDecSpecialDecToUni ? IDC_DECSP_DEC2UNI:
						   IDC_DECSP_DO_NOT,
						   BST_CHECKED);

			CheckDlgButton(hWnd, IDC_DEC2UNICODE_BOXDRAWING,
						   (ts->UnicodeDecSpMapping & 0x01) != 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_PUNCTUATION,
						   (ts->UnicodeDecSpMapping & 0x02) != 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_MIDDLEDOT,
						   (ts->UnicodeDecSpMapping & 0x04) != 0 ? BST_CHECKED : BST_UNCHECKED);

			// character width (現在の値)
			SetDropDownList(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CellWidthList, ts->UnicodeAmbiguousWidth == 1 ? 1 : 2);
			SetDropDownList(hWnd, IDC_EMOJI_WIDTH_COMBO, CellWidthList, ts->UnicodeEmojiWidth == 1 ? 1 : 2);
			CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, ts->UnicodeEmojiOverride ? BST_CHECKED : BST_UNCHECKED);

			// 文字ごとの文字幅オーバーライド設定
			const OverrideCharWidthInfo *info = &DlgData->unicode_override_charwidth_info;
			if (info->count == 0) {
				EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO), FALSE);
			}
			else {
				for (size_t i = 0; i < info->count; i++) {
					SendDlgItemMessageW(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO, CB_ADDSTRING, 0,
										(LPARAM)info->sets[i].name);
				}
				SendDlgItemMessageW(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO, CB_SETCURSEL,
									ts->UnicodeOverrideCharWidthSelected, 0);
				ExpandCBWidth(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO);
				const BOOL enable = (ts->UnicodeOverrideCharWidthEnable == 1) && (UnicodeOverrideWidthAvailable() != 0);
				EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH), TRUE);
				CheckDlgButton(hWnd, IDC_OVERRIDE_CHAR_WIDTH, enable ? BST_CHECKED : BST_UNCHECKED);
				EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO), enable);
			}

			ArrenageItems(hWnd, DlgData);

			return TRUE;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					CodingPPData *DlgData = (CodingPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);
					TTTSet *ts = DlgData->pts;

					ts->JIS7KatakanaSend = 0;
					ts->JIS7Katakana = 0;
					ts->KanjiIn = 0;
					ts->KanjiOut = 0;

					// 受信コード
					int curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					IdKanjiCode coding =
						(IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);
					ts->KanjiCode = coding;
					switch (coding) {
					case IdUTF8:
					case IdSJIS:
					case IdEUC:
						break;
					case IdJIS: {
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
						break;
					}
					case IdKoreanCP949:
					case IdCnGB2312:
					case IdCnBig5:
					case IdISO8859_1:
					case IdISO8859_2:
					case IdISO8859_3:
					case IdISO8859_4:
					case IdISO8859_5:
					case IdISO8859_6:
					case IdISO8859_7:
					case IdISO8859_8:
					case IdISO8859_9:
					case IdISO8859_10:
					case IdISO8859_11:
					case IdISO8859_13:
					case IdISO8859_14:
					case IdISO8859_15:
					case IdISO8859_16:
					case IdCP437:
					case IdCP737:
					case IdCP775:
					case IdCP850:
					case IdCP852:
					case IdCP855:
					case IdCP857:
					case IdCP860:
					case IdCP861:
					case IdCP862:
					case IdCP863:
					case IdCP864:
					case IdCP865:
					case IdCP866:
					case IdCP869:
					case IdCP874:
					case IdCP1250:
					case IdCP1251:
					case IdCP1252:
					case IdCP1253:
					case IdCP1254:
					case IdCP1255:
					case IdCP1256:
					case IdCP1257:
					case IdCP1258:
					case IdKOI8_NEW:
						// SBCS(Single Byte Code Set)
						break;
					case IdDebug:	// MinGW警告対策
						break;
#if !defined(__MINGW32__)
					default:
						// gcc/clangではswitchにenumのメンバがすべてないとき警告が出る
						assert(FALSE);
						break;
#endif
					}

					// 送信コード
					curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					coding = (IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);
					ts->KanjiCodeSend = coding;

					// characters as wide
					ts->UnicodeAmbiguousWidth = (BYTE)GetCurSel(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO);
					ts->UnicodeEmojiOverride = (BYTE)IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK);
					ts->UnicodeEmojiWidth = (BYTE)GetCurSel(hWnd, IDC_EMOJI_WIDTH_COMBO);

					// Override width
					{
						const OverrideCharWidthInfo *info = &DlgData->unicode_override_charwidth_info;

						BOOL enable = IsDlgButtonChecked(hWnd, IDC_OVERRIDE_CHAR_WIDTH);
						size_t selected =
							(size_t)SendDlgItemMessageW(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO, CB_GETCURSEL, 0, 0);
						if (!enable) {
							// disableする
							UnicodeOverrideWidthUninit();
						}
						else {
							if ((enable != ts->UnicodeOverrideCharWidthEnable) ||
								(selected != ts->UnicodeOverrideCharWidthSelected)) {
								// 設定を読み込む(enableにする/設定を変更する)
								const OverrideCharWidthInfoSet *set = &info->sets[selected];
								UnicodeOverrideWidthInit(set->file, set->section);
							}
						}

						ts->UnicodeOverrideCharWidthEnable = enable;
						ts->UnicodeOverrideCharWidthSelected = (BYTE)selected;
					}

					// DEC Special Graphics
					ts->Dec2Unicode =
						(DWORD)(IsDlgButtonChecked(hWnd, IDC_DECSP_DEC2UNI) ? IdDecSpecialDecToUni:
								IsDlgButtonChecked(hWnd, IDC_DECSP_UNI2DEC) ? IdDecSpecialUniToDec:
								IdDecSpecialDoNot);

					ts->UnicodeDecSpMapping =
						(WORD)((IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE_BOXDRAWING) << 0) |
							   (IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE_PUNCTUATION) << 1) |
							   (IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE_MIDDLEDOT) << 2));
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
					LRESULT sel_receive = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					if (IsDlgButtonChecked(hWnd, IDC_USE_DIFFERENT_CODE) == BST_UNCHECKED) {
						// 送信コードを同じ値にする
						SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, sel_receive, 0);
					}

					CodingPPData *DlgData = (CodingPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);
					ArrenageItems(hWnd, DlgData);
					break;
				}
				case IDC_TERMKANJISEND | (CBN_SELCHANGE << 16): {
					// 送信コード

					CodingPPData *DlgData = (CodingPPData *)GetWindowLongPtrW(hWnd, DWLP_USER);
					ArrenageItems(hWnd, DlgData);
					break;
				}
				case IDC_USE_DIFFERENT_CODE | (BN_CLICKED << 16): {
					const BOOL checked = (IsDlgButtonChecked(hWnd, IDC_USE_DIFFERENT_CODE) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), checked ? TRUE : FALSE);
					if (!checked) {
						// USE_DIFFRENT_CODEのチェックが入っていないるとき
						LRESULT recv_index = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
						LRESULT send_index = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
						if (recv_index != send_index) {
							// 送受信コードが異なっているなら、送信コードを受信と同一にする
							SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, recv_index, 0);
						}
					}
					break;
				}
				case IDC_EMOJI_WIDTH_CHECK | (BN_CLICKED << 16): {
					BOOL enable = (IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_EMOJI_WIDTH_COMBO), enable);
					break;
				}
				case IDC_OVERRIDE_CHAR_WIDTH | (BN_CLICKED << 16): {
					const BOOL enable = (IsDlgButtonChecked(hWnd, IDC_OVERRIDE_CHAR_WIDTH) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_OVERRIDE_CHAR_WIDTH_COMBO), enable);
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
	case PSPCB_CREATE: {
		CodingPPData *data = (CodingPPData *)ppsp->lParam;
		TTTSet *pts = data->pts;
		OverrideCharWidthInfoGet(pts->SetupFNameW, &data->unicode_override_charwidth_info);
		ret_val = 1;
		break;
	}
	case PSPCB_RELEASE: {
		CodingPPData *data = (CodingPPData *)ppsp->lParam;
		OverrideCharWidthInfoFree(&data->unicode_override_charwidth_info);
		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		free((void *)ppsp->lParam);
		ppsp->lParam = 0;
		break;
	}
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE CodingPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// 注 common/tt_res.h と coding_pp_res.h で値を一致させること
	int id = IDD_TABSHEET_CODING;

	CodingPPData *Param = (CodingPPData *)calloc(1, sizeof(CodingPPData));
	Param->UILanguageFileW = pts->UILanguageFileW;
	Param->pts = pts;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t* UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_ENCODING",
		         L"Encoding", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(id));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = Proc;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
