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

/* serial dialog */

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
#include "comportinfo.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "ttwinman.h"

#include "serial_pp.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

// 存在するシリアルポートのみを表示する
//#define SHOW_ONLY_EXSITING_PORT		TRUE
#define SHOW_ONLY_EXSITING_PORT		FALSE	// すべてのポートを表示する

// ドロップダウンからポート表示を切り替え
//#define ENABLE_SWITCH_PORT_DISPLAY	1
#define ENABLE_SWITCH_PORT_DISPLAY	0

// MaxComPortより大きなポートが存在するとき、MaxComPortをオーバーライドする
#define ENABLE_MAXCOMPORT_OVERRIDE	0

static const char *BaudList[] = {
	"110","300","600","1200","2400","4800","9600",
	"14400","19200","38400","57600","115200",
	"230400", "460800", "921600", NULL};
static const char *DataList[] = {"7 bit","8 bit",NULL};
static const char *ParityList[] = {"none", "odd", "even", "mark", "space", NULL};
static const char *StopList[] = {"1 bit", "2 bit", NULL};
static const char *FlowList[] = {"XON/XOFF", "RTS/CTS", "DSR/DTR", "NONE", NULL};
static const char *RtsList[] = {"Disable", "Enable", "Handshake", "Toggle", NULL};
static const char *DtrList[] = {"Disable", "Enable", "Handshake", NULL};
static UINT_PTR timer_id = NULL;

typedef struct {
	PTTSet pts;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
	HINSTANCE hInst;
	DLGTEMPLATE *dlg_templ;
	int g_deltaSumSerialDlg;					// マウスホイールのDelta累積用
	WNDPROC g_defSerialDlgEditWndProc;			// Edit Controlのサブクラス化用
	BOOL show_all_port;
} SerialDlgData;

/*
 * シリアルポート設定ダイアログのテキストボックスにCOMポートの詳細情報を表示する。
 *	@param	port_index		port_info[] のインデックス, -1のとき使用しない
 *	@param	port_no			ポート番号 "com%d", -1のとき使用しない
 *	@param	port_name		port_name ポート名
 */
static void serial_dlg_set_comport_info(HWND dlg, SerialDlgData *dlg_data, int port_index, int port_no, const wchar_t *port_name)
{
	const wchar_t *text = L"This port does not exist now.";
	if (port_index != -1) {
		if (port_index < dlg_data->ComPortInfoCount ) {
			const ComPortInfo_t *p = &dlg_data->ComPortInfoPtr[port_index];
			text = p->property;
		}
	}
	else if (port_no != -1) {
		const ComPortInfo_t *p = dlg_data->ComPortInfoPtr;
		for (int i = 0; i < dlg_data->ComPortInfoCount; p++,i++) {
			if (p->port_no == port_no) {
				text =  p->property;
				break;
			}
		}
	}
	else if (port_name != NULL) {
		const ComPortInfo_t *p = dlg_data->ComPortInfoPtr;
		for (int i = 0; i < dlg_data->ComPortInfoCount; p++,i++) {
			if (wcscmp(p->port_name, port_name) == 0) {
				text =  p->property;
				break;
			}
		}
	}
	SetDlgItemTextW(dlg, IDC_SERIALTEXT, text);
}

/*
 * シリアルポート設定ダイアログのテキストボックスのプロシージャ
 */
static LRESULT CALLBACK SerialDlgEditWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

	switch (msg) {
		case WM_KEYDOWN:
			// Edit control上で CTRL+A を押下すると、テキストを全選択する。
			if (wp == 'A' && GetKeyState(VK_CONTROL) < 0) {
				PostMessage(hWnd, EM_SETSEL, 0, -1);
				return 0;
			}
			break;

		case WM_MOUSEWHEEL: {
			// CTRLorSHIFT + マウスホイールの場合、横スクロールさせる。
			WORD keys;
			short delta;
			BOOL page;
			keys = GET_KEYSTATE_WPARAM(wp);
			delta = GET_WHEEL_DELTA_WPARAM(wp);
			page = keys & (MK_CONTROL | MK_SHIFT);

			if (page == 0)
				break;

			dlg_data->g_deltaSumSerialDlg += delta;

			if (dlg_data->g_deltaSumSerialDlg >= WHEEL_DELTA) {
				dlg_data->g_deltaSumSerialDlg -= WHEEL_DELTA;
				SendMessage(hWnd, WM_HSCROLL, SB_PAGELEFT , 0);
			} else if (dlg_data->g_deltaSumSerialDlg <= -WHEEL_DELTA) {
				dlg_data->g_deltaSumSerialDlg += WHEEL_DELTA;
				SendMessage(hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
			}

			break;
		}
	}
	return CallWindowProc(dlg_data->g_defSerialDlgEditWndProc, hWnd, msg, wp, lp);
}

/**
 *	シリアルポートドロップダウンを設定する
 *	ITEMDATA
 *		0...	PortInfoの先頭からの番号 0からPortInfoCount-1まで
 *		-1		現在存在しないポート
 *		-2		"存在するポートのみ表示"
 *		-3		"全てのポートを表示"
 */
static void SetPortDrop(HWND hWnd, int id, SerialDlgData *dlg_data)
{
	PTTSet ts = dlg_data->pts;
	BOOL show_all_port = dlg_data->show_all_port;
	int max_com = ts->MaxComPort;
	if (max_com < ts->ComPort) {
		max_com = ts->ComPort;
	}
#if ENABLE_MAXCOMPORT_OVERRIDE
	if (dlg_data->ComPortInfoCount != 0) {
		int max_exist_port = dlg_data->ComPortInfoPtr[dlg_data->ComPortInfoCount-1].port_no;
		if (max_com < max_exist_port) {
			max_com = max_exist_port;
		}
	}
#endif
	if (dlg_data->ComPortInfoCount == 0) {
		// ポートが存在していないときはすべて表示する
		show_all_port = TRUE;
	}

	// "COM%d" ではないポート
	int port_index = 0;
	int skip_count = 0;
	const ComPortInfo_t *port_info = dlg_data->ComPortInfoPtr;
	const ComPortInfo_t *p;
	for (int i = 0; i < dlg_data->ComPortInfoCount; i++) {
		p = port_info + port_index;
		if (wcsncmp(p->port_name, L"COM", 3) == 0) {
			break;
		}
		port_index++;
		wchar_t *name;
		if (p->friendly_name != NULL) {
			aswprintf(&name, L"%s: %s", p->port_name, p->friendly_name);
		}
		else {
			aswprintf(&name, L"%s: (no name, exists)", p->port_name);
		}
		LRESULT index = SendDlgItemMessageW(hWnd, id, CB_ADDSTRING, 0, (LPARAM)name);
		free(name);
		SendDlgItemMessageA(hWnd, id, CB_SETITEMDATA, index, i);
		skip_count++;
	}

	// 伝統的な "COM%d"
	int sel_index = 0;
	int com_count = 0;
	BOOL all = show_all_port;
	for (int i = 1; i <= max_com; i++) {

		wchar_t *com_name = NULL;
		int item_data;
		if (port_index < dlg_data->ComPortInfoCount) {
			p = port_info + port_index;
			if (p->port_no == i) {
				// ComPortInfo で検出できたポート
				item_data = port_index;
				port_index++;

				if (p->friendly_name != NULL) {
					aswprintf(&com_name, L"%s: %s", p->port_name, p->friendly_name);
				}
				else {
					aswprintf(&com_name, L"%s: (no friendly name, exist)", p->port_name);
				}
			}
		}

		if (com_name == NULL) {
			// 検出できなかったポート(存在しないポート)
			if (!all && (i != ts->ComPort)) {
				// すべて表示しない
				continue;
			}
			aswprintf(&com_name, L"COM%d", i);
			item_data = -1;
		}

		LRESULT index = SendDlgItemMessageW(hWnd, id, CB_ADDSTRING, 0, (LPARAM)com_name);
		SendDlgItemMessageA(hWnd, id, CB_SETITEMDATA, index, item_data);
		free(com_name);
		com_count++;

		if (i == ts->ComPort) {
			sel_index = com_count + skip_count;
		}
	}

	if (ENABLE_SWITCH_PORT_DISPLAY) {
		if (show_all_port) {
			LRESULT index = SendDlgItemMessageW(hWnd, id, CB_ADDSTRING, 0, (LPARAM)L"<Show only existing ports>");
			SendDlgItemMessageA(hWnd, id, CB_SETITEMDATA, index, -2);
		}
		else {
			wchar_t *s;
			aswprintf(&s, L"<Show to COM%d>", max_com);
			LRESULT index = SendDlgItemMessageW(hWnd, id, CB_ADDSTRING, 0, (LPARAM)s);
			free(s);
			SendDlgItemMessageA(hWnd, id, CB_SETITEMDATA, index, -3);
		}
	}

	ExpandCBWidth(hWnd, id);
	if (dlg_data->ComPortInfoCount == 0) {
		// COMポートなし
		serial_dlg_set_comport_info(hWnd, dlg_data, -1, -1, NULL);
	}
	else {
		if (cv.Open && (cv.PortType == IdSerial)) {
			// 接続中の時は選択できないようにする
			EnableWindow(GetDlgItem(hWnd, id), FALSE);
		}
		serial_dlg_set_comport_info(hWnd, dlg_data, -1, ts->ComPort, NULL);
	}
	SendDlgItemMessage(hWnd, id, CB_SETCURSEL, sel_index - 1, 0);  // sel_index = 1 origin
}

/*
 * シリアルポートの状態をダイアログに反映する
 */
static void SetComStatus(HWND Dialog, int FlowCtrlRTS, int FlowCtrlDTR)
{
	DCB dcb;
	if (GetCommState(cv.ComID, &dcb)) {
		dcb.fDtrControl = SendDlgItemMessageA(Dialog, IDC_SERIALDTR, CB_GETCURSEL, 0, 0);
		dcb.fRtsControl = SendDlgItemMessageA(Dialog, IDC_SERIALRTS, CB_GETCURSEL, 0, 0);

		WORD Flow = (WORD)SendDlgItemMessageA(Dialog, IDC_SERIALFLOW, CB_GETCURSEL, 0, 0);
		if (Flow == 1 /* RTS/CTS */) {
			dcb.fOutxCtsFlow = TRUE;
		} else {
			dcb.fOutxCtsFlow = FALSE;
		}
		if (Flow == 2 /* DSR/DTR */) {
			dcb.fOutxDsrFlow = TRUE;
		} else {
			dcb.fOutxDsrFlow = FALSE;
		}

		SetCommState(cv.ComID, &dcb);
	}

	EscapeCommFunction(cv.ComID, FlowCtrlRTS == IdDisable || FlowCtrlRTS == IdToggle ? CLRRTS : SETRTS);
	SendMessage(GetDlgItem(Dialog, IDC_CHECK_RTS), BM_SETCHECK, FlowCtrlRTS == IdDisable || FlowCtrlRTS == IdToggle ? BST_UNCHECKED : BST_CHECKED, 0);
	EscapeCommFunction(cv.ComID, FlowCtrlDTR == IdDisable ? CLRDTR : SETDTR);
	SendMessage(GetDlgItem(Dialog, IDC_CHECK_DTR), BM_SETCHECK, FlowCtrlDTR == IdDisable ? BST_UNCHECKED : BST_CHECKED, 0);

	EnableWindow(GetDlgItem(Dialog, IDC_CHECK_RTS), FlowCtrlRTS == IdDisable || FlowCtrlRTS == IdEnable ? TRUE : FALSE);
	EnableWindow(GetDlgItem(Dialog, IDC_CHECK_DTR), FlowCtrlDTR == IdDisable || FlowCtrlDTR == IdEnable ? TRUE : FALSE);
}

static void CALLBACK ComPortStatusUpdateProc(HWND Dialog, UINT, UINT_PTR, DWORD) {
	DWORD status;
	if (GetCommModemStatus(cv.ComID, &status)) {
		SendMessage(GetDlgItem(Dialog, IDC_CHECK_CTS),  BM_SETCHECK, status & MS_CTS_ON  ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(Dialog, IDC_CHECK_DSR),  BM_SETCHECK, status & MS_DSR_ON  ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(Dialog, IDC_CHECK_RING), BM_SETCHECK, status & MS_RING_ON ? BST_CHECKED : BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(Dialog, IDC_CHECK_RLSD), BM_SETCHECK, status & MS_RLSD_ON ? BST_CHECKED : BST_UNCHECKED, 0);
	}
}

/*
 * シリアルポート設定ダイアログ
 *
 * シリアルポート数が0の時は呼ばれない
 */
static INT_PTR CALLBACK SerialDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_SERIALPORT_LABEL, "DLG_SERIAL_PORT" },
		{ IDC_SERIALBAUD_LEBAL, "DLG_SERIAL_BAUD" },
		{ IDC_SERIALDATA_LABEL, "DLG_SERIAL_DATA" },
		{ IDC_SERIALPARITY_LABEL, "DLG_SERIAL_PARITY" },
		{ IDC_SERIALSTOP_LABEL, "DLG_SERIAL_STOP" },
		{ IDC_SERIALFLOW_LABEL, "DLG_SERIAL_FLOW" },
		{ IDC_SERIALDELAY, "DLG_SERIAL_DELAY" },
		{ IDC_SERIALDELAYCHAR_LABEL, "DLG_SERIAL_DELAYCHAR" },
		{ IDC_SERIALDELAYLINE_LABEL, "DLG_SERIAL_DELAYLINE" },
	};

	switch (Message) {
		case WM_INITDIALOG: {

			SerialDlgData *dlg_data = (SerialDlgData *)(((PROPSHEETPAGEW_V1 *)lParam)->lParam);
			SetWindowLongPtrW(Dialog, DWLP_USER, (LPARAM)dlg_data);
			PTTSet ts = dlg_data->pts;

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

			SetPortDrop(Dialog, IDC_SERIALPORT, dlg_data);
			// speed, baud rate
			{
				int i, sel;
				SetDropDownList(Dialog, IDC_SERIALBAUD, BaudList, 0);
				i = sel = 0;
				while (BaudList[i] != NULL) {
					if ((WORD)atoi(BaudList[i]) == ts->Baud) {
						SendDlgItemMessage(Dialog, IDC_SERIALBAUD, CB_SETCURSEL, i, 0);
						sel = 1;
						break;
					}
					i++;
				}
				if (!sel) {
					SetDlgItemInt(Dialog, IDC_SERIALBAUD, ts->Baud, FALSE);
				}
			}

			SetDropDownList(Dialog, IDC_SERIALDATA, DataList, ts->DataBit);
			SetDropDownList(Dialog, IDC_SERIALPARITY, ParityList, ts->Parity);
			SetDropDownList(Dialog, IDC_SERIALSTOP, StopList, ts->StopBit);

			/*
			 * value               display
			 * 1 IdFlowX           1 Xon/Xoff
			 * 2 IdFlowHard        2 RTS/CTS
			 * 3 IdFlowNone        4 none
			 * 4 IdFlowHardDsrDtr  3 DSR/DTR
			 */
			WORD Flow = ts->Flow;
			if (Flow == 3) {
				Flow = 4;
			}
			else if (Flow == 4) {
				Flow = 3;
			}
			SetDropDownList(Dialog, IDC_SERIALFLOW, FlowList, Flow);

			SetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,ts->DelayPerChar,FALSE);
			SendDlgItemMessage(Dialog, IDC_SERIALDELAYCHAR, EM_LIMITTEXT,4, 0);

			SetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,ts->DelayPerLine,FALSE);
			SendDlgItemMessage(Dialog, IDC_SERIALDELAYLINE, EM_LIMITTEXT,4, 0);

			CenterWindow(Dialog, GetParent(Dialog));

			// Edit controlをサブクラス化する。
			dlg_data->g_deltaSumSerialDlg = 0;
			SetWindowLongPtrW(GetDlgItem(Dialog, IDC_SERIALTEXT), GWLP_USERDATA, (LONG_PTR)dlg_data);
			dlg_data->g_defSerialDlgEditWndProc = (WNDPROC)SetWindowLongPtrW(
				GetDlgItem(Dialog, IDC_SERIALTEXT),
				GWLP_WNDPROC,
				(LONG_PTR)SerialDlgEditWindowProc);

			SetDropDownList(Dialog, IDC_SERIALRTS, RtsList, ts->FlowCtrlRTS + 1);
			SetDropDownList(Dialog, IDC_SERIALDTR, DtrList, ts->FlowCtrlDTR + 1);
			SetComStatus(Dialog, ts->FlowCtrlRTS, ts->FlowCtrlDTR);

			timer_id = SetTimer(Dialog, NULL, 100, ComPortStatusUpdateProc);

			return TRUE;
		}
		case WM_NOTIFY: {
			NMHDR *nmhdr = (NMHDR *)lParam;
			SerialDlgData *data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
			PTTSet ts = data->pts;
			switch (nmhdr->code) {
			case PSN_APPLY: {
				int w;
				char Temp[128];
				Temp[0] = 0;
				GetDlgItemTextA(Dialog, IDC_SERIALPORT, Temp, sizeof(Temp)-1);
				if (strncmp(Temp, "COM", 3) == 0 && Temp[3] != '\0') {
					ts->ComPort = (WORD)atoi(&Temp[3]);
				}

				GetDlgItemTextA(Dialog, IDC_SERIALBAUD, Temp, sizeof(Temp)-1);
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
					/*
					 * display    value
					 * 1 Xon/Xoff 1 IdFlowX
					 * 2 RTS/CTS  2 IdFlowHard
					 * 3 DSR/DTR  4 IdFlowHardDsrDtr
					 * 4 none     3 IdFlowNone
					 */
					WORD Flow = w;
					if (Flow == 3) {
						Flow = 4;
					}
					else if (Flow == 4) {
						Flow = 3;
					}
					ts->Flow = Flow;
				}

				int rts = (int)SendDlgItemMessageA(Dialog, IDC_CHECK_RTS, BM_GETCHECK, 0, 0);
				EscapeCommFunction(cv.ComID, rts ? SETRTS : CLRRTS);
				int dtr = (int)SendDlgItemMessageA(Dialog, IDC_CHECK_DTR, BM_GETCHECK, 0, 0);
				EscapeCommFunction(cv.ComID, dtr ? SETDTR : CLRDTR);
				ts->FlowCtrlRTS = SendDlgItemMessageA(Dialog, IDC_SERIALRTS, CB_GETCURSEL, 0, 0);
				ts->FlowCtrlDTR = SendDlgItemMessageA(Dialog, IDC_SERIALDTR, CB_GETCURSEL, 0, 0);
				DCB dcb;
				if (GetCommState(cv.ComID, &dcb)) {
					dcb.fRtsControl = ts->FlowCtrlRTS;
					dcb.fDtrControl = ts->FlowCtrlDTR;

					WORD Flow = (WORD)SendDlgItemMessageA(Dialog, IDC_SERIALFLOW, CB_GETCURSEL, 0, 0);
					if (Flow == 1 /* RTS/CTS */) {
						dcb.fOutxCtsFlow = TRUE;
					} else {
						dcb.fOutxCtsFlow = FALSE;
					}
					if (Flow == 2 /* DSR/DTR */) {
						dcb.fOutxDsrFlow = TRUE;
					} else {
						dcb.fOutxDsrFlow = FALSE;
					}

					SetCommState(cv.ComID, &dcb);
				}

				ts->DelayPerChar = GetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,NULL,FALSE);

				ts->DelayPerLine = GetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,NULL,FALSE);

				// TODO 削除
				// 全般タブの「標準のポート」でデフォルトの接続ポートを指定する
				// ここでは指定しない
				// ts->PortType = IdSerial;

				// ボーレートが変更されることがあるので、タイトル再表示の
				// メッセージを飛ばすようにした。 (2007.7.21 maya)
				PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);

				break;
			}
			case PSN_RESET: {
				if (timer_id) {
					KillTimer(Dialog, timer_id);
					timer_id = NULL;
				}
				DCB dcb;
				if (GetCommState(cv.ComID, &dcb)) {
					dcb.fRtsControl = ts->FlowCtrlRTS;
					dcb.fDtrControl = ts->FlowCtrlDTR;

					if (ts->Flow == 1 /* RTS/CTS */) {
						dcb.fOutxCtsFlow = TRUE;
					} else {
						dcb.fOutxCtsFlow = FALSE;
						}
					if (ts->Flow == 2 /* DSR/DTR */) {
						dcb.fOutxDsrFlow = TRUE;
					} else {
						dcb.fOutxDsrFlow = FALSE;
					}

					SetCommState(cv.ComID, &dcb);
				}
				break;
			}
			case PSN_HELP: {
				HWND vtwin = GetParent(GetParent(Dialog));
				PostMessage(vtwin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalSerialPort, 0);
				break;
			}
			}
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_SERIALPORT:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						// リストからCOMポートが選択された
						SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
						LRESULT sel = SendDlgItemMessageA(Dialog, IDC_SERIALPORT, CB_GETCURSEL, 0, 0);
						int item_data = (int)SendDlgItemMessageA(Dialog, IDC_SERIALPORT, CB_GETITEMDATA, sel, 0);
						if (item_data >= 0) {
							// 詳細情報を表示する
							serial_dlg_set_comport_info(Dialog, dlg_data, item_data, -1, NULL);
						}
						else if (item_data == -1) {
							// 情報なしを表示する
							serial_dlg_set_comport_info(Dialog, dlg_data, -1, -1, NULL);
						}
						else {
							// 選択方法変更
							dlg_data->show_all_port = (item_data == -2) ? FALSE : TRUE;
							SendDlgItemMessageA(Dialog, IDC_SERIALPORT, CB_RESETCONTENT, 0, 0);
							SetPortDrop(Dialog, IDC_SERIALPORT, dlg_data);
						}
						break;
					}
					return TRUE;

				case IDC_SERIALFLOW:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
						WORD Flow = (WORD)SendDlgItemMessageA(Dialog, IDC_SERIALFLOW, CB_GETCURSEL, 0, 0) + 1;
						if (Flow == 4) {
							Flow = 3;
						} else if (Flow == 3) {
							Flow = 4;
						}
						int rts, dtr;
						switch (Flow) {
							case IdFlowX:				// XON/XOFF
							case IdFlowNone:			// NONE
								rts = 1; // Enable
								dtr = 1; // Enable
								break;
							case IdFlowHard:			// RTS/CTS
								rts = 2; // Handshake
								dtr = 1; // Enable
								break;
							case IdFlowHardDsrDtr:		// DSR/DTR
								rts = 1; // Enable
								dtr = 2; // Handshake
								break;
						}
						SendDlgItemMessage(Dialog, IDC_SERIALRTS, CB_SETCURSEL, rts, 0);
						SendDlgItemMessage(Dialog, IDC_SERIALDTR, CB_SETCURSEL, dtr, 0);
						SetComStatus(Dialog, rts, dtr);
					}
					return TRUE;

				case IDC_CHECK_RTS:
					if (HIWORD(wParam) == BN_CLICKED) {
						SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
						int rts = (int)SendDlgItemMessageA(Dialog, IDC_CHECK_RTS, BM_GETCHECK, 0, 0);
						EscapeCommFunction(cv.ComID, rts ? CLRRTS : SETRTS); // 即時反映する
						SendMessage(GetDlgItem(Dialog, IDC_CHECK_RTS),  BM_SETCHECK, rts ? BST_UNCHECKED : BST_CHECKED, 0);
					}
					return TRUE;

				case IDC_CHECK_DTR:
					if (HIWORD(wParam) == BN_CLICKED) {
						SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
						int dtr = (int)SendDlgItemMessageA(Dialog, IDC_CHECK_DTR, BM_GETCHECK, 0, 0);
						EscapeCommFunction(cv.ComID, dtr ? CLRDTR : SETDTR); // 即時反映する
						SendMessage(GetDlgItem(Dialog, IDC_CHECK_DTR),  BM_SETCHECK, dtr ? BST_UNCHECKED : BST_CHECKED, 0);
					}
					return TRUE;

				case IDC_SERIALRTS:
				case IDC_SERIALDTR:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
						WORD Flow = (WORD)SendDlgItemMessageA(Dialog, IDC_SERIALFLOW, CB_GETCURSEL, 0, 0) + 1;
						if (Flow == 4) {
							Flow = 3;
						} else if (Flow == 3) {
							Flow = 4;
						}
						int FlowCtrlRTS = SendDlgItemMessageA(Dialog, IDC_SERIALRTS, CB_GETCURSEL, 0, 0);
						int FlowCtrlDTR = SendDlgItemMessageA(Dialog, IDC_SERIALDTR, CB_GETCURSEL, 0, 0);
						SetComStatus(Dialog, FlowCtrlRTS, FlowCtrlDTR);
					}
					return TRUE;
			}
	}
	return FALSE;
}

static UINT CALLBACK CallBack(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW *ppsp)
{
	UINT ret_val = 0;
	switch (uMsg) {
	case PSPCB_CREATE:
		ret_val = 1;
		break;
	case PSPCB_RELEASE: {
		SerialDlgData *dlg_data = (SerialDlgData *)ppsp->lParam;

		free((void *)ppsp->pResource);
		ppsp->pResource = NULL;
		ComPortInfoFree(dlg_data->ComPortInfoPtr, dlg_data->ComPortInfoCount);
		free(dlg_data);
		ppsp->lParam = (LPARAM)NULL;
		break;
	}
	default:
		break;
	}
	return ret_val;
}

HPROPSHEETPAGE SerialPageCreate(HINSTANCE inst, TTTSet *pts)
{
	SerialDlgData *Param = (SerialDlgData *)calloc(1, sizeof(SerialDlgData));
	Param->hInst = inst;
	Param->pts = pts;
	Param->ComPortInfoPtr = ComPortInfoGet(&Param->ComPortInfoCount);
	Param->show_all_port = !SHOW_ONLY_EXSITING_PORT;

	PROPSHEETPAGEW_V1 psp = {};
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK | PSP_USETITLE | PSP_HASHELP;
	psp.hInstance = inst;
	psp.pfnCallback = CallBack;
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_SERIAL_TITLE",
				 L"Serial port", pts->UILanguageFileW, &UIMsg);
	psp.pszTitle = UIMsg;
	psp.pszTemplate = MAKEINTRESOURCEW(IDD_SERIALDLG);
#if defined(REWRITE_TEMPLATE)
	psp.dwFlags |= PSP_DLGINDIRECT;
	Param->dlg_templ = TTGetDlgTemplate(inst, MAKEINTRESOURCEW(IDD_SERIALDLG));
	psp.pResource = Param->dlg_templ;
#endif

	psp.pfnDlgProc = SerialDlg;
	psp.lParam = (LPARAM)Param;

	HPROPSHEETPAGE hpsp = CreatePropertySheetPageW((LPPROPSHEETPAGEW)&psp);
	free(UIMsg);
	return hpsp;
}
