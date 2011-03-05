/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTTEK.DLL, TEK escape sequences */
#include "teraterm.h"
#include "tttypes.h"
#include "tektypes.h"
#include "ttcommon.h"
#include <math.h>
#include <string.h>

#include "tekesc.h"

void Log1Byte(PComVar cv, BYTE b)
{
  ((PCHAR)(cv->LogBuf))[cv->LogPtr] = b;
  cv->LogPtr++;
  if (cv->LogPtr >= InBuffSize)
    cv->LogPtr = cv->LogPtr - InBuffSize;
  if (cv->LCount >= InBuffSize)
  {
    cv->LCount = InBuffSize;
    cv->LStart = cv->LogPtr;
  }
  else cv->LCount++;
}

void ChangeTextSize(PTEKVar tk, PTTSet ts)
{
  SelectObject(tk->MemDC,tk->TEKFont[tk->TextSize]);
  tk->FontWidth = tk->FW[tk->TextSize];
  tk->FontHeight = tk->FH[tk->TextSize];

  if (tk->Active) TEKChangeCaret(tk, ts);
}

void BackSpace(PTEKVar tk, PComVar cv)
{
  tk->CaretX = tk->CaretX - tk->FontWidth;
  if (tk->CaretX < 0) tk->CaretX = 0;
  if (cv->HLogBuf != NULL) Log1Byte(cv,BS);
}

void LineFeed(PTEKVar tk, PComVar cv)
{
  tk->CaretY = tk->CaretY + tk->FontHeight;
  if (tk->CaretY > tk->ScreenHeight)
  {
    tk->CaretY = tk->FontHeight;
    if ((tk->CaretOffset==0) || (tk->CaretX < tk->CaretOffset))
    {
      tk->CaretOffset = tk->ScreenWidth >> 1;
      tk->CaretX = tk->CaretX + tk->CaretOffset;
      if (tk->CaretX >= tk->ScreenWidth)
	tk->CaretX = tk->CaretOffset;
    }
    else {
      tk->CaretX = tk->CaretX - tk->CaretOffset;
      tk->CaretOffset = 0;
    }
  }
  if (cv->HLogBuf != NULL) Log1Byte(cv,LF);
}

void CarriageReturn(PTEKVar tk, PComVar cv)
{
  tk->CaretX = tk->CaretOffset;
  if (cv->HLogBuf != NULL) Log1Byte(cv,CR);
}

void Tab(PTEKVar tk, PComVar cv)
{
  if (cv->HLogBuf != NULL) Log1Byte(cv,HT);
  tk->CaretX = tk->CaretX + (tk->FontWidth << 3);
  if (tk->CaretX >= tk->ScreenWidth)
  {
    CarriageReturn(tk,cv);
    LineFeed(tk,cv);
  }
}

void EnterVectorMode(PTEKVar tk)
{
  tk->MoveFlag = TRUE;
  tk->Drawing = FALSE;
  tk->DispMode = IdVectorMode;
}

void EnterMarkerMode(PTEKVar tk)
{
  tk->MoveFlag = TRUE;
  tk->Drawing = FALSE;
  tk->DispMode = IdMarkerMode;
}

void EnterPlotMode(PTEKVar tk)
{
  if (tk->DispMode == IdAlphaMode)
    tk->CaretOffset = 0;

  tk->PlotX =
    (int)((float)(tk->CaretX) / (float)(tk->ScreenWidth) *
	  (float)(ViewSizeX));
  tk->PlotY =
    (int)((float)(tk->ScreenHeight-1-tk->CaretY) /
	  (float)(tk->ScreenHeight) * (float)(ViewSizeY));
  tk->DispMode = IdPlotMode;
  tk->JustAfterRS = TRUE;
}

void EnterAlphaMode(PTEKVar tk)
{
  if ((tk->DispMode == IdVectorMode) ||
      (tk->DispMode == IdPlotMode))
    tk->CaretOffset = 0;

  tk->DispMode = IdAlphaMode;
  tk->Drawing = FALSE;
}

void EnterUnknownMode(PTEKVar tk)
{
  if ((tk->DispMode == IdVectorMode) ||
      (tk->DispMode == IdPlotMode))
    tk->CaretOffset = 0;

  tk->DispMode = IdUnknownMode;
}

void TEKMoveTo(PTEKVar tk, PTTSet ts, int X, int Y, BOOL Draw)
{
  HDC DC;
  HPEN OldPen;

  if (Draw)
  {
    DC = GetDC(tk->HWin);
    OldPen = SelectObject(DC,tk->Pen);
    SetBkColor(DC,ts->TEKColor[1]);
    SetBkMode(DC, 1);
    if ((tk->CaretX == X) && (tk->CaretY==Y))
    {
      SetPixel(DC,X,Y,tk->PenColor);
      SetPixel(tk->MemDC,X,Y,tk->MemPenColor);
    }
    else {
      MoveToEx(DC,tk->CaretX,tk->CaretY,NULL);
      LineTo(DC,X,Y);
      MoveToEx(tk->MemDC,tk->CaretX,tk->CaretY,NUL);
      LineTo(tk->MemDC,X,Y);
    }
    SelectObject(DC,OldPen);
    ReleaseDC(tk->HWin,DC);
  }
  tk->CaretX = X;
  tk->CaretY = Y;
}

void DrawMarker(PTEKVar tk)
{
  BYTE b;
  RECT R;
  HFONT OldFont;
  COLORREF OldColor;

  switch (tk->MarkerType) {
    case 0: b = 0xb7; break;
    case 1: b = '+'; break;
    case 2: b = 0xc5; break;
    case 3: b = '*'; break;
    case 4: b = 'o'; break;
    case 5: b = 0xb4; break;
    case 6: b = 'O'; break;
    case 7: b = 0xe0; break;
    case 8: b = 0xc6; break;
    case 9: b = 0xa8; break;
    case 10: b = 0xa7; break;
    default:
      b = '+';
  }
  OldFont = SelectObject(tk->MemDC,tk->MarkerFont);
  OldColor = SetTextColor(tk->MemDC,tk->PenColor);
  SetTextAlign(tk->MemDC,TA_CENTER | TA_BOTTOM | TA_NOUPDATECP);
  ExtTextOut(tk->MemDC,tk->CaretX,tk->CaretY+tk->MarkerH/2,
    0,NULL,&b,1,NULL);
  SetTextAlign(tk->MemDC,TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);
  SelectObject(tk->MemDC,OldFont);
  SetTextColor(tk->MemDC,OldColor);
  R.left = tk->CaretX-	tk->MarkerW;
  R.right = tk->CaretX + tk->MarkerW;
  R.top = tk->CaretY - tk->MarkerH;
  R.bottom = tk->CaretY + tk->MarkerH;
  InvalidateRect(tk->HWin,&R,FALSE);
}

void Draw(PTEKVar tk, PComVar cv, PTTSet ts, BYTE b)
{
  int X, Y;

  if (! tk->Drawing)
  {
    tk->LoXReceive = FALSE;
    tk->LoCount = 0;
    tk->Drawing = TRUE;
    if (tk->DispMode==IdMarkerMode)
    {
      tk->MoveFlag = TRUE;
      tk->MarkerFlag = TRUE;
    }
    else
      tk->MarkerFlag = FALSE;
  }

  if (b<=0x1f)
  {
    CommInsert1Byte(cv,b);
    return;
  }
  else if ((b>=0x20) && (b<=0x3f))
  {
    if (tk->LoCount==0)
      tk->HiY = b - 0x20;
    else
      tk->HiX = b - 0x20;
  }
  else if ((b>=0x40) && (b<=0x5f))
  {
    tk->LoX = b - 0x40;
    tk->LoXReceive = TRUE;
  }
  else if ((b>=0x60) && (b<=0x7f))
  {
    tk->LoA = tk->LoB;
    tk->LoB = b - 0x60;
    tk->LoCount++;
  }

  if (! tk->LoXReceive) return;

  tk->Drawing = FALSE;
  tk->ParseMode = ModeFirst;

  if (tk->LoCount > 1)
    tk->Extra = tk->LoA;
  if (tk->LoCount > 0)
    tk->LoY = tk->LoB;

  X = (int)((float)(tk->HiX*128 + tk->LoX*4 + (tk->Extra & 3)) /
	    (float)(ViewSizeX) * (float)(tk->ScreenWidth));
  Y = tk->ScreenHeight - 1 -
    (int)((float)(tk->HiY * 128 + tk->LoY * 4 + tk->Extra / 4) /
	  (float)(ViewSizeY) * (float)(tk->ScreenHeight));
  TEKMoveTo(tk,ts,X,Y,! tk->MoveFlag);
  tk->MoveFlag = FALSE;
  if (tk->MarkerFlag)
    DrawMarker(tk);
  tk->MarkerFlag = FALSE;
}

void Plot(PTEKVar tk, PTTSet ts, BYTE b)
{
  int X, Y;

  X = 0;
  Y = 0;
  switch (b) {
    case 'A': X = 4; break;
    case 'B': X = -4; break;
    case 'D': Y = 4; break;
    case 'E':
      X = 4;
      Y = 4;
      break;
    case 'F':
      X = -4;
      Y = 4;
      break;
    case 'H': Y = -4; break;
    case 'I':
      X = 4;
      Y = -4;
      break;
    case 'J':
      X = -4;
      Y = -4;
      break;
    default:
      return;
  }
  tk->PlotX = tk->PlotX + X;
  if (tk->PlotX < 0) tk->PlotX = 0;
  if (tk->PlotX >= ViewSizeX)
    tk->PlotX = ViewSizeX - 1;
  tk->PlotY = tk->PlotY + Y;
  if (tk->PlotY < 0) tk->PlotY = 0;
  if (tk->PlotY >= ViewSizeY)
    tk->PlotY = ViewSizeY - 1;
  X = (int)((float)(tk->PlotX) / (float)(ViewSizeX) *
	    (float)(tk->ScreenWidth));
  Y = tk->ScreenHeight - 1 -
    (int)((float)(tk->PlotY) / (float)(ViewSizeY) *
	  (float)(tk->ScreenHeight));
  TEKMoveTo(tk,ts,X,Y,tk->PenDown);
}

void DispChar(PTEKVar tk, PComVar cv, BYTE b)
{
  int Dx[2];
  RECT R;

  Dx[0] = tk->FontWidth;
  Dx[1] = tk->FontWidth;
  ExtTextOut(tk->MemDC,tk->CaretX,tk->CaretY,0,NULL,&b,1,&Dx[0]);
  R.left = tk->CaretX;
  R.right = tk->CaretX + tk->FontWidth;
  R.top = tk->CaretY - tk->FontHeight;
  R.bottom = tk->CaretY;
  InvalidateRect(tk->HWin,&R,FALSE);
  tk->CaretX = R.right;

  if (cv->HLogBuf != NULL) Log1Byte(cv,b);

  if (tk->CaretX > tk->ScreenWidth - tk->FontWidth)
  {
    CarriageReturn(tk,cv);
    LineFeed(tk,cv);
  }
}

void TEKBeep(PTTSet ts)
{
  if (ts->Beep!=0)
    MessageBeep(0);
}

void ParseFirst(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b)
{
  if (tk->DispMode == IdAlphaMode)
  {
    switch (b) {
      case 0x07: TEKBeep(ts); break;
      case 0x08: BackSpace(tk,cv); break;
      case 0x09: Tab(tk,cv); break;
      case 0x0a: LineFeed(tk,cv); break;
      case 0x0b:
      case 0x0c: break;
      case 0x0d:
	CarriageReturn(tk,cv);
	if (ts->CRReceive==IdCRLF)
	  CommInsert1Byte(cv,0x0A);
	break;
      case 0x18:
	if (ts->AutoWinSwitch > 0)
	tk->ChangeEmu = IdVT; /* Enter VT Mode */
	break;
      case 0x19:
      case 0x1a: break;
      case 0x1b:
	tk->ParseMode = ModeEscape;
	return;
      case 0x1c: EnterMarkerMode(tk); break;
      case 0x1d: EnterVectorMode(tk); break;
      case 0x1e: EnterPlotMode(tk); break;
      case 0x1f: break;
      case 0x7f: if (tk->Drawing) Draw(tk,cv,ts,b); break;
      default:
	if (b<=0x06) break;
	if ((b>=0x0e) && (b<=0x17)) break;
	if (tk->Drawing) Draw(tk,cv,ts,b);
	  else DispChar(tk,cv,b);
    }
  }
  else if ((tk->DispMode==IdVectorMode) ||
	   (tk->DispMode==IdMarkerMode))
  {
    switch (b) {
      case 0x07:
	if (tk->MoveFlag)
	  tk->MoveFlag = FALSE;
	else
	  TEKBeep(ts);
	break;
      case 0x0d:
	CommInsert1Byte(cv,0x0d);
	EnterAlphaMode(tk); /* EnterAlphaMode */
	break;
      case 0x18:
	if (ts->AutoWinSwitch > 0)
	tk->ChangeEmu = IdVT; /* Enter VT Mode */
	break;
      case 0x19:
      case 0x1a: break;
      case 0x1b:
	tk->ParseMode = ModeEscape;
	return;
      case 0x1c: EnterMarkerMode(tk); break;
      case 0x1d: EnterVectorMode(tk); break;
      case 0x1e: EnterPlotMode(tk); break;
      case 0x1f: EnterAlphaMode(tk); break;
      default:
	if (b<=0x06) break;
	if ((b>=0x08) && (b<=0x0c)) break;
	if ((b>=0x0e) && (b<=0x17)) break;
	Draw(tk,cv,ts,b);
    }
  }
  else if (tk->DispMode==IdPlotMode)
  {
    switch (b) {
      case 0x07: TEKBeep(ts); break;
      case 0x0d:
	CommInsert1Byte(cv,0x0d);
	EnterAlphaMode(tk); /* EnterAlphaMode */
	break;
      case 0x18:
	if (ts->AutoWinSwitch > 0)
	tk->ChangeEmu = IdVT; /* Enter VT Mode */
	break;
      case 0x19:
      case 0x1a: break;
      case 0x1b:
	tk->ParseMode = ModeEscape;
	return;
      case 0x1c: EnterMarkerMode(tk); break;
      case 0x1d: EnterVectorMode(tk); break;
      case 0x1e: EnterPlotMode(tk); break;
      case 0x1f: EnterAlphaMode(tk); break; /* EnterAlphaMode */
      case 0x7f: break;
      default:
	if (b<=0x06) break;
	if ((b>=0x08) && (b<=0x0c)) break;
	if ((b>=0x0e) && (b<=0x17)) break;

	if (tk->JustAfterRS)
	{
	  if (b==0x20)
	    tk->PenDown = FALSE;
	  else if (b=='P')
	    tk->PenDown = TRUE;
	  else
	    EnterUnknownMode(tk);
	  tk->JustAfterRS = FALSE;
	}
	else
	  Plot(tk,ts,b);
    }
  }
  else {
    switch (b) {
      case 0x1f: EnterAlphaMode(tk); break;
    }
  }

  tk->ParseMode = ModeFirst;
}

void SelectCode(PTEKVar tk, PTTSet ts, BYTE b)
{
  switch (b) {
    case ' ':
      if (tk->SelectCodeFlag == '#')
	tk->SelectCodeFlag = b;
      return;
    case '!':
      if (tk->SelectCodeFlag == '%')
	tk->SelectCodeFlag = b;
      return;
    case '0': break;
    case '1':
    case '2':
      if ((tk->SelectCodeFlag == ' ') ||
	  (tk->SelectCodeFlag == '!'))
	tk->ChangeEmu = IdVT; /* enter VT mode */
      break;
    case '3':
      if (tk->SelectCodeFlag == '!')
	tk->ChangeEmu = IdVT; /* enter VT mode */
      break;
    case '5':
      if (tk->SelectCodeFlag == ' ')
	tk->ChangeEmu = IdVT; /* enter VT mode */
      break;
    default:
      if (b <= US) return;
  }
  if ((0x20<=b) && (b<0x7f)) tk->ParseMode = ModeFirst;
  tk->SelectCodeFlag = 0;
  if (ts->AutoWinSwitch == 0) tk->ChangeEmu = 0;
}

COLORREF ColorIndex(COLORREF Fore, COLORREF Back, WORD w)
{
  switch (w) {
    case 1:  return Fore;
    case 2:  return RGB(255,  0,  0); /* Red */
    case 3:  return RGB(  0,255,  0); /* Green */
    case 4:  return RGB(  0,  0,255); /* Blue */
    case 5:  return RGB(  0,255,255); /* Cyan */
    case 6:  return RGB(255,  0,255); /* Magenta */
    case 7:  return RGB(255,255,  0); /* Yellow */
    case 8:  return RGB(255,128,  0); /* Orange */
    case 9:  return RGB(128,255,  0); /* Green-Yellow */
    case 10: return RGB(  0,255,128); /* Green-Cyan */
    case 11: return RGB(  0,128,255); /* Blue-Cyan */
    case 12: return RGB(128,  0,255); /* Blue-Magenta */
    case 13: return RGB(255,  0,128); /* Red-Magenta */
    case 14: return RGB( 85, 85, 85); /* Dark gray */
    case 15: return RGB(170,170,170); /* Light gray */
  }
  return Back;
}

void SetLineIndex(PTEKVar tk, PTTSet ts, WORD w)
// change graphic color
{
  HDC TempDC;

  tk->PenColor = ColorIndex(ts->TEKColor[0],ts->TEKColor[1],w);
  TempDC = GetDC(tk->HWin);
  tk->PenColor = GetNearestColor(TempDC,tk->PenColor);
  ReleaseDC(tk->HWin,TempDC);
  if (ts->TEKColorEmu > 0)
    tk->MemPenColor = tk->PenColor;
  else
    if (tk->PenColor == ts->TEKColor[1])
      tk->MemPenColor = tk->MemBackColor;
    else
      tk->MemPenColor = tk->MemForeColor;

  DeleteObject(tk->Pen);
  tk->Pen = CreatePen(tk->ps,1,tk->PenColor);
  
  SelectObject(tk->MemDC, tk->OldMemPen);
  DeleteObject(tk->MemPen);
  tk->MemPen = CreatePen(tk->ps,1,tk->MemPenColor);
  tk->OldMemPen = SelectObject(tk->MemDC,tk->MemPen);
}

void SetTextIndex(PTEKVar tk, PTTSet ts, WORD w)
// change text color
{
  HDC TempDC;

  tk->TextColor = ColorIndex(ts->TEKColor[0],ts->TEKColor[1],w);
  TempDC = GetDC(tk->HWin);
  tk->TextColor = GetNearestColor(TempDC,tk->TextColor);
  ReleaseDC(tk->HWin,TempDC);
  if (ts->TEKColorEmu > 0)
    tk->MemTextColor = tk->TextColor;
  else
    if (tk->TextColor == ts->TEKColor[1])
      tk->MemTextColor = tk->MemBackColor;
    else
      tk->MemTextColor = tk->MemForeColor;

  SetTextColor(tk->MemDC, tk->MemTextColor);
}

void SetColorIndex(PTEKVar tk, PTTSet ts, WORD w)
// change color for text & graphics
{
  SetLineIndex(tk,ts,w);
  SetTextIndex(tk,ts,w);
}

void SetLineStyle(PTEKVar tk, BYTE b)
{
  switch (b) {
    case 0: tk->ps = PS_SOLID; break;
    case 1: tk->ps = PS_DOT; break;
    case 2: tk->ps = PS_DASHDOT; break;
    case 3: tk->ps = PS_DASH; break;
    case 4: tk->ps = PS_DASH; break;
    case 5: tk->ps = PS_DASHDOTDOT; break;
    case 6: tk->ps = PS_DASHDOT; break;
    case 7: tk->ps = PS_DASH; break;
    case 8: tk->ps = PS_DASHDOTDOT; break;
    default:
      tk->ps = PS_SOLID;
  }
  if (tk->Pen != NULL) DeleteObject(tk->Pen);
  tk->Pen = CreatePen(tk->ps,1,tk->PenColor);

  SelectObject(tk->MemDC, tk->OldMemPen);
  if (tk->MemPen != NULL) DeleteObject(tk->MemPen);
  tk->MemPen = CreatePen(tk->ps,1,tk->MemPenColor);
  tk->OldMemPen = SelectObject(tk->MemDC,tk->MemPen);
}

void TwoOpCode(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b)
{
  double Re;

  if (tk->OpCount == 2)
  {
    tk->OpCount--;
    tk->Op2OC = b << 8;
    return;
  }
  else if (tk->OpCount == 1)
  {
    if (b <= 0x1a) return;
    else if ((b>=0x1b) && (b<=0x1f))
    {
      CommInsert1Byte(cv,b);
      tk->ParseMode = ModeFirst;
      return;
    }
    else {
      tk->OpCount--;
      tk->Op2OC = tk->Op2OC + b;
      switch (tk->Op2OC) {
	case IdMove: tk->PrmCountMax = 0; break;
	case IdDraw: tk->PrmCountMax = 0; break;
	case IdDrawMarker: tk->PrmCountMax = 0; break;
	case IdGraphText: tk->PrmCountMax = 1; break;
	case IdSetDialogVisibility: tk->PrmCountMax = 1; break;
	case IdSetGraphTextSize: tk->PrmCountMax = 3; break;
	case IdSetWriteMode: tk->PrmCountMax = 1; break;
	case IdSetLineIndex: tk->PrmCountMax = 1; break;
	case IdSetMarkerType: tk->PrmCountMax = 1; break;
	case IdSetCharPath: tk->PrmCountMax = 1; break;
	case IdSetPrecision: tk->PrmCountMax = 1; break;
	case IdSetRotation: tk->PrmCountMax = 2; break;
	case IdSetTextIndex: tk->PrmCountMax = 1; break;
	case IdSetLineStyle: tk->PrmCountMax = 1; break;
	default:
	  tk->PrmCountMax = 0;
      }
      memset(tk->Prm2OC,0,sizeof(tk->Prm2OC));
      tk->PrmCount = 0;
      if (tk->PrmCountMax > 0) return;
    }
  }

  if (tk->PrmCount < tk->PrmCountMax)
  {
    if (b <= 0x1a) return;
    else if ((b>=0x1b) && (b<=0x1f))
    {
      CommInsert1Byte(cv,b);
      tk->PrmCount = tk->PrmCountMax;
      tk->ParseMode = ModeFirst;
    }
    else if ((b>=0x20) && (b<=0x2f))
    {  /* LoI (minus) */
      tk->Prm2OC[tk->PrmCount] =
	- (tk->Prm2OC[tk->PrmCount]*16 + b - 0x20);
      tk->PrmCount++;
    }
    else if ((b>=0x30) && (b<=0x3f))
    {  /* LoI (plus) */
      tk->Prm2OC[tk->PrmCount] =
      tk->Prm2OC[tk->PrmCount]*16 + b - 0x30;
      tk->PrmCount++;
    }
    else if ((b>=0x40) && (b<=0x7f))
      tk->Prm2OC[tk->PrmCount] =
      tk->Prm2OC[tk->PrmCount]*64 + b - 0x40;
  }

  if (tk->PrmCount < tk->PrmCountMax) return;

  switch (tk->Op2OC) {
    case IdMove:
      tk->LoXReceive = FALSE;
      tk->LoCount = 0;
      tk->Drawing = TRUE;
      tk->MoveFlag = TRUE;
      tk->MarkerFlag = FALSE;
      break;
    case IdDraw:
      tk->LoXReceive = FALSE;
      tk->LoCount = 0;
      tk->Drawing = TRUE;
      tk->MoveFlag = FALSE;
      tk->MarkerFlag = FALSE;
      break;
    case IdDrawMarker:
      tk->LoXReceive = FALSE;
      tk->LoCount = 0;
      tk->Drawing = TRUE;
      tk->MoveFlag = TRUE;
      tk->MarkerFlag = TRUE;
      break;
    case IdGraphText:
      tk->GTCount = 0;
      tk->GTLen = tk->Prm2OC[0];
      if (tk->GTLen>0)
	tk->ParseMode = ModeGT;
      else
	tk->ParseMode = ModeFirst;
      return;
    case IdSetDialogVisibility: break;
    case IdSetGraphTextSize:
      tk->GTWidth = tk->Prm2OC[0];
      tk->GTHeight = tk->Prm2OC[1];
      tk->GTSpacing = tk->Prm2OC[2];
      if ((tk->GTWidth==0) &&
	  (tk->GTHeight==0) &&
	  (tk->GTSpacing==0))
      {
	tk->GTWidth=39;
	tk->GTHeight=59;
	tk->GTSpacing=12;
      }
      if (tk->GTWidth<=0) tk->GTWidth=39;
      if (tk->GTHeight<=0) tk->GTWidth=59;
      if (tk->GTSpacing<=0) tk->GTSpacing=0;
      break;
    case IdSetWriteMode: break;
    case IdSetLineIndex:
      SetLineIndex(tk,ts,tk->Prm2OC[0]);
      break;
    case IdSetMarkerType:
      tk->MarkerType = tk->Prm2OC[0];
      break;
    case IdSetCharPath: break;
    case IdSetPrecision: break;
    case IdSetRotation:
      Re = (double)tk->Prm2OC[0] *
	pow(2.0,(double)tk->Prm2OC[1]);
      Re = fmod(Re,360.0);
      if (Re<0) Re = Re + 360.0;
      if (Re<45.0)
	tk->GTAngle=0;
      else if (Re<135.0)
	tk->GTAngle = 1;
      else if (Re<315.0)
	tk->GTAngle = 2;
      else
	tk->GTAngle = 3;
      break;
    case IdSetTextIndex:
      SetTextIndex(tk,ts,tk->Prm2OC[0]);
      break;
    case IdSetLineStyle:
      SetLineStyle(tk,(BYTE)(tk->Prm2OC[0]));
      break;
  }

  tk->ParseMode = ModeFirst;
}

  void EscPage(PTEKVar tk) /* Clear Screen and enter ALPHAMODE */
  {
    RECT R;

    SetLineStyle(tk,0);
    GetClientRect(tk->HWin,&R);
    FillRect(tk->MemDC,&R,tk->MemBackGround);
    InvalidateRect(tk->HWin,&R,FALSE);
    UpdateWindow(tk->HWin);
    EnterAlphaMode(tk);
    tk->CaretX = 0;
    tk->CaretOffset = 0;
    tk->CaretY = 0;
  }

void TEKEscape(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b)
{
  switch (b) {
    case 0x00: return;
    case 0x0a: return;
    case 0x0c: EscPage(tk); break;
    case 0x0d: return;
    case 0x1a:
      tk->GIN = TRUE;
      /* Capture mouse */
      SetCapture(tk->HWin);
      EnterAlphaMode(tk);
      break;
    case 0x1b: return;
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
      CommInsert1Byte(cv,b); break;
    case '#':
      tk->ParseMode = ModeSelectCode;
      tk->SelectCodeFlag = b;
      return;
    case '%':
      tk->ParseMode = ModeSelectCode;
      tk->SelectCodeFlag = b;
      return;
    case '2':
      if (ts->AutoWinSwitch > 0)
	tk->ChangeEmu = IdVT; /* Enter VT Mode */
      break;
    case '[':
      tk->CSCount = 0;
      tk->ParseMode = ModeCS;
      return;
    case 0x7f: return;
    default:
      if ((b>='8') && (b<=';'))
      {
	if (tk->TextSize != b-0x38)
	{
	  tk->TextSize = b-0x38;
	  ChangeTextSize(tk,ts);
	}
	break;
      }
      else if ((b>='I') && (b<='Z'))
      {
	tk->ParseMode = Mode2OC;
	tk->OpCount = 2;
	TwoOpCode(tk,ts,cv,b);
	return;
      }
      else if ((b>=0x60) && (b<=0x6f))
      {
	SetLineStyle(tk,(BYTE)((b-0x60) & 7));
	break;
      }
  }
  tk->ParseMode = ModeFirst;
}

  void CSSetAttr(PTEKVar tk, PTTSet ts)
  {
    int i, P;

    for (i=1 ; i<=tk->NParam ; i++)
    {
      P = tk->Param[i];
      if (P<0) P = 0;
      switch (P) {
	/* Clear */
	case 0:
	  SetColorIndex(tk,ts,1);
	  break;
	/* Bold */
	case 1:
	  break;
	/* Under line */
	case 4:
	  break;
	/* Blink */
	case 5:
	  break;
	/* Reverse */
	case 7:
	  break;
	/* Bold off */
	case 22:
	  break;
	/* Under line off */
	case 24:
	  break;
	/* Blink off */
	case 25:
	  break;
	/* Reverse off */
	case 27:
	  break;
	// colors for text & graphics
	case 30: SetColorIndex(tk,ts,0); break;
	case 31: SetColorIndex(tk,ts,2); break;
	case 32: SetColorIndex(tk,ts,3); break;
	case 33: SetColorIndex(tk,ts,7); break;
	case 34: SetColorIndex(tk,ts,4); break;
	case 35: SetColorIndex(tk,ts,6); break;
	case 36: SetColorIndex(tk,ts,5); break;
	case 37: SetColorIndex(tk,ts,1); break;
	case 39: SetColorIndex(tk,ts,1); break;
      }
    }
  }

void ParseCS(PTEKVar tk, PTTSet ts, PComVar cv)
{
  #define IntCharMax 5
  int i = 0;
  BYTE IntChar[IntCharMax+1];
  int ICount = 0;
  BOOL FirstPrm = TRUE;
  BYTE Prv = 0;
  BOOL MoveToVT;
  BYTE b;

  tk->NParam = 1;
  tk->Param[1] = -1;
  do {
    b = tk->CSBuff[i++];
    if ((b>=0x20) && (b<=0x2F))
    {
      if (ICount<IntCharMax) ICount++;
      IntChar[ICount] = b;
    }
    else if ((b>=0x30) && (b<=0x39))
    {
      if (tk->Param[tk->NParam] < 0)
	tk->Param[tk->NParam] = 0; 
      if (tk->Param[tk->NParam]<1000)
	tk->Param[tk->NParam] = tk->Param[tk->NParam]*10 + b - 0x30;
    }
    else if (b==0x3B)
    {
      if (tk->NParam < NParamMax)
      {
	tk->NParam++;
	tk->Param[tk->NParam] = -1;
      }
    }
    else if ((b>=0x3C) && (b<=0x3F))
    {
      if (FirstPrm) Prv = b;
    }
    FirstPrm = FALSE;
  } while ((i<tk->CSCount) &&
	   ! ((b>=0x40) && (b<=0x7E)));

  MoveToVT = FALSE;
  if ((b>=0x40) && (b<=0x7E))
  {
    switch (ICount) {
      /* no intermediate char */
      case 0:
	switch (Prv) {
	  /* no private parameters */
	  case 0:
	    switch (b) {
	      case 'm': CSSetAttr(tk,ts); break;
	      default: MoveToVT = TRUE;
	    }
	    break;
	  case '?':
	    // ignore ^[?38h (select TEK mode)
	    if ((b!='h') || (tk->Param[1]!=38))
	      MoveToVT = TRUE;
	    break;
	  default:
	    MoveToVT = TRUE;
	}
	break;
      default:
	MoveToVT = TRUE;
    } /* end of switch (Icount) */
  }
  else
    MoveToVT = TRUE;

  if ((ts->AutoWinSwitch>0) && MoveToVT)
  {
    for (i=tk->CSCount-1 ; i>=0; i--)
      CommInsert1Byte(cv,tk->CSBuff[i]);
    CommInsert1Byte(cv,0x5B);
    CommInsert1Byte(cv,0x1B);
    tk->ChangeEmu = IdVT;
  }
  tk->ParseMode = ModeFirst;
}

void ControlSequence(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b)
{
  switch (b) {
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
      CommInsert1Byte(cv,b); break;
    default:
      if (tk->CSCount < sizeof(tk->CSBuff))
      {
	tk->CSBuff[tk->CSCount++] = b;
	if ((b>=0x40) && (b<=0x7E))
	  ParseCS(tk,ts,cv);
	return;
      }
  }
  tk->ParseMode = ModeFirst;
}

void GraphText(PTEKVar tk, PTTSet ts, PComVar cv, BYTE b)
{
  int i, Dx[80];
  HFONT TempFont, TempOld;
  LOGFONT lf;
  RECT R;
  int W, H;
  TEXTMETRIC Metrics;

  switch (b) {
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
      CommInsert1Byte(cv,b); break;
    default:
      tk->GTBuff[tk->GTCount++] = b;
      if ((tk->GTCount>=sizeof(tk->GTBuff)) ||
	  (tk->GTCount>=tk->GTLen))
      {
	memcpy(&lf,&tk->TEKlf,sizeof(lf));
	switch (tk->GTAngle) {
	  case 0: lf.lfEscapement = 0; break;
	  case 1: lf.lfEscapement = 900; break;
	  case 2: lf.lfEscapement = 1800; break;
	  case 3: lf.lfEscapement = 2700; break;
	}
	W = (int)((float)tk->GTWidth / (float)ViewSizeX *
		  (float)tk->ScreenWidth);
	H = (int)((float)tk->GTHeight / (float)ViewSizeY *
		  (float)tk->ScreenHeight);
	lf.lfWidth = W;
	lf.lfHeight = H;
	TempFont = CreateFontIndirect(&lf);		
	TempOld = SelectObject(tk->MemDC,TempFont);
	W = (int)((float)(tk->GTWidth + tk->GTSpacing) /
		  (float)ViewSizeX *
		  (float)tk->ScreenWidth);
	GetTextMetrics(tk->MemDC, &Metrics);
	if (W < Metrics.tmAveCharWidth)
	  W = Metrics.tmAveCharWidth;
	H = Metrics.tmHeight;
	for (i=0; i <= tk->GTLen; i++)
	  Dx[i] = W;
	SetTextAlign(tk->MemDC,TA_LEFT | TA_BASELINE | TA_NOUPDATECP);
	ExtTextOut(tk->MemDC,tk->CaretX,tk->CaretY,0,NULL,tk->GTBuff,tk->GTLen,&Dx[0]);
	SetTextAlign(tk->MemDC,TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);
	SelectObject(tk->MemDC,TempOld);
	DeleteObject(TempFont);
	switch (tk->GTAngle) {
	  case 0:
	    R.left = tk->CaretX;
	    R.top = tk->CaretY - H;
	    R.right = tk->CaretX + tk->GTLen*W;
	    R.bottom = tk->CaretY + H;
	    tk->CaretX = R.right;
	    break;
	  case 1:
	    R.left = tk->CaretX - H;
	    R.top = tk->CaretY - tk->GTLen*W;
	    R.right = tk->CaretX + H;
	    R.bottom = tk->CaretY;
	    tk->CaretY = R.top;
	    break;
	  case 2:
	    R.left = tk->CaretX - tk->GTLen*W;
	    R.top = tk->CaretY - H;
	    R.right = tk->CaretX;
	    R.bottom = tk->CaretY + H;
	    tk->CaretX = R.left;
	    break;
	  case 3:
	    R.left = tk->CaretX - H;
	    R.top = tk->CaretY;
	    R.right = tk->CaretX + H;
	    R.bottom = tk->CaretY + tk->GTLen*W;
	    tk->CaretY = R.bottom;
	    break;
	}
	InvalidateRect(tk->HWin,&R,FALSE);
	break;
      }
      return;
  }
  tk->ParseMode = ModeFirst;
}
