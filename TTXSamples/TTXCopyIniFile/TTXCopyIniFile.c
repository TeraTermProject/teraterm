#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "compat_w95.h"

#define ORDER 9999

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
	PReadIniFile origReadIniFile;
	PWriteIniFile origWriteIniFile;
	char origIniFileName[MAXPATHLEN];
} TInstVar;

static TInstVar FAR * pvar;
static TInstVar InstVar;

static void PASCAL FAR TTXReadIniFile(PCHAR fn, PTTSet ts) {
	strcpy_s(pvar->origIniFileName, sizeof(pvar->origIniFileName), fn);
	(pvar->origReadIniFile)(fn, ts);
}

static void PASCAL FAR TTXWriteIniFile(PCHAR fn, PTTSet ts) {
	CopyFile(pvar->origIniFileName, fn, TRUE);
	(pvar->origWriteIniFile)(fn, ts);
}

static void PASCAL FAR TTXGetSetupHooks(TTXSetupHooks FAR * hooks) {
	if (pvar->origIniFileName[0] == 0) {
		pvar->origReadIniFile = *hooks->ReadIniFile;
		*hooks->ReadIniFile = TTXReadIniFile;
	}
	else {
		pvar->origWriteIniFile = *hooks->WriteIniFile;
		*hooks->WriteIniFile = TTXWriteIniFile;
	}
}

static TTXExports Exports = {
	sizeof(TTXExports),
	ORDER,

	NULL,	// TTXInit,
	NULL,	// TTXGetUIHooks,
	TTXGetSetupHooks,
	NULL,	// TTXOpenTCP,
	NULL,	// TTXCloseTCP,
	NULL,	// TTXSetWinSize,
	NULL,	// TTXModifyMenu,
	NULL,	// TTXModifyPopupMenu,
	NULL,	// TTXProcessCommand,
	NULL,	// TTXEnd,
	NULL,	// TTXSetCommandLine,
	NULL,	// TTXOpenFile,
	NULL	// TTXCloseFile
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
