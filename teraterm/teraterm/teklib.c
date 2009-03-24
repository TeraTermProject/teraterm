/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTTEK.DLL interface */

#include "teraterm.h"
#include "tttypes.h"
#include "tektypes.h"

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
  HTTTEK = LoadLibrary("TTPTEK.DLL");
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
