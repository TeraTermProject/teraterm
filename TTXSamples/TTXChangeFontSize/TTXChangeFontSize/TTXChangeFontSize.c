#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include "tt-version.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if TT_VERSION_MAJOR == 5
  #include "compat_win.h"
  #include "inifile_com.h"
  #pragma comment(lib, "common_static.lib")
  typedef const wchar_t *TTXIniFile;
#else
  #include "compat_w95.h"
  #define GetPrivateProfileStringAFileW GetPrivateProfileString
  #define GetPrivateProfileIntAFileW GetPrivateProfileInt
  typedef char *TTXIniFile
#endif

#define ORDER 5900
#define SECTION "Change Font Size"

#define DEFAULT_MENU_TITLE "FontSize(&x)"

#define ID_MENUID_BASE 58101
#define MAX_MENU_ITEMS 20
#define ID_MENUID_INC_SIZE (ID_MENUID_BASE + 50)
#define ID_MENUID_DEC_SIZE (ID_MENUID_BASE + 51)

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  HMENU FontSizeMenu;
  BOOL ReplaceWinDlg;
  BOOL ReplaceTermDlg;
  PReadIniFile origReadIniFile;
  int MenuItems;
  int ResizeList[MAX_MENU_ITEMS];
  char MenuTitle[40];
} TInstVar;

static TInstVar *pvar;

/* WIN32 allows multiple instances of a DLL */
static TInstVar InstVar;

void InitMenu() {
  int i;
  char tmp[20];

  if (pvar->FontSizeMenu != NULL) {
    DestroyMenu(pvar->FontSizeMenu);
  }

  if (pvar->MenuItems > 0) {
    pvar->FontSizeMenu = CreateMenu();

    for (i=0; i < pvar->MenuItems; i++) {
      if (i<15)
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%d (&%x)", pvar->ResizeList[i], i+1);
      else
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%d", pvar->ResizeList[i]);

      InsertMenu(pvar->FontSizeMenu, -1, MF_BYPOSITION, ID_MENUID_BASE+i, tmp);
    }
    InsertMenu(pvar->FontSizeMenu, -1, MF_BYPOSITION, ID_MENUID_INC_SIZE, "&Increase");
    InsertMenu(pvar->FontSizeMenu, -1, MF_BYPOSITION, ID_MENUID_DEC_SIZE, "&Decrease");
  }
  else {
    pvar->FontSizeMenu = NULL;
  }
}

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->ReplaceTermDlg = FALSE;
  pvar->ReplaceWinDlg = FALSE;
  pvar->FontSizeMenu = NULL;
  pvar->MenuItems = 0;
  strncpy_s(pvar->MenuTitle, sizeof(pvar->MenuTitle), DEFAULT_MENU_TITLE, _TRUNCATE);
}

static BOOL PASCAL TTXSetupWin(HWND parent, PTTSet ts) {
  pvar->ReplaceWinDlg = FALSE;
  return (TRUE);
}

static BOOL PASCAL TTXSetupTerm(HWND parent, PTTSet ts) {
  pvar->ReplaceTermDlg = FALSE;
  return (TRUE);
}

static void PASCAL TTXGetUIHooks(TTXUIHooks *hooks) {
  if (pvar->ReplaceWinDlg) {
    *hooks->SetupWin = TTXSetupWin;
  }
  if (pvar->ReplaceTermDlg) {
    *hooks->SetupTerminal = TTXSetupTerm;
  }
  return;
}

static void PASCAL TTXReadIniFile(TTXIniFile fn, PTTSet ts) {
  int i, size;
  char Key[20];

  (pvar->origReadIniFile)(fn, ts);

  GetPrivateProfileStringAFileW(SECTION, "MenuTitle", DEFAULT_MENU_TITLE, pvar->MenuTitle, sizeof(pvar->MenuTitle), fn);

  for (i=0; i<MAX_MENU_ITEMS; i++) {
    _snprintf_s(Key, sizeof(Key), _TRUNCATE, "FontSize%d", i+1);
    size = GetPrivateProfileIntAFileW(SECTION, Key, 0, fn);

    if (size != 0) {
      pvar->ResizeList[i] = size;
    }
    else {
      break;
    }
  }

  pvar->MenuItems = i;
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
  pvar->origReadIniFile = *hooks->ReadIniFile;
  *hooks->ReadIniFile = TTXReadIniFile;
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
    mi.hSubMenu = pvar->FontSizeMenu;
    mi.dwTypeData = pvar->MenuTitle;
    InsertMenuItem(menu, ID_HELPMENU, FALSE, &mi);
  }
}

static int PASCAL TTXProcessCommand(HWND HWin, WORD cmd) {
  if (cmd >= ID_MENUID_BASE && cmd < ID_MENUID_BASE + pvar->MenuItems) {
    pvar->ts->VTFontSize.y = pvar->ResizeList[cmd - ID_MENUID_BASE];
    pvar->ReplaceWinDlg = TRUE;
  }
  else if (cmd == ID_MENUID_INC_SIZE) {
    if (pvar->ts->VTFontSize.y > 0) {
      (pvar->ts->VTFontSize.y) += 1;
    }
    else {
      (pvar->ts->VTFontSize.y) -= 1;
    }
    pvar->ReplaceWinDlg = TRUE;
  }
  else if (cmd == ID_MENUID_DEC_SIZE) {
    if (pvar->ts->VTFontSize.y > 0) {
      (pvar->ts->VTFontSize.y) -= 1;
    }
    else {
      (pvar->ts->VTFontSize.y) += 1;
    }
    pvar->ReplaceWinDlg = TRUE;
  }

  if (pvar->ReplaceWinDlg) {
    int orgTermWidth = pvar->ts->TerminalWidth;

    // Call Setup-Window dialog
    SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);

    // Call Setup-Terminal dialog
    pvar->ReplaceTermDlg = TRUE;
    pvar->ts->TerminalWidth = orgTermWidth+1;
    SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);

    // Call Setup-Terminal dialog
    pvar->ReplaceTermDlg = TRUE;
    pvar->ts->TerminalWidth = orgTermWidth;
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
  NULL, // TTXModifyPopupMenu,
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
#if TT_VERSION_MAJOR < 5
      DoCover_IsDebuggerPresent();
#endif
      hInst = hInstance;
      pvar = &InstVar;
      break;
    case DLL_PROCESS_DETACH:
      /* do process cleanup */
      break;
  }
  return TRUE;
}
