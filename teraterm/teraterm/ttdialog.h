/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

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
