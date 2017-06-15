/* Tera Term extension mechanism
 *
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) Robert O'Callahan (roc+tt@cs.cmu.edu)
 * (C) 2008-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#ifndef __TTPLUGIN_H
#define __TTPLUGIN_H

#include "teraterm.h"
#include "ttdialog.h"
#include "ttwsk.h"
#include "ttsetup.h"
#include "ttfileio.h"

typedef struct {
  Tclosesocket * Pclosesocket;
  Tconnect * Pconnect;
  Thtonl * Phtonl;
  Thtons * Phtons;
  Tinet_addr * Pinet_addr;
  Tioctlsocket * Pioctlsocket;
  Trecv * Precv;
  Tselect * Pselect;
  Tsend * Psend;
  Tsetsockopt * Psetsockopt;
  Tsocket * Psocket;
  TWSAAsyncSelect * PWSAAsyncSelect;
  TWSAAsyncGetHostByName * PWSAAsyncGetHostByName;
  TWSACancelAsyncRequest * PWSACancelAsyncRequest;
  TWSAGetLastError * PWSAGetLastError;
//  Tgetaddrinfo * Pgetaddrinfo;
  Tfreeaddrinfo * Pfreeaddrinfo;
  TWSAAsyncGetAddrInfo * PWSAAsyncGetAddrInfo;
} TTXSockHooks;

typedef struct {
  TCreateFile * PCreateFile;
  TCloseFile * PCloseFile;
  TReadFile * PReadFile;
  TWriteFile * PWriteFile;
} TTXFileHooks;

typedef struct {
  PReadIniFile * ReadIniFile;
  PWriteIniFile * WriteIniFile;
  PReadKeyboardCnf * ReadKeyboardCnf;
  PCopyHostList * CopyHostList;
  PAddHostToList * AddHostToList;
  PParseParam * ParseParam;
} TTXSetupHooks;

typedef struct {
  PSetupTerminal * SetupTerminal;
  PSetupWin * SetupWin;
  PSetupKeyboard * SetupKeyboard;
  PSetupSerialPort * SetupSerialPort;
  PSetupTCPIP * SetupTCPIP;
  PGetHostName * GetHostName;
  PChangeDirectory * ChangeDirectory;
  PAboutDialog * AboutDialog;
  PChooseFontDlg * ChooseFontDlg;
  PSetupGeneral * SetupGeneral;
  PWindowWindow * WindowWindow;
} TTXUIHooks;

typedef struct {
  int size;
  int loadOrder; /* smaller numbers get loaded first */
  void (PASCAL * TTXInit)(PTTSet ts, PComVar cv); /* called first to last */
  void (PASCAL * TTXGetUIHooks)(TTXUIHooks * UIHooks); /* called first to last */
  void (PASCAL * TTXGetSetupHooks)(TTXSetupHooks * setupHooks); /* called first to last */
  void (PASCAL * TTXOpenTCP)(TTXSockHooks * hooks); /* called first to last */
  void (PASCAL * TTXCloseTCP)(TTXSockHooks * hooks); /* called last to first */
  void (PASCAL * TTXSetWinSize)(int rows, int cols); /* called first to last */
  void (PASCAL * TTXModifyMenu)(HMENU menu); /* called first to last */
  void (PASCAL * TTXModifyPopupMenu)(HMENU menu); /* called first to last */
  int (PASCAL * TTXProcessCommand)(HWND hWin, WORD cmd); /* returns TRUE if handled, called last to first */
  void (PASCAL * TTXEnd)(void); /* called last to first */
  void (PASCAL * TTXSetCommandLine)(PCHAR cmd, int cmdlen, PGetHNRec rec); /* called first to last */
  void (PASCAL * TTXOpenFile)(TTXFileHooks * hooks); /* called first to last */
  void (PASCAL * TTXCloseFile)(TTXFileHooks * hooks); /* called last to first */
} TTXExports;

/* On entry, 'size' is set to the size of the structure and the rest of
   the fields are set to 0 or NULL. Any fields not understood by the extension DLL
   should be left untouched, i.e. NULL. Any NULL functions are assumed to have
   default behaviour, i.e. do nothing.
   This is all for binary compatibility across releases; if the record gets bigger,
   then the extra functions will be NULL for DLLs that don't understand them. */
typedef BOOL (PASCAL * TTXBindProc)(WORD Version, TTXExports * exports);

#endif
