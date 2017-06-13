/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, TTTEK.DLL interface */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (PASCAL *PTEKInit)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKResizeWindow)
  (PTEKVar tk, PTTSet ts, int W, int H);
typedef void (PASCAL *PTEKChangeCaret)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKDestroyCaret)
  (PTEKVar tk, PTTSet ts);
typedef int (PASCAL *PTEKParse)
  (PTEKVar tk, PTTSet ts, PComVar cv);
typedef void (PASCAL *PTEKReportGIN)
  (PTEKVar tk, PTTSet ts, PComVar cv, BYTE KeyCode);
typedef void (PASCAL *PTEKPaint)
  (PTEKVar tk, PTTSet ts, HDC PaintDC, PAINTSTRUCT *PaintInfo);
typedef void (PASCAL *PTEKWMLButtonDown)
  (PTEKVar tk, PTTSet ts, PComVar cv, POINT pos);
typedef void (PASCAL *PTEKWMLButtonUp)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKWMMouseMove)
  (PTEKVar tk, PTTSet ts, POINT p);
typedef void (PASCAL *PTEKWMSize)
  (PTEKVar tk, PTTSet ts, int W, int H, int cx, int cy);
typedef void (PASCAL *PTEKCMCopy)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKCMCopyScreen)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKPrint)
  (PTEKVar tk, PTTSet ts, HDC PrintDC, BOOL SelFlag);
typedef void (PASCAL *PTEKClearScreen)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKSetupFont)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKResetWin)
  (PTEKVar tk, PTTSet ts, WORD EmuOld);
typedef void (PASCAL *PTEKRestoreSetup)
  (PTEKVar tk, PTTSet ts);
typedef void (PASCAL *PTEKEnd)
  (PTEKVar tk);

extern PTEKInit TEKInit;
extern PTEKResizeWindow TEKResizeWindow;
extern PTEKChangeCaret TEKChangeCaret;
extern PTEKDestroyCaret TEKDestroyCaret;
extern PTEKParse TEKParse;
extern PTEKReportGIN TEKReportGIN;
extern PTEKPaint TEKPaint;
extern PTEKWMLButtonDown TEKWMLButtonDown;
extern PTEKWMLButtonUp TEKWMLButtonUp;
extern PTEKWMMouseMove TEKWMMouseMove;
extern PTEKWMSize TEKWMSize;
extern PTEKCMCopy TEKCMCopy;
extern PTEKCMCopyScreen TEKCMCopyScreen;
extern PTEKPrint TEKPrint;
extern PTEKClearScreen TEKClearScreen;
extern PTEKSetupFont TEKSetupFont;
extern PTEKResetWin TEKResetWin;
extern PTEKRestoreSetup TEKRestoreSetup;
extern PTEKEnd TEKEnd;

/* proto types */
BOOL LoadTTTEK();
void FreeTTTEK();

#ifdef __cplusplus
}
#endif
