/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* TTTEK.DLL, for TEK window */
#include "teraterm.h"
#include "tttypes.h"
#include "tektypes.h"
#include "ttlib.h"
#include <stdlib.h>
#include <string.h>

#include "ttcommon.h"
#include "tslib.h"
#include "codeconv.h"

#include "tekesc.h"
#include "tttek.h"

void PASCAL TEKInit(PTEKVar tk, PTTSet ts)
{
  int i;

  tk->MemDC = NULL;
  tk->HBits = NULL;
  tk->Pen = NULL;
  tk->MemPen = NULL;
  tk->ps = PS_SOLID;
  tk->BackGround = NULL;
  tk->MemBackGround = NULL;
  for (i = 0 ; i <= 3; i++)
    tk->TEKFont[i] = NULL;
  tk->ScreenHeight = 0;
  tk->ScreenWidth = 0;
  tk->AdjustSize = FALSE;
  tk->ScaleFont = FALSE;
  tk->TextSize = 0;

  tk->MoveFlag = TRUE;

  tk->ParseMode = ModeFirst;
  tk->DispMode = IdAlphaMode;
  tk->Drawing = FALSE;

  tk->RubberBand = FALSE;
  tk->Select = FALSE;
  tk->ButtonDown = FALSE;

  tk->GIN = FALSE;
  tk->CrossHair = FALSE;
  tk->IgnoreCount = 0;
  tk->GINX = 0;
  tk->GINY = 0;

  tk->GTWidth = 39;
  tk->GTHeight = 59;
  tk->GTSpacing = 12;

  tk->MarkerType = 1;
  tk->MarkerFont = NULL;
  tk->MarkerFlag = FALSE;
}

void ToggleCrossHair(PTEKVar tk, PTTSet ts, BOOL OnFlag)
{
  HDC DC;
  HPEN TempPen, OldPen;

  if (tk->CrossHair==OnFlag) return;
  DC = GetDC(tk->HWin);
  TempPen = CreatePen(PS_SOLID, 1, ts->TEKColor[0]);
  OldPen = SelectObject(DC,TempPen);
  SetROP2(DC, R2_NOT);
  MoveToEx(DC,tk->GINX,0,NULL);
  LineTo(DC,tk->GINX,tk->ScreenHeight-1);
  MoveToEx(DC,0,tk->GINY,NULL);
  LineTo(DC,tk->ScreenWidth-1,tk->GINY);
  SelectObject(DC,OldPen);
  DeleteObject(TempPen);
  ReleaseDC(tk->HWin,DC);
  tk->CrossHair = OnFlag;
}

void TEKCaretOn(PTEKVar tk, PTTSet ts)
{
  if (! tk->Active) return;
  if (tk->DispMode == IdAlphaMode)
  {
    if (ts->CursorShape==IdHCur)
      SetCaretPos(tk->CaretX,
      tk->CaretY - CurWidth);
    else
      SetCaretPos(tk->CaretX, tk->CaretY-tk->FontHeight);

    while (tk->CaretStatus > 0)
    {
      ShowCaret(tk->HWin);
      tk->CaretStatus--;
    }
  }
}

void TEKCaretOff(PTEKVar tk)
{
  if (! tk->Active) return;
  if (tk->CaretStatus == 0)
  {
    HideCaret(tk->HWin);
    tk->CaretStatus++;
  }
}

void SwitchRubberBand(PTEKVar tk, PTTSet ts, BOOL OnFlag)
{
  HDC DC;
  HPEN TempPen, OldPen;
  int OldMemRop;
  HBRUSH OldMemBrush;

  if (tk->RubberBand==OnFlag) return;

  TempPen = CreatePen(PS_DOT, 1, ts->TEKColor[0]);
  DC = GetDC(tk->HWin);
  SetBkMode(DC,1);
  SelectObject(DC,GetStockObject(HOLLOW_BRUSH));
  OldPen = SelectObject(DC,TempPen);
  SetROP2(DC, R2_NOT);
  Rectangle(DC,tk->SelectStart.x,tk->SelectStart.y,
    tk->SelectEnd.x,tk->SelectEnd.y);
  SelectObject(DC,OldPen);
  ReleaseDC(tk->HWin,DC);
  DeleteObject(TempPen);

  TempPen = CreatePen(PS_DOT, 1, tk->MemForeColor);
  OldMemBrush = SelectObject(tk->MemDC,GetStockObject(HOLLOW_BRUSH));
  SelectObject(tk->MemDC,TempPen);
  OldMemRop = SetROP2(tk->MemDC, R2_XORPEN);
  Rectangle(tk->MemDC,tk->SelectStart.x,tk->SelectStart.y,
    tk->SelectEnd.x,tk->SelectEnd.y);
  SelectObject(tk->MemDC,OldMemBrush);
  SetROP2(tk->MemDC,OldMemRop);
  SelectObject(tk->MemDC,tk->MemPen);
  DeleteObject(TempPen);

  tk->RubberBand = OnFlag;
}

void PASCAL TEKChangeCaret(PTEKVar tk, PTTSet ts)
{
  UINT T;

  if (! tk->Active) return;
  switch (ts->CursorShape) {
    case IdHCur:
      CreateCaret(tk->HWin, 0, tk->FontWidth, CurWidth);
      break;
    case IdVCur:
      CreateCaret(tk->HWin, 0, CurWidth, tk->FontHeight);
      break;
    default:
      CreateCaret(tk->HWin, 0, tk->FontWidth, tk->FontHeight);
  }
  tk->CaretStatus = 1;
  TEKCaretOn(tk,ts);
  if (ts->NonblinkingCursor!=0)
  {
    T = GetCaretBlinkTime() * 2 / 3;
    SetTimer(tk->HWin,IdCaretTimer,T,NULL);
  }
}

void PASCAL TEKDestroyCaret(PTEKVar tk, PTTSet ts)
{
  DestroyCaret();
  if (ts->NonblinkingCursor!=0)
	KillTimer(tk->HWin,IdCaretTimer);
}

void PASCAL TEKResizeWindow(PTEKVar tk, PTTSet ts, int W, int H)
{
  int i, Height, Width;
  TEXTMETRIC Metrics;
  HDC TempDC;
  HFONT TempOldFont;
  RECT R;
  LOGFONTW logfont;

  if (tk->Select)
    SwitchRubberBand(tk, ts, FALSE);
  tk->Select = FALSE;

  /* Delete old MemDC */
  if (tk->MemDC != NULL)
  {
    SelectObject(tk->MemDC, tk->OldMemFont);
    SelectObject(tk->MemDC, tk->OldMemBmp);
    SelectObject(tk->MemDC, tk->OldMemPen);
    DeleteDC(tk->MemDC);
  }

  /* Delete old fonts */
  for (i =0; i <= 3; i++)
    if (tk->TEKFont[i] != NULL)
      DeleteObject(tk->TEKFont[i]);
  if (tk->MarkerFont!=NULL)
	DeleteObject(tk->MarkerFont);

  /* Delete old bitmap */
  if (tk->HBits != NULL) DeleteObject(tk->HBits);

  /* Delete old pen */
  if (tk->Pen != NULL) DeleteObject(tk->Pen);
  if (tk->MemPen != NULL) DeleteObject(tk->MemPen);

  /* Delete old brush */
  if (tk->BackGround != NULL) DeleteObject(tk->BackGround);
  if (tk->MemBackGround != NULL) DeleteObject(tk->MemBackGround);

  /* get DC */
  TempDC = GetDC(tk->HWin);
  tk->MemDC = CreateCompatibleDC(TempDC);

  TSGetLogFont(tk->HWin, ts, 1, 0, &logfont);

  /* Create standard size font */
  if (tk->ScaleFont)
  {
    logfont.lfHeight = (int)((float)tk->ScreenHeight / 35.0);
    logfont.lfWidth = (int)((float)tk->ScreenWidth / 74.0);
  }

  logfont.lfWeight = FW_NORMAL;
  logfont.lfItalic = 0;
  logfont.lfUnderline = 0;
  logfont.lfStrikeOut = 0;
  logfont.lfOutPrecision  = OUT_CHARACTER_PRECIS;
  logfont.lfClipPrecision = CLIP_CHARACTER_PRECIS;
  logfont.lfQuality       = DEFAULT_QUALITY;
  logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

  tk->TEKFont[0] = CreateFontIndirectW(&logfont);
  /* Check standard font size */
  TempOldFont = SelectObject(TempDC,tk->TEKFont[0]);
  GetTextMetrics(TempDC, &Metrics);
  tk->FW[0] = Metrics.tmAveCharWidth;
  tk->FH[0] = Metrics.tmHeight;

  if (! tk->ScaleFont)
  {
    tk->ScreenHeight = tk->FH[0]*35;
    tk->ScreenWidth = tk->FW[0]*74;
    Width = (int)((float)tk->ScreenHeight /
			(float)ViewSizeY * (float)ViewSizeX);
    if (tk->ScreenWidth < Width)
      tk->ScreenWidth = Width;
  }

  Height = logfont.lfHeight;
  Width = logfont.lfWidth;

  /* marker font */
  logfont.lfCharSet = SYMBOL_CHARSET;
  wcsncpy_s(logfont.lfFaceName, _countof(logfont.lfFaceName), L"Symbol", _TRUNCATE);
  tk->MarkerFont = CreateFontIndirectW(&logfont);
  logfont.lfCharSet = ts->TEKFontCharSet;
  ACPToWideChar_t(ts->TEKFont, logfont.lfFaceName, _countof(logfont.lfFaceName));
  SelectObject(TempDC,tk->MarkerFont);
  GetTextMetrics(TempDC, &Metrics);
  tk->MarkerW = Metrics.tmAveCharWidth;
  tk->MarkerH = Metrics.tmHeight;

  /* second font */
  logfont.lfHeight = (int)((float)tk->ScreenHeight / 38.0);
  logfont.lfWidth = (int)((float)tk->ScreenWidth / 80.0);
  tk->TEKFont[1] = CreateFontIndirectW(&logfont);
  SelectObject(TempDC,tk->TEKFont[1]);
  GetTextMetrics(TempDC, &Metrics);
  tk->FW[1] = Metrics.tmAveCharWidth;
  tk->FH[1] = Metrics.tmHeight;

  /* third font */
  logfont.lfHeight = (int)((float)tk->ScreenHeight / 58.0);
  logfont.lfWidth = (int)((float)tk->ScreenWidth / 121.0);
  tk->TEKFont[2] = CreateFontIndirectW(&logfont);
  SelectObject(TempDC,tk->TEKFont[2]);
  GetTextMetrics(TempDC, &Metrics);
  tk->FW[2] = Metrics.tmAveCharWidth;
  tk->FH[2] = Metrics.tmHeight;

  /* forth font */
  logfont.lfHeight = (int)((float)tk->ScreenHeight / 64.0);
  logfont.lfWidth = (int)((float)tk->ScreenWidth / 133.0);
  tk->TEKFont[3] = CreateFontIndirectW(&logfont);
  SelectObject(TempDC,tk->TEKFont[3]);
  GetTextMetrics(TempDC, &Metrics);
  tk->FW[3] = Metrics.tmAveCharWidth;
  tk->FH[3] = Metrics.tmHeight;

  tk->OldMemFont =
    SelectObject(tk->MemDC,tk->TEKFont[tk->TextSize]);
  tk->FontWidth = tk->FW[tk->TextSize];
  tk->FontHeight = tk->FH[tk->TextSize];

  logfont.lfHeight = Height;
  logfont.lfWidth = Width;

  if (ts->TEKColorEmu>0)
    tk->HBits =
      CreateCompatibleBitmap(TempDC,tk->ScreenWidth,tk->ScreenHeight);
  else
    tk->HBits =
      CreateBitmap(tk->ScreenWidth, tk->ScreenHeight, 1, 1, NULL);

  tk->OldMemBmp = SelectObject(tk->MemDC, tk->HBits);

  tk->TextColor = ts->TEKColor[0];
  if (ts->TEKColorEmu>0)
  {
    tk->MemForeColor = ts->TEKColor[0];
    tk->MemBackColor = ts->TEKColor[1];
  }
  else {
    tk->MemForeColor = RGB(0,0,0);
    tk->MemBackColor = RGB(255,255,255);
  }
  tk->MemTextColor = tk->MemForeColor;

  SetTextColor(tk->MemDC, tk->MemTextColor);
  SetBkColor(tk->MemDC, tk->MemBackColor);
  SetBkMode(tk->MemDC, 1);
  SetTextAlign(tk->MemDC,TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);

  tk->BackGround = CreateSolidBrush(ts->TEKColor[1]);
  tk->MemBackGround = CreateSolidBrush(tk->MemBackColor);

  tk->PenColor = ts->TEKColor[0];
  tk->Pen = CreatePen(tk->ps,1,tk->PenColor);

  tk->MemPenColor = tk->MemForeColor;
  tk->MemPen = CreatePen(tk->ps,1,tk->MemPenColor);
  tk->OldMemPen = SelectObject(tk->MemDC, tk->MemPen);

  SelectObject(TempDC,TempOldFont);
  ReleaseDC(tk->HWin,TempDC);

  R.left = 0;
  R.right = tk->ScreenWidth;
  R.top = 0;
  R.bottom = tk->ScreenHeight;
  FillRect(tk->MemDC,&R,tk->MemBackGround);
  InvalidateRect(tk->HWin, &R, TRUE);

  tk->DispMode = IdAlphaMode;
  tk->HiY = 0;
  tk->Extra = 0;
  tk->LoY = 0;
  tk->HiX = 0;
  tk->LoX = 0;
  tk->CaretX = 0;
  tk->CaretOffset = 0;
  tk->CaretY = tk->FontHeight;
  if (tk->Active) TEKChangeCaret(tk,ts);

  GetClientRect(tk->HWin,&R);
  Width = W + tk->ScreenWidth - R.right + R.left;
  Height = H + tk->ScreenHeight - R.bottom + R.top;

  if ((Width != W) || (Height != H))
  {
    tk->AdjustSize = TRUE;
    SetWindowPos(tk->HWin,HWND_TOP,0,0,Width,Height,SWP_NOMOVE);
  }

  tk->ScaleFont = FALSE;
  tk->TEKlf = logfont;	// GraphText() tekesc.c で参照
}

int PASCAL TEKParse(PTEKVar tk, PTTSet ts, PComVar cv)
{
  BOOL f;
  int c;
  BYTE b;

  if (tk->ButtonDown)
    return 0;

  tk->ChangeEmu = 0;
  f = TRUE;

  do {
    c = CommRead1Byte(cv,&b);

    if (c > 0)
    {
      if (tk->IgnoreCount <= 0)
      {
        if (f)
        {
          TEKCaretOff(tk);
          f = FALSE;
          if (tk->GIN) ToggleCrossHair(tk,ts,FALSE);
          if (tk->RubberBand) SwitchRubberBand(tk,ts,FALSE);
        }

        switch (tk->ParseMode) {
          case ModeFirst:
			TEKParseFirst(tk,ts,cv,b);
			break;
          case ModeEscape:
			TEKEscape(tk,ts,cv,b);
			break;
		  case ModeCS:
			ControlSequence(tk,ts,cv,b);
			break;
          case ModeSelectCode:
			SelectCode(tk,ts,b);
			break;
          case Mode2OC:
			TwoOpCode(tk,ts,cv,b);
			break;
		  case ModeGT:
			GraphText(tk,ts,cv,b);
			break;
          default:
            tk->ParseMode = ModeFirst;
            TEKParseFirst(tk,ts,cv,b);
        }
      }
      else tk->IgnoreCount--;
    }
  } while ((c!=0) && (tk->ChangeEmu==0));

  ToggleCrossHair(tk,ts,tk->GIN);
  if (! f)
  {
    TEKCaretOn(tk,ts);
    SwitchRubberBand(tk,ts,tk->Select);
  }

  if (tk->ChangeEmu > 0) tk->ParseMode = ModeFirst;
  return (tk->ChangeEmu);
}

void PASCAL TEKReportGIN(PTEKVar tk, PTTSet ts, PComVar cv, BYTE KeyCode)
{
  BYTE Code[11];
  int X, Y;

  ToggleCrossHair(tk,ts,FALSE);
  X = (int)((float)tk->GINX /
			(float)tk->ScreenWidth * (float)ViewSizeX);
  Y = (int)((1.0 - (float)(tk->GINY+1) /
			 (float)tk->ScreenHeight) * (float)ViewSizeY);
  Code[0] = KeyCode;
  Code[1] = (X >> 7) + 32;
  Code[2] = ((X >> 2) & 0x1f) + 32;
  Code[3] = (Y >> 7) + 32;
  Code[4] = ((Y >> 2) & 0x1f) + 32;
  Code[5] = 0x0d;
  tk->GIN = FALSE;
  ReleaseCapture();
  CommBinaryOut(cv, &Code[0],6);
  tk->IgnoreCount = 6;
}

void PASCAL TEKPaint
  (PTEKVar tk, PTTSet ts, HDC PaintDC, PAINTSTRUCT *PaintInfo)
{
  int X,Y,W,H;

  if (PaintInfo->fErase)
    FillRect(PaintDC, &(PaintInfo->rcPaint),tk->BackGround);

  if (tk->GIN) ToggleCrossHair(tk,ts,FALSE);
  if (tk->Select) SwitchRubberBand(tk,ts,FALSE);
  X = PaintInfo->rcPaint.left;
  Y = PaintInfo->rcPaint.top;
  W = PaintInfo->rcPaint.right - X;
  H = PaintInfo->rcPaint.bottom - Y;
  SetTextColor(PaintDC, ts->TEKColor[0]);
  SetBkColor(PaintDC,  ts->TEKColor[1]);
  BitBlt(PaintDC,X,Y,W,H,tk->MemDC,X,Y,SRCCOPY);
  SwitchRubberBand(tk,ts,tk->Select);
  if (tk->GIN) ToggleCrossHair(tk,ts,TRUE);
}

void PASCAL TEKWMLButtonDown
  (PTEKVar tk, PTTSet ts, PComVar cv, POINT pos)
{
  BYTE b;

  if (tk->GIN)
  {
    b = ts->GINMouseCode & 0xff;
    TEKReportGIN(tk,ts,cv,b);
    return;
  }

  if (tk->ButtonDown) return;

  /* Capture mouse */
  SetCapture(tk->HWin);

  /* Is the position in client area? */
  if ((pos.x>=0) && (pos.x < tk->ScreenWidth) &&
      (pos.y>=0) && (pos.y < tk->ScreenHeight))
  {
    SwitchRubberBand(tk,ts,FALSE);
    tk->Select = FALSE;

    tk->SelectStart.x = pos.x;
    tk->SelectStart.y = pos.y;
    tk->SelectEnd = tk->SelectStart;
    tk->ButtonDown = TRUE;
  }
}

void PASCAL TEKWMLButtonUp(PTEKVar tk, PTTSet ts)
{
  int X;

  ReleaseCapture();
  tk->ButtonDown = FALSE;
  if ((abs(tk->SelectEnd.y - tk->SelectStart.y) > 2) &&
      (abs(tk->SelectEnd.x - tk->SelectStart.x) > 2))
  {
    if (tk->SelectStart.x > tk->SelectEnd.x)
    {
      X = tk->SelectEnd.x;
      tk->SelectEnd.x = tk->SelectStart.x;
      tk->SelectStart.x = X;
    }
    if (tk->SelectStart.y > tk->SelectEnd.y)
    {
      X = tk->SelectEnd.y;
      tk->SelectEnd.y = tk->SelectStart.y;
      tk->SelectStart.y = X;
    }
    tk->Select = TRUE;
  }
  else {
    SwitchRubberBand(tk,ts,FALSE);
    tk->Select = FALSE;
  }
}

void PASCAL TEKWMMouseMove(PTEKVar tk, PTTSet ts, POINT p)
{
  int X, Y;

  if ((! tk->ButtonDown) && (! tk->GIN)) return;
  /* get position */
  X = p.x + 1;
  Y = p.y + 1;

  /* if out of client area, force into client area */
  if (X<0) X = 0;
  else if (X > tk->ScreenWidth) X = tk->ScreenWidth - 1;
  if (Y<0) Y = 0;
  else if (Y > tk->ScreenHeight) Y = tk->ScreenHeight - 1;

  if (tk->GIN)
  {
    ToggleCrossHair(tk,ts,FALSE);
    tk->GINX = X;
    tk->GINY = Y;
    ToggleCrossHair(tk,ts,TRUE);
  }
  else {
    SwitchRubberBand(tk,ts,FALSE);
    tk->SelectEnd.x = X + 1;
    tk->SelectEnd.y = Y + 1;
    SwitchRubberBand(tk,ts,TRUE);
  }

  if (tk->GIN) SetCapture(tk->HWin);
}

void PASCAL TEKWMSize(PTEKVar tk, PTTSet ts, int W, int H, int cx, int cy)
{
  int Width, Height;

  Width = cx;
  Height = cy;

  if ((tk->ScreenWidth == Width) &&
      (tk->ScreenHeight == Height))
  {
    tk->AdjustSize = FALSE;
    return;
  }

  if (tk->AdjustSize)
  {		// resized by myself
    Width = W + tk->ScreenWidth - Width;
    Height = H + tk->ScreenHeight - Height;
    SetWindowPos(tk->HWin,HWND_TOP,0,0,Width,Height,SWP_NOMOVE);
  }
  else {
    if ((tk->ScreenWidth==0) ||			// resized
		(tk->ScreenHeight==0)) return; //  during initialization
	// resized by user
	tk->ScreenWidth = Width;
    tk->ScreenHeight = Height;
    tk->ScaleFont = TRUE;
    TEKResizeWindow(tk,ts,W,H);
  }
}

void CopyToClipboard
  (PTEKVar tk, PTTSet ts, int x, int y, int Width, int Height)
{

  if (tk->Select) SwitchRubberBand(tk,ts,FALSE);
  TEKCaretOff(tk);
  if (OpenClipboard(tk->HWin) && EmptyClipboard())
  {
    /* Create the new bitmap */
    HDC CopyDC;
    HBITMAP CopyBitmap;
    CopyDC = CreateCompatibleDC(tk->MemDC);
    CopyBitmap = CreateCompatibleBitmap(tk->MemDC, Width, Height);
    CopyBitmap = SelectObject(CopyDC, CopyBitmap);
    BitBlt(CopyDC, 0, 0, Width, Height, tk->MemDC, x, y, SRCCOPY);
    CopyBitmap = SelectObject(CopyDC, CopyBitmap);
    /* Transfer the new bitmap to the clipboard */
    SetClipboardData(CF_BITMAP, CopyBitmap);
    DeleteDC(CopyDC);
  }

  CloseClipboard();

  TEKCaretOn(tk,ts);
  SwitchRubberBand(tk,ts,tk->Select);
}

void PASCAL TEKCMCopy(PTEKVar tk, PTTSet ts)
{
  int x, y;

  if (! tk->Select) return;

  if (tk->SelectStart.x < tk->SelectEnd.x)
    x = tk->SelectStart.x;
  else x = tk->SelectEnd.x;
  if (tk->SelectStart.y < tk->SelectEnd.y)
    y = tk->SelectStart.y;
  else y = tk->SelectEnd.y;
  /* copy selected area to clipboard */
  CopyToClipboard(tk, ts, x, y,
    abs(tk->SelectEnd.x - tk->SelectStart.x),
    abs(tk->SelectEnd.y - tk->SelectStart.y));
}

void PASCAL TEKCMCopyScreen(PTEKVar tk, PTTSet ts)
{
  /* copy fullscreen to clipboard */
  CopyToClipboard(tk, ts, 0, 0, tk->ScreenWidth, tk->ScreenHeight);
}

void PASCAL TEKPrint(PTEKVar tk, PTTSet ts, HDC PrintDC, BOOL SelFlag)
{
  POINT PPI;
  RECT Margin;
  int Caps;
  int PrnWidth, PrnHeight;
  int MemWidth, MemHeight;
  POINT PrintStart, PrintEnd;

  if (PrintDC == NULL) return;

  if (SelFlag)
  {
    /* print selection */
    PrintStart = tk->SelectStart;
    PrintEnd = tk->SelectEnd;
  }
  else {
    /* print current page */
    PrintStart.x = 0;
    PrintStart.y = 0;
    PrintEnd.x = tk->ScreenWidth;
    PrintEnd.y = tk->ScreenHeight;
  }

  Caps = GetDeviceCaps(PrintDC,RASTERCAPS);
  if ((Caps & RC_BITBLT) != RC_BITBLT)
  {
	  static const TTMessageBoxInfoW info = {
		  "Tera Term",
		  "MSG_TT_ERROR", L"Tera Term: Error",
		  "MSG_TEK_PRINT_ERROR", L"Printer dose not support graphics",
		  MB_ICONEXCLAMATION };
	  TTMessageBoxW(tk->HWin, &info, ts->UILanguageFileW);
	  return;
  }
  if (tk->Active) TEKCaretOff(tk);
  if (tk->RubberBand) SwitchRubberBand(tk,ts,FALSE);

  MemWidth = PrintEnd.x - PrintStart.x;
  MemHeight = PrintEnd.y - PrintStart.y;
  if ((MemWidth==0) || (MemHeight==0)) return;

  StartPage(PrintDC);

  if ((ts->TEKPPI.x>0) && (ts->TEKPPI.y>0))
	PPI = ts->TEKPPI;
  else {
    PPI.x = GetDeviceCaps(PrintDC,LOGPIXELSX);
    PPI.y = GetDeviceCaps(PrintDC,LOGPIXELSY);
  }

  Margin.left = /* left margin */
	(int)((float)ts->PrnMargin[0] / 100.0 * (float)PPI.x);
  Margin.right = /* right margin */
	GetDeviceCaps(PrintDC,HORZRES) -
	(int)((float)ts->PrnMargin[1] / 100.0 * (float)PPI.x);
  Margin.top = /* top margin */
    (int)((float)ts->PrnMargin[2] / 100.0 * (float)PPI.y);
  Margin.bottom = /* bottom margin */
	GetDeviceCaps(PrintDC,VERTRES) -
	(int)((float)ts->PrnMargin[3] / 100.0 * (float)PPI.y);

  if ((Caps & RC_STRETCHBLT) == RC_STRETCHBLT)
  {
    PrnWidth = (int)((float)MemWidth /
      (float)GetDeviceCaps(tk->MemDC,LOGPIXELSX) * (float)PPI.x);
    PrnHeight = (int)((float)MemHeight /
      (float)GetDeviceCaps(tk->MemDC,LOGPIXELSY) * (float)PPI.y);
    StretchBlt(PrintDC, Margin.left, Margin.top, PrnWidth, PrnHeight,
               tk->MemDC, PrintStart.x, PrintStart.y, MemWidth, MemHeight, SRCCOPY);
  }
  else
    BitBlt(PrintDC, Margin.left, Margin.top, MemWidth, MemHeight,
               tk->MemDC, PrintStart.x, PrintStart.y, SRCCOPY);

  EndPage(PrintDC);

  SwitchRubberBand(tk,ts,tk->Select);
}

void PASCAL TEKClearScreen(PTEKVar tk, PTTSet ts)
{
  RECT R;

  GetClientRect(tk->HWin,&R);
  FillRect(tk->MemDC,&R,tk->MemBackGround);
  InvalidateRect(tk->HWin,&R,FALSE);
  tk->DispMode = IdAlphaMode;
  tk->CaretX = 0;
  tk->CaretOffset = 0;
  tk->CaretY = tk->FontHeight;
  TEKCaretOn(tk,ts);
}

void PASCAL TEKSetupFont(PTEKVar tk, PTTSet ts)
{
  int W, H;
//  BOOL Ok;
  RECT R;

  GetWindowRect(tk->HWin, &R);
  W = R.right - R.left;
  H = R.bottom - R.top;
  tk->TextSize = 0;
  TEKResizeWindow(tk,ts,W,H);
}

void PASCAL TEKResetWin(PTEKVar tk, PTTSet ts, WORD EmuOld)
{
  HDC TmpDC;
  RECT R;

  /* Change caret shape */
  TEKChangeCaret(tk,ts);

  /* Change display color*/
  TmpDC = GetDC(tk->HWin);

  if (ts->TEKColorEmu != EmuOld)
  {
    SelectObject(tk->MemDC,tk->OldMemBmp);
    if (tk->HBits!=NULL) DeleteObject(tk->HBits);
    if (ts->TEKColorEmu > 0)
      tk->HBits =
	CreateCompatibleBitmap(TmpDC,tk->ScreenWidth,tk->ScreenHeight);
    else
      tk->HBits =
	CreateBitmap(tk->ScreenWidth, tk->ScreenHeight, 1, 1, NULL);
    tk->OldMemBmp = SelectObject(tk->MemDC, tk->HBits);
    GetClientRect(tk->HWin,&R);
    FillRect(tk->MemDC,&R,tk->MemBackGround);
  }

  ReleaseDC(tk->HWin,TmpDC);

  tk->TextColor = ts->TEKColor[0];

  tk->ps = PS_SOLID;
  tk->PenColor = ts->TEKColor[0];
  if (tk->Pen!=NULL) DeleteObject(tk->Pen);
  tk->Pen = CreatePen(tk->ps,1,tk->PenColor);

  if (tk->BackGround!=NULL) DeleteObject(tk->BackGround);
  tk->BackGround = CreateSolidBrush(ts->TEKColor[1]);

  if (ts->TEKColorEmu>0)
  {
    tk->MemForeColor = ts->TEKColor[0];
    tk->MemBackColor = ts->TEKColor[1];
  }
  else {
    tk->MemForeColor = RGB(0,0,0);
    tk->MemBackColor = RGB(255,255,255);
  }
  tk->MemTextColor = tk->MemForeColor;
  tk->MemPenColor = tk->MemForeColor;

  SelectObject(tk->MemDC, tk->OldMemPen);
  if (tk->MemPen!=NULL) DeleteObject(tk->MemPen);
  tk->MemPen = CreatePen(tk->ps,1,tk->MemPenColor);
  tk->OldMemPen = SelectObject(tk->MemDC,tk->MemPen);

  if (tk->MemBackGround!=NULL) DeleteObject(tk->MemBackGround);
  tk->MemBackGround = CreateSolidBrush(tk->MemBackColor);

  SetTextColor(tk->MemDC, tk->MemTextColor);
  SetBkColor(tk->MemDC, tk->MemBackColor);

  if ((ts->TEKColorEmu>0) ||
      (ts->TEKColorEmu!=EmuOld))
    TEKClearScreen(tk,ts);

  InvalidateRect(tk->HWin,NULL,TRUE);
}

void PASCAL TEKRestoreSetup(PTEKVar tk, PTTSet ts)
{
  int W, H;
  RECT R;

  /* change window */
  tk->TextSize = 0;

  GetWindowRect(tk->HWin, &R);
  W = R.right - R.left;
  H = R.bottom - R.top;
  TEKResizeWindow(tk,ts,W,H);
}

void PASCAL TEKEnd(PTEKVar tk)
{
  int i;

  if (tk->MemDC != NULL) DeleteDC(tk->MemDC);
  for (i = 0; i <= 3; i++)
    if (tk->TEKFont[i]!=NULL) DeleteObject(tk->TEKFont[i]);
  if (tk->MarkerFont != NULL) DeleteObject(tk->MarkerFont);
  if (tk->HBits != NULL) DeleteObject(tk->HBits);
  if (tk->Pen != NULL) DeleteObject(tk->Pen);
  if (tk->MemPen != NULL) DeleteObject(tk->MemPen);
  if (tk->BackGround != NULL) DeleteObject(tk->BackGround);
  if (tk->MemBackGround != NULL) DeleteObject(tk->MemBackGround);
}
