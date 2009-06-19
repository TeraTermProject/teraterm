/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TTDLG.DLL, dialog boxes */
#include "teraterm.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <commdlg.h>
#include <Dlgs.h>
#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "ttcommon.h"
#include "dlg_res.h"
#include "svnversion.h"

// Oniguruma: Regular expression library
#define ONIG_EXTERN extern
#include "oniguruma.h"
#undef ONIG_EXTERN

#ifndef NO_INET6
#include <winsock2.h>
static char FAR * ProtocolFamilyList[] = { "UNSPEC", "IPv6", "IPv4", NULL };
#endif /* NO_INET6 */

#include "compat_w95.h"

static HANDLE hInst;

static HFONT DlgAboutFont;
static HFONT DlgDirFont;
static HFONT DlgGenFont;
static HFONT DlgHostFont;
static HFONT DlgKeybFont;
static HFONT DlgSerialFont;
static HFONT DlgTcpipFont;
static HFONT DlgTermFont;
static HFONT DlgWinFont;
static HFONT DlgWinlistFont;

char UILanguageFile[MAX_PATH];

static PCHAR far NLListRcv[] = {"CR","CR+LF", "LF", NULL};
static PCHAR far NLList[] = {"CR","CR+LF", NULL};
static PCHAR far TermList[] =
	{"VT100","VT101","VT102","VT282","VT320","VT382",NULL};
static WORD TermJ_Term[] = {1,1,2,3,3,4,4,5,6};
static WORD Term_TermJ[] = {1,3,4,7,8,9};
static PCHAR far TermListJ[] =
	{"VT100","VT100J","VT101","VT102","VT102J","VT220J",
	 "VT282","VT320","VT382",NULL};
static PCHAR far KanjiList[] = {"SJIS","EUC","JIS", "UTF-8", "UTF-8m", NULL};
static PCHAR far KanjiListSend[] = {"SJIS","EUC","JIS", "UTF-8", NULL};
static PCHAR far KanjiInList[] = {"^[$@","^[$B",NULL};
static PCHAR far KanjiOutList[] = {"^[(B","^[(J",NULL};
static PCHAR far KanjiOutList2[] = {"^[(B","^[(J","^[(H",NULL};
static PCHAR far RussList[] = {"Windows","KOI8-R","CP 866","ISO 8859-5",NULL};
static PCHAR far RussList2[] = {"Windows","KOI8-R",NULL};
static PCHAR far LocaleList[] = {"japanese","chinese", "chinese-simplified", "chinese-traditional", NULL};

// HKS
static PCHAR far KoreanList[] = {"KS5601", "UTF-8", "UTF-8m", NULL};
static PCHAR far KoreanListSend[] = {"KS5601", "UTF-8", NULL};

// UTF-8
static PCHAR far Utf8List[] = {"UTF-8", "UTF-8m", NULL};
static PCHAR far Utf8ListSend[] = {"UTF-8", NULL};


BOOL CALLBACK TermDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PTTSet ts;
	WORD w;
	//  char Temp[HostNameMaxLength + 1]; // 81(yutaka)
	char Temp[81]; // 81(yutaka)
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgTermFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_TERMWIDTHLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMWIDTH, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMX, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMHEIGHT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMISWIN, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMRESIZE, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMNEWLINE, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMCRRCVLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMCRRCV, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMCRSENDLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMCRSEND, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMIDLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMID, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMLOCALECHO, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMANSBACKTEXT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMANSBACK, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMAUTOSWITCH, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERMHELP, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				if (ts->Language==IdJapanese) {
					SendDlgItemMessage(Dialog, IDC_TERMKANJILABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANJI, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANA, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANJISENDLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANJISEND, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANASEND, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKINTEXT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKIN, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKOUTTEXT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKOUT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_LOCALE_LABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_LOCALE_EDIT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_CODEPAGE_LABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_CODEPAGE_EDIT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				}
				else if (ts->Language==IdRussian) {
					SendDlgItemMessage(Dialog, IDC_TERMRUSSCHARSET, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMRUSSHOSTLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMRUSSHOST, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMRUSSCLIENTLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMRUSSCLIENT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMRUSSFONTLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMRUSSFONT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				}
				else if (ts->Language==IdUtf8 || ts->Language==IdKorean) {
					SendDlgItemMessage(Dialog, IDC_TERMKANJILABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANJI, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANJISENDLABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_TERMKANJISEND, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_LOCALE_LABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_LOCALE_EDIT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_CODEPAGE_LABEL, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
					SendDlgItemMessage(Dialog, IDC_CODEPAGE_EDIT, WM_SETFONT, (WPARAM)DlgTermFont, MAKELPARAM(TRUE,0));
				}
			}
			else {
				DlgTermFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_TERMWIDTHLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_WIDTHLABEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMWIDTHLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_TERMISWIN, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_ISWIN", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMISWIN, uimsg);
			GetDlgItemText(Dialog, IDC_TERMRESIZE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_RESIZE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMRESIZE, uimsg);
			GetDlgItemText(Dialog, IDC_TERMNEWLINE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_NEWLINE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMNEWLINE, uimsg);
			GetDlgItemText(Dialog, IDC_TERMCRRCVLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_CRRCV", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMCRRCVLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_TERMCRSENDLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_CRSEND", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMCRSENDLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_TERMIDLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_ID", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMIDLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_TERMLOCALECHO, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_LOCALECHO", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMLOCALECHO, uimsg);
			GetDlgItemText(Dialog, IDC_TERMANSBACKTEXT, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_ANSBACK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMANSBACKTEXT, uimsg);
			GetDlgItemText(Dialog, IDC_TERMAUTOSWITCH, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TERM_AUTOSWITCH", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMAUTOSWITCH, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_TERMHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TERMHELP, uimsg);
			if ( ts->Language==IdJapanese ) {
				GetDlgItemText(Dialog, IDC_TERMKANJILABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_KANJI", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKANJILABEL, uimsg);
				GetDlgItemText(Dialog, IDC_TERMKANA, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_KANA", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKANA, uimsg);
				GetDlgItemText(Dialog, IDC_TERMKANJISENDLABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_KANJISEND", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKANJISENDLABEL, uimsg);
				GetDlgItemText(Dialog, IDC_TERMKANASEND, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_KANASEND", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKANASEND, uimsg);
				GetDlgItemText(Dialog, IDC_TERMKINTEXT, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_KIN", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKINTEXT, uimsg);
				GetDlgItemText(Dialog, IDC_TERMKOUTTEXT, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_KOUT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKOUTTEXT, uimsg);
				GetDlgItemText(Dialog, IDC_LOCALE_LABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_LOCALE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_LOCALE_LABEL, uimsg);
				GetDlgItemText(Dialog, IDC_CODEPAGE_LABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_CODEPAGE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_CODEPAGE_LABEL, uimsg);
			}
			else if ( ts->Language==IdRussian ) {
				GetDlgItemText(Dialog, IDC_TERMRUSSCHARSET, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_RUSSCHARSET", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMRUSSCHARSET, uimsg);
				GetDlgItemText(Dialog, IDC_TERMRUSSHOSTLABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_RUSSHOST", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMRUSSHOSTLABEL, uimsg);
				GetDlgItemText(Dialog, IDC_TERMRUSSCLIENTLABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_RUSSCLIENT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMRUSSCLIENTLABEL, uimsg);
				GetDlgItemText(Dialog, IDC_TERMRUSSFONTLABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_RUSSFONT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMRUSSFONTLABEL, uimsg);
			}
			else if (ts->Language==IdUtf8 || ts->Language==IdKorean) {
				GetDlgItemText(Dialog, IDC_TERMKANJILABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERMK_KANJI", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKANJILABEL, uimsg);
				GetDlgItemText(Dialog, IDC_TERMKANJISENDLABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERMK_KANJISEND", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_TERMKANJISENDLABEL, uimsg);
				GetDlgItemText(Dialog, IDC_LOCALE_LABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_LOCALE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_LOCALE_LABEL, uimsg);
				GetDlgItemText(Dialog, IDC_CODEPAGE_LABEL, uimsg2, sizeof(uimsg2));
				get_lang_msg("DLG_TERM_CODEPAGE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
				SetDlgItemText(Dialog, IDC_CODEPAGE_LABEL, uimsg);
			}

			SetDlgItemInt(Dialog,IDC_TERMWIDTH,ts->TerminalWidth,FALSE);
			SendDlgItemMessage(Dialog, IDC_TERMWIDTH, EM_LIMITTEXT,3, 0);

			SetDlgItemInt(Dialog,IDC_TERMHEIGHT,ts->TerminalHeight,FALSE);
			SendDlgItemMessage(Dialog, IDC_TERMHEIGHT, EM_LIMITTEXT,3, 0);

			SetRB(Dialog,ts->TermIsWin,IDC_TERMISWIN,IDC_TERMISWIN);
			SetRB(Dialog,ts->AutoWinResize,IDC_TERMRESIZE,IDC_TERMRESIZE);
			if ( ts->TermIsWin>0 )
				DisableDlgItem(Dialog,IDC_TERMRESIZE,IDC_TERMRESIZE);

			SetDropDownList(Dialog, IDC_TERMCRRCV, NLListRcv, ts->CRReceive); // add 'LF' (2007.1.21 yutaka)
			SetDropDownList(Dialog, IDC_TERMCRSEND, NLList, ts->CRSend);

			if ( ts->Language!=IdJapanese ) { /* non-Japanese mode */
				if ((ts->TerminalID>=1) &&
					(ts->TerminalID<=9)) {
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
				Str2Hex(ts->Answerback,Temp,ts->AnswerbackLen,
					sizeof(Temp)-1,FALSE);
				SetDlgItemText(Dialog, IDC_TERMANSBACK, Temp);
				SendDlgItemMessage(Dialog, IDC_TERMANSBACK, EM_LIMITTEXT,
					sizeof(Temp) - 1, 0);
			}

			SetRB(Dialog,ts->AutoWinSwitch,IDC_TERMAUTOSWITCH,IDC_TERMAUTOSWITCH);

			if (ts->Language==IdJapanese) {
				SetDropDownList(Dialog, IDC_TERMKANJI, KanjiList, ts->KanjiCode);
				if ( ts->KanjiCode!=IdJIS ) {
					DisableDlgItem(Dialog,IDC_TERMKANA,IDC_TERMKANA);
				}
				SetRB(Dialog,ts->JIS7Katakana,IDC_TERMKANA,IDC_TERMKANA);
				SetDropDownList(Dialog, IDC_TERMKANJISEND, KanjiListSend, ts->KanjiCodeSend);
				if ( ts->KanjiCodeSend!=IdJIS ) {
					DisableDlgItem(Dialog,IDC_TERMKANASEND,IDC_TERMKOUT);
				}
				SetRB(Dialog,ts->JIS7KatakanaSend,IDC_TERMKANASEND,IDC_TERMKANASEND);
				SetDropDownList(Dialog,IDC_TERMKIN,KanjiInList,ts->KanjiIn);
				if ((ts->TermFlag & TF_ALLOWWRONGSEQUENCE)!=0) {
					SetDropDownList(Dialog,IDC_TERMKOUT,KanjiOutList2,ts->KanjiOut);
				}
				else {
					SetDropDownList(Dialog,IDC_TERMKOUT,KanjiOutList,ts->KanjiOut);
				}

				// ロケール用テキストボックス
				SetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale);
				SendDlgItemMessage(Dialog, IDC_LOCALE_EDIT, EM_LIMITTEXT, sizeof(ts->Locale), 0);

				SetDlgItemInt(Dialog, IDC_CODEPAGE_EDIT, ts->CodePage, FALSE);
			}
			else if (ts->Language==IdRussian) {
				SetDropDownList(Dialog,IDC_TERMRUSSHOST,RussList,ts->RussHost);
				SetDropDownList(Dialog,IDC_TERMRUSSCLIENT,RussList,ts->RussClient);
				SetDropDownList(Dialog,IDC_TERMRUSSFONT,RussList,ts->RussFont);
			}
			else if (ts->Language==IdKorean) { // HKS
				SetDropDownList(Dialog, IDC_TERMKANJI, KoreanList, KanjiCode2List(ts->Language,ts->KanjiCode));
				SetDropDownList(Dialog, IDC_TERMKANJISEND, KoreanListSend, KanjiCode2List(ts->Language,ts->KanjiCodeSend));

				// ロケール用テキストボックス
				SetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale);
				SendDlgItemMessage(Dialog, IDC_LOCALE_EDIT, EM_LIMITTEXT, sizeof(ts->Locale), 0);

				SetDlgItemInt(Dialog, IDC_CODEPAGE_EDIT, ts->CodePage, FALSE);
			}
			else if (ts->Language==IdUtf8) {
				SetDropDownList(Dialog, IDC_TERMKANJI, Utf8List, KanjiCode2List(ts->Language,ts->KanjiCode));
				SetDropDownList(Dialog, IDC_TERMKANJISEND, Utf8ListSend, KanjiCode2List(ts->Language,ts->KanjiCodeSend));

				// ロケール用テキストボックス
				SetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale);
				SendDlgItemMessage(Dialog, IDC_LOCALE_EDIT, EM_LIMITTEXT, sizeof(ts->Locale), 0);

				SetDlgItemInt(Dialog, IDC_CODEPAGE_EDIT, ts->CodePage, FALSE);
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);

					if ( ts!=NULL ) {
						ts->TerminalWidth = GetDlgItemInt(Dialog,IDC_TERMWIDTH,NULL,FALSE);
						if ( ts->TerminalWidth<1 ) {
							ts->TerminalWidth = 1;
						}
						if ( ts->TerminalWidth>500 ) {
							ts->TerminalWidth = 500;
						}

						ts->TerminalHeight = GetDlgItemInt(Dialog,IDC_TERMHEIGHT,NULL,FALSE);
						if ( ts->TerminalHeight<1 ) {
							ts->TerminalHeight = 1;
						}
						if ( ts->TerminalHeight>500 ) {
							ts->TerminalHeight = 500;
						}

						GetRB(Dialog,&ts->TermIsWin,IDC_TERMISWIN,IDC_TERMISWIN);
						GetRB(Dialog,&ts->AutoWinResize,IDC_TERMRESIZE,IDC_TERMRESIZE);

						ts->CRReceive = (WORD)GetCurSel(Dialog, IDC_TERMCRRCV);
						ts->CRSend = (WORD)GetCurSel(Dialog, IDC_TERMCRSEND);

						w = (WORD)GetCurSel(Dialog, IDC_TERMID);
						if ( ts->Language!=IdJapanese ) { /* non-Japanese mode */
							if ((w==0) || (w>6)) w = 1;
							w = Term_TermJ[w-1];
						}
						ts->TerminalID = w;

						GetRB(Dialog,&ts->LocalEcho,IDC_TERMLOCALECHO,IDC_TERMLOCALECHO);

						if ((ts->FTFlag & FT_BPAUTO)==0) {
							GetDlgItemText(Dialog, IDC_TERMANSBACK,Temp,sizeof(Temp));
							ts->AnswerbackLen = Hex2Str(Temp,ts->Answerback,sizeof(ts->Answerback));
						}

						GetRB(Dialog,&ts->AutoWinSwitch,IDC_TERMAUTOSWITCH,IDC_TERMAUTOSWITCH);

						if (ts->Language==IdJapanese) {
							BOOL ret;

							ts->KanjiCode = (WORD)GetCurSel(Dialog, IDC_TERMKANJI);
							GetRB(Dialog,&ts->JIS7Katakana,IDC_TERMKANA,IDC_TERMKANA);
							ts->KanjiCodeSend = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND);
							GetRB(Dialog,&ts->JIS7KatakanaSend,IDC_TERMKANASEND,IDC_TERMKANASEND);
							ts->KanjiIn = (WORD)GetCurSel(Dialog, IDC_TERMKIN);
							ts->KanjiOut = (WORD)GetCurSel(Dialog, IDC_TERMKOUT);

							GetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale, sizeof(ts->Locale));
							ts->CodePage = GetDlgItemInt(Dialog, IDC_CODEPAGE_EDIT, &ret, FALSE);
						}
						else if (ts->Language==IdRussian) {
							ts->RussHost = (WORD)GetCurSel(Dialog, IDC_TERMRUSSHOST);
							ts->RussClient = (WORD)GetCurSel(Dialog, IDC_TERMRUSSCLIENT);
							ts->RussFont = (WORD)GetCurSel(Dialog, IDC_TERMRUSSFONT);
						}
						else if (ts->Language==IdKorean || // HKS
						         ts->Language==IdUtf8) {
							BOOL ret;
							WORD listId;

							listId = (WORD)GetCurSel(Dialog, IDC_TERMKANJI);
							ts->KanjiCode = List2KanjiCode(ts->Language,listId);
							listId = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND);
							ts->KanjiCodeSend = List2KanjiCode(ts->Language,listId);

							ts->JIS7KatakanaSend=0;
							ts->JIS7Katakana=0;
							ts->KanjiIn = 0;
							ts->KanjiOut = 0;

							GetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale, sizeof(ts->Locale));
							ts->CodePage = GetDlgItemInt(Dialog, IDC_CODEPAGE_EDIT, &ret, FALSE);
						}

					}
					EndDialog(Dialog, 1);
					if (DlgTermFont != NULL) {
						DeleteObject(DlgTermFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgTermFont != NULL) {
						DeleteObject(DlgTermFont);
					}
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
					if (w==IdJIS) {
						EnableDlgItem(Dialog,IDC_TERMKANA,IDC_TERMKANA);
					}
					else {
						DisableDlgItem(Dialog,IDC_TERMKANA,IDC_TERMKANA);
						}
					break;

				case IDC_TERMKANJISEND:
					w = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND);
					if (w==IdJIS) {
						EnableDlgItem(Dialog,IDC_TERMKANASEND,IDC_TERMKOUT);
					}
					else {
						DisableDlgItem(Dialog,IDC_TERMKANASEND,IDC_TERMKOUT);
					}
					break;

				case IDC_TERMHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
					break;
			}
	}
	return FALSE;
}

void DispSample(HWND Dialog, PTTSet ts, int IAttr)
{
	int i,x,y;
	COLORREF Text, Back;
	int DX[3];
	TEXTMETRIC Metrics;
	RECT Rect, TestRect;
	int FW,FH;
	HDC DC;

	DC = GetDC(Dialog);
	Text = RGB(ts->TmpColor[IAttr][0],
	           ts->TmpColor[IAttr][1],
	           ts->TmpColor[IAttr][2]);
	Text = GetNearestColor(DC, Text);
	Back = RGB(ts->TmpColor[IAttr][3],
	           ts->TmpColor[IAttr][4],
	           ts->TmpColor[IAttr][5]);
	Back = GetNearestColor(DC, Back);
	SetTextColor(DC, Text);
	SetBkColor(DC, Back);
	SelectObject(DC,ts->SampleFont);
	GetTextMetrics(DC, &Metrics);
	FW = Metrics.tmAveCharWidth;
	FH = Metrics.tmHeight;
	for (i = 0 ; i <= 2 ; i++)
		DX[i] = FW;
	GetClientRect(Dialog,&Rect);
	TestRect.left = Rect.left + (int)((Rect.right-Rect.left)*0.65);
	TestRect.right = Rect.left + (int)((Rect.right-Rect.left)*0.93);
	TestRect.top = Rect.top + (int)((Rect.bottom-Rect.top)*0.54);
#ifdef USE_NORMAL_BGCOLOR
	TestRect.bottom = Rect.top + (int)((Rect.bottom-Rect.top)*0.90);
#else
	TestRect.bottom = Rect.top + (int)((Rect.bottom-Rect.top)*0.94);
#endif
	x = (int)((TestRect.left+TestRect.right) / 2 - FW * 1.5);
	y = (TestRect.top+TestRect.bottom-FH) / 2;
	ExtTextOut(DC, x,y, ETO_CLIPPED | ETO_OPAQUE,
	           &TestRect, "ABC", 3, &(DX[0]));
	ReleaseDC(Dialog,DC);
}

void ChangeColor(HWND Dialog, PTTSet ts, int IAttr, int IOffset)
{
	SetDlgItemInt(Dialog,IDC_WINRED,ts->TmpColor[IAttr][IOffset],FALSE);
	SetDlgItemInt(Dialog,IDC_WINGREEN,ts->TmpColor[IAttr][IOffset+1],FALSE);
	SetDlgItemInt(Dialog,IDC_WINBLUE,ts->TmpColor[IAttr][IOffset+2],FALSE);

	DispSample(Dialog,ts,IAttr);
}

void ChangeSB (HWND Dialog, PTTSet ts, int IAttr, int IOffset)
{
	HWND HRed, HGreen, HBlue;

	HRed = GetDlgItem(Dialog, IDC_WINREDBAR);
	HGreen = GetDlgItem(Dialog, IDC_WINGREENBAR);
	HBlue = GetDlgItem(Dialog, IDC_WINBLUEBAR);

	SetScrollPos(HRed,SB_CTL,ts->TmpColor[IAttr][IOffset+0],TRUE);
	SetScrollPos(HGreen,SB_CTL,ts->TmpColor[IAttr][IOffset+1],TRUE);
	SetScrollPos(HBlue,SB_CTL,ts->TmpColor[IAttr][IOffset+2],TRUE);
	ChangeColor(Dialog,ts,IAttr,IOffset);
}

void RestoreVar(HWND Dialog, PTTSet ts, int *IAttr, int *IOffset)
{
	WORD w;

	GetRB(Dialog,&w,IDC_WINTEXT,IDC_WINBACK);
	if (w==2) {
		*IOffset = 3;
	}
	else {
		*IOffset = 0;
	}
	if ((ts!=NULL) && (ts->VTFlag>0)) {
		*IAttr = GetCurSel(Dialog,IDC_WINATTR);
		if (*IAttr>0) (*IAttr)--;
	}
	else {
		*IAttr = 0;
	}
}

BOOL CALLBACK WinDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PTTSet ts;
	HWND Wnd, HRed, HGreen, HBlue;
	int IAttr, IOffset;
	WORD i, pos, ScrollCode, NewPos;
	HDC DC;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgWinFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_WINTITLELABEL, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINTITLE, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINCURSOR, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINBLOCK, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINVERT, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINHORZ, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_FONTBOLD, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINHIDETITLE, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINHIDEMENU, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINCOLOREMU, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINAIXTERM16, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINXTERM256, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINSCROLL1, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINSCROLL3, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINSCROLL2, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINCOLOR, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINTEXT, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINBACK, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINREV, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINATTRTEXT, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINATTR, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINREDLABEL, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINRED, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINGREENLABEL, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINGREEN, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINBLUELABEL, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINBLUE, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINUSENORMALBG, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINHELP, WM_SETFONT, (WPARAM)DlgWinFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgWinFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_WINTITLELABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_TITLELABEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINTITLELABEL, uimsg);
			GetDlgItemText(Dialog, IDC_WINCURSOR, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_CURSOR", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINCURSOR, uimsg);
			GetDlgItemText(Dialog, IDC_WINBLOCK, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_BLOCK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINBLOCK, uimsg);
			GetDlgItemText(Dialog, IDC_WINVERT, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_VERT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINVERT, uimsg);
			GetDlgItemText(Dialog, IDC_WINHORZ, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_HORZ", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINHORZ, uimsg);
			GetDlgItemText(Dialog, IDC_FONTBOLD, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_BOLDFONT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_FONTBOLD, uimsg);
			GetDlgItemText(Dialog, IDC_WINHIDETITLE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_HIDETITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINHIDETITLE, uimsg);
			GetDlgItemText(Dialog, IDC_WINHIDEMENU, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_HIDEMENU", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINHIDEMENU, uimsg);
			GetDlgItemText(Dialog, IDC_WINCOLOREMU, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_COLOREMU", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINCOLOREMU, uimsg);
			GetDlgItemText(Dialog, IDC_WINAIXTERM16, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_AIXTERM16", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINAIXTERM16, uimsg);
			GetDlgItemText(Dialog, IDC_WINXTERM256, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_XTERM256", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINXTERM256, uimsg);
			GetDlgItemText(Dialog, IDC_WINSCROLL1, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_SCROLL1", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINSCROLL1, uimsg);
			GetDlgItemText(Dialog, IDC_WINSCROLL3, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_SCROLL3", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINSCROLL3, uimsg);
			GetDlgItemText(Dialog, IDC_WINCOLOR, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_COLOR", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINCOLOR, uimsg);
			GetDlgItemText(Dialog, IDC_WINTEXT, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_TEXT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINTEXT, uimsg);
			GetDlgItemText(Dialog, IDC_WINBACK, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_BG", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINBACK, uimsg);
			GetDlgItemText(Dialog, IDC_WINATTRTEXT, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_ATTRIB", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINATTRTEXT, uimsg);
			GetDlgItemText(Dialog, IDC_WINREV, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_REVERSE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINREV, uimsg);
			GetDlgItemText(Dialog, IDC_WINREDLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_R", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINREDLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_WINGREENLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_G", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINGREENLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_WINBLUELABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_B", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINBLUELABEL, uimsg);
			GetDlgItemText(Dialog, IDC_WINUSENORMALBG, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WIN_ALWAYSBG", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINUSENORMALBG, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_WINHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINHELP, uimsg);

			SetDlgItemText(Dialog, IDC_WINTITLE, ts->Title);
			SendDlgItemMessage(Dialog, IDC_WINTITLE, EM_LIMITTEXT,
			                   sizeof(ts->Title)-1, 0);

			SetRB(Dialog,ts->HideTitle,IDC_WINHIDETITLE,IDC_WINHIDETITLE);
			SetRB(Dialog,ts->PopupMenu,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
			if ( ts->HideTitle>0 )
				DisableDlgItem(Dialog,IDC_WINHIDEMENU,IDC_WINHIDEMENU);

			if (ts->VTFlag>0) {
				get_lang_msg("DLG_WIN_PCBOLD16", uimsg, sizeof(uimsg), "&16 Colors (PC style)", UILanguageFile);
				SetDlgItemText(Dialog, IDC_WINCOLOREMU,uimsg);
				SetRB(Dialog, (ts->ColorFlag&CF_PCBOLD16)!=0, IDC_WINCOLOREMU, IDC_WINCOLOREMU);
				SetRB(Dialog, (ts->ColorFlag&CF_AIXTERM16)!=0, IDC_WINAIXTERM16, IDC_WINAIXTERM16);
				SetRB(Dialog, (ts->ColorFlag&CF_XTERM256)!=0,IDC_WINXTERM256,IDC_WINXTERM256);
				ShowDlgItem(Dialog,IDC_WINAIXTERM16,IDC_WINXTERM256);
				ShowDlgItem(Dialog,IDC_WINSCROLL1,IDC_WINSCROLL3);
				SetRB(Dialog,ts->EnableScrollBuff,IDC_WINSCROLL1,IDC_WINSCROLL1);
				SetDlgItemInt(Dialog,IDC_WINSCROLL2,ts->ScrollBuffSize,FALSE);

				SendDlgItemMessage(Dialog, IDC_WINSCROLL2, EM_LIMITTEXT, 6, 0);

				if ( ts->EnableScrollBuff==0 ) {
					DisableDlgItem(Dialog,IDC_WINSCROLL2,IDC_WINSCROLL3);
				}

				for (i = 0 ; i <= 1 ; i++) {
					if (ts->ColorFlag & CF_REVERSEVIDEO) {
						//
						// Reverse Videoモード(DECSCNMがon)の時
						//
						if (ts->ColorFlag & CF_REVERSECOLOR) {
							//
							// VTReverseColorが有効の時は
							// VTColorとVTReverseColorを入れ替える
							//
							ts->TmpColor[0][i*3]   = GetRValue(ts->VTReverseColor[i]);
							ts->TmpColor[0][i*3+1] = GetGValue(ts->VTReverseColor[i]);
							ts->TmpColor[0][i*3+2] = GetBValue(ts->VTReverseColor[i]);
							ts->TmpColor[3][i*3]   = GetRValue(ts->VTColor[i]);
							ts->TmpColor[3][i*3+1] = GetGValue(ts->VTColor[i]);
							ts->TmpColor[3][i*3+2] = GetBValue(ts->VTColor[i]);
						}
						else {
							//
							// VTReverseColorが無効の時は
							// ・VTColorの文字色と背景色を入れ替える
							// ・VTReverseColorはいじらない
							//
							ts->TmpColor[0][i*3]   = GetRValue(ts->VTColor[!i]);
							ts->TmpColor[0][i*3+1] = GetGValue(ts->VTColor[!i]);
							ts->TmpColor[0][i*3+2] = GetBValue(ts->VTColor[!i]);
						}
						//
						// 他の属性色は文字色と背景色を入れ替える
						//
						ts->TmpColor[1][i*3]   = GetRValue(ts->VTBoldColor[!i]);
						ts->TmpColor[1][i*3+1] = GetGValue(ts->VTBoldColor[!i]);
						ts->TmpColor[1][i*3+2] = GetBValue(ts->VTBoldColor[!i]);
						ts->TmpColor[2][i*3]   = GetRValue(ts->VTBlinkColor[!i]);
						ts->TmpColor[2][i*3+1] = GetGValue(ts->VTBlinkColor[!i]);
						ts->TmpColor[2][i*3+2] = GetBValue(ts->VTBlinkColor[!i]);
						ts->TmpColor[4][i*3]   = GetRValue(ts->URLColor[!i]);
						ts->TmpColor[4][i*3+1] = GetGValue(ts->URLColor[!i]);
						ts->TmpColor[4][i*3+2] = GetBValue(ts->URLColor[!i]);
					}
					else {
						ts->TmpColor[0][i*3]   = GetRValue(ts->VTColor[i]);
						ts->TmpColor[0][i*3+1] = GetGValue(ts->VTColor[i]);
						ts->TmpColor[0][i*3+2] = GetBValue(ts->VTColor[i]);
						ts->TmpColor[1][i*3]   = GetRValue(ts->VTBoldColor[i]);
						ts->TmpColor[1][i*3+1] = GetGValue(ts->VTBoldColor[i]);
						ts->TmpColor[1][i*3+2] = GetBValue(ts->VTBoldColor[i]);
						ts->TmpColor[2][i*3]   = GetRValue(ts->VTBlinkColor[i]);
						ts->TmpColor[2][i*3+1] = GetGValue(ts->VTBlinkColor[i]);
						ts->TmpColor[2][i*3+2] = GetBValue(ts->VTBlinkColor[i]);
						ts->TmpColor[3][i*3]   = GetRValue(ts->VTReverseColor[i]);
						ts->TmpColor[3][i*3+1] = GetGValue(ts->VTReverseColor[i]);
						ts->TmpColor[3][i*3+2] = GetBValue(ts->VTReverseColor[i]);
						/* begin - ishizaki */
						ts->TmpColor[4][i*3]   = GetRValue(ts->URLColor[i]);
						ts->TmpColor[4][i*3+1] = GetGValue(ts->URLColor[i]);
						ts->TmpColor[4][i*3+2] = GetBValue(ts->URLColor[i]);
						/* end - ishizaki */
					}
				}
				ShowDlgItem(Dialog,IDC_WINATTRTEXT,IDC_WINATTR);
				get_lang_msg("DLG_WIN_NORMAL", uimsg, sizeof(uimsg), "Normal", UILanguageFile);
				SendDlgItemMessage(Dialog, IDC_WINATTR, CB_ADDSTRING, 0, (LPARAM)uimsg);
				get_lang_msg("DLG_WIN_BOLD", uimsg, sizeof(uimsg), "Bold", UILanguageFile);
				SendDlgItemMessage(Dialog, IDC_WINATTR, CB_ADDSTRING, 0, (LPARAM)uimsg);
				get_lang_msg("DLG_WIN_BLINK", uimsg, sizeof(uimsg), "Blink", UILanguageFile);
				SendDlgItemMessage(Dialog, IDC_WINATTR, CB_ADDSTRING, 0, (LPARAM)uimsg);
				get_lang_msg("DLG_WIN_REVERSEATTR", uimsg, sizeof(uimsg), "Reverse", UILanguageFile);
				SendDlgItemMessage(Dialog, IDC_WINATTR, CB_ADDSTRING, 0, (LPARAM)uimsg);
				/* begin - ishizaki */
				SendDlgItemMessage(Dialog, IDC_WINATTR, CB_ADDSTRING, 0, (LPARAM)"URL");
				/* end - ishizaki */
				SendDlgItemMessage(Dialog, IDC_WINATTR, CB_SETCURSEL, 0,0);
#ifdef USE_NORMAL_BGCOLOR
				ShowDlgItem(Dialog,IDC_WINUSENORMALBG,IDC_WINUSENORMALBG);
				SetRB(Dialog,ts->UseNormalBGColor,IDC_WINUSENORMALBG,IDC_WINUSENORMALBG);
#endif
				ShowDlgItem(Dialog, IDC_FONTBOLD, IDC_FONTBOLD);
				SetRB(Dialog, ts->EnableBold, IDC_FONTBOLD,IDC_FONTBOLD);
			}
			else {
				for (i = 0 ; i <=1 ; i++) {
					ts->TmpColor[0][i*3]   = GetRValue(ts->TEKColor[i]);
					ts->TmpColor[0][i*3+1] = GetGValue(ts->TEKColor[i]);
					ts->TmpColor[0][i*3+2] = GetBValue(ts->TEKColor[i]);
				}
				SetRB(Dialog,ts->TEKColorEmu,IDC_WINCOLOREMU,IDC_WINCOLOREMU);
			}
			SetRB(Dialog,1,IDC_WINTEXT,IDC_WINBACK);

			SetRB(Dialog,ts->CursorShape,IDC_WINBLOCK,IDC_WINHORZ);

			IAttr = 0;
			IOffset = 0;

			HRed = GetDlgItem(Dialog, IDC_WINREDBAR);
			SetScrollRange(HRed,SB_CTL,0,255,TRUE);

			HGreen = GetDlgItem(Dialog, IDC_WINGREENBAR);
			SetScrollRange(HGreen,SB_CTL,0,255,TRUE);

			HBlue = GetDlgItem(Dialog, IDC_WINBLUEBAR);
			SetScrollRange(HBlue,SB_CTL,0,255,TRUE);

			ChangeSB(Dialog,ts,IAttr,IOffset);

			return TRUE;

		case WM_COMMAND:
			ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
			RestoreVar(Dialog,ts,&IAttr,&IOffset);
			switch (LOWORD(wParam)) {
				case IDOK:
					if ( ts!=NULL ) {
						GetDlgItemText(Dialog,IDC_WINTITLE,ts->Title,sizeof(ts->Title));
						GetRB(Dialog,&ts->HideTitle,IDC_WINHIDETITLE,IDC_WINHIDETITLE);
						GetRB(Dialog,&ts->PopupMenu,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
						DC = GetDC(Dialog);
						if (ts->VTFlag>0) {
							GetRB(Dialog,&i,IDC_WINCOLOREMU,IDC_WINCOLOREMU);
							if (i!=0) {
								ts->ColorFlag |= CF_PCBOLD16;
							}
							else {
								ts->ColorFlag &= ~(WORD)CF_PCBOLD16;
							}
							GetRB(Dialog,&i,IDC_WINAIXTERM16,IDC_WINAIXTERM16);
							if (i!=0) {
								ts->ColorFlag |= CF_AIXTERM16;
							}
							else {
								ts->ColorFlag &= ~(WORD)CF_AIXTERM16;
							}
							GetRB(Dialog,&i,IDC_WINXTERM256,IDC_WINXTERM256);
							if (i!=0) {
								ts->ColorFlag |= CF_XTERM256;
							}
							else {
								ts->ColorFlag &= ~(WORD)CF_XTERM256;
							}
							GetRB(Dialog,&ts->EnableScrollBuff,IDC_WINSCROLL1,IDC_WINSCROLL1);
							if ( ts->EnableScrollBuff>0 ) {
								ts->ScrollBuffSize =
									GetDlgItemInt(Dialog,IDC_WINSCROLL2,NULL,FALSE);
							}
							for (i = 0 ; i <= 1 ; i++) {
								if (ts->ColorFlag & CF_REVERSEVIDEO) {
									//
									// Reverse Videoモード(DECSCNMがon)の時
									//
									if (ts->ColorFlag & CF_REVERSECOLOR) {
										//
										// VTReverseColorが有効の時は
										// VTColorとVTReverseColorを入れ替える
										//
										ts->VTReverseColor[i] =
											RGB(ts->TmpColor[0][i*3],
											    ts->TmpColor[0][i*3+1],
											    ts->TmpColor[0][i*3+2]);
										ts->VTColor[i] =
											RGB(ts->TmpColor[3][i*3],
											    ts->TmpColor[3][i*3+1],
											    ts->TmpColor[3][i*3+2]);
									}
									else {
										//
										// VTReverseColorが無効の時は
										// ・VTColorの文字色と背景色を入れ替える
										// ・VTReverseColorはいじらない
										//
										ts->VTColor[i] =
											RGB(ts->TmpColor[0][(!i)*3],
											    ts->TmpColor[0][(!i)*3+1],
											    ts->TmpColor[0][(!i)*3+2]);
									}
									//
									// ・他の属性色は文字色と背景色を入れ替える
									//
									ts->VTBoldColor[i] =
										RGB(ts->TmpColor[1][(!i)*3],
										    ts->TmpColor[1][(!i)*3+1],
										    ts->TmpColor[1][(!i)*3+2]);
									ts->VTBlinkColor[i] =
										RGB(ts->TmpColor[2][(!i)*3],
										    ts->TmpColor[2][(!i)*3+1],
										    ts->TmpColor[2][(!i)*3+2]);
									ts->URLColor[i] =
										RGB(ts->TmpColor[4][(!i)*3],
										    ts->TmpColor[4][(!i)*3+1],
										    ts->TmpColor[4][(!i)*3+2]);
								}
								else {
									ts->VTColor[i] =
										RGB(ts->TmpColor[0][i*3],
										    ts->TmpColor[0][i*3+1],
										    ts->TmpColor[0][i*3+2]);
									ts->VTBoldColor[i] =
										RGB(ts->TmpColor[1][i*3],
										    ts->TmpColor[1][i*3+1],
										    ts->TmpColor[1][i*3+2]);
									ts->VTBlinkColor[i] =
										RGB(ts->TmpColor[2][i*3],
										    ts->TmpColor[2][i*3+1],
										    ts->TmpColor[2][i*3+2]);
									ts->VTReverseColor[i] =
										RGB(ts->TmpColor[3][i*3],
										    ts->TmpColor[3][i*3+1],
										    ts->TmpColor[3][i*3+2]);
									/* begin - ishizaki */
									ts->URLColor[i] =
										RGB(ts->TmpColor[4][i*3],
										    ts->TmpColor[4][i*3+1],
										    ts->TmpColor[4][i*3+2]);
									/* end - ishizaki */
								}
								ts->VTColor[i] = GetNearestColor(DC,ts->VTColor[i]);
								ts->VTBoldColor[i] = GetNearestColor(DC,ts->VTBoldColor[i]);
								ts->VTBlinkColor[i] = GetNearestColor(DC,ts->VTBlinkColor[i]);
								ts->VTReverseColor[i] = GetNearestColor(DC,ts->VTReverseColor[i]);
								/* begin - ishizaki */
								ts->URLColor[i] = GetNearestColor(DC,ts->URLColor[i]);
								/* end - ishizaki */
							}
#ifdef USE_NORMAL_BGCOLOR
							GetRB(Dialog,&ts->UseNormalBGColor,
							      IDC_WINUSENORMALBG,IDC_WINUSENORMALBG);
							// 2006/03/11 by 337
							if (ts->ColorFlag & CF_REVERSEVIDEO) {
								if (ts->UseNormalBGColor) {
									//
									// Reverse Videoモード(DECSCNMがon)の時
									//
									if (ts->ColorFlag & CF_REVERSECOLOR) {
										//
										// VTReverseColorが有効の時は
										// 文字色を反転背景色に合わせる
										//
										ts->VTBoldColor[0] =
										ts->VTBlinkColor[0] =
										ts->URLColor[0] =
											ts->VTReverseColor[1];
									}
									else {
										//
										// VTReverseColorが無効の時は
										// 文字色を通常文字色に合わせる
										//
										ts->VTBoldColor[0] =
										ts->VTBlinkColor[0] =
										ts->URLColor[0] =
											ts->VTColor[0];
									}
								}
							}
							else {
								if (ts->UseNormalBGColor) {
									ts->VTBoldColor[1] =
									ts->VTBlinkColor[1] =
									ts->URLColor[1] =
										ts->VTColor[1];
								}
							}
#endif
							GetRB(Dialog,&ts->EnableBold,IDC_FONTBOLD,IDC_FONTBOLD);
						}
						else {
							for (i = 0 ; i <= 1 ; i++) {
								ts->TEKColor[i] =
									RGB(ts->TmpColor[0][i*3],
									    ts->TmpColor[0][i*3+1],
									    ts->TmpColor[0][i*3+2]);
								ts->TEKColor[i] = GetNearestColor(DC,ts->TEKColor[i]);
							}
							GetRB(Dialog,&ts->TEKColorEmu,IDC_WINCOLOREMU,IDC_WINCOLOREMU);
						}
						ReleaseDC(Dialog,DC);

						GetRB(Dialog,&ts->CursorShape,IDC_WINBLOCK,IDC_WINHORZ);
					}
					EndDialog(Dialog, 1);
					if (DlgWinFont != NULL) {
						DeleteObject(DlgWinFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgWinFont != NULL) {
						DeleteObject(DlgWinFont);
					}
					return TRUE;

				case IDC_WINHIDETITLE:
					GetRB(Dialog,&i,IDC_WINHIDETITLE,IDC_WINHIDETITLE);
					if (i>0) {
						DisableDlgItem(Dialog,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
					}
					else {
						EnableDlgItem(Dialog,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
					}
					break;

				case IDC_WINSCROLL1:
					if ( ts==NULL ) {
						return TRUE;
					}
					GetRB(Dialog,&i,IDC_WINSCROLL1,IDC_WINSCROLL1);
					if ( i>0 ) {
						EnableDlgItem(Dialog,IDC_WINSCROLL2,IDC_WINSCROLL3);
					}
					else {
						DisableDlgItem(Dialog,IDC_WINSCROLL2,IDC_WINSCROLL3);
					}
					break;

				case IDC_WINTEXT:
					if ( ts==NULL ) {
						return TRUE;
					}
					IOffset = 0;
					ChangeSB(Dialog,ts,IAttr,IOffset);
					break;

				case IDC_WINBACK:
					if ( ts==NULL ) {
						return TRUE;
					}
					IOffset = 3;
					ChangeSB(Dialog,ts,IAttr,IOffset);
					break;

				case IDC_WINREV:
					if ( ts==NULL ) {
						return TRUE;
					}
					i = ts->TmpColor[IAttr][0];
					ts->TmpColor[IAttr][0] = ts->TmpColor[IAttr][3];
					ts->TmpColor[IAttr][3] = i;
					i = ts->TmpColor[IAttr][1];
					ts->TmpColor[IAttr][1] = ts->TmpColor[IAttr][4];
					ts->TmpColor[IAttr][4] = i;
					i = ts->TmpColor[IAttr][2];
					ts->TmpColor[IAttr][2] = ts->TmpColor[IAttr][5];
					ts->TmpColor[IAttr][5] = i;

					ChangeSB(Dialog,ts,IAttr,IOffset);
					break;

				case IDC_WINATTR:
					if ( ts!=NULL ) {
						ChangeSB(Dialog,ts,IAttr,IOffset);
					}
					break;

				case IDC_WINHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
					break;
			}
			break;

		case WM_PAINT:
			ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
			if ( ts==NULL ) {
				return TRUE;
			}
			RestoreVar(Dialog,ts,&IAttr,&IOffset);
			DispSample(Dialog,ts,IAttr);
			break;

		case WM_HSCROLL:
			ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
			if (ts==NULL) {
				return TRUE;
			}
			RestoreVar(Dialog,ts,&IAttr,&IOffset);
			HRed = GetDlgItem(Dialog, IDC_WINREDBAR);
			HGreen = GetDlgItem(Dialog, IDC_WINGREENBAR);
			HBlue = GetDlgItem(Dialog, IDC_WINBLUEBAR);
			Wnd = (HWND)lParam;
			ScrollCode = LOWORD(wParam);
			NewPos = HIWORD(wParam);
			if ( Wnd == HRed ) {
				i = IOffset;
			}
			else if ( Wnd == HGreen ) {
				i = IOffset + 1;
			}
			else if ( Wnd == HBlue ) {
				i = IOffset + 2;
			}
			else {
				return TRUE;
			}
			pos = ts->TmpColor[IAttr][i];
			switch (ScrollCode) {
				case SB_BOTTOM:
					pos = 255;
					break;
				case SB_LINEDOWN:
					if (pos<255) {
						pos++;
					}
					break;
				case SB_LINEUP:
					if (pos>0) {
						pos--;
					}
					break;
				case SB_PAGEDOWN:
					pos += 16;
					break;
				case SB_PAGEUP:
					if (pos < 16) {
						pos = 0;
					}
					else {
						pos -= 16;
					}
					break;
				case SB_THUMBPOSITION:
					pos = NewPos;
					break;
				case SB_THUMBTRACK:
					pos = NewPos;
					break;
				case SB_TOP:
					pos = 0;
					break;
				default:
					return TRUE;
			}
			if (pos > 255) {
				pos = 255;
			}
			ts->TmpColor[IAttr][i] = pos;
			SetScrollPos(Wnd,SB_CTL,pos,TRUE);
			ChangeColor(Dialog,ts,IAttr,IOffset);
			return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK KeybDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PTTSet ts;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgKeybFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_KEYBTRANS, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBBS, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBDEL, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBKEYBTEXT, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBKEYB, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBMETA, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBDISABLE, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBAPPKEY, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBAPPCUR, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_KEYBHELP, WM_SETFONT, (WPARAM)DlgKeybFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgKeybFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBTRANS, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_TRANSMIT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBTRANS, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBBS, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_BS", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBBS, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBDEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_DEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBDEL, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBKEYBTEXT, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_KEYB", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBKEYBTEXT, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBMETA, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_META", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBMETA, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBDISABLE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_DISABLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBDISABLE, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBAPPKEY, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_APPKEY", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBAPPKEY, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBAPPCUR, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_KEYB_APPCUR", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBAPPCUR, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_KEYBHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_KEYBHELP, uimsg);

			SetRB(Dialog,ts->BSKey-1,IDC_KEYBBS,IDC_KEYBBS);
			SetRB(Dialog,ts->DelKey,IDC_KEYBDEL,IDC_KEYBDEL);
			SetRB(Dialog,ts->MetaKey,IDC_KEYBMETA,IDC_KEYBMETA);
			SetRB(Dialog,ts->DisableAppKeypad,IDC_KEYBAPPKEY,IDC_KEYBAPPKEY);
			SetRB(Dialog,ts->DisableAppCursor,IDC_KEYBAPPCUR,IDC_KEYBAPPCUR);
			if (ts->Language==IdRussian) {
				ShowDlgItem(Dialog,IDC_KEYBKEYBTEXT,IDC_KEYBKEYB);
				SetDropDownList(Dialog, IDC_KEYBKEYB, RussList2, ts->RussKeyb);
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
					if ( ts!=NULL ) {
						GetRB(Dialog,&ts->BSKey,IDC_KEYBBS,IDC_KEYBBS);
						ts->BSKey++;
						GetRB(Dialog,&ts->DelKey,IDC_KEYBDEL,IDC_KEYBDEL);
						GetRB(Dialog,&ts->MetaKey,IDC_KEYBMETA,IDC_KEYBMETA);
						GetRB(Dialog,&ts->DisableAppKeypad,IDC_KEYBAPPKEY,IDC_KEYBAPPKEY);
						GetRB(Dialog,&ts->DisableAppCursor,IDC_KEYBAPPCUR,IDC_KEYBAPPCUR);
						if (ts->Language==IdRussian)
							ts->RussKeyb = (WORD)GetCurSel(Dialog, IDC_KEYBKEYB);
					}
					EndDialog(Dialog, 1);
					if (DlgKeybFont != NULL) {
						DeleteObject(DlgKeybFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgKeybFont != NULL) {
						DeleteObject(DlgKeybFont);
					}
					return TRUE;

				case IDC_KEYBHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

static PCHAR far DataList[] = {"7 bit","8 bit",NULL};
static PCHAR far ParityList[] = {"even","odd","none",NULL};
static PCHAR far StopList[] = {"1 bit","2 bit",NULL};
static PCHAR far FlowList[] = {"Xon/Xoff","hardware","none",NULL};

BOOL CALLBACK SerialDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PTTSet ts;
	int i, w;
	char Temp[128];
	WORD ComPortTable[MAXCOMPORT];
	static char *ComPortDesc[MAXCOMPORT];
	int comports;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgSerialFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_SERIALPORT_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALPORT, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALBAUD_LEBAL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALBAUD, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDATA_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDATA, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALPARITY_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALPARITY, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALSTOP_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALSTOP, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALFLOW_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALFLOW, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDELAY, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDELAYCHAR_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDELAYCHAR, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDELAYLINE_LABEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALDELAYLINE, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_SERIALHELP, WM_SETFONT, (WPARAM)DlgSerialFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgSerialFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALPORT_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_PORT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALPORT_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALBAUD_LEBAL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_BAUD", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALBAUD_LEBAL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALDATA_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_DATA", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALDATA_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALPARITY_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_PARITY", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALPARITY_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALSTOP_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_STOP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALSTOP_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALFLOW_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_FLOW", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALFLOW_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALDELAY, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_DELAY", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALDELAY, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALDELAYCHAR_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_DELAYCHAR", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALDELAYCHAR_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALDELAYLINE_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_SERIAL_DELAYLINE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALDELAYLINE_LABEL, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_SERIALHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_SERIALHELP, uimsg);

			w = 0;

			if ((comports = DetectComPorts(ComPortTable, ts->MaxComPort, ComPortDesc)) > 0) {
				for (i=0; i<comports; i++) {
					// MaxComPort を越えるポートは表示しない
					if (ComPortTable[i] > ts->MaxComPort) {
						continue;
					}

					_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "COM%d", ComPortTable[i]);
// Serial dialogはドロップダウンリストの幅が大きくできないので、Descriptionはなしとする。
#if 0
					strncat_s(Temp, sizeof(Temp), ": ", _TRUNCATE);
					strncat_s(Temp, sizeof(Temp), ComPortDesc[i], _TRUNCATE);
#endif
					SendDlgItemMessage(Dialog, IDC_SERIALPORT, CB_ADDSTRING,
					                   0, (LPARAM)Temp);
					if (ComPortTable[i] == ts->ComPort) {
						w = i;
					}
				}
			} else if (comports == 0) {
				DisableDlgItem(Dialog, IDC_SERIALPORT, IDC_SERIALPORT);
				DisableDlgItem(Dialog, IDC_SERIALPORT_LABEL, IDC_SERIALPORT_LABEL);
			} else {
				for (i=1; i<=ts->MaxComPort; i++) {
					_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "COM%d", i);
					SendDlgItemMessage(Dialog, IDC_SERIALPORT, CB_ADDSTRING,
					                   0, (LPARAM)Temp);
				}
				if (ts->ComPort<=ts->MaxComPort) {
					w = ts->ComPort-1;
				}

			}
			SendDlgItemMessage(Dialog, IDC_SERIALPORT, CB_SETCURSEL, w, 0);

			SetDropDownList(Dialog, IDC_SERIALBAUD, BaudList, ts->Baud);
			SetDropDownList(Dialog, IDC_SERIALDATA, DataList, ts->DataBit);
			SetDropDownList(Dialog, IDC_SERIALPARITY, ParityList, ts->Parity);
			SetDropDownList(Dialog, IDC_SERIALSTOP, StopList, ts->StopBit);
			SetDropDownList(Dialog, IDC_SERIALFLOW, FlowList, ts->Flow);

			SetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,ts->DelayPerChar,FALSE);
			SendDlgItemMessage(Dialog, IDC_SERIALDELAYCHAR, EM_LIMITTEXT,4, 0);

			SetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,ts->DelayPerLine,FALSE);
			SendDlgItemMessage(Dialog, IDC_SERIALDELAYLINE, EM_LIMITTEXT,4, 0);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
					if ( ts!=NULL ) {
						memset(Temp, 0, sizeof(Temp));
						GetDlgItemText(Dialog, IDC_SERIALPORT, Temp, sizeof(Temp)-1);
						if (strncmp(Temp, "COM", 3) == 0 && Temp[3] != '\0') {
							ts->ComPort = (WORD)atoi(&Temp[3]);
						} else {
							ts->ComPort = 0;
						}

						ts->Baud = (WORD)GetCurSel(Dialog, IDC_SERIALBAUD);
						ts->DataBit = (WORD)GetCurSel(Dialog, IDC_SERIALDATA);
						ts->Parity = (WORD)GetCurSel(Dialog, IDC_SERIALPARITY);
						ts->StopBit = (WORD)GetCurSel(Dialog, IDC_SERIALSTOP);
						ts->Flow = (WORD)GetCurSel(Dialog, IDC_SERIALFLOW);

						ts->DelayPerChar = GetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,NULL,FALSE);

						ts->DelayPerLine = GetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,NULL,FALSE);

						ts->PortType = IdSerial;

						// ボーレートが変更されることがあるので、タイトル再表示の
						// メッセージを飛ばすようにした。 (2007.7.21 maya)
						PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);
					}

					EndDialog(Dialog, 1);
					if (DlgSerialFont != NULL) {
						DeleteObject(DlgSerialFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgSerialFont != NULL) {
						DeleteObject(DlgSerialFont);
					}
					return TRUE;

				case IDC_SERIALHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

BOOL CALLBACK TCPIPDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PTTSet ts;
	char EntName[10];
	char TempHost[HostNameMaxLength+1];
	UINT i, Index;
	WORD w;
	BOOL Ok;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgTcpipFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_TCPIPHOSTLIST, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPHOST, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPADD, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPLIST, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPUP, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPREMOVE, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPDOWN, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPTELNETKEEPALIVELABEL, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPTELNETKEEPALIVE, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPTELNETKEEPALIVESEC, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPHISTORY, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPAUTOCLOSE, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPPORTLABEL, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPPORT, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPTELNET, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPTERMTYPELABEL, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPTERMTYPE, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TCPIPHELP, WM_SETFONT, (WPARAM)DlgTcpipFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgTcpipFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPHOSTLIST, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_HOSTLIST", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPHOSTLIST, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPADD, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_ADD", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPADD, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPUP, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_UP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPUP, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPREMOVE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_REMOVE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPREMOVE, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPDOWN, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_DOWN", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPDOWN, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPHISTORY, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_HISTORY", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPHISTORY, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPAUTOCLOSE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_AUTOCLOSE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPAUTOCLOSE, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPPORTLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_PORT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPPORTLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPTELNET, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_TELNET", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPTELNET, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPTELNETKEEPALIVELABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_KEEPALIVE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPTELNETKEEPALIVELABEL, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPTELNETKEEPALIVESEC, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_KEEPALIVE_SEC", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPTELNETKEEPALIVESEC, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPTERMTYPELABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_TCPIP_TERMTYPE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPTERMTYPELABEL, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_TCPIPHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_TCPIPHELP, uimsg);

			SendDlgItemMessage(Dialog, IDC_TCPIPHOST, EM_LIMITTEXT,
			                   HostNameMaxLength-1, 0);

			i = 1;
			do {
				_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "Host%d", i);
				GetPrivateProfileString("Hosts",EntName,"",
				                        TempHost,sizeof(TempHost),ts->SetupFName);
				if (strlen(TempHost) > 0) {
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_ADDSTRING,
					                   0, (LPARAM)TempHost);
				}
				i++;
			} while ((i <= MAXHOSTLIST) && (strlen(TempHost)>0));

			/* append a blank item to the bottom */
			TempHost[0] = 0;
			SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_ADDSTRING, 0, (LPARAM)TempHost);
			SetRB(Dialog,ts->HistoryList,IDC_TCPIPHISTORY,IDC_TCPIPHISTORY);
			SetRB(Dialog,ts->AutoWinClose,IDC_TCPIPAUTOCLOSE,IDC_TCPIPAUTOCLOSE);
			SetDlgItemInt(Dialog,IDC_TCPIPPORT,ts->TCPPort,FALSE);
			SetDlgItemInt(Dialog,IDC_TCPIPTELNETKEEPALIVE,ts->TelKeepAliveInterval,FALSE);
			SetRB(Dialog,ts->Telnet,IDC_TCPIPTELNET,IDC_TCPIPTELNET);
			SetDlgItemText(Dialog, IDC_TCPIPTERMTYPE, ts->TermType);
			SendDlgItemMessage(Dialog, IDC_TCPIPTERMTYPE, EM_LIMITTEXT, sizeof(ts->TermType)-1, 0);

			// SSH接続のときにも TERM を送るので、telnetが無効でも disabled にしない。(2005.11.3 yutaka)
			EnableDlgItem(Dialog,IDC_TCPIPTERMTYPELABEL,IDC_TCPIPTERMTYPE);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
					if (ts!=NULL) {
						WritePrivateProfileString("Hosts",NULL,NULL,ts->SetupFName);

						Index = SendDlgItemMessage(Dialog,IDC_TCPIPLIST,LB_GETCOUNT,0,0);
						if (Index==(UINT)LB_ERR) {
							Index = 0;
						}
						else {
							Index--;
						}
						if (Index>MAXHOSTLIST) {
							Index = MAXHOSTLIST;
						}
						for (i = 1 ; i <= Index ; i++) {
							SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETTEXT,
							                   i-1, (LPARAM)TempHost);
							_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "Host%i", i);
							WritePrivateProfileString("Hosts",EntName,TempHost,ts->SetupFName);
						}
						GetRB(Dialog,&ts->HistoryList,IDC_TCPIPHISTORY,IDC_TCPIPHISTORY);
						GetRB(Dialog,&ts->AutoWinClose,IDC_TCPIPAUTOCLOSE,IDC_TCPIPAUTOCLOSE);
						ts->TCPPort = GetDlgItemInt(Dialog,IDC_TCPIPPORT,&Ok,FALSE);
						if (! Ok) {
							ts->TCPPort = ts->TelPort;
						}
						ts->TelKeepAliveInterval = GetDlgItemInt(Dialog,IDC_TCPIPTELNETKEEPALIVE,&Ok,FALSE);
						GetRB(Dialog,&ts->Telnet,IDC_TCPIPTELNET,IDC_TCPIPTELNET);
						GetDlgItemText(Dialog, IDC_TCPIPTERMTYPE, ts->TermType,
						               sizeof(ts->TermType));
					}
					EndDialog(Dialog, 1);
					if (DlgTcpipFont != NULL) {
						DeleteObject(DlgTcpipFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgTcpipFont != NULL) {
						DeleteObject(DlgTcpipFont);
					}
					return TRUE;

				case IDC_TCPIPHOST:
					if (HIWORD(wParam)==EN_CHANGE) {
						GetDlgItemText(Dialog, IDC_TCPIPHOST, TempHost, sizeof(TempHost));
						if (strlen(TempHost)==0) {
							DisableDlgItem(Dialog,IDC_TCPIPADD,IDC_TCPIPADD);
						}
						else {
							EnableDlgItem(Dialog,IDC_TCPIPADD,IDC_TCPIPADD);
						}
					}
					break;

				case IDC_TCPIPADD:
					GetDlgItemText(Dialog, IDC_TCPIPHOST, TempHost, sizeof(TempHost));
					if (strlen(TempHost)>0) {
						Index = SendDlgItemMessage(Dialog,IDC_TCPIPLIST,LB_GETCURSEL,0,0);
						if (Index==(UINT)LB_ERR) {
							Index = 0;
						}

						SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_INSERTSTRING,
						                   Index, (LPARAM)TempHost);

						SetDlgItemText(Dialog, IDC_TCPIPHOST, 0);
						SetFocus(GetDlgItem(Dialog, IDC_TCPIPHOST));
					}
					break;

				case IDC_TCPIPLIST:
					if (HIWORD(wParam)==LBN_SELCHANGE) {
						i = SendDlgItemMessage(Dialog,IDC_TCPIPLIST,LB_GETCOUNT,0,0);
						Index = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
						if ((i<=1) || (Index==(UINT)LB_ERR) || (Index==i-1)) {
							DisableDlgItem(Dialog,IDC_TCPIPUP,IDC_TCPIPDOWN);
						}
						else {
							EnableDlgItem(Dialog,IDC_TCPIPREMOVE,IDC_TCPIPREMOVE);
							if (Index==0) {
								DisableDlgItem(Dialog,IDC_TCPIPUP,IDC_TCPIPUP);
							}
							else {
								EnableDlgItem(Dialog,IDC_TCPIPUP,IDC_TCPIPUP);
							}
							if (Index>=i-2) {
								DisableDlgItem(Dialog,IDC_TCPIPDOWN,IDC_TCPIPDOWN);
							}
							else {
								EnableDlgItem(Dialog,IDC_TCPIPDOWN,IDC_TCPIPDOWN);
							}
						}
					}
					break;

				case IDC_TCPIPUP:
				case IDC_TCPIPDOWN:
					i = SendDlgItemMessage(Dialog,IDC_TCPIPLIST,LB_GETCOUNT,0,0);
					Index = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
					if (Index==(UINT)LB_ERR) {
						return TRUE;
					}
					if (LOWORD(wParam)==IDC_TCPIPDOWN) {
						Index++;
					}
					if ((Index==0) || (Index>=i-1)) {
						return TRUE;
					}
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETTEXT,
					                   Index, (LPARAM)TempHost);
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_DELETESTRING,
					                   Index, 0);
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_INSERTSTRING,
					                   Index-1, (LPARAM)TempHost);
					if (LOWORD(wParam)==IDC_TCPIPUP) {
						Index--;
					}
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_SETCURSEL,Index,0);
					if (Index==0) {
						DisableDlgItem(Dialog,IDC_TCPIPUP,IDC_TCPIPUP);
					}
					else {
						EnableDlgItem(Dialog,IDC_TCPIPUP,IDC_TCPIPUP);
					}
					if (Index>=i-2) {
						DisableDlgItem(Dialog,IDC_TCPIPDOWN,IDC_TCPIPDOWN);
					}
					else {
						EnableDlgItem(Dialog,IDC_TCPIPDOWN,IDC_TCPIPDOWN);
					}
					SetFocus(GetDlgItem(Dialog, IDC_TCPIPLIST));
					break;

				case IDC_TCPIPREMOVE:
					i = SendDlgItemMessage(Dialog,IDC_TCPIPLIST,LB_GETCOUNT,0,0);
					Index = SendDlgItemMessage(Dialog,IDC_TCPIPLIST,LB_GETCURSEL, 0, 0);
					if ((Index==(UINT)LB_ERR) ||
						(Index==i-1)) {
						return TRUE;
					}
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETTEXT,
					                   Index, (LPARAM)TempHost);
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_DELETESTRING,
					                   Index, 0);
					SetDlgItemText(Dialog, IDC_TCPIPHOST, TempHost);
					DisableDlgItem(Dialog,IDC_TCPIPUP,IDC_TCPIPDOWN);
					SetFocus(GetDlgItem(Dialog, IDC_TCPIPHOST));
					break;

				case IDC_TCPIPTELNET:
					GetRB(Dialog,&w,IDC_TCPIPTELNET,IDC_TCPIPTELNET);
					if (w==1) {
						EnableDlgItem(Dialog,IDC_TCPIPTERMTYPELABEL,IDC_TCPIPTERMTYPE);
						ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
						if (ts!=NULL) {
							SetDlgItemInt(Dialog,IDC_TCPIPPORT,ts->TelPort,FALSE);
						}
					}
					else {
						// SSH接続のときにも TERM を送るので、telnetが無効でも disabled にしない。(2005.11.3 yutaka)
						EnableDlgItem(Dialog,IDC_TCPIPTERMTYPELABEL,IDC_TCPIPTERMTYPE);
					}
					break;

				case IDC_TCPIPHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
					break;
			}
	}
	return FALSE;
}

// C-p/C-n/C-b/C-f/C-a/C-e をサポート (2007.9.5 maya)
// C-d/C-k をサポート (2007.10.3 yutaka)
// ドロップダウンの中のエディットコントロールを
// サブクラス化するためのウインドウプロシージャ
WNDPROC OrigHostnameEditProc; // Original window procedure
LRESULT CALLBACK HostnameEditProc(HWND dlg, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
	HWND parent;
	int  max, select, len;
	char *str, *orgstr;

	switch (msg) {
		// キーが押されたのを検知する
		case WM_KEYDOWN:
			if (GetKeyState(VK_CONTROL) < 0) {
				switch (wParam) {
					case 0x50: // Ctrl+p ... up
						parent = GetParent(dlg);
						select = SendMessage(parent, CB_GETCURSEL, 0, 0);
						if (select > 0) {
							PostMessage(parent, CB_SETCURSEL, select - 1, 0);
						}
						return 0;
					case 0x4e: // Ctrl+n ... down
						parent = GetParent(dlg);
						max = SendMessage(parent, CB_GETCOUNT, 0, 0);
						select = SendMessage(parent, CB_GETCURSEL, 0, 0);
						if (select < max - 1) {
							PostMessage(parent, CB_SETCURSEL, select + 1, 0);
						}
						return 0;
					case 0x42: // Ctrl+b ... left
						SendMessage(dlg, EM_GETSEL, 0, (LPARAM)&select);
						PostMessage(dlg, EM_SETSEL, select-1, select-1);
						return 0;
					case 0x46: // Ctrl+f ... right
						SendMessage(dlg, EM_GETSEL, 0, (LPARAM)&select);
						max = GetWindowTextLength(dlg) ;
						PostMessage(dlg, EM_SETSEL, select+1, select+1);
						return 0;
					case 0x41: // Ctrl+a ... home
						PostMessage(dlg, EM_SETSEL, 0, 0);
						return 0;
					case 0x45: // Ctrl+e ... end
						max = GetWindowTextLength(dlg) ;
						PostMessage(dlg, EM_SETSEL, max, max);
						return 0;

					case 0x44: // Ctrl+d
					case 0x4b: // Ctrl+k
					case 0x55: // Ctrl+u
						SendMessage(dlg, EM_GETSEL, 0, (LPARAM)&select);
						max = GetWindowTextLength(dlg);
						max++; // '\0'
						orgstr = str = malloc(max);
						if (str != NULL) {
							len = GetWindowText(dlg, str, max);
							if (select >= 0 && select < len) {
								if (wParam == 0x44) { // カーソル配下の文字のみを削除する
									memmove(&str[select], &str[select + 1], len - select - 1);
									str[len - 1] = '\0';

								} else if (wParam == 0x4b) { // カーソルから行末まで削除する
									str[select] = '\0';

								}
							} 

							if (wParam == 0x55) { // カーソルより左側をすべて消す
								if (select >= len) {
									str[0] = '\0';
								} else {
									str = &str[select];
								}
								select = 0;
							}

							SetWindowText(dlg, str);
							SendMessage(dlg, EM_SETSEL, select, select);
							free(orgstr);
							return 0;
						}
						break;
				}
			}
			break;

		// 上のキーを押した結果送られる文字で音が鳴るので捨てる
		case WM_CHAR:
			switch (wParam) {
				case 0x01:
				case 0x02:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x0b:
				case 0x0e:
				case 0x10:
				case 0x15:
					return 0;
			}
	}

	return CallWindowProc(OrigHostnameEditProc, dlg, msg, wParam, lParam);
}

BOOL CALLBACK HostDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PGetHNRec GetHNRec;
	char EntName[128];
	char TempHost[HostNameMaxLength+1];
	WORD i, j, w;
	BOOL Ok;
	WORD ComPortTable[MAXCOMPORT];
	static char *ComPortDesc[MAXCOMPORT];
	int comports;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;
	static HWND hwndHostname     = NULL; // HOSTNAME dropdown
	static HWND hwndHostnameEdit = NULL; // Edit control on HOSTNAME dropdown

	switch (Message) {
		case WM_INITDIALOG:
			GetHNRec = (PGetHNRec)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgHostFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_HOSTTCPIP, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTNAMELABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTNAME, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTTELNET, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPORTLABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPORT, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOLLABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTSERIAL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTCOMLABEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTCOM, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_HOSTHELP, WM_SETFONT, (WPARAM)DlgHostFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgHostFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_HOST_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_HOSTNAMELABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_HOST_TCPIPHOST", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_HOSTNAMELABEL, uimsg);
			GetDlgItemText(Dialog, IDC_HOSTTCPPORTLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_HOST_TCPIPPORT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_HOSTTCPPORTLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_HOSTTCPPROTOCOLLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_HOST_TCPIPPROTOCOL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_HOSTTCPPROTOCOLLABEL, uimsg);
			GetDlgItemText(Dialog, IDC_HOSTSERIAL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_HOST_SERIAL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_HOSTSERIAL, uimsg);
			GetDlgItemText(Dialog, IDC_HOSTCOMLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_HOST_SERIALPORT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_HOSTCOMLABEL, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_HOSTHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_HOSTHELP, uimsg);

			if ( GetHNRec->PortType==IdFile ) {
				GetHNRec->PortType = IdTCPIP;
			}

			i = 1;
			do {
				_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "Host%d", i);
				GetPrivateProfileString("Hosts",EntName,"",
				                        TempHost,sizeof(TempHost),GetHNRec->SetupFN);
				if ( strlen(TempHost) > 0 ) {
					SendDlgItemMessage(Dialog, IDC_HOSTNAME, CB_ADDSTRING,
					                   0, (LPARAM)TempHost);
				}
				i++;
			} while ((i <= MAXHOSTLIST) && (strlen(TempHost)>0));

			SendDlgItemMessage(Dialog, IDC_HOSTNAME, EM_LIMITTEXT,
			                   HostNameMaxLength-1, 0);

			SendDlgItemMessage(Dialog, IDC_HOSTNAME, CB_SETCURSEL,0,0);

			// C-n/C-p のためにサブクラス化 (2007.9.4 maya)
			hwndHostname = GetDlgItem(Dialog, IDC_HOSTNAME);
			hwndHostnameEdit = GetWindow(hwndHostname, GW_CHILD);
			OrigHostnameEditProc = (WNDPROC)GetWindowLong(hwndHostnameEdit, GWL_WNDPROC);
			SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)HostnameEditProc);

			SetRB(Dialog,GetHNRec->Telnet,IDC_HOSTTELNET,IDC_HOSTTELNET);
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPORT, EM_LIMITTEXT,5,0);
			SetDlgItemInt(Dialog,IDC_HOSTTCPPORT,GetHNRec->TCPPort,FALSE);
#ifndef NO_INET6
			for (i=0; ProtocolFamilyList[i]; ++i) {
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_ADDSTRING,
				                   0, (LPARAM)ProtocolFamilyList[i]);
			}
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, EM_LIMITTEXT,
			                   ProtocolFamilyMaxLength-1, 0);
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_SETCURSEL,0,0);
#endif /* NO_INET6 */


			j = 0;
			w = 1;
			if ((comports=DetectComPorts(ComPortTable, GetHNRec->MaxComPort, ComPortDesc)) >= 0) {
				for (i=0; i<comports; i++) {
					// MaxComPort を越えるポートは表示しない
					if (ComPortTable[i] > GetHNRec->MaxComPort) {
						continue;
					}

					// 使用中のポートは表示しない
					if (CheckCOMFlag(ComPortTable[i]) == 1) {
						continue;
					}

					_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "COM%d", ComPortTable[i]);
					if (ComPortDesc[i] != NULL) {
						strncat_s(EntName, sizeof(EntName), ": ", _TRUNCATE);
						strncat_s(EntName, sizeof(EntName), ComPortDesc[i], _TRUNCATE);
					}
					SendDlgItemMessage(Dialog, IDC_HOSTCOM, CB_ADDSTRING,
					                   0, (LPARAM)EntName);
					j++;
					if (GetHNRec->ComPort==ComPortTable[i]) {
						w = j;
					}
				}
			} else {
				for (i=1; i<=GetHNRec->MaxComPort ;i++) {
					// 使用中のポートは表示しない
					if (CheckCOMFlag(i) == 1) {
						continue;
					}
					_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "COM%d", i);
					SendDlgItemMessage(Dialog, IDC_HOSTCOM, CB_ADDSTRING,
					                   0, (LPARAM)EntName);
					j++;
					if (GetHNRec->ComPort==i) {
						w = j;
					}
				}
			}
			if (j>0) {
				SendDlgItemMessage(Dialog, IDC_HOSTCOM, CB_SETCURSEL,w-1,0);
			}
			else { /* All com ports are already used */
				GetHNRec->PortType = IdTCPIP;
				DisableDlgItem(Dialog,IDC_HOSTSERIAL,IDC_HOSTSERIAL);
			}

			SetRB(Dialog,GetHNRec->PortType,IDC_HOSTTCPIP,IDC_HOSTSERIAL);

			if ( GetHNRec->PortType==IdTCPIP ) {
				DisableDlgItem(Dialog,IDC_HOSTCOMLABEL,IDC_HOSTCOM);
			}
#ifndef INET6
			else {
				DisableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
			}
#else
			else {
				DisableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
				DisableDlgItem(Dialog,IDC_HOSTTCPPROTOCOLLABEL,IDC_HOSTTCPPROTOCOL);
			}
#endif /* NO_INET6 */

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					GetHNRec = (PGetHNRec)GetWindowLong(Dialog,DWL_USER);
					if ( GetHNRec!=NULL ) {
#ifndef NO_INET6
						char afstr[BUFSIZ];
#endif /* NO_INET6 */
						GetRB(Dialog,&GetHNRec->PortType,IDC_HOSTTCPIP,IDC_HOSTSERIAL);
						if ( GetHNRec->PortType==IdTCPIP ) {
							GetDlgItemText(Dialog, IDC_HOSTNAME, GetHNRec->HostName, HostNameMaxLength);
						}
						else {
							GetHNRec->HostName[0] = 0;
						}
						GetRB(Dialog,&GetHNRec->Telnet,IDC_HOSTTELNET,IDC_HOSTTELNET);
						i = GetDlgItemInt(Dialog,IDC_HOSTTCPPORT,&Ok,FALSE);
						if (Ok) {
							GetHNRec->TCPPort = i;
						}
#ifndef NO_INET6
#define getaf(str) \
	((strcmp((str), "IPv6") == 0) ? AF_INET6 : \
	((strcmp((str), "IPv4") == 0) ? AF_INET : AF_UNSPEC))
						memset(afstr, 0, sizeof(afstr));
						GetDlgItemText(Dialog, IDC_HOSTTCPPROTOCOL, afstr, sizeof(afstr));
						GetHNRec->ProtocolFamily = getaf(afstr);
#endif /* NO_INET6 */
						memset(EntName,0,sizeof(EntName));
						GetDlgItemText(Dialog, IDC_HOSTCOM, EntName, sizeof(EntName)-1);
						if (strncmp(EntName, "COM", 3) == 0 && EntName[3] != '\0') {
#if 0
							GetHNRec->ComPort = (BYTE)(EntName[3])-0x30;
							if (strlen(EntName)>4)
								GetHNRec->ComPort = GetHNRec->ComPort*10 + (BYTE)(EntName[4])-0x30;
#else
							GetHNRec->ComPort = atoi(&EntName[3]);
#endif
							if (GetHNRec->ComPort > GetHNRec->MaxComPort) {
								GetHNRec->ComPort = 1;
							}
						}
						else {
							GetHNRec->ComPort = 1;
						}
					}
					SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)OrigHostnameEditProc);
					EndDialog(Dialog, 1);
					if (DlgHostFont != NULL) {
						DeleteObject(DlgHostFont);
					}
					return TRUE;

				case IDCANCEL:
					SetWindowLong(hwndHostnameEdit, GWL_WNDPROC, (LONG)OrigHostnameEditProc);
					EndDialog(Dialog, 0);
					if (DlgHostFont != NULL) {
						DeleteObject(DlgHostFont);
					}
					return TRUE;

				case IDC_HOSTTCPIP:
					EnableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
#ifndef NO_INET6
					EnableDlgItem(Dialog,IDC_HOSTTCPPROTOCOLLABEL,IDC_HOSTTCPPROTOCOL);
#endif /* NO_INET6 */
					DisableDlgItem(Dialog,IDC_HOSTCOMLABEL,IDC_HOSTCOM);
					return TRUE;

				case IDC_HOSTSERIAL:
					EnableDlgItem(Dialog,IDC_HOSTCOMLABEL,IDC_HOSTCOM);
					DisableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
#ifndef NO_INET6
					DisableDlgItem(Dialog,IDC_HOSTTCPPROTOCOLLABEL,IDC_HOSTTCPPROTOCOL);
#endif /* NO_INET6 */
					break;

				case IDC_HOSTTELNET:
					GetRB(Dialog,&i,IDC_HOSTTELNET,IDC_HOSTTELNET);
					if ( i==1 ) {
						GetHNRec = (PGetHNRec)GetWindowLong(Dialog,DWL_USER);
						if ( GetHNRec!=NULL ) {
							SetDlgItemInt(Dialog,IDC_HOSTTCPPORT,GetHNRec->TelPort,FALSE);
						}
					}
					break;

				case IDC_HOSTCOM:
					if(HIWORD(wParam) == CBN_DROPDOWN) {
						HWND hostcom = GetDlgItem(Dialog, IDC_HOSTCOM);
						int count = SendMessage(hostcom, CB_GETCOUNT, 0, 0);
						int i, len, max_len = 0;
						char *lbl;
						HDC TmpDC = GetDC(hostcom);
						SIZE s;
						for (i=0; i<count; i++) {
							len = SendMessage(hostcom, CB_GETLBTEXTLEN, i, 0);
							lbl = (char *)calloc(len+1, sizeof(char));
							SendMessage(hostcom, CB_GETLBTEXT, i, (LPARAM)lbl);
							GetTextExtentPoint32(TmpDC, lbl, len, &s);
							if (s.cx > max_len) {
								max_len = s.cx;
							}
							free(lbl);
						}
						SendMessage(hostcom, CB_SETDROPPEDWIDTH,
						            max_len + GetSystemMetrics(SM_CXVSCROLL), 0);
					}
					break;

				case IDC_HOSTHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

BOOL CALLBACK DirDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PCHAR CurDir;
	char HomeDir[MAXPATHLEN];
	char TmpDir[MAXPATHLEN];
	RECT R;
	HDC TmpDC;
	SIZE s;
	HWND HDir, HSel, HOk, HCancel, HHelp;
	POINT D, B, S;
	int WX, WY, WW, WH, CW, DW, DH, BW, BH, SW, SH;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgDirFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_DIRCURRENT_LABEL, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_DIRCURRENT, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_DIRNEW_LABEL, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_DIRNEW, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_DIRHELP, WM_SETFONT, (WPARAM)DlgDirFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgDirFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_DIR_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_DIRCURRENT_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_DIR_CURRENT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_DIRCURRENT_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_DIRNEW_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_DIR_NEW", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_DIRNEW_LABEL, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_DIRHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_DIRHELP, uimsg);

			CurDir = (PCHAR)(lParam);
			SetWindowLong(Dialog, DWL_USER, lParam);

			SetDlgItemText(Dialog, IDC_DIRCURRENT, CurDir);
			SendDlgItemMessage(Dialog, IDC_DIRNEW, EM_LIMITTEXT,
			                   MAXPATHLEN-1, 0);

// adjust dialog size
			// get size of current dir text
			HDir = GetDlgItem(Dialog, IDC_DIRCURRENT);
			GetWindowRect(HDir,&R);
			D.x = R.left;
			D.y = R.top;
			ScreenToClient(Dialog,&D);
			DH = R.bottom-R.top;  
			TmpDC = GetDC(Dialog);
			GetTextExtentPoint32(TmpDC,CurDir,strlen(CurDir),&s);
			ReleaseDC(Dialog,TmpDC);
			DW = s.cx + s.cx/10;

			// get button size
			HOk = GetDlgItem(Dialog, IDOK);
			HCancel = GetDlgItem(Dialog, IDCANCEL);
			HHelp = GetDlgItem(Dialog, IDC_DIRHELP);
			GetWindowRect(HHelp,&R);
			B.x = R.left;
			B.y = R.top;
			ScreenToClient(Dialog,&B);
			BW = R.right-R.left;
			BH = R.bottom-R.top;

			// calc new dialog size
			GetWindowRect(Dialog,&R);
			WX = R.left;
			WY = R.top;
			WW = R.right-R.left;
			WH = R.bottom-R.top;
			GetClientRect(Dialog,&R);
			CW = R.right-R.left;
			if (D.x+DW < CW) {
				DW = CW-D.x;
			}
			WW = WW + D.x+DW - CW;

			// resize current dir text
			MoveWindow(HDir,D.x,D.y,DW,DH,TRUE);
			// move buttons
			MoveWindow(HOk,(D.x+DW-4*BW)/2,B.y,BW,BH,TRUE);
			MoveWindow(HCancel,(D.x+DW-BW)/2,B.y,BW,BH,TRUE);
			MoveWindow(HHelp,(D.x+DW+2*BW)/2,B.y,BW,BH,TRUE);
			// resize edit box
			HDir = GetDlgItem(Dialog, IDC_DIRNEW);
			GetWindowRect(HDir,&R);
			D.x = R.left;
			D.y = R.top;
			ScreenToClient(Dialog,&D);
			DW = R.right-R.left;
			if (DW<s.cx) {
				DW = s.cx;
			}
			MoveWindow(HDir,D.x,D.y,DW,R.bottom-R.top,TRUE);
			// select dir button
			HSel = GetDlgItem(Dialog, IDC_SELECT_DIR);
			GetWindowRect(HSel, &R);
			SW = R.right-R.left;
			SH = R.bottom-R.top;
			S.x = R.bottom;
			S.y = R.top;
			ScreenToClient(Dialog, &S);
			MoveWindow(HSel, D.x + DW + 8, S.y, SW, SH, TRUE);
			WW = WW + SW;

			// resize dialog
			MoveWindow(Dialog,WX,WY,WW,WH,TRUE);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					CurDir = (PCHAR)GetWindowLong(Dialog,DWL_USER);
					if ( CurDir!=NULL ) {
						_getcwd(HomeDir,sizeof(HomeDir));
						_chdir(CurDir);
						GetDlgItemText(Dialog, IDC_DIRNEW, TmpDir, sizeof(TmpDir));
						if ( strlen(TmpDir)>0 ) {
							if (_chdir(TmpDir) != 0) {
								get_lang_msg("MSG_TT_ERROR", uimsg2, sizeof(uimsg2), "Tera Term: Error", UILanguageFile);
								get_lang_msg("MSG_FIND_DIR_ERROR", uimsg, sizeof(uimsg), "Cannot find directory", UILanguageFile);
								MessageBox(Dialog,uimsg,uimsg2,MB_ICONEXCLAMATION);
								_chdir(HomeDir);
								return TRUE;
							}
							_getcwd(CurDir,MAXPATHLEN);
						}
						_chdir(HomeDir);
					}
					EndDialog(Dialog, 1);
					if (DlgDirFont != NULL) {
						DeleteObject(DlgDirFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgDirFont != NULL) {
						DeleteObject(DlgDirFont);
					}
					return TRUE;

				case IDC_SELECT_DIR:
					get_lang_msg("DLG_SELECT_DIR_TITLE", uimsg, sizeof(uimsg),
					             "Select new directory", UILanguageFile);
					GetDlgItemText(Dialog, IDC_DIRCURRENT, uimsg2, sizeof(uimsg2));
					doSelectFolder(Dialog, uimsg2, sizeof(uimsg2), uimsg);
					SetDlgItemText(Dialog, IDC_DIRNEW, uimsg2);
					return TRUE;

				case IDC_DIRHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

// 実行ファイルからバージョン情報を得る (2005.2.28 yutaka)
static void get_file_version(char *exefile, int *major, int *minor, int *release, int *build)
{
	typedef struct {
		WORD wLanguage;
		WORD wCodePage;
	} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;
	LPLANGANDCODEPAGE lplgcode;
	UINT unLen;
	DWORD size;
	char *buf = NULL;
	BOOL ret;
	int i;
	char fmt[80];
	char *pbuf;

	size = GetFileVersionInfoSize(exefile, NULL);
	if (size == 0) {
		goto error;
	}
	buf = malloc(size);
	ZeroMemory(buf, size);

	if (GetFileVersionInfo(exefile, 0, size, buf) == FALSE) {
		goto error;
	}

	ret = VerQueryValue(buf, "\\VarFileInfo\\Translation", 
	                    (LPVOID *)&lplgcode, &unLen);
	if (ret == FALSE) {
		goto error;
	}

	for (i = 0 ; i < (int)(unLen / sizeof(LANGANDCODEPAGE)) ; i++) {
		_snprintf_s(fmt, sizeof(fmt), _TRUNCATE, "\\StringFileInfo\\%04x%04x\\FileVersion",
		            lplgcode[i].wLanguage, lplgcode[i].wCodePage);
		VerQueryValue(buf, fmt, &pbuf, &unLen);
		if (unLen > 0) { // get success
			int n, a, b, c, d;

			n = sscanf(pbuf, "%d, %d, %d, %d", &a, &b, &c, &d);
			if (n == 4) { // convert success
				*major = a;
				*minor = b;
				*release = c;
				*build = d;
				break;
			}
		}
	}

	free(buf);
	return;

error:
	free(buf);
	*major = *minor = *release = *build = 0;
}


//
// static textに書かれたURLをダブルクリックすると、ブラウザが起動するようにする。
// based on sakura editor 1.5.2.1 # CDlgAbout.cpp
// (2005.4.7 yutaka)
//

typedef struct {
	WNDPROC proc;
	BOOL mouseover;
	HFONT font;
	HWND hWnd;
	int timer_done;
} url_subclass_t;

static url_subclass_t author_url_class, forum_url_class;

// static textに割り当てるプロシージャ
static LRESULT CALLBACK UrlWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	url_subclass_t *parent = (url_subclass_t *)GetWindowLongPtr( hWnd, GWLP_USERDATA );
	HDC hdc;
	POINT pt;
	RECT rc;

	switch (msg) {
#if 0
	case WM_SETCURSOR:
		{
			// カーソル形状変更
			HCURSOR hc;

			hc = (HCURSOR)LoadImage(NULL,
					MAKEINTRESOURCE(IDC_HAND),
					IMAGE_CURSOR,
					0,
					0,
					LR_DEFAULTSIZE | LR_SHARED);
			if (hc != NULL) {
				SetClassLongPtr(hWnd, GCLP_HCURSOR, (LONG_PTR)hc);
			}
			return (LRESULT)0;
		}
#endif

	case WM_LBUTTONDBLCLK:
		{
			char url[128];

			// get URL
			SendMessage(hWnd, WM_GETTEXT , sizeof(url), (LPARAM)url);
			// kick WWW browser
			ShellExecute(NULL, NULL, url, NULL, NULL,SW_SHOWNORMAL);
		}
		break;

	case WM_MOUSEMOVE:
		{
			BOOL bHilighted;
			pt.x = LOWORD( lParam );
			pt.y = HIWORD( lParam );
			GetClientRect( hWnd, &rc );
			bHilighted = PtInRect( &rc, pt );

			if (parent->mouseover != bHilighted) {
				parent->mouseover = bHilighted;
				InvalidateRect( hWnd, NULL, TRUE );
				if (parent->timer_done == 0) {
					parent->timer_done = 1;
					SetTimer( hWnd, 1, 200, NULL );
				}
			}

		}
		break;

	case WM_TIMER:
		// URLの上にマウスカーソルがあるなら、システムカーソルを変更する。
		if (author_url_class.mouseover || forum_url_class.mouseover) {
			HCURSOR hc;
			//SetCapture(hWnd);

			hc = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_HAND),
			                        IMAGE_CURSOR, 0, 0,
			                        LR_DEFAULTSIZE | LR_SHARED);

			SetSystemCursor(CopyCursor(hc), 32512 /* OCR_NORMAL */);    // 矢印
			SetSystemCursor(CopyCursor(hc), 32513 /* OCR_IBEAM */);     // Iビーム

		} else {
			//ReleaseCapture();
			// マウスカーソルを元に戻す。
			SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

		}

		// カーソルがウィンドウ外にある場合にも WM_MOUSEMOVE を送る
		GetCursorPos( &pt );
		ScreenToClient( hWnd, &pt );
		GetClientRect( hWnd, &rc );
		if( !PtInRect( &rc, pt ) ) {
			SendMessage( hWnd, WM_MOUSEMOVE, 0, MAKELONG( pt.x, pt.y ) );
		}
		break;

	case WM_PAINT: 
		{
		// ウィンドウの描画
		PAINTSTRUCT ps;
		HFONT hFont;
		HFONT hOldFont;
		TCHAR szText[512];

		hdc = BeginPaint( hWnd, &ps );

		// 現在のクライアント矩形、テキスト、フォントを取得する
		GetClientRect( hWnd, &rc );
		GetWindowText( hWnd, szText, 512 );
		hFont = (HFONT)SendMessage( hWnd, WM_GETFONT, (WPARAM)0, (LPARAM)0 );

		// テキスト描画
		SetBkMode( hdc, TRANSPARENT );
		SetTextColor( hdc, parent->mouseover ? RGB( 0x84, 0, 0 ): RGB( 0, 0, 0xff ) );
		hOldFont = (HFONT)SelectObject( hdc, (HGDIOBJ)hFont );
		TextOut( hdc, 2, 0, szText, lstrlen( szText ) );
		SelectObject( hdc, (HGDIOBJ)hOldFont );

		// フォーカス枠描画
		if( GetFocus() == hWnd )
			DrawFocusRect( hdc, &rc );

		EndPaint( hWnd, &ps );
		return 0;
		}

	case WM_ERASEBKGND:
		hdc = (HDC)wParam;
		GetClientRect( hWnd, &rc );

		// 背景描画
		if( parent->mouseover ){
			// ハイライト時背景描画
			SetBkColor( hdc, RGB( 0xff, 0xff, 0 ) );
			ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL );
		}else{
			// 親にWM_CTLCOLORSTATICを送って背景ブラシを取得し、背景描画する
			HBRUSH hbr;
			HBRUSH hbrOld;
			hbr = (HBRUSH)SendMessage( GetParent( hWnd ), WM_CTLCOLORSTATIC, wParam, (LPARAM)hWnd );
			hbrOld = (HBRUSH)SelectObject( hdc, hbr );
			FillRect( hdc, &rc, hbr );
			SelectObject( hdc, hbrOld );
		}
		return (LRESULT)1;

	case WM_DESTROY:
		// 後始末
		SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR)parent->proc );
		if( parent->font != NULL ) {
			DeleteObject( parent->font );
		}

		// マウスカーソルを元に戻す。
		SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
		return (LRESULT)0;
	}

	return CallWindowProc( parent->proc, hWnd, msg, wParam, lParam );
}

// static textにプロシージャを設定し、サブクラス化する。
static void do_subclass_window(HWND hWnd, url_subclass_t *parent)
{
	HFONT hFont;
	LOGFONT lf;
	LONG_PTR lptr;

	//SetCapture(hWnd);

	if (!IsWindow(hWnd)) {
		return;
	}

	// 親のプロシージャをサブクラスから参照できるように、ポインタを登録しておく。
	lptr = SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)parent );
	// サブクラスのプロシージャを登録する。
	parent->proc = (WNDPROC)SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR)UrlWndProc);

	// 下線を付ける
	hFont = (HFONT)SendMessage( hWnd, WM_GETFONT, (WPARAM)0, (LPARAM)0 );
	GetObject( hFont, sizeof(lf), &lf );
	lf.lfUnderline = TRUE;
	parent->font = hFont = CreateFontIndirect( &lf ); // 不要になったら削除すること
	if (hFont != NULL) {
		SendMessage( hWnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );
	}

	parent->hWnd = hWnd;
	parent->timer_done = 0;
}


BOOL CALLBACK AboutDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	int a, b, c, d;
	char buf[128], tmpbuf[128];
	HDC hdc;
	HWND hwnd;
	RECT r;
	DWORD dwExt;
	WORD w, h;
	POINT point;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgAboutFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_TT_PRO, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TT_VERSION, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_INLUCDE_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_ONIGURUMA_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TT23_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TT23DATE_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_TERANISHI_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_PROJECT_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_RIGHTS_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_AUTHOR_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_AUTHOR_URL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_FORUM_LABEL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_FORUM_URL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_BUILDTIME, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_BUILDTOOL, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgAboutFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgAboutFont = NULL;
			}

			// アイコンを動的にセット
			{
				int fuLoad = LR_DEFAULTCOLOR;
				HICON hicon;

				if (is_NT4()) {
					fuLoad = LR_VGACOLOR;
				}

				hicon = LoadImage(hInst, MAKEINTRESOURCE(IDI_TTERM),
								  IMAGE_ICON, 32, 32, fuLoad);
				SendDlgItemMessage(Dialog, IDC_TT_ICON, STM_SETICON, (WPARAM)hicon, 0);
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_ABOUT_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);

			// Tera Termのバージョンを設定する (2005.2.28 yutaka)
			// __argv[0]では WinExec() したプロセスから参照できないようなので削除。(2005.3.14 yutaka)
			get_file_version("ttermpro.exe", &a, &b, &c, &d);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Version %d.%d", a, b);
#ifdef SVNVERSION
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " (SVN# %d)", SVNVERSION);
			strncat_s(buf, sizeof(buf), tmpbuf, _TRUNCATE);
#endif
			SendMessage(GetDlgItem(Dialog, IDC_TT_VERSION), WM_SETTEXT, 0, (LPARAM)buf);

			// Onigurumaのバージョンを設定する 
			// バージョンの取得は onig_version() を呼び出すのが適切だが、そのためだけにライブラリを
			// リンクしたくなかったので、以下のようにした。Onigurumaのバージョンが上がった場合、
			// ビルドエラーとなるかもしれない。
			// (2005.10.8 yutaka)
			// ライブラリをリンクし、正規の手順でバージョンを得ることにした。
			// (2006.7.24 yutaka)
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Oniguruma %s", onig_version());
			SendMessage(GetDlgItem(Dialog, IDC_ONIGURUMA_LABEL), WM_SETTEXT, 0, (LPARAM)buf);

			// ビルドしたときに使われたVisual C++のバージョンを設定する。(2009.3.3 yutaka)
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Built using Microsoft Visual C++");
#ifdef _MSC_FULL_VER
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " %d.%d",
				(_MSC_FULL_VER / 10000000) - 6,
				(_MSC_FULL_VER / 100000) % 100);
			strncat_s(buf, sizeof(buf), tmpbuf, _TRUNCATE);
			if (_MSC_FULL_VER % 100000) {
				_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " build %d",
					_MSC_FULL_VER % 100000);
				strncat_s(buf, sizeof(buf), tmpbuf, _TRUNCATE);
			}
#elif defined(_MSC_VER)
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " %d.%d",
				(_MSC_VER / 100) - 6,
				_MSC_VER % 100);
			strncat_s(buf, sizeof(buf), tmpbuf, _TRUNCATE);
#endif
			SendMessage(GetDlgItem(Dialog, IDC_BUILDTOOL), WM_SETTEXT, 0, (LPARAM)buf);

			// ビルドタイムを設定する。(2009.3.4 yutaka)
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Build time: %s %s", __DATE__, __TIME__);
			SendMessage(GetDlgItem(Dialog, IDC_BUILDTIME), WM_SETTEXT, 0, (LPARAM)buf);

			// static text のサイズを変更 (2007.4.16 maya)
			hwnd = GetDlgItem(Dialog, IDC_AUTHOR_URL);
			hdc = GetDC(hwnd);
			SelectObject(hdc, DlgAboutFont);
			GetDlgItemText(Dialog, IDC_AUTHOR_URL, uimsg, sizeof(uimsg));
			dwExt = GetTabbedTextExtent(hdc,uimsg,strlen(uimsg),0,NULL);
			w = LOWORD(dwExt) + 5; // 幅が若干足りないので補正
			h = HIWORD(dwExt);
			GetWindowRect(hwnd, &r);
			point.x = r.left;
			point.y = r.top;
			ScreenToClient(Dialog, &point);
			MoveWindow(hwnd, point.x, point.y, w, h, TRUE);

			hwnd = GetDlgItem(Dialog, IDC_FORUM_URL);
			hdc = GetDC(hwnd);
			SelectObject(hdc, DlgAboutFont);
			GetDlgItemText(Dialog, IDC_FORUM_URL, uimsg, sizeof(uimsg));
			dwExt = GetTabbedTextExtent(hdc,uimsg,strlen(uimsg),0,NULL);
			w = LOWORD(dwExt) + 5; // 幅が若干足りないので補正
			h = HIWORD(dwExt);
			GetWindowRect(hwnd, &r);
			point.x = r.left;
			point.y = r.top;
			ScreenToClient(Dialog, &point);
			MoveWindow(hwnd, point.x, point.y, w, h, TRUE);

			// static textをサブクラス化する。ただし、tabstop, notifyプロパティを有効にしておかないと
			// メッセージが拾えない。(2005.4.5 yutaka)
			do_subclass_window(GetDlgItem(Dialog, IDC_AUTHOR_URL), &author_url_class);
			do_subclass_window(GetDlgItem(Dialog, IDC_FORUM_URL), &forum_url_class);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;
			}
			if (DlgAboutFont != NULL) {
				DeleteObject(DlgAboutFont);
			}
			break;
	}
	return FALSE;
}

static PCHAR far LangList[] = {"English","Japanese","Russian","Korean","UTF-8",NULL};

BOOL CALLBACK GenDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PTTSet ts;
	WORD w;
	char Temp[7];
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgGenFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_GENPORT_LABEL, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_GENPORT, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_GENLANGLABEL, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_GENLANG, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_GENHELP, WM_SETFONT, (WPARAM)DlgGenFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgGenFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_GEN_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_GENPORT_LABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_GEN_PORT", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_GENPORT_LABEL, uimsg);
			GetDlgItemText(Dialog, IDC_GENLANGLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_GEN_LANG", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_GENLANGLABEL, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_GENHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_GENHELP, uimsg);

			SendDlgItemMessage(Dialog, IDC_GENPORT, CB_ADDSTRING,
			                   0, (LPARAM)"TCP/IP");
			for (w=1;w<=ts->MaxComPort;w++) {
				_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "COM%d", w);
				SendDlgItemMessage(Dialog, IDC_GENPORT, CB_ADDSTRING,
				                   0, (LPARAM)Temp);
			}
			if (ts->PortType==IdSerial) {
				if (ts->ComPort <= ts->MaxComPort) {
					w = ts->ComPort;
				}
				else {
					w = 1; // COM1
				}
			}
			else {
				w = 0; // TCP/IP
			}
			SendDlgItemMessage(Dialog, IDC_GENPORT, CB_SETCURSEL,w,0);

			if ((ts->MenuFlag & MF_NOLANGUAGE)==0) {
				ShowDlgItem(Dialog,IDC_GENLANGLABEL,IDC_GENLANG);
				SetDropDownList(Dialog, IDC_GENLANG, LangList, ts->Language);
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLong(Dialog,DWL_USER);
					if (ts!=NULL) {
						w = (WORD)GetCurSel(Dialog, IDC_GENPORT);
						if (w>1) {
							ts->PortType = IdSerial;
							ts->ComPort = w-1;
						}
						else {
							ts->PortType = IdTCPIP;
						}
						if ((ts->MenuFlag & MF_NOLANGUAGE)==0) {
							WORD language = (WORD)GetCurSel(Dialog, IDC_GENLANG);

							// Language が変更されたとき、
							// KanjiCode/KanjiCodeSend を変更先の Language に存在する値に置き換える
							if (language != ts->Language) {
								WORD KanjiCode = ts->KanjiCode;
								WORD KanjiCodeSend = ts->KanjiCodeSend;
								ts->KanjiCode = KanjiCodeTranslate(language,KanjiCode);
								ts->KanjiCodeSend = KanjiCodeTranslate(language,KanjiCodeSend);
							}

							ts->Language = language;
						}
					}

					// TTXKanjiMenu のために、OK 押下時にメニュー再描画の
					// メッセージを飛ばすようにした。 (2007.7.14 maya)
					PostMessage(GetParent(Dialog),WM_USER_CHANGEMENU,0,0);

					EndDialog(Dialog, 1);
					if (DlgGenFont != NULL) {
						DeleteObject(DlgGenFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgGenFont != NULL) {
						DeleteObject(DlgGenFont);
					}
					return TRUE;

				case IDC_GENHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

BOOL CALLBACK WinListDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PBOOL Close;
	int n;
	HWND Hw;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font;

	switch (Message) {
		case WM_INITDIALOG:
			Close = (PBOOL)lParam;
			SetWindowLong(Dialog, DWL_USER, lParam);

			font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
			GetObject(font, sizeof(LOGFONT), &logfont);
			if (get_lang_font("DLG_SYSTEM_FONT", Dialog, &logfont, &DlgWinlistFont, UILanguageFile)) {
				SendDlgItemMessage(Dialog, IDC_WINLISTLABEL, WM_SETFONT, (WPARAM)DlgWinlistFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINLISTLIST, WM_SETFONT, (WPARAM)DlgWinlistFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDOK, WM_SETFONT, (WPARAM)DlgWinlistFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDCANCEL, WM_SETFONT, (WPARAM)DlgWinlistFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINLISTCLOSE, WM_SETFONT, (WPARAM)DlgWinlistFont, MAKELPARAM(TRUE,0));
				SendDlgItemMessage(Dialog, IDC_WINLISTHELP, WM_SETFONT, (WPARAM)DlgWinlistFont, MAKELPARAM(TRUE,0));
			}
			else {
				DlgWinlistFont = NULL;
			}

			GetWindowText(Dialog, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WINLIST_TITLE", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetWindowText(Dialog, uimsg);
			GetDlgItemText(Dialog, IDC_WINLISTLABEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WINLIST_LABEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINLISTLABEL, uimsg);
			GetDlgItemText(Dialog, IDOK, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WINLIST_OPEN", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDOK, uimsg);
			GetDlgItemText(Dialog, IDCANCEL, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDCANCEL, uimsg);
			GetDlgItemText(Dialog, IDC_WINLISTCLOSE, uimsg2, sizeof(uimsg2));
			get_lang_msg("DLG_WINLSIT_CLOSEWIN", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINLISTCLOSE, uimsg);
			GetDlgItemText(Dialog, IDC_WINLISTHELP, uimsg2, sizeof(uimsg2));
			get_lang_msg("BTN_HELP", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
			SetDlgItemText(Dialog, IDC_WINLISTHELP, uimsg);

			SetWinList(GetParent(Dialog),Dialog,IDC_WINLISTLIST);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					n = SendDlgItemMessage(Dialog,IDC_WINLISTLIST,
					LB_GETCURSEL, 0, 0);
					if (n!=CB_ERR) {
						SelectWin(n);
					}
					EndDialog(Dialog, 1);
					if (DlgWinlistFont != NULL) {
						DeleteObject(DlgWinlistFont);
					}
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					if (DlgWinlistFont != NULL) {
						DeleteObject(DlgWinlistFont);
					}
					return TRUE;

				case IDC_WINLISTLIST:
					if (HIWORD(wParam)==LBN_DBLCLK) {
						PostMessage(Dialog,WM_COMMAND,IDOK,0);
					}
					break;

				case IDC_WINLISTCLOSE:
					n = SendDlgItemMessage(Dialog,IDC_WINLISTLIST,
					LB_GETCURSEL, 0, 0);
					if (n==CB_ERR) {
						break;
					}
					Hw = GetNthWin(n);
					if (Hw!=GetParent(Dialog)) {
						if (! IsWindowEnabled(Hw)) {
							MessageBeep(0);
							break;
						}
						SendDlgItemMessage(Dialog,IDC_WINLISTLIST,
						                   LB_DELETESTRING,n,0);
						PostMessage(Hw,WM_SYSCOMMAND,SC_CLOSE,0);
						if (DlgWinlistFont != NULL) {
							DeleteObject(DlgWinlistFont);
						}
					}
					else {
						Close = (PBOOL)GetWindowLong(Dialog,DWL_USER);
						if (Close!=NULL) {
							*Close = TRUE;
						}
						EndDialog(Dialog, 1);
						if (DlgWinlistFont != NULL) {
							DeleteObject(DlgWinlistFont);
						}
						return TRUE;
					}
					break;

				case IDC_WINLISTHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

BOOL FAR PASCAL SetupTerminal(HWND WndParent, PTTSet ts)
{
	int i;

	switch (ts->Language) {
	case IdJapanese: // Japanese mode
		i = IDD_TERMDLGJ;
		break;
	case IdKorean: // Korean mode //HKS
	case IdUtf8:   // UTF-8 mode
		i = IDD_TERMDLGK;
		break;
	case IdRussian: // Russian mode
		i = IDD_TERMDLGR;
		break;
	default:  // English mode
		i = IDD_TERMDLG;
	}

	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(i),
		                     WndParent, TermDlg, (LPARAM)ts);
}

BOOL FAR PASCAL SetupWin(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_WINDLG),
		                     WndParent, WinDlg, (LPARAM)ts);
}

BOOL FAR PASCAL SetupKeyboard(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_KEYBDLG),
		                     WndParent, KeybDlg, (LPARAM)ts);
}

BOOL FAR PASCAL SetupSerialPort(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_SERIALDLG),
		                     WndParent, SerialDlg, (LPARAM)ts);
}

BOOL FAR PASCAL SetupTCPIP(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_TCPIPDLG),
		                     WndParent, TCPIPDlg, (LPARAM)ts);
}

BOOL FAR PASCAL GetHostName(HWND WndParent, PGetHNRec GetHNRec)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_HOSTDLG),
		                     WndParent, HostDlg, (LPARAM)GetHNRec);
}

BOOL FAR PASCAL ChangeDirectory(HWND WndParent, PCHAR CurDir)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_DIRDLG),
		                     WndParent, DirDlg, (LPARAM)CurDir);
}

BOOL FAR PASCAL AboutDialog(HWND WndParent)
{
	return
		(BOOL)DialogBox(hInst,
		                MAKEINTRESOURCE(IDD_ABOUTDLG),
		                WndParent, AboutDlg);
}

BOOL CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) {
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(Dialog, cmb2), FALSE);
			SetFocus(GetDlgItem(Dialog,cmb1));
			break;
	}
	return FALSE;
}

BOOL FAR PASCAL ChooseFontDlg(HWND WndParent, LPLOGFONT LogFont, PTTSet ts)
{
	CHOOSEFONT cf;
	BOOL Ok;

	memset(&cf, 0, sizeof(CHOOSEFONT));
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = WndParent;
	cf.lpLogFont = LogFont;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
	           CF_FIXEDPITCHONLY | CF_SHOWHELP | CF_NOVERTFONTS |
	           CF_ENABLEHOOK;
	cf.lpfnHook = (LPCFHOOKPROC)(&TFontHook);
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = hInst;
	Ok = ChooseFont(&cf);
	return Ok;
}

BOOL FAR PASCAL SetupGeneral(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_GENDLG),
		                     WndParent, (DLGPROC)&GenDlg, (LPARAM)ts);
}

BOOL FAR PASCAL WindowWindow(HWND WndParent, PBOOL Close)
{
	*Close = FALSE;
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_WINLISTDLG),
		                     WndParent,
		                     (DLGPROC)&WinListDlg, (LPARAM)Close);
}

void FAR PASCAL TTDLGSetUILanguageFile(char *file)
{
	strncpy_s(UILanguageFile, sizeof(UILanguageFile), file, _TRUNCATE);
}

BOOL WINAPI DllMain(HANDLE hInstance,
                    ULONG ul_reason_for_call,
                    LPVOID lpReserved)
{
	//PMap pm;
	//HANDLE HMap = NULL;

	hInst = hInstance;
	switch (ul_reason_for_call) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
				break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
			//HMap = CreateFileMapping(
			//	(HANDLE) 0xFFFFFFFF, NULL, PAGE_READONLY,
			//	0, sizeof(TMap), TT_FILEMAPNAME);
			//if (HMap != NULL) {
			//	pm = (PMap)MapViewOfFile(
			//		HMap,FILE_MAP_READ,0,0,0);
			//	if (pm != NULL) {
			//		strncpy_s(UILanguageFile, sizeof(UILanguageFile),
			//		          pm->ts.UILanguageFile, _TRUNCATE);
			//	}
			//}
			DoCover_IsDebuggerPresent();
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			break;
	}
	return TRUE;
}
