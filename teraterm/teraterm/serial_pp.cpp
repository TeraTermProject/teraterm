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

// �e���v���[�g�̏����������s��
#define REWRITE_TEMPLATE

// ���݂���V���A���|�[�g�݂̂�\������
//#define SHOW_ONLY_EXSITING_PORT		TRUE
#define SHOW_ONLY_EXSITING_PORT		FALSE	// ���ׂẴ|�[�g��\������

// �h���b�v�_�E������|�[�g�\����؂�ւ�
//#define ENABLE_SWITCH_PORT_DISPLAY	1
#define ENABLE_SWITCH_PORT_DISPLAY	0

// MaxComPort���傫�ȃ|�[�g�����݂���Ƃ��AMaxComPort���I�[�o�[���C�h����
#define ENABLE_MAXCOMPORT_OVERRIDE	0

static const char *BaudList[] = {
	"110","300","600","1200","2400","4800","9600",
	"14400","19200","38400","57600","115200",
	"230400", "460800", "921600", NULL};
static const char *DataList[] = {"7 bit","8 bit",NULL};
static const char *ParityList[] = {"none", "odd", "even", "mark", "space", NULL};
static const char *StopList[] = {"1 bit", "2 bit", NULL};
static const char *FlowList[] = {"XON/XOFF", "RTS/CTS", "DSR/DTR", "NONE", NULL};

typedef struct {
	PTTSet pts;
	ComPortInfo_t *ComPortInfoPtr;
	int ComPortInfoCount;
	HINSTANCE hInst;
	DLGTEMPLATE *dlg_templ;
	int g_deltaSumSerialDlg;					// �}�E�X�z�C�[����Delta�ݐϗp
	WNDPROC g_defSerialDlgEditWndProc;			// Edit Control�̃T�u�N���X���p
	BOOL show_all_port;
} SerialDlgData;

/*
 * �V���A���|�[�g�ݒ�_�C�A���O�̃e�L�X�g�{�b�N�X��COM�|�[�g�̏ڍ׏���\������B
 *	@param	port_index		port_info[] �̃C���f�b�N�X, -1�̂Ƃ��g�p���Ȃ�
 *	@param	port_no			�|�[�g�ԍ� "com%d", -1�̂Ƃ��g�p���Ȃ�
 *	@param	port_name		port_name �|�[�g��
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
 * �V���A���|�[�g�ݒ�_�C�A���O�̃e�L�X�g�{�b�N�X�̃v���V�[�W��
 */
static LRESULT CALLBACK SerialDlgEditWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

	switch (msg) {
		case WM_KEYDOWN:
			// Edit control��� CTRL+A ����������ƁA�e�L�X�g��S�I������B
			if (wp == 'A' && GetKeyState(VK_CONTROL) < 0) {
				PostMessage(hWnd, EM_SETSEL, 0, -1);
				return 0;
			}
			break;

		case WM_MOUSEWHEEL: {
			// CTRLorSHIFT + �}�E�X�z�C�[���̏ꍇ�A���X�N���[��������B
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
 *	�V���A���|�[�g�h���b�v�_�E����ݒ肷��
 *	ITEMDATA
 *		0...	PortInfo�̐擪����̔ԍ� 0����PortInfoCount-1�܂�
 *		-1		���ݑ��݂��Ȃ��|�[�g
 *		-2		"���݂���|�[�g�̂ݕ\��"
 *		-3		"�S�Ẵ|�[�g��\��"
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
		// �|�[�g�����݂��Ă��Ȃ��Ƃ��͂��ׂĕ\������
		show_all_port = TRUE;
	}

	// "COM%d" �ł͂Ȃ��|�[�g
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

	// �`���I�� "COM%d"
	int sel_index = 0;
	int com_count = 0;
	BOOL all = show_all_port;
	for (int i = 1; i <= max_com; i++) {

		wchar_t *com_name = NULL;
		int item_data;
		if (port_index < dlg_data->ComPortInfoCount) {
			p = port_info + port_index;
			if (p->port_no == i) {
				// ComPortInfo �Ō��o�ł����|�[�g
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
			// ���o�ł��Ȃ������|�[�g(���݂��Ȃ��|�[�g)
			if (!all && (i != ts->ComPort)) {
				// ���ׂĕ\�����Ȃ�
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
		// COM�|�[�g�Ȃ�
		serial_dlg_set_comport_info(hWnd, dlg_data, -1, -1, NULL);
	}
	else {
		if (cv.Open && (cv.PortType == IdSerial)) {
			// �ڑ����̎��͑I���ł��Ȃ��悤�ɂ���
			EnableWindow(GetDlgItem(hWnd, id), FALSE);
		}
		serial_dlg_set_comport_info(hWnd, dlg_data, -1, ts->ComPort, NULL);
	}
	SendDlgItemMessage(hWnd, id, CB_SETCURSEL, sel_index - 1, 0);  // sel_index = 1 origin
}

/*
 * �V���A���|�[�g�ݒ�_�C�A���O
 *
 * �V���A���|�[�g����0�̎��͌Ă΂�Ȃ�
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

			// Edit control���T�u�N���X������B
			dlg_data->g_deltaSumSerialDlg = 0;
			SetWindowLongPtrW(GetDlgItem(Dialog, IDC_SERIALTEXT), GWLP_USERDATA, (LONG_PTR)dlg_data);
			dlg_data->g_defSerialDlgEditWndProc = (WNDPROC)SetWindowLongPtrW(
				GetDlgItem(Dialog, IDC_SERIALTEXT),
				GWLP_WNDPROC,
				(LONG_PTR)SerialDlgEditWindowProc);

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

				ts->DelayPerChar = GetDlgItemInt(Dialog,IDC_SERIALDELAYCHAR,NULL,FALSE);

				ts->DelayPerLine = GetDlgItemInt(Dialog,IDC_SERIALDELAYLINE,NULL,FALSE);

				// TODO �폜
				// �S�ʃ^�u�́u�W���̃|�[�g�v�Ńf�t�H���g�̐ڑ��|�[�g���w�肷��
				// �����ł͎w�肵�Ȃ�
				// ts->PortType = IdSerial;

				// �{�[���[�g���ύX����邱�Ƃ�����̂ŁA�^�C�g���ĕ\����
				// ���b�Z�[�W���΂��悤�ɂ����B (2007.7.21 maya)
				PostMessage(GetParent(Dialog),WM_USER_CHANGETITLE,0,0);

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
						// ���X�g����COM�|�[�g���I�����ꂽ
						SerialDlgData *dlg_data = (SerialDlgData *)GetWindowLongPtrW(Dialog, DWLP_USER);
						LRESULT sel = SendDlgItemMessageA(Dialog, IDC_SERIALPORT, CB_GETCURSEL, 0, 0);
						int item_data = (int)SendDlgItemMessageA(Dialog, IDC_SERIALPORT, CB_GETITEMDATA, sel, 0);
						if (item_data >= 0) {
							// �ڍ׏���\������
							serial_dlg_set_comport_info(Dialog, dlg_data, item_data, -1, NULL);
						}
						else if (item_data == -1) {
							// ���Ȃ���\������
							serial_dlg_set_comport_info(Dialog, dlg_data, -1, -1, NULL);
						}
						else {
							// �I����@�ύX
							dlg_data->show_all_port = (item_data == -2) ? FALSE : TRUE;
							SendDlgItemMessageA(Dialog, IDC_SERIALPORT, CB_RESETCONTENT, 0, 0);
							SetPortDrop(Dialog, IDC_SERIALPORT, dlg_data);
						}
						break;
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
