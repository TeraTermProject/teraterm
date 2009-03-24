#ifndef __TTPLUG_H
#define __TTPLUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ttplugin.h"

/* This function initializes the extensions and is called at the beginning
   of the program. */
void PASCAL FAR TTXInit(PTTSet ts, PComVar cv);

/* This function is called when a TCP connection is about to be opened.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL FAR TTXInternalOpenTCP(TTXSockHooks FAR * hooks);
#define TTXOpenTCP()                                             \
  do {                                                           \
    static TTXSockHooks SockHooks = {                            \
      &Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,   \
      &Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,     \
      &Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,       \
      &PWSACancelAsyncRequest, &PWSAGetLastError                 \
    };                                                           \
    TTXInternalOpenTCP(&SockHooks);                              \
  } while (0)

/* This function is called when a TCP connection has been closed.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL FAR TTXInternalCloseTCP(TTXSockHooks FAR * hooks);
#define TTXCloseTCP()                                            \
  do {                                                           \
    static TTXSockHooks SockHooks = {                            \
      &Pclosesocket, &Pconnect, &Phtonl, &Phtons, &Pinet_addr,   \
      &Pioctlsocket, &Precv, &Pselect, &Psend, &Psetsockopt,     \
      &Psocket, &PWSAAsyncSelect, &PWSAAsyncGetHostByName,       \
      &PWSACancelAsyncRequest, &PWSAGetLastError                 \
    };                                                           \
    TTXInternalCloseTCP(&SockHooks);                             \
  } while (0)

void PASCAL FAR TTXInternalOpenFile(TTXFileHooks FAR * hooks);
#define TTXOpenFile()                                            \
  do {                                                           \
    static TTXFileHooks FileHooks = {                            \
      &PCreateFile, &PCloseFile, &PReadFile, &PWriteFile         \
    };                                                           \
    TTXInternalOpenFile(&FileHooks);                             \
  } while (0)

void PASCAL FAR TTXInternalCloseFile(TTXFileHooks FAR * hooks);
#define TTXCloseFile()                                           \
  do {                                                           \
    static TTXFileHooks FileHooks = {                            \
      &PCreateFile, &PCloseFile, &PReadFile, &PWriteFile         \
    };                                                           \
    TTXInternalCloseFile(&FileHooks);                            \
  } while (0)

/* This function is called after the TTDLG DLL has been loaded.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL FAR TTXInternalGetUIHooks(TTXUIHooks FAR * hooks);
#define TTXGetUIHooks()                                            \
  do {                                                             \
    static TTXUIHooks UIHooks = {                                  \
      &SetupTerminal, &SetupWin, &SetupKeyboard, &SetupSerialPort, \
      &SetupTCPIP, &GetHostName, &ChangeDirectory, &AboutDialog,   \
      &ChooseFontDlg, &SetupGeneral, &WindowWindow                 \
    };                                                             \
    TTXInternalGetUIHooks(&UIHooks);                               \
  } while (0)

/* This function is called after the TTSET DLL has been loaded.
   This macro stuff is to make sure that the functions in the caller's
   EXE or DLL are hooked. */
void PASCAL FAR TTXInternalGetSetupHooks(TTXSetupHooks FAR * hooks);
#define TTXGetSetupHooks()                                            \
  do {                                                                \
    static TTXSetupHooks SetupHooks = {                               \
      &ReadIniFile, &WriteIniFile, &ReadKeyboardCnf, &CopyHostList,   \
      &AddHostToList, &ParseParam                                     \
    };                                                                \
    TTXInternalGetSetupHooks(&SetupHooks);                            \
  } while (0)

/* This function is called when the window size has changed. */
void PASCAL FAR TTXSetWinSize(int rows, int cols);

/* This function adds the extensions' entries to the menu, which is the
   handle for the program's menubar. */
void PASCAL FAR TTXModifyMenu(HMENU menu);

/* This function is called when a popup menu is about to be displayed.
   The status of the entries is set appropriately. */
void PASCAL FAR TTXModifyPopupMenu(HMENU menu);

/* This function calls on the extensions to handle a command. It returns
   TRUE if they handle it, otherwise FALSE. */
BOOL PASCAL FAR TTXProcessCommand(HWND hWin, WORD cmd);

/* This function is called to see whether Telnet mode can be turned on when
   Tera Term thinks it has detected a telnetd */
void PASCAL FAR TTXEnd(void);

/* This function is called when a new Tera Term is being started with certain
   settings and the extension may wish to add some options to the command line */
void PASCAL FAR TTXSetCommandLine(PCHAR cmd, int cmdlen, PGetHNRec rec);
#ifdef __cplusplus
}
#endif

#endif
