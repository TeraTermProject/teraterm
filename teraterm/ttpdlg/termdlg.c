/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2024- TeraTerm Project
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

/* terminal dialog */

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <windows.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "dlg_res.h"
#include "helpid.h"
#include "ttlib_charset.h"
#include "asprintf.h"
#include "ttwinman.h"

#include "ttdlg.h"

static const char *NLListRcv[] = {"CR", "CR+LF", "LF", "AUTO", NULL};
static const char *NLList[] = {"CR","CR+LF", "LF", NULL};
static const char *TermList[] =
	{"VT100", "VT101", "VT102", "VT282", "VT320", "VT382",
	 "VT420", "VT520", "VT525", NULL};
static WORD Term_TermJ[] =
	{IdVT100, IdVT101, IdVT102, IdVT282, IdVT320, IdVT382,
	 IdVT420, IdVT520, IdVT525};
static WORD TermJ_Term[] = {1, 1, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9};
static const char *TermListJ[] =
	{"VT100", "VT100J", "VT101", "VT102", "VT102J", "VT220J", "VT282",
	 "VT320", "VT382", "VT420", "VT520", "VT525", NULL};
static const char *KanjiInList[] = {"^[$@","^[$B",NULL};
static const char *KanjiOutList[] = {"^[(B","^[(J",NULL};
static const char *KanjiOutList2[] = {"^[(B","^[(J","^[(H",NULL};

typedef struct {
	TTTSet *pts;
	HWND VTWin;
} DialogData;

static void SetKanjiCodeDropDownList(HWND HDlg, int id, int language, int sel_code)
{
	int i;
	LRESULT sel_index = 0;

	for(i = 0;; i++) {
		LRESULT index;
		const TKanjiList *p = GetKanjiList(i);
		if (p == NULL) {
			break;
		}
		if (p->lang != language) {
			continue;
		}

		index = SendDlgItemMessageA(HDlg, id, CB_ADDSTRING, 0, (LPARAM)p->KanjiCode);
		SendDlgItemMessageA(HDlg, id, CB_SETITEMDATA, index, p->coding);
		if (p->coding == sel_code) {
			sel_index = index;
		}
	}
	SendDlgItemMessageA(HDlg, id, CB_SETCURSEL, sel_index, 0);
}

static INT_PTR CALLBACK TermDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfosCom[] = {
		{ 0, "DLG_TERM_TITLE" },
		{ IDC_TERMWIDTHLABEL, "DLG_TERM_WIDTHLABEL" },
		{ IDC_TERMISWIN, "DLG_TERM_ISWIN" },
		{ IDC_TERMRESIZE, "DLG_TERM_RESIZE" },
		{ IDC_TERMNEWLINE, "DLG_TERM_NEWLINE" },
		{ IDC_TERMCRRCVLABEL, "DLG_TERM_CRRCV" },
		{ IDC_TERMCRSENDLABEL, "DLG_TERM_CRSEND" },
		{ IDC_TERMIDLABEL, "DLG_TERM_ID" },
		{ IDC_TERMLOCALECHO, "DLG_TERM_LOCALECHO" },
		{ IDC_TERMANSBACKTEXT, "DLG_TERM_ANSBACK" },
		{ IDC_TERMAUTOSWITCH, "DLG_TERM_AUTOSWITCH" },
		{ IDC_TERMKANJILABEL, "DLG_TERM_KANJI" },
		{ IDC_TERMKANJISENDLABEL, "DLG_TERM_KANJISEND" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_TERMHELP, "BTN_HELP" },
	};
	WORD w;

	switch (Message) {
		case WM_INITDIALOG: {
			DialogData *data = (DialogData *)lParam;
			TTTSet *ts = data->pts;
			SetWindowLongPtrW(Dialog, DWLP_USER, lParam);

			SetDlgTextsW(Dialog, TextInfosCom, _countof(TextInfosCom), ts->UILanguageFileW);
			if (ts->Language==IdJapanese) {
				// 日本語の時だけ4つの項目が存在する
				static const DlgTextInfo TextInfosJp[] = {
					{ IDC_TERMKANA, "DLG_TERM_KANA" },
					{ IDC_TERMKANASEND, "DLG_TERM_KANASEND" },
					{ IDC_TERMKINTEXT, "DLG_TERM_KIN" },
					{ IDC_TERMKOUTTEXT, "DLG_TERM_KOUT" },
				};
				SetDlgTextsW(Dialog, TextInfosJp, _countof(TextInfosJp), ts->UILanguageFileW);
			}

			SetDlgItemInt(Dialog,IDC_TERMWIDTH,ts->TerminalWidth,FALSE);
			SendDlgItemMessage(Dialog, IDC_TERMWIDTH, EM_LIMITTEXT,3, 0);

			SetDlgItemInt(Dialog,IDC_TERMHEIGHT,ts->TerminalHeight,FALSE);
			SendDlgItemMessage(Dialog, IDC_TERMHEIGHT, EM_LIMITTEXT,3, 0);

			SetRB(Dialog,ts->TermIsWin,IDC_TERMISWIN,IDC_TERMISWIN);
			SetRB(Dialog,ts->AutoWinResize,IDC_TERMRESIZE,IDC_TERMRESIZE);
			if ( ts->TermIsWin>0 )
				DisableDlgItem(Dialog,IDC_TERMRESIZE,IDC_TERMRESIZE);

			SetDropDownList(Dialog, IDC_TERMCRRCV, NLListRcv, ts->CRReceive); // add 'LF' (2007.1.21 yutaka), added "AUTO" (9th Apr 2012, tentner)
			SetDropDownList(Dialog, IDC_TERMCRSEND, NLList, ts->CRSend);

			if ( ts->Language!=IdJapanese ) { /* non-Japanese mode */
				if ((ts->TerminalID>=1) &&
					(ts->TerminalID <= sizeof(TermJ_Term)/sizeof(WORD))) {
					w = TermJ_Term[ts->TerminalID-1];
				}
				else {
					w = 1;
				}
				SetDropDownList(Dialog, IDC_TERMID, TermList, w);
			}
			else {
				SetDropDownList(Dialog, IDC_TERMID, TermListJ, ts->TerminalID);
			}

			SetRB(Dialog,ts->LocalEcho,IDC_TERMLOCALECHO,IDC_TERMLOCALECHO);

			if ((ts->FTFlag & FT_BPAUTO)!=0) {
				DisableDlgItem(Dialog,IDC_TERMANSBACKTEXT,IDC_TERMANSBACK);
			}
			else {
				char Temp[81];
				Str2Hex(ts->Answerback,Temp,ts->AnswerbackLen,
					sizeof(Temp)-1,FALSE);
				SetDlgItemText(Dialog, IDC_TERMANSBACK, Temp);
				SendDlgItemMessage(Dialog, IDC_TERMANSBACK, EM_LIMITTEXT,
					sizeof(Temp) - 1, 0);
			}

			SetRB(Dialog,ts->AutoWinSwitch,IDC_TERMAUTOSWITCH,IDC_TERMAUTOSWITCH);

			SetKanjiCodeDropDownList(Dialog, IDC_TERMKANJI, ts->Language, ts->KanjiCode);
			SetKanjiCodeDropDownList(Dialog, IDC_TERMKANJISEND, ts->Language, ts->KanjiCodeSend);
			if (ts->Language==IdJapanese) {
				if ( ts->KanjiCode!=IdJIS ) {
					DisableDlgItem(Dialog,IDC_TERMKANA,IDC_TERMKANA);
				}
				SetRB(Dialog,ts->JIS7Katakana,IDC_TERMKANA,IDC_TERMKANA);
				if ( ts->KanjiCodeSend!=IdJIS ) {
					DisableDlgItem(Dialog,IDC_TERMKANASEND,IDC_TERMKOUT);
				}
				SetRB(Dialog,ts->JIS7KatakanaSend,IDC_TERMKANASEND,IDC_TERMKANASEND);

				{
					const char **kanji_out_list;
					int n;
					n = ts->KanjiIn;
					n = (n <= 0 || 2 < n) ? IdKanjiInB : n;
					SetDropDownList(Dialog, IDC_TERMKIN, KanjiInList, n);

					kanji_out_list = (ts->TermFlag & TF_ALLOWWRONGSEQUENCE) ? KanjiOutList2 : KanjiOutList;
					n = ts->KanjiOut;
					n = (n <= 0 || 3 < n) ? IdKanjiOutB : n;
					SetDropDownList(Dialog, IDC_TERMKOUT, kanji_out_list, n);
				}
			}
			CenterWindow(Dialog, GetParent(Dialog));
			return TRUE;
		}

		case WM_COMMAND: {
			DialogData *data = (DialogData *)GetWindowLongPtrW(Dialog,DWLP_USER);
			assert(data != NULL);
			TTTSet *ts = data->pts;
			switch (LOWORD(wParam)) {
				case IDOK: {
					int width, height;

					width = GetDlgItemInt(Dialog, IDC_TERMWIDTH, NULL, FALSE);
					if (width > TermWidthMax) {
						ts->TerminalWidth = TermWidthMax;
					}
					else if (width > 0) {
						ts->TerminalWidth = width;
					}
					else { // 0 以下の場合は変更しない
						;
					}

					height = GetDlgItemInt(Dialog, IDC_TERMHEIGHT, NULL, FALSE);
					if (height > TermHeightMax) {
						ts->TerminalHeight = TermHeightMax;
					}
					else if (height > 0) {
						ts->TerminalHeight = height;
					}
					else { // 0 以下の場合は変更しない
						;
					}

					GetRB(Dialog,&ts->TermIsWin,IDC_TERMISWIN,IDC_TERMISWIN);
					GetRB(Dialog,&ts->AutoWinResize,IDC_TERMRESIZE,IDC_TERMRESIZE);

					if ((w = (WORD)GetCurSel(Dialog, IDC_TERMCRRCV)) > 0) {
						ts->CRReceive = w;
					}
					if ((w = (WORD)GetCurSel(Dialog, IDC_TERMCRSEND)) > 0) {
						ts->CRSend = w;
					}

					if ((w = (WORD)GetCurSel(Dialog, IDC_TERMID)) > 0) {
						if ( ts->Language!=IdJapanese ) { /* non-Japanese mode */
							if (w > sizeof(Term_TermJ)/sizeof(WORD)) w = 1;
							w = Term_TermJ[w-1];
						}
						ts->TerminalID = w;
					}

					GetRB(Dialog,&ts->LocalEcho,IDC_TERMLOCALECHO,IDC_TERMLOCALECHO);

					if ((ts->FTFlag & FT_BPAUTO)==0) {
						char Temp[81];
						GetDlgItemText(Dialog, IDC_TERMANSBACK,Temp,sizeof(Temp));
						ts->AnswerbackLen = Hex2Str(Temp,ts->Answerback,sizeof(ts->Answerback));
					}

					GetRB(Dialog,&ts->AutoWinSwitch,IDC_TERMAUTOSWITCH,IDC_TERMAUTOSWITCH);

					if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKANJI)) > 0) {
						w = (int)SendDlgItemMessageA(Dialog, IDC_TERMKANJI, CB_GETITEMDATA, w - 1, 0);
						ts->KanjiCode = w;
					}
					if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND)) > 0) {
						w = (int)SendDlgItemMessageA(Dialog, IDC_TERMKANJISEND, CB_GETITEMDATA, w - 1, 0);
						ts->KanjiCodeSend = w;
					}
					if (ts->Language==IdJapanese) {
						GetRB(Dialog,&ts->JIS7Katakana,IDC_TERMKANA,IDC_TERMKANA);
						GetRB(Dialog,&ts->JIS7KatakanaSend,IDC_TERMKANASEND,IDC_TERMKANASEND);
						if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKIN)) > 0) {
							ts->KanjiIn = w;
						}
						if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKOUT)) > 0) {
							ts->KanjiOut = w;
						}
					}
					else {
						ts->JIS7KatakanaSend=0;
						ts->JIS7Katakana=0;
						ts->KanjiIn = 0;
						ts->KanjiOut = 0;
					}

					TTEndDialog(Dialog, 1);
					return TRUE;
				}

				case IDCANCEL:
					TTEndDialog(Dialog, 0);
					return TRUE;

				case IDC_TERMISWIN:
					GetRB(Dialog,&w,IDC_TERMISWIN,IDC_TERMISWIN);
					if ( w==0 ) {
						EnableDlgItem(Dialog,IDC_TERMRESIZE,IDC_TERMRESIZE);
					}
					else {
						DisableDlgItem(Dialog,IDC_TERMRESIZE,IDC_TERMRESIZE);
					}
					break;

				case IDC_TERMKANJI:
					w = (WORD)GetCurSel(Dialog, IDC_TERMKANJI);
					w = (WORD)SendDlgItemMessageA(Dialog, IDC_TERMKANJI, CB_GETITEMDATA, w - 1, 0);
					if (w==IdJIS) {
						EnableDlgItem(Dialog,IDC_TERMKANA,IDC_TERMKANA);
					}
					else {
						DisableDlgItem(Dialog,IDC_TERMKANA,IDC_TERMKANA);
						}
					break;

				case IDC_TERMKANJISEND:
					w = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND);
					w = (WORD)SendDlgItemMessageA(Dialog, IDC_TERMKANJISEND, CB_GETITEMDATA, w - 1, 0);
					if (w==IdJIS) {
						EnableDlgItem(Dialog,IDC_TERMKANASEND,IDC_TERMKOUT);
					}
					else {
						DisableDlgItem(Dialog,IDC_TERMKANASEND,IDC_TERMKOUT);
					}
					break;

				case IDC_TERMHELP: {
					WPARAM HelpId;
					switch (ts->Language) {
					case IdJapanese:
						HelpId = HlpSetupTerminalJa;
						break;
					case IdEnglish:
						HelpId = HlpSetupTerminalEn;
						break;
					case IdKorean:
						HelpId = HlpSetupTerminalKo;
						break;
					case IdRussian:
						HelpId = HlpSetupTerminalRu;
						break;
					case IdUtf8:
						HelpId = HlpSetupTerminalUtf8;
						break;
					default:
						HelpId = HlpSetupTerminal;
						break;
					}
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HelpId,0);
					break;
				}
			}
		}
	}
	return FALSE;
}

BOOL WINAPI _SetupTerminal(HWND WndParent, PTTSet ts)
{
	DialogData *data;
	int i;

	data = (DialogData *)malloc(sizeof(*data));
	data->pts = ts;
	data->VTWin = WndParent;

	switch (ts->Language) {
	case IdJapanese: // Japanese mode
		i = IDD_TERMDLGJ;
		break;
	case IdKorean: // Korean mode //HKS
	case IdUtf8:   // UTF-8 mode
	case IdChinese:
	case IdRussian: // Russian mode
	case IdEnglish:  // English mode
		i = IDD_TERMDLGK;
		break;
	default:
		// 使っていない
		i = IDD_TERMDLG;
	}

	BOOL r = (BOOL)TTDialogBoxParam(hInst,
							  MAKEINTRESOURCE(i),
							  WndParent, TermDlg, (LPARAM)data);
	free(data);
	return r;
}
