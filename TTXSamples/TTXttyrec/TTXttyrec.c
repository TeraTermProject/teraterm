#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "gettimeofday.h"

#include "compat_w95.h"

#define ORDER 6000
#define ID_MENUITEM 55301

#define INISECTION "ttyrec"

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  PReadIniFile origReadIniFile;
  Trecv origPrecv;
  TReadFile origPReadFile;
  HANDLE fh;
  HMENU FileMenu;
  BOOL record;
  BOOL rec_stsize;
} TInstVar;

struct recheader {
  struct timeval tv;
  int len;
};

static TInstVar *pvar;
static TInstVar InstVar;

#define GetFileMenu(menu)	GetSubMenuByChildID(menu, ID_FILE_NEWCONNECTION)
#define GetEditMenu(menu)	GetSubMenuByChildID(menu, ID_EDIT_COPY2)
#define GetSetupMenu(menu)	GetSubMenuByChildID(menu, ID_SETUP_TERMINAL)
#define GetControlMenu(menu)	GetSubMenuByChildID(menu, ID_CONTROL_RESETTERMINAL)
#define GetHelpMenu(menu)	GetSubMenuByChildID(menu, ID_HELP_ABOUT)

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

BOOL GetOnOff(PCHAR sect, PCHAR key, PCHAR fn, BOOL def) {
  char buff[4];

  GetPrivateProfileString(sect, key, "", buff, sizeof(buff), fn);

  if (def) {
    if (_stricmp(buff, "off") == 0) {
      return FALSE;
    }
    else {
      return TRUE;
    }
  }
  else {
    if (_stricmp(buff, "on") == 0) {
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
}

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->origPrecv = NULL;
  pvar->origPReadFile = NULL;
  pvar->fh = INVALID_HANDLE_VALUE;
  pvar->rec_stsize = FALSE;
  pvar->record = FALSE;
}

static void PASCAL TTXReadIniFile(PCHAR fn, PTTSet ts) {
  (pvar->origReadIniFile)(fn, ts);
  pvar->rec_stsize = GetOnOff(INISECTION, "RecordStartSize", fn, TRUE);
}

void WriteData(HANDLE fh, char *buff, int len) {
  struct timeval t;
  int b[3];
  DWORD w;

  gettimeofday(&t /*, NULL*/ );
  b[0] = t.tv_sec;
  b[1] = t.tv_usec;
  b[2] = len;
  WriteFile(pvar->fh, b, sizeof(b), &w, NULL);
  WriteFile(pvar->fh, buff, len, &w, NULL);

  return;
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
  pvar->origReadIniFile = *hooks->ReadIniFile;
  *hooks->ReadIniFile = TTXReadIniFile;
}

int PASCAL TTXrecv(SOCKET s, char *buff, int len, int flags) {
  int rlen;

  rlen = pvar->origPrecv(s, buff, len, flags);
  if (pvar->record && rlen > 0) {
    WriteData(pvar->fh, buff, rlen);
  }
  return rlen;
}

BOOL PASCAL TTXReadFile(HANDLE fh, LPVOID buff, DWORD len, LPDWORD rbytes, LPOVERLAPPED rol) {
  if (!pvar->origPReadFile(fh, buff, len, rbytes, rol))
    return FALSE;

  if (pvar->record && *rbytes > 0) {
    WriteData(pvar->fh, buff, *rbytes);
  }
  return TRUE;
}

static void PASCAL TTXOpenTCP(TTXSockHooks *hooks) {
  pvar->origPrecv = *hooks->Precv;
  *hooks->Precv = TTXrecv;
}

static void PASCAL TTXCloseTCP(TTXSockHooks *hooks) {
  if (pvar->origPrecv) {
    *hooks->Precv = pvar->origPrecv;
  }
}

static void PASCAL TTXOpenFile(TTXFileHooks *hooks) {
  if (pvar->cv->PortType == IdSerial) {
    pvar->origPReadFile = *hooks->PReadFile;
    *hooks->PReadFile = TTXReadFile;
  }
}

static void PASCAL TTXCloseFile(TTXFileHooks *hooks) {
  if (pvar->origPReadFile) {
    *hooks->PReadFile = pvar->origPReadFile;
  }
}

static void PASCAL TTXModifyMenu(HMENU menu) {
  UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

  pvar->FileMenu = GetFileMenu(menu);
  if (pvar->record) {
    flag |= MF_CHECKED;
  }
  InsertMenu(pvar->FileMenu, ID_FILE_PRINT2,
		flag, ID_MENUITEM, "TT&Y Record");
//  InsertMenu(pvar->FileMenu, ID_FILE_SENDFILE,
//		MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);
}

static void PASCAL TTXModifyPopupMenu(HMENU menu) {
  if (menu==pvar->FileMenu) {
    if (pvar->cv->Ready)
      EnableMenuItem(pvar->FileMenu, ID_MENUITEM, MF_BYCOMMAND | MF_ENABLED);
    else
      EnableMenuItem(pvar->FileMenu, ID_MENUITEM, MF_BYCOMMAND | MF_GRAYED);
  }
}

static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
  OPENFILENAME ofn;
  char fname[MAX_PATH];
  char buff[20];

  fname[0] = '\0';

  if (cmd==ID_MENUITEM) {
    if (pvar->record) {
      if (pvar->fh != INVALID_HANDLE_VALUE) {
	CloseHandle(pvar->fh);
	pvar->fh = INVALID_HANDLE_VALUE;
      }
      pvar->record = FALSE;
      CheckMenuItem(pvar->FileMenu, ID_MENUITEM, MF_BYCOMMAND | MF_UNCHECKED);
    }
    else {
      if (pvar->fh != INVALID_HANDLE_VALUE) {
	CloseHandle(pvar->fh);
      }

      memset(&ofn, 0, sizeof(ofn));
      ofn.lStructSize = get_OPENFILENAME_SIZE();
      ofn.hwndOwner = hWin;
      ofn.lpstrFilter = "ttyrec(*.tty)\0*.tty\0All files(*.*)\0*.*\0\0";
      ofn.lpstrFile = fname;
      ofn.nMaxFile = sizeof(fname);
      ofn.lpstrDefExt = "tty";
      // ofn.lpstrTitle = "";
      ofn.Flags = OFN_OVERWRITEPROMPT;
      if (GetSaveFileName(&ofn)) {
	pvar->fh = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pvar->fh != INVALID_HANDLE_VALUE) {
	  pvar->record = TRUE;
	  CheckMenuItem(pvar->FileMenu, ID_MENUITEM, MF_BYCOMMAND | MF_CHECKED);
	  if (pvar->rec_stsize) {
	    _snprintf_s(buff, sizeof(buff), _TRUNCATE, "\033[8;%d;%dt",
	                pvar->ts->TerminalHeight, pvar->ts->TerminalWidth);
	    WriteData(pvar->fh, buff, (int)strlen(buff));
	  }
	}
      }
    }
    return 1;
  }
  return 0;
}

static void PASCAL TTXEnd(void) {
  if (pvar->fh != INVALID_HANDLE_VALUE) {
    CloseHandle(pvar->fh);
    pvar->fh = INVALID_HANDLE_VALUE;
  }
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
  TTXEnd,
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
