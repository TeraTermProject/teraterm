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

/* TERATERM.EXE, TTSET interface */
#include "teraterm.h"
#include "tttypes.h"

#include "ttsetup.h"
#include "ttplug.h" /* TTPLUG */

#include "ttwinman.h"

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
  HTTSET = LoadHomeDLL("TTPSET.DLL");
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
