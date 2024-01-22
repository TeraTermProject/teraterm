/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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
#include "teraterm.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include <commdlg.h>
#include <dlgs.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <winsock2.h>

#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "ttcommon.h"
#include "dlg_res.h"
#include "ttdlg.h"
#include "tipwin.h"
#include "comportinfo.h"
#include "codeconv.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "ttlib_charset.h"
#include "asprintf.h"
#include "ttwinman.h"
#include "resize_helper.h"

#undef DialogBoxParam
#define DialogBoxParam(p1,p2,p3,p4,p5) \
	TTDialogBoxParam(p1,p2,p3,p4,p5)
#undef DialogBox
#define DialogBox(p1,p2,p3,p4) \
	TTDialogBox(p1,p2,p3,p4)
#undef EndDialog
#define EndDialog(p1,p2) \
	TTEndDialog(p1, p2)

static const char *ProtocolFamilyList[] = { "AUTO", "IPv6", "IPv4", NULL };
static const char *RussList2[] = {"Windows","KOI8-R",NULL};
static const char *MetaList[] = {"off", "on", "left", "right", NULL};
static const char *MetaList2[] = {"off", "on", NULL};

static const char *BaudList[] =
	{"110","300","600","1200","2400","4800","9600",
	 "14400","19200","38400","57600","115200",
	 "230400", "460800", "921600", NULL};

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

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

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
			ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
			switch (LOWORD(wParam)) {
				case IDOK:
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

				case IDC_KEYBHELP: {
					WPARAM HelpId;
					if (ts->Language==IdRussian) {
						HelpId = HlpSetupKeyboardRuss;
					}
					else {
						HelpId = HlpSetupKeyboard;
					}
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HelpId,0);
					break;
				}
			}
	}
	return FALSE;
}

typedef struct {
	PTTSet pts;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
} SerialDlgData;

static const char *DataList[] = {"7 bit","8 bit",NULL};
static const char *ParityList[] = {"none", "odd", "even", "mark", "space", NULL};
static const char *StopList[] = {"1 bit", "2 bit", NULL};
static const char *FlowList[] = {"Xon/Xoff", "RTS/CTS", "DSR/DTR", "none", NULL};
static int g_deltaSumSerialDlg = 0;        // マウスホイールのDelta累積用
static WNDPROC g_defSerialDlgEditWndProc;  // Edit Controlのサブクラス化用
static WNDPROC g_defSerialDlgSpeedComboboxWndProc;  // Combo-box Controlのサブクラス化用
static TipWin *g_SerialDlgSpeedTip;

/*
 * シリアルポート設定ダイアログのOKボタンを接続先に応じて名称を切り替える。
 * 条件判定は OnSetupSerialPort() と合わせる必要がある。
 */
static void serial_dlg_change_OK_button(HWND dlg, int portno, const wchar_t *UILanguageFileW)
{
	wchar_t *uimsg;
	if ( cv.Ready && (cv.PortType != IdSerial) ) {
		uimsg = TTGetLangStrW("Tera Term",
							  "DLG_SERIAL_OK_CONNECTION",
							  L"Connect with &New window",
							  UILanguageFileW);
	} else {
		if (cv.Open) {
			if (portno != cv.ComPort) {
				uimsg = TTGetLangStrW("Tera Term",
									  "DLG_SERIAL_OK_CLOSEOPEN",
									  L"Close and &New open",
									  UILanguageFileW);
			} else {
				uimsg = TTGetLangStrW("Tera Term",
									  "DLG_SERIAL_OK_RESET",
									  L"&New setting",
									  UILanguageFileW);
			}

		} else {
			uimsg = TTGetLangStrW("Tera Term",
								  "DLG_SERIAL_OK_OPEN",
								  L"&New open",
								  UILanguageFileW);
		}
	}
	SetDlgItemTextW(dlg, IDOK, uimsg);
	free(uimsg);
}

/*
 * シリアルポート設定ダイアログのテキストボックスにCOMポートの詳細情報を表示する。
 *
 */
static void serial_dlg_set_comport_info(HWND dlg, SerialDlgData *dlg_data, int port_index)
{
	if (port_index + 1 > dlg_data->ComPortInfoCount) {
		SetDlgItemTextW(dlg, IDC_SERIALTEXT, NULL);
	}
	else {
		const ComPortInfo_t *p = &dlg_data->ComPortInfoPtr[port_index];
		SetDlgItemTextW(dlg, IDC_SERIALTEXT, p->property);
	}
}

/*
 * シリアルポート設定ダイアログのテキストボックスのプロシージャ
 */
static LRESULT CALLBACK SerialDlgEditWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	WORD keys;
	short delta;
	BOOL page;

	switch (msg) {
		case WM_KEYDOWN:
			// Edit control上で CTRL+A を押下すると、テキストを全選択する。
			if (wp == 'A' && GetKeyState(VK_CONTROL) < 0) {
				PostMessage(hWnd, EM_SETSEL, 0, -1);
				return 0;
			}
			break;

		case WM_MOUSEWHEEL:
			// CTRLorSHIFT + マウスホイールの場合、横スクロールさせる。
			keys = GET_KEYSTATE_WPARAM(wp);
			delta = GET_WHEEL_DELTA_WPARAM(wp);
			page = keys & (MK_CONTROL | MK_SHIFT);

			if (page == 0)
				break;

			g_deltaSumSerialDlg += delta;

			if (g_deltaSumSerialDlg >= WHEEL_DELTA) {
				g_deltaSumSerialDlg -= WHEEL_DELTA;
				SendMessage(hWnd, WM_HSCROLL, SB_PAGELEFT , 0);
			} else if (g_deltaSumSerialDlg <= -WHEEL_DELTA) {
				g_deltaSumSerialDlg += WHEEL_DELTA;
				SendMessage(hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
			}

			break;
	}
    return CallWindowProc(g_defSerialDlgEditWndProc, hWnd, msg, wp, lp);
}

/*
 * シリアルポート設定ダイアログのSPEED(BAUD)のプロシージャ
 */
static LRESULT CALLBACK SerialDlgSpeedComboboxWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	const int tooltip_timeout = 1000;  // msec
	POINT pt;
	int w, h;
	int cx, cy;
	RECT wr;
	wchar_t *uimsg;
	SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	const wchar_t *UILanguageFileW;

	switch (msg) {
		case WM_MOUSEMOVE:
			UILanguageFileW = dlg_data->pts->UILanguageFileW;
			uimsg = TTGetLangStrW("Tera Term",
								  "DLG_SERIAL_SPEED_TOOLTIP", L"You can directly specify a number", UILanguageFileW);

			// Combo-boxの左上座標を求める
			GetWindowRect(hWnd, &wr);
			pt.x = wr.left;
			pt.y = wr.top;

			// 文字列の縦横サイズを取得する
			TipWinGetTextWidthHeightW(hWnd, uimsg, &w, &h);

			cx = pt.x;
			cy = pt.y - (h + TIP_WIN_FRAME_WIDTH * 6);

			// ツールチップを表示する
			if (g_SerialDlgSpeedTip == NULL) {
				g_SerialDlgSpeedTip = TipWinCreate(hInst, hWnd);
				TipWinSetHideTimer(g_SerialDlgSpeedTip, tooltip_timeout);
			}
			if (!TipWinIsVisible(g_SerialDlgSpeedTip))
				TipWinSetVisible(g_SerialDlgSpeedTip, TRUE);

			TipWinSetTextW(g_SerialDlgSpeedTip, uimsg);
			TipWinSetPos(g_SerialDlgSpeedTip, cx, cy);
			TipWinSetHideTimer(g_SerialDlgSpeedTip, tooltip_timeout);

			free(uimsg);

			break;
	}
    return CallWindowProc(g_defSerialDlgSpeedComboboxWndProc, hWnd, msg, wp, lp);
}

/*
 * シリアルポート設定ダイアログ
 *
 * シリアルポート数が0の時は呼ばれない
 */
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
	SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
	PTTSet ts = dlg_data == NULL ? NULL : dlg_data->pts;
	int i, w, sel;
	WORD Flow;

	switch (Message) {
		case WM_INITDIALOG:
			dlg_data = (SerialDlgData *)lParam;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);
			ts = dlg_data->pts;

			assert(dlg_data->ComPortInfoCount > 0);
			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

			w = 0;

			for (i = 0; i < dlg_data->ComPortInfoCount; i++) {
				ComPortInfo_t *p = dlg_data->ComPortInfoPtr + i;
				wchar_t *EntNameW;

				// MaxComPort を越えるポートは表示しない
				if (i > ts->MaxComPort) {
					continue;
				}

				aswprintf(&EntNameW, L"%s", p->port_name);
				SendDlgItemMessageW(Dialog, IDC_SERIALPORT, CB_ADDSTRING, 0, (LPARAM)EntNameW);
				free(EntNameW);

				if (p->port_no == ts->ComPort) {
					w = i;
				}
			}
			serial_dlg_set_comport_info(Dialog, dlg_data, w);
			SendDlgItemMessage(Dialog, IDC_SERIALPORT, CB_SETCURSEL, w, 0);

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
			Flow = ts->Flow;
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
			g_deltaSumSerialDlg = 0;
			g_defSerialDlgEditWndProc = (WNDPROC)SetWindowLongPtr(
				GetDlgItem(Dialog, IDC_SERIALTEXT),
				GWLP_WNDPROC,
				(LONG_PTR)SerialDlgEditWindowProc);

			// Combo-box controlをサブクラス化する。
			SetWindowLongPtrW(GetDlgItem(Dialog, IDC_SERIALBAUD), GWLP_USERDATA, (LONG_PTR)dlg_data);
			g_defSerialDlgSpeedComboboxWndProc = (WNDPROC)SetWindowLongPtr(
				GetDlgItem(Dialog, IDC_SERIALBAUD),
				GWLP_WNDPROC,
				(LONG_PTR)SerialDlgSpeedComboboxWindowProc);

			// 現在の接続状態と新しいポート番号の組み合わせで、接続処理が変わるため、
			// それに応じてOKボタンのラベル名を切り替える。
			serial_dlg_change_OK_button(Dialog, dlg_data->ComPortInfoPtr[w].port_no, ts->UILanguageFileW);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					if ( ts!=NULL ) {
						char Temp[128];
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
							/*
							 * display    value
							 * 1 Xon/Xoff 1 IdFlowX
							 * 2 RTS/CTS  2 IdFlowHard
							 * 3 DSR/DTR  4 IdFlowHardDsrDtr
							 * 4 none     3 IdFlowNone
							 */
							Flow = w;
							if (Flow == 3) {
								Flow = 4;
							}
							else if (Flow == 4) {
								Flow = 3;
							}
							ts->Flow = Flow;
						}

						ts->DelayPerChar = GetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,NULL,FALSE);

						ts->DelayPerLine = GetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,NULL,FALSE);

						ts->PortType = IdSerial;

						// ボーレートが変更されることがあるので、タイトル再表示の
						// メッセージを飛ばすようにした。 (2007.7.21 maya)
						PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);
					}

					// ツールチップを廃棄する
					if (g_SerialDlgSpeedTip) {
						TipWinDestroy(g_SerialDlgSpeedTip);
						g_SerialDlgSpeedTip = NULL;
					}

					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					// ツールチップを廃棄する
					if (g_SerialDlgSpeedTip) {
						TipWinDestroy(g_SerialDlgSpeedTip);
						g_SerialDlgSpeedTip = NULL;
					}

					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_SERIALHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpSetupSerialPort,0);
					return TRUE;

				case IDC_SERIALPORT:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						// リストからCOMポートが選択された
						int portno;
						sel = SendDlgItemMessage(Dialog, IDC_SERIALPORT, CB_GETCURSEL, 0, 0);
						portno = dlg_data->ComPortInfoPtr[sel].port_no;	 // ポート番号

						// 詳細情報を表示する
						serial_dlg_set_comport_info(Dialog, dlg_data, sel);

						// 現在の接続状態と新しいポート番号の組み合わせで、接続処理が変わるため、
						// それに応じてOKボタンのラベル名を切り替える。
						serial_dlg_change_OK_button(Dialog, portno, ts->UILanguageFileW);

						break;

					}

					return TRUE;
			}
	}
	return FALSE;
}

/**
 *	リストボックスの横スクロール幅を設定する
 */
static void ModifyListboxHScrollWidth(HWND dlg, int dlg_item)
{
	LRESULT i;
	LRESULT item_count;
	int max_width;

	HWND listbox = GetDlgItem(dlg, dlg_item);
	HDC listbox_dc = GetDC(listbox);
	HFONT listbox_font = (HFONT)SendMessageW(listbox, WM_GETFONT, 0, 0);
	HFONT listbox_font_old = (HFONT)SelectObject(listbox_dc, listbox_font);

	// リストボックス内の最大文字列幅取得
	max_width = 0;
	item_count = SendMessageW(listbox, LB_GETCOUNT, 0, 0);
	for (i = 0; i < item_count; i++) {
		SIZE size;
		wchar_t *strW;
		GetDlgItemIndexTextW(dlg, dlg_item, (WPARAM)i, &strW);
		GetTextExtentPoint32W(listbox_dc, strW, (int)wcslen(strW), &size);
		free(strW);
		if (max_width < size.cx) {
			max_width = size.cx;
		}
	}

	// 横幅を設定する
	SendMessageW(listbox, LB_SETHORIZONTALEXTENT, max_width + 5, 0);	// +5 幅に少し余裕を持たせる

	SelectObject(listbox_dc, listbox_font_old);
	ReleaseDC(listbox, listbox_dc);
}

/**
 *	リストボックス(ホスト履歴)をファイルに書き出す
 */
static void WriteListBoxHostHistory(HWND dlg, int dlg_item, int maxhostlist, const wchar_t *SetupFNW)
{
	LRESULT item_count;
	LRESULT i;

	item_count = SendDlgItemMessageW(dlg, dlg_item, LB_GETCOUNT, 0, 0);
	if (item_count == LB_ERR) {
		return;
	}

	// [Hosts] をすべて削除
	WritePrivateProfileStringW(L"Hosts", NULL, NULL, SetupFNW);

	if (item_count > maxhostlist) {
		item_count = maxhostlist;
	}
	for (i = 0; i < item_count; i++) {
		wchar_t EntNameW[10];
		wchar_t *strW;
		GetDlgItemIndexTextW(dlg, dlg_item, (WPARAM)i, &strW);
		_snwprintf_s(EntNameW, _countof(EntNameW), _TRUNCATE, L"Host%i", (int)i + 1);
		WritePrivateProfileStringW(L"Hosts", EntNameW, strW, SetupFNW);
		free(strW);
	}
}

/**
 *	Up/Remove/Downボタンをenable/disableする
 */
static void TCPIPDlgButtons(HWND Dialog)
{
	LRESULT item_count;
	LRESULT cur_pos;
	item_count = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCOUNT, 0, 0);
	cur_pos = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
	if ((cur_pos == LB_ERR) || (item_count == LB_ERR)) {
		DisableDlgItem(Dialog, IDC_TCPIPUP, IDC_TCPIPDOWN);
	}
	else {
		EnableDlgItem(Dialog, IDC_TCPIPREMOVE, IDC_TCPIPREMOVE);
		if (cur_pos == 0) {
			DisableDlgItem(Dialog, IDC_TCPIPUP, IDC_TCPIPUP);
		}
		else {
			EnableDlgItem(Dialog, IDC_TCPIPUP, IDC_TCPIPUP);
		}
		if (cur_pos + 1 >= item_count) {
			DisableDlgItem(Dialog, IDC_TCPIPDOWN, IDC_TCPIPDOWN);
		}
		else {
			EnableDlgItem(Dialog, IDC_TCPIPDOWN, IDC_TCPIPDOWN);
		}
	}
}

typedef struct {
	PTTSet ts;
	ReiseDlgHelper_t *resize_helper;
} TCPIPDlgData;

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
	static const ResizeHelperInfo resize_info[] = {
		{ IDC_TCPIPADD, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPUP, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPREMOVE, RESIZE_HELPER_ANCHOR_RT },
		{ IDC_TCPIPDOWN, RESIZE_HELPER_ANCHOR_RT },
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
		{ IDC_TCPIPHOST, RESIZE_HELPER_ANCHOR_LRT },
		{ IDC_TCPIPHOSTLIST, RESIZE_HELPER_ANCHOR_LRTB },
		{ IDC_TCPIPLIST, RESIZE_HELPER_ANCHOR_LRTB },
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

			SendDlgItemMessage(Dialog, IDC_TCPIPHOST, EM_LIMITTEXT, HostNameMaxLength - 1, 0);

			SetComboBoxHostHistory(Dialog, IDC_TCPIPLIST, MAXHOSTLIST, ts->SetupFNameW);
			ModifyListboxHScrollWidth(Dialog, IDC_TCPIPLIST);

			/* append a blank item to the bottom */
			SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_ADDSTRING, 0, 0);
			SetRB(Dialog, ts->HistoryList, IDC_TCPIPHISTORY, IDC_TCPIPHISTORY);
			SetRB(Dialog, ts->AutoWinClose, IDC_TCPIPAUTOCLOSE, IDC_TCPIPAUTOCLOSE);
			SetDlgItemInt(Dialog, IDC_TCPIPPORT, ts->TCPPort, FALSE);
			SetDlgItemInt(Dialog, IDC_TCPIPTELNETKEEPALIVE, ts->TelKeepAliveInterval, FALSE);
			SetRB(Dialog, ts->Telnet, IDC_TCPIPTELNET, IDC_TCPIPTELNET);
			SetDlgItemTextA(Dialog, IDC_TCPIPTERMTYPE, ts->TermType);
			SendDlgItemMessage(Dialog, IDC_TCPIPTERMTYPE, EM_LIMITTEXT, sizeof(ts->TermType) - 1, 0);

			// SSH接続のときにも TERM を送るので、telnetが無効でも disabled にしない。(2005.11.3 yutaka)
			EnableDlgItem(Dialog, IDC_TCPIPTERMTYPELABEL, IDC_TCPIPTERMTYPE);

			data->resize_helper = ReiseHelperInit(Dialog, resize_info, _countof(resize_info));

			CenterWindow(Dialog, GetParent(Dialog));

			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					TCPIPDlgData *data = (TCPIPDlgData *)GetWindowLongPtr(Dialog, DWLP_USER);
					PTTSet ts = data->ts;
					BOOL Ok;
					WriteListBoxHostHistory(Dialog, IDC_TCPIPLIST, MAXHOSTLIST, ts->SetupFNameW);
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

				case IDC_TCPIPHOST:
					if (HIWORD(wParam) == EN_CHANGE) {
						wchar_t *host;
						hGetDlgItemTextW(Dialog, IDC_TCPIPHOST, &host);
						if (wcslen(host) == 0) {
							DisableDlgItem(Dialog, IDC_TCPIPADD, IDC_TCPIPADD);
						}
						else {
							EnableDlgItem(Dialog, IDC_TCPIPADD, IDC_TCPIPADD);
						}
						free(host);
					}
					break;

				case IDC_TCPIPADD: {
					wchar_t *host;
					hGetDlgItemTextW(Dialog, IDC_TCPIPHOST, &host);
					if (wcslen(host) > 0) {
						LRESULT cur_pos;
						cur_pos = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
						if (cur_pos == LB_ERR) {
							cur_pos = 0;
						}
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_INSERTSTRING, cur_pos, (LPARAM)host);
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos, 0);
						SetDlgItemTextA(Dialog, IDC_TCPIPHOST, NULL);
						TCPIPDlgButtons(Dialog);
						ModifyListboxHScrollWidth(Dialog, IDC_TCPIPLIST);
						SetFocus(GetDlgItem(Dialog, IDC_TCPIPHOST));
					}
					free(host);
					break;
				}
				case IDC_TCPIPLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						TCPIPDlgButtons(Dialog);
					}
					break;

				case IDC_TCPIPUP:
				case IDC_TCPIPDOWN: {
					wchar_t *host;
					LRESULT item_count;
					LRESULT cur_pos;
					item_count = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCOUNT, 0, 0);
					cur_pos = SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
					if (item_count == LB_ERR || cur_pos == LB_ERR) {
						return TRUE;
					}
					if (LOWORD(wParam) == IDC_TCPIPDOWN) {
						cur_pos++;
					}
					if ((cur_pos == 0) || (cur_pos >= item_count)) {
						return TRUE;
					}
					GetDlgItemIndexTextW(Dialog, IDC_TCPIPLIST, cur_pos, &host);
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_DELETESTRING, cur_pos, 0);
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_INSERTSTRING, cur_pos - 1, (LPARAM)host);
					free(host);
					if (LOWORD(wParam) == IDC_TCPIPUP) {
						cur_pos--;
					}
					SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos, 0);
					TCPIPDlgButtons(Dialog);
					SetFocus(GetDlgItem(Dialog, IDC_TCPIPLIST));
					break;
				}

				case IDC_TCPIPREMOVE: {
					LRESULT item_count;
					LRESULT cur_pos;
					wchar_t *host;
					item_count = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCOUNT, 0, 0);
					cur_pos = SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_GETCURSEL, 0, 0);
					if ((item_count == LB_ERR) || (cur_pos == LB_ERR)) {
						return TRUE;
					}
					GetDlgItemIndexTextW(Dialog, IDC_TCPIPLIST, cur_pos, &host);
					SendDlgItemMessage(Dialog, IDC_TCPIPLIST, LB_DELETESTRING, cur_pos, 0);
					ModifyListboxHScrollWidth(Dialog, IDC_TCPIPLIST);
					if (item_count - 1 == 0) {
						// 0個になった
					}
					else if (cur_pos == item_count - 1) {
						// 一番最後を削除
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos - 1, 0);
					}
					else {
						SendDlgItemMessageW(Dialog, IDC_TCPIPLIST, LB_SETCURSEL, cur_pos, 0);
					}
					SetDlgItemTextW(Dialog, IDC_TCPIPHOST, host);
					TCPIPDlgButtons(Dialog);
					SetFocus(GetDlgItem(Dialog, IDC_TCPIPHOST));
					free(host);
					break;
				}

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

typedef struct {
	PGetHNRec GetHNRec;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
} TTXHostDlgData;

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
			GetHNRec = (PGetHNRec)lParam;
			dlg_data = (TTXHostDlgData *)calloc(sizeof(*dlg_data), 1);
			SetWindowLongPtr(Dialog, DWLP_USER, (LPARAM)dlg_data);
			dlg_data->GetHNRec = GetHNRec;

			dlg_data->ComPortInfoPtr = ComPortInfoGet(&dlg_data->ComPortInfoCount);

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

			// ファイルおよび名前付きパイプの場合、TCP/IP扱いとする。
			if ( GetHNRec->PortType==IdFile ||
				 GetHNRec->PortType==IdNamedPipe
				) {
				GetHNRec->PortType = IdTCPIP;
			}

			SetComboBoxHostHistory(Dialog, IDC_HOSTNAME, MAXHOSTLIST, GetHNRec->SetupFNW);
			ExpandCBWidth(Dialog, IDC_HOSTNAME);

			SendDlgItemMessage(Dialog, IDC_HOSTNAME, EM_LIMITTEXT,
			                   HostNameMaxLength-1, 0);

			SendDlgItemMessage(Dialog, IDC_HOSTNAME, CB_SETCURSEL,0,0);

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
			for (i = 0; i < dlg_data->ComPortInfoCount; i++) {
				ComPortInfo_t *p = dlg_data->ComPortInfoPtr + i;
				wchar_t *EntNameW;
				int index;

				// MaxComPort を越えるポートは表示しない
				if (i > GetHNRec->MaxComPort) {
					continue;
				}
				j++;

				// 使用中のポートは表示しない
				if (CheckCOMFlag(p->port_no) == 1) {
					continue;
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
				SendDlgItemMessage(Dialog, IDC_HOSTCOM, CB_SETCURSEL,0,0);
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
			}
			break;
		case WM_DESTROY:
			ComPortInfoFree(dlg_data->ComPortInfoPtr, dlg_data->ComPortInfoCount);
			free(dlg_data);
			break;
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

	switch (Message) {
		case WM_INITDIALOG: {
			PTTSet ts;
			wchar_t *CurDir;
			RECT R;
			HDC TmpDC;
			SIZE s;
			HWND HDir, HSel, HOk, HCancel, HHelp;
			POINT D, B, S;
			int WX, WY, WW, WH, CW, DW, DH, BW, BH, SW, SH;

			ts = (PTTSet)lParam;
			CurDir = ts->FileDirW;
			SetWindowLongPtr(Dialog, DWLP_USER, lParam);
			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);
			SetDlgItemTextW(Dialog, IDC_DIRCURRENT, CurDir);

// adjust dialog size
			// get size of current dir text
			HDir = GetDlgItem(Dialog, IDC_DIRCURRENT);
			GetWindowRect(HDir,&R);
			D.x = R.left;
			D.y = R.top;
			ScreenToClient(Dialog,&D);
			DH = R.bottom-R.top;
			TmpDC = GetDC(Dialog);
			GetTextExtentPoint32W(TmpDC,CurDir,(int)wcslen(CurDir),&s);
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
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					PTTSet ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
					BOOL OK = FALSE;
					wchar_t *current_dir;
					wchar_t *new_dir;
					hGetCurrentDirectoryW(&current_dir);
					hGetDlgItemTextW(Dialog, IDC_DIRNEW, &new_dir);
					if (wcslen(new_dir) > 0) {
						wchar_t *FileDirExpanded;
						hExpandEnvironmentStringsW(new_dir, &FileDirExpanded);
						if (DoesFolderExistW(FileDirExpanded)) {
							free(ts->FileDirW);
							ts->FileDirW = new_dir;
							OK = TRUE;
						}
						else {
							free(new_dir);
						}
						free(FileDirExpanded);
					}
					SetCurrentDirectoryW(current_dir);
					free(current_dir);
					if (OK) {
						EndDialog(Dialog, 1);
					}
					else {
						static const TTMessageBoxInfoW info = {
							"Tera Term",
							"MSG_TT_ERROR", L"Tera Term: Error",
							"MSG_FIND_DIR_ERROR", L"Cannot find directory",
							MB_ICONEXCLAMATION
						};
						TTMessageBoxW(Dialog, &info, ts->UILanguageFileW);
					}
					return TRUE;
				}

				case IDCANCEL:
					EndDialog(Dialog, 0);
					return TRUE;

				case IDC_SELECT_DIR: {
					PTTSet ts = (PTTSet)GetWindowLongPtr(Dialog,DWLP_USER);
					wchar_t *uimsgW;
					wchar_t *buf;
					wchar_t *FileDirExpanded;
					GetI18nStrWW("Tera Term", "DLG_SELECT_DIR_TITLE", L"Select new directory", ts->UILanguageFileW, &uimsgW);
					hGetDlgItemTextW(Dialog, IDC_DIRNEW, &buf);
					if (buf[0] == 0) {
						free(buf);
						hGetDlgItemTextW(Dialog, IDC_DIRCURRENT, &buf);
					}
					hExpandEnvironmentStringsW(buf, &FileDirExpanded);
					free(buf);
					if (doSelectFolderW(Dialog, FileDirExpanded, uimsgW, &buf)) {
						SetDlgItemTextW(Dialog, IDC_DIRNEW, buf);
						free(buf);
					}
					free(FileDirExpanded);
					free(uimsgW);
					return TRUE;
				}

				case IDC_DIRHELP:
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpFileChangeDir,0);
					break;
			}
	}
	return FALSE;
}

static const wchar_t **LangUIList = NULL;
#define LANG_EXT L".lng"

static const wchar_t *get_lang_folder()
{
	return (IsWindowsNTKernel()) ? L"lang_utf16le" : L"lang";
}

// メモリフリー
static void free_lang_ui_list()
{
	if (LangUIList) {
		const wchar_t **p = LangUIList;
		while (*p) {
			free((void *)*p);
			p++;
		}
		free((void *)LangUIList);
		LangUIList = NULL;
	}
}

static int make_sel_lang_ui(const wchar_t *dir)
{
	int    i;
	int    file_num;
	wchar_t *fullpath;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;

	free_lang_ui_list();

	aswprintf(&fullpath, L"%s\\%s\\*%s", dir, get_lang_folder(), LANG_EXT);

	file_num = 0;
	hFind = FindFirstFileW(fullpath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				file_num++;
			}
		} while(FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}

	file_num++;  // NULL
	LangUIList = calloc(file_num, sizeof(wchar_t *));

	i = 0;
	hFind = FindFirstFileW(fullpath, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				LangUIList[i++] = _wcsdup(fd.cFileName);
			}
		} while(FindNextFileW(hFind, &fd) && i < file_num);
		FindClose(hFind);
	}
	LangUIList[i] = NULL;
	free(fullpath);

	return i;
}

static int get_sel_lang_ui(const wchar_t **list, const wchar_t *selstr)
{
	size_t n = 0;
	const wchar_t **p = list;

	if (selstr == NULL || selstr[0] == '\0') {
		n = 0;  // English
	} else {
		while (*p) {
			if (wcsstr(selstr, *p)) {
				n = p - list;
				break;
			}
			p++;
		}
	}

	return (int)(n + 1);  // 1origin
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

			SetDlgTextsW(Dialog, TextInfos, _countof(TextInfos), ts->UILanguageFileW);

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
				int sel = 0;
				int i;
				ShowDlgItem(Dialog,IDC_GENLANGLABEL,IDC_GENLANG);
				for (i = 0;; i++) {
					const TLanguageList *lang = GetLanguageList(i);
					if (lang == NULL) {
						break;
					}
					if (ts->Language == lang->language) {
						sel = i;
					}
					SendDlgItemMessageA(Dialog, IDC_GENLANG, CB_ADDSTRING, 0, (LPARAM)lang->str);
					SendDlgItemMessageA(Dialog, IDC_GENLANG, CB_SETITEMDATA, i, (LPARAM)lang->language);
				}
				SendDlgItemMessage(Dialog, IDC_GENLANG, CB_SETCURSEL, sel, 0);
			}

			// 最初に指定されている言語ファイルの番号を覚えておく。
			uilist_count = make_sel_lang_ui(ts->ExeDirW);
			langui_sel = get_sel_lang_ui(LangUIList, ts->UILanguageFileW_ini);
			if (LangUIList[0] != NULL) {
				int i = 0;
				while (LangUIList[i] != 0) {
					SendDlgItemMessageW(Dialog, IDC_GENLANG_UI, CB_ADDSTRING, 0, (LPARAM)LangUIList[i]);
					i++;
				}
				SendDlgItemMessage(Dialog, IDC_GENLANG_UI, CB_SETCURSEL, langui_sel - 1, 0);
			}
			else {
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
							WORD language;
							w = (WORD)GetCurSel(Dialog, IDC_GENLANG);
							language = (int)SendDlgItemMessageA(Dialog, IDC_GENLANG, CB_GETITEMDATA, w - 1, 0);

							// Language が変更されたとき、
							// KanjiCode/KanjiCodeSend を変更先の Language に存在する値に置き換える
							if (1 <= language && language < IdLangMax && language != ts->Language) {
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
							aswprintf(&ts->UILanguageFileW_ini, L"%s\\%s", get_lang_folder(), LangUIList[w - 1]);
							WideCharToACP_t(ts->UILanguageFileW_ini, ts->UILanguageFile_ini, sizeof(ts->UILanguageFile_ini));

							ts->UILanguageFileW = GetUILanguageFileFullW(ts->ExeDirW, ts->UILanguageFileW_ini);
							WideCharToACP_t(ts->UILanguageFileW, ts->UILanguageFile, sizeof(ts->UILanguageFile));

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
					PostMessage(GetParent(Dialog),WM_USER_DLGHELP2,HlpSetupGeneral,0);
					break;
			}
			break;

		case WM_DESTROY:
			free_lang_ui_list();
			break;
	}
	return FALSE;
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
	BOOL r;
	SerialDlgData *dlg_data = calloc(sizeof(*dlg_data), 1);
	dlg_data->pts = ts;
	dlg_data->ComPortInfoPtr = ComPortInfoGet(&dlg_data->ComPortInfoCount);
	if (dlg_data->ComPortInfoCount == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_TT_NOTICE", L"Tera Term: Notice",
			NULL, L"No serial port",
			MB_ICONINFORMATION | MB_OK
		};
		TTMessageBoxW(WndParent, &info, ts->UILanguageFileW);
		return FALSE; // 変更しなかった
	}

	r = (BOOL)DialogBoxParam(hInst,
							 MAKEINTRESOURCE(IDD_SERIALDLG),
							 WndParent, SerialDlg, (LPARAM)dlg_data);

	ComPortInfoFree(dlg_data->ComPortInfoPtr, dlg_data->ComPortInfoCount);
	free(dlg_data);
	return r;
}

BOOL WINAPI _SetupTCPIP(HWND WndParent, PTTSet ts)
{
	TCPIPDlgData *data = (TCPIPDlgData *)calloc(sizeof(*data), 1);
	data->ts = ts;
	BOOL r= (BOOL)DialogBoxParam(hInst,
								 MAKEINTRESOURCE(IDD_TCPIPDLG),
								 WndParent, TCPIPDlg, (LPARAM)data);
	free(data);
	return r;
}

BOOL WINAPI _GetHostName(HWND WndParent, PGetHNRec GetHNRec)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_HOSTDLG),
		                     WndParent, HostDlg, (LPARAM)GetHNRec);
}

BOOL WINAPI _ChangeDirectory(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_DIRDLG),
		                     WndParent, DirDlg, (LPARAM)ts);
}

static UINT_PTR CALLBACK TFontHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) {
		case WM_INITDIALOG:
		{
			PTTSet ts;
			wchar_t *uimsg;

			//EnableWindow(GetDlgItem(Dialog, cmb2), FALSE);
			ts = (PTTSet)((CHOOSEFONTA *)lParam)->lCustData;

			GetI18nStrWW("Tera Term", "DLG_CHOOSEFONT_STC6", L"\"Font style\" selection here won't affect actual font appearance.",
						 ts->UILanguageFileW, &uimsg);
			SetDlgItemTextW(Dialog, stc6, uimsg);
			free(uimsg);

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

BOOL WINAPI _ChooseFontDlg(HWND WndParent, LPLOGFONTA LogFont, PTTSet ts)
{
	CHOOSEFONTA cf;
	BOOL Ok;

	memset(&cf, 0, sizeof(cf));
	cf.lStructSize = sizeof(cf);
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
	Ok = ChooseFontA(&cf);
	return Ok;
}

BOOL WINAPI _SetupGeneral(HWND WndParent, PTTSet ts)
{
	return
		(BOOL)DialogBoxParam(hInst,
		                     MAKEINTRESOURCE(IDD_GENDLG),
		                     WndParent, GenDlg, (LPARAM)ts);
}
