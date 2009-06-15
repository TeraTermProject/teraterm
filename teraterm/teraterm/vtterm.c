/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, VT terminal emulation */
#include "teraterm.h"
#include "tttypes.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
#include <locale.h>

#include "buffer.h"
#include "ttwinman.h"
#include "ttcommon.h"
#include "commlib.h"
#include "vtdisp.h"
#include "keyboard.h"
#include "ttlib.h"
#include "ttftypes.h"
#include "filesys.h"
#include "teraprn.h"
#include "telnet.h"

#include "vtterm.h"

#define MAPSIZE(x) (sizeof(x)/sizeof((x)[0]))

  /* Parsing modes */
#define ModeFirst 0
#define ModeESC   1
#define ModeDCS   2
#define ModeDCUserKey 3
#define ModeSOS   4
#define ModeCSI   5
#define ModeXS    6
#define ModeDLE   7
#define ModeCAN   8

#define NParamMax 16
#define IntCharMax 5

void VisualBell();

/* character attribute */
static TCharAttr CharAttr;

/* various modes of VT emulation */
static BOOL RelativeOrgMode;
static BOOL InsertMode;
static BOOL LFMode;
static BOOL AutoWrapMode;
static BOOL FocusReportMode;
int MouseReportMode;

// save/restore cursor
typedef struct {
  int CursorX, CursorY;
  TCharAttr Attr;
  int Glr[2], Gn[4]; // G0-G3, GL & GR
  BOOL AutoWrapMode;
  BOOL RelativeOrgMode;
} TStatusBuff;
typedef TStatusBuff *PStatusBuff;

// status buffer for main screen & status line
static TStatusBuff SBuff1, SBuff2;

static BOOL ESCFlag, JustAfterESC;
static BOOL KanjiIn;
static BOOL EUCkanaIn, EUCsupIn;
static int  EUCcount;
static BOOL Special;

static int Param[NParamMax+1];
static int NParam;
static BOOL FirstPrm;
static BYTE IntChar[IntCharMax+1];
static int ICount;
static BYTE Prv;
static int ParseMode, SavedMode;
static int ChangeEmu;

/* user defined keys */
static BOOL WaitKeyId, WaitHi;

/* GL, GR code group */
static int Glr[2];
/* G0, G1, G2, G3 code group */
static int Gn[4];
/* GL for single shift 2/3 */
static int GLtmp;
/* single shift 2/3 flag */
static BOOL SSflag;
/* JIS -> SJIS conversion flag */
static BOOL ConvJIS;
static WORD Kanji;

// variables for status line mode
static int StatusX=0;
static BOOL StatusWrap=FALSE;
static BOOL StatusCursor=TRUE;
static int MainX, MainY; //cursor registers
static int MainTop, MainBottom; // scroll region registers
static BOOL MainWrap;
static BOOL MainCursor=TRUE;

/* status for printer escape sequences */
static BOOL PrintEX = TRUE;  // printing extent
			    // (TRUE: screen, FALSE: scroll region)
static BOOL AutoPrintMode = FALSE;
static BOOL PrinterMode = FALSE;
static BOOL DirectPrn = FALSE;

/* User key */
static BYTE NewKeyStr[FuncKeyStrMax];
static int NewKeyId, NewKeyLen;

static _locale_t CLocale = NULL;

void ResetSBuffers()
{
  SBuff1.CursorX = 0;
  SBuff1.CursorY = 0;
  SBuff1.Attr = DefCharAttr;
  if (ts.Language==IdJapanese)
  {
    SBuff1.Gn[0] = IdASCII;
    SBuff1.Gn[1] = IdKatakana;
    SBuff1.Gn[2] = IdKatakana;
    SBuff1.Gn[3] = IdKanji;
    SBuff1.Glr[0] = 0;
    if ((ts.KanjiCode==IdJIS) &&
	(ts.JIS7Katakana==0))
      SBuff1.Glr[1] = 2;  // 8-bit katakana
    else
      SBuff1.Glr[1] = 3;
  }
  else {
    SBuff1.Gn[0] = IdASCII;
    SBuff1.Gn[1] = IdSpecial;
    SBuff1.Gn[2] = IdASCII;
    SBuff1.Gn[3] = IdASCII;
    SBuff1.Glr[0] = 0;
    SBuff1.Glr[1] = 0;
  }
  SBuff1.AutoWrapMode = TRUE;
  SBuff1.RelativeOrgMode = FALSE;
  // copy SBuff1 to SBuff2
  SBuff2 = SBuff1;
}

void ResetTerminal() /*reset variables but don't update screen */
{
  DispReset();
  BuffReset();

  /* Attribute */
  CharAttr = DefCharAttr;
  Special = FALSE;
  BuffSetCurCharAttr(CharAttr);

  /* Various modes */
  InsertMode = FALSE;
  LFMode = (ts.CRSend == IdCRLF);
  AutoWrapMode = TRUE;
  AppliKeyMode = FALSE;
  AppliCursorMode = FALSE;
  RelativeOrgMode = FALSE;
  ts.ColorFlag &= ~CF_REVERSEVIDEO;
  AutoRepeatMode = TRUE;
  Send8BitMode = ts.Send8BitCtrl;
  FocusReportMode = FALSE;
  MouseReportMode = IdMouseTrackNone;

  if (CLocale == NULL) {
    CLocale = _create_locale(LC_ALL, "C");
  }

  /* Character sets */
  ResetCharSet();

  /* ESC flag for device control sequence */
  ESCFlag = FALSE;
  /* for TEK sequence */
  JustAfterESC = FALSE;

  /* Parse mode */
  ParseMode = ModeFirst;

  /* Clear printer mode */
  PrinterMode = FALSE;

  // status buffers
  ResetSBuffers();
}

void ResetCharSet()
{
  if (ts.Language==IdJapanese)
  {
    Gn[0] = IdASCII;
    Gn[1] = IdKatakana;
    Gn[2] = IdKatakana;
    Gn[3] = IdKanji;
    Glr[0] = 0;
    if ((ts.KanjiCode==IdJIS) &&
	(ts.JIS7Katakana==0))
      Glr[1] = 2;  // 8-bit katakana
    else
      Glr[1] = 3;
  }
  else {
    Gn[0] = IdASCII;
    Gn[1] = IdSpecial;
    Gn[2] = IdASCII;
    Gn[3] = IdASCII;
    Glr[0] = 0;
    Glr[1] = 0;
    cv.SendCode = IdASCII;
    cv.SendKanjiFlag = FALSE;
    cv.EchoCode = IdASCII;
    cv.EchoKanjiFlag = FALSE;
  }
  /* Kanji flag */
  KanjiIn = FALSE;
  EUCkanaIn = FALSE;
  EUCsupIn = FALSE;
  SSflag = FALSE;

  cv.Language = ts.Language;
  cv.CRSend = ts.CRSend;
  cv.KanjiCodeEcho = ts.KanjiCode;
  cv.JIS7KatakanaEcho = ts.JIS7Katakana;
  cv.KanjiCodeSend = ts.KanjiCodeSend;
  cv.JIS7KatakanaSend = ts.JIS7KatakanaSend;
  cv.KanjiIn = ts.KanjiIn;
  cv.KanjiOut = ts.KanjiOut;
}

void ResetKeypadMode(BOOL DisabledModeOnly)
{
  if (!DisabledModeOnly || ts.DisableAppKeypad) AppliKeyMode = FALSE;
  if (!DisabledModeOnly || ts.DisableAppCursor) AppliCursorMode = FALSE;
}

void MoveToMainScreen()
{
  StatusX = CursorX;
  StatusWrap = Wrap;
  StatusCursor = IsCaretEnabled();

  CursorTop = MainTop;
  CursorBottom = MainBottom;
  Wrap = MainWrap;
  DispEnableCaret(MainCursor);
  MoveCursor(MainX,MainY); // move to main screen
}

void MoveToStatusLine()
{
  MainX = CursorX;
  MainY = CursorY;
  MainTop = CursorTop;
  MainBottom = CursorBottom;
  MainWrap = Wrap;
  MainCursor = IsCaretEnabled();

  DispEnableCaret(StatusCursor);
  MoveCursor(StatusX,NumOfLines-1); // move to status line
  CursorTop = NumOfLines-1;
  CursorBottom = CursorTop;
  Wrap = StatusWrap;
}

void HideStatusLine()
{
  if ((StatusLine>0) &&
      (CursorY==NumOfLines-1))
    MoveToMainScreen();
  StatusX = 0;
  StatusWrap = FALSE;
  StatusCursor = TRUE;
  ShowStatusLine(0); //hide
}

void ChangeTerminalSize(int Nx, int Ny)
{
  BuffChangeTerminalSize(Nx,Ny);
  StatusX = 0;
  MainX = 0;
  MainY = 0;
  MainTop = 0;
  MainBottom = NumOfColumns-1;
}

void SendCSIstr(char *str, int len) {
	if (str == NULL || len <= 0)
		return;

	if (Send8BitMode)
		CommBinaryOut(&cv,"\233", 1);
	else
		CommBinaryOut(&cv,"\033[", 2);

	CommBinaryOut(&cv, str, len);
}

void SendOSCstr(char *str, int len) {
	if (str == NULL || len <= 0)
		return;

	if (Send8BitMode)
		CommBinaryOut(&cv,"\235", 1);
	else
		CommBinaryOut(&cv,"\033]", 2);

	CommBinaryOut(&cv, str, len);
}

void BackSpace()
{
  if (CursorX == 0)
  {
    if ((CursorY>0) &&
	((ts.TermFlag & TF_BACKWRAP)!=0))
    {
      MoveCursor(NumOfColumns-1,CursorY-1);
//      if (cv.HLogBuf!=0) Log1Byte(BS);
// (2005.2.20 yutaka)
	  if (cv.HLogBuf!=0 && !ts.LogTypePlainText) Log1Byte(BS);
    }
  }
  else if (CursorX > 0)
  {
    MoveCursor(CursorX-1,CursorY);
//    if (cv.HLogBuf!=0) Log1Byte(BS);
// (2005.2.20 yutaka)
	  if (cv.HLogBuf!=0 && !ts.LogTypePlainText) Log1Byte(BS);
  }
}

void CarriageReturn(BOOL logFlag)
{
#ifndef NO_COPYLINE_FIX
	if (!ts.EnableContinuedLineCopy || logFlag)
#endif /* NO_COPYLINE_FIX */
        if (cv.HLogBuf!=0) Log1Byte(CR);

	if (CursorX>0)
		MoveCursor(0,CursorY);
}

void LineFeed(BYTE b, BOOL logFlag)
{
	/* for auto print mode */
	if ((AutoPrintMode) &&
		(b>=LF) && (b<=FF))
		BuffDumpCurrentLine(b);

#ifndef NO_COPYLINE_FIX
	if (!ts.EnableContinuedLineCopy || logFlag)
#endif /* NO_COPYLINE_FIX */
	if (cv.HLogBuf!=0) Log1Byte(LF);

	if (CursorY < CursorBottom)
		MoveCursor(CursorX,CursorY+1);
	else if (CursorY == CursorBottom) BuffScrollNLines(1);
	else if (CursorY < NumOfLines-StatusLine-1)
		MoveCursor(CursorX,CursorY+1);

#ifndef NO_COPYLINE_FIX
	ClearLineContinued();
#endif /* NO_COPYLINE_FIX */

	if (LFMode) CarriageReturn(logFlag);
}

void Tab()
{
  if (Wrap && !ts.VTCompatTab) {
      CarriageReturn(FALSE);
      LineFeed(LF,FALSE);
#ifndef NO_COPYLINE_FIX
      if (ts.EnableContinuedLineCopy) {
	SetLineContinued();
      }
#endif /* NO_COPYLINE_FIX */
      Wrap = FALSE;
  }
  CursorForwardTab(1, AutoWrapMode);
  if (cv.HLogBuf!=0) Log1Byte(HT);
}

void PutChar(BYTE b)
{
  BOOL SpecialNew;
  TCharAttr CharAttrTmp;

  CharAttrTmp = CharAttr;

  if (PrinterMode) { // printer mode
    WriteToPrnFile(b,TRUE);
    return;
  }

  if (Wrap)
  {
    CarriageReturn(FALSE);
    LineFeed(LF,FALSE);
#ifndef NO_COPYLINE_FIX
    CharAttrTmp.Attr |= ts.EnableContinuedLineCopy ? AttrLineContinued : 0;
#endif /* NO_COPYLINE_FIX */
  }

//  if (cv.HLogBuf!=0) Log1Byte(b);
// (2005.2.20 yutaka)
  if (ts.LogTypePlainText) {
	  if (__isascii(b) && !isprint(b)) {
		  // ASCII文字で、非表示な文字はログ採取しない。
	  } else {
		if (cv.HLogBuf!=0) Log1Byte(b);
	  }
  } else {
	  if (cv.HLogBuf!=0) Log1Byte(b);
  }

  Wrap = FALSE;

  SpecialNew = FALSE;
  if ((b>0x5F) && (b<0x80))
  {
    if (SSflag)
      SpecialNew = (Gn[GLtmp]==IdSpecial);
    else
      SpecialNew = (Gn[Glr[0]]==IdSpecial);
  }
  else if (b>0xDF)
  {
    if (SSflag)
      SpecialNew = (Gn[GLtmp]==IdSpecial);
    else
      SpecialNew = (Gn[Glr[1]]==IdSpecial);
  }

  if (SpecialNew != Special)
  {
    UpdateStr();
    Special = SpecialNew;
  }

  if (Special)
  {
    b = b & 0x7F;
    CharAttrTmp.Attr |= AttrSpecial;
  }
  else
    CharAttrTmp.Attr |= CharAttr.Attr;

  BuffPutChar(b, CharAttrTmp, InsertMode);

  if (CursorX < NumOfColumns-1)
    MoveRight();
  else {
    UpdateStr();
    Wrap = AutoWrapMode;
  }
}

void PutDecSp(BYTE b)
{
  TCharAttr CharAttrTmp;

  CharAttrTmp = CharAttr;

  if (PrinterMode) { // printer mode
    WriteToPrnFile(b, TRUE);
    return;
  }

  if (Wrap) {
    CarriageReturn(FALSE);
    LineFeed(LF, FALSE);
#ifndef NO_COPYLINE_FIX
    CharAttrTmp.Attr |= ts.EnableContinuedLineCopy ? AttrLineContinued : 0;
#endif /* NO_COPYLINE_FIX */
  }

  if (cv.HLogBuf!=0) Log1Byte(b);
/*
  if (ts.LogTypePlainText && __isascii(b) && !isprint(b)) {
    // ASCII文字で、非表示な文字はログ採取しない。
  } else {
    if (cv.HLogBuf!=0) Log1Byte(b);
  }
 */

  Wrap = FALSE;

  if (!Special) {
    UpdateStr();
    Special = TRUE;
  }

  CharAttrTmp.Attr |= AttrSpecial;
  BuffPutChar(b, CharAttrTmp, InsertMode);

  if (CursorX < NumOfColumns-1)
    MoveRight();
  else {
    UpdateStr();
    Wrap = AutoWrapMode;
  }
}

void PutKanji(BYTE b)
{
#ifndef NO_COPYLINE_FIX
  TCharAttr CharAttrTmp;

  CharAttrTmp = CharAttr;
#endif /* NO_COPYLINE_FIX */
  Kanji = Kanji + b;

  if (PrinterMode && DirectPrn)
  {
    WriteToPrnFile(HIBYTE(Kanji),FALSE);
    WriteToPrnFile(LOBYTE(Kanji),TRUE);
    return;
  }

  if (ConvJIS)
    Kanji = JIS2SJIS((WORD)(Kanji & 0x7f7f));

  if (PrinterMode) { // printer mode
    WriteToPrnFile(HIBYTE(Kanji),FALSE);
    WriteToPrnFile(LOBYTE(Kanji),TRUE);
    return;
  }

  if (Wrap)
  {
    CarriageReturn(FALSE);
    LineFeed(LF,FALSE);
#ifndef NO_COPYLINE_FIX
    if (ts.EnableContinuedLineCopy)
      CharAttrTmp.Attr |= AttrLineContinued;
#endif /* NO_COPYLINE_FIX */
  }
  else if (CursorX > NumOfColumns-2)
    if (AutoWrapMode)
    {
#ifndef NO_COPYLINE_FIX
      if (ts.EnableContinuedLineCopy)
      {
      CharAttrTmp.Attr |= AttrLineContinued;
      if (CursorX == NumOfColumns-1)
	BuffPutChar(0x20, CharAttr, FALSE);
      }
#endif /* NO_COPYLINE_FIX */
      CarriageReturn(FALSE);
      LineFeed(LF,FALSE);
    }
    else return;

  Wrap = FALSE;

  if (cv.HLogBuf!=0)
  {
    Log1Byte(HIBYTE(Kanji));
    Log1Byte(LOBYTE(Kanji));
  }

  if (Special)
  {
    UpdateStr();
    Special = FALSE;
  }

#ifndef NO_COPYLINE_FIX
  BuffPutKanji(Kanji, CharAttrTmp, InsertMode);
#else
  BuffPutKanji(Kanji, CharAttr, InsertMode);
#endif /* NO_COPYLINE_FIX */

  if (CursorX < NumOfColumns-2)
  {
    MoveRight();
    MoveRight();
  }
  else {
    UpdateStr();
    Wrap = AutoWrapMode;
  }
}

void PutDebugChar(BYTE b)
{
  InsertMode = FALSE;
  AutoWrapMode = TRUE;

  if ((b & 0x80) == 0x80)
  {
    UpdateStr();
    CharAttr.Attr = AttrReverse;
    b = b & 0x7f;
  }

  if (b<=US)
  {
    PutChar('^');
    PutChar((char)(b+0x40));
  }
  else if (b==DEL)
  {
    PutChar('<');
    PutChar('D');
    PutChar('E');
    PutChar('L');
    PutChar('>');
  }
  else
    PutChar(b);

  if (CharAttr.Attr != AttrDefault)
  {
    UpdateStr();
    CharAttr.Attr = AttrDefault;
  }
}

void PrnParseControl(BYTE b) // printer mode
{
  switch (b) {
    case NUL: return;
    case SO:
      if (! DirectPrn)
      {
	if ((ts.Language==IdJapanese) &&
	    (ts.KanjiCode==IdJIS) &&
	    (ts.JIS7Katakana==1) &&
	    ((ts.TermFlag & TF_FIXEDJIS)!=0))
	  Gn[1] = IdKatakana;
	Glr[0] = 1; /* LS1 */
	return;
      }
      break;
    case SI:
      if (! DirectPrn)
      {
	Glr[0] = 0; /* LS0 */
	return;
      }
      break;
    case DC1:
    case DC3: return;
    case ESC:
      ICount = 0;
      JustAfterESC = TRUE;
      ParseMode = ModeESC;
      WriteToPrnFile(0,TRUE); // flush prn buff
      return;
    case CSI:
      if ((ts.TerminalID<IdVT220J) ||
	  ((ts.TermFlag & TF_ACCEPT8BITCTRL)==0))
      {
	PutChar(b); /* Disp C1 char in VT100 mode */
	return;
      }
      ICount = 0;
      FirstPrm = TRUE;
      NParam = 1;
      Param[1] = -1;
      Prv = 0;
      ParseMode = ModeCSI;
      WriteToPrnFile(0,TRUE); // flush prn buff
      WriteToPrnFile(b,FALSE);
      return;
  }
  /* send the uninterpreted character to printer */
  WriteToPrnFile(b,TRUE);
}

void ParseControl(BYTE b)
{
  if (PrinterMode) { // printer mode
    PrnParseControl(b);
    return;
  }

  if (b>=0x80) /* C1 char */
  {
    /* English mode */
    if (ts.Language==IdEnglish)
    {
      if ((ts.TerminalID<IdVT220J) ||
	  ((ts.TermFlag & TF_ACCEPT8BITCTRL)==0))
      {
	PutChar(b); /* Disp C1 char in VT100 mode */
	return;
      }
    }
    else { /* Japanese mode */
      if ((ts.TermFlag & TF_ACCEPT8BITCTRL)==0)
	return; /* ignore C1 char */
      /* C1 chars are interpreted as C0 chars in VT100 mode */
      if (ts.TerminalID<IdVT220J)
	b = b & 0x7F;
    }
  }
  switch (b) {
    /* C0 group */
    case ENQ:
      CommBinaryOut(&cv,&(ts.Answerback[0]),ts.AnswerbackLen);
      break;
    case BEL:
      switch (ts.Beep) {
      case IdBeepOff:
	/* nothing to do */
        break;
      case IdBeepOn:
        MessageBeep(0);
        break;
      case IdBeepVisual:
	VisualBell();
        break;
      }
      break;
    case BS: BackSpace(); break;
    case HT: Tab(); break;

    case LF:
		// 受信時の改行コードが LF の場合は、サーバから LF のみが送られてくると仮定し、
		// CR+LFとして扱うようにする。
		// cf. http://www.neocom.ca/forum/viewtopic.php?t=216
		// (2007.1.21 yutaka)
		if (ts.CRReceive == IdLF) {
			CarriageReturn(TRUE);
			LineFeed(b, TRUE);
			break;
		}

    case VT: LineFeed(b,TRUE); break;

    case FF:
      if ((ts.AutoWinSwitch>0) && JustAfterESC)
      {
	CommInsert1Byte(&cv,b);
	CommInsert1Byte(&cv,ESC);
	ChangeEmu = IdTEK;  /* Enter TEK Mode */
      }
      else
	LineFeed(b,TRUE);
      break;
    case CR:
      CarriageReturn(TRUE);
      if (ts.CRReceive==IdCRLF)
	CommInsert1Byte(&cv,LF);
      break;
    case SO:
      if ((ts.Language==IdJapanese) &&
	  (ts.KanjiCode==IdJIS) &&
	  (ts.JIS7Katakana==1) &&
	  ((ts.TermFlag & TF_FIXEDJIS)!=0))
	Gn[1] = IdKatakana;

      Glr[0] = 1; /* LS1 */
      break;
    case SI: Glr[0] = 0; break; /* LS0 */
    case DLE:
      if ((ts.FTFlag & FT_BPAUTO)!=0)
	ParseMode = ModeDLE; /* Auto B-Plus activation */
      break;
    case CAN:
      if ((ts.FTFlag & FT_ZAUTO)!=0)
	ParseMode = ModeCAN; /* Auto ZMODEM activation */
//	else if (ts.AutoWinSwitch>0)
//		ChangeEmu = IdTEK;  /* Enter TEK Mode */
      else
	ParseMode = ModeFirst;
      break;
    case SUB: ParseMode = ModeFirst; break;
    case ESC:
      ICount = 0;
      JustAfterESC = TRUE;
      ParseMode = ModeESC;
      break;
    case FS:
    case GS:
    case RS:
    case US:
      if (ts.AutoWinSwitch>0)
      {
	CommInsert1Byte(&cv,b);
	ChangeEmu = IdTEK;  /* Enter TEK Mode */
      }
      break;

    /* C1 char */
    case IND: LineFeed(0,TRUE); break;
    case NEL:
      LineFeed(0,TRUE);
      CarriageReturn(TRUE);
      break;
    case HTS: SetTabStop(); break;
    case RI: CursorUpWithScroll(); break;
    case SS2:
      GLtmp = 2;
      SSflag = TRUE;
      break;
    case SS3:
      GLtmp = 3;
      SSflag = TRUE;
      break;
    case DCS:
      SavedMode = ParseMode;
      ESCFlag = FALSE;
      NParam = 1;
      Param[1] = -1;
      ParseMode = ModeDCS;
      break;
    case SOS:
      SavedMode = ParseMode;
      ESCFlag = FALSE;
      ParseMode = ModeSOS;
      break;
    case CSI:
      ICount = 0;
      FirstPrm = TRUE;
      NParam = 1;
      Param[1] = -1;
      Prv = 0;
      ParseMode = ModeCSI;
      break;
    case OSC:
      Param[1] = 0;
      ParseMode = ModeXS;
      break;
    case PM:
    case APC:
      SavedMode = ParseMode;
      ESCFlag = FALSE;
      ParseMode = ModeSOS;
      break;
  }
}

void SaveCursor()
{
  int i;
  PStatusBuff Buff;

  if ((StatusLine>0) &&
      (CursorY==NumOfLines-1))
    Buff = &SBuff2; // for status line
  else
    Buff = &SBuff1; // for main screen

  Buff->CursorX = CursorX;
  Buff->CursorY = CursorY;
  Buff->Attr = CharAttr;
  Buff->Glr[0] = Glr[0];
  Buff->Glr[1] = Glr[1];
  for (i=0 ; i<=3; i++)
    Buff->Gn[i] = Gn[i];
  Buff->AutoWrapMode = AutoWrapMode;
  Buff->RelativeOrgMode = RelativeOrgMode;
}

void  RestoreCursor()
{
  int i;
  PStatusBuff Buff;
  UpdateStr();

  if ((StatusLine>0) &&
      (CursorY==NumOfLines-1))
    Buff = &SBuff2; // for status line
  else
    Buff = &SBuff1; // for main screen

  if (Buff->CursorX > NumOfColumns-1)
    Buff->CursorX = NumOfColumns-1;
  if (Buff->CursorY > NumOfLines-1-StatusLine)
    Buff->CursorY = NumOfLines-1-StatusLine;
  MoveCursor(Buff->CursorX,Buff->CursorY);
  CharAttr = Buff->Attr;
  Glr[0] = Buff->Glr[0];
  Glr[1] = Buff->Glr[1];
  for (i=0 ; i<=3; i++)
    Gn[i] = Buff->Gn[i];
  AutoWrapMode = Buff->AutoWrapMode;
  RelativeOrgMode = Buff->RelativeOrgMode;
}

void AnswerTerminalType()
{
  char Tmp[31];

  if (ts.TerminalID<IdVT320)
    strncpy_s(Tmp, sizeof(Tmp),"\033[?", _TRUNCATE);
  else
    strncpy_s(Tmp, sizeof(Tmp),"\233?", _TRUNCATE);

  switch (ts.TerminalID) {
    case IdVT100:
      strncat_s(Tmp,sizeof(Tmp),"1;2",_TRUNCATE);
      break;
    case IdVT100J:
      strncat_s(Tmp,sizeof(Tmp),"5;2",_TRUNCATE);
      break;
    case IdVT101:
      strncat_s(Tmp,sizeof(Tmp),"1;0",_TRUNCATE);
      break;
    case IdVT102:
      strncat_s(Tmp,sizeof(Tmp),"6",_TRUNCATE);
      break;
    case IdVT102J:
      strncat_s(Tmp,sizeof(Tmp),"15",_TRUNCATE);
      break;
    case IdVT220J:
      strncat_s(Tmp,sizeof(Tmp),"62;1;2;5;6;7;8",_TRUNCATE);
      break;
    case IdVT282:
      strncat_s(Tmp,sizeof(Tmp),"62;1;2;4;5;6;7;8;10;11",_TRUNCATE);
      break;
    case IdVT320:
      strncat_s(Tmp,sizeof(Tmp),"63;1;2;6;7;8",_TRUNCATE);
      break;
    case IdVT382:
      strncat_s(Tmp,sizeof(Tmp),"63;1;2;4;5;6;7;8;10;15",_TRUNCATE);
      break;
  }
  strncat_s(Tmp,sizeof(Tmp),"c",_TRUNCATE);

  CommBinaryOut(&cv,Tmp,strlen(Tmp)); /* Report terminal ID */
}

void ESCSpace(BYTE b)
{
  switch (b) {
    case 'F': Send8BitMode = FALSE; break;	// S7C1T
    case 'G': Send8BitMode = TRUE; break;	// S8C1T
  }
}

void ESCSharp(BYTE b)
{
  switch (b) {
    case '8':  /* Fill screen with "E" */
      BuffUpdateScroll();
      BuffFillWithE();
      MoveCursor(0,0);
      ParseMode = ModeFirst;
      break;
  }
}

/* select double byte code set */
void ESCDBCSSelect(BYTE b)
{
  int Dist;

  if (ts.Language!=IdJapanese) return;

  switch (ICount) {
    case 1:
      if ((b=='@') || (b=='B'))
      {
	Gn[0] = IdKanji; /* Kanji -> G0 */
	if ((ts.TermFlag & TF_AUTOINVOKE)!=0)
	  Glr[0] = 0; /* G0->GL */
      }
      break;
    case 2:
      /* Second intermediate char must be
	 '(' or ')' or '*' or '+'. */
      Dist = (IntChar[2]-'(') & 3; /* G0 - G3 */
      if ((b=='1') || (b=='3') ||
	  (b=='@') || (b=='B'))
      {
	Gn[Dist] = IdKanji; /* Kanji -> G0-3 */
	if (((ts.TermFlag & TF_AUTOINVOKE)!=0) &&
	    (Dist==0))
	  Glr[0] = 0; /* G0->GL */
      }
      break;
  }
}

void ESCSelectCode(BYTE b)
{
  switch (b) {
    case '0':
      if (ts.AutoWinSwitch>0)
	ChangeEmu = IdTEK; /* enter TEK mode */
      break;
  }
}

  /* select single byte code set */
void ESCSBCSSelect(BYTE b)
{
  int Dist;

  /* Intermediate char must be
     '(' or ')' or '*' or '+'.	*/
  Dist = (IntChar[1]-'(') & 3; /* G0 - G3 */

  switch (b) {
    case '0': Gn[Dist] = IdSpecial; break;
    case '<': Gn[Dist] = IdASCII; break;
    case '>': Gn[Dist] = IdASCII; break;
    case 'A': Gn[Dist] = IdASCII; break;
    case 'B': Gn[Dist] = IdASCII; break;
    case 'H': Gn[Dist] = IdASCII; break;
    case 'I':
      if (ts.Language==IdJapanese)
	Gn[Dist] = IdKatakana;
      break;
    case 'J': Gn[Dist] = IdASCII; break;
  }

  if (((ts.TermFlag & TF_AUTOINVOKE)!=0) &&
      (Dist==0))
    Glr[0] = 0;  /* G0->GL */
}

void PrnParseEscape(BYTE b) // printer mode
{
  int i;

  ParseMode = ModeFirst;
  switch (ICount) {
    /* no intermediate char */
    case 0:
      switch (b) {
	case '[': /* CSI */
	  ICount = 0;
	  FirstPrm = TRUE;
	  NParam = 1;
	  Param[1] = -1;
	  Prv = 0;
	  WriteToPrnFile(ESC,FALSE);
	  WriteToPrnFile('[',FALSE);
	  ParseMode = ModeCSI;
	  return;
      } /* end of case Icount=0 */
      break;
    /* one intermediate char */
    case 1:
      switch (IntChar[1]) {
	case '$':
	  if (! DirectPrn)
	  {
	    ESCDBCSSelect(b);
	    return;
	  }
	  break;
	case '(':
	case ')':
	case '*':
	case '+':
	  if (! DirectPrn)
	  {
	    ESCSBCSSelect(b);
	    return;
	  }
	  break;
      }
      break;
    /* two intermediate char */
    case 2:
      if ((! DirectPrn) &&
	  (IntChar[1]=='$') &&
	  ('('<=IntChar[2]) &&
	  (IntChar[2]<='+'))
      {
	ESCDBCSSelect(b);
	return;
      }
      break;
  }
  // send the uninterpreted sequence to printer
  WriteToPrnFile(ESC,FALSE);
  for (i=1; i<=ICount; i++)
    WriteToPrnFile(IntChar[i],FALSE);
  WriteToPrnFile(b,TRUE);
}

void ParseEscape(BYTE b) /* b is the final char */
{
  if (PrinterMode) { // printer mode
    PrnParseEscape(b);
    return;
  }

  switch (ICount) {
    /* no intermediate char */
    case 0:
      switch (b) {
	case '7': SaveCursor(); break;
	case '8': RestoreCursor(); break;
	case '=': AppliKeyMode = TRUE; break;
	case '>': AppliKeyMode = FALSE; break;
	case 'D': /* IND */
	  LineFeed(0,TRUE);
	  break;
	case 'E': /* NEL */
	  MoveCursor(0,CursorY);
	  LineFeed(0,TRUE);
	  break;
	case 'H': /* HTS */
	  SetTabStop();
	  break;
	case 'M': /* RI */
	  CursorUpWithScroll();
	  break;
	case 'N': /* SS2 */
	  GLtmp = 2;
	  SSflag = TRUE;
	  break;
	case 'O': /* SS3 */
	  GLtmp = 3;
	  SSflag = TRUE;
	  break;
	case 'P': /* DCS */
	  SavedMode = ParseMode;
	  ESCFlag = FALSE;
	  NParam = 1;
	  Param[1] = -1;
	  ParseMode = ModeDCS;
	  return;
	case 'X': /* SOS */
	  SavedMode = ParseMode;
	  ESCFlag = FALSE;
	  ParseMode = ModeSOS;
	  return;
	case 'Z': AnswerTerminalType(); break;
	case '[': /* CSI */
	  ICount = 0;
	  FirstPrm = TRUE;
	  NParam = 1;
	  Param[1] = -1;
	  Prv = 0;
	  ParseMode = ModeCSI;
	  return;
	case '\\': break; /* ST */
	case ']': /* XTERM sequence (OSC) */
	  NParam = 1;
	  Param[1] = 0;
	  ParseMode = ModeXS;
	  return;
	case '^':
	case '_': /* PM, APC */
	  SavedMode = ParseMode;
	  ESCFlag = FALSE;
	  ParseMode = ModeSOS;
	  return;
	case 'c': /* Hardware reset */
	  HideStatusLine();
	  ResetTerminal();
	  ClearUserKey();
	  ClearBuffer();
	  if (ts.PortType==IdSerial) // reset serial port
	    CommResetSerial(&ts, &cv, TRUE);
	  break;
	case 'g': /* Visual Bell (screen original?) */
	  VisualBell();
	  break;
	case 'n': Glr[0] = 2; break; /* LS2 */
	case 'o': Glr[0] = 3; break; /* LS3 */
	case '|': Glr[1] = 3; break; /* LS3R */
	case '}': Glr[1] = 2; break; /* LS2R */
	case '~': Glr[1] = 1; break; /* LS1R */
      } /* end of case Icount=0 */
      break;
    /* one intermediate char */
    case 1:
      switch (IntChar[1]) {
	case ' ': ESCSpace(b); break;
	case '#': ESCSharp(b); break;
	case '$': ESCDBCSSelect(b); break;
	case '%': break;
	case '(':
	case ')':
	case '*':
	case '+':
	  ESCSBCSSelect(b);
	  break;
      }
      break;
    /* two intermediate char */
    case 2:
      if ((IntChar[1]=='$') &&
	  ('('<=IntChar[2]) &&
	  (IntChar[2]<='+'))
	ESCDBCSSelect(b);
      else if ((IntChar[1]=='%') &&
	       (IntChar[2]=='!'))
	ESCSelectCode(b);
      break;
  }
  ParseMode = ModeFirst;
}

void EscapeSequence(BYTE b)
{
  if (b<=US)
    ParseControl(b);
  else if ((b>=0x20) && (b<=0x2F))
  {
    if (ICount<IntCharMax) ICount++;
    IntChar[ICount] = b;
  }
  else if ((b>=0x30) && (b<=0x7E))
    ParseEscape(b);
  else if ((b>=0x80) && (b<=0x9F))
    ParseControl(b);

  JustAfterESC = FALSE;
}

  void CSInsertCharacter()
  {
  // Insert space characters at cursor
    int Count;

    BuffUpdateScroll();
    if (Param[1]<1) Param[1] = 1;
    Count = Param[1];
    BuffInsertSpace(Count);
  }

  void CSCursorUp()
  {
    if (Param[1]<1) Param[1] = 1;

    if (CursorY >= CursorTop)
    {
      if (CursorY-Param[1] > CursorTop)
	MoveCursor(CursorX,CursorY-Param[1]);
      else
	MoveCursor(CursorX,CursorTop);
    }
    else {
      if (CursorY > 0)
	MoveCursor(CursorX,CursorY-Param[1]);
      else
	MoveCursor(CursorX,0);
    }
  }

  void CSCursorUp1()
  {
    MoveCursor(0,CursorY);
    CSCursorUp();
  }

  void CSCursorDown()
  {
    if (Param[1]<1) Param[1] = 1;

    if (CursorY <= CursorBottom)
    {
      if (CursorY+Param[1] < CursorBottom)
	MoveCursor(CursorX,CursorY+Param[1]);
      else
	MoveCursor(CursorX,CursorBottom);
    }
    else {
      if (CursorY < NumOfLines-StatusLine-1)
	MoveCursor(CursorX,CursorY+Param[1]);
      else
	MoveCursor(CursorX,NumOfLines-StatusLine);
    }
  }

  void CSCursorDown1()
  {
    MoveCursor(0,CursorY);
    CSCursorDown();
  }

void CSScreenErase()
{
	if (Param[1] == -1) Param[1] = 0;
	BuffUpdateScroll();
	switch (Param[1]) {
	case 0:
		// <ESC>[H(Cursor in left upper corner)によりカーソルが左上隅を指している場合、
		// <ESC>[Jは<ESC>[2Jと同じことなので、処理を分け、現行バッファをスクロールアウト
		// させるようにする。(2005.5.29 yutaka)
		// コンフィグレーションで切り替えられるようにした。(2008.5.3 yutaka)
		if (ts.ScrollWindowClearScreen && 
			(CursorX == 0 && CursorY == 0)) {
			//	Erase screen (scroll out)
			BuffClearScreen();
			UpdateWindow(HVTWin);

		} else {
			//	Erase characters from cursor to the end of screen
			BuffEraseCurToEnd();
		}
		break;

	case 1:
		//	Erase characters from home to cursor
		BuffEraseHomeToCur();
		break;

	case 2:
		//	Erase screen (scroll out)
		BuffClearScreen();
		UpdateWindow(HVTWin);
		break;
	}
}

  void CSInsertLine()
  {
  // Insert lines at current position
    int Count, YEnd;

    if (CursorY < CursorTop) return;
    if (CursorY > CursorBottom) return;
    if (Param[1]<1) Param[1] = 1;
    Count = Param[1];

    YEnd = CursorBottom;
    if (CursorY > YEnd) YEnd = NumOfLines-1-StatusLine;
    if (Count > YEnd+1 - CursorY) Count = YEnd+1 - CursorY;

    BuffInsertLines(Count,YEnd);
  }

  void CSLineErase()
  {
    if (Param[1] == -1) Param[1] = 0;
    BuffUpdateScroll();
    switch (Param[1]) {
      /* erase char from cursor to end of line */
      case 0:
	BuffEraseCharsInLine(CursorX,NumOfColumns-CursorX);
	break;
      /* erase char from start of line to cursor */
      case 1:
	BuffEraseCharsInLine(0,CursorX+1);
	break;
      /* erase entire line */
      case 2:
	BuffEraseCharsInLine(0,NumOfColumns);
	break;
    }
  }

  void CSDeleteNLines()
  // Delete lines from current line
  {
    int Count, YEnd;

    if (CursorY < CursorTop) return;
    if (CursorY > CursorBottom) return;
    Count = Param[1];
    if (Count<1) Count = 1;

    YEnd = CursorBottom;
    if (CursorY > YEnd) YEnd = NumOfLines-1-StatusLine;
    if (Count > YEnd+1-CursorY) Count = YEnd+1-CursorY;
    BuffDeleteLines(Count,YEnd);
  }

  void CSDeleteCharacter()
  {
  // Delete characters in current line from cursor

    if (Param[1]<1) Param[1] = 1;
    BuffUpdateScroll();
    BuffDeleteChars(Param[1]);
  }

  void CSEraseCharacter()
  {
    if (Param[1]<1) Param[1] = 1;
    BuffUpdateScroll();
    BuffEraseChars(Param[1]);
  }

  void CSScrollUP()
  {
    if (Param[1]<1) Param[1] = 1;
    BuffUpdateScroll();
    BuffRegionScrollUpNLines(Param[1]);
  }

  void CSScrollDown()
  {
    if (Param[1]<1) Param[1] = 1;
    BuffUpdateScroll();
    BuffRegionScrollDownNLines(Param[1]);
  }

  void CSForwardTab()
  {
    if (Param[1]<1) Param[1] = 1;
    CursorForwardTab(Param[1], AutoWrapMode);
  }

  void CSBackwardTab()
  {
    if (Param[1]<1) Param[1] = 1;
    CursorBackwardTab(Param[1]);
  }

  void CSMoveToColumnN()
  {
    if (Param[1]<1) Param[1] = 1;
    Param[1]--;
    if (Param[1] < 0) Param[1] = 0;
    if (Param[1] > NumOfColumns-1) Param[1] = NumOfColumns-1;
    MoveCursor(Param[1],CursorY);
  }

  void CSCursorRight()
  {
    if (Param[1]<1) Param[1] = 1;
    if (CursorX + Param[1] > NumOfColumns-1)
      MoveCursor(NumOfColumns-1,CursorY);
    else
      MoveCursor(CursorX+Param[1],CursorY);
  }

  void CSCursorLeft()
  {
    if (Param[1]<1) Param[1] = 1;
    if (CursorX-Param[1] < 0)
      MoveCursor(0,CursorY);
    else
      MoveCursor(CursorX-Param[1],CursorY);
  }

  void CSMoveToLineN()
  {
    if (Param[1]<1) Param[1] = 1;
    if (RelativeOrgMode)
    {
      if (CursorTop+Param[1]-1 > CursorBottom)
	MoveCursor(CursorX,CursorBottom);
      else
	MoveCursor(CursorX,CursorTop+Param[1]-1);
    }
    else {
      if (Param[1] > NumOfLines-StatusLine)
	MoveCursor(CursorX,NumOfLines-1-StatusLine);
      else
	MoveCursor(CursorX,Param[1]-1);
    }
  }

  void CSMoveToXY()
  {
    int NewX, NewY;

    if (Param[1]<1) Param[1] = 1;
    if ((NParam < 2) || (Param[2]<1)) Param[2] = 1;
    NewX = Param[2] - 1;
    if (NewX > NumOfColumns-1) NewX = NumOfColumns-1;

    if ((StatusLine>0) && (CursorY==NumOfLines-1))
      NewY = CursorY;
    else if (RelativeOrgMode)
    {
      NewY = CursorTop + Param[1] - 1;
      if (NewY > CursorBottom) NewY = CursorBottom;
    }
    else {
      NewY = Param[1] - 1;
      if (NewY > NumOfLines-1-StatusLine)
	NewY = NumOfLines-1-StatusLine;
    }
    MoveCursor(NewX,NewY);
  }

  void CSDeleteTabStop()
  {
    if (Param[1]==-1) Param[1] = 0;
    ClearTabStop(Param[1]);
  }

  void CS_h_Mode()
  {
    switch (Param[1]) {
      case 2:	// KAM
        KeybEnabled = FALSE; break;
      case 4:	// IRM
        InsertMode = TRUE; break;
      case 12:	// SRM
	ts.LocalEcho = 0;
	if (cv.Ready && cv.TelFlag && (ts.TelEcho>0))
	  TelChangeEcho();
	break;
      case 20:	// LF/NL
	LFMode = TRUE;
	ts.CRSend = IdCRLF;
	cv.CRSend = IdCRLF;
	break;
      case 33:	// WYSTCURM
	ts.NonblinkingCursor = TRUE;
	ChangeCaret();
	break;
      case 34:	// WYULCURM
	ts.CursorShape = IdHCur;
	ChangeCaret();
	break;
    }
  }

  void CS_i_Mode()
  {
    if (Param[1]==-1) Param[1] = 0;
    switch (Param[1]) {
      /* print screen */
	//  PrintEX --	TRUE: print screen
	//		FALSE: scroll region
      case 0: BuffPrint(! PrintEX); break;
      /* printer controller mode off */
      case 4: break; /* See PrnParseCS() */
      /* printer controller mode on */
      case 5:
	if (! AutoPrintMode)
	  OpenPrnFile();
	DirectPrn = (ts.PrnDev[0]!=0);
	PrinterMode = TRUE;
	break;
    }
  }

  void CS_l_Mode()
  {
    switch (Param[1]) {
      case 2:	// KAM
        KeybEnabled = TRUE; break;
      case 4:	// IRM
        InsertMode = FALSE; break;
      case 12:	// SRM
	ts.LocalEcho = 1;
	if (cv.Ready && cv.TelFlag && (ts.TelEcho>0))
	  TelChangeEcho();
	break;
      case 20:	// LF/NL
	LFMode = FALSE;
	ts.CRSend = IdCR;
	cv.CRSend = IdCR;
	break;
      case 33:	// WYSTCURM
	ts.NonblinkingCursor = FALSE;
	ChangeCaret();
	break;
      case 34:	// WYULCURM
	ts.CursorShape = IdBlkCur;
	ChangeCaret();
	break;
    }
  }

  void CS_n_Mode()
  {
    char Report[16];
    int Y, len;

    switch (Param[1]) {
      case 5:
	/* Device Status Report -> Ready */
	SendCSIstr("0n", 2);
	break;
      case 6:
	/* Cursor Position Report */
	Y = CursorY+1;
	if ((StatusLine>0) &&
	    (Y==NumOfLines))
	  Y = 1;
	len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "%u;%uR", CLocale, Y, CursorX+1);
	SendCSIstr(Report, len);
	break;
    }
  }

void CSSetAttr()
{
	int i, P;

	UpdateStr();
	for (i=1 ; i<=NParam ; i++)
	{
		P = Param[i];
		if (P<0) P = 0;
		switch (P) {
		case   0:	/* Clear all */
			CharAttr = DefCharAttr;
			BuffSetCurCharAttr(CharAttr);
			break;

		case   1:	/* Bold */
			CharAttr.Attr |= AttrBold;
			BuffSetCurCharAttr(CharAttr);
			break;

		case   4:	/* Under line */
			CharAttr.Attr |= AttrUnder;
			BuffSetCurCharAttr(CharAttr);
			break;

		case   5:	/* Blink */
			CharAttr.Attr |= AttrBlink;
			BuffSetCurCharAttr(CharAttr);
			break;

		case   7:	/* Reverse */
			CharAttr.Attr |= AttrReverse;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  22:	/* Bold off */
			CharAttr.Attr &= ~ AttrBold;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  24:	/* Under line off */
			CharAttr.Attr &= ~ AttrUnder;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  25:	/* Blink off */
			CharAttr.Attr &= ~ AttrBlink;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  27:	/* Reverse off */
			CharAttr.Attr &= ~ AttrReverse;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  30:
		case  31:
		case  32:
		case  33:
		case  34:
		case  35:
		case  36:
		case  37:	/* text color */
			CharAttr.Attr2 |= Attr2Fore;
			CharAttr.Fore = P - 30;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  38:	/* text color (256color mode) */
			if ((ts.ColorFlag & CF_XTERM256) && i < NParam && Param[i+1] == 5) {
				i++;
				if (i < NParam) {
					P = Param[++i];
					if (P<0) {
						P = 0;
					}
					CharAttr.Attr2 |= Attr2Fore;
					CharAttr.Fore = P;
					BuffSetCurCharAttr(CharAttr);
				}
			}
			break;

		case  39:	/* Reset text color */
			CharAttr.Attr2 &= ~ Attr2Fore;
			CharAttr.Fore = AttrDefaultFG;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  40:
		case  41:
		case  42:
		case  43:
		case  44:
		case  45:
		case  46:
		case  47:	/* Back color */
			CharAttr.Attr2 |= Attr2Back;
			CharAttr.Back = P - 40;
			BuffSetCurCharAttr(CharAttr);
			break;

		case  48:	/* Back color (256color mode) */
			if ((ts.ColorFlag & CF_XTERM256) && i < NParam && Param[i+1] == 5) {
				i++;
				if (i < NParam) {
					P = Param[++i];
					if (P<0) {
						P = 0;
					}
					CharAttr.Attr2 |= Attr2Back;
					CharAttr.Back = P;
					BuffSetCurCharAttr(CharAttr);
				}
			}
			break;

		case  49:	/* Reset back color */
			CharAttr.Attr2 &= ~ Attr2Back;
			CharAttr.Back = AttrDefaultBG;
			BuffSetCurCharAttr(CharAttr);
			break;

		case 90:
		case 91:
		case 92:
		case 93:
		case 94:
		case 95:
		case 96:
		case 97:	/* aixterm style text color */
			if (ts.ColorFlag & CF_AIXTERM16) {
				CharAttr.Attr2 |= Attr2Fore;
				CharAttr.Fore = P - 90 + 8;
				BuffSetCurCharAttr(CharAttr);
			}
			break;

		case 100:
			if (! (ts.ColorFlag & CF_AIXTERM16)) {
				/* Reset text and back color */
				CharAttr.Attr2 &= ~ (Attr2Fore | Attr2Back);
				CharAttr.Fore = AttrDefaultFG;
				CharAttr.Back = AttrDefaultBG;
				BuffSetCurCharAttr(CharAttr);
				break;
			}
			/* fall through to aixterm style back color */

		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:	/* aixterm style back color */
			if (ts.ColorFlag & CF_AIXTERM16) {
				CharAttr.Attr2 |= Attr2Back;
				CharAttr.Back = P - 100 + 8;
				BuffSetCurCharAttr(CharAttr);
			}
			break;
		}
	}
}

  void CSSetScrollRegion()
  {
    if ((StatusLine>0) &&
	(CursorY==NumOfLines-1))
    {
      MoveCursor(0,CursorY);
      return;
    }
    if (Param[1]<1) Param[1] =1;
    if ((NParam < 2) | (Param[2]<1))
      Param[2] = NumOfLines-StatusLine;
    Param[1]--;
    Param[2]--;
    if (Param[1] > NumOfLines-1-StatusLine)
      Param[1] = NumOfLines-1-StatusLine;
    if (Param[2] > NumOfLines-1-StatusLine)
      Param[2] = NumOfLines-1-StatusLine;
    if (Param[1] >= Param[2]) return;
    CursorTop = Param[1];
    CursorBottom = Param[2];
    if (RelativeOrgMode) MoveCursor(0,CursorTop);
		    else MoveCursor(0,0);
  }

  void CSSunSequence() /* Sun terminal private sequences */
  {
    int x, y, len;
    char Report[16];

    switch (Param[1]) {
      case 1: // De-iconify window
	DispShowWindow(WINDOW_RESTORE);
	break;
      case 2: // Iconify window
	DispShowWindow(WINDOW_MINIMIZE);
	break;
      case 3: // set window position
	if (NParam < 2) Param[2] = 0;
	if (NParam < 3) Param[3] = 0;
	DispMoveWindow(Param[2], Param[3]);
	break;
      case 4: // set window size
        if (NParam < 2) Param[2] = 0;
	if (NParam < 3) Param[3] = 0;
	DispResizeWin(Param[3], Param[2]);
	break;
      case 5: // Raise window
	DispShowWindow(WINDOW_RAISE);
	break;
      case 6: // Lower window
	DispShowWindow(WINDOW_LOWER);
	break;
      case 7: // Refresh window
	DispShowWindow(WINDOW_REFRESH);
	break;
      case 8: /* set terminal size */
	if ((Param[2]<=1) || (NParam<2)) Param[2] = 24;
	if ((Param[3]<=1) || (NParam<3)) Param[3] = 80;
	ChangeTerminalSize(Param[3],Param[2]);
	break;
      case 9: // Maximize/Restore window
	if (NParam < 2 || Param[2] == 0) {
	  DispShowWindow(WINDOW_RESTORE);
	}
	else {
	  DispShowWindow(WINDOW_MAXIMIZE);
	}
	break;
      case 11: // Report window state
	len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "%dt", CLocale, DispWindowIconified()?2:1);
	SendCSIstr(Report, len);
	break;
      case 13: // Report window position
	DispGetWindowPos(&x, &y);
	len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "3;%d;%dt", CLocale, x, y);
	SendCSIstr(Report, len);
	break;
      case 14: /* get window size??? */
	/* this is not actual window size */
	SendCSIstr("4;640;480t", 10);
	break;
      case 18: /* get terminal size */
	len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "8;%u;%u;t", CLocale, NumOfLines-StatusLine, NumOfColumns);
	SendCSIstr(Report, len);
	break;
      case 19: // Report display size (character)
	DispGetRootWinSize(&x, &y);
	len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "9;%d;%dt", CLocale, y, x);
	SendCSIstr(Report, len);
	break;
    }
  }

  void CSGT(BYTE b)
  {
    switch (b) {
      case 'c': /* second terminal report (Secondary DA) */
	if (Param[1] < 1) {
	  SendCSIstr(">32;10;2c", 9); /* VT382 */
	}
	break;
      case 'J':
	if (Param[1]==3) // IO-8256 terminal
	{
	  if (Param[2]<1) Param[2]=1;
	  if (Param[3]<1) Param[3]=1;
	  if (Param[4]<1) Param[4]=1;
	  if (Param[5]<1) Param[5]=1;
	  BuffEraseBox(Param[3]-1,Param[2]-1,
		       Param[5]-1,Param[4]-1);
	}
	break;
      case 'K':
	if ((NParam>=2) && (Param[1]==5))
	{	// IO-8256 terminal
	  switch (Param[2]) {
	    case 3:
	    case 4:
	    case 5:
	    case 6:
	      BuffDrawLine(CharAttr, Param[2], Param[3]);
	      break;
	    case 12:
	      /* Text color */
	      if ((Param[3]>=0) && (Param[3]<=7))
	      {
		switch (Param[3]) {
		  case 3: CharAttr.Fore = IdBlue; break;
		  case 4: CharAttr.Fore = IdCyan; break;
		  case 5: CharAttr.Fore = IdYellow; break;
		  case 6: CharAttr.Fore = IdMagenta; break;
		  default: CharAttr.Fore = Param[3]; break;
		}
		CharAttr.Attr2 |= Attr2Fore;
		BuffSetCurCharAttr(CharAttr);
	      }
	      break;
	  }
	}
	else if (Param[1]==3)
	{// IO-8256 terminal
	  if (Param[2]<1) Param[2] = 1;
	  if (Param[3]<1) Param[2] = 1;
	  BuffEraseCharsInLine(Param[2]-1,Param[3]-Param[2]+1);
	}
	break;
    }
  }

    void CSQExchangeColor()
    {
      COLORREF ColorRef;

      BuffUpdateScroll();

      if (ts.ColorFlag & CF_REVERSECOLOR) {
        ColorRef = ts.VTColor[0];
        ts.VTColor[0] = ts.VTReverseColor[0];
        ts.VTReverseColor[0] = ColorRef;
        ColorRef = ts.VTColor[1];
        ts.VTColor[1] = ts.VTReverseColor[1];
        ts.VTReverseColor[1] = ColorRef;
      }
      else {
        ColorRef = ts.VTColor[0];
        ts.VTColor[0] = ts.VTColor[1];
        ts.VTColor[1] = ColorRef;
      }

      ColorRef = ts.VTBoldColor[0];
      ts.VTBoldColor[0] = ts.VTBoldColor[1];
      ts.VTBoldColor[1] = ColorRef;

      ColorRef = ts.VTBlinkColor[0];
      ts.VTBlinkColor[0] = ts.VTBlinkColor[1];
      ts.VTBlinkColor[1] = ColorRef;

      ColorRef = ts.URLColor[0];
      ts.URLColor[0] = ts.URLColor[1];
      ts.URLColor[1] = ColorRef;

      ts.ColorFlag ^= CF_REVERSEVIDEO;

#ifdef ALPHABLEND_TYPE2
      BGInitialize();
#endif
      DispChangeBackground();
      UpdateWindow(HVTWin);
    }

    void CSQ_h_Mode()
    {
      int i;

      for (i = 1 ; i<=NParam ; i++)
	switch (Param[i]) {
	  case 1: AppliCursorMode = TRUE; break;
	  case 3:
	    ChangeTerminalSize(132,NumOfLines-StatusLine);
	    break;
	  case 5: /* Reverse Video */
	    if (!(ts.ColorFlag & CF_REVERSEVIDEO))
	      CSQExchangeColor(); /* Exchange text/back color */
	    break;
	  case 6:
	    if ((StatusLine>0) &&
		(CursorY==NumOfLines-1))
	      MoveCursor(0,CursorY);
	    else {
	      RelativeOrgMode = TRUE;
	      MoveCursor(0,CursorTop);
	    }
	    break;
	  case 7: AutoWrapMode = TRUE; break;
	  case 8: AutoRepeatMode = TRUE; break;
	  case 9:
	    if (ts.MouseEventTracking)
	      MouseReportMode = IdMouseTrackX10;
	    break;
	  case 12: ts.NonblinkingCursor = FALSE; ChangeCaret(); break;
	  case 19: PrintEX = TRUE; break;
	  case 25: DispEnableCaret(TRUE); break; // cursor on
	  case 38:
	    if (ts.AutoWinSwitch>0)
	      ChangeEmu = IdTEK; /* Enter TEK Mode */
	    break;
	  case 59:
	    if (ts.Language==IdJapanese)
	    { /* kanji terminal */
	      Gn[0] = IdASCII;
	      Gn[1] = IdKatakana;
	      Gn[2] = IdKatakana;
	      Gn[3] = IdKanji;
	      Glr[0] = 0;
	      if ((ts.KanjiCode==IdJIS) &&
		  (ts.JIS7Katakana==0))
		Glr[1] = 2;  // 8-bit katakana
	      else
		Glr[1] = 3;
	    }
	    break;
	  case 66: AppliKeyMode = TRUE; break;
	  case 67: ts.BSKey = IdBS; break;
	  case 1000:
	    if (ts.MouseEventTracking)
	      MouseReportMode = IdMouseTrackVT200;
	    break;
	  case 1001:
	    if (ts.MouseEventTracking)
	      MouseReportMode = IdMouseTrackVT200Hl;
	    break;
	  case 1002:
	    if (ts.MouseEventTracking)
	      MouseReportMode = IdMouseTrackBtnEvent;
	    break;
	  case 1003:
	    if (ts.MouseEventTracking)
	      MouseReportMode = IdMouseTrackAllEvent;
	    break;
	  case 1004:
	    if (ts.MouseEventTracking)
	      FocusReportMode = TRUE;
	    break;
      }
    }

    void CSQ_i_Mode()
    {
      if (Param[1]==-1) Param[1] = 0;
      switch (Param[1]) {
	case 1:
	  OpenPrnFile();
	  BuffDumpCurrentLine(LF);
	  if (! AutoPrintMode)
	    ClosePrnFile();
	  break;
	/* auto print mode off */
	case 4:
	  if (AutoPrintMode)
	  {
	    ClosePrnFile();
	    AutoPrintMode = FALSE;
	  }
	  break;
	/* auto print mode on */
	case 5:
	  if (! AutoPrintMode)
	  {
	    OpenPrnFile();
	    AutoPrintMode = TRUE;
	  }
	  break;
      }
    }

    void CSQ_l_Mode()
    {
      int i;

      for (i = 1 ; i <= NParam ; i++)
	switch (Param[i]) {
	  case 1: AppliCursorMode = FALSE; break;
	  case 3:
	    ChangeTerminalSize(80,NumOfLines-StatusLine);
	    break;
	  case 5: /* Normal Video */
	    if (ts.ColorFlag & CF_REVERSEVIDEO)
	      CSQExchangeColor(); /* Exchange text/back color */
	    break;
	  case 6:
	    if ((StatusLine>0) &&
		(CursorY==NumOfLines-1))
	      MoveCursor(0,CursorY);
	    else {
	      RelativeOrgMode = FALSE;
	      MoveCursor(0,0);
	    }
	    break;
	  case 7: AutoWrapMode = FALSE; break;
	  case 8: AutoRepeatMode = FALSE; break;
	  case 9: MouseReportMode = IdMouseTrackNone; break;
	  case 12: ts.NonblinkingCursor = TRUE; ChangeCaret(); break;
	  case 19: PrintEX = FALSE; break;
	  case 25: DispEnableCaret(FALSE); break; // cursor off
	  case 59:
	    if (ts.Language==IdJapanese)
	    { /* katakana terminal */
	      Gn[0] = IdASCII;
	      Gn[1] = IdKatakana;
	      Gn[2] = IdKatakana;
	      Gn[3] = IdKanji;
	      Glr[0] = 0;
	      if ((ts.KanjiCode==IdJIS) &&
		  (ts.JIS7Katakana==0))
		Glr[1] = 2;  // 8-bit katakana
	      else
		Glr[1] = 3;
	    }
	    break;
	  case 66: AppliKeyMode = FALSE; break;
	  case 67: ts.BSKey = IdDEL; break;
	  case 1000:
	  case 1001:
	  case 1002:
	  case 1003: MouseReportMode = IdMouseTrackNone; break;
	  case 1004: FocusReportMode = FALSE; break;
	}
    }

    void CSQ_n_Mode()
    {
    }

  void CSQuest(BYTE b)
  {
    switch (b) {
      case 'K': CSLineErase(); break;
      case 'h': CSQ_h_Mode(); break;
      case 'i': CSQ_i_Mode(); break;
      case 'l': CSQ_l_Mode(); break;
      case 'n': CSQ_n_Mode(); break;
    }
  }

  void SoftReset()
  // called by software-reset escape sequence handler
  {
    UpdateStr();
    AutoRepeatMode = TRUE;
    DispEnableCaret(TRUE); // cursor on
    InsertMode = FALSE;
    RelativeOrgMode = FALSE;
    AppliKeyMode = FALSE;
    AppliCursorMode = FALSE;
    if ((StatusLine>0) &&
	(CursorY == NumOfLines-1))
      MoveToMainScreen();
    CursorTop = 0;
    CursorBottom = NumOfLines-1-StatusLine;
    ResetCharSet();

    Send8BitMode = ts.Send8BitCtrl;

    /* Attribute */
    CharAttr = DefCharAttr;
    Special = FALSE;
    BuffSetCurCharAttr(CharAttr);

    // status buffers
    ResetSBuffers();
  }

  void CSExc(BYTE b)
  {
    switch (b) {
      case 'p':
	/* Software reset */
	SoftReset();
	break;
    }
  }

  void CSDouble(BYTE b)
  {
    switch (b) {
      case 'p':
	/* Select terminal mode (software reset) */
	SoftReset();
	if (NParam > 0) {
	  switch (Param[1]) {
	    case 61: // VT100 Mode
	      Send8BitMode = FALSE; break;
	    case 62: // VT200 Mode
	    case 63: // VT300 Mode
	    case 64: // VT400 Mode
	      if (NParam > 1 && Param[2] == 1)
		Send8BitMode = FALSE;
	      else
		Send8BitMode = TRUE;
	      break;
	  }
	}
	break;
    }
  }

  void CSDol(BYTE b)
  {
    switch (b) {
      case '}':
	if ((ts.TermFlag & TF_ENABLESLINE)==0) return;
	if (StatusLine==0) return;
	if ((Param[1]<1) && (CursorY==NumOfLines-1))
	  MoveToMainScreen();
	else if ((Param[1]==1) && (CursorY<NumOfLines-1))
	  MoveToStatusLine();
	break;
      case '~':
	if ((ts.TermFlag & TF_ENABLESLINE)==0) return;
	if (Param[1]<=1)
	  HideStatusLine();
	else if ((StatusLine==0) && (Param[1]==2))
	  ShowStatusLine(1); // show
	break;
    }
  }

  void CSSpace(BYTE b) {
    switch (b) {
      case 'q':
        if (NParam > 0) {
          if (Param[1] < 0) Param[1] = 0;
          switch (Param[1]) {
            case 0:
            case 1:
              ts.CursorShape = IdBlkCur;
              ts.NonblinkingCursor = FALSE;
              break;
            case 2:
              ts.CursorShape = IdBlkCur;
              ts.NonblinkingCursor = TRUE;
              break;
            case 3:
              ts.CursorShape = IdHCur;
              ts.NonblinkingCursor = FALSE;
              break;
            case 4:
              ts.CursorShape = IdHCur;
              ts.NonblinkingCursor = TRUE;
              break;
            case 5:
              ts.CursorShape = IdVCur;
              ts.NonblinkingCursor = FALSE;
              break;
            case 6:
              ts.CursorShape = IdVCur;
              ts.NonblinkingCursor = TRUE;
              break;
	    default:
	      return;
          }
	  ChangeCaret();
        }
        break;
    }
  }

void PrnParseCS(BYTE b) // printer mode
{
  ParseMode = ModeFirst;
  switch (ICount) {
    /* no intermediate char */
    case 0:
      switch (Prv) {
	/* no private parameter */
	case 0:
	  switch (b) {
	    case 'i':
	      if (Param[1]==4)
	      {
		PrinterMode = FALSE;
		// clear prn buff
		WriteToPrnFile(0,FALSE);
		if (! AutoPrintMode)
		  ClosePrnFile();
		return;
	      }
	      break;
	  } /* of case Prv=0 */
	  break;
      }
      break;
    /* one intermediate char */
    case 1: break;
  } /* of case Icount */

  WriteToPrnFile(b,TRUE);
}

void ParseCS(BYTE b) /* b is the final char */
{
  if (PrinterMode) { // printer mode
    PrnParseCS(b);
    return;
  }

  switch (ICount) {
    /* no intermediate char */
    case 0:
      switch (Prv) {
	/* no private parameter */
	case 0:
	  switch (b) {
	    // ISO/IEC 6429 / ECMA-48 Sequence
	    case '@': CSInsertCharacter(); break;       // ICH
	    case 'A': CSCursorUp(); break;              // CUU
	    case 'B': CSCursorDown(); break;            // CUD
	    case 'C': CSCursorRight(); break;           // CUF
	    case 'D': CSCursorLeft(); break;            // CUB
	    case 'E': CSCursorDown1(); break;           // CNL
	    case 'F': CSCursorUp1(); break;             // CPL
	    case 'G': CSMoveToColumnN(); break;         // CHA
	    case 'H': CSMoveToXY(); break;              // CUP
	    case 'I': CSForwardTab(); break;            // CHT
	    case 'J': CSScreenErase(); break;           // ED
	    case 'K': CSLineErase(); break;             // EL
	    case 'L': CSInsertLine(); break;            // IL
	    case 'M': CSDeleteNLines(); break;          // DL
//	    case 'N': break;				// EF   -- Not support
//	    case 'O': break;				// EA   -- Not support
	    case 'P': CSDeleteCharacter(); break;       // DCH
//	    case 'Q': break;				// SEE  -- Not support
//	    case 'R': break;				// CPR  -- Not support
	    case 'S': CSScrollUP(); break;              // SU
	    case 'T': CSScrollDown(); break;            // SD
//	    case 'U': break;				// NP   -- Not support
//	    case 'V': break;				// PP   -- Not support
//	    case 'W': break;				// CTC  -- Not support
	    case 'X': CSEraseCharacter(); break;        // ECH
//	    case 'Y': break;				// CVT  -- Not support
	    case 'Z': CSBackwardTab(); break;           // CBT
//	    caes '[': break;                            // SRS  -- Not support
//	    caes '\\': break;                           // PTX  -- Not support
//	    caes ']': break;                            // SDS  -- Not support
//	    caes '^': break;                            // SIMD -- Not support
	    case '`': CSMoveToColumnN(); break;         // HPA
	    case 'a': CSCursorRight(); break;           // HPR
//	    caes 'b': break;                            // REP  -- Not support
	    case 'c': AnswerTerminalType(); break;      // DA
	    case 'd': CSMoveToLineN(); break;           // VPA
	    case 'e': CSCursorUp(); break;              // VPR
	    case 'f': CSMoveToXY(); break;              // HVP
	    case 'g': CSDeleteTabStop(); break;         // TBC
	    case 'h': CS_h_Mode(); break;               // SM
	    case 'i': CS_i_Mode(); break;               // MC
//	    caes 'b': break;                            // HPB  -- Not support
//	    caes 'b': break;                            // VPB  -- Not support
	    case 'l': CS_l_Mode(); break;               // RM
	    case 'm': CSSetAttr(); break;               // SGR
	    case 'n': CS_n_Mode(); break;               // DSR
//	    caes 'o': break;                            // DAQ  -- Not support

	    // Private Sequence
	    case 'r': CSSetScrollRegion(); break;       // DECSTBM
	    case 's': SaveCursor(); break;              // SCP (Save cursor (ANSI.SYS/SCO?))
	    case 't': CSSunSequence(); break;           // DECSLPP / Window manipulation(dtterm?)
	    case 'u': RestoreCursor(); break;           // RCP (Restore cursor (ANSI.SYS/SCO))
	  } /* of case Prv=0 */
	  break;
	/* private parameter = '>' */
	case '>': CSGT(b); break;
	/* private parameter = '?' */
	case '?': CSQuest(b); break;
      } /* end of siwtch (Prv) */
      break;
    /* one intermediate char */
    case 1:
      switch (IntChar[1]) {
        /* intermediate char = ' ' */
	case ' ': CSSpace(b); break;
	/* intermediate char = '!' */
	case '!': CSExc(b); break;
	/* intermediate char = '"' */
	case '"': CSDouble(b); break;
	/* intermediate char = '$' */
	case '$': CSDol(b); break;
      }
      break;
  } /* of case Icount */

  ParseMode = ModeFirst;
}

void ControlSequence(BYTE b)
{
  if ((b<=US) || (b>=0x80) && (b<=0x9F))
    ParseControl(b); /* ctrl char */
  else if ((b>=0x40) && (b<=0x7E))
    ParseCS(b); /* terminate char */
  else {
    if (PrinterMode)
      WriteToPrnFile(b,FALSE);

    if ((b>=0x20) && (b<=0x2F))
    { /* intermediate char */
      if (ICount<IntCharMax) ICount++;
      IntChar[ICount] = b;
    }
    else if ((b>=0x30) && (b<=0x39))
    {
      if (Param[NParam] < 0)
	Param[NParam] = 0;
      if (Param[NParam]<1000)
       Param[NParam] = Param[NParam]*10 + b - 0x30;
    }
    else if (b==0x3B)
    {
      if (NParam < NParamMax)
      {
	NParam++;
	Param[NParam] = -1;
      }
    }
    else if ((b>=0x3C) && (b<=0x3F))
    { /* private char */
      if (FirstPrm) Prv = b;
    }
  }
  FirstPrm = FALSE;
}

void DeviceControl(BYTE b)
{
  if (ESCFlag && (b=='\\') || (b==ST && ts.KanjiCode!=IdSJIS))
  {
    ESCFlag = FALSE;
    ParseMode = SavedMode;
    return;
  }

  if (b==ESC)
  {
    ESCFlag = TRUE;
    return;
  }
  else ESCFlag = FALSE;

  if (b<=US)
    ParseControl(b);
  else if ((b>=0x30) && (b<=0x39))
  {
    if (Param[NParam] < 0) Param[NParam] = 0;
    if (Param[NParam]<1000)
      Param[NParam] = Param[NParam]*10 + b - 0x30;
  }
  else if (b==0x3B)
  {
    if (NParam < NParamMax)
    {
      NParam++;
      Param[NParam] = -1;
    }
  }
  else if ((b>=0x40) && (b<=0x7E))
  {
    if (b=='|')
    {
      ParseMode = ModeDCUserKey;
      if (Param[1] < 1) ClearUserKey();
      WaitKeyId = TRUE;
      NewKeyId = 0;
    }
    else ParseMode = ModeSOS;
  }
}

void DCUserKey(BYTE b)
{
  if (ESCFlag && (b=='\\') || (b==ST && ts.KanjiCode!=IdSJIS))
  {
    if (! WaitKeyId) DefineUserKey(NewKeyId,NewKeyStr,NewKeyLen);
    ESCFlag = FALSE;
    ParseMode = SavedMode;
    return;
  }

  if (b==ESC)
  {
    ESCFlag = TRUE;
    return;
  }
  else ESCFlag = FALSE;

  if (WaitKeyId)
  {
    if ((b>=0x30) && (b<=0x39))
    {
      if (NewKeyId<1000)
	NewKeyId = NewKeyId*10 + b - 0x30;
    }
    else if (b==0x2F)
    {
      WaitKeyId = FALSE;
      WaitHi = TRUE;
      NewKeyLen = 0;
    }
  }
  else {
    if (b==0x3B)
    {
      DefineUserKey(NewKeyId,NewKeyStr,NewKeyLen);
      WaitKeyId = TRUE;
      NewKeyId = 0;
    }
    else {
      if (NewKeyLen < FuncKeyStrMax)
      {
	if (WaitHi)
	{
	  NewKeyStr[NewKeyLen] = ConvHexChar(b) << 4;
	  WaitHi = FALSE;
	}
	else {
	  NewKeyStr[NewKeyLen] = NewKeyStr[NewKeyLen] +
				 ConvHexChar(b);
	  WaitHi = TRUE;
	  NewKeyLen++;
	}
      }
    }
  }
}

void IgnoreString(BYTE b)
{
  if ((ESCFlag && (b=='\\')) ||
      (b<=US && b!=ESC && b!=HT) ||
      (b==ST && ts.KanjiCode!=IdSJIS))
    ParseMode = SavedMode;

  if (b==ESC) ESCFlag = TRUE;
	 else ESCFlag = FALSE;
}

BOOL XsParseColor(char *colspec, COLORREF *color)
{
	unsigned int r, g, b;
//	double dr, dg, db;

	r = g = b = 255;

	if (colspec == NULL || color == NULL) {
		return FALSE;
	}

	if (_strnicmp(colspec, "rgb:", 4) == 0) {
		switch (strlen(colspec)) {
		  case  9:	// rgb:R/G/B
			if (sscanf(colspec, "rgb:%1x/%1x/%1x", &r, &g, &b) != 3) {
				return FALSE;
			}
			r *= 17; g *= 17; b *= 17;
			break;
		  case 12:	// rgb:RR/GG/BB
			if (sscanf(colspec, "rgb:%2x/%2x/%2x", &r, &g, &b) != 3) {
				return FALSE;
			}
			break;
		  case 15:	// rgb:RRR/GGG/BBB
			if (sscanf(colspec, "rgb:%3x/%3x/%3x", &r, &g, &b) != 3) {
				return FALSE;
			}
			r >>= 4; g >>= 4; b >>= 4;
			break;
		  case 18:	// rgb:RRRR/GGGG/BBBB
			if (sscanf(colspec, "rgb:%4x/%4x/%4x", &r, &g, &b) != 3) {
				return FALSE;
			}
			r >>= 8; g >>= 8; b >>= 8;
			break;
		  default:
			return FALSE;
		}
	}
//	else if (_strnicmp(colspec, "rgbi:", 5) == 0) {
//		; /* nothing to do */
//	}
	else if (colspec[0] == '#') {
		switch (strlen(colspec)) {
		  case  4:	// #RGB
			if (sscanf(colspec, "#%1x%1x%1x", &r, &g, &b) != 3) {
				return FALSE;
			}
			r <<= 4; g <<= 4; b <<= 4;
			break;
		  case  7:	// #RRGGBB
			if (sscanf(colspec, "#%2x%2x%2x", &r, &g, &b) != 3) {
				return FALSE;
			}
			break;
		  case 10:	// #RRRGGGBBB
			if (sscanf(colspec, "#%3x%3x%3x", &r, &g, &b) != 3) {
				return FALSE;
			}
			r >>= 4; g >>= 4; b >>= 4;
			break;
		  case 13:	// #RRRRGGGGBBBB
			if (sscanf(colspec, "#%4x%4x%4x", &r, &g, &b) != 3) {
				return FALSE;
			}
			r >>= 8; g >>= 8; b >>= 8;
			break;
		  default:
			return FALSE;
		}
	}
	else {
		return FALSE;
	}

	if (r > 255 || g > 255 || b > 255) {
		return FALSE;
	}

	*color = RGB(r, g, b);
	return TRUE;
}

#define ModeXsFirst     1
#define ModeXsString    2
#define ModeXsColorNum  3
#define ModeXsColorSpec 4
#define ModeXsEsc       5
void XSequence(BYTE b)
{
	static BYTE XsParseMode = ModeXsFirst, PrevMode;
	static char StrBuff[sizeof(ts.Title)];
	static unsigned int ColorNumber, StrLen;
	int len;
	COLORREF color;

	switch (XsParseMode) {
	  case ModeXsFirst:
		if (isdigit(b)) {
			if (Param[1] < 1000) {
				Param[1] = Param[1]*10 + b - '0';
			}
		}
		else if (b == ';') {
			StrBuff[0] = '\0';
			StrLen = 0;
			if (Param[1] == 4) {
				ColorNumber = 0;
				XsParseMode = ModeXsColorNum;
			}
			else {
				XsParseMode = ModeXsString;
			}
		}
		else {
			ParseMode = ModeFirst;
		}
		break;
	  case ModeXsString:
		if ((b==ST && ts.KanjiCode!=IdSJIS) || b==BEL) { /* String Terminator */
			StrBuff[StrLen] = '\0';
			switch (Param[1]) {
			  case 0: /* Change window title and icon name */
			  case 1: /* Change icon name */
			  case 2: /* Change window title */
				if (ts.AcceptTitleChangeRequest) {
					strncpy_s(cv.TitleRemote, sizeof(cv.TitleRemote), StrBuff, _TRUNCATE);
					// (2006.6.15 maya) タイトルに渡す文字列をSJISに変換
					ConvertToCP932(cv.TitleRemote, sizeof(cv.TitleRemote));
					ChangeTitle();
				}
				break;
			  default:
				/* nothing to do */;
			}
			ParseMode = ModeFirst;
			XsParseMode = ModeXsFirst;
		}
		else if (b == ESC) { /* Escape */
			PrevMode = ModeXsString;
			XsParseMode = ModeXsEsc;
		}
		else if (b <= US) { /* Other control character -- invalid sequence */
			ParseMode = ModeFirst;
			XsParseMode = ModeXsFirst;
		}
		else if (StrLen < sizeof(StrBuff) - 1) {
			StrBuff[StrLen++] = b;
		}
		break;
	  case ModeXsColorNum:
		if (isdigit(b)) {
			ColorNumber = ColorNumber*10 + b - '0';
		}
		else if (b == ';') {
			XsParseMode = ModeXsColorSpec;
			StrBuff[0] = '\0';
			StrLen = 0;
		}
		else {
			ParseMode = ModeFirst;
			XsParseMode = ModeXsFirst;
		}
		break;
	  case ModeXsColorSpec:
		if ((b==ST && ts.KanjiCode!=IdSJIS) || b==BEL) { /* String Terminator */
			StrBuff[StrLen] = '\0';
			if ((ts.ColorFlag & CF_XTERM256) && ColorNumber <= 255) {
				if (strcmp(StrBuff, "?") == 0) {
					color = DispGetANSIColor(ColorNumber);
					len =_snprintf_s_l(StrBuff, sizeof(StrBuff), _TRUNCATE,
						"4;%d;rgb:%02x/%02x/%02x\234", CLocale, ColorNumber,
						GetRValue(color), GetGValue(color), GetBValue(color));
					ParseMode = ModeFirst;
					XsParseMode = ModeXsFirst;
					SendOSCstr(StrBuff, len);
					break;
				}
				else if (XsParseColor(StrBuff, &color)) {
					DispSetANSIColor(ColorNumber, color);
				}
			}
			ParseMode = ModeFirst;
			XsParseMode = ModeXsFirst;
		}
		else if (b == ESC) {
			PrevMode = ModeXsColorSpec;
			XsParseMode = ModeXsEsc;
		}
		else if (b <= US) { /* Other control character -- invalid sequence */
			ParseMode = ModeFirst;
			XsParseMode = ModeXsFirst;
		}
		else if (b == ';') {
			if ((ts.ColorFlag & CF_XTERM256) && ColorNumber <= 255) {
				if (strcmp(StrBuff, "?") == 0) {
					color = DispGetANSIColor(ColorNumber);
					len =_snprintf_s_l(StrBuff, sizeof(StrBuff), _TRUNCATE,
						"4;%d;rgb:%02x/%02x/%02x\234", CLocale, ColorNumber,
						GetRValue(color), GetGValue(color), GetBValue(color));
					XsParseMode = ModeXsColorNum;
					SendOSCstr(StrBuff, len);
				}
				else if (XsParseColor(StrBuff, &color)) {
					DispSetANSIColor(ColorNumber, color);
				}
			}
			ColorNumber = 0;
			StrBuff[0] = '\0';
			StrLen = 0;
			XsParseMode = ModeXsColorNum;
		}
		else if (StrLen < sizeof(StrBuff) - 1) {
			StrBuff[StrLen++] = b;
		}
		break;
	  case ModeXsEsc:
		if (b == '\\') { /* String Terminator */
			XsParseMode = PrevMode;
//			XSequence(ST);
			XSequence(BEL);
		}
		else { /* Other character -- invalid sequence */
			ParseMode = ModeFirst;
			XsParseMode = ModeXsFirst;
		}
		break;
//	  default:
//		ParseMode = ModeFirst;
//		XsParseMode = ModeXsFirst;
	}
}

void DLESeen(BYTE b)
{
  ParseMode = ModeFirst;
  if (((ts.FTFlag & FT_BPAUTO)!=0) && (b=='B'))
    BPStart(IdBPAuto); /* Auto B-Plus activation */
  ChangeEmu = -1;
}

void CANSeen(BYTE b)
{
  ParseMode = ModeFirst;
  if (((ts.FTFlag & FT_ZAUTO)!=0) && (b=='B'))
    ZMODEMStart(IdZAuto); /* Auto ZMODEM activation */
  ChangeEmu = -1;
}

BOOL CheckKanji(BYTE b)
{
  BOOL Check;

  if (ts.Language!=IdJapanese) return FALSE;

  ConvJIS = FALSE;

  if (ts.KanjiCode==IdSJIS)
  {
    if ((0x80<b) && (b<0xa0) || (0xdf<b) && (b<0xfd))
      return TRUE; // SJIS kanji
    if ((0xa1<=b) && (b<=0xdf))
      return FALSE; // SJIS katakana
  }

  if ((b>=0x21) && (b<=0x7e))
  {
    Check = (Gn[Glr[0]]==IdKanji);
    ConvJIS = Check;
  }
  else if ((b>=0xA1) && (b<=0xFE))
  {
    Check = (Gn[Glr[1]]==IdKanji);
    if (ts.KanjiCode==IdEUC)
      Check = TRUE;
    else if (ts.KanjiCode==IdJIS)
    {
      if (((ts.TermFlag & TF_FIXEDJIS)!=0) &&
	  (ts.JIS7Katakana==0))
	Check = FALSE; // 8-bit katakana
    }
    ConvJIS = Check;
  }
  else
    Check = FALSE;

  return Check;
}

BOOL CheckKorean(BYTE b)
{
	BOOL Check;
	if (ts.Language!=IdKorean)
		return FALSE;

	if (ts.KanjiCode == IdSJIS) {
		if ((0xA1<=b) && (b<=0xFE)) {
			Check = TRUE;
		}
		else {
			Check = FALSE;
		}
	}

	return Check;
}

BOOL ParseFirstJP(BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	if (KanjiIn) {
		if ((! ConvJIS) && (0x3F<b) && (b<0xFD) ||
		      ConvJIS && ( (0x20<b) && (b<0x7f) ||
		                   (0xa0<b) && (b<0xff) ))
		{
			PutKanji(b);
			KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			KanjiIn = FALSE;
		}
		else if ((b==CR) && Wrap) {
			CarriageReturn(FALSE);
			LineFeed(LF,FALSE);
			Wrap = FALSE;
		}
	}

	if (SSflag) {
		if (Gn[GLtmp] == IdKanji) {
			Kanji = b << 8;
			KanjiIn = TRUE;
			SSflag = FALSE;
			return TRUE;
		}
		else if (Gn[GLtmp] == IdKatakana) {
			b = b | 0x80;
		}

		PutChar(b);
		SSflag = FALSE;
		return TRUE;
	}

	if ((!EUCsupIn) && (!EUCkanaIn) && (!KanjiIn) && CheckKanji(b)) {
		Kanji = b << 8;
		KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		ParseControl(b);
	}
	else if (b==0x20) {
		PutChar(b);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
		if (EUCsupIn) {
			EUCcount--;
			EUCsupIn = (EUCcount==0);
			return TRUE;
		}

		if ((Gn[Glr[0]] == IdKatakana) || EUCkanaIn) {
			b = b | 0x80;
			EUCkanaIn = FALSE;
		}
		PutChar(b);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((b>=0x80) && (b<=0x8D)) {
		ParseControl(b);
	}
	else if (b==0x8E) { // SS2
		if (ts.KanjiCode==IdEUC) {
			EUCkanaIn = TRUE;
		}
		else {
			ParseControl(b);
		}
	}
	else if (b==0x8F) { // SS3
		if (ts.KanjiCode==IdEUC) {
			EUCcount = 2;
			EUCsupIn = TRUE;
		}
		else {
			ParseControl(b);
		}
	}
	else if ((b>=0x90) && (b<=0x9F)) {
		ParseControl(b);
	}
	else if (b==0xA0) {
		PutChar(0x20);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (EUCsupIn) {
			EUCcount--;
			EUCsupIn = (EUCcount==0);
			return TRUE;
		}

		if ((Gn[Glr[1]] != IdASCII) ||
		    (ts.KanjiCode==IdEUC) && EUCkanaIn ||
		    (ts.KanjiCode==IdSJIS) ||
		    (ts.KanjiCode==IdJIS) &&
		    (ts.JIS7Katakana==0) &&
		    ((ts.TermFlag & TF_FIXEDJIS)!=0))
			PutChar(b);	// katakana
		else {
			if (Gn[Glr[1]] == IdASCII) {
				b = b & 0x7f;
			}
			PutChar(b);
		}
		EUCkanaIn = FALSE;
	}
	else {
		PutChar(b);
	}

	return TRUE;
}

BOOL ParseFirstKR(BYTE b)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	if (KanjiIn) {
		if ((0x41<=b) && (b<=0x5A) ||
		    (0x61<=b) && (b<=0x7A) ||
		    (0x81<=b) && (b<=0xFE))
		{
			PutKanji(b);
			KanjiIn = FALSE;
			return TRUE;
		}
		else if ((ts.TermFlag & TF_CTRLINKANJI)==0) {
			KanjiIn = FALSE;
		}
		else if ((b==CR) && Wrap) {
			CarriageReturn(FALSE);
			LineFeed(LF,FALSE);
			Wrap = FALSE;
		}
	}

	if ((!KanjiIn) && CheckKorean(b)) {
		Kanji = b << 8;
		KanjiIn = TRUE;
		return TRUE;
	}

	if (b<=US) {
		ParseControl(b);
	}
	else if (b==0x20) {
		PutChar(b);
	}
	else if ((b>=0x21) && (b<=0x7E)) {
//		if (Gn[Glr[0]] == IdKatakana) {
//			b = b | 0x80;
//		}
		PutChar(b);
	}
	else if (b==0x7f) {
		return TRUE;
	}
	else if ((0x80<=b) && (b<=0x9F)) {
		ParseControl(b);
	}
	else if (b==0xA0) {
		PutChar(0x20);
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		if (Gn[Glr[1]] == IdASCII) {
			b = b & 0x7f;
		}
		PutChar(b);
	}
	else {
		PutChar(b);
	}

	return TRUE;
}

static void ParseASCII(BYTE b)
{
	if (SSflag) {
		PutChar(b);
		SSflag = FALSE;
		return;
	}

	if (b<=US) {
		ParseControl(b);
	} else if ((b>=0x20) && (b<=0x7E)) {
		//Kanji = 0;
		//PutKanji(b);
		PutChar(b);
	} else if ((b>=0x80) && (b<=0x9F)) {
		ParseControl(b);
	} else if (b>=0xA0) {
		//Kanji = 0;
		//PutKanji(b);
		PutChar(b);
	}
}

//
// UTF-8
//
#include "uni2sjis.map"
#include "unisym2decsp.map"
extern unsigned short ConvertUnicode(unsigned short code, codemap_t *table, int tmax);


//
// UTF-8 for Mac OS X(HFS plus)
//
#include "hfs_plus.map"

unsigned short GetIllegalUnicode(int start_index, unsigned short first_code, unsigned short code,
								 hfsplus_codemap_t *table, int tmax)
{
	unsigned short result = 0;
	int i;

	for (i = start_index ; i < tmax ; i++) {
		if (table[i].first_code != first_code) { // 1文字目が異なるなら、以降はもう調べなくてよい。
			break;
		}

		if (table[i].second_code == code) {
			result = table[i].illegal_code;
			break;
		}
	}

	return (result);
}

int GetIndexOfHFSPlusFirstCode(unsigned short code, hfsplus_codemap_t *table, int tmax)
{
	int low, mid, high;
	int index = -1;

	low = 0;
	high = tmax - 1;

	// binary search
	while (low < high) {
		mid = (low + high) / 2;
		if (table[mid].first_code < code) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	if (table[low].first_code == code) {
		while (low >= 0 && table[low].first_code == code) {
			index = low;
			low--;
		}
	}

	return (index);
}


static void UnicodeToCP932(unsigned int code)
{
	int ret;
	char mbchar[32];
	unsigned char wchar[32];
	unsigned short cset = 0;

#if 0
	Kanji = code & 0xff00;
	PutKanji(code & 0x00ff);
	return;
#else

	wchar[0] = code & 0xff;
	wchar[1] = (code >> 8) & 0xff;

	if (ts.UnicodeDecSpMapping) {
		cset = ConvertUnicode(code, mapUnicodeSymbolToDecSp, MAPSIZE(mapUnicodeSymbolToDecSp));
	}
	if (((cset >> 8) & ts.UnicodeDecSpMapping) != 0) {
		PutDecSp(cset & 0xff);
	}
	else {
		// Unicode -> CP932
		ret = wctomb(mbchar, ((wchar_t *)wchar)[0]);
		switch (ret) {
		  case -1:
			if (_stricmp(ts.Locale, DEFAULT_LOCALE) == 0) {
				// U+301Cなどは変換できない。Unicode -> Shift_JISへ変換してみる。
				cset = ConvertUnicode(code, mapUnicodeToSJIS, MAPSIZE(mapUnicodeToSJIS));
				if (cset != 0) {
					Kanji = cset & 0xff00;
					PutKanji(cset & 0x00ff);
				}
			}

			if (cset == 0) {
				PutChar('?');
				if (ts.UnknownUnicodeCharaAsWide) {
					PutChar('?');
				}
			}
			break;
		  case 1:
			PutChar(mbchar[0]);
			break;
		  default:
			Kanji = mbchar[0] << 8;
			PutKanji(mbchar[1]);
			break;
		}
	}
#endif
}

// UTF-8で受信データを処理する
BOOL ParseFirstUTF8(BYTE b, int hfsplus_mode)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	static BYTE buf[3];
	static int count = 0;
	static int maybe_hfsplus = 0;
	static unsigned int first_code;
	static int first_code_index;

	unsigned int code;
	char mbchar[32];
	unsigned short cset;
	char *locptr;

	locptr = setlocale(LC_ALL, ts.Locale);

	if ((b & 0x80) != 0x80 || ((b & 0xe0) == 0x80 && count == 0)) {
		// 1バイト目および2バイト目がASCIIの場合は、すべてASCII出力とする。
		// 1バイト目がC1制御文字(0x80-0x9f)の場合も同様。
		if (count == 0 || count == 1) {
			if (hfsplus_mode == 1 && maybe_hfsplus == 1) {
				UnicodeToCP932(first_code);
				maybe_hfsplus = 0;
			}

			if (count == 1) {
				ParseASCII(buf[0]);
			}
			ParseASCII(b);

			count = 0;  // reset counter
			return TRUE;
		}
	}

	buf[count++] = b;
	if (count < 2) {
		return TRUE;
	}

	memset(mbchar, 0, sizeof(mbchar));

	// 2バイトコードの場合
	if ((buf[0] & 0xe0) == 0xc0) {
		if ((buf[1] & 0xc0) == 0x80) {

			if (hfsplus_mode == 1 && maybe_hfsplus == 1) {
				UnicodeToCP932(first_code);
				maybe_hfsplus = 0;
			}

			code = ((buf[0] & 0x1f) << 6);
			code |= ((buf[1] & 0x3f));

			UnicodeToCP932(code);
		}
		else {
			ParseASCII(buf[0]);
			ParseASCII(buf[1]);
		}
		count = 0;
		return TRUE;
	}

	if (count < 3) {
		return TRUE;
	}

	if ((buf[0] & 0xe0) == 0xe0 &&
		(buf[1] & 0xc0) == 0x80 &&
		(buf[2] & 0xc0) == 0x80) { // 3バイトコードの場合

		// UTF-8 BOM(Byte Order Mark)
		if (buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf) {
			goto skip;
		}

		code = ((buf[0] & 0xf) << 12);
		code |= ((buf[1] & 0x3f) << 6);
		code |= ((buf[2] & 0x3f));

		if (hfsplus_mode == 1) {
			if (maybe_hfsplus == 0) {
				if ((first_code_index = GetIndexOfHFSPlusFirstCode(
						code, mapHFSPlusUnicode, MAPSIZE(mapHFSPlusUnicode)
						)) != -1) {
					maybe_hfsplus = 1;
					first_code = code;
					count = 0;
					return (TRUE);
				}
			} else {
				maybe_hfsplus = 0;
				cset = GetIllegalUnicode(first_code_index, first_code, code, mapHFSPlusUnicode, MAPSIZE(mapHFSPlusUnicode));
				if (cset != 0) { // success
					code = cset;

				} else { // error
					// 2つめの文字が半濁点の1文字目に相当する場合は、再度検索を続ける。(2005.10.15 yutaka)
					if ((first_code_index = GetIndexOfHFSPlusFirstCode(
							code, mapHFSPlusUnicode, MAPSIZE(mapHFSPlusUnicode)
							)) != -1) {

						// 1つめの文字はそのまま出力する
						UnicodeToCP932(first_code);

						maybe_hfsplus = 1;
						first_code = code;
						count = 0;
						return (TRUE);
					}

					UnicodeToCP932(first_code);
					UnicodeToCP932(code);
					count = 0;
					return (TRUE);
				}
			}
		}

		UnicodeToCP932(code);

skip:
		count = 0;

	} else {
		ParseASCII(buf[0]);
		ParseASCII(buf[1]);
		ParseASCII(buf[2]);
		count = 0;

	}

	return TRUE;
}


BOOL ParseFirstRus(BYTE b)
// returns if b is processed
{
  if (b>=128)
  {
    b = RussConv(ts.RussHost,ts.RussClient,b);
    PutChar(b);
    return TRUE;
  }
  return FALSE;
}

void ParseFirst(BYTE b)
{
	switch (ts.Language) {
	  case IdUtf8:
	  	ParseFirstUTF8(b, ts.KanjiCode == IdUTF8m);
		return;

	  case IdJapanese:
	  	switch (ts.KanjiCode) {
		  case IdUTF8:
		  	if (ParseFirstUTF8(b, 0)) {
				return;
			}
			break;
		  case IdUTF8m:
		  	if (ParseFirstUTF8(b, 1)) {
				return;
			}
			break;
		  default:
			if (ParseFirstJP(b))  {
				return;
			}
		}
		break;

	  case IdKorean:
	  	switch (ts.KanjiCode) {
		  case IdUTF8:
		  	if (ParseFirstUTF8(b, 0)) {
				return;
			}
			break;
		  case IdUTF8m:
		  	if (ParseFirstUTF8(b, 1)) {
				return;
			}
			break;
		  default:
			if (ParseFirstKR(b))  {
				return;
			}
		}
		break;


	  case IdRussian:
		if (ParseFirstRus(b)) {
			return;
		}
		break;
	}

	if (SSflag) {
		PutChar(b);
		SSflag = FALSE;
		return;
	}

	if (b<=US)
		ParseControl(b);
	else if ((b>=0x20) && (b<=0x7E))
		PutChar(b);
	else if ((b>=0x80) && (b<=0x9F))
		ParseControl(b);
	else if (b>=0xA0)
		PutChar(b);
}

int VTParse()
{
  BYTE b;
  int c;

  c = CommRead1Byte(&cv,&b);

  if (c==0) return 0;

  CaretOff();
  UpdateCaretPosition(FALSE);  // 非アクティブの場合のみ再描画する

  ChangeEmu = 0;

  /* Get Device Context */
  DispInitDC();

  LockBuffer();

  while ((c>0) && (ChangeEmu==0))
  {
    if (DebugFlag)
      PutDebugChar(b);
    else {
      switch (ParseMode) {
	case ModeFirst: ParseFirst(b); break;
	case ModeESC: EscapeSequence(b); break;
	case ModeDCS: DeviceControl(b); break;
	case ModeDCUserKey: DCUserKey(b); break;
	case ModeSOS: IgnoreString(b); break;
	case ModeCSI: ControlSequence(b); break;
	case ModeXS:  XSequence(b); break;
	case ModeDLE: DLESeen(b); break;
	case ModeCAN: CANSeen(b); break;
	default:
	  ParseMode = ModeFirst;
	  ParseFirst(b);
      }
    }

    if (ChangeEmu==0)
      c = CommRead1Byte(&cv,&b);
  }

  BuffUpdateScroll();

  BuffSetCaretWidth();
  UnlockBuffer();

  /* release device context */
  DispReleaseDC();

  CaretOn();

  if (ChangeEmu > 0) ParseMode = ModeFirst;
  return ChangeEmu;
}

int MakeMouseReportStr(char *buff, size_t buffsize, int mb, int x, int y) {
  return _snprintf_s_l(buff, buffsize, _TRUNCATE, "M%c%c%c", CLocale, mb+32, x+32, y+32);
}

BOOL MouseReport(int Event, int Button, int Xpos, int Ypos) {
  char Report[10];
  int x, y, len, modifier;

  len = 0;

  if (MouseReportMode == IdMouseTrackNone)
    return FALSE;

  if (ts.DisableMouseTrackingByCtrl && ControlKey())
    return FALSE;

  DispConvWinToScreen(Xpos, Ypos, &x, &y, NULL);
  x++; y++;

  if (x < 1) x = 1;
  if (y < 1) y = 1;

  if (MouseReportMode != IdMouseTrackDECELR) {
    if (x > 0xff - 32) x = 0xff - 32;
    if (x > 0xff - 32) y = 0xff - 32;
  }

  if (ShiftKey())
    modifier = 4;
  else
    modifier = 0;

  if (ControlKey())
    modifier |= 8;

  if (AltKey())
    modifier |= 16;

  modifier = (ShiftKey()?4:0) | (ControlKey()?8:0) | (AltKey()?16:0);

  switch (Event) {
    case IdMouseEventBtnDown:
      switch (MouseReportMode) {
	case IdMouseTrackX10:
	  len = MakeMouseReportStr(Report, sizeof Report, Button, x, y);
	  break;

	case IdMouseTrackVT200:
	case IdMouseTrackBtnEvent:
	case IdMouseTrackAllEvent:
	  len = MakeMouseReportStr(Report, sizeof Report, Button | modifier, x, y);
	  break;

	case IdMouseTrackDECELR: /* not supported yet */
	case IdMouseTrackVT200Hl: /* not supported yet */
	default:
	  return FALSE;
      }
      break;

    case IdMouseEventBtnUp:
      switch (MouseReportMode) {
	case IdMouseTrackVT200:
	  len = MakeMouseReportStr(Report, sizeof Report, 3 | modifier, x, y);
	  break;

	case IdMouseTrackBtnEvent:
	case IdMouseTrackAllEvent:
	  MouseReport(IdMouseEventMove, Button, Xpos, Ypos);
	  len = MakeMouseReportStr(Report, sizeof Report, 3 | modifier, x, y);
	  break;

	case IdMouseTrackX10: /* nothing to do */
	case IdMouseTrackDECELR: /* not supported yet */
	case IdMouseTrackVT200Hl: /* not supported yet */
	default:
	  return FALSE;
      }
      break;

    case IdMouseEventMove:
      switch (MouseReportMode) {
	case IdMouseTrackBtnEvent: /* not supported yet */
	case IdMouseTrackAllEvent: /* not supported yet */
	  len = MakeMouseReportStr(Report, sizeof Report, Button | modifier | 32, x, y);
	  break;

	case IdMouseTrackDECELR: /* not supported yet */
	case IdMouseTrackVT200Hl: /* not supported yet */
	case IdMouseTrackX10: /* nothing to do */
	case IdMouseTrackVT200: /* nothing to do */
	default:
	  return FALSE;
      }
      break;

    case IdMouseEventWheel:
      switch (MouseReportMode) {
	case IdMouseTrackVT200:
	case IdMouseTrackBtnEvent:
	case IdMouseTrackAllEvent:
	  len = MakeMouseReportStr(Report, sizeof Report, Button | modifier | 64, x, y);
	  break;

	case IdMouseTrackX10: /* nothing to do */
	case IdMouseTrackDECELR: /* not supported yet */
	case IdMouseTrackVT200Hl: /* not supported yet */
	  return FALSE;
      }
      break;
  }

  if (len == 0)
    return FALSE;

  SendCSIstr(Report, len);
  return TRUE;
}

void FocusReport(BOOL focus) {
  if (!FocusReportMode)
    return;

  if (focus) {
    // Focus In
    SendCSIstr("I", 1);
  } else {
    // Focus Out
    SendCSIstr("O", 1);
  }
}

void VisualBell() {
	CSQExchangeColor();
	Sleep(10);
	CSQExchangeColor();
}

void EndTerm() {
	if (CLocale) {
		_free_locale(CLocale);
	}
	CLocale = NULL;
}
