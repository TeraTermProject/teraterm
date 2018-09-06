#include "teraterm.h"
#include "tttypes.h"

BOOL WINAPI _SetupTerminal(HWND WndParent, PTTSet ts);
BOOL WINAPI _SetupWin(HWND WndParent, PTTSet ts);
BOOL WINAPI _SetupKeyboard(HWND WndParent, PTTSet ts);
BOOL WINAPI _SetupSerialPort(HWND WndParent, PTTSet ts);
BOOL WINAPI _SetupTCPIP(HWND WndParent, PTTSet ts);
BOOL WINAPI _GetHostName(HWND WndParent, PGetHNRec GetHNRec);
BOOL WINAPI _ChangeDirectory(HWND WndParent, PCHAR CurDir);
BOOL WINAPI _AboutDialog(HWND WndParent);
BOOL WINAPI _ChooseFontDlg(HWND WndParent, LPLOGFONT LogFont, PTTSet ts);
BOOL WINAPI _SetupGeneral(HWND WndParent, PTTSet ts);
BOOL WINAPI _WindowWindow(HWND WndParent, PBOOL Close);
BOOL WINAPI _TTDLGSetUILanguageFile(char *file);
