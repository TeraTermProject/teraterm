#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"

#ifndef NO_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif /* NO_INET6 */

#include "compat_w95.h"

#define ORDER 4800
// #define ID_MENUITEM 37000
#define INISECTION "AdditionalTitle"

#define IdModeFirst   0
#define IdModeESC     1
#define IdModeOSC     2
#define IdModeESC2    3
#define IdModeProc    4

#define ADD_NONE   0
#define ADD_TOP    1
#define ADD_BOTTOM 2

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  Trecv origPrecv;
  TReadFile origPReadFile;
  PReadIniFile origReadIniFile;
  PWriteIniFile origWriteIniFile;
  PSetupWin origSetupWindowDlg;
  PParseParam origParseParam;
  WORD add_mode;
  BOOL ChangeTitle;
  char add_title[TitleBuffSize];
  char orig_title[TitleBuffSize];
} TInstVar;

static TInstVar FAR * pvar;
static TInstVar InstVar;

static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->origPrecv = NULL;
  pvar->origPReadFile = NULL;
  pvar->origReadIniFile = NULL;
  pvar->origWriteIniFile = NULL;
  pvar->ChangeTitle = FALSE;
  pvar->add_mode = ADD_NONE;
  pvar->add_title[0] = 0;
  pvar->orig_title[0] = 0;
}

void SetTitleStr(char *str, BOOL update) {
  switch (pvar->add_mode) {
    case ADD_TOP:
      _snprintf_s(pvar->ts->Title, sizeof(pvar->ts->Title), _TRUNCATE, "%s%s", pvar->add_title, str);
      break;
    case ADD_BOTTOM:
      _snprintf_s(pvar->ts->Title, sizeof(pvar->ts->Title), _TRUNCATE, "%s%s", str, pvar->add_title);
      break;
    default:
      return; // nothing to do
  }

  if (update) {
    pvar->ChangeTitle = TRUE;
    SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
  }
}

void ParseInputStr(char *rstr, int rcount) {
  static WORD mode = IdModeFirst;
  static char buff[InBuffSize];
  static unsigned int blen;
  char *p;
  int i;
  unsigned int func;

#define AcceptC1Control \
  ((pvar->cv->KanjiCodeEcho == IdEUC || pvar->cv->KanjiCodeEcho == IdJIS) && \
    pvar->ts->TerminalID >= IdVT220J && (pvar->ts->TermFlag & TF_ACCEPT8BITCTRL) != 0)

  for (i=0; i<rcount; i++) {
    switch (mode) {
      case IdModeFirst:
        if (rstr[i] == ESC) {
          mode = IdModeESC;
        }
        else if (rstr[i] == OSC && AcceptC1Control) {
          mode = IdModeOSC;
        }
        break;
      case IdModeESC:
        if (rstr[i] == ']') {
          mode = IdModeOSC;
        }
        else {
          mode = IdModeFirst;
        }
        break;
      case IdModeOSC:
        if (rstr[i] == ESC) {
          mode = IdModeESC2;
        }
        else if (rstr[i] == '\a' || rstr[i] == ST && AcceptC1Control) {
          mode = IdModeProc;
          i--;
        }
        else if (blen < InBuffSize - 1) {
          buff[blen++] = rstr[i];
        }
        if (blen >= InBuffSize - 1) {
          mode = IdModeProc;
          i--;
        }
        break;
      case IdModeESC2:
        if (rstr[i] == '\\') {
          mode = IdModeProc;
          i--;
        }
        else {
          if (blen < InBuffSize - 1) {
            buff[blen++] = ESC;
            buff[blen++] = rstr[i];
          }
          if (blen >= InBuffSize - 1) {
            mode = IdModeProc;
            i--;
          }
          else {
            mode = IdModeOSC;
          }
        }
        break;
      case IdModeProc:
        i++;
        buff[(blen<InBuffSize)?blen:InBuffSize-1] = '\0';
	p = buff;
	
	for (p=buff, func=0; isdigit(*p); p++) {
	  func = func * 10 + *p - '0';
	}
	if (*p != ';' || p == buff) {
	  blen = 0;
	  mode = IdModeFirst;
	  break;
	}
	p++;
	switch (func) {
	  case 0:
	  case 1:
	  case 2:
            strncpy_s(pvar->orig_title, sizeof(pvar->orig_title), p, _TRUNCATE);
	    SetTitleStr(p, TRUE);
	    break;
	  default:
	    ; /* nothing to do */
	}
	blen = 0;
	mode = IdModeFirst;
      default:
        ; /* not reached */
    }
  }
}

int PASCAL FAR TTXrecv(SOCKET s, char FAR *buff, int len, int flags) {
  int rlen;

  if ((rlen = pvar->origPrecv(s, buff, len, flags)) > 0) {
    ParseInputStr(buff, rlen);
  }
  return rlen;
}

BOOL PASCAL FAR TTXReadFile(HANDLE fh, LPVOID buff, DWORD len, LPDWORD rbytes, LPOVERLAPPED rol) {
  BOOL result;

  if ((result = pvar->origPReadFile(fh, buff, len, rbytes, rol)) != FALSE)
    ParseInputStr(buff, *rbytes);
  return result;
}

static void PASCAL FAR TTXOpenTCP(TTXSockHooks FAR * hooks) {
  pvar->origPrecv = *hooks->Precv;
  *hooks->Precv = TTXrecv;
}

static void PASCAL FAR TTXCloseTCP(TTXSockHooks FAR * hooks) {
  if (pvar->origPrecv) {
    *hooks->Precv = pvar->origPrecv;
  }
}

static void PASCAL FAR TTXOpenFile(TTXFileHooks FAR * hooks) {
  pvar->origPReadFile = *hooks->PReadFile;
  *hooks->PReadFile = TTXReadFile;
}

static void PASCAL FAR TTXCloseFile(TTXFileHooks FAR * hooks) {
  if (pvar->origPReadFile) {
    *hooks->PReadFile = pvar->origPReadFile;
  }
}

static BOOL FAR PASCAL TTXSetupWin(HWND parent, PTTSet ts) {
  BOOL ret;

  strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), pvar->orig_title, _TRUNCATE);
  ret = (pvar->origSetupWindowDlg)(parent, ts);
  if (ret) {
    strncpy_s(pvar->orig_title, sizeof(pvar->orig_title), pvar->ts->Title, _TRUNCATE);
  }
  SetTitleStr(pvar->orig_title, FALSE);

  return ret;
}

static BOOL FAR PASCAL TTXDummySetupWin(HWND parent, PTTSet ts) {
  return (TRUE);
}

static void PASCAL FAR TTXGetUIHooks(TTXUIHooks FAR * hooks) {
  if (pvar->ChangeTitle) {
    pvar->ChangeTitle = FALSE;
    *hooks->SetupWin = TTXDummySetupWin;
  }
  else {
    pvar->origSetupWindowDlg = *hooks->SetupWin;
    *hooks->SetupWin = TTXSetupWin;
  }
  return;
}

static void PASCAL FAR TTXReadIniFile(PCHAR fn, PTTSet ts) {
  char buff[sizeof(pvar->ts->Title)];

  (pvar->origReadIniFile)(fn, ts);
  GetPrivateProfileString(INISECTION, "AdditionalTitle", "", pvar->add_title, sizeof(pvar->add_title), fn);

  strncpy_s(pvar->orig_title, sizeof(pvar->orig_title), pvar->ts->Title, _TRUNCATE);

  GetPrivateProfileString(INISECTION, "AddMode", "Off", buff, sizeof(buff), fn);
  if (_stricmp(buff, "top") == 0) {
    pvar->add_mode = ADD_TOP;
    pvar->ts->AcceptTitleChangeRequest = FALSE;
    SetTitleStr(pvar->orig_title, FALSE);
  }
  else if (_stricmp(buff, "bottom") == 0) {
    pvar->add_mode = ADD_BOTTOM;
    pvar->ts->AcceptTitleChangeRequest = FALSE;
    SetTitleStr(pvar->orig_title, FALSE);
  }
  else {
    pvar->add_mode = ADD_NONE;
  }
}

static void PASCAL FAR TTXWriteIniFile(PCHAR fn, PTTSet ts) {
  strncpy_s(pvar->ts->Title, sizeof(pvar->ts->Title), pvar->orig_title, _TRUNCATE);
  (pvar->origWriteIniFile)(fn, ts);
  SetTitleStr(pvar->orig_title, FALSE);

  WritePrivateProfileString(INISECTION, "AdditionalTitle", pvar->add_title, fn);
  switch (pvar->add_mode) {
    case ADD_NONE:
      WritePrivateProfileString(INISECTION, "AddMode", "Off", fn);
      break;
    case ADD_TOP:
      WritePrivateProfileString(INISECTION, "AddMode", "Top", fn);
      break;
    case ADD_BOTTOM:
      WritePrivateProfileString(INISECTION, "AddMode", "Bottom", fn);
      break;
    default:
      ; // not reached
  }
}

PCHAR GetParam(PCHAR buff, int size, PCHAR param) {
  int i = 0;
  BOOL quoted = FALSE;

  while (*param == ' ') {
    param++;
  }

  if (*param == '\0' || *param == ';') {
    return NULL;
  }

  while (*param != '\0' && (quoted || *param != ';') && (quoted || *param != ' ')) {
    if (*param == '"') {
      quoted = !quoted;
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
    if (_strnicmp(buff, "/W=", 3) == 0) {
      strncpy_s(pvar->orig_title, sizeof(pvar->orig_title), pvar->ts->Title, _TRUNCATE);
      SetTitleStr(pvar->orig_title, FALSE);
      break;
    }
  }
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR * hooks) {
  pvar->origReadIniFile = *hooks->ReadIniFile;
  *hooks->ReadIniFile = TTXReadIniFile;
  pvar->origWriteIniFile = *hooks->WriteIniFile;
  *hooks->WriteIniFile = TTXWriteIniFile;
  pvar->origParseParam = *hooks->ParseParam;
  *hooks->ParseParam = TTXParseParam;
}

/*
static int PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd) {
  if (cmd==ID_MENUITEM) {
    return 1;
  }
  return 0;
}
*/

static TTXExports Exports = {
  sizeof(TTXExports),
  ORDER,

  TTXInit,
  TTXGetUIHooks,
  TTXGetSetupHooks,
  TTXOpenTCP,
  TTXCloseTCP,
  NULL, // TTXSetWinSize,
  NULL, // TTXModifyMenu,
  NULL, // TTXModifyPopupMenu,
  NULL, // TTXProcessCommand,
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
