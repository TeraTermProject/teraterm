/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2020 TeraTerm Project
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

#if !defined(DllExport)
#define DllExport __declspec(dllimport)
#endif

DllExport int PASCAL DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc);
DllExport int PASCAL CheckComPort(WORD ComPort);
DllExport void PASCAL CopyShmemToTTSet(PTTSet ts);
DllExport void PASCAL CopyTTSetToShmem(PTTSet ts);
DllExport BOOL PASCAL StartTeraTerm(PTTSet ts);
DllExport void PASCAL RestartTeraTerm(HWND hwnd, PTTSet ts);
DllExport void PASCAL ChangeDefaultSet(PTTSet ts, PKeyMap km);
DllExport void PASCAL GetDefaultSet(PTTSet ts);
// void PASCAL LoadDefaultSet(PCHAR SetupFName);
DllExport WORD PASCAL GetKeyCode(PKeyMap KeyMap, WORD Scan);
DllExport void PASCAL GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
                          BOOL AppliKeyMode, BOOL AppliCursorMode,
                          BOOL Send8BitMode, PCHAR KeyStr,
                          int destlen, LPINT Len, LPWORD Type);

DllExport void PASCAL SetCOMFlag(int com);
DllExport void PASCAL ClearCOMFlag(int com);
DllExport int PASCAL CheckCOMFlag(int com);

DllExport int PASCAL RegWin(HWND HWinVT, HWND HWinTEK);
DllExport void PASCAL UnregWin(HWND HWin);
DllExport void PASCAL SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag);
DllExport void PASCAL SetWinList(HWND HWin, HWND HDlg, int IList);
DllExport void PASCAL SelectWin(int WinId);
DllExport void PASCAL SelectNextWin(HWND HWin, int Next, BOOL SkipIconic);
DllExport HWND PASCAL GetNthWin(int n);
DllExport int PASCAL GetRegisteredWindowCount(void);
DllExport void PASCAL ShowAllWin(int stat);
DllExport void PASCAL ShowAllWinSidebySide(HWND);
DllExport void PASCAL ShowAllWinStacked(HWND);
DllExport void PASCAL ShowAllWinCascade(HWND);
DllExport void PASCAL BroadcastClosingMessage(HWND myhwnd);
DllExport void PASCAL UndoAllWin(void);
DllExport void PASCAL OpenHelp(UINT Command, DWORD Data, char *UILanguageFile);

DllExport int PASCAL CommReadRawByte(PComVar cv, LPBYTE b);
DllExport int PASCAL CommRead1Byte(PComVar cv, LPBYTE b);
DllExport void PASCAL CommInsert1Byte(PComVar cv, BYTE b);
DllExport int PASCAL CommRawOut(PComVar cv, PCHAR B, int C);
DllExport int PASCAL CommBinaryOut(PComVar cv, PCHAR B, int C);
DllExport int PASCAL CommBinaryBuffOut(PComVar cv, PCHAR B, int C);
DllExport int PASCAL CommTextOut(PComVar cv, PCHAR B, int C);
DllExport int PASCAL CommTextOutW(PComVar cv, const wchar_t *B, int C);
DllExport int PASCAL CommBinaryEcho(PComVar cv, PCHAR B, int C);
DllExport int PASCAL CommTextEcho(PComVar cv, PCHAR B, int C);
DllExport int PASCAL CommTextEchoW(PComVar cv, const wchar_t *B, int C);

DllExport void PASCAL CreateNotifyIcon(PComVar cv);
DllExport void PASCAL DeleteNotifyIcon(PComVar cv);
DllExport void PASCAL NotifyMessageW(PComVar cv, const wchar_t *message, const wchar_t *title, DWORD flag);
DllExport void PASCAL NotifyMessage(PComVar cv, const char *message, const char *title, DWORD flag);
DllExport void PASCAL ShowNotifyIcon(PComVar cv);
DllExport void PASCAL HideNotifyIcon(PComVar cv);
DllExport void PASCAL SetVerNotifyIcon(PComVar cv, unsigned int ver);
DllExport void PASCAL SetCustomNotifyIcon(HICON icon);
DllExport HICON PASCAL GetCustomNotifyIcon(void);

#define NotifyInfoMessage(cv, msg, title) NotifyMessage(cv, msg, title, 1)
#define NotifyWarnMessage(cv, msg, title) NotifyMessage(cv, msg, title, 2)
#define NotifyErrorMessage(cv, msg, title) NotifyMessage(cv, msg, title, 3)
#define NotifyInfoMessageW(cv, msg, title) NotifyMessageW(cv, msg, title, 1)
#define NotifyWarnMessageW(cv, msg, title) NotifyMessageW(cv, msg, title, 2)
#define NotifyErrorMessageW(cv, msg, title) NotifyMessageW(cv, msg, title, 3)

#include "../ttpcmn/language.h"

#ifdef __cplusplus
}
#endif

