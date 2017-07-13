#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"
#include <stdlib.h>
#include <stdio.h>

#include "compat_w95.h"

#define ORDER 5800

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  PParseParam origParseParam;
} TInstVar;

static TInstVar *pvar;
static TInstVar InstVar;

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
}

BOOL ColorStr2ColorRef(COLORREF *color, PCHAR Str) {
  int TmpColor[3];
  int i, result;
  PCHAR cur, next;

  cur = Str;

  for (i=0; i<3; i++) {
    if (!cur)
      return FALSE;

    if ((next = strchr(cur, ',')) != NULL)
      *next = 0;

    result = sscanf_s(cur, "%d", &TmpColor[i]);

    if (next)
      *next++ = ',';

    if (result != 1 || TmpColor[i] < 0 || TmpColor[i] > 255)
      return FALSE;

    cur = next;
  }

  *color = RGB((BYTE)TmpColor[0], (BYTE)TmpColor[1], (BYTE)TmpColor[2]);
  return TRUE;
}

static void PASCAL TTXParseParam(PCHAR Param, PTTSet ts, PCHAR DDETopic) {
  char buff[1024];
  PCHAR start, cur, next;
  int x, y;

  /* the first term shuld be executable filename of Tera Term */
  start = GetParam(buff, sizeof(buff), Param);

  cur = start;
  while (next = GetParam(buff, sizeof(buff), cur)) {
    if (_strnicmp(buff, "/FG=", 4) == 0) {
      ColorStr2ColorRef(&(ts->VTColor[0]), &buff[4]);
      memset(cur, ' ', next - cur);
    }
    else if (_strnicmp(buff, "/BG=", 4) == 0) {
      ColorStr2ColorRef(&(ts->VTColor[1]), &buff[4]);
      memset(cur, ' ', next - cur);
    }
    else if (_strnicmp(buff, "/SIZE=", 6) == 0) {
      if (sscanf_s(buff+6, "%dx%d", &x, &y) == 2 || sscanf_s(buff+6, "%d,%d", &x, &y) == 2) {
	ts->TerminalWidth = x;
	ts->TerminalHeight = y;
      }
      memset(cur, ' ', next - cur);
    }
    cur = next;
  }

  pvar->origParseParam(Param, ts, DDETopic);
}

static void PASCAL TTXGetSetupHooks(TTXSetupHooks *hooks) {
  pvar->origParseParam = *hooks->ParseParam;
  *hooks->ParseParam = TTXParseParam;
}

static TTXExports Exports = {
  sizeof(TTXExports),
  ORDER,

  TTXInit,
  NULL, // TTXGetUIHooks,
  TTXGetSetupHooks,
  NULL, // TTXOpenTCP,
  NULL, // TTXCloseTCP,
  NULL, // TTXSetWinSize,
  NULL, // TTXModifyMenu,
  NULL, // TTXModifyPopupMenu,
  NULL, // TTXProcessCommand,
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
