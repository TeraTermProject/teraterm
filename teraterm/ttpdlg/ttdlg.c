/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2019 TeraTerm Project
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
/* IPv6 modification is Copyright(C) 2000 Jun-ya Kato <kato@win6.jp> */

/* TTDLG.DLL, dialog boxes */
#include "teraterm_conf.h"
#include "teraterm.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include <commdlg.h>
#include <dlgs.h>
#include <tchar.h>
#include <crtdbg.h>
#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "ttcommon.h"
#include "dlg_res.h"
#include "svnversion.h"
#include "ttdlg.h"

// Oniguruma: Regular expression library
#define ONIG_EXTERN extern
#include "oniguruma.h"
#undef ONIG_EXTERN

#include <winsock2.h>
#undef EFFECT_ENABLED	// エフェクトの有効可否
#undef TEXTURE_ENABLED	// テクスチャの有効可否

#ifdef _DEBUG
#define calloc(c, s)  _calloc_dbg((c), (s), _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)       _free_dbg((p), _NORMAL_BLOCK)
#define _strdup(s)	  _strdup_dbg((s), _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#undef DialogBoxParam
#define DialogBoxParam(p1,p2,p3,p4,p5) \
	TTDialogBoxParam(p1,p2,p3,p4,p5)
#undef DialogBox
#define DialogBox(p1,p2,p3,p4) \
	TTDialogBox(p1,p2,p3,p4)
#undef EndDialog
#define EndDialog(p1,p2) \
	TTEndDialog(p1, p2)

extern HANDLE hInst;

static char UILanguageFile[MAX_PATH];

static const char *ProtocolFamilyList[] = { "UNSPEC", "IPv6", "IPv4", NULL };
static PCHAR NLListRcv[] = {"CR","CR+LF", "LF", "AUTO", NULL};
static PCHAR NLList[] = {"CR","CR+LF", "LF", NULL};
static PCHAR TermList[] =
	{"VT100", "VT101", "VT102", "VT282", "VT320", "VT382",
	 "VT420", "VT520", "VT525", NULL};
static WORD Term_TermJ[] =
	{IdVT100, IdVT101, IdVT102, IdVT282, IdVT320, IdVT382,
	 IdVT420, IdVT520, IdVT525};
static WORD TermJ_Term[] = {1, 1, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9};
static PCHAR TermListJ[] =
	{"VT100", "VT100J", "VT101", "VT102", "VT102J", "VT220J", "VT282",
	 "VT320", "VT382", "VT420", "VT520", "VT525", NULL};
static PCHAR KanjiList[] = {"SJIS","EUC","JIS", "UTF-8", "UTF-8m", NULL};
static PCHAR KanjiListSend[] = {"SJIS","EUC","JIS", "UTF-8", NULL};
static PCHAR KanjiInList[] = {"^[$@","^[$B",NULL};
static PCHAR KanjiOutList[] = {"^[(B","^[(J",NULL};
static PCHAR KanjiOutList2[] = {"^[(B","^[(J","^[(H",NULL};
static PCHAR RussList[] = {"Windows","KOI8-R","CP 866","ISO 8859-5",NULL};
static PCHAR RussList2[] = {"Windows","KOI8-R",NULL};
static PCHAR LocaleList[] = {"japanese","chinese", "chinese-simplified", "chinese-traditional", NULL};
static PCHAR MetaList[] = {"off", "on", "left", "right", NULL};
static PCHAR MetaList2[] = {"off", "on", NULL};

// HKS
static PCHAR KoreanList[] = {"KS5601", "UTF-8", "UTF-8m", NULL};
static PCHAR KoreanListSend[] = {"KS5601", "UTF-8", NULL};

// UTF-8
static PCHAR Utf8List[] = {"UTF-8", "UTF-8m", NULL};
static PCHAR Utf8ListSend[] = {"UTF-8", NULL};

static PCHAR BaudList[] =
	{"110","300","600","1200","2400","4800","9600",
	 "14400","19200","38400","57600","115200",
	 "230400", "460800", "921600", NULL};


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
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_TERMHELP, "BTN_HELP" },
	};
	PTTSet ts;
	WORD w;
	//  char Temp[HostNameMaxLength + 1]; // 81(yutaka)
	char Temp[81]; // 81(yutaka)

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfosCom, _countof(TextInfosCom), UILanguageFile);
				if (ts->Language==IdJapanese) {
				static const DlgTextInfo TextInfosJp[] = {
					{ IDC_TERMKANJILABEL, "DLG_TERM_KANJI" },
					{ IDC_TERMKANA, "DLG_TERM_KANA" },
					{ IDC_TERMKANJISENDLABEL, "DLG_TERM_KANJISEND" },
					{ IDC_TERMKANASEND, "DLG_TERM_KANASEND" },
					{ IDC_TERMKINTEXT, "DLG_TERM_KIN" },
					{ IDC_TERMKOUTTEXT, "DLG_TERM_KOUT" },
					{ IDC_LOCALE_LABEL, "DLG_TERM_LOCALE" },
				};
				SetDlgTexts(Dialog, TextInfosJp, _countof(TextInfosJp), UILanguageFile);
			}
			else if ( ts->Language==IdRussian ) {
				static const DlgTextInfo TextInfosRu[] = {
					{ IDC_TERMRUSSCHARSET, "DLG_TERM_RUSSCHARSET" },
					{ IDC_TERMRUSSHOSTLABEL, "DLG_TERM_RUSSHOST" },
					{ IDC_TERMRUSSCLIENTLABEL, "DLG_TERM_RUSSCLIENT" },
					{ IDC_TERMRUSSFONTLABEL, "DLG_TERM_RUSSFONT" },
				};
				SetDlgTexts(Dialog, TextInfosRu, _countof(TextInfosRu), UILanguageFile);
			}
			else if (ts->Language==IdUtf8 || ts->Language==IdKorean) {
				static const DlgTextInfo TextInfosKo[] = {
					{ IDC_TERMKANJILABEL, "DLG_TERMK_KANJI" },
					{ IDC_TERMKANJISENDLABEL, "DLG_TERMK_KANJISEND" },
					{ IDC_LOCALE_LABEL, "DLG_TERM_LOCALE" },
				};
				SetDlgTexts(Dialog, TextInfosKo, _countof(TextInfosKo), UILanguageFile);
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
			}
			else if (ts->Language==IdUtf8) {
				SetDropDownList(Dialog, IDC_TERMKANJI, Utf8List, KanjiCode2List(ts->Language,ts->KanjiCode));
				SetDropDownList(Dialog, IDC_TERMKANJISEND, Utf8ListSend, KanjiCode2List(ts->Language,ts->KanjiCodeSend));

				// ロケール用テキストボックス
				SetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale);
				SendDlgItemMessage(Dialog, IDC_LOCALE_EDIT, EM_LIMITTEXT, sizeof(ts->Locale), 0);
			}
			CenterWindow(Dialog, GetParent(Dialog));
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);

					if ( ts!=NULL ) {
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
							GetDlgItemText(Dialog, IDC_TERMANSBACK,Temp,sizeof(Temp));
							ts->AnswerbackLen = Hex2Str(Temp,ts->Answerback,sizeof(ts->Answerback));
						}

						GetRB(Dialog,&ts->AutoWinSwitch,IDC_TERMAUTOSWITCH,IDC_TERMAUTOSWITCH);

						if (ts->Language==IdJapanese) {
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKANJI)) > 0) {
								ts->KanjiCode = w;
							}
							GetRB(Dialog,&ts->JIS7Katakana,IDC_TERMKANA,IDC_TERMKANA);
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND)) > 0) {
								ts->KanjiCodeSend = w;
							}
							GetRB(Dialog,&ts->JIS7KatakanaSend,IDC_TERMKANASEND,IDC_TERMKANASEND);
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKIN)) > 0) {
								ts->KanjiIn = w;
							}
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKOUT)) > 0) {
								ts->KanjiOut = w;
							}

							GetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale, sizeof(ts->Locale));
						}
						else if (ts->Language==IdRussian) {
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMRUSSHOST)) > 0) {
								ts->RussHost = w;
							}
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMRUSSCLIENT)) > 0) {
								ts->RussClient = w;
							}
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMRUSSFONT)) > 0) {
								ts->RussFont = w;
							}
						}
						else if (ts->Language==IdKorean || // HKS
						         ts->Language==IdUtf8) {
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKANJI)) > 0) {
								ts->KanjiCode = List2KanjiCode(ts->Language, w);
							}
							if ((w = (WORD)GetCurSel(Dialog, IDC_TERMKANJISEND)) > 0) {
								ts->KanjiCodeSend = List2KanjiCode(ts->Language, w);
							}

							ts->JIS7KatakanaSend=0;
							ts->JIS7Katakana=0;
							ts->KanjiIn = 0;
							ts->KanjiOut = 0;

							GetDlgItemText(Dialog, IDC_LOCALE_EDIT, ts->Locale, sizeof(ts->Locale));
						}

					}
					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
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
	TestRect.left = Rect.left + (int)((Rect.right-Rect.left)*0.70);
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

static INT_PTR CALLBACK WinDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_WIN_TITLE" },
		{ IDC_WINTITLELABEL, "DLG_WIN_TITLELABEL" },
		{ IDC_WINCURSOR, "DLG_WIN_CURSOR" },
		{ IDC_WINBLOCK, "DLG_WIN_BLOCK" },
		{ IDC_WINVERT, "DLG_WIN_VERT" },
		{ IDC_WINHORZ, "DLG_WIN_HORZ" },
		{ IDC_FONTBOLD, "DLG_WIN_BOLDFONT" },
		{ IDC_WINHIDETITLE, "DLG_WIN_HIDETITLE" },
		{ IDC_WINHIDEMENU, "DLG_WIN_HIDEMENU" },
		{ IDC_WINCOLOREMU, "DLG_WIN_COLOREMU" },
		{ IDC_WINAIXTERM16, "DLG_WIN_AIXTERM16" },
		{ IDC_WINXTERM256, "DLG_WIN_XTERM256" },
		{ IDC_WINSCROLL1, "DLG_WIN_SCROLL1" },
		{ IDC_WINSCROLL3, "DLG_WIN_SCROLL3" },
		{ IDC_WINCOLOR, "DLG_WIN_COLOR" },
		{ IDC_WINTEXT, "DLG_WIN_TEXT" },
		{ IDC_WINBACK, "DLG_WIN_BG" },
		{ IDC_WINATTRTEXT, "DLG_WIN_ATTRIB" },
		{ IDC_WINREV, "DLG_WIN_REVERSE" },
		{ IDC_WINREDLABEL, "DLG_WIN_R" },
		{ IDC_WINGREENLABEL, "DLG_WIN_G" },
		{ IDC_WINBLUELABEL, "DLG_WIN_B" },
		{ IDC_WINUSENORMALBG, "DLG_WIN_ALWAYSBG" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_WINHELP, "BTN_HELP" },
		{ IDC_WINCOLOREMU, "DLG_WIN_PCBOLD16" },		// get_lang_msg
	};
	PTTSet ts;
	HWND Wnd, HRed, HGreen, HBlue;
	int IAttr, IOffset;
	WORD i, pos, ScrollCode, NewPos;
	HDC DC;
	TCHAR uimsg[MAX_UIMSG];

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
			SetDlgItemTextA(Dialog, IDC_WINTITLE, ts->Title);
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
				SetRB(Dialog, (ts->FontFlag & FF_BOLD) > 0, IDC_FONTBOLD,IDC_FONTBOLD);
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

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
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
							GetRB(Dialog, &i, IDC_FONTBOLD, IDC_FONTBOLD);
							if (i > 0) {
								ts->FontFlag |= FF_BOLD;
							}
							else {
								ts->FontFlag &= ~(WORD)FF_BOLD;
							}
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
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
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
			ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
			if ( ts==NULL ) {
				return TRUE;
			}
			RestoreVar(Dialog,ts,&IAttr,&IOffset);
			DispSample(Dialog,ts,IAttr);
			break;

		case WM_HSCROLL:
			ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
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

static INT_PTR CALLBACK KeybDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_KEYB_TITLE" },
		{ IDC_KEYBTRANS, "DLG_KEYB_TRANSMIT" },
		{ IDC_KEYBBS, "DLG_KEYB_BS" },
		{ IDC_KEYBDEL, "DLG_KEYB_DEL" },
		{ IDC_KEYBKEYBTEXT, "DLG_KEYB_KEYB" },
		{ IDC_KEYBMETATEXT, "DLG_KEYB_META" },
		{ IDC_KEYBDISABLE, "DLG_KEYB_DISABLE" },
		{ IDC_KEYBAPPKEY, "DLG_KEYB_APPKEY" },
		{ IDC_KEYBAPPCUR, "DLG_KEYB_APPCUR" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_KEYBHELP, "BTN_HELP" },
	};
	PTTSet ts;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);

			SetRB(Dialog,ts->BSKey-1,IDC_KEYBBS,IDC_KEYBBS);
			SetRB(Dialog,ts->DelKey,IDC_KEYBDEL,IDC_KEYBDEL);
			SetRB(Dialog,ts->MetaKey,IDC_KEYBMETA,IDC_KEYBMETA);
			SetRB(Dialog,ts->DisableAppKeypad,IDC_KEYBAPPKEY,IDC_KEYBAPPKEY);
			SetRB(Dialog,ts->DisableAppCursor,IDC_KEYBAPPCUR,IDC_KEYBAPPCUR);

			if (!IsWindowsNTKernel()) {
				SetDropDownList(Dialog, IDC_KEYBMETA, MetaList2, ts->MetaKey + 1);
			}
			else {
				SetDropDownList(Dialog, IDC_KEYBMETA, MetaList, ts->MetaKey + 1);
			}

			if (ts->Language==IdRussian) {
				ShowDlgItem(Dialog,IDC_KEYBKEYBTEXT,IDC_KEYBKEYB);
				SetDropDownList(Dialog, IDC_KEYBKEYB, RussList2, ts->RussKeyb);
			}

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
					if ( ts!=NULL ) {
						WORD w;

						GetRB(Dialog,&ts->BSKey,IDC_KEYBBS,IDC_KEYBBS);
						ts->BSKey++;
						GetRB(Dialog,&ts->DelKey,IDC_KEYBDEL,IDC_KEYBDEL);
						GetRB(Dialog,&ts->DisableAppKeypad,IDC_KEYBAPPKEY,IDC_KEYBAPPKEY);
						GetRB(Dialog,&ts->DisableAppCursor,IDC_KEYBAPPCUR,IDC_KEYBAPPCUR);
						if ((w = (WORD)GetCurSel(Dialog, IDC_KEYBMETA)) > 0) {
							ts->MetaKey = w - 1;
						}
						if (ts->Language==IdRussian) {
							if ((w = (WORD)GetCurSel(Dialog, IDC_KEYBKEYB)) > 0) {
								ts->RussKeyb = w;
							}
						}
					}
					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_KEYBHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

static PCHAR DataList[] = {"7 bit","8 bit",NULL};
static PCHAR ParityList[] = {"none", "odd", "even", "mark", "space", NULL};
static PCHAR StopList[] = {"1 bit", "1.5 bit", "2 bit", NULL};
static PCHAR FlowList[] = {"Xon/Xoff","RTS/CTS","none", "DSR/DTR", NULL};

static INT_PTR CALLBACK SerialDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_SERIAL_TITLE" },
		{ IDC_SERIALPORT_LABEL, "DLG_SERIAL_PORT" },
		{ IDC_SERIALBAUD_LEBAL, "DLG_SERIAL_BAUD" },
		{ IDC_SERIALDATA_LABEL, "DLG_SERIAL_DATA" },
		{ IDC_SERIALPARITY_LABEL, "DLG_SERIAL_PARITY" },
		{ IDC_SERIALSTOP_LABEL, "DLG_SERIAL_STOP" },
		{ IDC_SERIALFLOW_LABEL, "DLG_SERIAL_FLOW" },
		{ IDC_SERIALDELAY, "DLG_SERIAL_DELAY" },
		{ IDC_SERIALDELAYCHAR_LABEL, "DLG_SERIAL_DELAYCHAR" },
		{ IDC_SERIALDELAYLINE_LABEL, "DLG_SERIAL_DELAYLINE" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_SERIALHELP, "BTN_HELP" },
	};
	PTTSet ts;
	int i, w, sel;
	char Temp[128];
	WORD ComPortTable[MAXCOMPORT];
	static char *ComPortDesc[MAXCOMPORT];
	int comports;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);

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

			SetDropDownList(Dialog, IDC_SERIALBAUD, BaudList, 0);
			i = sel = 0;
			while (BaudList[i] != NULL) {
				if (atoi(BaudList[i]) == ts->Baud) {
					SendDlgItemMessage(Dialog, IDC_SERIALBAUD, CB_SETCURSEL, i, 0);
					sel = 1;
					break;
				}
				i++;
			}
			if (!sel) {
				SetDlgItemInt(Dialog, IDC_SERIALBAUD, ts->Baud, FALSE);
			}

			SetDropDownList(Dialog, IDC_SERIALDATA, DataList, ts->DataBit);
			SetDropDownList(Dialog, IDC_SERIALPARITY, ParityList, ts->Parity);
			SetDropDownList(Dialog, IDC_SERIALSTOP, StopList, ts->StopBit);
			SetDropDownList(Dialog, IDC_SERIALFLOW, FlowList, ts->Flow);

			SetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,ts->DelayPerChar,FALSE);
			SendDlgItemMessage(Dialog, IDC_SERIALDELAYCHAR, EM_LIMITTEXT,4, 0);

			SetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,ts->DelayPerLine,FALSE);
			SendDlgItemMessage(Dialog, IDC_SERIALDELAYLINE, EM_LIMITTEXT,4, 0);

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
					if ( ts!=NULL ) {
						memset(Temp, 0, sizeof(Temp));
						GetDlgItemText(Dialog, IDC_SERIALPORT, Temp, sizeof(Temp)-1);
						if (strncmp(Temp, "COM", 3) == 0 && Temp[3] != '\0') {
							ts->ComPort = (WORD)atoi(&Temp[3]);
						}

						GetDlgItemText(Dialog, IDC_SERIALBAUD, Temp, sizeof(Temp)-1);
						if (atoi(Temp) != 0) {
							ts->Baud = (DWORD)atoi(Temp);
						}
						if ((w = (WORD)GetCurSel(Dialog, IDC_SERIALDATA)) > 0) {
							ts->DataBit = w;
						}
						if ((w = (WORD)GetCurSel(Dialog, IDC_SERIALPARITY)) > 0) {
							ts->Parity = w;
						}
						if ((w = (WORD)GetCurSel(Dialog, IDC_SERIALSTOP)) > 0) {
							ts->StopBit = w;
						}
						if ((w = (WORD)GetCurSel(Dialog, IDC_SERIALFLOW)) > 0) {
							ts->Flow = w;
						}

						ts->DelayPerChar = GetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,NULL,FALSE);

						ts->DelayPerLine = GetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,NULL,FALSE);

						ts->PortType = IdSerial;

						// ボーレートが変更されることがあるので、タイトル再表示の
						// メッセージを飛ばすようにした。 (2007.7.21 maya)
						PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);
					}

					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_SERIALHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

static INT_PTR CALLBACK TCPIPDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_TCPIP_TITLE" },
		{ IDC_TCPIPHOSTLIST, "DLG_TCPIP_HOSTLIST" },
		{ IDC_TCPIPADD, "DLG_TCPIP_ADD" },
		{ IDC_TCPIPUP, "DLG_TCPIP_UP" },
		{ IDC_TCPIPREMOVE, "DLG_TCPIP_REMOVE" },
		{ IDC_TCPIPDOWN, "DLG_TCPIP_DOWN" },
		{ IDC_TCPIPHISTORY, "DLG_TCPIP_HISTORY" },
		{ IDC_TCPIPAUTOCLOSE, "DLG_TCPIP_AUTOCLOSE" },
		{ IDC_TCPIPPORTLABEL, "DLG_TCPIP_PORT" },
		{ IDC_TCPIPTELNET, "DLG_TCPIP_TELNET" },
		{ IDC_TCPIPTELNETKEEPALIVELABEL, "DLG_TCPIP_KEEPALIVE" },
		{ IDC_TCPIPTELNETKEEPALIVESEC, "DLG_TCPIP_KEEPALIVE_SEC" },
		{ IDC_TCPIPTERMTYPELABEL, "DLG_TCPIP_TERMTYPE" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_TCPIPHELP, "BTN_HELP" },
	};
	PTTSet ts;
	char EntName[10];
	char TempHost[HostNameMaxLength+1];
	UINT i, Index;
	WORD w;
	BOOL Ok;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
				
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
			} while (i <= MAXHOSTLIST);

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

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
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
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
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
						ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
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

static INT_PTR CALLBACK HostDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_HOST_TITLE" },
		{ IDC_HOSTNAMELABEL, "DLG_HOST_TCPIPHOST" },
		{ IDC_HOSTTCPPORTLABEL, "DLG_HOST_TCPIPPORT" },
		{ IDC_HOSTTCPPROTOCOLLABEL, "DLG_HOST_TCPIPPROTOCOL" },
		{ IDC_HOSTSERIAL, "DLG_HOST_SERIAL" },
		{ IDC_HOSTCOMLABEL, "DLG_HOST_SERIALPORT" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_HOSTHELP, "BTN_HELP" },
	};
	PGetHNRec GetHNRec;
	char EntName[128];
	char TempHost[HostNameMaxLength+1];
	WORD i, j, w;
	BOOL Ok;
	WORD ComPortTable[MAXCOMPORT];
	static char *ComPortDesc[MAXCOMPORT];
	int comports;

	switch (Message) {
		case WM_INITDIALOG:
			GetHNRec = (PGetHNRec)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
		
			// ファイルおよび名前付きパイプの場合、TCP/IP扱いとする。
			if ( GetHNRec->PortType==IdFile ||
				 GetHNRec->PortType==IdNamedPipe
				) {
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
			} while (i <= MAXHOSTLIST);

			SendDlgItemMessage(Dialog, IDC_HOSTNAME, EM_LIMITTEXT,
			                   HostNameMaxLength-1, 0);

			SendDlgItemMessage(Dialog, IDC_HOSTNAME, CB_SETCURSEL,0,0);

			SetEditboxSubclass(Dialog, IDC_HOSTNAME, TRUE);

			SetRB(Dialog,GetHNRec->Telnet,IDC_HOSTTELNET,IDC_HOSTTELNET);
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPORT, EM_LIMITTEXT,5,0);
			SetDlgItemInt(Dialog,IDC_HOSTTCPPORT,GetHNRec->TCPPort,FALSE);
			for (i=0; ProtocolFamilyList[i]; ++i) {
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_ADDSTRING,
				                   0, (LPARAM)ProtocolFamilyList[i]);
			}
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, EM_LIMITTEXT,
			                   ProtocolFamilyMaxLength-1, 0);
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_SETCURSEL,0,0);

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
			else {
				DisableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
				DisableDlgItem(Dialog,IDC_HOSTTCPPROTOCOLLABEL,IDC_HOSTTCPPROTOCOL);
			}

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					GetHNRec = (PGetHNRec)GetWindowLongPtr(Dialog,DWLP_USER);
					if ( GetHNRec!=NULL ) {
						char afstr[BUFSIZ];
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
#define getaf(str) \
	((strcmp((str), "IPv6") == 0) ? AF_INET6 : \
	((strcmp((str), "IPv4") == 0) ? AF_INET : AF_UNSPEC))
						memset(afstr, 0, sizeof(afstr));
						GetDlgItemText(Dialog, IDC_HOSTTCPPROTOCOL, afstr, sizeof(afstr));
						GetHNRec->ProtocolFamily = getaf(afstr);
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
					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_HOSTTCPIP:
					EnableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
					EnableDlgItem(Dialog,IDC_HOSTTCPPROTOCOLLABEL,IDC_HOSTTCPPROTOCOL);
					DisableDlgItem(Dialog,IDC_HOSTCOMLABEL,IDC_HOSTCOM);
					return TRUE;

				case IDC_HOSTSERIAL:
					EnableDlgItem(Dialog,IDC_HOSTCOMLABEL,IDC_HOSTCOM);
					DisableDlgItem(Dialog,IDC_HOSTNAMELABEL,IDC_HOSTTCPPORT);
					DisableDlgItem(Dialog,IDC_HOSTTCPPROTOCOLLABEL,IDC_HOSTTCPPROTOCOL);
					break;

				case IDC_HOSTTELNET:
					GetRB(Dialog,&i,IDC_HOSTTELNET,IDC_HOSTTELNET);
					if ( i==1 ) {
						GetHNRec = (PGetHNRec)GetWindowLongPtr(Dialog,DWLP_USER);
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

static INT_PTR CALLBACK DirDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_DIR_TITLE" },
		{ IDC_DIRCURRENT_LABEL, "DLG_DIR_CURRENT" },
		{ IDC_DIRNEW_LABEL, "DLG_DIR_NEW" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_DIRHELP, "BTN_HELP" },
	};
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
	char buf[MAX_PATH], buf2[MAX_PATH];

	switch (Message) {
		case WM_INITDIALOG:

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
				
			CurDir = (PCHAR)(lParam);
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

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

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					CurDir = (PCHAR)GetWindowLongPtr(Dialog,DWLP_USER);
					if ( CurDir!=NULL ) {
						char FileDirExpanded[MAX_PATH];
						_getcwd(HomeDir,sizeof(HomeDir));
						ExpandEnvironmentStrings(CurDir, FileDirExpanded, sizeof(FileDirExpanded));
						_chdir(FileDirExpanded);
						GetDlgItemText(Dialog, IDC_DIRNEW, TmpDir, sizeof(TmpDir));
						if ( strlen(TmpDir)>0 ) {
							ExpandEnvironmentStrings(TmpDir, FileDirExpanded, sizeof(FileDirExpanded));
							if (_chdir(FileDirExpanded) != 0) {
								get_lang_msg("MSG_TT_ERROR", uimsg2, sizeof(uimsg2), "Tera Term: Error", UILanguageFile);
								get_lang_msg("MSG_FIND_DIR_ERROR", uimsg, sizeof(uimsg), "Cannot find directory", UILanguageFile);
								MessageBox(Dialog,uimsg,uimsg2,MB_ICONEXCLAMATION);
								_chdir(HomeDir);
								return TRUE;
							}
							strncpy_s(CurDir, MAXPATHLEN, TmpDir, _TRUNCATE);
						}
						_chdir(HomeDir);
					}
					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_SELECT_DIR:
					get_lang_msg("DLG_SELECT_DIR_TITLE", uimsg, sizeof(uimsg),
					             "Select new directory", UILanguageFile);
					{
						char FileDirExpanded[MAX_PATH];
						GetDlgItemText(Dialog, IDC_DIRNEW, buf, sizeof(buf));
						ExpandEnvironmentStrings(buf, FileDirExpanded, sizeof(FileDirExpanded));
						if (doSelectFolder(Dialog, buf2, sizeof(buf2), FileDirExpanded, uimsg)) {
							SetDlgItemText(Dialog, IDC_DIRNEW, buf2);
						}
					}
					return TRUE;

				case IDC_DIRHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
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

	// シングルクリックでブラウザが起動するように変更する。(2015.11.16 yutaka)
	//case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
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

	//SetCapture(hWnd);

	if (!IsWindow(hWnd)) {
		return;
	}

	// 親のプロシージャをサブクラスから参照できるように、ポインタを登録しておく。
	SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)parent );
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

#if defined(_MSC_VER)
// ビルドしたときに使われたVisual C++のバージョンを取得する(2009.3.3 yutaka)
static void GetCompilerInfo(char *buf, size_t buf_size)
{
	char tmpbuf[128];
	int msc_ver, vs_ver, msc_low_ver;

	strcpy(buf, "Microsoft Visual C++");
#ifdef _MSC_FULL_VER
	// _MSC_VER  VS Ver.  VS internal Ver.  MSVC++ Ver.
	// 1400      2005     8.0               8.0
	// 1500      2008     9.0               9.0
	// 1600      2010     10.0              10.0
	// 1700      2012     11.0              11.0
	// 1800      2013     12.0              12.0
	// 1900      2015     14.0              14.0
	// 1910      2017     15.0              14.10
	// 1910      2017     15.1              14.10
	// 1910      2017     15.2              14.10
	// 1911      2017     15.3.x            14.11
	// 1911      2017     15.4.x            14.11
	// 1912      2017     15.5.x            14.12
	// 1913      2017     15.6.x            14.13
	// 1914      2017     15.7.x            14.14
	// 1915      2017     15.8.x            14.15
	// 1916      2017     15.9.x            14.16
	// 1920      2019     16.0.x            14.20
	// 1921      2019     16.1.x            14.21
	msc_ver = (_MSC_FULL_VER / 10000000);
	msc_low_ver = (_MSC_FULL_VER / 100000) % 100;
	if (msc_ver < 19) {
		vs_ver = msc_ver - 6;
	}
	else {
		vs_ver = msc_ver - 5;
	}

	_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " %d.%d",
				vs_ver,
				msc_low_ver);
	strncat_s(buf, buf_size, tmpbuf, _TRUNCATE);
	if (_MSC_FULL_VER % 100000) {
		_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " build %d",
					_MSC_FULL_VER % 100000);
		strncat_s(buf, buf_size, tmpbuf, _TRUNCATE);
	}
#elif defined(_MSC_VER)
	_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " %d.%d",
				(_MSC_VER / 100) - 6,
				_MSC_VER % 100);
	strncat_s(buf, buf_size, tmpbuf, _TRUNCATE);
#endif
}

#elif defined(__MINGW32__)
static void GetCompilerInfo(char *buf, size_t buf_size)
{
#if defined(__GNUC__) || defined(__clang__)
	_snprintf_s(buf, buf_size, _TRUNCATE,
				"mingw " __MINGW64_VERSION_STR " "
#if defined(__clang__)
				"clang " __clang_version__
#elif defined(__GNUC__)
				"gcc " __VERSION__
#endif
		);
#else
	strncat_s(buf, buf_size, "mingw", _TRUNCATE);
#endif
}

#else
static void GetCompilerInfo(char *buf, size_t buf_size)
{
	strncpy_s(buf, buf_size, "unknown compiler");
}
#endif

static INT_PTR CALLBACK AboutDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_ABOUT_TITLE" },
	};
	char buf[128], tmpbuf[128];
	HDC hdc;
	HWND hwnd;
	RECT r;
	DWORD dwExt;
	WORD w, h;
	POINT point;
	char uimsg[MAX_UIMSG];

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
	// for animation
	static HDC dlgdc = NULL;
	static int dlgw, dlgh;
	static HBITMAP dlgbmp = NULL, dlgprevbmp = NULL;
	static LPDWORD dlgpixel = NULL;
	static HICON dlghicon = NULL;
	const int icon_x = 15, icon_y = 10, icon_w = 32, icon_h = 32;
	const int ID_EFFECT_TIMER = 1;
	RECT dlgrc = {0};
	BITMAPINFO       bmi;
	BITMAPINFOHEADER bmiHeader;
	int x, y;
#define POS(x,y) ((x) + (y)*dlgw)
	static short *wavemap = NULL;
	static short *wavemap_old = NULL;
	static LPDWORD dlgorgpixel = NULL;
	static int waveflag = 0;
	static int fullcolor = 0;
	int bitspixel;
#endif

	switch (Message) {
		case WM_INITDIALOG:
			// アイコンを動的にセット
			{
				int fuLoad = LR_DEFAULTCOLOR;
				HICON hicon;

				if (IsWindowsNT4()) {
					fuLoad = LR_VGACOLOR;
				}

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
				hicon = LoadImage(hInst, MAKEINTRESOURCE(IDI_TTERM),
				                  IMAGE_ICON, icon_w, icon_h, fuLoad);
				// Picture Control に描画すると、なぜか透過色が透過にならず、黒となってしまうため、
				// WM_PAINT で描画する。
				dlghicon = hicon;
#else
				hicon = LoadImage(hInst, MAKEINTRESOURCE(IDI_TTERM),
				                  IMAGE_ICON, 32, 32, fuLoad);
				SendDlgItemMessage(Dialog, IDC_TT_ICON, STM_SETICON, (WPARAM)hicon, 0);
#endif
			}

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);

			// Tera Term 本体のバージョン
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Version %d.%d", TT_VERSION_MAJOR, TT_VERSION_MINOR);
#ifdef SVNVERSION
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " (SVN# %d)", SVNVERSION);
			strncat_s(buf, sizeof(buf), tmpbuf, _TRUNCATE);
#else
			_snprintf_s(tmpbuf, sizeof(tmpbuf), _TRUNCATE, " (SVN# UNK)");
			strncat_s(buf, sizeof(buf), tmpbuf, _TRUNCATE);
#endif
			SetDlgItemTextA(Dialog, IDC_TT_VERSION, buf);

			// Onigurumaのバージョンを設定する
			// バージョンの取得は onig_version() を呼び出すのが適切だが、そのためだけにライブラリを
			// リンクしたくなかったので、以下のようにした。Onigurumaのバージョンが上がった場合、
			// ビルドエラーとなるかもしれない。
			// (2005.10.8 yutaka)
			// ライブラリをリンクし、正規の手順でバージョンを得ることにした。
			// (2006.7.24 yutaka)
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Oniguruma %s", onig_version());
			SetDlgItemTextA(Dialog, IDC_ONIGURUMA_LABEL, buf);

			// ビルドしたときに使われたコンパイラを設定する。(2009.3.3 yutaka)
			GetCompilerInfo(tmpbuf, sizeof(tmpbuf));
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Built using %s", tmpbuf);
			SetDlgItemTextA(Dialog, IDC_BUILDTOOL, buf);

			// ビルドタイムを設定する。(2009.3.4 yutaka)
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Build time: %s %s", __DATE__, __TIME__);
			SetDlgItemTextA(Dialog, IDC_BUILDTIME, buf);

			// static text のサイズを変更 (2007.4.16 maya)
			hwnd = GetDlgItem(Dialog, IDC_AUTHOR_URL);
			hdc = GetDC(hwnd);
			SelectObject(hdc, (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0));
			GetDlgItemTextA(Dialog, IDC_AUTHOR_URL, uimsg, sizeof(uimsg));
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
			SelectObject(hdc, (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0));
			GetDlgItemTextA(Dialog, IDC_FORUM_URL, uimsg, sizeof(uimsg));
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

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
			/*
			 * ダイアログのビットマップ化を行い、背景にエフェクトをかけられるようにする。
			 * (2011.5.7 yutaka)
			 */
			// ダイアログのサイズ
			GetWindowRect(Dialog, &dlgrc);
			dlgw = dlgrc.right - dlgrc.left;
			dlgh = dlgrc.bottom - dlgrc.top;
			// ビットマップの作成
			dlgdc = CreateCompatibleDC(NULL);
			ZeroMemory(&bmiHeader, sizeof(BITMAPINFOHEADER));
			bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
			bmiHeader.biWidth     = dlgw;
			bmiHeader.biHeight    = -dlgh;
			bmiHeader.biPlanes    = 1;
			bmiHeader.biBitCount  = 32;
			bmi.bmiHeader = bmiHeader;
			dlgbmp = CreateDIBSection(NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &dlgpixel, NULL, 0);
			dlgprevbmp = (HBITMAP)SelectObject(dlgdc, dlgbmp);
			// ビットマップの背景色（朝焼けっぽい）を作る。
			for (y = 0 ; y < dlgh ; y++) {
				double dx = (double)(255 - 180) / dlgw;
				double dy = (double)255/dlgh;
				BYTE r, g, b;
				for (x = 0 ; x < dlgw ; x++) {
					r = min((int)(180+dx*x), 255);
					g = min((int)(180+dx*x), 255);
					b = max((int)(255-y*dx), 0);
					// 画素の並びは、下位バイトからB, G, R, Aとなる。
					dlgpixel[POS(x, y)] = b | g << 8 | r << 16;
				}
			}
			// 2D Water effect 用
			wavemap = calloc(sizeof(short), dlgw * dlgh);
			wavemap_old = calloc(sizeof(short), dlgw * dlgh);
			dlgorgpixel = calloc(sizeof(DWORD), dlgw * dlgh);
			memcpy(dlgorgpixel, dlgpixel, dlgw * dlgh * sizeof(DWORD));

			srand((unsigned int)time(NULL));


#ifdef EFFECT_ENABLED
			// エフェクトタイマーの開始
			SetTimer(Dialog, ID_EFFECT_TIMER, 100, NULL);
#endif

			// 画面の色数を調べる。
			hwnd = GetDesktopWindow();
			hdc = GetDC(hwnd);
			bitspixel = GetDeviceCaps(hdc, BITSPIXEL);
			fullcolor = (bitspixel == 32 ? 1 : 0);
			ReleaseDC(hwnd, hdc);
#endif

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
#ifdef EFFECT_ENABLED
			switch (LOWORD(wParam)) {
				int val;
				case IDOK:
					val = 1;
				case IDCANCEL:
					val = 0;
					KillTimer(Dialog, ID_EFFECT_TIMER);

					SelectObject(dlgdc, dlgprevbmp);
					DeleteObject(dlgbmp);
					DeleteDC(dlgdc);
					dlgdc = NULL;
					dlgprevbmp = dlgbmp = NULL;

					free(wavemap);
					free(wavemap_old);
					free(dlgorgpixel);

					EndDialog(Dialog, val);
					return TRUE;
			}
#else
			switch (LOWORD(wParam)) {
				case IDOK:
					EndDialog(Dialog, 1);
					return TRUE;
				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;
			}
#endif
			break;

#if defined(EFFECT_ENABLED) || defined(TEXTURE_ENABLED)
		// static textの背景を透過させる。
		case WM_CTLCOLORSTATIC:
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (BOOL)GetStockObject( NULL_BRUSH );
			break;

#ifdef EFFECT_ENABLED
		case WM_ERASEBKGND:
			return 0;
#endif

		case WM_PAINT:
			if (dlgdc) {
				PAINTSTRUCT ps;
				hdc = BeginPaint(Dialog, &ps);

				if (fullcolor) {
					BitBlt(hdc,
						ps.rcPaint.left, ps.rcPaint.top,
						ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
						dlgdc,
						ps.rcPaint.left, ps.rcPaint.top,
						SRCCOPY);
				}

				DrawIconEx(hdc, icon_x, icon_y, dlghicon, icon_w, icon_h, 0, 0, DI_NORMAL);

				EndPaint(Dialog, &ps);
			}
			break;

		case WM_MOUSEMOVE:
			{
				int xpos, ypos;
				static int idx = 0;
				short amplitudes[4] = {250, 425, 350, 650};

				xpos = LOWORD(lParam);
				ypos = HIWORD(lParam);

				wavemap[POS(xpos,ypos)] = amplitudes[idx++];
				idx %= 4;
			}
			break;

		case WM_TIMER:
			if (wParam == ID_EFFECT_TIMER)
			{
				int x, y;
				short height, xdiff;
				short *p_new, *p_old;

				if (waveflag == 0) {
					p_new = wavemap;
					p_old = wavemap_old;
				} else {
					p_new = wavemap_old;
					p_old = wavemap;
				}
				waveflag ^= 1;

				// 水面の計算
				// アルゴリズムは下記サイト(2D Water)より。
				// cf. http://freespace.virgin.net/hugo.elias/graphics/x_water.htm
				for (y = 1; y < dlgh - 1 ; y++) {
					for (x = 1; x < dlgw - 1 ; x++) {
						height = (p_new[POS(x,y-1)] +
							      p_new[POS(x-1,y)] +
							      p_new[POS(x+1,y)] +
							      p_new[POS(x,y+1)]) / 2 - p_old[POS(x,y)];
						height -= (height >> 5);
						p_old[POS(x,y)] = height;
					}
				}

				// 水面の描画
				for (y = 1; y < dlgh - 1 ; y++) {
					for (x = 1; x < dlgw - 1 ; x++) {
						xdiff = p_old[POS(x+1,y)] - p_old[POS(x,y)];
						dlgpixel[POS(x,y)] = dlgorgpixel[POS(x + xdiff, y)];
					}
				}

#if 0
				hdc = GetDC(Dialog);
				BitBlt(hdc,
					0, 0, dlgw, dlgh,
					dlgdc,
					0, 0,
					SRCCOPY);
				ReleaseDC(Dialog, hdc);
#endif

				InvalidateRect(Dialog, NULL, FALSE);
			}
			break;
#endif

	}
	return FALSE;
}

static PCHAR LangList[] = {"English","Japanese","Russian","Korean","UTF-8",NULL};
static char **LangUIList = NULL;
#define LANG_PATH "lang"
#define LANG_EXT ".lng"

// メモリフリー
static void free_lang_ui_list()
{
	if (LangUIList) {
		char **p = LangUIList;
		while (*p) {
			free(*p);
			p++;
		}
		free(LangUIList);
		LangUIList = NULL;
	}
}

static int make_sel_lang_ui(char *HomeDir)
{
	int    i;
	int    file_num;
	char   fullpath[1024];
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	free_lang_ui_list();

	_snprintf_s(fullpath, sizeof(fullpath), _TRUNCATE, "%s\\%s\\*%s", HomeDir, LANG_PATH, LANG_EXT);

	file_num = 0;
	hFind = FindFirstFile(fullpath,&fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				file_num++;
			}
		} while(FindNextFile(hFind,&fd));
		FindClose(hFind);
	}

	file_num++;  // NULL
	LangUIList = calloc(file_num, sizeof(char *));

	i = 0;
	hFind = FindFirstFile(fullpath,&fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				LangUIList[i++] = _strdup(fd.cFileName);
			}
		} while(FindNextFile(hFind,&fd) && i < file_num);
		FindClose(hFind);
	}
	LangUIList[i] = NULL;

	return i;
}

static int get_sel_lang_ui(char **list, char *selstr)
{
	int n = 0;
	char **p = list;

	if (selstr == NULL || selstr[0] == '\0') {
		n = 0;  // English
	} else {
		while (*p) {
			if (strstr(selstr, *p)) {
				n = p - list;
				break;
			}
			p++;
		}
	}

	return (n + 1);  // 1origin
}

static INT_PTR CALLBACK GenDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_GEN_TITLE" },
		{ IDC_GENPORT_LABEL, "DLG_GEN_PORT" },
		{ IDC_GENLANGLABEL, "DLG_GEN_LANG" },
		{ IDC_GENLANGUI_LABEL, "DLG_GEN_LANG_UI" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_GENHELP, "BTN_HELP" },
	};
	static int langui_sel = 1, uilist_count = 0;
	PTTSet ts;
	WORD w;

	switch (Message) {
		case WM_INITDIALOG:
			ts = (PTTSet)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
		
			SendDlgItemMessageA(Dialog, IDC_GENPORT, CB_ADDSTRING,
			                   0, (LPARAM)"TCP/IP");
			for (w=1;w<=ts->MaxComPort;w++) {
				char Temp[8];
				_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "COM%d", w);
				SendDlgItemMessageA(Dialog, IDC_GENPORT, CB_ADDSTRING,
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

			// 最初に指定されている言語ファイルの番号を覚えておく。
			uilist_count = make_sel_lang_ui(ts->HomeDir);
			langui_sel = get_sel_lang_ui(LangUIList, ts->UILanguageFile_ini);
			SetDropDownList(Dialog, IDC_GENLANG_UI, LangUIList, langui_sel);
			if (LangUIList[0] == NULL) {
				EnableWindow(GetDlgItem(Dialog, IDC_GENLANG_UI), FALSE);
			}

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
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
							if (1 <= language && language <= IdLangMax && language != ts->Language) {
								WORD KanjiCode = ts->KanjiCode;
								WORD KanjiCodeSend = ts->KanjiCodeSend;
								ts->KanjiCode = KanjiCodeTranslate(language,KanjiCode);
								ts->KanjiCodeSend = KanjiCodeTranslate(language,KanjiCodeSend);

								ts->Language = language;
							}

						}

						// 言語ファイルが変更されていた場合
						w = (WORD)GetCurSel(Dialog, IDC_GENLANG_UI);
						if (1 <= w && w <= uilist_count && w != langui_sel) {
							_snprintf_s(ts->UILanguageFile_ini, sizeof(ts->UILanguageFile_ini), _TRUNCATE,
								"%s\\%s", LANG_PATH, LangUIList[w - 1]);

							GetUILanguageFileFull(ts->HomeDir, ts->UILanguageFile_ini,
												  ts->UILanguageFile, sizeof(ts->UILanguageFile));

							// タイトルの更新を行う。(2014.2.23 yutaka)
							PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);
						}
					}

					// TTXKanjiMenu は Language を見てメニューを表示するので、変更の可能性がある
					// OK 押下時にメニュー再描画のメッセージを飛ばすようにした。 (2007.7.14 maya)
					// 言語ファイルの変更時にメニューの再描画が必要 (2012.5.5 maya)
					PostMessage(GetParent(Dialog),WM_USER_CHANGEMENU,0,0);

					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_GENHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
			break;

		case WM_DESTROY:
			free_lang_ui_list();
			break;
	}
	return FALSE;
}

static INT_PTR CALLBACK WinListDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_WINLIST_TITLE" },
		{ IDC_WINLISTLABEL, "DLG_WINLIST_LABEL" },
		{ IDOK, "DLG_WINLIST_OPEN" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_WINLISTCLOSE, "DLG_WINLIST_CLOSEWIN" },
		{ IDC_WINLISTHELP, "BTN_HELP" },
	};
	PBOOL Close;
	int n;
	HWND Hw;

	switch (Message) {
		case WM_INITDIALOG:
			Close = (PBOOL)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

			SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
		
			SetWinList(GetParent(Dialog),Dialog,IDC_WINLISTLIST);

			CenterWindow(Dialog, GetParent(Dialog));

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
					return TRUE;

				case IDCANCEL:
					EndDialog(Dialog, 0);
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
					}
					else {
						Close = (PBOOL)GetWindowLongPtr(Dialog,DWLP_USER);
						if (Close!=NULL) {
							*Close = TRUE;
						}
						EndDialog(Dialog, 1);
						return TRUE;
					}
					break;

				case IDC_WINLISTHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,0,0);
			}
	}
	return FALSE;
}

BOOL WINAPI _SetupTerminal(HWND WndParent, PTTSet ts)
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

BOOL WINAPI _SetupWin(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_WINDLG),
		                     WndParent, WinDlg, (LPARAM)ts);
}

BOOL WINAPI _SetupKeyboard(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_KEYBDLG),
		                     WndParent, KeybDlg, (LPARAM)ts);
}

BOOL WINAPI _SetupSerialPort(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_SERIALDLG),
		                     WndParent, SerialDlg, (LPARAM)ts);
}

BOOL WINAPI _SetupTCPIP(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_TCPIPDLG),
		                     WndParent, TCPIPDlg, (LPARAM)ts);
}

BOOL WINAPI _GetHostName(HWND WndParent, PGetHNRec GetHNRec)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_HOSTDLG),
		                     WndParent, HostDlg, (LPARAM)GetHNRec);
}

BOOL WINAPI _ChangeDirectory(HWND WndParent, PCHAR CurDir)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_DIRDLG),
		                     WndParent, DirDlg, (LPARAM)CurDir);
}

BOOL WINAPI _AboutDialog(HWND WndParent)
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
		{
			static LPCHOOSEFONT cf;
			PTTSet ts;
			char uimsg[MAX_UIMSG];

			//EnableWindow(GetDlgItem(Dialog, cmb2), FALSE);
			cf = (LPCHOOSEFONT)lParam;
			ts = (PTTSet)cf->lCustData;
			get_lang_msg("DLG_CHOOSEFONT_STC6", uimsg, sizeof(uimsg),
			             "\"Font style\" selection here won't affect actual font appearance.", ts->UILanguageFile);
			SetDlgItemText(Dialog, stc6, uimsg);

			SetFocus(GetDlgItem(Dialog,cmb1));

			CenterWindow(Dialog, GetParent(Dialog));

			break;
		}
#if 0
		case WM_COMMAND:
			if (LOWORD(wParam) == cmb2) {
				if (HIWORD(wParam) == CBN_SELCHANGE) {
					// フォントの変更による(メッセージによる)スタイルの変更では
					// cmb2 からの通知が来ない
					SendMessage(GetDlgItem(Dialog, cmb2), CB_GETCURSEL, 0, 0);
				}
			}
			else if (LOWORD(wParam) == cmb1) {
				if (HIWORD(wParam) == CBN_SELCHANGE) {
					// フォントの変更前に一時保存されたスタイルが
					// ここを抜けたあとに改めてセットされてしまうようだ
					SendMessage(GetDlgItem(Dialog, cmb2), CB_GETCURSEL, 0, 0);
				}
			}
			break;
#endif
	}
	return FALSE;
}

#ifndef CF_INACTIVEFONTS
#define CF_INACTIVEFONTS 0x02000000L
#endif
BOOL WINAPI _ChooseFontDlg(HWND WndParent, LPLOGFONTA LogFont, PTTSet ts)
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
	if (ts->ListHiddenFonts) {
		cf.Flags |= CF_INACTIVEFONTS;
	}
	cf.lpfnHook = (LPCFHOOKPROC)(&TFontHook);
	cf.nFontType = REGULAR_FONTTYPE;
	cf.hInstance = hInst;
	cf.lCustData = (LPARAM)ts;
	Ok = ChooseFont(&cf);
	return Ok;
}

BOOL WINAPI _SetupGeneral(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_GENDLG),
		                     WndParent, GenDlg, (LPARAM)ts);
}

BOOL WINAPI _WindowWindow(HWND WndParent, PBOOL Close)
{
	*Close = FALSE;
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_WINLISTDLG),
		                     WndParent,
							 WinListDlg, (LPARAM)Close);
}

BOOL WINAPI _TTDLGSetUILanguageFile(char *file)
{
	strncpy_s(UILanguageFile, sizeof(UILanguageFile), file, _TRUNCATE);
	return TRUE;
}
