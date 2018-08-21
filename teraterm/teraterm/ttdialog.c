/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
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

#include "ttdialog.h"
#include "ttwinman.h"

static HMODULE HTTDLG = NULL;
static TTDLGUseCount = 0;

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

#define IdSetupTerminal   1
#define IdSetupWin        2
#define IdSetupKeyboard   3
#define IdSetupSerialPort 4
#define IdSetupTCPIP      5
#define IdGetHostName     6
#define IdChangeDirectory 7
#define IdAboutDialog     8
#define IdChooseFontDlg   9
#define IdSetupGeneral    10
#define IdWindowWindow    11
#define IdTTDLGSetUILanguageFile  12

BOOL LoadTTDLG()
{
	BOOL Err;

	if (HTTDLG == NULL) {
		TTDLGUseCount = 0;

		HTTDLG = LoadHomeDLL("TTPDLG.DLL");
		if (HTTDLG==NULL) return FALSE;

		Err = FALSE;

		SetupTerminal = (PSetupTerminal)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdSetupTerminal));
		if (SetupTerminal==NULL) {
			Err = TRUE;
		}

		SetupWin = (PSetupWin)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdSetupWin));
		if (SetupWin==NULL) {
			Err = TRUE;
		}

		SetupKeyboard = (PSetupKeyboard)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdSetupKeyboard));
		if (SetupKeyboard==NULL) {
			Err = TRUE;
		}

		SetupSerialPort = (PSetupSerialPort)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdSetupSerialPort));
		if (SetupSerialPort==NULL) {
			Err = TRUE;
		}

		SetupTCPIP = (PSetupTCPIP)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdSetupTCPIP));
		if (SetupTCPIP==NULL) {
			Err = TRUE;
		}

		GetHostName = (PGetHostName)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdGetHostName));
		if (GetHostName==NULL) {
			Err = TRUE;
		}

		ChangeDirectory = (PChangeDirectory)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdChangeDirectory));
		if (ChangeDirectory==NULL) {
			Err = TRUE;
		}

		AboutDialog = (PAboutDialog)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdAboutDialog));
		if (AboutDialog==NULL) {
			Err = TRUE;
		}

		ChooseFontDlg = (PChooseFontDlg)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdChooseFontDlg));
		if (ChooseFontDlg==NULL) {
			Err = TRUE;
		}

		SetupGeneral = (PSetupGeneral)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdSetupGeneral));
		if (SetupGeneral==NULL) {
			Err = TRUE;
		}

		WindowWindow = (PWindowWindow)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdWindowWindow));
		if (WindowWindow==NULL) {
			Err = TRUE;
		}

		TTDLGSetUILanguageFile = (PTTDLGSetUILanguageFile)GetProcAddress(HTTDLG,
		  MAKEINTRESOURCE(IdTTDLGSetUILanguageFile));
		if (TTDLGSetUILanguageFile==NULL) {
			Err = TRUE;
		}
		else {
			TTDLGSetUILanguageFile(ts.UILanguageFile);
		}

		if (Err) {
			FreeLibrary(HTTDLG);
			HTTDLG = NULL;
			return FALSE;
		}

		TTXGetUIHooks(); /* TTPLUG */
	}
	TTDLGUseCount++;
	return TRUE;
}

BOOL FreeTTDLG()
{
	if (TTDLGUseCount==0) {
		return FALSE;
	}
	TTDLGUseCount--;
	if (TTDLGUseCount>0) {
		return TRUE;
	}
	if (HTTDLG!=NULL) {
		FreeLibrary(HTTDLG);
		HTTDLG = NULL;
	}
	return FALSE;
}

