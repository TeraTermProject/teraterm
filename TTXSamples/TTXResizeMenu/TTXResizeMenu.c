#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat_w95.h"

#define ORDER 5900
#define SECTION "Resize Menu"

#define ID_MENUID_BASE 55101
#define MAX_MENU_ITEMS 20

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  HMENU ResizeMenu;
  BOOL ReplaceTermDlg;
  BOOL useMultiMonitorAPI;
  PReadIniFile origReadIniFile;
  int MenuItems;
  int ResizeList[MAX_MENU_ITEMS][2];
} TInstVar;

static TInstVar *pvar;

/* WIN32 allows multiple instances of a DLL */
static TInstVar InstVar;

BOOL GetMonitorSizeByChar(int *width, int *height) {
  RECT rc_dsk, rc_wnd, rc_cl;

  if (width) {
    *width = 0;
  }
  if (height) {
    *height = 0;
  }

  if (pvar->useMultiMonitorAPI) {
    HMONITOR hm;
    MONITORINFO mi;

    hm = MonitorFromWindow(pvar->cv->HWin, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(MONITORINFO);
    if (! GetMonitorInfo(hm, &mi)) {
      return FALSE;
    }
    rc_dsk = mi.rcWork;
  }
  else {
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc_dsk, 0);
  }

  if (!GetWindowRect(pvar->cv->HWin, &rc_wnd) || !GetClientRect(pvar->cv->HWin, &rc_cl)) {
    return FALSE;
  }


  if (width) {
    int margin_w, cell_w;

    margin_w = (rc_wnd.right - rc_wnd.left) - (rc_cl.right - rc_cl.left);
    cell_w = (rc_cl.right - rc_cl.left) / pvar->ts->TerminalWidth;

    *width = (rc_dsk.right - rc_dsk.left - margin_w) / cell_w;
  }
  if (height) {
    int margin_h, cell_h;

    margin_h = (rc_wnd.bottom - rc_wnd.top) - (rc_cl.bottom - rc_cl.top);
    cell_h = (rc_cl.bottom - rc_cl.top) / pvar->ts->TerminalHeight;

    *height = (rc_dsk.bottom - rc_dsk.top - margin_h) / cell_h;
  }

  return TRUE;
}

int mkMenuEntry(char *buff, size_t buffsize, int x, int y, int c) {
  char tmp[20];

  if (x == 0)
    _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "height: %d", y);
  else if (y == 0)
    _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "width: %d", x);
  else
    _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%dx%d", x, y);

  if (c < 15)
    return _snprintf_s(buff, buffsize, _TRUNCATE, "%s(&%x)", tmp, c+1);
  else if (c < 35)
    return _snprintf_s(buff, buffsize, _TRUNCATE, "%s(&%c)", tmp, 'a' + 1 - 9);
  else
    return _snprintf_s(buff, buffsize, _TRUNCATE, "%s", tmp);
}

void InitMenu() {
  int i, x, y, full_w, full_h;
  char tmp[20];

  if (pvar->ResizeMenu != NULL) {
    DestroyMenu(pvar->ResizeMenu);
  }

  if (pvar->MenuItems > 0) {
    GetMonitorSizeByChar(&full_w, &full_h);
    pvar->ResizeMenu = CreateMenu();

    for (i=0; i < pvar->MenuItems; i++) {
      x = pvar->ResizeList[i][0];
      if (x == -1)
	x = full_w;
      y = pvar->ResizeList[i][1];
      if (y == -1)
	y = full_h;

      mkMenuEntry(tmp, sizeof(tmp), x, y, i);
      InsertMenu(pvar->ResizeMenu, -1, MF_BYPOSITION, ID_MENUID_BASE+i, tmp);
    }
  }
  else {
    pvar->ResizeMenu = NULL;
  }
}

void UpdateMenu() {
  int i, x, y, full_w, full_h;
  char tmp[20];

  if (pvar->ResizeMenu == NULL)
    return;

  GetMonitorSizeByChar(&full_w, &full_h);

  for (i=0; i < pvar->MenuItems; i++) {
    x = pvar->ResizeList[i][0];
    y = pvar->ResizeList[i][1];
    if (x == -1 || y == -1) {
      if (x == -1)
	x = full_w;
      if (y == -1)
	y = full_h;
      mkMenuEntry(tmp, sizeof(tmp), (x==-1)?full_w:x, (y==-1)?full_h:y, i);
      ModifyMenu(pvar->ResizeMenu, i, MF_BYPOSITION, ID_MENUID_BASE+i, tmp);
    }
  }
}

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->ReplaceTermDlg = FALSE;
  pvar->ResizeMenu = NULL;
  pvar->MenuItems = 0;

  if (!HasMultiMonitorSupport()) {
    pvar->useMultiMonitorAPI = FALSE;
  }
  else {
    pvar->useMultiMonitorAPI = TRUE;
  }
}

static BOOL PASCAL TTXSetupTerminal(HWND parent, PTTSet ts) {
  pvar->ReplaceTermDlg = FALSE;
  return (TRUE);
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks) {
  if (pvar->ReplaceTermDlg) {
    *hooks->SetupTerminal = TTXSetupTerminal;
  }
  return;
}

static void PASCAL ResizeMenuReadIniFile(PCHAR fn, PTTSet ts) {
  int i, x, y;
  char Key[20], Buff[100];

  (pvar->origReadIniFile)(fn, ts);

  for (i=0; i<MAX_MENU_ITEMS; i++) {
    _snprintf_s(Key, sizeof(Key), _TRUNCATE, "ResizeMenu%d", i+1);
    GetPrivateProfileString(SECTION, Key, "\n", Buff, sizeof(Buff), fn);

    if (sscanf_s(Buff, "%d , %d", &x, &y) == 2) {
      if (x < -1 ) {
        x = 0;
      }

      if (y < -1 ) {
        y = 0;
      }

      if (x > TermWidthMax ) {
        x = TermWidthMax;
      }

      if (y > TermHeightMax ) {
        y = TermHeightMax;
      }

      if (x == 0 && y == 0) {
        break;
      }

      pvar->ResizeList[i][0] = x;
      pvar->ResizeList[i][1] = y;
    }
    else {
      break;
    }
  }

  pvar->MenuItems = i;
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
  pvar->origReadIniFile = *hooks->ReadIniFile;
  *hooks->ReadIniFile = ResizeMenuReadIniFile;
}

static void PASCAL TTXModifyMenu(HMENU menu) {
  MENUITEMINFO mi;

  if (pvar->MenuItems > 0) {
    InitMenu();

    if (IsWindows2000OrLater()) {
      memset(&mi, 0, sizeof(MENUITEMINFO));
      mi.cbSize = sizeof(MENUITEMINFO);
    }
    else {
      memset(&mi, 0, sizeof(MENUITEMINFO)-sizeof(HBITMAP));
      mi.cbSize = sizeof(MENUITEMINFO)-sizeof(HBITMAP);
    }
    mi.fMask  = MIIM_TYPE | MIIM_SUBMENU;
    mi.fType  = MFT_STRING;
    mi.hSubMenu = pvar->ResizeMenu;
    mi.dwTypeData = "Resi&ze";
    InsertMenuItem(menu, ID_HELPMENU, FALSE, &mi);
  }
}

static void PASCAL TTXModifyPopupMenu(HMENU menu) {
  if (menu == pvar->ResizeMenu) {
    UpdateMenu();
  }
}

static int PASCAL TTXProcessCommand(HWND HWin, WORD cmd) {
  int num, full_h, full_w;
  if (cmd >= ID_MENUID_BASE && cmd < ID_MENUID_BASE + pvar->MenuItems) {
    GetMonitorSizeByChar(&full_w, &full_h);
    num = cmd - ID_MENUID_BASE;
    if (pvar->ResizeList[num][0] > 0) {
      pvar->ts->TerminalWidth = pvar->ResizeList[num][0];
    }
    else if (pvar->ResizeList[num][0] == -1 && full_w > 0) {
      pvar->ts->TerminalWidth = full_w;
    }
    if (pvar->ResizeList[num][1] > 0) {
      pvar->ts->TerminalHeight = pvar->ResizeList[num][1];
    }
    else if (pvar->ResizeList[num][1] == -1 && full_h > 0) {
      pvar->ts->TerminalHeight = full_h;
    }
    pvar->ReplaceTermDlg = TRUE;

    // Call Setup-Terminal dialog
    SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);
    return 1;
  }
  return 0;
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
  NULL, // TTXEnd
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

BOOL WINAPI DllMain(HANDLE hInstance, ULONG ul_reason, LPVOID lpReserved)
{
  switch (ul_reason) {
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
