/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2018 TeraTerm Project
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

/* TERATERM.EXE, TTDLG interface */
#include "teraterm.h"
#include "tttypes.h"
#include "ttplug.h" /* TTPLUG */
#include "ttutil.h"
#include "ttdlg.h"

#include "ttdialog.h"
#include "ttwinman.h"

static HMODULE HTTDLG = NULL;
static int TTDLGUseCount = 0;

PSetupTerminal SetupTerminal = _SetupTerminal;
PSetupWin SetupWin = _SetupWin;
PSetupKeyboard SetupKeyboard = _SetupKeyboard;
PSetupSerialPort SetupSerialPort = _SetupSerialPort;
PSetupTCPIP SetupTCPIP = _SetupTCPIP;
PGetHostName GetHostName = _GetHostName;
PChangeDirectory ChangeDirectory = _ChangeDirectory;
PAboutDialog AboutDialog = _AboutDialog;
PChooseFontDlg ChooseFontDlg = _ChooseFontDlg;
PSetupGeneral SetupGeneral = _SetupGeneral;
PWindowWindow WindowWindow = _WindowWindow;
PTTDLGSetUILanguageFile TTDLGSetUILanguageFile = _TTDLGSetUILanguageFile;

#if 0
PSetupTerminal SetupTerminal;
PSetupWin SetupWin;
PSetupKeyboard SetupKeyboard;
PSetupSerialPort SetupSerialPort;
PSetupTCPIP SetupTCPIP;
PGetHostName GetHostName;
PChangeDirectory ChangeDirectory;
PAboutDialog AboutDialog;
PChooseFontDlg ChooseFontDlg;
PSetupGeneral SetupGeneral;
PWindowWindow WindowWindow;
PTTDLGSetUILanguageFile TTDLGSetUILanguageFile;
#endif

static const GetProcAddressList ProcList[] = {
	{ &SetupTerminal, "SetupTerminal", 8 },
	{ &SetupWin, "SetupWin", 8 },
	{ &SetupKeyboard, "SetupKeyboard", 8 },
	{ &SetupSerialPort, "SetupSerialPort", 8 },
	{ &SetupTCPIP, "SetupTCPIP", 8 },
	{ &GetHostName, "GetHostName", 8 },
	{ &ChangeDirectory, "ChangeDirectory", 8 },
	{ &AboutDialog, "AboutDialog", 4 },
	{ &ChooseFontDlg, "ChooseFontDlg", 12 },
	{ &SetupGeneral, "SetupGeneral", 8 },
	{ &WindowWindow, "WindowWindow", 8 },
	{ &TTDLGSetUILanguageFile, "TTDLGSetUILanguageFile", 4 },
};

static void FreeTTDLGCommon()
{
	FreeLibrary(HTTDLG);
	HTTDLG = NULL;

	ClearProcAddressses(ProcList, _countof(ProcList));
}

BOOL LoadTTDLG()
{
	return TRUE;
}

BOOL FreeTTDLG()
{
	return TRUE;
}

#if 0
BOOL LoadTTDLG()
{
	if (TTDLGUseCount == 0) {
		BOOL ret;

		HTTDLG = LoadHomeDLL("TTPDLG.DLL");
		if (HTTDLG==NULL) return FALSE;

		ret = GetProcAddressses(HTTDLG, ProcList, _countof(ProcList));
		if (!ret) {
			FreeTTDLGCommon();
			return FALSE;
		}

		TTDLGSetUILanguageFile(ts.UILanguageFile);
		TTXGetUIHooks(); /* TTPLUG */
	}
	TTDLGUseCount++;
	return TRUE;
}

BOOL FreeTTDLG()
{
	if (TTDLGUseCount == 0) {
		return FALSE;
	}
	TTDLGUseCount--;
	if (TTDLGUseCount == 0) {
		FreeTTDLGCommon();
	}
	return TRUE;
}
#endif

