/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) Robert O'Callahan
 * (C) 2004- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"

#define _CRTDBG_MAP_ALLOC
// #include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <crtdbg.h>
/* for _findXXXX() functions */
#include <io.h>
#include "ttwinman.h"
#include "ttplugin.h"
#include "ttplug.h"
#include "codeconv.h"

#define MAXNUMEXTENSIONS 32
static HANDLE LibHandle[MAXNUMEXTENSIONS];
static int NumExtensions = 0;
static TTXExports * * Extensions;

typedef struct _ExtensionList {
  TTXExports * exports;
  struct _ExtensionList * next;
} ExtensionList;

static int compareOrder(const void * e1, const void * e2) {
  TTXExports * * exports1 = (TTXExports * *)e1;
  TTXExports * * exports2 = (TTXExports * *)e2;

  return (*exports1)->loadOrder - (*exports2)->loadOrder;
}

static void loadExtension(ExtensionList **extensions, wchar_t const *fileName)
{
	DWORD err;
	const wchar_t *sub_message;
	HMODULE hPlugin;

	if (NumExtensions >= MAXNUMEXTENSIONS)
		return;
	hPlugin = LoadLibraryW(fileName);
	if (hPlugin != NULL) {
		TTXBindProc bind = NULL;
#if defined(_MSC_VER)
		if (bind == NULL)
			bind = (TTXBindProc)GetProcAddress(hPlugin, "_TTXBind@8");
#else
		if (bind == NULL)
			bind = (TTXBindProc)GetProcAddress(hPlugin, "TTXBind@8");
#endif
		if (bind == NULL)
			bind = (TTXBindProc)GetProcAddress(hPlugin, "TTXBind");
		if (bind != NULL) {
			ExtensionList *newExtension = (ExtensionList *)malloc(sizeof(ExtensionList));

			newExtension->exports = (TTXExports *)malloc(sizeof(TTXExports));
			memset(newExtension->exports, 0, sizeof(TTXExports));
			newExtension->exports->size = sizeof(TTXExports);
			if (bind(TTVERSION, newExtension->exports)) {
				newExtension->next = *extensions;
				*extensions = newExtension;
				LibHandle[NumExtensions] = hPlugin;
				NumExtensions++;
				return;
			}
			else {
				free(newExtension->exports);
				free(newExtension);
			}
		}
		FreeLibrary(LibHandle[NumExtensions]);
	}

	err = GetLastError();
	switch (err) {
		case 31:
			sub_message = L"Unresolved dll entry";
			break;
		case 1114:
			sub_message = L"tls entry exists";
			break;
		case 2:
			sub_message = L"rejected by plugin";
			break;
		case 193:
			sub_message = L"invalid dll image";
			break;
		default:
			sub_message = L"unknown";
			break;
	}
	// 言語ファイルによるメッセージの国際化を行っているが、この時点では設定が
	// まだ読み込まれていない為、メッセージが英語のままとなる。要検討。
	static const TTMessageBoxInfoW info = {"Tera Term", "MSG_TT_ERROR", L"Tera Term: Error", "MSG_LOAD_EXT_ERROR",
										   L"Cannot load extension %s (%d, %s)"};
	TTMessageBoxW(NULL, &info, MB_OK | MB_ICONEXCLAMATION, ts.UILanguageFile, fileName, err, sub_message);
}

void PASCAL TTXInit(PTTSet ts_, PComVar cv_) {
  ExtensionList * extensionList = NULL;
  int i;

  // 環境変数の設定有無に関わらず、TTXを有効にする。
  //if (getenv("TERATERM_EXTENSIONS") != NULL) {
  if (1) {
    char buf[1024];
    struct _finddata_t searchData;
	intptr_t searchHandle;

    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s\\TTX*.DLL", ts_->HomeDir);

    searchHandle = _findfirst(buf, &searchData);
    if (searchHandle != -1L) {
      do {
			wchar_t *bufW;
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s\\%s", ts_->HomeDir, searchData.name);
			bufW = ToWcharA(buf);
			loadExtension(&extensionList, bufW);
			free(bufW);
      } while (_findnext(searchHandle, &searchData)==0);
      _findclose(searchHandle);
    }

    if (NumExtensions==0) return;

    Extensions = (TTXExports * *)malloc(sizeof(TTXExports *)*NumExtensions);
    for (i = 0; i < NumExtensions; i++) {
      ExtensionList * old;

      Extensions[i] = extensionList->exports;
      old = extensionList;
      extensionList = extensionList->next;
      free(old);
    }

    qsort(Extensions, NumExtensions, sizeof(Extensions[0]), compareOrder);

    for (i = 0; i < NumExtensions; i++) {
      if (Extensions[i]->TTXInit != NULL) {
        Extensions[i]->TTXInit(ts_, cv_);
      }
    }
  }
}

static void PASCAL TTXInternalOpenTCP(TTXSockHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXOpenTCP != NULL) {
      Extensions[i]->TTXOpenTCP(hooks);
    }
  }
}

void PASCAL TTXOpenTCP(void)
{
	do {
		static TTXSockHooks SockHooks = {
			&Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,
			&Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,
			&Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,
			&PWSACancelAsyncRequest, &PWSAGetLastError,
			/* &Pgetaddrinfo,*/ &Pfreeaddrinfo, &PWSAAsyncGetAddrInfo
		};
		TTXInternalOpenTCP(&SockHooks);
	} while (0);
}

static void PASCAL TTXInternalCloseTCP(TTXSockHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXCloseTCP != NULL) {
      Extensions[i]->TTXCloseTCP(hooks);
    }
  }
}

void PASCAL TTXCloseTCP(void)
{
	do {
		static TTXSockHooks SockHooks = {
			&Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,
			&Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,
			&Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,
			&PWSACancelAsyncRequest, &PWSAGetLastError,
			/* &Pgetaddrinfo,*/ &Pfreeaddrinfo, &PWSAAsyncGetAddrInfo
		};
		TTXInternalCloseTCP(&SockHooks);
	} while (0);
}

static void PASCAL TTXInternalOpenFile(TTXFileHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXOpenFile != NULL) {
      Extensions[i]->TTXOpenFile(hooks);
    }
  }
}

void PASCAL TTXOpenFile(void)
{
	do {
		static TTXFileHooks FileHooks = {
			&PCreateFile, &PCloseFile, &PReadFile, &PWriteFile
		};
		TTXInternalOpenFile(&FileHooks);
	} while (0);
}

static void PASCAL TTXInternalCloseFile(TTXFileHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXCloseFile != NULL) {
      Extensions[i]->TTXCloseFile(hooks);
    }
  }
}

void PASCAL TTXCloseFile(void)
{
	do {
		static TTXFileHooks FileHooks = {
			&PCreateFile, &PCloseFile, &PReadFile, &PWriteFile
		};
		TTXInternalCloseFile(&FileHooks);
	} while (0);
}

static void PASCAL TTXInternalGetUIHooks(TTXUIHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXGetUIHooks != NULL) {
      Extensions[i]->TTXGetUIHooks(hooks);
    }
  }
}

void PASCAL TTXGetUIHooks(void)
{
	do {
		static TTXUIHooks UIHooks = {
			&SetupTerminal, &SetupWin, &SetupKeyboard, &SetupSerialPort,
			&SetupTCPIP, &GetHostName, &ChangeDirectory, &AboutDialog,
			&ChooseFontDlg, &SetupGeneral, &WindowWindow
		};
		TTXInternalGetUIHooks(&UIHooks);
	} while (0);
}

static void PASCAL TTXInternalGetSetupHooks(TTXSetupHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXGetSetupHooks != NULL) {
      Extensions[i]->TTXGetSetupHooks(hooks);
    }
  }
}

void PASCAL TTXGetSetupHooks(void)
{
	do {
		static TTXSetupHooks SetupHooks = {
			&ReadIniFile, &WriteIniFile, &ReadKeyboardCnf, &CopyHostList,
			&AddHostToList, &ParseParam
		};
		TTXInternalGetSetupHooks(&SetupHooks);
	} while (0);
}

void PASCAL TTXSetWinSize(int rows, int cols) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXSetWinSize != NULL) {
      Extensions[i]->TTXSetWinSize(rows, cols);
    }
  }
}

void PASCAL TTXModifyMenu(HMENU menu) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXModifyMenu != NULL) {
      Extensions[i]->TTXModifyMenu(menu);
    }
  }
}

void PASCAL TTXModifyPopupMenu(HMENU menu) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXModifyPopupMenu != NULL) {
      Extensions[i]->TTXModifyPopupMenu(menu);
    }
  }
}

BOOL PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXProcessCommand != NULL) {
      if (Extensions[i]->TTXProcessCommand(hWin,cmd)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

void PASCAL TTXEnd(void) {
  int i;

  if (NumExtensions==0) return;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i]->TTXEnd != NULL) {
      Extensions[i]->TTXEnd();
    }
  }

  for (i=0; i<NumExtensions; i++)
    FreeLibrary(LibHandle[i]);

  for (i = 0; i < NumExtensions; i++) {
	  free(Extensions[i]);
  }

  free(Extensions);
  NumExtensions = 0;
}

void PASCAL TTXSetCommandLine(PCHAR cmd, int cmdlen, PGetHNRec rec) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i]->TTXSetCommandLine != NULL) {
      Extensions[i]->TTXSetCommandLine(cmd, cmdlen, rec);
    }
  }
}
