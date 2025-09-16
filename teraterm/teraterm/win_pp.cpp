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

// WinDlg

#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "dlg_res.h"
#include "tipwin.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "asprintf.h"
#include "color_sample.h"
#include "tslib.h"

#include "win_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

typedef struct {
	WORD TmpColor[12][6];
	PTTSet ts;
	WORD VTFlag;
	HWND VTWin;
	ColorSample *cs;
	HFONT sample_font;
} WinDlgWork;

static void DispSample(HWND Dialog, WinDlgWork *work, int IAttr)
{
	COLORREF Text, Back;
	Text = RGB(work->TmpColor[IAttr][0],
	           work->TmpColor[IAttr][1],
	           work->TmpColor[IAttr][2]);
	Back = RGB(work->TmpColor[IAttr][3],
	           work->TmpColor[IAttr][4],
	           work->TmpColor[IAttr][5]);
	ColorSampleSetColor(work->cs, Text, Back);
}

static void ChangeColor(HWND Dialog, WinDlgWork *work, int IAttr, int IOffset)
{
	SetDlgItemInt(Dialog,IDC_WINRED,work->TmpColor[IAttr][IOffset],FALSE);
	SetDlgItemInt(Dialog,IDC_WINGREEN,work->TmpColor[IAttr][IOffset+1],FALSE);
	SetDlgItemInt(Dialog,IDC_WINBLUE,work->TmpColor[IAttr][IOffset+2],FALSE);

	DispSample(Dialog,work,IAttr);
}

static void ChangeSB(HWND Dialog, WinDlgWork *work, int IAttr, int IOffset)
{
	HWND HRed, HGreen, HBlue;

	HRed = GetDlgItem(Dialog, IDC_WINREDBAR);
	HGreen = GetDlgItem(Dialog, IDC_WINGREENBAR);
	HBlue = GetDlgItem(Dialog, IDC_WINBLUEBAR);

	SetScrollPos(HRed,SB_CTL,work->TmpColor[IAttr][IOffset+0],TRUE);
	SetScrollPos(HGreen,SB_CTL,work->TmpColor[IAttr][IOffset+1],TRUE);
	SetScrollPos(HBlue,SB_CTL,work->TmpColor[IAttr][IOffset+2],TRUE);
	ChangeColor(Dialog,work,IAttr,IOffset);
}

static void RestoreVar(HWND Dialog, WinDlgWork* work, int *IAttr, int *IOffset)
{
	WORD w;

	GetRB(Dialog,&w,IDC_WINTEXT,IDC_WINBACK);
	if (w==2) {
		*IOffset = 3;
	}
	else {
		*IOffset = 0;
	}
	if (work->VTFlag>0) {
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
		{ IDC_WINTITLELABEL, "DLG_WIN_TITLELABEL" },
		{ IDC_WINCURSOR, "DLG_WIN_CURSOR" },
		{ IDC_WINBLOCK, "DLG_WIN_BLOCK" },
		{ IDC_WINVERT, "DLG_WIN_VERT" },
		{ IDC_WINHORZ, "DLG_WIN_HORZ" },
		{ IDC_FONTBOLD, "DLG_WIN_BOLDFONT" },
		{ IDC_WINHIDETITLE, "DLG_WIN_HIDETITLE" },
		{ IDC_NO_FRAME, "DLG_WIN_NO_FRAME" },
		{ IDC_WINHIDEMENU, "DLG_WIN_HIDEMENU" },
		{ IDC_WINAIXTERM16, "DLG_WIN_AIXTERM16" },
		{ IDC_WINXTERM256, "DLG_WIN_XTERM256" },
		{ IDC_WINSCROLL1, "DLG_WIN_SCROLL1" },
		{ IDC_WINSCROLL3, "DLG_WIN_SCROLL3" },
		{ IDC_WINCOLOR, "DLG_WIN_COLOR" },
		{ IDC_WINTEXT, "DLG_WIN_TEXT" },
		{ IDC_WINBACK, "DLG_WIN_BG" },
		{ IDC_WINATTRTEXT, "DLG_WIN_ATTRIB" },
		{ IDC_WIN_SWAP_COLORS, "DLG_WIN_SWAP_COLORS" },
		{ IDC_WINREDLABEL, "DLG_WIN_R" },
		{ IDC_WINGREENLABEL, "DLG_WIN_G" },
		{ IDC_WINBLUELABEL, "DLG_WIN_B" },
		{ IDC_WINUSENORMALBG, "DLG_WIN_ALWAYSBG" },
	};
	PTTSet ts;
	HWND Wnd, HRed, HGreen, HBlue;
	int IAttr, IOffset;
	WORD i, pos, ScrollCode, NewPos;
	WinDlgWork *work = (WinDlgWork *)GetWindowLongPtr(Dialog,DWLP_USER);

	switch (Message) {
		case WM_INITDIALOG:
			work = (WinDlgWork *)(((PROPSHEETPAGEW *)lParam)->lParam);
			SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)work);
			ts = work->ts;

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);
			{
				// VTWinとTEKWinでラベルが異なっている
				wchar_t *UIMsg;
				if (work->VTFlag>0) {
					GetI18nStrWW("Tera Term", "DLG_WIN_PCBOLD16",
								 L"&16 Colors (PC style)",
								 ts->UILanguageFileW, &UIMsg);
				} else {
					GetI18nStrWW("Tera Term", "DLG_WIN_COLOREMU",
								 L"&Color emulation",
								 ts->UILanguageFileW, &UIMsg);
				}
				SetDlgItemTextW(Dialog, IDC_WINCOLOREMU, UIMsg);
				free(UIMsg);
			}

			SetDlgItemTextA(Dialog, IDC_WINTITLE, ts->Title);
			SendDlgItemMessage(Dialog, IDC_WINTITLE, EM_LIMITTEXT,
			                   sizeof(ts->Title)-1, 0);

			SetRB(Dialog,ts->HideTitle,IDC_WINHIDETITLE,IDC_WINHIDETITLE);
			SetRB(Dialog,ts->EtermLookfeel.BGNoFrame,IDC_NO_FRAME,IDC_NO_FRAME);
			SetRB(Dialog,ts->PopupMenu,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
			if ( ts->HideTitle>0 ) {
				DisableDlgItem(Dialog,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
			} else {
				DisableDlgItem(Dialog,IDC_NO_FRAME,IDC_NO_FRAME);
			}

			if (work->VTFlag>0) {
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
					work->TmpColor[0][i*3]   = GetRValue(ts->VTColor[i]);
					work->TmpColor[0][i*3+1] = GetGValue(ts->VTColor[i]);
					work->TmpColor[0][i*3+2] = GetBValue(ts->VTColor[i]);
					work->TmpColor[1][i*3]   = GetRValue(ts->VTBoldColor[i]);
					work->TmpColor[1][i*3+1] = GetGValue(ts->VTBoldColor[i]);
					work->TmpColor[1][i*3+2] = GetBValue(ts->VTBoldColor[i]);
					work->TmpColor[2][i*3]   = GetRValue(ts->VTBlinkColor[i]);
					work->TmpColor[2][i*3+1] = GetGValue(ts->VTBlinkColor[i]);
					work->TmpColor[2][i*3+2] = GetBValue(ts->VTBlinkColor[i]);
					work->TmpColor[3][i*3]   = GetRValue(ts->VTReverseColor[i]);
					work->TmpColor[3][i*3+1] = GetGValue(ts->VTReverseColor[i]);
					work->TmpColor[3][i*3+2] = GetBValue(ts->VTReverseColor[i]);
					work->TmpColor[4][i*3]   = GetRValue(ts->URLColor[i]);
					work->TmpColor[4][i*3+1] = GetGValue(ts->URLColor[i]);
					work->TmpColor[4][i*3+2] = GetBValue(ts->URLColor[i]);
					work->TmpColor[5][i*3]   = GetRValue(ts->VTUnderlineColor[i]);
					work->TmpColor[5][i*3+1] = GetGValue(ts->VTUnderlineColor[i]);
					work->TmpColor[5][i*3+2] = GetBValue(ts->VTUnderlineColor[i]);
				}
				ShowDlgItem(Dialog,IDC_WINATTRTEXT,IDC_WINATTR);
				{
					static const I18nTextInfo infos[] = {
						{ "DLG_WIN_NORMAL", L"Normal" },
						{ "DLG_WIN_BOLD", L"Bold" },
						{ "DLG_WIN_BLINK", L"Blink" },
						{ "DLG_WIN_REVERSEATTR", L"Reverse" },
						{ "DLG_WIN_URL", L"URL" },
						{ "DLG_WIN_UNDERLINE", L"Underline" },
					};
					SetI18nListW("Tera Term", Dialog, IDC_WINATTR, infos, _countof(infos), ts->UILanguageFileW, 0);
				}
				ShowDlgItem(Dialog,IDC_WINUSENORMALBG,IDC_WINUSENORMALBG);
				SetRB(Dialog,ts->UseNormalBGColor,IDC_WINUSENORMALBG,IDC_WINUSENORMALBG);

				ShowDlgItem(Dialog, IDC_FONTBOLD, IDC_FONTBOLD);
				SetRB(Dialog, (ts->FontFlag & FF_BOLD) > 0, IDC_FONTBOLD,IDC_FONTBOLD);
			}
			else {
				for (i = 0 ; i <=1 ; i++) {
					work->TmpColor[0][i*3]   = GetRValue(ts->TEKColor[i]);
					work->TmpColor[0][i*3+1] = GetGValue(ts->TEKColor[i]);
					work->TmpColor[0][i*3+2] = GetBValue(ts->TEKColor[i]);
				}
				SetRB(Dialog,ts->TEKColorEmu,IDC_WINCOLOREMU,IDC_WINCOLOREMU);
			}
			SetRB(Dialog,1,IDC_WINTEXT,IDC_WINBACK);

			SetRB(Dialog,ts->CursorShape,IDC_WINBLOCK,IDC_WINHORZ);

			{
				// サンプル用フォント作成
				LOGFONTW logfont;
				TSGetLogFont(Dialog, ts, (work->VTFlag > 0) ? 0 : 1, 0, &logfont);
				work->sample_font = CreateFontIndirectW(&logfont);
			}
			work->cs = ColorSampleInit(GetDlgItem(Dialog, IDC_DRAW_SAMPLE_AREA), work->sample_font);

			IAttr = 0;
			IOffset = 0;

			HRed = GetDlgItem(Dialog, IDC_WINREDBAR);
			SetScrollRange(HRed,SB_CTL,0,255,TRUE);

			HGreen = GetDlgItem(Dialog, IDC_WINGREENBAR);
			SetScrollRange(HGreen,SB_CTL,0,255,TRUE);

			HBlue = GetDlgItem(Dialog, IDC_WINBLUEBAR);
			SetScrollRange(HBlue,SB_CTL,0,255,TRUE);

			ChangeSB(Dialog,work,IAttr,IOffset);

			return TRUE;

		case WM_NOTIFY: {
			ts = work->ts;
			RestoreVar(Dialog,work,&IAttr,&IOffset);
			assert(IAttr < _countof(work->TmpColor));
			NMHDR *nmhdr = (NMHDR *)lParam;
			switch (nmhdr->code) {
			case PSN_APPLY: {
				WORD w;
				//HDC DC;
				GetDlgItemText(Dialog, IDC_WINTITLE, ts->Title, sizeof(ts->Title));
				GetRB(Dialog, &ts->HideTitle, IDC_WINHIDETITLE, IDC_WINHIDETITLE);
				GetRB(Dialog, &w, IDC_NO_FRAME, IDC_NO_FRAME);
				ts->EtermLookfeel.BGNoFrame = w;
				GetRB(Dialog, &ts->PopupMenu, IDC_WINHIDEMENU, IDC_WINHIDEMENU);
				// DC = GetDC(Dialog);
				if (work->VTFlag > 0) {
					GetRB(Dialog, &i, IDC_WINCOLOREMU, IDC_WINCOLOREMU);
					if (i != 0) {
						ts->ColorFlag |= CF_PCBOLD16;
					}
					else {
						ts->ColorFlag &= ~(WORD)CF_PCBOLD16;
					}
					GetRB(Dialog, &i, IDC_WINAIXTERM16, IDC_WINAIXTERM16);
					if (i != 0) {
						ts->ColorFlag |= CF_AIXTERM16;
					}
					else {
						ts->ColorFlag &= ~(WORD)CF_AIXTERM16;
					}
					GetRB(Dialog, &i, IDC_WINXTERM256, IDC_WINXTERM256);
					if (i != 0) {
						ts->ColorFlag |= CF_XTERM256;
					}
					else {
						ts->ColorFlag &= ~(WORD)CF_XTERM256;
					}
					GetRB(Dialog, &ts->EnableScrollBuff, IDC_WINSCROLL1, IDC_WINSCROLL1);
					if (ts->EnableScrollBuff > 0) {
						ts->ScrollBuffSize =
							GetDlgItemInt(Dialog,IDC_WINSCROLL2,NULL,FALSE);
					}
					for (i = 0; i <= 1; i++) {
						ts->VTColor[i] =
							RGB(work->TmpColor[0][i*3],
								work->TmpColor[0][i*3+1],
								work->TmpColor[0][i*3+2]);
						ts->VTBoldColor[i] =
							RGB(work->TmpColor[1][i*3],
								work->TmpColor[1][i*3+1],
								work->TmpColor[1][i*3+2]);
						ts->VTBlinkColor[i] =
							RGB(work->TmpColor[2][i*3],
								work->TmpColor[2][i*3+1],
								work->TmpColor[2][i*3+2]);
						ts->VTReverseColor[i] =
							RGB(work->TmpColor[3][i*3],
								work->TmpColor[3][i*3+1],
								work->TmpColor[3][i*3+2]);
						ts->URLColor[i] =
							RGB(work->TmpColor[4][i*3],
								work->TmpColor[4][i*3+1],
								work->TmpColor[4][i*3+2]);
						ts->VTUnderlineColor[i] =
							RGB(work->TmpColor[5][i * 3],
								work->TmpColor[5][i * 3 + 1],
								work->TmpColor[5][i * 3 + 2]);
#if 0
						ts->VTColor[i] = GetNearestColor(DC, ts->VTColor[i]);
						ts->VTBoldColor[i] = GetNearestColor(DC, ts->VTBoldColor[i]);
						ts->VTBlinkColor[i] = GetNearestColor(DC, ts->VTBlinkColor[i]);
						ts->VTReverseColor[i] = GetNearestColor(DC, ts->VTReverseColor[i]);
						ts->URLColor[i] = GetNearestColor(DC, ts->URLColor[i]);
#endif
					}
					GetRB(Dialog, &ts->UseNormalBGColor,
						  IDC_WINUSENORMALBG, IDC_WINUSENORMALBG);
					GetRB(Dialog, &i, IDC_FONTBOLD, IDC_FONTBOLD);
					if (i > 0) {
						ts->FontFlag |= FF_BOLD;
					}
					else {
						ts->FontFlag &= ~(WORD)FF_BOLD;
					}
				}
				else {
					for (i = 0; i <= 1; i++) {
						ts->TEKColor[i] =
							RGB(work->TmpColor[0][i*3],
								work->TmpColor[0][i*3+1],
								work->TmpColor[0][i*3+2]);
#if 0
						ts->TEKColor[i] = GetNearestColor(DC, ts->TEKColor[i]);
#endif
					}
					GetRB(Dialog, &ts->TEKColorEmu, IDC_WINCOLOREMU, IDC_WINCOLOREMU);
				}
				// ReleaseDC(Dialog,DC);

				GetRB(Dialog, &ts->CursorShape, IDC_WINBLOCK, IDC_WINHORZ);
				//EndDialog(Dialog, 1);
				return TRUE;
			}
			case PSN_HELP: {
				const WPARAM HelpId = work->VTFlag > 0 ? HlpMenuSetupAdditionalWindow : HlpTEKSetupWindow;
				PostMessage(work->VTWin, WM_USER_DLGHELP2, HelpId, 0);
				break;
			}
			}
			break;
		}
		case WM_COMMAND:
			ts = work->ts;
			RestoreVar(Dialog,work,&IAttr,&IOffset);
			assert(IAttr < _countof(work->TmpColor));
			switch (LOWORD(wParam)) {
				case IDC_WINHIDETITLE:
					GetRB(Dialog,&i,IDC_WINHIDETITLE,IDC_WINHIDETITLE);
					if (i>0) {
						DisableDlgItem(Dialog,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
						EnableDlgItem(Dialog,IDC_NO_FRAME,IDC_NO_FRAME);
					}
					else {
						EnableDlgItem(Dialog,IDC_WINHIDEMENU,IDC_WINHIDEMENU);
						DisableDlgItem(Dialog,IDC_NO_FRAME,IDC_NO_FRAME);
					}
					break;

				case IDC_WINSCROLL1:
					GetRB(Dialog,&i,IDC_WINSCROLL1,IDC_WINSCROLL1);
					if ( i>0 ) {
						EnableDlgItem(Dialog,IDC_WINSCROLL2,IDC_WINSCROLL3);
					}
					else {
						DisableDlgItem(Dialog,IDC_WINSCROLL2,IDC_WINSCROLL3);
					}
					break;

				case IDC_WINTEXT:
					IOffset = 0;
					ChangeSB(Dialog,work,IAttr,IOffset);
					break;

				case IDC_WINBACK:
					IOffset = 3;
					ChangeSB(Dialog,work,IAttr,IOffset);
					break;

				case IDC_WIN_SWAP_COLORS:
					i = work->TmpColor[IAttr][0];
					work->TmpColor[IAttr][0] = work->TmpColor[IAttr][3];
					work->TmpColor[IAttr][3] = i;
					i = work->TmpColor[IAttr][1];
					work->TmpColor[IAttr][1] = work->TmpColor[IAttr][4];
					work->TmpColor[IAttr][4] = i;
					i = work->TmpColor[IAttr][2];
					work->TmpColor[IAttr][2] = work->TmpColor[IAttr][5];
					work->TmpColor[IAttr][5] = i;

					ChangeSB(Dialog,work,IAttr,IOffset);
					break;

				case IDC_WINATTR:
					ChangeSB(Dialog,work,IAttr,IOffset);
					break;
				}
			break;

		case WM_HSCROLL:
			ts = work->ts;
			RestoreVar(Dialog,work,&IAttr,&IOffset);
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
			pos = work->TmpColor[IAttr][i];
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
			work->TmpColor[IAttr][i] = pos;
			SetScrollPos(Wnd,SB_CTL,pos,TRUE);
			ChangeColor(Dialog,work,IAttr,IOffset);
			return FALSE;

		case WM_DESTROY:
			// サンプル用フォント破棄
			DeleteObject(work->sample_font);
			work->sample_font = 0;
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

/**
 *	@param	is_vt	TRUE: VTWin, FALSE: TEKWin
 */
static HPROPSHEETPAGE CreateWinPP(HINSTANCE inst, HWND vtwin, TTTSet *pts, BOOL is_vt)
{
	WinDlgWork *data = (WinDlgWork *)calloc(1, sizeof(*data));
	data->ts = pts;
	data->VTWin = vtwin;
	data->VTFlag = is_vt ? 1 : 0;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *uimsg;
	if (is_vt) {
		GetI18nStrWW("Tera Term", "DLG_WIN_TITLE", L"Window", pts->UILanguageFileW, &uimsg);
	}
	else {
		GetI18nStrWW("Tera Term", "DLG_WIN_TEK_TITLE", L"Window(TEK)", pts->UILanguageFileW, &uimsg);
	}
	psp.pszTitle = uimsg;
	psp.pszTemplate = MAKEINTRESOURCEW(IDD_WINDLG);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(IDD_WINDLG));
#endif
	psp.pfnDlgProc = WinDlg;
	psp.lParam = (LPARAM)data;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)&psp);
	free(uimsg);
	return hpsp;
}

HPROPSHEETPAGE CreateWinVTPP(HINSTANCE inst, HWND vtwin, TTTSet *pts)
{
	return CreateWinPP(inst, vtwin, pts, TRUE);
}

HPROPSHEETPAGE CreateWinTEKPP(HINSTANCE inst, HWND vtwin, TTTSet *pts)
{
	return CreateWinPP(inst, vtwin, pts, FALSE);
}
