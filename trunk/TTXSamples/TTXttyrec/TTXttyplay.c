#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#ifndef NO_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif /* NO_INET6 */

#include "gettimeofday.h"

#include "compat_w95.h"

#define ORDER 6001
#define ID_MENU_REPLAY 55302

#define BUFFSIZE 2000

#define INISECTION "ttyplay"

static HANDLE hInst; /* Instance handle of TTX*.DLL */

struct recheader {
	struct timeval tv;
	int len;
};

typedef struct {
	PTTSet ts;
	PComVar cv;
	TReadFile origPReadFile;
	TWriteFile origPWriteFile;
	PParseParam origParseParam;
	PReadIniFile origReadIniFile;
	BOOL enable;
	BOOL ChangeTitle;
	BOOL ReplaceHostDlg;
	HMENU FileMenu;
	HMENU ControlMenu;
	int maxwait;
	int speed;
	BOOL pause;
	BOOL nowait;
	struct timeval last;
	struct timeval wait;
	char openfn[MAX_PATH];
	char origTitle[TitleBuffSize];
	char origOLDTitle[TitleBuffSize];
} TInstVar;

static TInstVar FAR * pvar;
static TInstVar InstVar;

#define GetFileMenu(menu)	GetSubMenuByChildID(menu, ID_FILE_NEWCONNECTION)
#define GetEditMenu(menu)	GetSubMenuByChildID(menu, ID_EDIT_COPY2)
#define GetSetupMenu(menu)	GetSubMenuByChildID(menu, ID_SETUP_TERMINAL)
#define GetControlMenu(menu)	GetSubMenuByChildID(menu, ID_CONTROL_RESETTERMINAL)
#define GetHelpMenu(menu)	GetSubMenuByChildID(menu, ID_HELP_ABOUT)

void RestoreOLDTitle() {
	strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), pvar->origOLDTitle, _TRUNCATE);
	pvar->ChangeTitle = TRUE;
	SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
}

void ChangeTitleStatus() {
  char tbuff[TitleBuffSize];
  
  _snprintf_s(tbuff, sizeof(tbuff), _TRUNCATE, "Speed: %d, Pause: %s", pvar->speed, pvar->pause ? "ON": "OFF");
  strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), tbuff, _TRUNCATE);
  pvar->ChangeTitle = TRUE;
  SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
}

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

static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origPReadFile = NULL;
	pvar->origPWriteFile = NULL;
	pvar->enable = FALSE;
	pvar->ChangeTitle = FALSE;
	pvar->ReplaceHostDlg = FALSE;
	gettimeofday(&(pvar->last), NULL);
	pvar->wait.tv_sec = 0;
	pvar->wait.tv_usec = 1;
	pvar->pause = FALSE;
	pvar->nowait = FALSE;
}

void RestoreTitle() {
	ChangeTitleStatus ();
/*
	strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), pvar->origTitle, _TRUNCATE);
	pvar->ChangeTitle = TRUE;
	SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
*/
}

void ChangeTitle(char *title) {
	strncpy_s(pvar->origTitle, sizeof(pvar->origTitle), pvar->ts->Title, _TRUNCATE);
	strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), title, _TRUNCATE);
	pvar->ChangeTitle = TRUE;
	SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
}

static BOOL PASCAL FAR TTXReadFile(HANDLE fh, LPVOID obuff, DWORD oblen, LPDWORD rbytes, LPOVERLAPPED rol) {
	static struct recheader prh = { 0, 0, 0 };
	static unsigned int lbytes;
	static char ibuff[BUFFSIZE];
	static BOOL title_changed = FALSE, first_title_changed = FALSE;

	int b[3], rsize;
	struct recheader h;
	struct timeval curtime;
	struct timeval tdiff;

	*rbytes = 0;

	if (pvar->pause) {
		SetLastError(ERROR_IO_PENDING);
		return FALSE;
	}

	if (!pvar->enable) {
		return pvar->origPReadFile(fh, obuff, oblen, rbytes, rol);
	}

	if (!first_title_changed) {
		ChangeTitleStatus ();
		first_title_changed = TRUE;
	}

	if (prh.len == 0 && lbytes == 0) {
		if (!pvar->origPReadFile(fh, b, sizeof(b), &rsize, rol)) {
			return FALSE;
		}
		if (rsize == 0) {
			// EOF reached
			return TRUE;
		}
		else if (rsize != sizeof(b)) {
			MessageBox(pvar->cv->HWin, "rsize != sizeof(b), Disabled.", "TTXttyplay", MB_ICONEXCLAMATION);
			pvar->enable = FALSE;
			RestoreOLDTitle();
			return FALSE;
		}
		h.tv.tv_sec = b[0];
		h.tv.tv_usec = b[1];
		h.len = b[2];
		if (prh.tv.tv_sec != 0) {
			pvar->wait = tvshift(tvdiff(prh.tv, h.tv), pvar->speed);
			if (pvar->maxwait != 0 && pvar->wait.tv_sec >= pvar->maxwait) {
				char tbuff[TitleBuffSize];
				_snprintf_s(tbuff, sizeof(tbuff), _TRUNCATE, "%d.%06d secs idle. trim to %d secs.",
				pvar->wait.tv_sec, pvar->wait.tv_usec, pvar->maxwait);
				pvar->wait.tv_sec = pvar->maxwait;
				pvar->wait.tv_usec = 0;
				title_changed = TRUE;
				ChangeTitle(tbuff);
			}
		}
		prh = h;
	}

	if (!pvar->nowait) {
		gettimeofday(&curtime, NULL);
		tdiff = tvdiff(pvar->last, curtime);
	}

	if (pvar->nowait || tdiff.tv_sec > pvar->wait.tv_sec ||
	  (tdiff.tv_sec == pvar->wait.tv_sec && tdiff.tv_usec >= pvar->wait.tv_usec)) {
		if (title_changed) {
			RestoreTitle();
			title_changed = FALSE;
		}
		if (pvar->wait.tv_sec != 0 || pvar->wait.tv_usec != 0) {
			pvar->wait.tv_sec = 0;
			pvar->wait.tv_usec = 0;
			pvar->last = curtime;
		}

		if (prh.len != 0 && lbytes == 0) {
			if (!pvar->origPReadFile(fh, ibuff, (prh.len<BUFFSIZE) ? prh.len : BUFFSIZE, &lbytes, rol)) {
				return FALSE;
			}
			else if (lbytes == 0) {
				// EOF reached
				return TRUE;
			}
			prh.len -= lbytes;
		}

		if (lbytes > 0) {
			if (lbytes < oblen) {
				memcpy(obuff, ibuff, lbytes);
				*rbytes = lbytes;
				lbytes = 0;
			}
			else {
				memcpy(obuff, ibuff, oblen);
				lbytes -= oblen;
				memcpy(ibuff, ibuff+oblen, lbytes);
				*rbytes = oblen;
			}
			return TRUE;
		}
	}

	SetLastError(ERROR_IO_PENDING);
	return FALSE;
}

static BOOL PASCAL FAR TTXWriteFile(HANDLE fh, LPCVOID buff, DWORD len, LPDWORD wbytes, LPOVERLAPPED wol) {
	char tmpbuff[2048];
	unsigned int spos, dpos;
	char *ptr;
	BOOL speed_changed = FALSE;

	ptr = (char *)buff;
	*wbytes = 0;

	for (spos = dpos = 0; spos < len; spos++, ptr++) {
		switch (*ptr) {
		  case '1':
			pvar->speed = 0;
			speed_changed = TRUE;
			break;
		  case 'f':
		  case 'F':
		  case '+':
			if (pvar->speed < 8) {
				pvar->speed++;
				speed_changed = TRUE;
			}
			break;
		  case 's':
		  case 'S':
		  case '-':
			if (pvar->speed > -8) {
				pvar->speed--;
				speed_changed = TRUE;
			}
			break;
		  case 'p':
		  case 'P':
			pvar->pause = !(pvar->pause);
			speed_changed = TRUE;
			break;
		  case ' ':
		  case '.':
			pvar->wait.tv_sec = 0;
			break;
		  default:
			if (dpos < sizeof(tmpbuff)) {
				tmpbuff[dpos++] = *ptr;
			}
		}
	}

	if (speed_changed) {
		ChangeTitleStatus ();
	}
	if (dpos > 0) {
		pvar->origPWriteFile(fh, tmpbuff, dpos, wbytes, wol);
	}
	*wbytes = len;
	return TRUE;
}

static void PASCAL FAR TTXOpenFile(TTXFileHooks FAR * hooks) {
	if (pvar->cv->PortType == IdFile && pvar->enable) {
		pvar->origPReadFile = *hooks->PReadFile;
		pvar->origPWriteFile = *hooks->PWriteFile;
		*hooks->PReadFile = TTXReadFile;
		*hooks->PWriteFile = TTXWriteFile;

		strncpy_s(pvar->origOLDTitle, sizeof(pvar->origOLDTitle), pvar->ts->Title, _TRUNCATE);
	}
}

static void PASCAL FAR TTXCloseFile(TTXFileHooks FAR * hooks) {
	if (pvar->origPReadFile) {
		*hooks->PReadFile = pvar->origPReadFile;
	}
	if (pvar->origPWriteFile) {
		*hooks->PWriteFile = pvar->origPWriteFile;
	}
	if (pvar->enable) {
		RestoreOLDTitle();
		pvar->enable = FALSE;
	}
}

static void PASCAL FAR TTXModifyMenu(HMENU menu) {
	UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

	pvar->FileMenu = GetFileMenu(menu);
	pvar->ControlMenu = GetControlMenu(menu);

	if (pvar->enable) {
		flag |= MF_GRAYED;
	}
	InsertMenu(pvar->FileMenu, ID_FILE_PRINT2, flag, ID_MENU_REPLAY, "TTY R&eplay");
	InsertMenu(pvar->FileMenu, ID_FILE_PRINT2, MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);

//	InsertMenu(menu, ID_HELPMENU, MF_ENABLED, ID_MENU_REPLAY, "&t");
}

static void PASCAL FAR TTXModifyPopupMenu(HMENU menu) {
	if (menu==pvar->FileMenu) {
		if (pvar->enable) {
			EnableMenuItem(pvar->FileMenu, ID_MENU_REPLAY, MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(pvar->FileMenu, ID_MENU_REPLAY, MF_BYCOMMAND | MF_ENABLED);
		}
	}
}

PCHAR GetParam(PCHAR buff, int size, PCHAR param) {
	int i = 0;
	BOOL quoted = FALSE;

	while (*param == ' ' || *param == '\t') {
		param++;
	}

	if (*param == '\0' || *param == ';') {
		return NULL;
	}

	while (*param != '\0' && (quoted || (*param != ';' && *param != ' ' && *param != '\t'))) {
		if (*param == '"' && (*++param != '"' || !quoted)) {
			quoted = !quoted;
			continue;
		}
		else if (i < size - 1) {
			buff[i++] = *param;
		}
		param++;
	}

	buff[i] = '\0';
	return (param);
}

static void PASCAL FAR TTXParseParam(PCHAR Param, PTTSet ts, PCHAR DDETopic) {
	char buff[1024];
	PCHAR next;
	pvar->origParseParam(Param, ts, DDETopic);

	next = Param;
	while (next = GetParam(buff, sizeof(buff), next)) {
		if (_strnicmp(buff, "/ttyplay-nowait", 16) == 0 || _strnicmp(buff, "/tpnw", 6) == 0) {
			pvar->nowait = TRUE;
		}
		else if (_strnicmp(buff, "/TTYPLAY", 9) == 0 || _strnicmp(buff, "/TP", 4) == 0) {
			pvar->enable = TRUE;
		}
	}
}

static void PASCAL FAR TTXReadIniFile(PCHAR fn, PTTSet ts) {
	(pvar->origReadIniFile)(fn, ts);
//	ts->TitleFormat = 0;
	pvar->maxwait = GetPrivateProfileInt(INISECTION, "MaxWait", 0, fn);
	pvar->speed = GetPrivateProfileInt(INISECTION, "Speed", 0, fn);
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR * hooks) {
	pvar->origParseParam = *hooks->ParseParam;
	*hooks->ParseParam = TTXParseParam;
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXReadIniFile;
}

static int PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd) {
	OPENFILENAME ofn;

	switch (cmd) {
	case ID_MENU_REPLAY:
		if (!pvar->enable) {
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWin;
			ofn.lpstrFilter = "ttyrec(*.tty)\0*.tty\0All files(*.*)\0*.*\0\0";
			ofn.lpstrFile = pvar->openfn;
			ofn.nMaxFile = sizeof(pvar->openfn);
			ofn.lpstrDefExt = "tty";
			// ofn.lpstrTitle = "";
			ofn.Flags = OFN_FILEMUSTEXIST;
			
			if (GetOpenFileName(&ofn)) {
				pvar->ReplaceHostDlg = TRUE;
				// Call New-Connection dialog
				SendMessage(hWin, WM_COMMAND, MAKELONG(ID_FILE_NEWCONNECTION, 0), 0);
			}
		}
		return 1;
	}
	return 0;
}

static BOOL FAR PASCAL TTXSetupWin(HWND parent, PTTSet ts) {
	return (TRUE);
}

static BOOL FAR PASCAL TTXGetHostName(HWND parent, PGetHNRec GetHNRec) {
	GetHNRec->PortType = IdTCPIP;
	_snprintf_s(GetHNRec->HostName, MAXPATHLEN, _TRUNCATE, "/R=\"%s\" /TP", pvar->openfn);
	return (TRUE);
}

static void PASCAL FAR TTXGetUIHooks(TTXUIHooks FAR * hooks) {
	if (pvar->ChangeTitle) {
		pvar->ChangeTitle = FALSE;
		*hooks->SetupWin = TTXSetupWin;
	}
	if (pvar->ReplaceHostDlg) {
		pvar->ReplaceHostDlg = FALSE;
		*hooks->GetHostName = TTXGetHostName;
	}
	return;
}

static TTXExports Exports = {
	sizeof(TTXExports),
	ORDER,

	TTXInit,
	TTXGetUIHooks,
	TTXGetSetupHooks,
	NULL, // TTXOpenTCP,
	NULL, // TTXCloseTCP,
	NULL, // TTXSetWinSize,
	TTXModifyMenu,
	TTXModifyPopupMenu,
	TTXProcessCommand,
	NULL, // TTXEnd,
	NULL, // TTXSetCommandLine,
	TTXOpenFile,
	TTXCloseFile
};

BOOL __declspec(dllexport) PASCAL FAR TTXBind(WORD Version, TTXExports FAR * exports) {
	int size = sizeof(Exports) - sizeof(exports->size);

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char FAR *)exports + sizeof(exports->size),
		(char FAR *)&Exports + sizeof(exports->size),
		size);
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
