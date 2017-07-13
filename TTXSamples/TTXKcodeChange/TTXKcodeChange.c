#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttplugin.h"
#include "tt_res.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include "compat_w95.h"

#define ORDER 4800

#define IdModeFirst   0
#define IdModeESC     1
#define IdModeOSC     2
#define IdModeESC2    3
#define IdModeProc    4

static HANDLE hInst; /* Instance handle of TTX*.DLL */

typedef struct cfgstack {
    int val;
    struct cfgstack *next;
} TStack;
typedef TStack *PStack;

typedef struct {
  PTTSet ts;
  PComVar cv;
  PStack config[2];
  Trecv origPrecv;
  Tsend origPsend;
  TReadFile origPReadFile;
  TWriteFile origPWriteFile;
} TInstVar;

static TInstVar *pvar;
static TInstVar InstVar;

static void PASCAL TTXInit(PTTSet ts, PComVar cv) {
  pvar->ts = ts;
  pvar->cv = cv;
  pvar->config[0] = NULL;
  pvar->config[1] = NULL;
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

void ParseInputStr(unsigned char *rstr, int rcount) {
  static WORD mode = IdModeFirst;
  static unsigned char buff[InBuffSize];
  static unsigned int blen;
  unsigned char *p, *p2;
  int i;
  unsigned int func;
  PStack t;

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
	  case 5965:
	    while (p && *p) {
	      if ((p2 = strchr(p, ';')) != NULL)
		*p2++ = '\0';
	      if (_stricmp(p, "kt") == 0) {
		if ((t=malloc(sizeof(TStack))) != NULL) {
		  t->val = pvar->cv->KanjiCodeSend;
		  t->next = pvar->config[0];
		  pvar->config[0] = t;
		}
	      }
	      else if (_stricmp(p, "kr") == 0) {
		if ((t=malloc(sizeof(TStack))) != NULL) {
		  t->val = pvar->cv->KanjiCodeEcho;
		  t->next = pvar->config[1];
		  pvar->config[1] = t;
		}
	      }
	      p = p2;
	    }
	    break;
	  case 5966:
	    while (p && *p) {
	      if ((p2 = strchr(p, ';')) != NULL)
		*p2++ = '\0';
	      if (_stricmp(p, "kt") == 0) {
		if (pvar->config[0] != NULL) {
		  t = pvar->config[0];
		  pvar->cv->KanjiCodeSend = pvar->ts->KanjiCodeSend = t->val;
		  pvar->config[0] = t->next;
		  free(t);
		}
	      }
	      else if (_stricmp(p, "kr") == 0) {
		if (pvar->config[1] != NULL) {
		  t = pvar->config[1];
		  pvar->cv->KanjiCodeEcho = pvar->ts->KanjiCode = t->val;
		  pvar->config[1] = t->next;
		  free(t);
		}
	      }
	      p = p2;
	    }
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

int PASCAL TTXrecv(SOCKET s, char *buff, int len, int flags) {
  int rlen;

  if ((rlen = pvar->origPrecv(s, buff, len, flags)) > 0) {
    ParseInputStr(buff, rlen);
  }
  return rlen;
}

BOOL PASCAL TTXReadFile(HANDLE fh, LPVOID buff, DWORD len, LPDWORD rbytes, LPOVERLAPPED rol) {
  BOOL result;

  if ((result = pvar->origPReadFile(fh, buff, len, rbytes, rol)) != FALSE)
    ParseInputStr(buff, *rbytes);
  return result;
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
  pvar->origPReadFile = *hooks->PReadFile;
  *hooks->PReadFile = TTXReadFile;
}

static void PASCAL TTXCloseFile(TTXFileHooks *hooks) {
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
