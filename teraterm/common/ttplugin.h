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
  Tclosesocket FAR * Pclosesocket;
  Tconnect FAR * Pconnect;
  Thtonl FAR * Phtonl;
  Thtons FAR * Phtons;
  Tinet_addr FAR * Pinet_addr;
  Tioctlsocket FAR * Pioctlsocket;
  Trecv FAR * Precv;
  Tselect FAR * Pselect;
  Tsend FAR * Psend;
  Tsetsockopt FAR * Psetsockopt;
  Tsocket FAR * Psocket;
  TWSAAsyncSelect FAR * PWSAAsyncSelect;
  TWSAAsyncGetHostByName FAR * PWSAAsyncGetHostByName;
  TWSACancelAsyncRequest FAR * PWSACancelAsyncRequest;
  TWSAGetLastError FAR * PWSAGetLastError;
} TTXSockHooks;

typedef struct {
  TCreateFile FAR * PCreateFile;
  TCloseFile FAR * PCloseFile;
  TReadFile FAR * PReadFile;
  TWriteFile FAR * PWriteFile;
} TTXFileHooks;

typedef struct {
  PReadIniFile FAR * ReadIniFile;
  PWriteIniFile FAR * WriteIniFile;
  PReadKeyboardCnf FAR * ReadKeyboardCnf;
  PCopyHostList FAR * CopyHostList;
  PAddHostToList FAR * AddHostToList;
  PParseParam FAR * ParseParam;
} TTXSetupHooks;

typedef struct {
  PSetupTerminal FAR * SetupTerminal;
  PSetupWin FAR * SetupWin;
  PSetupKeyboard FAR * SetupKeyboard;
  PSetupSerialPort FAR * SetupSerialPort;
  PSetupTCPIP FAR * SetupTCPIP;
  PGetHostName FAR * GetHostName;
  PChangeDirectory FAR * ChangeDirectory;
  PAboutDialog FAR * AboutDialog;
  PChooseFontDlg FAR * ChooseFontDlg;
  PSetupGeneral FAR * SetupGeneral;
  PWindowWindow FAR * WindowWindow;
} TTXUIHooks;

typedef struct {
  int size;
  int loadOrder; /* smaller numbers get loaded first */
  void (PASCAL FAR * TTXInit)(PTTSet ts, PComVar cv); /* called first to last */
  void (PASCAL FAR * TTXGetUIHooks)(TTXUIHooks FAR * UIHooks); /* called first to last */
  void (PASCAL FAR * TTXGetSetupHooks)(TTXSetupHooks FAR * setupHooks); /* called first to last */
  void (PASCAL FAR * TTXOpenTCP)(TTXSockHooks FAR * hooks); /* called first to last */
  void (PASCAL FAR * TTXCloseTCP)(TTXSockHooks FAR * hooks); /* called last to first */
  void (PASCAL FAR * TTXSetWinSize)(int rows, int cols); /* called first to last */
  void (PASCAL FAR * TTXModifyMenu)(HMENU menu); /* called first to last */
  void (PASCAL FAR * TTXModifyPopupMenu)(HMENU menu); /* called first to last */
  int (PASCAL FAR * TTXProcessCommand)(HWND hWin, WORD cmd); /* returns TRUE if handled, called last to first */
  void (PASCAL FAR * TTXEnd)(void); /* called last to first */
  void (PASCAL FAR * TTXSetCommandLine)(PCHAR cmd, int cmdlen, PGetHNRec rec); /* called first to last */
  void (PASCAL FAR * TTXOpenFile)(TTXFileHooks FAR * hooks); /* called first to last */
  void (PASCAL FAR * TTXCloseFile)(TTXFileHooks FAR * hooks); /* called last to first */
} TTXExports;

/* On entry, 'size' is set to the size of the structure and the rest of
   the fields are set to 0 or NULL. Any fields not understood by the extension DLL
   should be left untouched, i.e. NULL. Any NULL functions are assumed to have
   default behaviour, i.e. do nothing.
   This is all for binary compatibility across releases; if the record gets bigger,
   then the extra functions will be NULL for DLLs that don't understand them. */
typedef BOOL (PASCAL FAR * TTXBindProc)(WORD Version, TTXExports FAR * exports);

#endif
