#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat_w95.h"

#define ORDER 5800
#define ID_MENUITEM 55199

#define TERM_WIDTH  80
#define TERM_HEIGHT 24
#define MENU_STR    "80x24"

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  HMENU SetupMenu;
  BOOL ReplaceTermDlg;
} TInstVar;

static TInstVar FAR * pvar;
static TInstVar InstVar;

static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->ReplaceTermDlg = FALSE;
}

static BOOL FAR PASCAL TTXSetupTerminal(HWND parent, PTTSet ts) {
  pvar->ReplaceTermDlg = FALSE;
  return (TRUE);
}

static void PASCAL FAR TTXGetUIHooks(TTXUIHooks FAR * hooks) {
  if (pvar->ReplaceTermDlg) {
    *hooks->SetupTerminal = TTXSetupTerminal;
  }
  return;
}

static void PASCAL FAR TTXModifyMenu(HMENU menu) {
  pvar->SetupMenu = menu;
  InsertMenu(pvar->SetupMenu, ID_HELPMENU, MF_ENABLED, ID_MENUITEM, MENU_STR);
}

static int PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd) {
  if (cmd == ID_MENUITEM) {
    pvar->ts->TerminalWidth = TERM_WIDTH;
    pvar->ts->TerminalHeight = TERM_HEIGHT;
    pvar->ReplaceTermDlg = TRUE;

    // Call Setup-Terminal dialog
    SendMessage(hWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);
    return 1;
  }
  return 0;
}

static TTXExports Exports = {
  sizeof(TTXExports),
  ORDER,

  TTXInit,
  TTXGetUIHooks,
  NULL, // TTXGetSetupHooks,
  NULL, // TTXOpenTCP,
  NULL, // TTXCloseTCP,
  NULL, // TTXSetWinSize,
  TTXModifyMenu,
  NULL, // TTXModifyPopupMenu,
  TTXProcessCommand,
  NULL, // TTXEnd
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
