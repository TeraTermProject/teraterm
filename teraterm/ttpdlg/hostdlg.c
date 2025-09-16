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

/* host dialog */

#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "ttcommon.h"
#include "dlg_res.h"
#include "comportinfo.h"
#include "codeconv.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "ttlib_charset.h"
#include "asprintf.h"
#include "ttwinman.h"
#include "edithistory.h"

#include "ttdlg.h"

static const char *ProtocolFamilyList[] = { "AUTO", "IPv6", "IPv4", NULL };

typedef struct {
	PGetHNRec GetHNRec;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
	BOOL HostDropOpen;
} TTXHostDlgData;

static BOOL IsEditHistorySelected(HWND dlg)
{
	LRESULT index = SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETCURSEL, 0, 0);
	LRESULT data = SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETITEMDATA, (WPARAM)index, 0);
	return data == 999;
}

static void SetHostDropdown(HWND dlg, const TTTSet *ts)
{
	LRESULT index;
	SetComboBoxHostHistory(dlg, IDC_HOSTNAME, MAXHOSTLIST, ts->SetupFNameW);
	ExpandCBWidth(dlg, IDC_HOSTNAME);
	SendDlgItemMessage(dlg, IDC_HOSTNAME, EM_LIMITTEXT, HostNameMaxLength - 1, 0);
	if (SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETCOUNT, 0, 0) != 0) {
		// 一番最初を選択する
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, 0, 0);
	} else {
		// 選択しない、エディットボックスは空になる
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, -1, 0);
	}

	// Edit historyを追加(ITEMDATA=999)
	index = SendDlgItemMessageW(dlg, IDC_HOSTNAME, CB_ADDSTRING, 0, (LPARAM)L"<Edit host list...>");
	SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETITEMDATA, index, 999);
}

static void OpenEditHistory(HWND dlg, TTTSet *ts)
{
	EditHistoryDlgData data = {0};
	data.UILanguageFileW = ts->UILanguageFileW;
	data.SetupFNameW = ts->SetupFNameW;
	data.vtwin = HVTWin;
	data.title = L"Edit Host list";
	if (EditHistoryDlg(NULL, dlg, &data)) {
		// 編集されたので、ドロップダウンを再設定する
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_RESETCONTENT, 0, 0);

		SetHostDropdown(dlg, ts);
	}

	if (SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_GETCOUNT, 0, 0) > 1) {
		// 一番最初を選択する
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, 0, 0);
	} else {
		// "<Edit History...>のみ、選択しない、エディットボックスは空になる
		SendDlgItemMessageA(dlg, IDC_HOSTNAME, CB_SETCURSEL, -1, 0);
	}
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
	TTXHostDlgData *dlg_data = (TTXHostDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
	PGetHNRec GetHNRec = dlg_data != NULL ? dlg_data->GetHNRec : NULL;

	switch (Message) {
		case WM_INITDIALOG: {
			WORD i;
			int j;
			int com_index;

			GetHNRec = (PGetHNRec)lParam;
			dlg_data = (TTXHostDlgData *)calloc(1, sizeof(*dlg_data));
			SetWindowLongPtr(Dialog, DWLP_USER, (LPARAM)dlg_data);
			dlg_data->GetHNRec = GetHNRec;

			dlg_data->ComPortInfoPtr = ComPortInfoGet(&dlg_data->ComPortInfoCount);
			dlg_data->HostDropOpen = FALSE;

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

			// ファイルおよび名前付きパイプの場合、TCP/IP扱いとする。
			if ( GetHNRec->PortType==IdFile ||
				 GetHNRec->PortType==IdNamedPipe
				) {
				GetHNRec->PortType = IdTCPIP;
			}

			// ホスト (コマンドライン)
			SetHostDropdown(Dialog, &ts);
			SetEditboxEmacsKeybind(Dialog, IDC_HOSTNAME);

			SetRB(Dialog,GetHNRec->Telnet,IDC_HOSTTELNET,IDC_HOSTTELNET);
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPORT, EM_LIMITTEXT,5,0);
			SetDlgItemInt(Dialog,IDC_HOSTTCPPORT,GetHNRec->TCPPort,FALSE);
			for (i=0; ProtocolFamilyList[i]; ++i) {
				SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_ADDSTRING,
				                   0, (LPARAM)ProtocolFamilyList[i]);
			}
			SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_SETCURSEL,0,0);

			j = 0;
			com_index = 1;
			for (i = 0; i < dlg_data->ComPortInfoCount; i++) {
				ComPortInfo_t *p = dlg_data->ComPortInfoPtr + i;
				wchar_t *EntNameW;
				int index;

				// MaxComPort を越えるポートは表示しない
				if (i > GetHNRec->MaxComPort) {
					continue;
				}

				// 使用中のポートは表示しない
				if (CheckCOMFlag(p->port_no) == 1) {
					continue;
				}

				j++;
				if (GetHNRec->ComPort == p->port_no) {
					com_index = j;
				}

				if (p->friendly_name == NULL) {
					aswprintf(&EntNameW, L"%s", p->port_name);
				}
				else {
					aswprintf(&EntNameW, L"%s: %s", p->port_name, p->friendly_name);
				}
				index = (int)SendDlgItemMessageW(Dialog, IDC_HOSTCOM, CB_ADDSTRING, 0, (LPARAM)EntNameW);
				SendDlgItemMessageA(Dialog, IDC_HOSTCOM, CB_SETITEMDATA, index, i);
				free(EntNameW);
			}
			if (j>0) {
				// GetHNRec->ComPort を選択する
				SendDlgItemMessageA(Dialog, IDC_HOSTCOM, CB_SETCURSEL, com_index - 1, 0);
			}
			else { /* All com ports are already used */
				GetHNRec->PortType = IdTCPIP;
				DisableDlgItem(Dialog,IDC_HOSTSERIAL,IDC_HOSTSERIAL);
			}
			ExpandCBWidth(Dialog, IDC_HOSTCOM);

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
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					if (IsEditHistorySelected(Dialog)) {
						OpenEditHistory(Dialog, &ts);
						break;
					}
					if (IsDlgButtonChecked(Dialog, IDC_HOSTTCPIP)) {
						int i;
						BOOL Ok;

						GetHNRec->PortType = IdTCPIP;
						GetDlgItemTextW(Dialog, IDC_HOSTNAME, GetHNRec->HostName, HostNameMaxLength);
						GetRB(Dialog,&GetHNRec->Telnet,IDC_HOSTTELNET,IDC_HOSTTELNET);
						i = GetDlgItemInt(Dialog,IDC_HOSTTCPPORT,&Ok,FALSE);
						if (Ok) {
							GetHNRec->TCPPort = i;
						}
						i = (int)SendDlgItemMessage(Dialog, IDC_HOSTTCPPROTOCOL, CB_GETCURSEL, 0, 0);
						GetHNRec->ProtocolFamily =
							i == 0 ? AF_UNSPEC :
							i == 1 ? AF_INET6 : AF_INET;
					}
					else {
						int pos;
						int index;

						GetHNRec->PortType = IdSerial;
						GetHNRec->HostName[0] = 0;
						pos = (int)SendDlgItemMessageA(Dialog, IDC_HOSTCOM, CB_GETCURSEL, 0, 0);
						assert(pos >= 0);
						index = (int)SendDlgItemMessageA(Dialog, IDC_HOSTCOM, CB_GETITEMDATA, pos, 0);
						GetHNRec->ComPort = dlg_data->ComPortInfoPtr[index].port_no;
					}
					EndDialog(Dialog, 1);
					return TRUE;
				}
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

				case IDC_HOSTTELNET: {
					WORD i;
					GetRB(Dialog,&i,IDC_HOSTTELNET,IDC_HOSTTELNET);
					if ( i==1 ) {
						SetDlgItemInt(Dialog,IDC_HOSTTCPPORT,GetHNRec->TelPort,FALSE);
					}
					break;
				}

				case IDC_HOSTHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpFileNewConnection,0);
					break;

				case IDC_HOSTNAME: {
					switch (HIWORD(wParam)) {
					case CBN_DROPDOWN:
						dlg_data->HostDropOpen = TRUE;
						break;
					case CBN_CLOSEUP:
						dlg_data->HostDropOpen = FALSE;
						break;
					case CBN_SELENDOK:
						if (dlg_data->HostDropOpen) {
							//	ドロップダウンしていないときは、キーかホイールで選択している
							//	決定(Enter or OK押下)すると編集開始
							if (IsEditHistorySelected(Dialog)) {
								OpenEditHistory(Dialog, &ts);
							}
						}
						break;
					}
					break;
				}
			}
			break;
		case WM_DESTROY:
			ComPortInfoFree(dlg_data->ComPortInfoPtr, dlg_data->ComPortInfoCount);
			free(dlg_data);
			break;
	}
	return FALSE;
}

BOOL WINAPI _GetHostName(HWND WndParent, PGetHNRec GetHNRec)
{
	return
		(BOOL)TTDialogBoxParam(hInst,
							   MAKEINTRESOURCEW(IDD_HOSTDLG),
							   WndParent, HostDlg, (LPARAM)GetHNRec);
}
