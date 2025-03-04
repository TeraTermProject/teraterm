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

#include "coding_pp.h"

// �e���v���[�g�̏����������s��
#define REWRITE_TEMPLATE

static const char *CellWidthList[] = { "1 Cell", "2 Cell", NULL };

struct CodingPPData {
	TTTSet *pts;
	const wchar_t *UILanguageFileW;
	DLGTEMPLATE *dlg_templ;
};

static void EnableWindows(HWND hWnd, const int *list, int count, BOOL enable)
{
	int i;
	for (i = 0; i < count; i++) {
		HWND w = GetDlgItem(hWnd, list[i]);
		EnableWindow(w, enable);
	}
}

static void ArrenageItems(HWND hWnd)
{
	static const int JJISReceiveItems[] = {
		IDC_TERMKINTEXT,
		IDC_TERMKIN,
		IDC_TERMKOUTTEXT,
		IDC_TERMKOUT,
		IDC_TERMKANA,
	};
	static const int JJISSendItems[] = {
		IDC_TERMKANASEND,
	};
	static const int UnicodeItems[] = {
		IDC_AMBIGUOUS_WIDTH_TITLE,
		IDC_AMBIGUOUS_WIDTH_COMBO,
		IDC_EMOJI_WIDTH_CHECK,
		IDC_EMOJI_WIDTH_COMBO,
	};

	// ��M�R�[�h
	LRESULT curPos = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
	IdKanjiCode coding_receive = (IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);

	// ���M�R�[�h
	curPos = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
	IdKanjiCode coding_send = (IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);

	bool is_unicode = ((coding_receive == IdUTF8) || (coding_send == IdUTF8));

	// Unicode character width
	if (is_unicode) {
		EnableWindows(hWnd, UnicodeItems, _countof(UnicodeItems), TRUE);
	}
	else {
		EnableWindows(hWnd, UnicodeItems, _countof(UnicodeItems), FALSE);
	}
	const int code_page = GetACP();
	if ((coding_send == IdUTF8 &&
		 (code_page == 932 || code_page == 949 || code_page == 936 || code_page == 950)) ||
		coding_send == IdSJIS || coding_send == IdEUC || coding_send == IdJIS ||
		coding_send == IdKoreanCP949 || coding_send == IdCnGB2312 || coding_send == IdCnBig5) {
		// UTF-8 �� CJK(CP932, CP949, CP936, CP950) �̂Ƃ�
		//   or DBCS(Unicode�ł͂Ȃ��̂őI���ł��Ȃ�)�̂Ƃ�
		SendDlgItemMessage(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CB_SETCURSEL, 1, 0);
		CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_CHECKED);
		SendDlgItemMessage(hWnd, IDC_EMOJI_WIDTH_COMBO, CB_SETCURSEL, 1, 0);
	}
	else {
		CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_UNCHECKED);
		SendDlgItemMessage(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CB_SETCURSEL, 0, 0);
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
		// { IDC_TERMKANJILABEL, "DLG_TERM_KANJI" },
		{ IDC_TERMKANJILABEL, "DLG_TERMK_KANJI" },
		//{ IDC_TERMKANA, "DLG_TERM_KANA" },
		{ IDC_TERMKANJISENDLABEL, "DLG_TERMK_KANJISEND" },
		//{ IDC_TERMKANASEND, "DLG_TERM_KANASEND" },
		{ IDC_TERMKINTEXT, "DLG_TERM_KIN" },
		{ IDC_TERMKOUTTEXT, "DLG_TERM_KOUT" },
		{ IDC_UNICODE2DEC, "DLG_CODING_UNICODE_TO_DEC" },
		{ IDC_DEC2UNICODE, "DLG_CODING_DEC_TO_UNICODE" },
		{ IDC_DEC2UNICODE_BOXDRAWING, "DLG_CODING_UNICODE_TO_DEC_BOXDRAWING" },
		{ IDC_DEC2UNICODE_PUNCTUATION, "DLG_CODING_UNICODE_TO_DEC_PUNCTUATION" },
		{ IDC_DEC2UNICODE_MIDDLEDOT, "DLG_CODING_UNICODE_TO_DEC_MIDDLEDOT" },
	};

	switch (msg) {
		case WM_INITDIALOG: {
			CodingPPData *DlgData = (CodingPPData *)(((PROPSHEETPAGEW_V1 *)lp)->lParam);
			const TTTSet *ts = DlgData->pts;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)DlgData);
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
					recv_index = index;
				}

				index = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_ADDSTRING, 0, (LPARAM)p->CodeStrGUI);
				SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETITEMDATA, index, (LPARAM)p->coding);
				if (ts->KanjiCodeSend == p->coding) {
					send_index = index;
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
			CheckDlgButton(hWnd, IDC_UNICODE2DEC, ts->Dec2Unicode ? BST_UNCHECKED : BST_CHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE, ts->Dec2Unicode ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_BOXDRAWING,
						   (ts->UnicodeDecSpMapping & 0x01) != 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_PUNCTUATION,
						   (ts->UnicodeDecSpMapping & 0x02) != 0 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_DEC2UNICODE_MIDDLEDOT,
						   (ts->UnicodeDecSpMapping & 0x04) != 0 ? BST_CHECKED : BST_UNCHECKED);

			ArrenageItems(hWnd);

			// character width (���݂̒l)
			SetDropDownList(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO, CellWidthList, ts->UnicodeAmbiguousWidth == 1 ? 1 : 2);
			SetDropDownList(hWnd, IDC_EMOJI_WIDTH_COMBO, CellWidthList, ts->UnicodeEmojiWidth == 1 ? 1 : 2);
			if (ts->UnicodeEmojiOverride) {
				CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_CHECKED);
			} else {
				CheckDlgButton(hWnd, IDC_EMOJI_WIDTH_CHECK, BST_UNCHECKED);
			}

			return TRUE;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lp;
			switch (nmhdr->code) {
				case PSN_APPLY: {
					CodingPPData *DlgData = (CodingPPData *)GetWindowLongPtr(hWnd, DWLP_USER);
					TTTSet *ts = DlgData->pts;

					ts->JIS7KatakanaSend = 0;
					ts->JIS7Katakana = 0;
					ts->KanjiIn = 0;
					ts->KanjiOut = 0;

					// ��M�R�[�h
					int curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					IdKanjiCode coding =
						(IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETITEMDATA, curPos, 0);
					ts->KanjiCode = coding;
					if (coding == IdUTF8) {
						;
					}
					else if (coding == IdSJIS || coding == IdEUC) {
						;
					}
					else if (coding == IdJIS) {
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
					else if (coding == IdWindows || coding == IdKOI8 || coding == Id866 || coding == IdISO) {
						;
					}
					else if (coding == IdKoreanCP949) {
						;
					}
					else if (coding == IdCnGB2312 || coding == IdCnBig5) {
						;
					}
					else if (LangIsEnglish(coding)) {
						;
					}
					else {
						assert(FALSE);
					}

					// ���M�R�[�h
					curPos = (int)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
					coding = (IdKanjiCode)SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETITEMDATA, curPos, 0);
					ts->KanjiCodeSend = coding;

					// characters as wide
					ts->UnicodeAmbiguousWidth = (BYTE)GetCurSel(hWnd, IDC_AMBIGUOUS_WIDTH_COMBO);
					ts->UnicodeEmojiOverride = (BYTE)IsDlgButtonChecked(hWnd, IDC_EMOJI_WIDTH_CHECK);
					ts->UnicodeEmojiWidth = (BYTE)GetCurSel(hWnd, IDC_EMOJI_WIDTH_COMBO);

					// DEC Special Graphics
					ts->Dec2Unicode = (BYTE)IsDlgButtonChecked(hWnd, IDC_DEC2UNICODE);
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
					// ��M�R�[�h
					LRESULT sel_receive = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
					if (IsDlgButtonChecked(hWnd, IDC_USE_DIFFERENT_CODE) == BST_UNCHECKED) {
						// ���M�R�[�h�𓯂��l�ɂ���
						SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_SETCURSEL, sel_receive, 0);
					}

					ArrenageItems(hWnd);
					break;
				}
				case IDC_TERMKANJISEND | (CBN_SELCHANGE << 16): {
					// ���M�R�[�h

					ArrenageItems(hWnd);
					break;
				}
				case IDC_USE_DIFFERENT_CODE | (BN_CLICKED << 16): {
					const BOOL checked = (IsDlgButtonChecked(hWnd, IDC_USE_DIFFERENT_CODE) == BST_CHECKED) ? TRUE : FALSE;
					EnableWindow(GetDlgItem(hWnd, IDC_TERMKANJISEND), checked ? TRUE : FALSE);
					if (!checked) {
						// USE_DIFFRENT_CODE�̃`�F�b�N�������Ă��Ȃ���Ƃ�
						LRESULT recv_index = SendDlgItemMessageA(hWnd, IDC_TERMKANJI, CB_GETCURSEL, 0, 0);
						LRESULT send_index = SendDlgItemMessageA(hWnd, IDC_TERMKANJISEND, CB_GETCURSEL, 0, 0);
						if (recv_index != send_index) {
							// ����M�R�[�h���قȂ��Ă���Ȃ�A���M�R�[�h����M�Ɠ���ɂ���
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

HPROPSHEETPAGE CodingPageCreate(HINSTANCE inst, TTTSet *pts)
{
	// �� common/tt_res.h �� coding_pp_res.h �Œl����v�����邱��
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
