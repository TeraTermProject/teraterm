/* Tera Term extension mechanism
   Robert O'Callahan (roc+tt@cs.cmu.edu)
   
   Tera Term by Takashi Teranishi (teranishi@rikaxp.riken.go.jp)
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
