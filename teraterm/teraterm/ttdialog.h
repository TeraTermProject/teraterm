/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTDLG interface */
#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (FAR PASCAL *PSetupTerminal)
  (HWND WndParent, PTTSet ts);
typedef BOOL (FAR PASCAL *PSetupWin)
  (HWND WndParent, PTTSet ts);
typedef BOOL (FAR PASCAL *PSetupKeyboard)
  (HWND WndParent, PTTSet ts);
typedef BOOL (FAR PASCAL *PSetupSerialPort)
  (HWND WndParent, PTTSet ts);
typedef BOOL (FAR PASCAL *PSetupTCPIP)
  (HWND WndParent, PTTSet ts);
typedef BOOL (FAR PASCAL *PGetHostName)
  (HWND WndParent, PGetHNRec GetHNRec);
typedef BOOL (FAR PASCAL *PChangeDirectory)
  (HWND WndParent, PCHAR CurDir);
typedef BOOL (FAR PASCAL *PAboutDialog)
  (HWND WndParent);
typedef BOOL (FAR PASCAL *PChooseFontDlg)
  (HWND WndParent, LPLOGFONT LogFont, PTTSet ts);
typedef BOOL (FAR PASCAL *PSetupGeneral)
  (HWND WndParent, PTTSet ts);
typedef BOOL (FAR PASCAL *PWindowWindow)
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

extern void FAR PASCAL TTDLGSetUILanguageFile(char *file);

/* proto types */
BOOL LoadTTDLG();
BOOL FreeTTDLG();

#ifdef __cplusplus
}
#endif
