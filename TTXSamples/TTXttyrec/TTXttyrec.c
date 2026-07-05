#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"

#include "inifile_com.h"
#include "ttcommdlg.h"

#include "gettimeofday.h"
#include "autofilename.h"

#define ORDER 6000
#define ID_MENUITEM 55301

#define INISECTION "ttyrec"
#define INISECTIONW L"ttyrec"

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
  BOOL rec_auto;
  wchar_t *rec_name;
  wchar_t *rec_path;
  int tel_mode;
  int tel_buf_len;
  char *tel_buf;
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
	if ((m = GetSubMenu(menu, i))) {
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

BOOL GetOnOff(PCHAR sect, PCHAR key, const wchar_t *fn, BOOL def) {
  char buff[4];

  GetPrivateProfileStringAFileW(sect, key, "", buff, sizeof(buff), fn);

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
  pvar->rec_auto = FALSE;
  pvar->rec_name = NULL;
  pvar->rec_path = NULL;
  pvar->tel_mode = 0;
  pvar->tel_buf_len = 32;
  pvar->tel_buf = (char*)malloc(pvar->tel_buf_len);
}

static void PASCAL TTXReadIniFile(const wchar_t *fn, PTTSet ts) {
  (pvar->origReadIniFile)(fn, ts);
  pvar->rec_stsize = GetOnOff(INISECTION, "RecordStartSize", fn, TRUE);
  pvar->rec_auto = GetOnOff(INISECTION, "RecordAutoStart", fn, FALSE);

  size_t len = MAX_PATH;
  wchar_t *buff = (wchar_t*)malloc(len);
  GetPrivateProfileStringW(INISECTIONW, L"RecordDefaultName", L"teraterm.tty", buff, len, fn);
  free(pvar->rec_name);
  pvar->rec_name = buff;

  buff = (wchar_t*)malloc(len);
  GetPrivateProfileStringW(INISECTIONW, L"RecordDefaultPath", L"", buff, len, fn);
  free(pvar->rec_path);
  pvar->rec_path = buff;
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
  pvar->origReadIniFile = *hooks->ReadIniFile;
  *hooks->ReadIniFile = TTXReadIniFile;
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

void WriteDataTelnet(HANDLE fh, char *buff, int len) {
  char *p;
  int sz;

  if (pvar->tel_buf_len < len){
    free(pvar->tel_buf);
    pvar->tel_buf_len = len;
    pvar->tel_buf = (char *)malloc(pvar->tel_buf_len);
  }
  p = pvar->tel_buf;
  for (int i=0; i < len; i ++){
    char ch = buff[i];
    switch (pvar->tel_mode) {
      case 0:
        if (ch == (char)0xff) {
          pvar->tel_mode = 1;
        }
        else {
          *p++ = ch;
        }
        break;
      case 1:
        if (ch == (char)0xff) {
          *p++ = ch;
          pvar->tel_mode = 0;
        }
        else if ((char)0xfb <= ch && ch <= (char)0xfe) {
          pvar->tel_mode = 2;
        }
        else if (ch == (char)0xfa) {
          pvar->tel_mode = 3;
        }
        else {
          pvar->tel_mode = 0;
        }
        break;
      case 2:
        pvar->tel_mode = 0;
        break;
      case 3:
        if (ch == (char)0xff) {
          pvar->tel_mode = 4;
        }
        break;
      case 4:
        if (ch == (char)0xf0) {
          pvar->tel_mode = 0;
        }
        else if (ch != (char)0xff) {
          pvar->tel_mode = 3;
        }
        break;
      default:
        pvar->tel_mode = 0;
        break;
    }
  }

  sz = (int)(p - pvar->tel_buf);
  if (sz > 0) {
    WriteData(fh, pvar->tel_buf, sz);
  }

  return;
}

void StartRecording(wchar_t *fname)
{
  pvar->fh = CreateFileW(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (pvar->fh != INVALID_HANDLE_VALUE) {
    pvar->record = TRUE;
    CheckMenuItem(pvar->FileMenu, ID_MENUITEM, MF_BYCOMMAND | MF_CHECKED);
    if (pvar->cv->Ready) {
      if (pvar->rec_stsize) {
        char buff[20];
        _snprintf_s(buff, sizeof(buff), _TRUNCATE, "\033[8;%d;%dt",
          pvar->ts->TerminalHeight, pvar->ts->TerminalWidth);
        WriteData(pvar->fh, buff, (int)strlen(buff));
      }
    }
  }
}

void StopRecording()
{
  if (pvar->fh != INVALID_HANDLE_VALUE) {
    CloseHandle(pvar->fh);
    pvar->fh = INVALID_HANDLE_VALUE;
  }
  pvar->record = FALSE;
  CheckMenuItem(pvar->FileMenu, ID_MENUITEM, MF_BYCOMMAND | MF_UNCHECKED);
}

int PASCAL TTXrecv(SOCKET s, char *buff, int len, int flags) {
  int rlen;

  rlen = pvar->origPrecv(s, buff, len, flags);
  if (pvar->record && rlen > 0) {
    if (pvar->cv->TelFlag) {
      WriteDataTelnet(pvar->fh, buff, rlen);
    }
    else {
      WriteData(pvar->fh, buff, rlen);
    }
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
  if (pvar->rec_auto) {
    wchar_t *_fname;
    _fname = GetAutoFilename(pvar->cv, pvar->rec_name, pvar->rec_path, L"teraterm.tty");
    StartRecording(_fname);
    free(_fname);
  }
}

static void PASCAL TTXCloseTCP(TTXSockHooks *hooks) {
  if (pvar->origPrecv) {
    *hooks->Precv = pvar->origPrecv;
  }
  if (pvar->rec_auto) {
    StopRecording();
  }
}

static void PASCAL TTXOpenFile(TTXFileHooks *hooks) {
  if (pvar->cv->PortType == IdSerial) {
    pvar->origPReadFile = *hooks->PReadFile;
    *hooks->PReadFile = TTXReadFile;
    if (pvar->rec_auto) {
      wchar_t *_fname;
      _fname = GetAutoFilename(pvar->cv, pvar->rec_name, pvar->rec_path, L"teraterm.tty");
      StartRecording(_fname);
      free(_fname);
    }
  }
}

static void PASCAL TTXCloseFile(TTXFileHooks *hooks) {
  if (pvar->origPReadFile) {
    *hooks->PReadFile = pvar->origPReadFile;
  }
  if (pvar->rec_auto) {
    StopRecording();
  }
}

static void PASCAL TTXModifyMenu(HMENU menu) {
  UINT flag = MF_BYCOMMAND | MF_STRING | MF_ENABLED;

  pvar->FileMenu = GetFileMenu(menu);
  if (pvar->record) {
    flag |= MF_CHECKED;
  }

  wchar_t *uimsg;
  GetI18nStrWW("TTXttyrec", "MENU_RECORD", L"TT&Y Record", pvar->ts->UILanguageFileW, &uimsg);
  InsertMenuW(pvar->FileMenu, ID_FILE_PRINT2,
		flag, ID_MENUITEM, uimsg);
  free(uimsg);
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

static int PASCAL TTXProcessCommand(HWND hWin, WORD cmd)
{
	if (cmd==ID_MENUITEM) {
		if (pvar->record) {
			StopRecording();
		}
		else {
			TTOPENFILENAMEW ofn;
			wchar_t *fname;
			wchar_t *uimsg1, *uimsg2;

			StopRecording();

			GetI18nStrWW("TTXttyrec", "FILE_FILTER", L"ttyrec(*.tty)\\0*.tty\\0All files(*.*)\\0*.*\\0\\0", pvar->ts->UILanguageFileW, &uimsg1);
			GetI18nStrWW("TTXttyrec", "FILE_DEFAULTEXTENSION", L"tty", pvar->ts->UILanguageFileW, &uimsg2);
			memset(&ofn, 0, sizeof(ofn));
			ofn.hwndOwner = hWin;
			ofn.lpstrFilter = uimsg1;
			ofn.lpstrFile = NULL;
			ofn.lpstrDefExt = uimsg2;
			ofn.lpstrInitialDir = pvar->ts->LogDirW;
			//ofn.lpstrTitle = L"";
			ofn.Flags = OFN_OVERWRITEPROMPT;
			if (TTGetSaveFileNameW(&ofn, &fname)) {
				StartRecording(fname);
				free(fname);
			}
			free(uimsg1);
			free(uimsg2);
		}
		return 1;
	}
	return 0;
}

static void PASCAL TTXEnd(void) {
  StopRecording();
  free(pvar->tel_buf);
  pvar->tel_buf = NULL;
  free(pvar->rec_name);
  pvar->rec_name = NULL;
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
      hInst = hInstance;
      pvar = &InstVar;
      break;
    case DLL_PROCESS_DETACH:
      /* do process cleanup */
      break;
  }
  return TRUE;
}
