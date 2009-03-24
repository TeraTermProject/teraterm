/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTCMN interface */

/* proto types */
#ifdef __cplusplus
extern "C" {
#endif

int PASCAL DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc);
void PASCAL CopyShmemToTTSet(PTTSet ts);
void PASCAL CopyTTSetToShmem(PTTSet ts);
BOOL FAR PASCAL StartTeraTerm(PTTSet ts);
void FAR PASCAL ChangeDefaultSet(PTTSet ts, PKeyMap km);
void FAR PASCAL GetDefaultSet(PTTSet ts);
// void FAR PASCAL LoadDefaultSet(PCHAR SetupFName);
WORD FAR PASCAL GetKeyCode(PKeyMap KeyMap, WORD Scan);
void FAR PASCAL GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
                          BOOL AppliKeyMode, BOOL AppliCursorMode,
                          BOOL Send8BitMode, PCHAR KeyStr,
                          int destlen, LPINT Len, LPWORD Type);

void FAR PASCAL SetCOMFlag(int com);
void FAR PASCAL ClearCOMFlag(int com);
int FAR PASCAL CheckCOMFlag(int com);

int FAR PASCAL RegWin(HWND HWinVT, HWND HWinTEK);
void FAR PASCAL UnregWin(HWND HWin);
void FAR PASCAL SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag);
void FAR PASCAL SetWinList(HWND HWin, HWND HDlg, int IList);
void FAR PASCAL SelectWin(int WinId);
void FAR PASCAL SelectNextWin(HWND HWin, int Next);
HWND FAR PASCAL GetNthWin(int n);

int FAR PASCAL CommReadRawByte(PComVar cv, LPBYTE b);
int FAR PASCAL CommRead1Byte(PComVar cv, LPBYTE b);
void FAR PASCAL CommInsert1Byte(PComVar cv, BYTE b);
int FAR PASCAL CommRawOut(PComVar cv, PCHAR B, int C);
int FAR PASCAL CommBinaryOut(PComVar cv, PCHAR B, int C);
int FAR PASCAL CommTextOut(PComVar cv, PCHAR B, int C);
int FAR PASCAL CommBinaryEcho(PComVar cv, PCHAR B, int C);
int FAR PASCAL CommTextEcho(PComVar cv, PCHAR B, int C);

WORD FAR PASCAL SJIS2JIS(WORD KCode);
WORD FAR PASCAL SJIS2EUC(WORD KCode);
WORD FAR PASCAL JIS2SJIS(WORD KCode);
BYTE FAR PASCAL RussConv(int cin, int cout, BYTE b);
void FAR PASCAL RussConvStr
  (int cin, int cout, PCHAR Str, int count);

#ifdef __cplusplus
}
#endif

