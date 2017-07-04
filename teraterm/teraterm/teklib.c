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

/* TERATERM.EXE, TTTEK.DLL interface */

#include "teraterm.h"
#include "tttypes.h"
#include "tektypes.h"
#include "ttwinman.h"

#include "teklib.h"

PTEKInit TEKInit;
PTEKResizeWindow TEKResizeWindow;
PTEKChangeCaret TEKChangeCaret;
PTEKDestroyCaret TEKDestroyCaret;
PTEKParse TEKParse;
PTEKReportGIN TEKReportGIN;
PTEKPaint TEKPaint;
PTEKWMLButtonDown TEKWMLButtonDown;
PTEKWMLButtonUp TEKWMLButtonUp;
PTEKWMMouseMove TEKWMMouseMove;
PTEKWMSize TEKWMSize;
PTEKCMCopy TEKCMCopy;
PTEKCMCopyScreen TEKCMCopyScreen;

PTEKPrint TEKPrint;
PTEKClearScreen TEKClearScreen;
PTEKSetupFont TEKSetupFont;
PTEKResetWin TEKResetWin;
PTEKRestoreSetup TEKRestoreSetup;
PTEKEnd TEKEnd;

static HMODULE HTTTEK = NULL;

#define IdTEKInit	   1
#define IdTEKResizeWindow  2
#define IdTEKChangeCaret   3
#define IdTEKDestroyCaret  4
#define IdTEKParse	   5
#define IdTEKReportGIN	   6
#define IdTEKPaint	   7
#define IdTEKWMLButtonDown 8
#define IdTEKWMLButtonUp   9
#define IdTEKWMMouseMove   10
#define IdTEKWMSize	   11
#define IdTEKCMCopy	   12
#define IdTEKCMCopyScreen  13
#define IdTEKPrint	   14
#define IdTEKClearScreen   15
#define IdTEKSetupFont	   16
#define IdTEKResetWin	   17
#define IdTEKRestoreSetup  18
#define IdTEKEnd	   19

BOOL LoadTTTEK()
{
  BOOL Err;

  if (HTTTEK != NULL) return TRUE;
  HTTTEK = LoadHomeDLL("TTPTEK.DLL");
  if (HTTTEK == NULL) return FALSE;

  Err = FALSE;
  TEKInit = (PTEKInit)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKInit));
  if (TEKInit==NULL) Err = TRUE;

  TEKResizeWindow = (PTEKResizeWindow)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKResizeWindow));
  if (TEKResizeWindow==NULL) Err = TRUE;

  TEKChangeCaret = (PTEKChangeCaret)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKChangeCaret));
  if (TEKChangeCaret==NULL) Err = TRUE;

  TEKDestroyCaret = (PTEKDestroyCaret)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKDestroyCaret));
  if (TEKDestroyCaret==NULL) Err = TRUE;

  TEKParse = (PTEKParse)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKParse));
  if (TEKParse==NULL) Err = TRUE;

  TEKReportGIN = (PTEKReportGIN)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKReportGIN));
  if (TEKReportGIN==NULL) Err = TRUE;

  TEKPaint = (PTEKPaint)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKPaint));
  if (TEKPaint==NULL) Err = TRUE;

  TEKWMLButtonDown = (PTEKWMLButtonDown)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKWMLButtonDown));
  if (TEKWMLButtonDown==NULL) Err = TRUE;

  TEKWMLButtonUp = (PTEKWMLButtonUp)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKWMLButtonUp));
  if (TEKWMLButtonUp==NULL) Err = TRUE;

  TEKWMMouseMove = (PTEKWMMouseMove)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKWMMouseMove));
  if (TEKWMMouseMove==NULL) Err = TRUE;

  TEKWMSize = (PTEKWMSize)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKWMSize));
  if (TEKWMSize==NULL) Err = TRUE;

  TEKCMCopy = (PTEKCMCopy)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKCMCopy));
  if (TEKCMCopy==NULL) Err = TRUE;

  TEKCMCopyScreen = (PTEKCMCopyScreen)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKCMCopyScreen));
  if (TEKCMCopyScreen==NULL) Err = TRUE;

  TEKPrint = (PTEKPrint)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKPrint));
  if (TEKPrint==NULL) Err = TRUE;

  TEKClearScreen = (PTEKClearScreen)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKClearScreen));
  if (TEKClearScreen==NULL) Err = TRUE;

  TEKSetupFont = (PTEKSetupFont)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKSetupFont));
  if (TEKSetupFont==NULL) Err = TRUE;

  TEKResetWin = (PTEKResetWin)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKResetWin));
  if (TEKResetWin==NULL) Err = TRUE;

  TEKRestoreSetup = (PTEKRestoreSetup)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKRestoreSetup));
  if (TEKRestoreSetup==NULL) Err = TRUE;

  TEKEnd = (PTEKEnd)GetProcAddress(HTTTEK,
    MAKEINTRESOURCE(IdTEKEnd));
  if (TEKEnd==NULL) Err = TRUE;

  if (Err)
  {
    FreeLibrary(HTTTEK);
    HTTTEK = NULL;
  }
  return (! Err);
}

void FreeTTTEK()
{
  if (HTTTEK!=NULL)
  {
    FreeLibrary(HTTTEK);
    HTTTEK = NULL;
  }
}
