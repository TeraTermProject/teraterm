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
#include <winsock2.h>
#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <crtdbg.h>
#include "ttwinman.h"
#include "ttplugin.h"
#include "codeconv.h"
#include "asprintf.h"

#include "ttplug.h"

static int NumExtensions = 0;

typedef struct _ExtensionList {
	TTXExports * exports;
	HANDLE LibHandle;
} ExtensionList;

static ExtensionList *Extensions;

static int compareOrder(const void * e1, const void * e2) {
  TTXExports * * exports1 = (TTXExports * *)e1;
  TTXExports * * exports2 = (TTXExports * *)e2;

  return (*exports1)->loadOrder - (*exports2)->loadOrder;
}

static void loadExtension(wchar_t const *fileName)
{
	DWORD err;
	const wchar_t *sub_message;
	HMODULE hPlugin;

	hPlugin = LoadLibraryW(fileName);
	if (hPlugin != NULL) {
		TTXBindProc bind = NULL;
		FARPROC *pbind = (FARPROC *)&bind;
#if defined(_MSC_VER)
		if (bind == NULL)
			*pbind = GetProcAddress(hPlugin, "_TTXBind@8");
#else
		if (bind == NULL)
			*pbind = GetProcAddress(hPlugin, "TTXBind@8");
#endif
		if (bind == NULL)
			*pbind = GetProcAddress(hPlugin, "TTXBind");
		if (bind != NULL) {
			TTXExports * exports = (TTXExports *)malloc(sizeof(TTXExports));
			if (exports == NULL) {
				return;
			}
			memset(exports, 0, sizeof(TTXExports));
			exports->size = sizeof(TTXExports);

			if (bind(TTVERSION, exports)) {
				ExtensionList *newExtension;
				ExtensionList *extensions = (ExtensionList *)realloc(Extensions, sizeof(ExtensionList) * (NumExtensions + 1));
				if (extensions == NULL) {
					free(exports);
					FreeLibrary(hPlugin);
					return;
				}
				Extensions = extensions;
				newExtension = &extensions[NumExtensions];
				NumExtensions++;

				newExtension->exports = exports;
				newExtension->LibHandle = hPlugin;
				return;
			}
			else {
				free(exports);
			}
		}
		FreeLibrary(hPlugin);
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

void PASCAL TTXInit(PTTSet ts_, PComVar cv_)
{
	int i;
	wchar_t *load_mask;
	WIN32_FIND_DATAW fd;
	HANDLE hFind;
	wchar_t *HomeDirW = ToWcharA(ts_->HomeDir);

	aswprintf(&load_mask, L"%s\\TTX*.DLL", HomeDirW);

	hFind = FindFirstFileW(load_mask, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			wchar_t *filename;
			aswprintf(&filename, L"%s\\%s", HomeDirW, fd.cFileName);
			loadExtension(filename);
			free(filename);
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	free(load_mask);
	free(HomeDirW);

	if (NumExtensions==0) return;

	qsort(Extensions, NumExtensions, sizeof(Extensions[0]), compareOrder);

	for (i = 0; i < NumExtensions; i++) {
		if (Extensions[i].exports->TTXInit != NULL) {
			Extensions[i].exports->TTXInit(ts_, cv_);
		}
	}
}

static void PASCAL TTXInternalOpenTCP(TTXSockHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXOpenTCP != NULL) {
      Extensions[i].exports->TTXOpenTCP(hooks);
    }
  }
}

void PASCAL TTXOpenTCP(void)
{
	static TTXSockHooks SockHooks = {
		&Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,
		&Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,
		&Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,
		&PWSACancelAsyncRequest, &PWSAGetLastError,
		/* &Pgetaddrinfo,*/ &Pfreeaddrinfo, &PWSAAsyncGetAddrInfo
	};
	TTXInternalOpenTCP(&SockHooks);
}

static void PASCAL TTXInternalCloseTCP(TTXSockHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i].exports->TTXCloseTCP != NULL) {
      Extensions[i].exports->TTXCloseTCP(hooks);
    }
  }
}

void PASCAL TTXCloseTCP(void)
{
	static TTXSockHooks SockHooks = {
		&Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,
		&Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,
		&Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,
		&PWSACancelAsyncRequest, &PWSAGetLastError,
		/* &Pgetaddrinfo,*/ &Pfreeaddrinfo, &PWSAAsyncGetAddrInfo
	};
	TTXInternalCloseTCP(&SockHooks);
}

static void PASCAL TTXInternalOpenFile(TTXFileHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXOpenFile != NULL) {
      Extensions[i].exports->TTXOpenFile(hooks);
    }
  }
}

void PASCAL TTXOpenFile(void)
{
	static TTXFileHooks FileHooks = {
		&PCreateFile, &PCloseFile, &PReadFile, &PWriteFile
	};
	TTXInternalOpenFile(&FileHooks);
}

static void PASCAL TTXInternalCloseFile(TTXFileHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i].exports->TTXCloseFile != NULL) {
      Extensions[i].exports->TTXCloseFile(hooks);
    }
  }
}

void PASCAL TTXCloseFile(void)
{
	static TTXFileHooks FileHooks = {
		&PCreateFile, &PCloseFile, &PReadFile, &PWriteFile
	};
	TTXInternalCloseFile(&FileHooks);
}

static void PASCAL TTXInternalGetUIHooks(TTXUIHooks * hooks) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXGetUIHooks != NULL) {
      Extensions[i].exports->TTXGetUIHooks(hooks);
    }
  }
}

void PASCAL TTXGetUIHooks(void)
{
	static TTXUIHooks UIHooks = {
		&SetupTerminal, &SetupWin, &SetupKeyboard, &SetupSerialPort,
		&SetupTCPIP, &GetHostName, &ChangeDirectory, &AboutDialog,
		&ChooseFontDlg, &SetupGeneral, &WindowWindow
	};
	TTXInternalGetUIHooks(&UIHooks);
}

static void PASCAL TTXInternalGetSetupHooks(TTXSetupHooks * hooks) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i].exports->TTXGetSetupHooks != NULL) {
      Extensions[i].exports->TTXGetSetupHooks(hooks);
    }
  }
}

void PASCAL TTXGetSetupHooks(void)
{
	static TTXSetupHooks SetupHooks = {
		&ReadIniFile, &WriteIniFile, &ReadKeyboardCnf, &CopyHostList,
		&AddHostToList, &ParseParam
	};
	TTXInternalGetSetupHooks(&SetupHooks);
}

void PASCAL TTXSetWinSize(int rows, int cols) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXSetWinSize != NULL) {
      Extensions[i].exports->TTXSetWinSize(rows, cols);
    }
  }
}

void PASCAL TTXModifyMenu(HMENU menu) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXModifyMenu != NULL) {
      Extensions[i].exports->TTXModifyMenu(menu);
    }
  }
}

void PASCAL TTXModifyPopupMenu(HMENU menu) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXModifyPopupMenu != NULL) {
      Extensions[i].exports->TTXModifyPopupMenu(menu);
    }
  }
}

BOOL PASCAL TTXProcessCommand(HWND hWin, WORD cmd) {
  int i;

  for (i = NumExtensions - 1; i >= 0; i--) {
    if (Extensions[i].exports->TTXProcessCommand != NULL) {
      if (Extensions[i].exports->TTXProcessCommand(hWin,cmd)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

void PASCAL TTXEnd(void)
{
	int i;

	if (NumExtensions == 0)
		return;

	for (i = NumExtensions - 1; i >= 0; i--) {
		if (Extensions[i].exports->TTXEnd != NULL) {
			Extensions[i].exports->TTXEnd();
		}
	}

	for (i = 0; i < NumExtensions; i++) {
		free(Extensions[i].exports);
		FreeLibrary(Extensions[i].LibHandle);
	}

	free(Extensions);
	Extensions = NULL;
	NumExtensions = 0;
}

void PASCAL TTXSetCommandLine(PCHAR cmd, int cmdlen, PGetHNRec rec) {
  int i;

  for (i = 0; i < NumExtensions; i++) {
    if (Extensions[i].exports->TTXSetCommandLine != NULL) {
      Extensions[i].exports->TTXSetCommandLine(cmd, cmdlen, rec);
    }
  }
}
