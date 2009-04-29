#include <ctype.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "ttlib.h"
#include "tt_res.h"
#include "resource.h"

#include "compat_w95.h"

#define ORDER 4000

#define MINIMUM_INTERVAL 10
#define DEFAULT_INTERVAL 300

#define IdRecurringTimer 3001

#define ID_MENU_SETUP 55500

#define SECTION "TTXRecurringCommand"

static HANDLE hInst;

typedef struct {
	PTTSet ts;
	PComVar cv;
	Tsend origPsend;
	TWriteFile origPWriteFile;
	PReadIniFile origReadIniFile;
	PWriteIniFile origWriteIniFile;
	HMENU SetupMenu;
	int interval;
	BOOL enable;
	int cmdLen;
	unsigned char command[50];
	unsigned char orgCommand[50];
} TInstVar;

typedef TInstVar FAR * PTInstVar;
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

WORD GetOnOff(PCHAR Sect, PCHAR Key, PCHAR FName, BOOL Default)
{
	char Temp[4];
	GetPrivateProfileString(Sect, Key, "", Temp, sizeof(Temp), FName);
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
// \n, \t等を展開する。
// common/ttlib.c:RestoreNewLine()がベース。
//
void UnEscapeStr(BYTE *Text)
{
	unsigned int i, j=0, k;
	size_t size;
	unsigned char *buf;

	size = strlen(Text);
	buf = malloc(size+1);

	memset(buf, 0, size+1);
	for (i=0; i<size; i++) {
		if (Text[i] == '\\') {
			switch (Text[i+1]) {
				case '\\':
					buf[j] = '\\';
					i++;
					break;
				case 'n':
					buf[j] = '\n';
					i++;
					break;
				case 't':
					buf[j] = '\t';
					i++;
					break;
				case 'a':
					buf[j] = '\a';
					i++;
					break;
				case 'b':
					buf[j] = '\b';
					i++;
					break;
				case 'f':
					buf[j] = '\f';
					i++;
					break;
				case 'r':
					buf[j] = '\r';
					i++;
					break;
				case 'v':
					buf[j] = '\v';
					i++;
					break;
				case 'e':
					buf[j] = '\033';
					i++;
					break;
				case 'x':
					if (isxdigit(Text[i+2]) && isxdigit(Text[i+3])) {
						i+=2;
						// buf[j] = ((Text[i]|0x20) - (isalpha(Text[i])?'a'-10:'0')) << 4;
						if (isalpha(Text[i]))
							buf[j] = (Text[i]|0x20) - 'a' + 10;
						else
							buf[j] = Text[i] - '0';
						buf[j] <<= 4;

						i++;
						// buf[j] += ((Text[i]|0x20) - (isalpha(Text[i])?'a'-10:'0'));
						if (isalpha(Text[i]))
							buf[j] += (Text[i]|0x20) - 'a' + 10;
						else
							buf[j] += Text[i] - '0';
					}
					else {
						buf[j++] = '\\';
						buf[j] = 'x';
						i++;
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
					buf[j] = 0;
					for (k=0; k<3; k++, i++) {
						if (Text[i+1] < '0' || Text[i+1] > '7')
							break;
						buf[j] = (buf[j] << 3) + Text[i+1] - '0';
					}
					break;
				default:
					buf[j] = '\\';
			}
		}
		else {
			buf[j] = Text[i];
		}
		j++;
	}
	/* use memcpy to copy with '\0' */
	memcpy(Text, buf, size);

	free(buf);
}

//
//  出力バッファに文字列を書き込む
//  ttpcmn/ttcmn.c:CommTextOut()がベース
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
// タイマがタイムアウトした時のコールバック関数
//	有効にされているならば、出力バッファにコマンドを書き込む
//
void CALLBACK RecurringTimerProc(HWND hwnd, UINT msg, UINT_PTR ev, DWORD now) {
	if (pvar->enable && pvar->cmdLen > 0 && pvar->cv->Ready && pvar->cv->OutBuffCount == 0) {
		CommOut(pvar->command, pvar->cmdLen);
	}
//	SendMessage(hwnd, WM_IDLE, 0, 0);
	return;
}

//
//  TTXInit -- 起動時処理
//
static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origPsend = NULL;
	pvar->origPWriteFile = NULL;
	pvar->origReadIniFile = NULL;
	pvar->origWriteIniFile = NULL;
	pvar->interval = DEFAULT_INTERVAL;
}

//
//  TTXSend, TTXWriteFile -- キー入力処理
//	キー入力があったら、タイマを延長する
//
static int PASCAL FAR TTXsend(SOCKET s, const char FAR *buf, int len, int flags) {
	if (pvar->enable && len > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
	return pvar->origPsend(s, buf, len, flags);
}

static BOOL PASCAL FAR TTXWriteFile(HANDLE fh, LPCVOID buff, DWORD len, LPDWORD wbytes, LPOVERLAPPED wol) {
	if (pvar->enable && len > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
	return pvar->origPWriteFile(fh, buff, len, wbytes, wol);
}

//
// TTXOpenTCP, TTXOpenFile -- セッション開始処理
//	Psend, WriteFileをフックし、有効ならばタイマをセットする。
//
static void PASCAL FAR TTXOpenTCP(TTXSockHooks FAR * hooks) {
	pvar->origPsend = *hooks->Psend;
	*hooks->Psend = TTXsend;

	if (pvar->enable && pvar->cmdLen > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
}

static void PASCAL FAR TTXOpenFile(TTXFileHooks FAR * hooks) {
	pvar->origPWriteFile = *hooks->PWriteFile;
	*hooks->PWriteFile = TTXWriteFile;

	if (pvar->enable && pvar->cmdLen > 0) {
		SetTimer(pvar->cv->HWin, IdRecurringTimer, pvar->interval * 1000, RecurringTimerProc);
	}
}

//
// TTXCloseTCP, TTXCloseFile -- セッション終了時処理
//	Psend, WriteFileのフックを解除し、タイマを止める。
//
static void PASCAL FAR TTXCloseTCP(TTXSockHooks FAR * hooks) {
	if (pvar->origPsend) {
		*hooks->Psend = pvar->origPsend;
	}
	KillTimer(pvar->cv->HWin, IdRecurringTimer);
}

static void PASCAL FAR TTXCloseFile(TTXFileHooks FAR * hooks) {
	if (pvar->origPWriteFile) {
		*hooks->PWriteFile = pvar->origPWriteFile;
	}
	KillTimer(pvar->cv->HWin, IdRecurringTimer);
}

//
// TTXReadIniFile, TTXWriteIniFile -- 設定ファイルの読み書き
//
static void PASCAL FAR TTXReadIniFile(PCHAR fn, PTTSet ts) {
	pvar->origReadIniFile(fn, ts);

	GetPrivateProfileString(SECTION, "Command", "", pvar->orgCommand, sizeof(pvar->orgCommand), fn);
	strncpy_s(pvar->command, sizeof(pvar->command), pvar->orgCommand, _TRUNCATE);
	UnEscapeStr(pvar->command);
	pvar->cmdLen = (int)strlen(pvar->command);

	pvar->interval = GetPrivateProfileInt(SECTION, "Interval", DEFAULT_INTERVAL, fn);
	if (pvar->interval < MINIMUM_INTERVAL) {
		pvar->interval = MINIMUM_INTERVAL;
	}

	pvar->enable = GetOnOff(SECTION, "Enable", fn, FALSE);

	return;
}

static void PASCAL FAR TTXWriteIniFile(PCHAR fn, PTTSet ts) {
	char buff[20];

	pvar->origWriteIniFile(fn, ts);

	WritePrivateProfileString(SECTION, "Enable", pvar->enable?"on":"off", fn);

	WritePrivateProfileString(SECTION, "Command", pvar->orgCommand, fn);

	_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%d", pvar->interval);
	WritePrivateProfileString(SECTION, "Interval", buff, fn);

	return;
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR * hooks) {
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXReadIniFile;

	pvar->origWriteIniFile = *hooks->WriteIniFile;
	*hooks->WriteIniFile = TTXWriteIniFile;
}

//
// メニュー処理
//	コントロールメニューにRecurringCommandを追加。
//
static void PASCAL FAR TTXModifyMenu(HMENU menu) {
	UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

	pvar->SetupMenu = GetSetupMenu(menu);

	InsertMenu(pvar->SetupMenu, ID_SETUP_ADDITIONALSETTINGS, flag, ID_MENU_SETUP, "Rec&urring command");
}

//
// RecurringCommand設定ダイアログのコールバック関数。
//
static LRESULT CALLBACK RecurringCommandSetting(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	  case WM_INITDIALOG:
		SendMessage(GetDlgItem(dlg, IDC_ENABLE), BM_SETCHECK,
		            pvar->enable?BST_CHECKED:BST_UNCHECKED, 0);
		SetDlgItemInt(dlg, IDC_INTERVAL, pvar->interval, FALSE);
		SetDlgItemText(dlg, IDC_COMMAND, pvar->orgCommand);

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
// メニューコマンド処理
//
static int PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd) {
	switch (cmd) {
	  case ID_MENU_SETUP:
		switch (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SETUP_RECURRINGCOMMAND),
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
	NULL, // TTXModifyPopupMenu,
	TTXProcessCommand,
	NULL, // TTXEnd,
	NULL, // TTXSetCommandLine,
	TTXOpenFile,
	TTXCloseFile
};

BOOL __declspec(dllexport) PASCAL FAR TTXBind(WORD Version, TTXExports FAR * exports) {
	int size = sizeof(Exports) - sizeof(exports->size);
	/* do version checking if necessary */
	/* if (Version!=TTVERSION) return FALSE; */

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char FAR *)exports + sizeof(exports->size),
	       (char FAR *)&Exports + sizeof(exports->size), size);
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
		DoCover_IsDebuggerPresent();
		hInst = hInstance;
		pvar = &InstVar;
		break;
	  case DLL_PROCESS_DETACH:
		/* do process cleanup */
		break;
	}
	return TRUE;
}
