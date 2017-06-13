/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTDLG interface */
#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (PASCAL *PSetupTerminal)
  (HWND WndParent, PTTSet ts);
typedef BOOL (PASCAL *PSetupWin)
  (HWND WndParent, PTTSet ts);
typedef BOOL (PASCAL *PSetupKeyboard)
  (HWND WndParent, PTTSet ts);
typedef BOOL (PASCAL *PSetupSerialPort)
  (HWND WndParent, PTTSet ts);
typedef BOOL (PASCAL *PSetupTCPIP)
  (HWND WndParent, PTTSet ts);
typedef BOOL (PASCAL *PGetHostName)
  (HWND WndParent, PGetHNRec GetHNRec);
typedef BOOL (PASCAL *PChangeDirectory)
  (HWND WndParent, PCHAR CurDir);
typedef BOOL (PASCAL *PAboutDialog)
  (HWND WndParent);
typedef BOOL (PASCAL *PChooseFontDlg)
  (HWND WndParent, LPLOGFONT LogFont, PTTSet ts);
typedef BOOL (PASCAL *PSetupGeneral)
  (HWND WndParent, PTTSet ts);
typedef BOOL (PASCAL *PWindowWindow)
  (HWND WndParent, PBOOL Close);

extern PSetupTerminal SetupTerminal;
extern PSetupWin SetupWin;
extern PSetupKeyboard SetupKeyboard;
extern PSetupSerialPort SetupSerialPort;
extern PSetupTCPIP SetupTCPIP;
extern PGetHostName GetHostName;
extern PChangeDirectory ChangeDirectory;
extern PAboutDialog AboutDialog;
extern PChooseFontDlg ChooseFontDlg;
extern PSetupGeneral SetupGeneral;
extern PWindowWindow WindowWindow;

extern void PASCAL TTDLGSetUILanguageFile(char *file);

/* proto types */
BOOL LoadTTDLG();
BOOL FreeTTDLG();

#ifdef __cplusplus
}
#endif
