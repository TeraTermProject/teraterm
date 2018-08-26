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

#if defined(TTPCMN_DLL)
#if !defined(DllExport)
#define DllExport __declspec(dllexport)
#endif
#elif defined(TTPCMN_IMPORT)
#if !defined(DllExport)
#define DllExport __declspec(dllimport)
#else
#endif
#else
#undef DllExport
#define DllExport	// direct link
#endif

DllExport int WINAPI DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc);
DllExport int WINAPI CheckComPort(WORD ComPort);
DllExport void WINAPI CopyShmemToTTSet(PTTSet ts);
DllExport void WINAPI CopyTTSetToShmem(PTTSet ts);
DllExport BOOL WINAPI StartTeraTerm(PTTSet ts);
DllExport void WINAPI RestartTeraTerm(HWND hwnd, PTTSet ts);
DllExport void WINAPI ChangeDefaultSet(PTTSet ts, PKeyMap km);
DllExport void WINAPI GetDefaultSet(PTTSet ts);
// void WINAPI LoadDefaultSet(PCHAR SetupFName);
DllExport WORD WINAPI GetKeyCode(PKeyMap KeyMap, WORD Scan);
DllExport void WINAPI GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
								BOOL AppliKeyMode, BOOL AppliCursorMode,
								BOOL Send8BitMode, PCHAR KeyStr,
								int destlen, LPINT Len, LPWORD Type);

DllExport void WINAPI SetCOMFlag(int com);
DllExport void WINAPI ClearCOMFlag(int com);
DllExport int WINAPI CheckCOMFlag(int com);

DllExport int WINAPI RegWin(HWND HWinVT, HWND HWinTEK);
DllExport void WINAPI UnregWin(HWND HWin);
DllExport void WINAPI SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag);
DllExport void WINAPI SetWinList(HWND HWin, HWND HDlg, int IList);
DllExport void WINAPI SelectWin(int WinId);
DllExport void WINAPI SelectNextWin(HWND HWin, int Next, BOOL SkipIconic);
DllExport HWND WINAPI GetNthWin(int n);
DllExport int WINAPI GetRegisteredWindowCount();
DllExport void WINAPI ShowAllWin(int stat);
DllExport void WINAPI ShowAllWinSidebySide(HWND);
DllExport void WINAPI ShowAllWinStacked(HWND);
DllExport void WINAPI ShowAllWinCascade(HWND);
DllExport void WINAPI BroadcastClosingMessage(HWND myhwnd);
DllExport void WINAPI UndoAllWin();
DllExport void WINAPI OpenHelp(UINT Command, DWORD Data, char *UILanguageFile);

DllExport int WINAPI CommReadRawByte(PComVar cv, LPBYTE b);
DllExport int WINAPI CommRead1Byte(PComVar cv, LPBYTE b);
DllExport void WINAPI CommInsert1Byte(PComVar cv, BYTE b);
DllExport int WINAPI CommRawOut(PComVar cv, PCHAR B, int C);
DllExport int WINAPI CommBinaryOut(PComVar cv, PCHAR B, int C);
DllExport int WINAPI CommBinaryBuffOut(PComVar cv, PCHAR B, int C);
DllExport int WINAPI CommTextOut(PComVar cv, PCHAR B, int C);
DllExport int WINAPI CommBinaryEcho(PComVar cv, PCHAR B, int C);
DllExport int WINAPI CommTextEcho(PComVar cv, PCHAR B, int C);

DllExport void WINAPI CreateNotifyIcon(PComVar cv);
DllExport void WINAPI DeleteNotifyIcon(PComVar cv);
DllExport void WINAPI NotifyMessage(PComVar cv, PCHAR message, PCHAR title, DWORD flag);
DllExport void WINAPI ShowNotifyIcon(PComVar cv);
DllExport void WINAPI HideNotifyIcon(PComVar cv);
DllExport void WINAPI SetVerNotifyIcon(PComVar cv, unsigned int ver);
DllExport void WINAPI SetCustomNotifyIcon(HICON icon);
DllExport HICON WINAPI GetCustomNotifyIcon();

#define NotifyInfoMessage(cv, msg, title) NotifyMessage(cv, msg, title, 1)
#define NotifyWarnMessage(cv, msg, title) NotifyMessage(cv, msg, title, 2)
#define NotifyErrorMessage(cv, msg, title) NotifyMessage(cv, msg, title, 3)

#include "../ttpcmn/language.h"

#ifdef __cplusplus
}
#endif

