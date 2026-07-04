#include <winsock2.h>
#include <ws2tcpip.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inifile_com.h"
#include "ttcommdlg.h"
#include "codeconv.h"
#include "ttlib_types.h"

#include "gettimeofday.h"

#define ORDER 6001
#define ID_MENU_REPLAY 55302
#define ID_MENU_AGAIN  55303

#define BUFFSIZE 2000

#define INISECTION "ttyplay"
#define INISECTIONW L"ttyplay"

#define TITLE_MODE_TIME 1
#define TITLE_MODE_MSEC 2

static HANDLE hInst; /* Instance handle of TTX*.DLL */

enum ParseMode {
  MODE_FIRST,
  MODE_ESC,
  MODE_CSI,
  MODE_STRING,
  MODE_STR_ESC
};

struct recheader {
	struct timeval tv;
	int len;
};

typedef struct {
	PTTSet ts;
	PComVar cv;
	TCreateFile origPCreateFile;
	TReadFile origPReadFile;
	TWriteFile origPWriteFile;
	PParseParam origParseParam;
	PReadIniFile origReadIniFile;
	BOOL enable;
	BOOL active;
	BOOL ChangeTitle;
	BOOL ReplaceHostDlg;
	BOOL played;
	HMENU FileMenu;
	HMENU ControlMenu;
	int skip;
	int skip_ini;
	int maxwait;
	int minwait;
	int speed;
	BOOL pause;
	BOOL nowait;
	BOOL nowait_ini;
	BOOL open_error;
	struct timeval last;
	struct timeval wait;
	wchar_t *openfnW;
	char origTitle[TitleBuffSize];
	char origOLDTitle[TitleBuffSize];
	int mode_flag;
	char *fmt_time;
} TInstVar;

static TInstVar *pvar;
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
  wchar_t *uimsg1, *uimsg2, *uimsg3;
  char *msg1, *msg2, *msg3;

  GetI18nStrWW("TTXttyrec", "TITLE_STATUS_MSG", L"Speed: %d, Pause: %s", pvar->ts->UILanguageFileW, &uimsg1);
  GetI18nStrWW("TTXttyrec", "TITLE_STATUS_ON",  L"ON",                   pvar->ts->UILanguageFileW, &uimsg2);
  GetI18nStrWW("TTXttyrec", "TITLE_STATUS_OFF", L"OFF",                  pvar->ts->UILanguageFileW, &uimsg3);
  msg1 = ToCharW(uimsg1);
  msg2 = ToCharW(uimsg2);
  msg3 = ToCharW(uimsg3);
  free(uimsg1);
  free(uimsg2);
  free(uimsg3);
  _snprintf_s(tbuff, sizeof(tbuff), _TRUNCATE, msg1, pvar->speed, pvar->pause ? msg2: msg3);
  free(msg1);
  free(msg2);
  free(msg3);
  strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), tbuff, _TRUNCATE);
  pvar->ChangeTitle = TRUE;
  SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
}

void ConvertSafeStrFtimeFormat(wchar_t *fmt) {
	static const wchar_t *kws=L"aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYz%N";
	if (fmt != NULL) {
		while (*fmt != L'\0') {
			if (*fmt == L'%') {
				fmt++;
				if (*fmt == L'\0') {
					fmt--;
					*fmt=L'\0';
					break;
				}
				if (wcschr(kws, *fmt) == NULL) {
					*fmt = L'%';
				}
			}
			fmt++;
		}
	}
}

void ChangeTitleTime(struct timeval tv) {
	char tbuff[TitleBuffSize];
  	time_t tvs = tv.tv_sec;
  	struct tm tm_local;
	char fmt[40];
	char buff[8];
	char *p;

  	if (localtime_s(&tm_local, &tvs) != 0) {
		return;
  	}
	if (pvar->fmt_time == 0 || *pvar->fmt_time == '\0') {
		return;
	}

	strncpy_s(fmt, sizeof(fmt), pvar->fmt_time, _TRUNCATE);
	p = strstr(fmt, "%N");
	if (p != NULL) {
		*p = '\0';
		_snprintf_s(buff, sizeof(buff), _TRUNCATE, "%03d", tv.tv_usec/1000);
		strncat_s(fmt, sizeof(fmt), buff, _TRUNCATE);
		strncat_s(fmt, sizeof(fmt), pvar->fmt_time - fmt + p + 2, _TRUNCATE);
	}
	else if (pvar->mode_flag & TITLE_MODE_MSEC) {
		p = strstr(fmt, "%S");
		if (p != NULL) {
			p += 2;
			*p = '\0';
			_snprintf_s(buff, sizeof(buff), _TRUNCATE, ".%03d", tv.tv_usec/1000);
			strncat_s(fmt, sizeof(fmt), buff, _TRUNCATE);
			strncat_s(fmt, sizeof(fmt), pvar->fmt_time - fmt + p, _TRUNCATE);
		}
	}

	if (strftime(tbuff, sizeof(tbuff), fmt, &tm_local) == 0) {
		return;
	}

	strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), tbuff, _TRUNCATE);
	SendMessage(pvar->cv->HWin, WM_USER_CHANGETITLE, 0, 0);
}

void ChangeTitleTimePeriod(struct timeval tv, int period) {
	static time_t tvs_org;
	time_t tvs = (time_t)tv.tv_sec*1000+tv.tv_usec/1000;

	if (tvs == tvs_org) return;
	if (llabs(tvs - tvs_org) < ((period<1000) ? period : 1000)) return;
	tvs_org = tvs;

	ChangeTitleTime(tv);
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

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origPCreateFile = NULL;
	pvar->origPReadFile = NULL;
	pvar->origPWriteFile = NULL;
	pvar->enable = FALSE;
	pvar->active = FALSE;
	pvar->ChangeTitle = FALSE;
	pvar->ReplaceHostDlg = FALSE;
	pvar->played = FALSE;
	gettimeofday(&(pvar->last) /*, NULL*/ );
	pvar->wait.tv_sec = 0;
	pvar->wait.tv_usec = 1;
	pvar->openfnW = NULL;
	pvar->skip = 0;
	pvar->skip_ini = 5;
	pvar->maxwait = 0;
	pvar->minwait = 0;
	pvar->pause = FALSE;
	pvar->nowait = FALSE;
	pvar->nowait_ini = FALSE;
	pvar->open_error = FALSE;
	pvar->mode_flag = 0;
	pvar->fmt_time = NULL;
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

static HANDLE PASCAL TTXCreateFile(LPCSTR FName, DWORD AcMode, DWORD ShMode,
    LPSECURITY_ATTRIBUTES SecAttr, DWORD CreateDisposition, DWORD FileAttr, HANDLE Template) {

	HANDLE ret;

	if (AcMode == GENERIC_READ && ShMode == 0) {
		ShMode = FILE_SHARE_READ;
	}

	ret = pvar->origPCreateFile(FName, AcMode, ShMode, SecAttr, CreateDisposition, FileAttr, Template);

	if (pvar->enable) {
		if (ret == INVALID_HANDLE_VALUE) {
			pvar->enable = FALSE;
			pvar->open_error = TRUE;
			pvar->played = FALSE;
		}
		else {
			pvar->open_error = FALSE;
		}
	}

	return ret;
}

static BOOL PASCAL TTXReadFile(HANDLE fh, LPVOID obuff, DWORD oblen, LPDWORD rbytes, LPOVERLAPPED rol) {
	static struct recheader prh = { 0, 0, 0 };
	static struct recheader orh = { 0, 0, 0 };
	static int speed = 0;
	static DWORD lbytes;
	static char ibuff[BUFFSIZE];
	static BOOL title_changed = FALSE, first_title_changed = FALSE;

	int b[3];
	DWORD rsize;
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

	if (!pvar->active) {
		pvar->active = TRUE;
		prh.len = 0;
		prh.tv.tv_sec = 0;
		prh.tv.tv_usec = 0;
		title_changed = FALSE;
		first_title_changed = FALSE;
	}

	if (!first_title_changed) {
		if (!(pvar->mode_flag & TITLE_MODE_TIME)) {
			ChangeTitleStatus ();
		}
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
			static const TTMessageBoxInfoW info = {
				"TTXttyrec",
				"MSG_TITLE", L"TTXttyplay",
				"MSG_RSIZE", L"rsize != sizeof(b), Disabled.",
				MB_ICONEXCLAMATION
			};
			TTMessageBoxW(pvar->cv->HWin, &info, pvar->ts->UILanguageFileW);
			pvar->enable = FALSE;
			RestoreOLDTitle();
			return FALSE;
		}
		h.tv.tv_sec = b[0];
		h.tv.tv_usec = b[1];
		h.len = b[2];
		if (prh.tv.tv_sec != 0) {
			pvar->wait = tvshift(tvdiff(prh.tv, h.tv), pvar->speed);
			if (pvar->wait.tv_sec < pvar->minwait || pvar->skip > 0) {
				if (pvar->skip > 0) {
					pvar->skip --;
				}
				pvar->wait.tv_sec = 0;
				pvar->wait.tv_usec = 0;
			}
			else if (pvar->maxwait != 0 && pvar->wait.tv_sec >= pvar->maxwait) {
				char tbuff[TitleBuffSize];
				wchar_t *uimsg;
				char *msg;
				GetI18nStrWW("TTXttyrec", "TITLE_TRIM", L"%d.%06d secs idle. trim to %d secs.", pvar->ts->UILanguageFileW, &uimsg);
				msg = ToCharW(uimsg);
				free(uimsg);
				_snprintf_s(tbuff, sizeof(tbuff), _TRUNCATE, msg, pvar->wait.tv_sec, pvar->wait.tv_usec, pvar->maxwait);
				free(msg);
				pvar->wait.tv_sec = pvar->maxwait;
				pvar->wait.tv_usec = 0;
				title_changed = TRUE;
				ChangeTitle(tbuff);
			}
		}
		speed = pvar->speed;
		orh = prh;
		prh = h;
	}

	if (!pvar->nowait) {
		gettimeofday(&curtime /*, NULL*/ );
		tdiff = tvdiff(pvar->last, curtime);
		if (pvar->mode_flag & TITLE_MODE_TIME) {
			int period = (81<<(8+speed))/256;
			ChangeTitleTimePeriod(tvadd(orh.tv, tvshift(tdiff, -speed)), period);
		}
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
			if (!pvar->nowait) {
				pvar->last = curtime;
			}
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

static BOOL PASCAL TTXWriteFile(HANDLE fh, LPCVOID buff, DWORD len, LPDWORD wbytes, LPOVERLAPPED wol) {
	char tmpbuff[2048];
	unsigned int spos, dpos;
	char *ptr;
	enum ParseMode mode = MODE_FIRST;
	BOOL speed_changed = FALSE;

	ptr = (char *)buff;
	*wbytes = 0;

	for (spos = dpos = 0; spos < len; spos++, ptr++) {
		switch (mode) {
		case MODE_FIRST:
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
			  case 't':
			  case 'T':
				pvar->mode_flag ^= TITLE_MODE_TIME;
				speed_changed = TRUE;
				break;
			  case 'q':
			  case 'Q':
				pvar->nowait = !pvar->nowait;
				break;
			  case 'k':
			  case 'K':
				pvar->wait.tv_sec = 0;
				pvar->skip += pvar->skip_ini;
				break;
			  case ESC:
				mode = MODE_ESC;
				break;
			  default:
				if (dpos < sizeof(tmpbuff)) {
					tmpbuff[dpos++] = *ptr;
				}
			}
			break;
		case MODE_ESC:
			switch (*ptr) {
			case '[':
				mode = MODE_CSI;
				break;
			case 'P': // DCS
			case ']': // OSC
			case 'X': // SOS
			case '^': // PM
			case '_': // APC
				mode = MODE_STRING;
				break;
			default:
				mode = MODE_FIRST;
				break;
			}
			break;
		case MODE_CSI:
			if (*ptr < ' ' || *ptr > '?') {
				mode = MODE_FIRST;
			}
			break;
		case MODE_STRING:
			if (*ptr == ESC) {
				mode = MODE_STR_ESC;
			}
			else if (*ptr == BEL) {
				mode = MODE_FIRST;
			}
			break;
		case MODE_STR_ESC:
			if (*ptr == '\\') {
				mode = MODE_FIRST;
			}
			else if (*ptr != ESC) {
				mode = MODE_STRING;
			}
			break;
		}
	}

	if (speed_changed) {
		if (!(pvar->mode_flag & TITLE_MODE_TIME)) {
			ChangeTitleStatus ();
		}
	}
	if (dpos > 0) {
		pvar->origPWriteFile(fh, tmpbuff, dpos, wbytes, wol);
	}
	*wbytes = len;
	return TRUE;
}

static void PASCAL TTXOpenFile(TTXFileHooks *hooks) {
	if (pvar->cv->PortType == IdFile && pvar->enable) {
		pvar->origPCreateFile = *hooks->PCreateFile;
		pvar->origPReadFile = *hooks->PReadFile;
		pvar->origPWriteFile = *hooks->PWriteFile;
		*hooks->PCreateFile = TTXCreateFile;
		*hooks->PReadFile = TTXReadFile;
		*hooks->PWriteFile = TTXWriteFile;

		strncpy_s(pvar->origOLDTitle, sizeof(pvar->origOLDTitle), pvar->ts->Title, _TRUNCATE);
	}
}

static void PASCAL TTXCloseFile(TTXFileHooks *hooks) {
	if (pvar->origPCreateFile) {
		*hooks->PCreateFile = pvar->origPCreateFile;
	}
	if (pvar->origPReadFile) {
		*hooks->PReadFile = pvar->origPReadFile;
	}
	if (pvar->origPWriteFile) {
		*hooks->PWriteFile = pvar->origPWriteFile;
	}
	if (pvar->enable) {
		RestoreOLDTitle();
		pvar->enable = FALSE;
		pvar->active = FALSE;
		pvar->played = TRUE;
		pvar->wait.tv_sec = 0;
		pvar->wait.tv_usec = 0;
		pvar->nowait = pvar->nowait_ini;
		pvar->speed = 0;
	}
}

static void PASCAL TTXModifyMenu(HMENU menu) {
	UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

	pvar->FileMenu = GetFileMenu(menu);
	pvar->ControlMenu = GetControlMenu(menu);

	if (pvar->enable) {
		flag |= MF_GRAYED;
	}

	wchar_t *uimsg;
	GetI18nStrWW("TTXttyrec", "MENU_REPLAY", L"TTY R&eplay", pvar->ts->UILanguageFileW, &uimsg);
	InsertMenuW(pvar->FileMenu, ID_FILE_PRINT2, flag, ID_MENU_REPLAY, uimsg);
	free(uimsg);
	if (pvar->played) {
		GetI18nStrWW("TTXttyrec", "MENU_REPLAYAGAIN", L"Replay again", pvar->ts->UILanguageFileW, &uimsg);
		InsertMenuW(pvar->FileMenu, ID_FILE_PRINT2, flag, ID_MENU_AGAIN, uimsg);
		free(uimsg);
	}
	InsertMenu(pvar->FileMenu, ID_FILE_PRINT2, MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);

//	InsertMenu(menu, ID_HELPMENU, MF_ENABLED, ID_MENU_REPLAY, "&t");
}

static void PASCAL TTXModifyPopupMenu(HMENU menu) {
	if (menu==pvar->FileMenu) {
		if (pvar->enable) {
			EnableMenuItem(pvar->FileMenu, ID_MENU_REPLAY, MF_BYCOMMAND | MF_GRAYED);
		}
		else {
			EnableMenuItem(pvar->FileMenu, ID_MENU_REPLAY, MF_BYCOMMAND | MF_ENABLED);
		}
	}
}

static void PASCAL TTXParseParam(wchar_t *Param, PTTSet ts, PCHAR DDETopic) {
	wchar_t buff[1024];
	wchar_t *next;
	pvar->origParseParam(Param, ts, DDETopic);
	int max, min;

	next = Param;
	while (next = GetParam(buff, sizeof(buff), next)) {
		if (_wcsnicmp(buff, L"/ttyplay-nowait", 16) == 0 || _wcsnicmp(buff, L"/tpnw", 6) == 0) {
			pvar->nowait = TRUE;
			pvar->nowait_ini = pvar->nowait;
		}
		else if (_wcsnicmp(buff, L"/TTYPLAY", 9) == 0 || _wcsnicmp(buff, L"/TP", 4) == 0 ||
			_wcsnicmp(buff, L"/TP=",4) == 0) {
			pvar->enable = TRUE;
			if (ts->PortType == IdFile && strlen(ts->HostName) > 0) {
				wchar_t *HostNameW = ToWcharA(ts->HostName);
				free(pvar->openfnW);
				pvar->openfnW = HostNameW;
			}
			if (buff[3] == '=') {
				pvar->mode_flag = _wtol(&buff[4]);
			}
		}
		else if (_wcsnicmp(buff, L"/TPW=", 5) == 0) {
			max = 0;
			min = 0;
	    	if (swscanf_s(&buff[5], L"%d,%d", &max, &min) == 0) {
	    		swscanf_s(&buff[5], L",%d", &min);
			}
			pvar->maxwait = max;
			pvar->minwait = min;
		}
	}
}

static void PASCAL TTXReadIniFile(const wchar_t *fn, PTTSet ts) {
	wchar_t buff[40];

	(pvar->origReadIniFile)(fn, ts);
//	ts->TitleFormat = 0;
	pvar->maxwait = GetPrivateProfileIntAFileW(INISECTION, "MaxWait", 0, fn);
	pvar->speed = GetPrivateProfileIntAFileW(INISECTION, "Speed", 0, fn);
	GetPrivateProfileStringW(INISECTIONW, L"TimeFormat", L"%Y/%m/%d %H:%M:%S", buff, _countof(buff), fn);
	ConvertSafeStrFtimeFormat(buff);
	free(pvar->fmt_time);
	pvar->fmt_time = ToCharW(buff);
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
	pvar->origParseParam = *hooks->ParseParam;
	*hooks->ParseParam = TTXParseParam;
	pvar->origReadIniFile = *hooks->ReadIniFile;
	*hooks->ReadIniFile = TTXReadIniFile;
}

static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
	switch (cmd) {
	case ID_MENU_REPLAY:
		if (!pvar->enable) {
			TTOPENFILENAMEW ofn;
			wchar_t *openfn;
			wchar_t *uimsg1, *uimsg2;
			wchar_t *logdir;

			if (pvar->openfnW == NULL) {
				logdir = GetTermLogDir(pvar->ts);
			}
			else {
				logdir = ExtractDirNameW(pvar->openfnW);
			}

			GetI18nStrWW("TTXttyrec", "FILE_FILTER", L"ttyrec(*.tty)\\0*.tty\\0All files(*.*)\\0*.*\\0\\0", pvar->ts->UILanguageFileW, &uimsg1);
			GetI18nStrWW("TTXttyrec", "FILE_DEFAULTEXTENSION", L"tty", pvar->ts->UILanguageFileW, &uimsg2);
			memset(&ofn, 0, sizeof(ofn));
			ofn.hwndOwner = hWin;
			ofn.lpstrFilter = uimsg1;
			ofn.lpstrDefExt = uimsg2;
			ofn.lpstrInitialDir = logdir;
			// ofn.lpstrTitle = L"";
			ofn.Flags = OFN_FILEMUSTEXIST;

			if (TTGetOpenFileNameW(&ofn, &openfn)) {
				free(pvar->openfnW);
				pvar->openfnW = openfn;
				pvar->ReplaceHostDlg = TRUE;
				pvar->nowait = FALSE;
				// Call New-Connection dialog
				SendMessage(hWin, WM_COMMAND, MAKELONG(ID_FILE_NEWCONNECTION, 0), 0);
			}
			free(uimsg1);
			free(uimsg2);
			free(logdir);
		}
		return 1;
	case ID_MENU_AGAIN:
		if (pvar->played) {
			// Clear Buffer
			SendMessage(hWin, WM_COMMAND, MAKELONG(ID_EDIT_CLEARBUFFER, 0), 0);
			pvar->played = FALSE;
			pvar->ReplaceHostDlg = TRUE;
			pvar->nowait = FALSE;
			// Call New-Connection dialog
			SendMessage(hWin, WM_COMMAND, MAKELONG(ID_FILE_NEWCONNECTION, 0), 0);
		}
		return 1;
	}
	return 0;
}

static BOOL PASCAL TTXSetupWin(HWND parent, PTTSet ts) {
	return (TRUE);
}

static BOOL PASCAL TTXGetHostName(HWND parent, PGetHNRec GetHNRec) {
	GetHNRec->PortType = IdTCPIP;
	_snwprintf_s(GetHNRec->HostName, HostNameMaxLength, _TRUNCATE, L"/R=\"%s\" /TP", pvar->openfnW);
	return (TRUE);
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks) {
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

BOOL __declspec(dllexport) PASCAL TTXBind(WORD Version, TTXExports *exports) {
	int size = sizeof(Exports) - sizeof(exports->size);

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char *)exports + sizeof(exports->size),
		(char *)&Exports + sizeof(exports->size),
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
			hInst = hInstance;
			pvar = &InstVar;
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			free(pvar->openfnW);
			pvar->openfnW = NULL;
			free(pvar->fmt_time);
			pvar->fmt_time = NULL;
			break;
	}
	return TRUE;
}
