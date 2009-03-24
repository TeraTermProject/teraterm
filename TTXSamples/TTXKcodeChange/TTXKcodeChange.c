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

#define IdModeFirst   0
#define IdModeESC     1
#define IdModeOSC     2
#define IdModeESC2    3
#define IdModeProc    4

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct {
  PTTSet ts;
  PComVar cv;
  Trecv origPrecv;
  Tsend origPsend;
  TReadFile origPReadFile;
  TWriteFile origPWriteFile;
} TInstVar;

static TInstVar FAR * pvar;
static TInstVar InstVar;

static void PASCAL FAR TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->origPrecv = NULL;
  pvar->origPsend = NULL;
  pvar->origPReadFile = NULL;
  pvar->origPWriteFile = NULL;
}

void CommOut(char *str, int len) {
  int outlen, c;
  char *p;

  if (len > OutBuffSize - pvar->cv->OutBuffCount)
    outlen = OutBuffSize - pvar->cv->OutBuffCount;
  else
    outlen = len;

  if (pvar->cv->OutPtr > 0) {
    memmove(pvar->cv->OutBuff, &(pvar->cv->OutBuff[pvar->cv->OutPtr]), pvar->cv->OutBuffCount);
    pvar->cv->OutPtr = 0;
  }

  c = pvar->cv->OutBuffCount;
  for (p = str; outlen>0; p++, outlen--) {
    switch (*p) {
      case 0x0d:
        switch (pvar->cv->CRSend) {
	  case IdCR:
            pvar->cv->OutBuff[c++] = 0x0d;
	    if (c < OutBuffSize && pvar->cv->TelFlag && ! pvar->cv->TelBinSend) {
              pvar->cv->OutBuff[c++] = 0;
	    }
	    break;
	  case IdLF:
            pvar->cv->OutBuff[c++] = 0x0a;
	    break;
	  case IdCRLF:
            pvar->cv->OutBuff[c++] = 0x0d;
	    if (c < OutBuffSize) {
              pvar->cv->OutBuff[c++] = 0x0a;
	    }
	    break;
	}
	if (c + outlen > OutBuffSize) {
	  outlen--;
	}
	break;
      case 0xff:
        if (pvar->cv->TelFlag) {
	  if (c < OutBuffSize - 1) {
            pvar->cv->OutBuff[c++] = 0xff;
	    pvar->cv->OutBuff[c++] = 0xff;
	  }
	}
	else {
	  pvar->cv->OutBuff[c++] = 0xff;
	}
	if (c + outlen > OutBuffSize) {
	  outlen--;
	}
	break;
      default:
        pvar->cv->OutBuff[c++] = *p;
    }
  }

  pvar->cv->OutBuffCount = c;
}

void ParseInputStr(char *rstr, int rcount) {
  static WORD mode = IdModeFirst;
  static char buff[InBuffSize];
  static unsigned int blen;
  char *p, *p2;
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
	  case 5963:
	    while (p && *p) {
	      if ((p2 = strchr(p, ';')) != NULL)
		*p2++ = '\0';
	      if (_strnicmp(p, "kt=", 3) == 0) {
	        p+= 3;
		if (_stricmp(p, "SJIS") == 0)
		  pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = IdSJIS;
		else if (_stricmp(p, "EUC") == 0)
		  pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = IdEUC;
		else if (_stricmp(p, "JIS") == 0)
		  pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = IdJIS;
		else if (_stricmp(p, "UTF8") == 0 || _stricmp(p, "UTF-8") == 0)
		  pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = IdUTF8;
	      }
	      else if (_strnicmp(p, "kr=", 3) == 0) {
		p += 3;
		if (_stricmp(p, "SJIS") == 0)
		  pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = IdSJIS;
		else if (_stricmp(p, "EUC") == 0)
		  pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = IdEUC;
		else if (_stricmp(p, "JIS") == 0)
		  pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = IdJIS;
		else if (_stricmp(p, "UTF8") == 0 || _stricmp(p, "UTF-8") == 0)
		  pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = IdUTF8;
		else if (_stricmp(p, "UTF8m") == 0 || _stricmp(p, "UTF-8m") == 0)
		  pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = IdUTF8m;
	      }
	      p = p2;
	    }
	    break;
	  case 5964:
	    while (p && *p) {
	      if ((p2 = strchr(p, ';')) != NULL)
		*p2++ = '\0';
	      if (_stricmp(p, "kt") == 0) {
		switch (pvar->cv->KanjiCodeSend) {
		  case IdSJIS:
		    CommOut("kt=SJIS;", 8);
		    break;
		  case IdEUC:
		    CommOut("kt=EUC;", 7);
		    break;
		  case IdJIS:
		    CommOut("kt=JIS;", 7);
		    break;
		  case IdUTF8:
		    CommOut("kt=UTF8;", 8);
		    break;
		}
	      }
	      else if (_stricmp(p, "kr") == 0) {
		switch (pvar->cv->KanjiCodeEcho) {
		  case IdSJIS:
		    CommOut("kr=SJIS;", 8);
		    break;
		  case IdEUC:
		    CommOut("kr=EUC;", 7);
		    break;
		  case IdJIS:
		    CommOut("kr=JIS;", 7);
		    break;
		  case IdUTF8:
		    CommOut("kr=UTF8;", 8);
		    break;
		  case IdUTF8m:
		    CommOut("kr=UTF8m;", 9);
		    break;
		}
	      }
	      p = p2;
	    }
	    CommOut("\r", 1);
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
