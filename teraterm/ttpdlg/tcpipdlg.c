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

/* tcpip dialog */
#include "teraterm.h"
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
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "ttwinman.h"
#include "resize_helper.h"

#include "ttdlg.h"

typedef struct {
	PTTSet ts;
	ReiseDlgHelper_t *resize_helper;
} TCPIPDlgData;

static INT_PTR CALLBACK TCPIPDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_TCPIP_TITLE" },
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
	static const ResizeHelperInfo resize_info[] = {
		{ IDC_TCPIPHISTORY, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPTELNET, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPAUTOCLOSE, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPPORTLABEL, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPPORT, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPTELNETKEEPALIVELABEL, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPTELNETKEEPALIVE, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPTELNETKEEPALIVESEC, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPTERMTYPELABEL, RESIZE_HELPER_ANCHOR_B },
		{ IDC_TCPIPTERMTYPE, RESIZE_HELPER_ANCHOR_B },
		{ IDOK, RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H },
		{ IDCANCEL, RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H },
		{ IDC_TCPIPHELP, RESIZE_HELPER_ANCHOR_B + RESIZE_HELPER_ANCHOR_NONE_H },
	};

	switch (Message) {
		case WM_INITDIALOG: {
			TCPIPDlgData *data = (TCPIPDlgData *)lParam;
			PTTSet ts = data->ts;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);

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

			data->resize_helper = ReiseHelperInit(Dialog, TRUE, resize_info, _countof(resize_info));

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
					PTTSet ts = data->ts;
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
					EndDialog(Dialog, 1);
					return TRUE;
				}
				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

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

				case IDC_TCPIPHELP:
					PostMessage(GetParent(Dialog), WM_USER_DLGHELP2, HlpSetupTCPIP, 0);
					break;
			}
			break;

		case WM_SIZE: {
			TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
			ReiseDlgHelper_WM_SIZE(data->resize_helper);
			break;
		}

		case WM_GETMINMAXINFO: {
			TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
			ReiseDlgHelper_WM_GETMINMAXINFO(data->resize_helper, lParam);
			break;
		}

		case WM_DESTROY: {
			TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
			ReiseDlgHelperDelete(data->resize_helper);
			data->resize_helper = NULL;
			break;
		}
	}
	return FALSE;
}

BOOL WINAPI _SetupTCPIP(HWND WndParent, PTTSet ts)
{
	BOOL r;
	TCPIPDlgData *data = (TCPIPDlgData *)calloc(1, sizeof(*data));
	if (data == NULL) {
		return FALSE;
	}
	data->ts = ts;
	r= (BOOL)TTDialogBoxParam(hInst,
							  MAKEINTRESOURCE(IDD_TCPIPDLG),
							  WndParent, TCPIPDlg, (LPARAM)data);
	free(data);
	return r;
}
