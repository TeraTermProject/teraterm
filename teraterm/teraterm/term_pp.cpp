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

#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_charset.h"
#include "ttlib.h"
#include "dlglib.h"
#include "dlg_res.h"
#include "helpid.h"
#include "ttlib_charset.h"
#include "asprintf.h"
#include "ttwinman.h"
#include "helpid.h"

#include "term_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

static const char *NLListRcv[] = {"CR", "CR+LF", "LF", "AUTO", NULL};
static const char *NLList[] = {"CR","CR+LF", "LF", NULL};

typedef struct {
	TTTSet *pts;
	HWND VTWin;
} DialogData;

static INT_PTR CALLBACK TermDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfosCom[] = {
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
	};
	WORD w;

	switch (Message) {
		case WM_INITDIALOG: {
			DialogData *data = (DialogData *)(((PROPSHEETPAGEW *)lParam)->lParam);
			TTTSet *ts = data->pts;
			SetWindowLongPtrW(Dialog, DWLP_USER, (LONG_PTR)data);

			SetDlgTextsW(Dialog, TextInfosCom, _countof(TextInfosCom), ts->UILanguageFileW);

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

			int cur_sel = -1;
			for (int i = 0;; i++) {
				const TermIDList *term_id = TermIDGetList(i);
				if (term_id == NULL) {
					break;
				}
				int index = (int)SendDlgItemMessageA(Dialog, IDC_TERMID,
													 CB_ADDSTRING, 0, (LPARAM)term_id->TermIDStr);
				SendDlgItemMessageA(Dialog, IDC_TERMID, CB_SETITEMDATA, index, (LPARAM)term_id->TermID);
				if (term_id->TermID == ts->TerminalID) {
					cur_sel = index;
				}
			}
			SendDlgItemMessage(Dialog, IDC_TERMID, CB_SETCURSEL, cur_sel, 0);

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

			return TRUE;
		}

		case WM_NOTIFY: {
			DialogData *data = (DialogData *)GetWindowLongPtrW(Dialog, DWLP_USER);
			TTTSet *ts;
			assert(data != NULL);
			ts = data->pts;
			NMHDR *nmhdr = (NMHDR *)lParam;
			if (nmhdr->code == PSN_APPLY) {
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

				LRESULT sel = SendDlgItemMessageA(Dialog, IDC_TERMID, CB_GETCURSEL, 0, 0);
				ts->TerminalID = (int)SendDlgItemMessageA(Dialog, IDC_TERMID, CB_GETITEMDATA, sel, 0);

				GetRB(Dialog,&ts->LocalEcho,IDC_TERMLOCALECHO,IDC_TERMLOCALECHO);

				if ((ts->FTFlag & FT_BPAUTO)==0) {
					char Temp[81];
					GetDlgItemText(Dialog, IDC_TERMANSBACK,Temp,sizeof(Temp));
					ts->AnswerbackLen = Hex2Str(Temp,ts->Answerback,sizeof(ts->Answerback));
				}

				GetRB(Dialog,&ts->AutoWinSwitch,IDC_TERMAUTOSWITCH,IDC_TERMAUTOSWITCH);

				return TRUE;
			}
			else if (nmhdr->code == PSN_HELP) {
				const WPARAM HelpId = HlpMenuSetupAdditionalTerminal;
				PostMessage(data->VTWin, WM_USER_DLGHELP2, HelpId, 0);
				break;
			}
			break;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case IDC_TERMISWIN:
					GetRB(Dialog,&w,IDC_TERMISWIN,IDC_TERMISWIN);
					if ( w==0 ) {
						EnableDlgItem(Dialog,IDC_TERMRESIZE,IDC_TERMRESIZE);
					}
					else {
						DisableDlgItem(Dialog,IDC_TERMRESIZE,IDC_TERMRESIZE);
					}
					break;
			}
			break;
		}
		default:
			break;
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

HPROPSHEETPAGE CreateTerminalPP(HINSTANCE inst, HWND vtwin, TTTSet *pts)
{
	DialogData *data = (DialogData *)calloc(1, sizeof(*data));
	data->pts = &ts;
	data->VTWin = vtwin;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", "DLG_TERM_TITLE", L"Terminal", pts->UILanguageFileW, &uimsg);
	psp.pszTitle = uimsg;
	psp.pszTemplate = MAKEINTRESOURCEW(IDD_TERMDLG);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(IDD_TERMDLG));
#endif
	psp.pfnDlgProc = TermDlg;
	psp.lParam = (LPARAM)data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)&psp);
	free(uimsg);
	return hpsp;
}
