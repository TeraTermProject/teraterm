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

int WINAPI DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc);
int WINAPI CheckComPort(WORD ComPort);
void WINAPI CopyShmemToTTSet(PTTSet ts);
void WINAPI CopyTTSetToShmem(PTTSet ts);
BOOL WINAPI StartTeraTerm(PTTSet ts);
void WINAPI RestartTeraTerm(HWND hwnd, PTTSet ts);
void WINAPI ChangeDefaultSet(PTTSet ts, PKeyMap km);
void WINAPI GetDefaultSet(PTTSet ts);
// void WINAPI LoadDefaultSet(PCHAR SetupFName);
WORD WINAPI GetKeyCode(PKeyMap KeyMap, WORD Scan);
void WINAPI GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
                          BOOL AppliKeyMode, BOOL AppliCursorMode,
                          BOOL Send8BitMode, PCHAR KeyStr,
                          int destlen, LPINT Len, LPWORD Type);

void WINAPI SetCOMFlag(int com);
void WINAPI ClearCOMFlag(int com);
int WINAPI CheckCOMFlag(int com);

int WINAPI RegWin(HWND HWinVT, HWND HWinTEK);
void WINAPI UnregWin(HWND HWin);
void WINAPI SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag);
void WINAPI SetWinList(HWND HWin, HWND HDlg, int IList);
void WINAPI SelectWin(int WinId);
void WINAPI SelectNextWin(HWND HWin, int Next, BOOL SkipIconic);
HWND WINAPI GetNthWin(int n);
int WINAPI GetRegisteredWindowCount();
void WINAPI ShowAllWin(int stat);
void WINAPI ShowAllWinSidebySide(HWND);
void WINAPI ShowAllWinStacked(HWND);
void WINAPI ShowAllWinCascade(HWND);
void WINAPI BroadcastClosingMessage(HWND myhwnd);
void WINAPI UndoAllWin();
void WINAPI OpenHelp(UINT Command, DWORD Data, char *UILanguageFile);

int WINAPI CommReadRawByte(PComVar cv, LPBYTE b);
int WINAPI CommRead1Byte(PComVar cv, LPBYTE b);
void WINAPI CommInsert1Byte(PComVar cv, BYTE b);
int WINAPI CommRawOut(PComVar cv, PCHAR B, int C);
int WINAPI CommBinaryOut(PComVar cv, PCHAR B, int C);
int WINAPI CommBinaryBuffOut(PComVar cv, PCHAR B, int C);
int WINAPI CommTextOut(PComVar cv, PCHAR B, int C);
int WINAPI CommBinaryEcho(PComVar cv, PCHAR B, int C);
int WINAPI CommTextEcho(PComVar cv, PCHAR B, int C);

void WINAPI CreateNotifyIcon(PComVar cv);
void WINAPI DeleteNotifyIcon(PComVar cv);
void WINAPI NotifyMessage(PComVar cv, PCHAR message, PCHAR title, DWORD flag);
void WINAPI ShowNotifyIcon(PComVar cv);
void WINAPI HideNotifyIcon(PComVar cv);
void WINAPI SetVerNotifyIcon(PComVar cv, unsigned int ver);
void WINAPI SetCustomNotifyIcon(HICON icon);
HICON WINAPI GetCustomNotifyIcon();

#define NotifyInfoMessage(cv, msg, title) NotifyMessage(cv, msg, title, 1)
#define NotifyWarnMessage(cv, msg, title) NotifyMessage(cv, msg, title, 2)
#define NotifyErrorMessage(cv, msg, title) NotifyMessage(cv, msg, title, 3)

WORD WINAPI SJIS2JIS(WORD KCode);
WORD WINAPI SJIS2EUC(WORD KCode);
WORD WINAPI JIS2SJIS(WORD KCode);
BYTE WINAPI RussConv(int cin, int cout, BYTE b);
void WINAPI RussConvStr
  (int cin, int cout, PCHAR Str, int count);

#ifdef __cplusplus
}
#endif

