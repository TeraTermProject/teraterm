#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
// #include "ttlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compat_w95.h"

#define WIDTH 80
#define HEIGHT 24

#define ORDER 5990

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  BOOL sizeModify;
  PSetupTerminal origSetupTerminalDlg;
  PReadIniFile origReadIniFile;
} TInstVar;

static TInstVar FAR * pvar;
static TInstVar InstVar;

static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
    pvar->ts = ts;
    pvar->cv = cv;

    pvar->sizeModify = FALSE;
    pvar->origSetupTerminalDlg = NULL;
    pvar->origReadIniFile = NULL;
}

static BOOL FAR PASCAL FixedSizeSetupTerminalDlg(HWND parent, PTTSet ts) {
    BOOL ret;
    if (pvar->sizeModify) {
	pvar->ts->TerminalWidth = WIDTH;
	pvar->ts->TerminalHeight = HEIGHT;
	pvar->sizeModify = FALSE;
	ret = TRUE;
    }
    else {
	ret = (pvar->origSetupTerminalDlg)(parent, ts);
	if (ret) {
	    pvar->ts->TerminalWidth = WIDTH;
	    pvar->ts->TerminalHeight = HEIGHT;
	}
    }

    return ret;
}

static void PASCAL FAR TTXGetUIHooks(TTXUIHooks FAR * hooks) {
    pvar->origSetupTerminalDlg = *hooks->SetupTerminal;
    *hooks->SetupTerminal = FixedSizeSetupTerminalDlg;
}

static void PASCAL FAR FixedSizeReadIniFile(PCHAR fn, PTTSet ts) {
    (pvar->origReadIniFile)(fn, ts);
    ts->TerminalWidth = WIDTH;
    ts->TerminalHeight = HEIGHT;
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR * hooks) {
    pvar->origReadIniFile = *hooks->ReadIniFile;
    *hooks->ReadIniFile = FixedSizeReadIniFile;
}

static void PASCAL FAR TTXSetWinSize(int rows, int cols) {
    if (rows != HEIGHT || cols != WIDTH) {
	pvar->sizeModify = TRUE;
	// Call Setup-Terminal dialog
	SendMessage(pvar->cv->HWin, WM_COMMAND, MAKELONG(50310, 0), 0);
    }
}

static TTXExports Exports = {
  sizeof(TTXExports),
  ORDER,

  TTXInit,
  TTXGetUIHooks,
  TTXGetSetupHooks,
  NULL, // TTXOpenTCP,
  NULL, // TTXCloseTCP,
  TTXSetWinSize,
  NULL, // TTXModifyMenu,
  NULL, // TTXModifyPopupMenu,
  NULL, // TTXProcessCommand,
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
