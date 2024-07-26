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

/* tcpip property page */
#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "helpid.h"
#include "win32helper.h"
#include "compat_win.h"
#include "edithistory.h"

#include "dlg_res.h"
#include "tcpip_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

typedef struct {
	HINSTANCE hInst;
	TTTSet *ts;
	DLGTEMPLATE *dlg_templ;
} TCPIPDlgData;

static INT_PTR CALLBACK TCPIPDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_TCPIPHISTORY, "DLG_TCPIP_HISTORY" },
		{ IDC_TCPIPAUTOCLOSE, "DLG_TCPIP_AUTOCLOSE" },
		{ IDC_TCPIPPORTLABEL, "DLG_TCPIP_PORT" },
		{ IDC_TCPIPTELNET, "DLG_TCPIP_TELNET" },
		{ IDC_TCPIPTELNETKEEPALIVELABEL, "DLG_TCPIP_KEEPALIVE" },
		{ IDC_TCPIPTELNETKEEPALIVESEC, "DLG_TCPIP_KEEPALIVE_SEC" },
		{ IDC_TCPIPTERMTYPELABEL, "DLG_TCPIP_TERMTYPE" },
	};

	switch (Message) {
		case WM_INITDIALOG: {
			TCPIPDlgData *data = (TCPIPDlgData *)(((PROPSHEETPAGEW_V1 *)lParam)->lParam);
			PTTSet ts = data->ts;
			SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)data);

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

			SetRB(Dialog, ts->HistoryList, IDC_TCPIPHISTORY, IDC_TCPIPHISTORY);
			SetRB(Dialog, ts->AutoWinClose, IDC_TCPIPAUTOCLOSE, IDC_TCPIPAUTOCLOSE);
			SetDlgItemInt(Dialog, IDC_TCPIPPORT, ts->TCPPort, FALSE);
			SetDlgItemInt(Dialog, IDC_TCPIPTELNETKEEPALIVE, ts->TelKeepAliveInterval, FALSE);
			SetRB(Dialog, ts->Telnet, IDC_TCPIPTELNET, IDC_TCPIPTELNET);
			SetDlgItemTextA(Dialog, IDC_TCPIPTERMTYPE, ts->TermType);
			SendDlgItemMessage(Dialog, IDC_TCPIPTERMTYPE, EM_LIMITTEXT, sizeof(ts->TermType) - 1, 0);

			// SSH接続のときにも TERM を送るので、telnetが無効でも disabled にしない。(2005.11.3 yutaka)
			EnableDlgItem(Dialog, IDC_TCPIPTERMTYPELABEL, IDC_TCPIPTERMTYPE);

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;
		}

		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lParam;
			TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
			PTTSet ts = data->ts;
			switch (nmhdr->code) {
			case PSN_APPLY: {
				BOOL Ok;

				GetRB(Dialog, &ts->HistoryList, IDC_TCPIPHISTORY, IDC_TCPIPHISTORY);
				GetRB(Dialog, &ts->AutoWinClose, IDC_TCPIPAUTOCLOSE, IDC_TCPIPAUTOCLOSE);
				ts->TCPPort = GetDlgItemInt(Dialog, IDC_TCPIPPORT, &Ok, FALSE);
				if (!Ok) {
					ts->TCPPort = ts->TelPort;
				}
				ts->TelKeepAliveInterval = GetDlgItemInt(Dialog, IDC_TCPIPTELNETKEEPALIVE, &Ok, FALSE);
				GetRB(Dialog, &ts->Telnet, IDC_TCPIPTELNET, IDC_TCPIPTELNET);
				GetDlgItemText(Dialog, IDC_TCPIPTERMTYPE, ts->TermType, sizeof(ts->TermType));
				break;
			}
			case PSN_HELP: {
				HWND vtwin = GetParent(GetParent(Dialog));
				PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalTCPIP, 0);
				break;
			}
			}
			break;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_TCPIPTELNET: {
					WORD w;
					GetRB(Dialog, &w, IDC_TCPIPTELNET, IDC_TCPIPTELNET);
					if (w == 1) {
						TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
						PTTSet ts = data->ts;
						EnableDlgItem(Dialog, IDC_TCPIPTERMTYPELABEL, IDC_TCPIPTERMTYPE);
						SetDlgItemInt(Dialog, IDC_TCPIPPORT, ts->TelPort, FALSE);
					}
					else {
						// SSH接続のときにも TERM を送るので、telnetが無効でも disabled にしない。(2005.11.3 yutaka)
						EnableDlgItem(Dialog, IDC_TCPIPTERMTYPELABEL, IDC_TCPIPTERMTYPE);
					}
					break;
				}
				case IDC_BUTTON_EDITHISTORY: {
					TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
					PTTSet ts = data->ts;
					EditHistoryDlg(Dialog, ts);
					break;
				}
			}
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
		ppsp->lParam = (LPARAM)NULL;
		break;
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE TcpIPPageCreate(HINSTANCE inst, TTTSet *pts)
{
	TCPIPDlgData *Param = (TCPIPDlgData *)calloc(1, sizeof(TCPIPDlgData));
	Param->hInst = inst;
	Param->ts = pts;

	PROPSHEETPAGEW_V1 psp = {0};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TCPIP_TITLE",
				 L"TCP/IP setup", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(IDD_TCPIPDLG);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(IDD_TCPIPDLG));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = TCPIPDlg;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
