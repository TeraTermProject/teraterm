#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "ttlib.h"
#include "tt_res.h"
#include "resource.h"
#include "i18n.h"
#include "dlglib.h"
#include "inifile_com.h"
#include "codeconv.h"

#define ORDER 4000

#define MINIMUM_INTERVAL 1
#define DEFAULT_INTERVAL 300

#define IdRecurringTimer 3001

#define ID_MENU_SETUP   55500
#define ID_MENU_CONTROL 55501
#define ID_MENU_ENABLE  55502
#define ID_MENU_DISABLE 55503

#define SECTION "TTXRecurringCommand"

static HANDLE hInst;

typedef struct {
	PTTSet ts;
	PComVar cv;
	Tsend origPsend;
	TWriteFile origPWriteFile;
	PParseParam origParseParam;
	PReadIniFile origReadIniFile;
	PWriteIniFile origWriteIniFile;
	HMENU SetupMenu;
	HMENU ControlMenu;
	int interval;
	BOOL enable;
	int cmdLen;
	BOOL add_nl;
	unsigned char command[OutBuffSize];
	unsigned char orgCommand[OutBuffSize];
} TInstVar;

typedef TInstVar *PTInstVar;
PTInstVar pvar;
static TInstVar InstVar;

#define GetFileMenu(menu)       GetSubMenuByChildID(menu, ID_FILE_NEWCONNECTION)
#define GetEditMenu(menu)       GetSubMenuByChildID(menu, ID_EDIT_COPY2)
#define GetSetupMenu(menu)      GetSubMenuByChildID(menu, ID_SETUP_TERMINAL)
#define GetControlMenu(menu)    GetSubMenuByChildID(menu, ID_CONTROL_RESETTERMINAL)
#define GetHelpMenu(menu)       GetSubMenuByChildID(menu, ID_HELP_ABOUT)

HMENU GetSubMenuByChildID(HMENU menu, UINT id) {
	int i, j, items, subitems, cur_id;
	HMENU m;

	items = GetMenuItemCount(menu);

	for (i=0; i<items; i++) {
		if (m = GetSubMenu(menu, i)) {
			subitems = GetMenuItemCount(m);
			for (j=0; j<subitems; j++) {
				cur_id = GetMenuItemID(m, j);
				if (cur_id == id) {
					return m;
				}
			}
		}
	}
	return NULL;
}

WORD GetOnOff(PCHAR Sect, PCHAR Key, const wchar_t *FName, BOOL Default)
{
	char Temp[4];
	GetPrivateProfileStringAFileW(Sect, Key, "", Temp, sizeof(Temp), FName);
	if (Default) {
		if (_stricmp(Temp, "off") == 0)
			return 0;
		else
			return 1;
	}
	else {
		if (_stricmp(Temp, "on") == 0)
			return 1;
		else
			return 0;
	}
}

//
// \n, \t����W�J����B
// common/ttlib.c:RestoreNewLine()���x�[�X�B
//
int UnEscapeStr(BYTE *Text)
{
	int i;
	unsigned char *src, *dst;

	src = dst = Text;
	while (*src && *src != '\\') {
		src++; dst++;
	}

	while (*src) {
		if (*src == '\\') {
			switch (*++src) {
				case '\\': *dst = '\\';   break;
				case 'n':  *dst = '\n';   break;
				case 't':  *dst = '\t';   break;
				case 'a':  *dst = '\a';   break;
				case 'b':  *dst = '\b';   break;
				case 'f':  *dst = '\f';   break;
				case 'r':  *dst = '\r';   break;
				case 'v':  *dst = '\v';   break;
				case 'e':  *dst = '\033'; break;
//				case '"':  *dst = '"';    break;
//				case '\'': *dst = '\'';   break;
				case 'x':
					if (isxdigit(src[1]) && isxdigit(src[2])) {
						src++;
						if (isalpha(*src)) {
							*dst = (*src|0x20) - 'a' + 10;
						}
						else {
							*dst = *src - '0';
						}
						*dst <<= 4;

						src++;
						if (isalpha(*src)) {
							*dst += (*src|0x20) - 'a' + 10;
						}
						else {
							*dst += *src - '0';
						}
					}
					else {
						*dst++ = '\\';
						*dst = 'x';
					}
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					*dst = 0;
					for (i=0; i<3; i++) {
						if (*src < '0' || *src > '7')
							break;
						*dst = (*dst << 3) + *src - '0';
						src++;
					}
					src--;
					break;
				default:
					*dst = '\\';
					src--;
			}
		}
		else {
			*dst = *src;
		}
		src++; dst++;
	}

	*dst = '\0';

	return (int)(dst - Text);
}

//
//  �o�̓o�b�t�@�ɕ��������������
//  ttpcmn/ttcmn.c:CommTextOut()���x�[�X
//
void CommOut(char *str, int len) {
	int outlen, c;
	char *p;

	if (len > OutBuffSize - pvar->cv->OutBuffCount)
		outlen = OutBuffSize - pvar->cv->OutBuffCount;
	else
		outlen = len;

	if (pvar->cv->OutPtr > 0) {
		memmove(pvar->cv->OutBuff, &(pvar->cv->OutBuff[pvar->cv->OutPtr]), pvar->cv->OutBuffCount);
		pvar->cv->OutPtr = 0;
	}

	c = pvar->cv->OutBuffCount;
	for (p=str; outlen>0; p++, outlen--) {
		switch (*p) {
		  case 0x0d:
			switch (pvar->cv->CRSend) {
			  case IdCR:
				pvar->cv->OutBuff[c++] = 0x0d;
				if (c < OutBuffSize && pvar->cv->TelFlag && ! pvar->cv->TelBinSend) {
					pvar->cv->OutBuff[c++] = 0;
				}
				break;
			  case IdLF:
				pvar->cv->OutBuff[c++] = 0x0a;
				break;
			  case IdCRLF:
				pvar->cv->OutBuff[c++] = 0x0d;
				if (c < OutBuffSize) {
					pvar->cv->OutBuff[c++] = 0x0a;
				}
				break;
			}
			if (c + outlen > OutBuffSize) {
				outlen--;
			}
			break;
		  case 0xff:
			if (pvar->cv->TelFlag) {
				if (c < OutBuffSize - 1) {
					pvar->cv->OutBuff[c++] = 0xff;
					pvar->cv->OutBuff[c++] = 0xff;
				}
			}
			else {
				pvar->cv->OutBuff[c++] = 0xff;
			}
			if (c + outlen > OutBuffSize) {
				outlen--;
			}
			break;
		  default:
			pvar->cv->OutBuff[c++] = *p;
		}
	}

	pvar->cv->OutBuffCount = c;
}

//
// �^�C�}���^�C���A�E�g�������̃R�[���o�b�N�֐�
//	�L���ɂ���Ă���Ȃ�΁A�o�̓o�b�t�@�ɃR�}���h����������
//
void CALLBACK RecurringTimerProc(HWND hwnd, UINT msg, UINT_PTR ev, DWORD now) {
	if (pvar->enable && pvar->cmdLen > 0 && pvar->cv->Ready && pvar->cv->OutBuffCount == 0) {
		CommOut(pvar->command, pvar->cmdLen);
	}
//	SendMessage(hwnd, WM_IDLE, 0, 0);
	return;
}

//
//  TTXInit -- �N��������
//
static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origPsend = NULL;
	pvar->origPWriteFile = NULL;
	pvar->origReadIniFile = NULL;
	pvar->origWriteIniFile = NULL;
	pvar->interval = DEFAULT_INTERVAL;
}

//
//  TTXSend, TTXWriteFile -- �L�[���͏���
//	�L�[���͂���������A�^�C�}����������
//
static int PASCAL TTXsend(SOCKET s, const char *buf, int len, int flags) {
	if (pvar->enable && len > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
	return pvar->origPsend(s, buf, len, flags);
}

static BOOL PASCAL TTXWriteFile(HANDLE fh, LPCVOID buff, DWORD len, LPDWORD wbytes, LPOVERLAPPED wol) {
	if (pvar->enable && len > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
	return pvar->origPWriteFile(fh, buff, len, wbytes, wol);
}

//
// TTXOpenTCP, TTXOpenFile -- �Z�b�V�����J�n����
//	Psend, WriteFile���t�b�N���A�L���Ȃ�΃^�C�}���Z�b�g����B
//
static void PASCAL TTXOpenTCP(TTXSockHooks *hooks) {
	pvar->origPsend = *hooks->Psend;
	*hooks->Psend = TTXsend;

	if (pvar->enable && pvar->cmdLen > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
}

static void PASCAL TTXOpenFile(TTXFileHooks *hooks) {
	pvar->origPWriteFile = *hooks->PWriteFile;
	*hooks->PWriteFile = TTXWriteFile;

	if (pvar->enable && pvar->cmdLen > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
}

//
// TTXCloseTCP, TTXCloseFile -- �Z�b�V�����I��������
//	Psend, WriteFile�̃t�b�N���������A�^�C�}���~�߂�B
//
static void PASCAL TTXCloseTCP(TTXSockHooks *hooks) {
	if (pvar->origPsend) {
		*hooks->Psend = pvar->origPsend;
	}
	KillTimer(pvar->cv->HWin, IdRecurringTimer);
}

static void PASCAL TTXCloseFile(TTXFileHooks *hooks) {
	if (pvar->origPWriteFile) {
		*hooks->PWriteFile = pvar->origPWriteFile;
	}
	KillTimer(pvar->cv->HWin, IdRecurringTimer);
}

//
// TTXReadIniFile, TTXWriteIniFile -- �ݒ�t�@�C���̓ǂݏ���
//
void ReadINI(const wchar_t *fn, PTTSet ts) {
	GetPrivateProfileStringAFileW(SECTION, "Command", "", pvar->orgCommand, sizeof(pvar->orgCommand), fn);
	strncpy_s(pvar->command, sizeof(pvar->command), pvar->orgCommand, _TRUNCATE);
	UnEscapeStr(pvar->command);
	pvar->cmdLen = (int)strlen(pvar->command);

	pvar->add_nl = GetOnOff(SECTION, "AddNewLine", fn, FALSE);
	if (pvar->add_nl && pvar->cmdLen < sizeof(pvar->command) - 1) {
		pvar->command[pvar->cmdLen++] = '\n';
		pvar->command[pvar->cmdLen] = '\0';
	}

	pvar->interval = GetPrivateProfileIntAFileW(SECTION, "Interval", DEFAULT_INTERVAL, fn);
	if (pvar->interval < MINIMUM_INTERVAL) {
		pvar->interval = MINIMUM_INTERVAL;
	}

	pvar->enable = GetOnOff(SECTION, "Enable", fn, FALSE);
}

static void PASCAL TTXReadIniFile(const wchar_t *fn, PTTSet ts) {
	pvar->origReadIniFile(fn, ts);
	ReadINI(fn, ts);

	return;
}

static void PASCAL TTXWriteIniFile(const wchar_t *fn, PTTSet ts) {
	char buff[20];

	pvar->origWriteIniFile(fn, ts);

	WritePrivateProfileStringAFileW(SECTION, "Enable", pvar->enable?"on":"off", fn);

	WritePrivateProfileStringAFileW(SECTION, "Command", pvar->orgCommand, fn);

	_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%d", pvar->interval);
	WritePrivateProfileStringAFileW(SECTION, "Interval", buff, fn);

	WritePrivateProfileStringAFileW(SECTION, "AddNewLine", pvar->add_nl?"on":"off", fn);

	return;
}

//
// TTXParseParam -- �R�}���h���C���I�v�V�����̉���
//	���̂Ƃ���ŗL�̃R�}���h���C���I�v�V�����͖����B(�K�v?)
//

static void PASCAL TTXParseParam(wchar_t *Param, PTTSet ts, PCHAR DDETopic) {
	pvar->origParseParam(Param, ts, DDETopic);

	return;
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXReadIniFile;

	pvar->origWriteIniFile = *hooks->WriteIniFile;
	*hooks->WriteIniFile = TTXWriteIniFile;

	pvar->origParseParam = *hooks->ParseParam;
	*hooks->ParseParam = TTXParseParam;

	return;
}

//
// ���j���[����
//	�R���g���[�����j���[��RecurringCommand��ǉ��B
//
static void PASCAL TTXModifyMenu(HMENU menu) {
	static const DlgTextInfo MenuTextInfo[] = {
		{ ID_MENU_SETUP, "MENU_SETUP_RECURRING" },
		{ ID_MENU_CONTROL, "MENU_CONTROL_RECURRING" },
	};
	UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

	pvar->SetupMenu = GetSetupMenu(menu);
	pvar->ControlMenu = GetControlMenu(menu);

	InsertMenu(pvar->SetupMenu, ID_SETUP_ADDITIONALSETTINGS, flag, ID_MENU_SETUP, "Rec&urring command");

	if (pvar->enable) {
		flag |= MF_CHECKED;
	}

	InsertMenu(pvar->ControlMenu, ID_CONTROL_MACRO, flag, ID_MENU_CONTROL, "Rec&urring command");
	InsertMenu(pvar->ControlMenu, ID_CONTROL_MACRO, MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);

	SetI18nMenuStrsW(menu, SECTION, MenuTextInfo, _countof(MenuTextInfo), pvar->ts->UILanguageFileW);
}

static void PASCAL TTXModifyPopupMenu(HMENU menu) {
	if (menu==pvar->ControlMenu) {
		if (pvar->enable) {
			CheckMenuItem(pvar->ControlMenu, ID_MENU_CONTROL, MF_BYCOMMAND | MF_CHECKED);
		}
		else {
			CheckMenuItem(pvar->ControlMenu, ID_MENU_CONTROL, MF_BYCOMMAND | MF_UNCHECKED);
		}
	}
}

//
// RecurringCommand�ݒ�_�C�A���O�̃R�[���o�b�N�֐��B
//
static INT_PTR CALLBACK RecurringCommandSetting(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_TITLE" },
		{ IDC_ENABLE, "DLG_ENABLE" },
		{ IDC_INTERVAL_LABEL, "DLG_INTERVAL" },
		{ IDC_COMMAND_LABEL, "DLG_COMMAND" },
		{ IDC_ADD_NL, "DLG_ADD_NEWLINE" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
	};

	switch (msg) {
	  case WM_INITDIALOG:
		SetI18nDlgStrsW(dlg, SECTION, text_info, _countof(text_info), pvar->ts->UILanguageFileW);

		SendMessage(GetDlgItem(dlg, IDC_ENABLE), BM_SETCHECK,
		            pvar->enable?BST_CHECKED:BST_UNCHECKED, 0);
		SetDlgItemInt(dlg, IDC_INTERVAL, pvar->interval, FALSE);
		SetDlgItemText(dlg, IDC_COMMAND, pvar->orgCommand);
		SendMessage(GetDlgItem(dlg, IDC_ADD_NL), BM_SETCHECK,
		            pvar->add_nl?BST_CHECKED:BST_UNCHECKED, 0);

		CenterWindow(dlg, GetParent(dlg));

		return TRUE;
	  case WM_COMMAND:
		switch (LOWORD(wParam)) {
		  case IDOK:
			pvar->enable = IsDlgButtonChecked(dlg, IDC_ENABLE) == BST_CHECKED;

			pvar->interval = GetDlgItemInt(dlg, IDC_INTERVAL, NULL, FALSE);
			if (pvar->interval < MINIMUM_INTERVAL) {
				pvar->interval = MINIMUM_INTERVAL;
			}

			GetDlgItemText(dlg, IDC_COMMAND, pvar->orgCommand, sizeof(pvar->orgCommand));
			strncpy_s(pvar->command, sizeof(pvar->command), pvar->orgCommand, _TRUNCATE);
			UnEscapeStr(pvar->command);
			pvar->cmdLen = (int)strlen(pvar->command);

			pvar->add_nl = IsDlgButtonChecked(dlg, IDC_ADD_NL) == BST_CHECKED;
			if (pvar->add_nl && pvar->cmdLen < sizeof(pvar->command) - 1) {
				pvar->command[pvar->cmdLen++] = '\n';
				pvar->command[pvar->cmdLen] = '\0';
			}

			if (pvar->cv->Ready) {
				if (pvar->enable) {
					SetTimer(pvar->cv->HWin, IdRecurringTimer,
					         pvar->interval * 1000, RecurringTimerProc);
				}
				else {
					KillTimer(pvar->cv->HWin, IdRecurringTimer);
				}
			}

			EndDialog(dlg, IDOK);
			return TRUE;

		  case IDCANCEL:
			EndDialog(dlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

//
// ���j���[�R�}���h����
//
static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
	switch (cmd) {
	  case ID_MENU_SETUP:
		SetDialogFont(pvar->ts->DialogFontNameW, pvar->ts->DialogFontPoint, pvar->ts->DialogFontCharSet,
					  pvar->ts->UILanguageFileW, SECTION, "DLG_TAHOMA_FONT");
		switch (TTDialogBoxParam(hInst, MAKEINTRESOURCEW(IDD_SETUP_RECURRINGCOMMAND),
								 hWin, RecurringCommandSetting, (LPARAM)NULL)) {
		  case IDOK:
			break;
		  case IDCANCEL:
			break;
		  case -1:
			MessageBox(hWin, "TTXRecurringCommand: Error", "Can't display dialog box.",
			           MB_OK | MB_ICONEXCLAMATION);
			break;
		}
		return 1;

	  case ID_MENU_CONTROL:
		pvar->enable = !pvar->enable;
		if (pvar->enable) {
			SetTimer(pvar->cv->HWin, IdRecurringTimer,
			         pvar->interval * 1000, RecurringTimerProc);
		}
		else {
			KillTimer(pvar->cv->HWin, IdRecurringTimer);
		}
		return 1;

	  case ID_MENU_ENABLE:
	  	if (!pvar->enable) {
			pvar->enable = TRUE;
			SetTimer(pvar->cv->HWin, IdRecurringTimer,
			         pvar->interval * 1000, RecurringTimerProc);
		}
		return 1;

	  case ID_MENU_DISABLE:
	  	if (pvar->enable) {
			pvar->enable = FALSE;
			KillTimer(pvar->cv->HWin, IdRecurringTimer);
		}
		return 1;
	}
	return 0;
}

static TTXExports Exports = {
	sizeof(TTXExports),
	ORDER,

	TTXInit,
	NULL, // TTXGetUIHooks,
	TTXGetSetupHooks,
	TTXOpenTCP,
	TTXCloseTCP,
	NULL, // TTXSetWinSize,
	TTXModifyMenu,
	TTXModifyPopupMenu,
	TTXProcessCommand,
	NULL, // TTXEnd,
	NULL, // TTXSetCommandLine,
	TTXOpenFile,
	TTXCloseFile
};

BOOL __declspec(dllexport) PASCAL TTXBind(WORD Version, TTXExports *exports) {
	int size = sizeof(Exports) - sizeof(exports->size);
	/* do version checking if necessary */
	/* if (Version!=TTVERSION) return FALSE; */

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char *)exports + sizeof(exports->size),
	       (char *)&Exports + sizeof(exports->size), size);
	return TRUE;
}

BOOL WINAPI DllMain(HANDLE hInstance,
		    ULONG ul_reason_for_call,
		    LPVOID lpReserved)
{
	switch( ul_reason_for_call ) {
	  case DLL_THREAD_ATTACH:
		/* do thread initialization */
		break;
	  case DLL_THREAD_DETACH:
		/* do thread cleanup */
		break;
	  case DLL_PROCESS_ATTACH:
		/* do process initialization */
		hInst = hInstance;
		pvar = &InstVar;
		break;
	  case DLL_PROCESS_DETACH:
		/* do process cleanup */
		break;
	}
	return TRUE;
}
