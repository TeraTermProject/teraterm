/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2019 TeraTerm Project
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
#include "ttdlg.h"

#include "ttdialog.h"
#include "ttwinman.h"

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

BOOL LoadTTDLG()
{
	SetupTerminal = _SetupTerminal;
	SetupWin = _SetupWin;
	SetupKeyboard = _SetupKeyboard;
	SetupSerialPort = _SetupSerialPort;
	SetupTCPIP = _SetupTCPIP;
	GetHostName = _GetHostName;
	ChangeDirectory = _ChangeDirectory;
	AboutDialog = _AboutDialog;
	ChooseFontDlg = _ChooseFontDlg;
	SetupGeneral = _SetupGeneral;
	WindowWindow = _WindowWindow;
	TTDLGSetUILanguageFile = _TTDLGSetUILanguageFile;

	TTDLGSetUILanguageFile(ts.UILanguageFile);
	TTXGetUIHooks(); /* TTPLUG */

	return TRUE;
}

BOOL FreeTTDLG()
{
	return TRUE;
}

