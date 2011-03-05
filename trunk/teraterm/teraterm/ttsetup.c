/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTSET interface */
#include "teraterm.h"
#include "tttypes.h"

#include "ttsetup.h"
#include "ttplug.h" /* TTPLUG */

PReadIniFile ReadIniFile;
PWriteIniFile WriteIniFile;
PReadKeyboardCnf ReadKeyboardCnf;
PCopyHostList CopyHostList;
PAddHostToList AddHostToList;
PParseParam ParseParam;
PCopySerialList CopySerialList;
PAddValueToList AddValueToList;

static HANDLE HTTSET = NULL;

#define IdReadIniFile     1
#define IdWriteIniFile    2
#define IdReadKeyboardCnf 3
#define IdCopyHostList    4
#define IdAddHostToList   5
#define IdParseParam      6
#define IdCopySerialList  7
#define IdAddValueToList  8

BOOL LoadTTSET()
{
  BOOL Err;

  if (HTTSET != NULL) return TRUE;
  HTTSET = LoadLibrary("TTPSET.DLL");
  if (HTTSET == NULL) return FALSE;

  Err = FALSE;
  ReadIniFile =
	(PReadIniFile)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdReadIniFile));
  if (ReadIniFile==NULL) Err = TRUE;

  WriteIniFile =
	(PWriteIniFile)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdWriteIniFile));
  if (WriteIniFile==NULL) Err = TRUE;

  ReadKeyboardCnf =
	(PReadKeyboardCnf)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdReadKeyboardCnf));
  if (ReadKeyboardCnf==NULL) Err = TRUE;

  CopyHostList =
	(PCopyHostList)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdCopyHostList));
  if (CopyHostList==NULL) Err = TRUE;

  AddHostToList =
	(PAddHostToList)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdAddHostToList));
  if (AddHostToList==NULL) Err = TRUE;

  ParseParam =
	(PParseParam)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdParseParam));
  if (ParseParam==NULL) Err = TRUE;

  CopySerialList =
	(PCopySerialList)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdCopySerialList));
  if (CopySerialList==NULL) Err = TRUE;

  AddValueToList =
	(PAddValueToList)GetProcAddress(HTTSET, MAKEINTRESOURCE(IdAddValueToList));
  if (AddValueToList==NULL) Err = TRUE;

  if (Err)
  {
    FreeLibrary(HTTSET);
    HTTSET = NULL;
    return FALSE;
  }
  TTXGetSetupHooks(); /* TTPLUG */
  return TRUE;
}

void FreeTTSET()
{
  if (HTTSET != NULL)
  {
    FreeLibrary(HTTSET);
    HTTSET = NULL;
  }
}
