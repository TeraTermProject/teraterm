/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2017 TeraTerm Project
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

/* TERATERM.EXE, TTCMN interface */

/* proto types */
#ifdef __cplusplus
extern "C" {
#endif

int PASCAL DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc);
int PASCAL CheckComPort(WORD ComPort);
void PASCAL CopyShmemToTTSet(PTTSet ts);
void PASCAL CopyTTSetToShmem(PTTSet ts);
BOOL PASCAL StartTeraTerm(PTTSet ts);
void PASCAL RestartTeraTerm(HWND hwnd, PTTSet ts);
void PASCAL ChangeDefaultSet(PTTSet ts, PKeyMap km);
void PASCAL GetDefaultSet(PTTSet ts);
// void PASCAL LoadDefaultSet(PCHAR SetupFName);
WORD PASCAL GetKeyCode(PKeyMap KeyMap, WORD Scan);
void PASCAL GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
                          BOOL AppliKeyMode, BOOL AppliCursorMode,
                          BOOL Send8BitMode, PCHAR KeyStr,
                          int destlen, LPINT Len, LPWORD Type);

void PASCAL SetCOMFlag(int com);
void PASCAL ClearCOMFlag(int com);
int PASCAL CheckCOMFlag(int com);

int PASCAL RegWin(HWND HWinVT, HWND HWinTEK);
void PASCAL UnregWin(HWND HWin);
void PASCAL SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag);
void PASCAL SetWinList(HWND HWin, HWND HDlg, int IList);
void PASCAL SelectWin(int WinId);
void PASCAL SelectNextWin(HWND HWin, int Next, BOOL SkipIconic);
HWND PASCAL GetNthWin(int n);
int PASCAL GetRegisteredWindowCount();
void PASCAL ShowAllWin(int stat);
void PASCAL ShowAllWinSidebySide(HWND);
void PASCAL ShowAllWinStacked(HWND);
void PASCAL ShowAllWinCascade(HWND);
void PASCAL BroadcastClosingMessage(HWND myhwnd);
void PASCAL UndoAllWin();
void PASCAL OpenHelp(UINT Command, DWORD Data, char *UILanguageFile);

int PASCAL CommReadRawByte(PComVar cv, LPBYTE b);
int PASCAL CommRead1Byte(PComVar cv, LPBYTE b);
void PASCAL CommInsert1Byte(PComVar cv, BYTE b);
int PASCAL CommRawOut(PComVar cv, PCHAR B, int C);
int PASCAL CommBinaryOut(PComVar cv, PCHAR B, int C);
int PASCAL CommBinaryBuffOut(PComVar cv, PCHAR B, int C);
int PASCAL CommTextOut(PComVar cv, PCHAR B, int C);
int PASCAL CommTextOutW(PComVar cv, const wchar_t *B, int C);
int PASCAL CommBinaryEcho(PComVar cv, PCHAR B, int C);
int PASCAL CommTextEcho(PComVar cv, PCHAR B, int C);
int PASCAL CommTextEchoW(PComVar cv, const wchar_t *B, int C);

void PASCAL CreateNotifyIcon(PComVar cv);
void PASCAL DeleteNotifyIcon(PComVar cv);
void PASCAL NotifyMessage(PComVar cv, PCHAR message, PCHAR title, DWORD flag);
void PASCAL ShowNotifyIcon(PComVar cv);
void PASCAL HideNotifyIcon(PComVar cv);
void PASCAL SetVerNotifyIcon(PComVar cv, unsigned int ver);
void PASCAL SetCustomNotifyIcon(HICON icon);
HICON PASCAL GetCustomNotifyIcon();

#define NotifyInfoMessage(cv, msg, title) NotifyMessage(cv, msg, title, 1)
#define NotifyWarnMessage(cv, msg, title) NotifyMessage(cv, msg, title, 2)
#define NotifyErrorMessage(cv, msg, title) NotifyMessage(cv, msg, title, 3)

WORD PASCAL SJIS2JIS(WORD KCode);
WORD PASCAL SJIS2EUC(WORD KCode);
WORD PASCAL JIS2SJIS(WORD KCode);
BYTE PASCAL RussConv(int cin, int cout, BYTE b);
void PASCAL RussConvStr
  (int cin, int cout, PCHAR Str, int count);

#ifdef __cplusplus
}
#endif

