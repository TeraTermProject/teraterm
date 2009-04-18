#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "ttlib.h"
//#include "tt_res.h"

#include "compat_w95.h"

#define ORDER 4000
#define BUFF_SIZE 4096

static HANDLE hInst;

typedef struct {
	PTTSet ts;
	PComVar cv;
	Tsend origPsend;
	char buff[BUFF_SIZE];
	int buff_used;
} TInstVar;

typedef TInstVar FAR * PTInstVar;
PTInstVar pvar;
static TInstVar InstVar;

//
//  TTXInit -- 起動時処理
//
static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
	pvar->ts = ts;
	pvar->cv = cv;
	pvar->origPsend = NULL;
	pvar->buff_used = 0;
}

//
//  TTXSend -- キー入力処理
//
static int PASCAL FAR TTXsend(SOCKET s, const char FAR *buf, int len, int flags) {
	int i, wlen;

	if (len > 0) {
		for (i=0; i<len; i++) {
			switch (buf[i]) {
			case '\n':
				if (pvar->buff_used < BUFF_SIZE)
					pvar->buff[pvar->buff_used++] = '\n';
				wlen = pvar->origPsend(s, pvar->buff, pvar->buff_used, flags);
				if (wlen > 0 && wlen < pvar->buff_used) {
					pvar->buff_used -= wlen;
					memmove(pvar->buff, &(pvar->buff[wlen]), pvar->buff_used);
				}
				else {
					pvar->buff_used = 0;
				}
				break;
			case 0x08: // ^H
				if (pvar->buff_used > 0)
					pvar->buff_used--;
				break;
			case 0x15: // ^U
				pvar->buff_used = 0;
				break;
			default:
				if (pvar->buff_used < BUFF_SIZE)
					pvar->buff[pvar->buff_used++] = buf[i];
				break;
			}
		}
	}

	return len;
}

//
// TTXOpen -- セッション開始処理
//	Psend をフックする。
//
static void PASCAL FAR TTXOpenTCP(TTXSockHooks FAR * hooks) {
	pvar->origPsend = *hooks->Psend;
	*hooks->Psend = TTXsend;
	pvar->buff_used = 0;
}

//
// TTXCloseTCP -- セッション終了時処理
//	Psend のフックを解除する。
//
static void PASCAL FAR TTXCloseTCP(TTXSockHooks FAR * hooks) {
	if (pvar->origPsend) {
		*hooks->Psend = pvar->origPsend;
	}
}

static TTXExports Exports = {
	sizeof(TTXExports),
	ORDER,

	TTXInit,
	NULL, // TTXGetUIHooks,
	NULL, // TTXGetSetupHooks,
	TTXOpenTCP,
	TTXCloseTCP,
	NULL, // TTXSetWinSize,
	NULL, // TTXModifyMenu,
	NULL, // TTXModifyPopupMenu,
	NULL, // TTXProcessCommand,
	NULL, // TTXEnd,
	NULL, // TTXSetCommandLine,
	NULL, // TTXOpenFile,
	NULL, // TTXCloseFile
};

BOOL __declspec(dllexport) PASCAL FAR TTXBind(WORD Version, TTXExports FAR * exports) {
	int size = sizeof(Exports) - sizeof(exports->size);
	/* do version checking if necessary */
	/* if (Version!=TTVERSION) return FALSE; */

	if (size > exports->size) {
		size = exports->size;
	}
	memcpy((char FAR *)exports + sizeof(exports->size),
	       (char FAR *)&Exports + sizeof(exports->size), size);
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
