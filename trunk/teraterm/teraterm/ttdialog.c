/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

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
#define IdWindowWindow	  11

BOOL LoadTTDLG()
{
	BOOL Err;

	if (HTTDLG == NULL) {
		TTDLGUseCount = 0;

		HTTDLG = LoadLibrary("TTPDLG.DLL");
		if (HTTDLG==NULL) return FALSE;

		TTDLGSetUILanguageFile(ts.UILanguageFile);

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

