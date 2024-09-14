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
#include "ttcommon.h"
#include "dlg_res.h"
#include "tipwin.h"
#include "comportinfo.h"
#include "codeconv.h"
#include "helpid.h"
#include "asprintf.h"
#include "win32helper.h"
#include "compat_win.h"
#include "asprintf.h"
#include "ttwinman.h"

#include "ttdlg.h"

static const char *BaudList[] =
	{"110","300","600","1200","2400","4800","9600",
	 "14400","19200","38400","57600","115200",
	 "230400", "460800", "921600", NULL};

typedef struct {
	PTTSet pts;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
} SerialDlgData;

static const char *DataList[] = {"7 bit","8 bit",NULL};
static const char *ParityList[] = {"none", "odd", "even", "mark", "space", NULL};
static const char *StopList[] = {"1 bit", "2 bit", NULL};
static const char *FlowList[] = {"Xon/Xoff", "RTS/CTS", "DSR/DTR", "none", NULL};
static int g_deltaSumSerialDlg = 0;        // �}�E�X�z�C�[����Delta�ݐϗp
static WNDPROC g_defSerialDlgEditWndProc;  // Edit Control�̃T�u�N���X���p
static WNDPROC g_defSerialDlgSpeedComboboxWndProc;  // Combo-box Control�̃T�u�N���X���p
static TipWin *g_SerialDlgSpeedTip;

/*
 * �V���A���|�[�g�ݒ�_�C�A���O��OK�{�^����ڑ���ɉ����Ė��̂�؂�ւ���B
 * ��������� OnSetupSerialPort() �ƍ��킹��K�v������B
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
 * �V���A���|�[�g�ݒ�_�C�A���O�̃e�L�X�g�{�b�N�X��COM�|�[�g�̏ڍ׏���\������B
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
 * �V���A���|�[�g�ݒ�_�C�A���O�̃e�L�X�g�{�b�N�X�̃v���V�[�W��
 */
static LRESULT CALLBACK SerialDlgEditWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	WORD keys;
	short delta;
	BOOL page;

	switch (msg) {
		case WM_KEYDOWN:
			// Edit control��� CTRL+A ����������ƁA�e�L�X�g��S�I������B
			if (wp == 'A' && GetKeyState(VK_CONTROL) < 0) {
				PostMessage(hWnd, EM_SETSEL, 0, -1);
				return 0;
			}
			break;

		case WM_MOUSEWHEEL:
			// CTRLorSHIFT + �}�E�X�z�C�[���̏ꍇ�A���X�N���[��������B
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
 * �V���A���|�[�g�ݒ�_�C�A���O��SPEED(BAUD)�̃v���V�[�W��
 */
static LRESULT CALLBACK SerialDlgSpeedComboboxWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	const int tooltip_timeout = 1000;  // msec
	int h;
	int cx, cy;
	RECT wr;
	SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	int frame_width;

	switch (msg) {
		case WM_MOUSEMOVE:
			// �c�[���`�b�v���쐬
			if (g_SerialDlgSpeedTip == NULL) {
				const wchar_t *UILanguageFileW;
				wchar_t *uimsg;
				UILanguageFileW = dlg_data->pts->UILanguageFileW;
				uimsg = TTGetLangStrW("Tera Term",
									  "DLG_SERIAL_SPEED_TOOLTIP", L"You can directly specify a number", UILanguageFileW);
				g_SerialDlgSpeedTip = TipWinCreate(hInst, hWnd);
				TipWinSetTextW(g_SerialDlgSpeedTip, uimsg);

				free(uimsg);
			}

			// Combo-box�̍�����W�����߂�
			GetWindowRect(hWnd, &wr);

			// �c�[���`�b�v��\������
			TipWinGetWindowSize(g_SerialDlgSpeedTip, NULL, &h);
			TipWinGetFrameSize(g_SerialDlgSpeedTip, &frame_width);
			cx = wr.left;
			cy = wr.top - (h + frame_width * 4);
			TipWinSetPos(g_SerialDlgSpeedTip, cx, cy);
			TipWinSetHideTimer(g_SerialDlgSpeedTip, tooltip_timeout);
			if (!TipWinIsVisible(g_SerialDlgSpeedTip))
				TipWinSetVisible(g_SerialDlgSpeedTip, TRUE);

			break;
	}
    return CallWindowProc(g_defSerialDlgSpeedComboboxWndProc, hWnd, msg, wp, lp);
}

/*
 * �V���A���|�[�g�ݒ�_�C�A���O
 *
 * �V���A���|�[�g����0�̎��͌Ă΂�Ȃ�
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

				// MaxComPort ���z����|�[�g�͕\�����Ȃ�
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

			// Edit control���T�u�N���X������B
			g_deltaSumSerialDlg = 0;
			g_defSerialDlgEditWndProc = (WNDPROC)SetWindowLongPtr(
				GetDlgItem(Dialog, IDC_SERIALTEXT),
				GWLP_WNDPROC,
				(LONG_PTR)SerialDlgEditWindowProc);

			// Combo-box control���T�u�N���X������B
			SetWindowLongPtrW(GetDlgItem(Dialog, IDC_SERIALBAUD), GWLP_USERDATA, (LONG_PTR)dlg_data);
			g_defSerialDlgSpeedComboboxWndProc = (WNDPROC)SetWindowLongPtr(
				GetDlgItem(Dialog, IDC_SERIALBAUD),
				GWLP_WNDPROC,
				(LONG_PTR)SerialDlgSpeedComboboxWindowProc);

			// ���݂̐ڑ���ԂƐV�����|�[�g�ԍ��̑g�ݍ��킹�ŁA�ڑ��������ς�邽�߁A
			// ����ɉ�����OK�{�^���̃��x������؂�ւ���B
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

						// �{�[���[�g���ύX����邱�Ƃ�����̂ŁA�^�C�g���ĕ\����
						// ���b�Z�[�W���΂��悤�ɂ����B (2007.7.21 maya)
						PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);
					}

					// �c�[���`�b�v��p������
					if (g_SerialDlgSpeedTip) {
						TipWinDestroy(g_SerialDlgSpeedTip);
						g_SerialDlgSpeedTip = NULL;
					}

					EndDialog(Dialog, 1);
					return TRUE;

				case IDCANCEL:
					// �c�[���`�b�v��p������
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
						// ���X�g����COM�|�[�g���I�����ꂽ
						int portno;
						sel = SendDlgItemMessage(Dialog, IDC_SERIALPORT, CB_GETCURSEL, 0, 0);
						portno = dlg_data->ComPortInfoPtr[sel].port_no;	 // �|�[�g�ԍ�

						// �ڍ׏���\������
						serial_dlg_set_comport_info(Dialog, dlg_data, sel);

						// ���݂̐ڑ���ԂƐV�����|�[�g�ԍ��̑g�ݍ��킹�ŁA�ڑ��������ς�邽�߁A
						// ����ɉ�����OK�{�^���̃��x������؂�ւ���B
						serial_dlg_change_OK_button(Dialog, portno, ts->UILanguageFileW);

						break;

					}

					return TRUE;
			}
	}
	return FALSE;
}

BOOL WINAPI _SetupSerialPort(HWND WndParent, PTTSet ts)
{
	BOOL r;
	SerialDlgData *dlg_data = (SerialDlgData *)calloc(1, sizeof(*dlg_data));
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
		return FALSE; // �ύX���Ȃ�����
	}

	r = (BOOL)TTDialogBoxParam(hInst,
							   MAKEINTRESOURCEW(IDD_SERIALDLG),
							   WndParent, SerialDlg, (LPARAM)dlg_data);

	ComPortInfoFree(dlg_data->ComPortInfoPtr, dlg_data->ComPortInfoCount);
	free(dlg_data);
	return r;
}
