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

/* TERATERM.EXE, VT terminal emulation */
#include "teraterm.h"
#include "tttypes.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
#include <locale.h>
#include <ctype.h>

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
#include "ttime.h"
#include "clipboar.h"

#include "vtterm.h"

void ParseFirst(BYTE b);

#define MAPSIZE(x) (sizeof(x)/sizeof((x)[0]))
#define Accept8BitCtrl ((VTlevel >= 2) && (ts.TermFlag & TF_ACCEPT8BITCTRL))

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
#define ModeIgnore 9

#define NParamMax  16
#define NSParamMax 16
#define IntCharMax  5

/* DEC Locator Flag */
#define DecLocatorOneShot    1
#define DecLocatorPixel      2
#define DecLocatorButtonDown 4
#define DecLocatorButtonUp   8
#define DecLocatorFiltered   16

void RingBell(int type);
void VisualBell();
BOOL DecLocatorReport(int Event, int Button);

/* character attribute */
static TCharAttr CharAttr;

/* various modes of VT emulation */
static BOOL RelativeOrgMode;
static BOOL InsertMode;
static BOOL LFMode;
static BOOL ClearThenHome;
static BOOL AutoWrapMode;
static BOOL FocusReportMode;
static BOOL AltScr;
static BOOL LRMarginMode;
static BOOL RectangleMode;
static BOOL BracketedPaste;

char BracketStart[] = "\033[200~";
char BracketEnd[] = "\033[201~";
int BracketStartLen = (sizeof(BracketStart)-1);
int BracketEndLen = (sizeof(BracketEnd)-1);

static int VTlevel;

BOOL AcceptWheelToCursor;

// save/restore cursor
typedef struct {
	int CursorX, CursorY;
	TCharAttr Attr;
	int Glr[2], Gn[4]; // G0-G3, GL & GR
	BOOL AutoWrapMode;
	BOOL RelativeOrgMode;
} TStatusBuff;
typedef TStatusBuff *PStatusBuff;

// currently only used for AUTO CR/LF receive mode
BYTE PrevCharacter;
BOOL PrevCRorLFGeneratedCRLF;	  // indicates that previous CR or LF really generated a CR+LF

// status buffer for main screen & status line
static TStatusBuff SBuff1, SBuff2, SBuff3;

static BOOL ESCFlag, JustAfterESC;
static BOOL KanjiIn;
static BOOL EUCkanaIn, EUCsupIn;
static int  EUCcount;
static BOOL Special;

static int Param[NParamMax+1];
static int SubParam[NParamMax+1][NSParamMax+1];
static int NParam, NSParam[NParamMax+1];
static BOOL FirstPrm;
static BYTE IntChar[IntCharMax+1];
static int ICount;
static BYTE Prv;
static int ParseMode;
static int ChangeEmu;

typedef struct tstack {
	char *title;
	struct tstack *next;
} TStack;
typedef TStack *PTStack;
PTStack TitleStack = NULL;

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
static BOOL Fallbacked;

// variables for status line mode
static int StatusX=0;
static BOOL StatusWrap=FALSE;
static BOOL StatusCursor=TRUE;
static int MainX, MainY; //cursor registers
static int MainTop, MainBottom; // scroll region registers
static BOOL MainWrap;
static BOOL MainCursor=TRUE;

/* status for printer escape sequences */
static BOOL PrintEX = TRUE; // printing extent
			    // (TRUE: screen, FALSE: scroll region)
static BOOL AutoPrintMode = FALSE;
static BOOL PrinterMode = FALSE;
static BOOL DirectPrn = FALSE;

/* User key */
static BYTE NewKeyStr[FuncKeyStrMax];
static int NewKeyId, NewKeyLen;

/* Mouse Report */
int MouseReportMode;
int MouseReportExtMode;
unsigned int DecLocatorFlag;
int LastX, LastY;
int ButtonStat;
int FilterTop, FilterBottom, FilterLeft, FilterRight;

/* IME Status */
BOOL IMEstat;

/* Beep over-used */
static DWORD BeepStartTime = 0;
static DWORD BeepSuppressTime = 0;
static DWORD BeepOverUsedCount = 0;

static _locale_t CLocale = NULL;

void ClearParams()
{
	ICount = 0;
	NParam = 1;
	NSParam[1] = 0;
	Param[1] = 0;
	Prv = 0;
}

void ResetSBuffer(PStatusBuff sbuff)
{
	sbuff->CursorX = 0;
	sbuff->CursorY = 0;
	sbuff->Attr = DefCharAttr;
	if (ts.Language==IdJapanese) {
		sbuff->Gn[0] = IdASCII;
		sbuff->Gn[1] = IdKatakana;
		sbuff->Gn[2] = IdKatakana;
		sbuff->Gn[3] = IdKanji;
		sbuff->Glr[0] = 0;
		if ((ts.KanjiCode==IdJIS) && (ts.JIS7Katakana==0))
			sbuff->Glr[1] = 2;	// 8-bit katakana
		else
			sbuff->Glr[1] = 3;
	}
	else {
		sbuff->Gn[0] = IdASCII;
		sbuff->Gn[1] = IdSpecial;
		sbuff->Gn[2] = IdASCII;
		sbuff->Gn[3] = IdASCII;
		sbuff->Glr[0] = 0;
		sbuff->Glr[1] = 0;
	}
	sbuff->AutoWrapMode = TRUE;
	sbuff->RelativeOrgMode = FALSE;
}

void ResetAllSBuffers()
{
	ResetSBuffer(&SBuff1);
	// copy SBuff1 to SBuff2
	SBuff2 = SBuff1;
	SBuff3 = SBuff1;
}

void ResetCurSBuffer()
{
	PStatusBuff Buff;

	if (AltScr) {
		Buff = &SBuff3; // Alternate screen buffer
	}
	else {
		Buff = &SBuff1; // Normal screen buffer
	}
	ResetSBuffer(Buff);
	SBuff2 = *Buff;
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
	AppliEscapeMode = FALSE;
	AcceptWheelToCursor = ts.TranslateWheelToCursor;
	RelativeOrgMode = FALSE;
	ts.ColorFlag &= ~CF_REVERSEVIDEO;
	AutoRepeatMode = TRUE;
	FocusReportMode = FALSE;
	MouseReportMode = IdMouseTrackNone;
	MouseReportExtMode = IdMouseTrackExtNone;
	DecLocatorFlag = 0;
	ClearThenHome = FALSE;
	RectangleMode = FALSE;

	ChangeTerminalID();

	LastX = 0;
	LastY = 0;
	ButtonStat = 0;

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
	ResetAllSBuffers();

	// Alternate Screen Buffer
	AltScr = FALSE;

	// Left/Right Margin Mode
	LRMarginMode = FALSE;

	// Bracketed Paste Mode
	BracketedPaste = FALSE;

	// Saved IME Status
	IMEstat = FALSE;

	// previous received character
	PrevCharacter = -1;	// none
	PrevCRorLFGeneratedCRLF = FALSE;

	// Beep over-used
	BeepStartTime = GetTickCount();
	BeepSuppressTime = BeepStartTime - ts.BeepSuppressTime * 1000;
	BeepStartTime -= (ts.BeepOverUsedTime * 1000);
	BeepOverUsedCount = ts.BeepOverUsedCount;
}

void ResetCharSet()
{
	if (ts.Language==IdJapanese) {
		Gn[0] = IdASCII;
		Gn[1] = IdKatakana;
		Gn[2] = IdKatakana;
		Gn[3] = IdKanji;
		Glr[0] = 0;
		if ((ts.KanjiCode==IdJIS) && (ts.JIS7Katakana==0))
			Glr[1] = 2;	// 8-bit katakana
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
	ConvJIS = FALSE;
	Fallbacked = FALSE;

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
	if (!DisabledModeOnly || ts.DisableAppKeypad)
		AppliKeyMode = FALSE;
	if (!DisabledModeOnly || ts.DisableAppCursor)
		AppliCursorMode = FALSE;
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
	MoveCursor(MainX, MainY); // move to main screen
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
	MoveCursor(StatusX, NumOfLines-1); // move to status line
	CursorTop = NumOfLines-1;
	CursorBottom = CursorTop;
	Wrap = StatusWrap;
}

void HideStatusLine()
{
	if (isCursorOnStatusLine)
		MoveToMainScreen();
	StatusX = 0;
	StatusWrap = FALSE;
	StatusCursor = TRUE;
	ShowStatusLine(0); //hide
}

void ChangeTerminalSize(int Nx, int Ny)
{
	BuffChangeTerminalSize(Nx, Ny);
	StatusX = 0;
	MainX = 0;
	MainY = 0;
	MainTop = 0;
	MainBottom = NumOfLines-StatusLine-1;
	LRMarginMode = FALSE;
}

void SendCSIstr(char *str, int len) {
	int l;

	if (str == NULL || len < 0)
		return;

	if (len == 0) {
		l = strlen(str);
	}
	else {
		l = len;
	}

	if (Send8BitMode)
		CommBinaryOut(&cv,"\233", 1);
	else
		CommBinaryOut(&cv,"\033[", 2);

	CommBinaryOut(&cv, str, l);
}

void SendOSCstr(char *str, int len, char TermChar) {
	int l;

	if (str == NULL || len < 0)
		return;

	if (len == 0) {
		l = strlen(str);
	}
	else {
		l = len;
	}

	if (TermChar == BEL) {
		CommBinaryOut(&cv,"\033]", 2);
		CommBinaryOut(&cv, str, l);
		CommBinaryOut(&cv,"\007", 1);
	}
	else if (Send8BitMode) {
		CommBinaryOut(&cv,"\235", 1);
		CommBinaryOut(&cv, str, l);
		CommBinaryOut(&cv,"\234", 1);
	}
	else {
		CommBinaryOut(&cv,"\033]", 2);
		CommBinaryOut(&cv, str, l);
		CommBinaryOut(&cv,"\033\\", 2);
	}

}

void SendDCSstr(char *str, int len) {
	int l;

	if (str == NULL || len < 0)
		return;

	if (len == 0) {
		l = strlen(str);
	}
	else {
		l = len;
	}

	if (Send8BitMode) {
		CommBinaryOut(&cv,"\220", 1);
		CommBinaryOut(&cv, str, l);
		CommBinaryOut(&cv,"\234", 1);
	}
	else {
		CommBinaryOut(&cv,"\033P", 2);
		CommBinaryOut(&cv, str, l);
		CommBinaryOut(&cv,"\033\\", 2);
	}

}

void BackSpace()
{
	if (CursorX == CursorLeftM || CursorX == 0) {
		if (CursorY > 0 && (ts.TermFlag & TF_BACKWRAP)) {
			MoveCursor(CursorRightM, CursorY-1);
			if (cv.HLogBuf!=0 && !ts.LogTypePlainText) Log1Byte(BS);
		}
	}
	else if (CursorX > 0) {
		MoveCursor(CursorX-1, CursorY);
		if (cv.HLogBuf!=0 && !ts.LogTypePlainText) Log1Byte(BS);
	}
}

void CarriageReturn(BOOL logFlag)
{
	if (!ts.EnableContinuedLineCopy || logFlag)
		if (cv.HLogBuf!=0) Log1Byte(CR);

	if (RelativeOrgMode || CursorX > CursorLeftM)
		MoveCursor(CursorLeftM, CursorY);
	else if (CursorX < CursorLeftM)
		MoveCursor(0, CursorY);

	Fallbacked = FALSE;
}

void LineFeed(BYTE b, BOOL logFlag)
{
	/* for auto print mode */
	if ((AutoPrintMode) &&
		(b>=LF) && (b<=FF))
		BuffDumpCurrentLine(b);

	if (!ts.EnableContinuedLineCopy || logFlag)
		if (cv.HLogBuf!=0) Log1Byte(LF);

	if (CursorY < CursorBottom)
		MoveCursor(CursorX,CursorY+1);
	else if (CursorY == CursorBottom) BuffScrollNLines(1);
	else if (CursorY < NumOfLines-StatusLine-1)
		MoveCursor(CursorX,CursorY+1);

	ClearLineContinued();

	if (LFMode) CarriageReturn(logFlag);

	Fallbacked = FALSE;
}

void Tab()
{
	if (Wrap && !ts.VTCompatTab) {
		CarriageReturn(FALSE);
		LineFeed(LF,FALSE);
		if (ts.EnableContinuedLineCopy) {
			SetLineContinued();
		}
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

	if (Wrap) {
		CarriageReturn(FALSE);
		LineFeed(LF,FALSE);
		CharAttrTmp.Attr |= ts.EnableContinuedLineCopy ? AttrLineContinued : 0;
	}

//	if (cv.HLogBuf!=0) Log1Byte(b);
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
	if ((b>0x5F) && (b<0x80)) {
		if (SSflag)
			SpecialNew = (Gn[GLtmp]==IdSpecial);
		else
			SpecialNew = (Gn[Glr[0]]==IdSpecial);
	}
	else if (b>0xDF) {
		if (SSflag)
			SpecialNew = (Gn[GLtmp]==IdSpecial);
		else
			SpecialNew = (Gn[Glr[1]]==IdSpecial);
	}

	if (SpecialNew != Special) {
		UpdateStr();
		Special = SpecialNew;
	}

	if (Special) {
		b = b & 0x7F;
		CharAttrTmp.Attr |= AttrSpecial;
	}
	else
		CharAttrTmp.Attr |= CharAttr.Attr;

	BuffPutChar(b, CharAttrTmp, InsertMode);

	if (CursorX == CursorRightM || CursorX >= NumOfColumns-1) {
		UpdateStr();
		Wrap = AutoWrapMode;
	}
	else {
		MoveRight();
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
		CharAttrTmp.Attr |= ts.EnableContinuedLineCopy ? AttrLineContinued : 0;
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

	if (CursorX == CursorRightM || CursorX >= NumOfColumns-1) {
		UpdateStr();
		Wrap = AutoWrapMode;
	}
	else {
		MoveRight();
	}
}

void PutKanji(BYTE b)
{
	int LineEnd;
	TCharAttr CharAttrTmp;
	CharAttrTmp = CharAttr;

	Kanji = Kanji + b;

	if (PrinterMode && DirectPrn) {
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

	if (CursorX > CursorRightM)
		LineEnd = NumOfColumns - 1;
	else
		LineEnd = CursorRightM;

	if (Wrap) {
		CarriageReturn(FALSE);
		LineFeed(LF,FALSE);
		if (ts.EnableContinuedLineCopy)
			CharAttrTmp.Attr |= AttrLineContinued;
	}
	else if (CursorX > LineEnd - 1) {
		if (AutoWrapMode) {
			if (ts.EnableContinuedLineCopy) {
				CharAttrTmp.Attr |= AttrLineContinued;
				if (CursorX == LineEnd)
					BuffPutChar(0x20, CharAttr, FALSE);
			}
			CarriageReturn(FALSE);
			LineFeed(LF,FALSE);
		}
		else {
			return;
		}
	}

	Wrap = FALSE;

	if (cv.HLogBuf!=0) {
		Log1Byte(HIBYTE(Kanji));
		Log1Byte(LOBYTE(Kanji));
	}

	if (Special) {
		UpdateStr();
		Special = FALSE;
	}

	BuffPutKanji(Kanji, CharAttrTmp, InsertMode);

	if (CursorX < LineEnd - 1) {
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
	static BYTE buff[3];
	int i;
	BOOL svInsertMode, svAutoWrapMode;
	BYTE svCharAttr;

	if (DebugFlag!=DEBUG_FLAG_NOUT) {
		svInsertMode = InsertMode;
		svAutoWrapMode = AutoWrapMode;
		InsertMode = FALSE;
		AutoWrapMode = TRUE;

		svCharAttr = CharAttr.Attr;
		if (CharAttr.Attr != AttrDefault) {
			UpdateStr();
			CharAttr.Attr = AttrDefault;
		}

		if (DebugFlag==DEBUG_FLAG_HEXD) {
			_snprintf(buff, 3, "%02X", (unsigned int) b);

			for (i=0; i<2; i++)
				PutChar(buff[i]);
			PutChar(' ');
		}
		else if (DebugFlag==DEBUG_FLAG_NORM) {

			if ((b & 0x80) == 0x80) {
				UpdateStr();
				CharAttr.Attr = AttrReverse;
				b = b & 0x7f;
			}

			if (b<=US) {
				PutChar('^');
				PutChar((char)(b+0x40));
			}
			else if (b==DEL) {
				PutChar('<');
				PutChar('D');
				PutChar('E');
				PutChar('L');
				PutChar('>');
			}
			else
				PutChar(b);
		}

		if (CharAttr.Attr != svCharAttr) {
			UpdateStr();
			CharAttr.Attr = svCharAttr;
		}
		InsertMode = svInsertMode;
		AutoWrapMode = svAutoWrapMode;
	}
}

void PrnParseControl(BYTE b) // printer mode
{
	switch (b) {
	case NUL:
		return;
	case SO:
		if ((ts.ISO2022Flag & ISO2022_SO) && ! DirectPrn) {
			if ((ts.Language==IdJapanese) &&
			    (ts.KanjiCode==IdJIS) &&
			    (ts.JIS7Katakana==1) &&
			    ((ts.TermFlag & TF_FIXEDJIS)!=0))
			{
				Gn[1] = IdKatakana;
			}
			Glr[0] = 1; /* LS1 */
			return;
		}
		break;
	case SI:
		if ((ts.ISO2022Flag & ISO2022_SI) && ! DirectPrn) {
			Glr[0] = 0; /* LS0 */
			return;
		}
		break;
	case DC1:
	case DC3:
		return;
	case ESC:
		ICount = 0;
		JustAfterESC = TRUE;
		ParseMode = ModeESC;
		WriteToPrnFile(0, TRUE); // flush prn buff
		return;
	case CSI:
		if (! Accept8BitCtrl) {
			PutChar(b); /* Disp C1 char in VT100 mode */
			return;
		}
		ClearParams();
		FirstPrm = TRUE;
		ParseMode = ModeCSI;
		WriteToPrnFile(0, TRUE); // flush prn buff
		WriteToPrnFile(b, FALSE);
		return;
	}
	/* send the uninterpreted character to printer */
	WriteToPrnFile(b, TRUE);
}

void ParseControl(BYTE b)
{
	if (PrinterMode) { // printer mode
		PrnParseControl(b);
		return;
	}

	if (b>=0x80) { /* C1 char */
		if (ts.Language==IdEnglish) { /* English mode */
			if (!Accept8BitCtrl) {
				PutChar(b); /* Disp C1 char in VT100 mode */
				return;
			}
		}
		else { /* Japanese mode */
			if ((ts.TermFlag & TF_ACCEPT8BITCTRL)==0) {
				return; /* ignore C1 char */
			}
			/* C1 chars are interpreted as C0 chars in VT100 mode */
			if (VTlevel < 2) {
				b = b & 0x7F;
			}
		}
	}
	switch (b) {
	/* C0 group */
	case ENQ:
		CommBinaryOut(&cv, &(ts.Answerback[0]), ts.AnswerbackLen);
		break;
	case BEL:
		if (ts.Beep != IdBeepOff)
			RingBell(ts.Beep);
		break;
	case BS:
		BackSpace();
		break;
	case HT:
		Tab();
		break;
	case LF:
		if (ts.CRReceive == IdLF) {
			// 受信時の改行コードが LF の場合は、サーバから LF のみが送られてくると仮定し、
			// CR+LFとして扱うようにする。
			// cf. http://www.neocom.ca/forum/viewtopic.php?t=216
			// (2007.1.21 yutaka)
			CarriageReturn(TRUE);
			LineFeed(b, TRUE);
			break;
		}
		else if (ts.CRReceive == IdAUTO) {
			// 9th Apr 2012: AUTO CR/LF mode (tentner)
			// a CR or LF will generated a CR+LF, if the next character is the opposite, it will be ignored
			if(PrevCharacter != CR || !PrevCRorLFGeneratedCRLF) {
				CarriageReturn(TRUE);
				LineFeed(b, TRUE);
				PrevCRorLFGeneratedCRLF = TRUE;
			}
			else {
				PrevCRorLFGeneratedCRLF = FALSE;
			}
			break;
		}

	case VT:
		LineFeed(b, TRUE);
		break;

	case FF:
		if ((ts.AutoWinSwitch>0) && JustAfterESC) {
			CommInsert1Byte(&cv, b);
			CommInsert1Byte(&cv, ESC);
			ChangeEmu = IdTEK;	/* Enter TEK Mode */
		}
		else
			LineFeed(b, TRUE);
		break;
	case CR:
		if (ts.CRReceive == IdAUTO) {
			// 9th Apr 2012: AUTO CR/LF mode (tentner)
			// a CR or LF will generated a CR+LF, if the next character is the opposite, it will be ignored
			if(PrevCharacter != LF || !PrevCRorLFGeneratedCRLF) {
				CarriageReturn(TRUE);
				LineFeed(b, TRUE);
				PrevCRorLFGeneratedCRLF = TRUE;
			}
			else {
				PrevCRorLFGeneratedCRLF = FALSE;
			}
		}
		else {
			CarriageReturn(TRUE);
			if (ts.CRReceive==IdCRLF) {
				CommInsert1Byte(&cv, LF);
			}
		}
		break;
	case SO: /* LS1 */
		if (ts.ISO2022Flag & ISO2022_SO) {
			if ((ts.Language==IdJapanese) &&
			    (ts.KanjiCode==IdJIS) &&
			    (ts.JIS7Katakana==1) &&
			    ((ts.TermFlag & TF_FIXEDJIS)!=0))
			{
				Gn[1] = IdKatakana;
			}

			Glr[0] = 1;
		}
		break;
	case SI: /* LS0 */
		if (ts.ISO2022Flag & ISO2022_SI) {
			Glr[0] = 0;
		}
		break;
	case DLE:
		if ((ts.FTFlag & FT_BPAUTO)!=0)
			ParseMode = ModeDLE; /* Auto B-Plus activation */
		break;
	case CAN:
		if ((ts.FTFlag & FT_ZAUTO)!=0)
			ParseMode = ModeCAN; /* Auto ZMODEM activation */
//		else if (ts.AutoWinSwitch>0)
//			ChangeEmu = IdTEK;	/* Enter TEK Mode */
		else
			ParseMode = ModeFirst;
		break;
	case SUB:
		ParseMode = ModeFirst;
		break;
	case ESC:
		ICount = 0;
		JustAfterESC = TRUE;
		ParseMode = ModeESC;
		break;
	case FS:
	case GS:
	case RS:
	case US:
		if (ts.AutoWinSwitch>0) {
			CommInsert1Byte(&cv, b);
			ChangeEmu = IdTEK;	/* Enter TEK Mode */
		}
		break;

	/* C1 char */
	case IND:
		LineFeed(0, TRUE);
		break;
	case NEL:
		LineFeed(0, TRUE);
		CarriageReturn(TRUE);
		break;
	case HTS:
		if (ts.TabStopFlag & TABF_HTS8)
			SetTabStop();
		break;
	case RI:
		CursorUpWithScroll();
		break;
	case SS2:
		if (ts.ISO2022Flag & ISO2022_SS2) {
			GLtmp = 2;
			SSflag = TRUE;
		}
		break;
	case SS3:
		if (ts.ISO2022Flag & ISO2022_SS3) {
			GLtmp = 3;
			SSflag = TRUE;
		}
		break;
	case DCS:
		ClearParams();
		ESCFlag = FALSE;
		ParseMode = ModeDCS;
		break;
	case SOS:
		ESCFlag = FALSE;
		ParseMode = ModeIgnore;
		break;
	case CSI:
		ClearParams();
		FirstPrm = TRUE;
		ParseMode = ModeCSI;
		break;
	case OSC:
		ClearParams();
		ParseMode = ModeXS;
		break;
	case PM:
	case APC:
		ESCFlag = FALSE;
		ParseMode = ModeIgnore;
		break;
	}
}

void SaveCursor()
{
	int i;
	PStatusBuff Buff;

	if (isCursorOnStatusLine)
		Buff = &SBuff2; // for status line
	else if (AltScr)
		Buff = &SBuff3; // for alternate screen
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

void RestoreCursor()
{
	int i;
	PStatusBuff Buff;

	UpdateStr();

	if (isCursorOnStatusLine)
		Buff = &SBuff2; // for status line
	else if (AltScr)
		Buff = &SBuff3; // for alternate screen
	else
		Buff = &SBuff1; // for main screen

	if (Buff->CursorX > NumOfColumns-1)
		Buff->CursorX = NumOfColumns-1;
	if (Buff->CursorY > NumOfLines-1-StatusLine)
		Buff->CursorY = NumOfLines-1-StatusLine;
	MoveCursor(Buff->CursorX, Buff->CursorY);

	CharAttr = Buff->Attr;
	BuffSetCurCharAttr(CharAttr);

	Glr[0] = Buff->Glr[0];
	Glr[1] = Buff->Glr[1];
	for (i=0 ; i<=3; i++)
		Gn[i] = Buff->Gn[i];

	AutoWrapMode = Buff->AutoWrapMode;
	RelativeOrgMode = Buff->RelativeOrgMode;
}

void AnswerTerminalType()
{
	char Tmp[50];

	if (ts.TerminalID<IdVT320 || !Send8BitMode)
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
	case IdVT420:
		strncat_s(Tmp,sizeof(Tmp),"64;1;2;7;8;9;15;18;21",_TRUNCATE);
		break;
	case IdVT520:
		strncat_s(Tmp,sizeof(Tmp),"65;1;2;7;8;9;12;18;19;21;23;24;42;44;45;46",_TRUNCATE);
		break;
	case IdVT525:
		strncat_s(Tmp,sizeof(Tmp),"65;1;2;7;9;12;18;19;21;22;23;24;42;44;45;46",_TRUNCATE);
		break;
	}
	strncat_s(Tmp,sizeof(Tmp),"c",_TRUNCATE);

	CommBinaryOut(&cv,Tmp,strlen(Tmp)); /* Report terminal ID */
}

void ESCSpace(BYTE b)
{
	switch (b) {
	case 'F':  // S7C1T
		Send8BitMode = FALSE;
		break;
	case 'G':  // S8C1T
		if (VTlevel >= 2) {
			Send8BitMode = TRUE;
		}
		break;
	}
}

void ESCSharp(BYTE b)
{
	switch (b) {
	case '8':  /* Fill screen with "E" (DECALN) */
		BuffUpdateScroll();
		BuffFillWithE();
		CursorTop = 0;
		CursorBottom = NumOfLines-1-StatusLine;
		CursorLeftM = 0;
		CursorRightM = NumOfColumns - 1;
		MoveCursor(0, 0);
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

	/* Intermediate char must be '(' or ')' or '*' or '+'. */
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

	if (((ts.TermFlag & TF_AUTOINVOKE)!=0) && (Dist==0))
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
			ClearParams();
			FirstPrm = TRUE;
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
			if (! DirectPrn) {
				ESCDBCSSelect(b);
				return;
			}
			break;
		case '(':
		case ')':
		case '*':
		case '+':
			if (! DirectPrn) {
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
	case 0: /* no intermediate char */
		switch (b) {
		case '6': // DECBI
			if (CursorY >= CursorTop && CursorY <= CursorBottom &&
			    CursorX >= CursorLeftM && CursorX <= CursorRightM) {
				if (CursorX == CursorLeftM)
					BuffScrollRight(1);
				else
					MoveCursor(CursorX-1, CursorY);
			}
		break;
		case '7': SaveCursor(); break;
		case '8': RestoreCursor(); break;
		case '9': // DECFI
			if (CursorY >= CursorTop && CursorY <= CursorBottom &&
			    CursorX >= CursorLeftM && CursorX <= CursorRightM) {
				if (CursorX == CursorRightM)
					BuffScrollLeft(1);
				else
					MoveCursor(CursorX+1, CursorY);
			}
			break;
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
			if (ts.TabStopFlag & TABF_HTS7)
				SetTabStop();
			break;
		case 'M': /* RI */
			CursorUpWithScroll();
			break;
		case 'N': /* SS2 */
			if (ts.ISO2022Flag & ISO2022_SS2) {
				GLtmp = 2;
				SSflag = TRUE;
			}
			break;
		case 'O': /* SS3 */
			if (ts.ISO2022Flag & ISO2022_SS3) {
				GLtmp = 3;
				SSflag = TRUE;
			}
			break;
		case 'P': /* DCS */
			ClearParams();
			ESCFlag = FALSE;
			ParseMode = ModeDCS;
			return;
		case 'X': /* SOS */
		case '^': /* APC */
		case '_': /* PM  */
			ESCFlag = FALSE;
			ParseMode = ModeIgnore;
			return;
		case 'Z': /* DECID */
			AnswerTerminalType();
			break;
		case '[': /* CSI */
			ClearParams();
			FirstPrm = TRUE;
			ParseMode = ModeCSI;
			return;
		case '\\': break; /* ST */
		case ']': /* XTERM sequence (OSC) */
			ClearParams();
			ParseMode = ModeXS;
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
			RingBell(IdBeepVisual);
			break;
		case 'n': /* LS2 */
			if (ts.ISO2022Flag & ISO2022_LS2) {
				Glr[0] = 2;
			}
			break;
		case 'o': /* LS3 */
			if (ts.ISO2022Flag & ISO2022_LS3) {
				Glr[0] = 3;
			}
			break;
		case '|': /* LS3R */
			if (ts.ISO2022Flag & ISO2022_LS3R) {
				Glr[1] = 3;
			}
			break;
		case '}': /* LS2R */
			if (ts.ISO2022Flag & ISO2022_LS2R) {
				Glr[1] = 2;
			}
			break;
		case '~': /* LS1R */
			if (ts.ISO2022Flag & ISO2022_LS1R) {
				Glr[1] = 1;
			}
			break;
		}
		break;
		/* end of case Icount=0 */

	case 1: /* one intermediate char */
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

	case 2: /* two intermediate char */
		if ((IntChar[1]=='$') && ('('<=IntChar[2]) && (IntChar[2]<='+'))
			ESCDBCSSelect(b);
		else if ((IntChar[1]=='%') && (IntChar[2]=='!'))
			ESCSelectCode(b);
		break;
	}
	ParseMode = ModeFirst;
}

void EscapeSequence(BYTE b)
{
	if (b<=US)
		ParseControl(b);
	else if ((b>=0x20) && (b<=0x2F)) {
		// TODO: ICount が IntCharMax に達した時、最後の IntChar を置き換えるのは妥当?
		if (ICount<IntCharMax)
			ICount++;
		IntChar[ICount] = b;
	}
	else if ((b>=0x30) && (b<=0x7E))
		ParseEscape(b);
	else if ((b>=0x80) && (b<=0x9F))
		ParseControl(b);
	else if (b>=0xA0) {
		ParseMode=ModeFirst;
		ParseFirst(b);
	}

	JustAfterESC = FALSE;
}

#define CheckParamVal(p,m) \
	if ((p) == 0) { \
		(p) = 1; \
	} \
	else if ((p) > (m) || p < 0) { \
		(p) = (m); \
	}

#define CheckParamValMax(p,m) \
	if ((p) > (m) || p <= 0) { \
		(p) = (m); \
	}

#define RequiredParams(n) \
	if ((n) > 1) { \
		while (NParam < n) { \
			NParam++; \
			Param[NParam] = 0; \
			NSParam[NParam] = 0; \
		} \
	}

void CSInsertCharacter()		// ICH
{
	// Insert space characters at cursor
	CheckParamVal(Param[1], NumOfColumns);

	BuffUpdateScroll();
	BuffInsertSpace(Param[1]);
}

void CSCursorUp(BOOL AffectMargin)	// CUU / VPB
{
	int topMargin, NewY;

	CheckParamVal(Param[1], CursorY);

	if (AffectMargin && CursorY >= CursorTop)
		topMargin = CursorTop;
	else
		topMargin = 0;

	NewY = CursorY - Param[1];
	if (NewY < topMargin)
		NewY = topMargin;

	MoveCursor(CursorX, NewY);
}

void CSCursorUp1()			// CPL
{
	MoveCursor(CursorLeftM, CursorY);
	CSCursorUp(TRUE);
}

void CSCursorDown(BOOL AffectMargin)	// CUD / VPR
{
	int bottomMargin, NewY;

	if (AffectMargin && CursorY <= CursorBottom)
		bottomMargin = CursorBottom;
	else
		bottomMargin = NumOfLines-StatusLine-1;

	CheckParamVal(Param[1], bottomMargin);

	NewY = CursorY + Param[1];
	if (NewY > bottomMargin)
		NewY = bottomMargin;

	MoveCursor(CursorX, NewY);
}

void CSCursorDown1()			// CNL
{
	MoveCursor(CursorLeftM, CursorY);
	CSCursorDown(TRUE);
}

void CSScreenErase()
{
	BuffUpdateScroll();
	switch (Param[1]) {
	  case 0:
		// <ESC>[H(Cursor in left upper corner)によりカーソルが左上隅を指している場合、
		// <ESC>[Jは<ESC>[2Jと同じことなので、処理を分け、現行バッファをスクロールアウト
		// させるようにする。(2005.5.29 yutaka)
		// コンフィグレーションで切り替えられるようにした。(2008.5.3 yutaka)
		if (ts.ScrollWindowClearScreen &&
			(CursorX == 0 && CursorY == 0)) {
			// Erase screen (scroll out)
			BuffClearScreen();
			UpdateWindow(HVTWin);

		} else {
			// Erase characters from cursor to the end of screen
			BuffEraseCurToEnd();
		}
		break;

	  case 1:
		// Erase characters from home to cursor
		BuffEraseHomeToCur();
		break;

	  case 2:
		// Erase screen (scroll out)
		BuffClearScreen();
		UpdateWindow(HVTWin);
		if (ClearThenHome && !isCursorOnStatusLine) {
			if (RelativeOrgMode) {
				MoveCursor(0, 0);
			}
			else {
				MoveCursor(CursorLeftM, CursorTop);
			}
		}
		break;
	}
}

void CSQSelScreenErase()
{
	BuffUpdateScroll();
	switch (Param[1]) {
	  case 0:
		// Erase characters from cursor to end
		BuffSelectedEraseCurToEnd();
		break;

	  case 1:
		// Erase characters from home to cursor
		BuffSelectedEraseHomeToCur();
		break;

	  case 2:
		// Erase entire screen
		BuffSelectedEraseScreen();
		break;
	}
}

void CSInsertLine()
{
	// Insert lines at current position
	int Count, YEnd;

	if (CursorY < CursorTop || CursorY > CursorBottom) {
		return;
	}

	CheckParamVal(Param[1], NumOfLines);

	Count = Param[1];

	YEnd = CursorBottom;
	if (CursorY > YEnd)
		YEnd = NumOfLines-1-StatusLine;

	if (Count > YEnd+1 - CursorY)
		Count = YEnd+1 - CursorY;

	BuffInsertLines(Count,YEnd);
}

void CSLineErase()
{
	BuffUpdateScroll();
	switch (Param[1]) {
	  case 0: /* erase char from cursor to end of line */
		BuffEraseCharsInLine(CursorX,NumOfColumns-CursorX);
		break;

	  case 1: /* erase char from start of line to cursor */
		BuffEraseCharsInLine(0,CursorX+1);
		break;

	  case 2: /* erase entire line */
		BuffEraseCharsInLine(0,NumOfColumns);
		break;
	}
}

void CSQSelLineErase()
{
	BuffUpdateScroll();
	switch (Param[1]) {
	  case 0: /* erase char from cursor to end of line */
		BuffSelectedEraseCharsInLine(CursorX,NumOfColumns-CursorX);
		break;

	  case 1: /* erase char from start of line to cursor */
		BuffSelectedEraseCharsInLine(0,CursorX+1);
		break;

	  case 2: /* erase entire line */
		BuffSelectedEraseCharsInLine(0,NumOfColumns);
		break;
	}
}

void CSDeleteNLines()
// Delete lines from current line
{
	int Count, YEnd;

	if (CursorY < CursorTop || CursorY > CursorBottom) {
		return;
	}

	CheckParamVal(Param[1], NumOfLines);
	Count = Param[1];

	YEnd = CursorBottom;
	if (CursorY > YEnd)
		YEnd = NumOfLines-1-StatusLine;

	if (Count > YEnd+1-CursorY)
		Count = YEnd+1-CursorY;

	BuffDeleteLines(Count,YEnd);
}

void CSDeleteCharacter()	// DCH
{
// Delete characters in current line from cursor
	CheckParamVal(Param[1], NumOfColumns);

	BuffUpdateScroll();
	BuffDeleteChars(Param[1]);
}

void CSEraseCharacter()		// ECH
{
	CheckParamVal(Param[1], NumOfColumns);

	BuffUpdateScroll();
	BuffEraseChars(Param[1]);
}

void CSScrollUp()
{
	// TODO: スクロールの最大値を端末行数に制限すべきか要検討
	CheckParamVal(Param[1], INT_MAX);

	BuffUpdateScroll();
	BuffRegionScrollUpNLines(Param[1]);
}

void CSScrollDown()
{
	CheckParamVal(Param[1], NumOfLines);

	BuffUpdateScroll();
	BuffRegionScrollDownNLines(Param[1]);
}

void CSForwardTab()
{
	CheckParamVal(Param[1], NumOfColumns);
	CursorForwardTab(Param[1], AutoWrapMode);
}

void CSBackwardTab()
{
	CheckParamVal(Param[1], NumOfColumns);
	CursorBackwardTab(Param[1]);
}

void CSMoveToColumnN()		// CHA / HPA
{
	CheckParamVal(Param[1], NumOfColumns);

	Param[1]--;

	if (RelativeOrgMode) {
		if (CursorLeftM + Param[1] > CursorRightM )
			MoveCursor(CursorRightM, CursorY);
		else
			MoveCursor(CursorLeftM + Param[1], CursorY);
	}
	else {
		MoveCursor(Param[1], CursorY);
	}
}

void CSCursorRight(BOOL AffectMargin)	// CUF / HPR
{
	int NewX, rightMargin;

	CheckParamVal(Param[1], NumOfColumns);

	if (AffectMargin && CursorX <= CursorRightM) {
		rightMargin = CursorRightM;
	}
	else {
		rightMargin = NumOfColumns-1;
	}

	NewX = CursorX + Param[1];
	if (NewX > rightMargin)
		NewX = rightMargin;

	MoveCursor(NewX, CursorY);
}

void CSCursorLeft(BOOL AffectMargin)	// CUB / HPB
{
	int NewX, leftMargin;

	CheckParamVal(Param[1], NumOfColumns);

	if (AffectMargin && CursorX >= CursorLeftM) {
		leftMargin = CursorLeftM;
	}
	else {
		leftMargin = 0;
	}

	NewX = CursorX  - Param[1];
	if (NewX < leftMargin) {
		NewX = leftMargin;
	}

	MoveCursor(NewX, CursorY);
}

void CSMoveToLineN()		// VPA
{
	CheckParamVal(Param[1], NumOfLines-StatusLine);

	if (RelativeOrgMode) {
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
	Fallbacked = FALSE;
}

void CSMoveToXY()		// CUP / HVP
{
	int NewX, NewY;

	RequiredParams(2);
	CheckParamVal(Param[1], NumOfLines-StatusLine);
	CheckParamVal(Param[2], NumOfColumns);

	NewY = Param[1] - 1;
	NewX = Param[2] - 1;

	if (isCursorOnStatusLine)
		NewY = CursorY;
	else if (RelativeOrgMode) {
		NewX += CursorLeftM;
		if (NewX > CursorRightM)
			NewX = CursorRightM;

		NewY += CursorTop;
		if (NewY > CursorBottom)
			NewY = CursorBottom;
	}
	else {
		if (NewY > NumOfLines-1-StatusLine)
			NewY = NumOfLines-1-StatusLine;
	}

	MoveCursor(NewX, NewY);
	Fallbacked = FALSE;
}

void CSDeleteTabStop()
{
	ClearTabStop(Param[1]);
}

void CS_h_Mode()		// SM
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
		if (ts.WindowFlag & WF_CURSORCHANGE) {
			ts.NonblinkingCursor = TRUE;
			ChangeCaret();
		}
		break;
	  case 34:	// WYULCURM
		if (ts.WindowFlag & WF_CURSORCHANGE) {
			ts.CursorShape = IdHCur;
			ChangeCaret();
		}
		break;
	}
}

void CS_i_Mode()		// MC
{
	switch (Param[1]) {
	  /* print screen */
	  //  PrintEX -- TRUE: print screen
	  //             FALSE: scroll region
	  case 0:
		if (ts.TermFlag&TF_PRINTERCTRL) {
			BuffPrint(! PrintEX);
		}
		break;
	  /* printer controller mode off */
	  case 4: break; /* See PrnParseCS() */
	  /* printer controller mode on */
	  case 5:
		if (ts.TermFlag&TF_PRINTERCTRL) {
			if (! AutoPrintMode)
				OpenPrnFile();
			DirectPrn = (ts.PrnDev[0]!=0);
			PrinterMode = TRUE;
		}
		break;
	}
}

void CS_l_Mode()		// RM
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
		if (ts.WindowFlag & WF_CURSORCHANGE) {
			ts.NonblinkingCursor = FALSE;
			ChangeCaret();
		}
		break;
	  case 34:	// WYULCURM
		if (ts.WindowFlag & WF_CURSORCHANGE) {
			ts.CursorShape = IdBlkCur;
			ChangeCaret();
		}
		break;
	}
}

void CS_n_Mode()		// DSR
{
	char Report[16];
	int X, Y, len;

	switch (Param[1]) {
	  case 5:
		/* Device Status Report -> Ready */
		SendCSIstr("0n", 0);
		break;
	  case 6:
		/* Cursor Position Report */
		if (isCursorOnStatusLine) {
			X = CursorX + 1;
			Y = 1;
		}
		else if (RelativeOrgMode) {
			X = CursorX - CursorLeftM + 1;
			Y = CursorY - CursorTop + 1;
		}
		else {
			X = CursorX + 1;
			Y = CursorY+1;
		}
		len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "%u;%uR", CLocale, Y, X);
		SendCSIstr(Report, len);
		break;
	}
}

void ParseSGRParams(PCharAttr attr, PCharAttr mask, int start)
{
	int i, j, P, r, g, b, color;
	TCharAttr dummy;

	if (mask == NULL) {
		mask = &dummy;
	}

	for (i=start ; i<=NParam ; i++) {
		P = Param[i];
		switch (P) {
		  case   0:	/* Clear all */
			attr->Attr = DefCharAttr.Attr;
			attr->Attr2 = DefCharAttr.Attr2 | (attr->Attr2&Attr2Protect);
			attr->Fore = DefCharAttr.Fore;
			attr->Back = DefCharAttr.Back;
			mask->Attr = AttrSgrMask;
			mask->Attr2 = Attr2ColorMask;
			break;

		  case   1:	/* Bold */
			attr->Attr |= AttrBold;
			mask->Attr |= AttrBold;
			break;

		  case   4:	/* Under line */
			attr->Attr |= AttrUnder;
			mask->Attr |= AttrUnder;
			break;

		  case   5:	/* Blink */
			attr->Attr |= AttrBlink;
			mask->Attr |= AttrBlink;
			break;

		  case   7:	/* Reverse */
			attr->Attr |= AttrReverse;
			mask->Attr |= AttrReverse;
			break;

		  case  22:	/* Bold off */
			attr->Attr &= ~ AttrBold;
			mask->Attr |= AttrBold;
			break;

		  case  24:	/* Under line off */
			attr->Attr &= ~ AttrUnder;
			mask->Attr |= AttrUnder;
			break;

		  case  25:	/* Blink off */
			attr->Attr &= ~ AttrBlink;
			mask->Attr |= AttrBlink;
			break;

		  case  27:	/* Reverse off */
			attr->Attr &= ~ AttrReverse;
			mask->Attr |= AttrReverse;
			break;

		  case  30:
		  case  31:
		  case  32:
		  case  33:
		  case  34:
		  case  35:
		  case  36:
		  case  37:	/* text color */
			attr->Attr2 |= Attr2Fore;
			mask->Attr2 |= Attr2Fore;
			attr->Fore = P - 30;
			break;

		  case  38:	/* text color (256color mode) */
			if (ts.ColorFlag & CF_XTERM256) {
				/*
				 * Change foreground color. accept following formats.
				 *
				 * 38 ; 2 ; r ; g ; b
				 * 38 ; 2 : r : g : b
				 * 38 : 2 : r : g : b
				 * 38 ; 5 ; idx
				 * 38 ; 5 : idx
				 * 38 : 5 : idx
				 *
				 */
				color = -1;
				j = 0;
				if (NSParam[i] > 0) {
					P = SubParam[i][1];
					j++;
				}
				else if (i < NParam) {
					P = Param[i+1];
					if (P == 2 || P == 5) {
						i++;
					}
				}
				switch (P) {
				  case 2:
					r = g = b = 0;
					if (NSParam[i] > 0) {
						if (j < NSParam[i]) {
							r = SubParam[i][++j];
							if (j < NSParam[i]) {
								g = SubParam[i][++j];
							}
							if (j < NSParam[i]) {
								b = SubParam[i][++j];
							}
							color = DispFindClosestColor(r, g, b);
						}
					}
					else if (i < NParam && NSParam[i+1] > 0) {
						r = Param[++i];
						g = SubParam[i][1];
						if (NSParam[i] > 1) {
							b = SubParam[i][2];
						}
						color = DispFindClosestColor(r, g, b);
					}
					else if (i+2 < NParam) {
						r = Param[++i];
						g = Param[++i];
						b = Param[++i];
						color = DispFindClosestColor(r, g, b);
					}
					break;
				  case 5:
					if (NSParam[i] > 0) {
						if (j < NSParam[i]) {
							color = SubParam[i][++j];
						}
					}
					else if (i < NParam) {
						color = Param[++i];
					}
					break;
				}
				if (color >= 0 && color < 256) {
					attr->Attr2 |= Attr2Fore;
					mask->Attr2 |= Attr2Fore;
					attr->Fore = color;
				}
			}
			break;

		  case  39:	/* Reset text color */
			attr->Attr2 &= ~ Attr2Fore;
			mask->Attr2 |= Attr2Fore;
			attr->Fore = AttrDefaultFG;
			break;

		  case  40:
		  case  41:
		  case  42:
		  case  43:
		  case  44:
		  case  45:
		  case  46:
		  case  47:	/* Back color */
			attr->Attr2 |= Attr2Back;
			mask->Attr2 |= Attr2Back;
			attr->Back = P - 40;
			break;

		  case  48:	/* Back color (256color mode) */
			if (ts.ColorFlag & CF_XTERM256) {
				color = -1;
				j = 0;
				if (NSParam[i] > 0) {
					P = SubParam[i][1];
					j++;
				}
				else if (i < NParam) {
					P = Param[i+1];
					if (P == 2 || P == 5) {
						i++;
					}
				}
				switch (P) {
				  case 2:
					r = g = b = 0;
					if (NSParam[i] > 0) {
						if (j < NSParam[i]) {
							r = SubParam[i][++j];
							if (j < NSParam[i]) {
								g = SubParam[i][++j];
							}
							if (j < NSParam[i]) {
								b = SubParam[i][++j];
							}
							color = DispFindClosestColor(r, g, b);
						}
					}
					else if (i < NParam && NSParam[i+1] > 0) {
						r = Param[++i];
						g = SubParam[i][1];
						if (NSParam[i] > 1) {
							b = SubParam[i][2];
						}
						color = DispFindClosestColor(r, g, b);
					}
					else if (i+2 < NParam) {
						r = Param[++i];
						g = Param[++i];
						b = Param[++i];
						color = DispFindClosestColor(r, g, b);
					}
					break;
				  case 5:
					if (NSParam[i] > 0) {
						if (j < NSParam[i]) {
							color = SubParam[i][++j];
						}
					}
					else if (i < NParam) {
						color = Param[++i];
					}
					break;
				}
				if (color >= 0 && color < 256) {
					attr->Attr2 |= Attr2Back;
					mask->Attr2 |= Attr2Back;
					attr->Back = color;
				}
			}
			break;

		  case  49:	/* Reset back color */
			attr->Attr2 &= ~ Attr2Back;
			mask->Attr2 |= Attr2Back;
			attr->Back = AttrDefaultBG;
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
				attr->Attr2 |= Attr2Fore;
				mask->Attr2 |= Attr2Fore;
				attr->Fore = P - 90 + 8;
			}
			break;

		  case 100:
			if (! (ts.ColorFlag & CF_AIXTERM16)) {
				/* Reset text and back color */
				attr->Attr2 &= ~ (Attr2Fore | Attr2Back);
				mask->Attr2 |= Attr2ColorMask;
				attr->Fore = AttrDefaultFG;
				attr->Back = AttrDefaultBG;
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
				attr->Attr2 |= Attr2Back;
				mask->Attr2 |= Attr2Back;
				attr->Back = P - 100 + 8;
			}
			break;
		}
	}
}

void CSSetAttr()		// SGR
{
	UpdateStr();
	ParseSGRParams(&CharAttr, NULL, 1);
	BuffSetCurCharAttr(CharAttr);
}

void CSSetScrollRegion()	// DECSTBM
{
	if (isCursorOnStatusLine) {
		MoveCursor(0,CursorY);
		return;
	}

	RequiredParams(2);
	CheckParamVal(Param[1], NumOfLines-StatusLine);
	CheckParamValMax(Param[2], NumOfLines-StatusLine);

	if (Param[1] >= Param[2])
		return;

	CursorTop = Param[1] - 1;
	CursorBottom = Param[2] - 1;

	if (RelativeOrgMode)
		// TODO: 左マージンを無視してる。要実機確認。
		MoveCursor(0, CursorTop);
	else
		MoveCursor(0, 0);
}

void CSSetLRScrollRegion()	// DECSLRM
{
//	TODO: ステータスライン上での挙動確認。
//	if (isCursorOnStatusLine) {
//		MoveCursor(0,CursorY);
//		return;
//	}

	RequiredParams(2);
	CheckParamVal(Param[1], NumOfColumns);
	CheckParamValMax(Param[2], NumOfColumns);

	if (Param[1] >= Param[2])
		return;

	CursorLeftM = Param[1] - 1;
	CursorRightM = Param[2] - 1;

	if (RelativeOrgMode)
		MoveCursor(CursorLeftM, CursorTop);
	else
		MoveCursor(0, 0);
}

void CSSunSequence() /* Sun terminal private sequences */
{
	int x, y, len;
	char Report[TitleBuffSize*2+10];
	PTStack t;

	switch (Param[1]) {
	  case 1: // De-iconify window
		if (ts.WindowFlag & WF_WINDOWCHANGE)
			DispShowWindow(WINDOW_RESTORE);
		break;

	  case 2: // Iconify window
		if (ts.WindowFlag & WF_WINDOWCHANGE)
			DispShowWindow(WINDOW_MINIMIZE);
		break;

	  case 3: // set window position
		if (ts.WindowFlag & WF_WINDOWCHANGE) {
			RequiredParams(3);
			DispMoveWindow(Param[2], Param[3]);
		}
		break;

	  case 4: // set window size
		if (ts.WindowFlag & WF_WINDOWCHANGE) {
			RequiredParams(3);
			DispResizeWin(Param[3], Param[2]);
		}
		break;

	  case 5: // Raise window
		if (ts.WindowFlag & WF_WINDOWCHANGE)
			DispShowWindow(WINDOW_RAISE);
		break;

	  case 6: // Lower window
		if (ts.WindowFlag & WF_WINDOWCHANGE)
			DispShowWindow(WINDOW_LOWER);
		break;

	  case 7: // Refresh window
		if (ts.WindowFlag & WF_WINDOWCHANGE)
			DispShowWindow(WINDOW_REFRESH);
		break;

	  case 8: /* set terminal size */
		if (ts.WindowFlag & WF_WINDOWCHANGE) {
			RequiredParams(3);
			if (Param[2] <= 1) Param[2] = 24;
			if (Param[3] <= 1) Param[3] = 80;
			ChangeTerminalSize(Param[3], Param[2]);
		}
		break;

	  case 9: // Maximize/Restore window
		if (ts.WindowFlag & WF_WINDOWCHANGE) {
			RequiredParams(2);
			if (Param[2] == 0) {
				DispShowWindow(WINDOW_RESTORE);
			}
			else if (Param[2] == 1) {
				DispShowWindow(WINDOW_MAXIMIZE);
			}
		}
		break;

	  case 11: // Report window state
		if (ts.WindowFlag & WF_WINDOWREPORT) {
			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "%dt", CLocale, DispWindowIconified()?2:1);
			SendCSIstr(Report, len);
		}
		break;

	  case 13: // Report window position
		if (ts.WindowFlag & WF_WINDOWREPORT) {
			RequiredParams(2);
			switch (Param[2]) {
			  case 0:
			  case 1:
				DispGetWindowPos(&x, &y, FALSE);
				break;
			  case 2:
				DispGetWindowPos(&x, &y, TRUE);
				break;
			  default:
				return;
			}
			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "3;%u;%ut", CLocale, (unsigned int)x, (unsigned int)y);
			SendCSIstr(Report, len);
		}
		break;

	  case 14: /* get window size */
		if (ts.WindowFlag & WF_WINDOWREPORT) {
			RequiredParams(2);
			switch (Param[2]) {
			  case 0:
			  case 1:
				DispGetWindowSize(&x, &y, TRUE);
				break;
			  case 2:
				DispGetWindowSize(&x, &y, FALSE);
				break;
			  default:
				return;
			}

			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "4;%d;%dt", CLocale, y, x);
			SendCSIstr(Report, len);
		}
		break;

	  case 15: // Report display size (pixel)
		if (ts.WindowFlag & WF_WINDOWREPORT) {
			DispGetRootWinSize(&x, &y, TRUE);
			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "5;%d;%dt", CLocale, y, x);
			SendCSIstr(Report, len);
		}
		break;

	  case 18: /* get terminal size */
		if (ts.WindowFlag & WF_WINDOWREPORT) {
			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "8;%u;%ut", CLocale,
			                    NumOfLines-StatusLine, NumOfColumns);
			SendCSIstr(Report, len);
		}
		break;

	  case 19: // Report display size (character)
		if (ts.WindowFlag & WF_WINDOWREPORT) {
			DispGetRootWinSize(&x, &y, FALSE);
			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "9;%d;%dt", CLocale, y, x);
			SendCSIstr(Report, len);
		}
		break;

	  case 20: // Report icon label
		switch (ts.WindowFlag & WF_TITLEREPORT) {
		  case IdTitleReportIgnore:
			// nothing to do
			break;

		  case IdTitleReportAccept:
			switch (ts.AcceptTitleChangeRequest) {
			  case IdTitleChangeRequestOff:
				len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "L%s", CLocale, ts.Title);
				break;

			  case IdTitleChangeRequestAhead:
				len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "L%s %s", CLocale, cv.TitleRemote, ts.Title);
				break;

			  case IdTitleChangeRequestLast:
				len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "L%s %s", CLocale, ts.Title, cv.TitleRemote);
				break;

			  default:
				if (cv.TitleRemote[0] == 0) {
					len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "L%s", CLocale, ts.Title);
				}
				else {
					len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "L%s", CLocale, cv.TitleRemote);
				}
			}
			SendOSCstr(Report, len, ST);
			break;

		  default: // IdTitleReportEmpty:
			SendOSCstr("L", 0, ST);
			break;
		}
		break;

	  case 21: // Report window title
		switch (ts.WindowFlag & WF_TITLEREPORT) {
		  case IdTitleReportIgnore:
			// nothing to do
			break;

		  case IdTitleReportAccept:
			switch (ts.AcceptTitleChangeRequest) {
			  case IdTitleChangeRequestOff:
				len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "l%s", CLocale, ts.Title);
				break;

			  case IdTitleChangeRequestAhead:
				len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "l%s %s", CLocale, cv.TitleRemote, ts.Title);
				break;

			  case IdTitleChangeRequestLast:
				len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "l%s %s", CLocale, ts.Title, cv.TitleRemote);
				break;

			  default:
				if (cv.TitleRemote[0] == 0) {
					len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "l%s", CLocale, ts.Title);
				}
				else {
					len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "l%s", CLocale, cv.TitleRemote);
				}
			}
			SendOSCstr(Report, len, ST);
			break;

		  default: // IdTitleReportEmpty:
			SendOSCstr("l", 0, ST);
			break;
		}
		break;

	  case 22: // Push Title
		RequiredParams(2);
		switch (Param[2]) {
		  case 0:
		  case 1:
		  case 2:
			if (ts.AcceptTitleChangeRequest && (t=malloc(sizeof(TStack))) != NULL) {
				if ((t->title = _strdup(cv.TitleRemote)) != NULL) {
					t->next = TitleStack;
					TitleStack = t;
				}
				else {
					free(t);
				}
			}
			break;
		}
		break;

	  case 23: // Pop Title
		RequiredParams(2);
		switch (Param[2]) {
		  case 0:
		  case 1:
		  case 2:
			if (ts.AcceptTitleChangeRequest && TitleStack != NULL) {
				t = TitleStack;
				TitleStack = t->next;
				strncpy_s(cv.TitleRemote, sizeof(cv.TitleRemote), t->title, _TRUNCATE);
				ChangeTitle();
				free(t->title);
				free(t);
			}
			break;
		}
	}
}

void CSLT(BYTE b)
{
	switch (b) {
	  case 'r':
		if (CanUseIME()) {
			SetIMEOpenStatus(IMEstat);
		}
		break;

	  case 's':
		if (CanUseIME()) {
			IMEstat = GetIMEOpenStatus();
		}
		break;

	  case 't':
		if (CanUseIME()) {
			SetIMEOpenStatus(Param[1] == 1);
		}
		break;
	}
}

void CSEQ(BYTE b)
{
	char Report[16];
	int len;

	switch (b) {
	  case 'c': /* Tertiary terminal report (Tertiary DA) */
		if (Param[1] == 0) {
			len = _snprintf_s_l(Report, sizeof(Report), _TRUNCATE, "!|%8s", CLocale, ts.TerminalUID);
			SendDCSstr(Report, len);
		}
		break;
	}
}

void CSGT(BYTE b)
{
	switch (b) {
	  case 'c': /* second terminal report (Secondary DA) */
		if (Param[1] == 0) {
			SendCSIstr(">32;331;0c", 0); /* VT382(>32) + xterm rev 331 */
		}
		break;

	  case 'J': // IO-8256 terminal
		if (Param[1]==3) {
			RequiredParams(5);
			CheckParamVal(Param[2], NumOfLines-StatusLine);
			CheckParamVal(Param[3], NumOfColumns);
			CheckParamValMax(Param[4], NumOfLines-StatusLine);
			CheckParamValMax(Param[5], NumOfColumns);

			if (Param[2] > Param[4] || Param[3] > Param[5]) {
				return;
			}

			BuffEraseBox(Param[3]-1, Param[2]-1, Param[5]-1, Param[4]-1);
		}
		break;

	  case 'K': // IO-8256 terminal
		switch (Param[1]) {
		  case 3:
			RequiredParams(3);
			CheckParamVal(Param[2], NumOfColumns);
			CheckParamVal(Param[3], NumOfColumns);

			if (Param[2] > Param[3]) {
				return;
			}

			BuffEraseCharsInLine(Param[2]-1, Param[3]-Param[2]+1);
			break;

		  case 5:
			RequiredParams(3);
			switch (Param[2]) {
			  case 3:
			  case 4:
			  case 5:
			  case 6: // Draw Line
				BuffDrawLine(CharAttr, Param[2], Param[3]);
				break;

			  case 12: // Text color
				if ((Param[3]>=0) && (Param[3]<=7)) {
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
			break;
		}
		break;
	}
}

void CSQExchangeColor()		// DECSCNM / Visual Bell
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
	BGExchangeColor();
#endif
	DispChangeBackground();
	UpdateWindow(HVTWin);
}

void CSQChangeColumnMode(int width)		// DECCOLM
{
	ChangeTerminalSize(width, NumOfLines-StatusLine);
	if ((ts.TermFlag & TF_CLEARONRESIZE) == 0) {
		MoveCursor(0, 0);
		BuffClearScreen();
		UpdateWindow(HVTWin);
	}
}

void CSQ_h_Mode() // DECSET
{
	int i;

	for (i = 1 ; i<=NParam ; i++) {
		switch (Param[i]) {
		  case 1: AppliCursorMode = TRUE; break;		// DECCKM
		  case 3: CSQChangeColumnMode(132); break;		// DECCOLM
		  case 5: /* Reverse Video (DECSCNM) */
			if (!(ts.ColorFlag & CF_REVERSEVIDEO))
				CSQExchangeColor(); /* Exchange text/back color */
			break;
		  case 6: // DECOM
			if (isCursorOnStatusLine)
				MoveCursor(0,CursorY);
			else {
				RelativeOrgMode = TRUE;
				MoveCursor(0,CursorTop);
			}
			break;
		  case 7: AutoWrapMode = TRUE; break;		// DECAWM
		  case 8: AutoRepeatMode = TRUE; break;		// DECARM
		  case 9: /* X10 Mouse Tracking */
			if (ts.MouseEventTracking)
				MouseReportMode = IdMouseTrackX10;
			break;
		  case 12: /* att610 cursor blinking */
			if (ts.WindowFlag & WF_CURSORCHANGE) {
				ts.NonblinkingCursor = FALSE;
				ChangeCaret();
			}
			break;
		  case 19: PrintEX = TRUE; break;		// DECPEX
		  case 25: DispEnableCaret(TRUE); break;	// cursor on (DECTCEM)
		  case 38: // DECTEK
			if (ts.AutoWinSwitch>0)
				ChangeEmu = IdTEK; /* Enter TEK Mode */
			break;
		  case 47: // Alternate Screen Buffer
			if ((ts.TermFlag & TF_ALTSCR) && !AltScr) {
				BuffSaveScreen();
				AltScr = TRUE;
			}
			break;
		  case 59:
			if (ts.Language==IdJapanese) {
				/* kanji terminal */
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
		  case 66: AppliKeyMode = TRUE; break;		// DECNKM
		  case 67: ts.BSKey = IdBS; break;		// DECBKM
		  case 69: LRMarginMode = TRUE; break;		// DECLRMM (DECVSSM)
		  case 1000: // Mouse Tracking
			if (ts.MouseEventTracking)
				MouseReportMode = IdMouseTrackVT200;
			break;
		  case 1001: // Hilite Mouse Tracking
			if (ts.MouseEventTracking)
				MouseReportMode = IdMouseTrackVT200Hl;
			break;
		  case 1002: // Button-Event Mouse Tracking
			if (ts.MouseEventTracking)
				MouseReportMode = IdMouseTrackBtnEvent;
			break;
		  case 1003: // Any-Event Mouse Tracking
			if (ts.MouseEventTracking)
				MouseReportMode = IdMouseTrackAllEvent;
			break;
		  case 1004: // Focus Report
			if (ts.MouseEventTracking)
				FocusReportMode = TRUE;
			break;
		  case 1005: // Extended Mouse Tracking (UTF-8)
			if (ts.MouseEventTracking)
				MouseReportExtMode = IdMouseTrackExtUTF8;
			break;
		  case 1006: // Extended Mouse Tracking (SGR)
			if (ts.MouseEventTracking)
				MouseReportExtMode = IdMouseTrackExtSGR;
			break;
		  case 1015: // Extended Mouse Tracking (rxvt-unicode)
			if (ts.MouseEventTracking)
				MouseReportExtMode = IdMouseTrackExtURXVT;
			break;
		  case 1047: // Alternate Screen Buffer
			if ((ts.TermFlag & TF_ALTSCR) && !AltScr) {
				BuffSaveScreen();
				AltScr = TRUE;
			}
			break;
		  case 1048: // Save Cursor Position (Alternate Screen Buffer)
			if (ts.TermFlag & TF_ALTSCR) {
				SaveCursor();
			}
			break;
		  case 1049: // Alternate Screen Buffer
			if ((ts.TermFlag & TF_ALTSCR) && !AltScr) {
				SaveCursor();
				BuffSaveScreen();
				BuffClearScreen();
				AltScr = TRUE;
			}
			break;
		  case 2004: // Bracketed Paste Mode
			BracketedPaste = TRUE;
			break;
		  case 7727: // mintty Application Escape Mode
			AppliEscapeMode = 1;
			break;
		  case 7786: // Wheel to Cursor translation
			if (ts.TranslateWheelToCursor) {
				AcceptWheelToCursor = TRUE;
			}
			break;
		  case 8200: // ClearThenHome
			ClearThenHome = TRUE;
			break;
		  case 14001: // NetTerm mouse mode
			if (ts.MouseEventTracking)
				MouseReportMode = IdMouseTrackNetTerm;
			break;
		  case 14002: // test Application Escape Mode 2
		  case 14003: // test Application Escape Mode 3
		  case 14004: // test Application Escape Mode 4
			AppliEscapeMode = Param[i] - 14000;
			break;
		}
	}
}

void CSQ_i_Mode()		// DECMC
{
	switch (Param[1]) {
	  case 1:
		if (ts.TermFlag&TF_PRINTERCTRL) {
			OpenPrnFile();
			BuffDumpCurrentLine(LF);
			if (! AutoPrintMode)
				ClosePrnFile();
		}
		break;
	  /* auto print mode off */
	  case 4:
		if (AutoPrintMode) {
			ClosePrnFile();
			AutoPrintMode = FALSE;
		}
		break;
	  /* auto print mode on */
	  case 5:
		if (ts.TermFlag&TF_PRINTERCTRL) {
			if (! AutoPrintMode) {
				OpenPrnFile();
				AutoPrintMode = TRUE;
			}
		}
		break;
	}
}

void CSQ_l_Mode()		// DECRST
{
	int i;

	for (i = 1 ; i <= NParam ; i++) {
		switch (Param[i]) {
		  case 1: AppliCursorMode = FALSE; break;	// DECCKM
		  case 3: CSQChangeColumnMode(80); break;	// DECCOLM
		  case 5: /* Normal Video (DECSCNM) */
			if (ts.ColorFlag & CF_REVERSEVIDEO)
				CSQExchangeColor(); /* Exchange text/back color */
			break;
		  case 6: // DECOM
			if (isCursorOnStatusLine)
				MoveCursor(0,CursorY);
			else {
				RelativeOrgMode = FALSE;
				MoveCursor(0,0);
			}
			break;
		  case 7: AutoWrapMode = FALSE; break;		// DECAWM
		  case 8: AutoRepeatMode = FALSE; break;	// DECARM
		  case 9: MouseReportMode = IdMouseTrackNone; break; /* X10 Mouse Tracking */
		  case 12: /* att610 cursor blinking */
			if (ts.WindowFlag & WF_CURSORCHANGE) {
				ts.NonblinkingCursor = TRUE;
				ChangeCaret();
			}
			break;
		  case 19: PrintEX = FALSE; break;		// DECPEX
		  case 25: DispEnableCaret(FALSE); break;	// cursor off (DECTCEM)
		  case 47: // Alternate Screen Buffer
			if ((ts.TermFlag & TF_ALTSCR) && AltScr) {
				BuffRestoreScreen();
				AltScr = FALSE;
			}
			break;
		  case 59:
			if (ts.Language==IdJapanese) {
				/* katakana terminal */
				Gn[0] = IdASCII;
				Gn[1] = IdKatakana;
				Gn[2] = IdKatakana;
				Gn[3] = IdKanji;
				Glr[0] = 0;
				if ((ts.KanjiCode==IdJIS) &&
				    (ts.JIS7Katakana==0))
					Glr[1] = 2;	// 8-bit katakana
				else
					Glr[1] = 3;
			}
			break;
		  case 66: AppliKeyMode = FALSE; break;		// DECNKM
		  case 67: ts.BSKey = IdDEL; break;		// DECBKM
		  case 69: // DECLRMM (DECVSSM)
			LRMarginMode = FALSE;
			CursorLeftM = 0;
			CursorRightM = NumOfColumns - 1;
			break;
		  case 1000: // Mouse Tracking
		  case 1001: // Hilite Mouse Tracking
		  case 1002: // Button-Event Mouse Tracking
		  case 1003: // Any-Event Mouse Tracking
			MouseReportMode = IdMouseTrackNone;
			break;
		  case 1004: // Focus Report
			FocusReportMode = FALSE;
			break;
		  case 1005: // Extended Mouse Tracking (UTF-8)
		  case 1006: // Extended Mouse Tracking (SGR)
		  case 1015: // Extended Mouse Tracking (rxvt-unicode)
			MouseReportExtMode = IdMouseTrackExtNone;
			break;
		  case 1047: // Alternate Screen Buffer
			if ((ts.TermFlag & TF_ALTSCR) && AltScr) {
				BuffClearScreen();
				BuffRestoreScreen();
				AltScr = FALSE;
			}
			break;
		  case 1048: // Save Cursor Position (Alternate Screen Buffer)
			if (ts.TermFlag & TF_ALTSCR) {
				RestoreCursor();
			}
			break;
		  case 1049: // Alternate Screen Buffer
			if ((ts.TermFlag & TF_ALTSCR) && AltScr) {
				BuffClearScreen();
				BuffRestoreScreen();
				AltScr = FALSE;
				RestoreCursor();
			}
			break;
		  case 2004: // Bracketed Paste Mode
			BracketedPaste = FALSE;
			break;
		  case 7727: // mintty Application Escape Mode
			AppliEscapeMode = 0;
			break;
		  case 7786: // Wheel to Cursor translation
			AcceptWheelToCursor = FALSE;
			break;
		  case 8200: // ClearThenHome
			ClearThenHome = FALSE;
			break;
		  case 14001: // NetTerm mouse mode
			MouseReportMode = IdMouseTrackNone;
			break;
		  case 14002: // test Application Escape Mode 2
		  case 14003: // test Application Escape Mode 3
		  case 14004: // test Application Escape Mode 4
			AppliEscapeMode = 0;
			break;
		}
	}
}

void CSQ_n_Mode()		// DECDSR
{
	switch (Param[1]) {
	  case 53:
	  case 55:
		/* Locator Device Status Report -> Ready */
		SendCSIstr("?50n", 0);
		break;
	}
}

void CSQuest(BYTE b)
{
	switch (b) {
	  case 'J': CSQSelScreenErase(); break;	// DECSED
	  case 'K': CSQSelLineErase(); break;	// DECSEL
	  case 'h': CSQ_h_Mode(); break;	// DECSET
	  case 'i': CSQ_i_Mode(); break;	// DECMC
	  case 'l': CSQ_l_Mode(); break;	// DECRST
	  case 'n': CSQ_n_Mode(); break;	// DECDSR
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
	AppliEscapeMode = FALSE;
	AcceptWheelToCursor = ts.TranslateWheelToCursor;
	if (isCursorOnStatusLine)
		MoveToMainScreen();
	CursorTop = 0;
	CursorBottom = NumOfLines-1-StatusLine;
	CursorLeftM = 0;
	CursorRightM = NumOfColumns - 1;
	ResetCharSet();

	/* Attribute */
	CharAttr = DefCharAttr;
	Special = FALSE;
	BuffSetCurCharAttr(CharAttr);

	// status buffers
	ResetCurSBuffer();

	// Saved IME status
	IMEstat = FALSE;
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
	  case 'p': // DECSCL
		/* Select terminal mode (software reset) */
		RequiredParams(2);

		SoftReset();
		ChangeTerminalID();
		if (Param[1] >= 61 && Param[1] <= 65) {
			if (VTlevel > Param[1] - 60) {
				VTlevel = Param[1] - 60;
			}
		}
		else {
			VTlevel = 1;
		}

		if (VTlevel < 2 || Param[2] == 1)
			Send8BitMode = FALSE;
		else
			Send8BitMode = TRUE;
		break;

	  case 'q': // DECSCA
		switch (Param[1]) {
		  case 0:
		  case 2:
			CharAttr.Attr2 &= ~Attr2Protect;
			BuffSetCurCharAttr(CharAttr);
			break;
		  case 1:
			CharAttr.Attr2 |= Attr2Protect;
			BuffSetCurCharAttr(CharAttr);
			break;
		  default:
			/* nothing to do */
			break;
		}
		break;
	}
}

void CSDolRequestMode() // DECRQM
{
	char buff[256];
	char *pp;
	int len, resp = 0;

	switch (Prv) {
	  case 0: /* ANSI Mode */
		resp = 4;
		pp = "";
		switch (Param[1]) {
		  case 2:	// KAM
			if (KeybEnabled)
				resp = 2;
			else
				resp = 1;
			break;
		  case 4:	// IRM
			if (InsertMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 12:	// SRM
			if (ts.LocalEcho)
				resp = 2;
			else
				resp = 1;
			break;
		  case 20:	// LNM
			if (LFMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 33:	// WYSTCURM
			if (ts.NonblinkingCursor)
				resp = 1;
			else
				resp = 2;
			if ((ts.WindowFlag & WF_CURSORCHANGE) == 0)
				resp += 2;
			break;
		  case 34:	// WYULCURM
			if (ts.CursorShape == IdHCur)
				resp = 1;
			else
				resp = 2;
			if ((ts.WindowFlag & WF_CURSORCHANGE) == 0)
				resp += 2;
			break;
		}
		break;

	  case '?': /* DEC Mode */
		pp = "?";
		switch (Param[1]) {
		  case 1:	// DECCKM
			if (AppliCursorMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 3:	// DECCOLM
			if (NumOfColumns == 132)
				resp = 1;
			else
				resp = 2;
			break;
		  case 5:	// DECSCNM
			if (ts.ColorFlag & CF_REVERSEVIDEO)
				resp = 1;
			else
				resp = 2;
			break;
		  case 6:	// DECOM
			if (RelativeOrgMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 7:	// DECAWM
			if (AutoWrapMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 8:	// DECARM
			if (AutoRepeatMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 9:	// XT_MSE_X10 -- X10 Mouse Tracking
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportMode == IdMouseTrackX10)
				resp = 1;
			else
				resp = 2;
			break;
		  case 12:	// XT_CBLINK -- att610 cursor blinking
			if (ts.NonblinkingCursor)
				resp = 2;
			else
				resp = 1;
			if ((ts.WindowFlag & WF_CURSORCHANGE) == 0)
				resp += 2;
			break;
		  case 19:	// DECPEX
			if (PrintEX)
				resp = 1;
			else
				resp = 2;
			break;
		  case 25:	// DECTCEM
			if (IsCaretEnabled())
				resp = 1;
			else
				resp = 2;
			break;
		  case 38:	// DECTEK
			resp = 4;
			break;
		  case 47:	// XT_ALTSCRN -- Alternate Screen / (DECGRPM)
			if ((ts.TermFlag & TF_ALTSCR) == 0)
				resp = 4;
			else if (AltScr)
				resp = 1;
			else
				resp = 2;
			break;
		  case 59:	// DECKKDM
			if (ts.Language!=IdJapanese)
				resp = 0;
			else if ((ts.KanjiCode == IdJIS) && (!ts.JIS7Katakana))
				resp = 4;
			else
				resp = 3;
			break;
		  case 66:	// DECNKM
			if (AppliKeyMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 67:	// DECBKM
			if (ts.BSKey==IdBS)
				resp = 1;
			else
				resp = 2;
			break;
		  case 69:	// DECRQM
			if (LRMarginMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case	1000:	// XT_MSE_X11
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportMode == IdMouseTrackVT200)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1001:	// XT_MSE_HL
#if 0
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportMode == IdMouseTrackVT200Hl)
				resp = 1;
			else
				resp = 2;
#else
			resp = 4;
#endif
			break;
		  case 1002:	// XT_MSE_BTN
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportMode == IdMouseTrackBtnEvent)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1003:	// XT_MSE_ANY
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportMode == IdMouseTrackAllEvent)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1004:	// XT_MSE_WIN
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (FocusReportMode)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1005:	// XT_MSE_UTF
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportExtMode == IdMouseTrackExtUTF8)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1006:	// XT_MSE_SGR
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportExtMode == IdMouseTrackExtSGR)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1015:	// urxvt-style extended mouse tracking
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportExtMode == IdMouseTrackExtURXVT)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1047:	// XT_ALTS_47
			if ((ts.TermFlag & TF_ALTSCR) == 0)
				resp = 4;
			else if (AltScr)
				resp = 1;
			else
				resp = 2;
			break;
		  case 1048:
			if ((ts.TermFlag & TF_ALTSCR) == 0)
				resp = 4;
			else
				resp = 1;
			break;
		  case 1049:	// XT_EXTSCRN
			if ((ts.TermFlag & TF_ALTSCR) == 0)
				resp = 4;
			else if (AltScr)
				resp = 1;
			else
				resp = 2;
			break;
		  case 2004:	// RL_BRACKET
			if (BracketedPaste)
				resp = 1;
			else
				resp = 2;
			break;
		  case 7727:	// MinTTY Application Escape Mode
			if (AppliEscapeMode == 1)
				resp = 1;
			else
				resp = 2;
			break;
		  case 7786:	// MinTTY Mousewheel reporting
			if (!ts.TranslateWheelToCursor)
				resp = 4;
			else if (AcceptWheelToCursor)
				resp = 1;
			else
				resp = 2;
			break;
		  case 8200:	// ClearThenHome
			if (ClearThenHome)
				resp = 1;
			else
				resp = 2;
			break;
		  case 14001:	// NetTerm Mouse Reporting (TT)
			if (!ts.MouseEventTracking)
				resp = 4;
			else if (MouseReportMode == IdMouseTrackNetTerm)
				resp = 1;
			else
				resp = 2;
			break;
		  case 14002:	// test Application Escape Mode 2
		  case 14003:	// test Application Escape Mode 3
		  case 14004:	// test Application Escape Mode 4
			if (AppliEscapeMode == Param[1] - 14000)
				resp = 1;
			else
				resp = 2;
			break;
		}
		break;
	}

	len = _snprintf_s(buff, sizeof(buff), _TRUNCATE, "%s%d;%d$y", pp, Param[1], resp);
	SendCSIstr(buff, len);
}

void CSDol(BYTE b)
{
	TCharAttr attr, mask;
	attr = DefCharAttr;
	mask = DefCharAttr;

	switch (b) {
	  case 'p': // DECRQM
		CSDolRequestMode();
		break;

	  case 'r': // DECCARA
	  case 't': // DECRARA
		RequiredParams(4);
		CheckParamVal(Param[1], NumOfLines-StatusLine);
		CheckParamVal(Param[2], NumOfColumns);
		CheckParamValMax(Param[3], NumOfLines-StatusLine);
		CheckParamValMax(Param[4], NumOfColumns);

		if (Param[1] > Param[3] || Param[2] > Param[4]) {
			return;
		}

		if (RelativeOrgMode) {
			Param[1] += CursorTop;
			if (Param[1] > CursorBottom) {
				Param[1] = CursorBottom + 1;
			}
			Param[3] += CursorTop;
			if (Param[3] > CursorBottom) {
				Param[3] = CursorBottom + 1;
			}

			// TODO: 左右マージンのチェックを行う。
		}

		ParseSGRParams(&attr, &mask, 5);
		if (b == 'r') { // DECCARA
			attr.Attr &= AttrSgrMask;
			mask.Attr &= AttrSgrMask;
			attr.Attr2 &= Attr2ColorMask;
			mask.Attr2 &= Attr2ColorMask;
			if (RectangleMode) {
				BuffChangeAttrBox(Param[2]-1, Param[1]-1, Param[4]-1, Param[3]-1, &attr, &mask);
			}
			else {
				BuffChangeAttrStream(Param[2]-1, Param[1]-1, Param[4]-1, Param[3]-1, &attr, &mask);
			}
		}
		else { // DECRARA
			attr.Attr &= AttrSgrMask;
			if (RectangleMode) {
			    BuffChangeAttrBox(Param[2]-1, Param[1]-1, Param[4]-1, Param[3]-1, &attr, NULL);
			}
			else {
			    BuffChangeAttrStream(Param[2]-1, Param[1]-1, Param[4]-1, Param[3]-1, &attr, NULL);
			}
		}
		break;

	  case 'v': // DECCRA
		RequiredParams(8);
		CheckParamVal(Param[1], NumOfLines-StatusLine);		// Src Y-start
		CheckParamVal(Param[2], NumOfColumns);			// Src X-start
		CheckParamValMax(Param[3], NumOfLines-StatusLine);	// Src Y-end
		CheckParamValMax(Param[4], NumOfColumns);		// Src X-end
		CheckParamVal(Param[5], 1);				// Src Page
		CheckParamVal(Param[6], NumOfLines-StatusLine);		// Dest Y
		CheckParamVal(Param[7], NumOfColumns);			// Dest X
		CheckParamVal(Param[8], 1);				// Dest Page

		if (Param[1] > Param[3] || Param[2] > Param[4]) {
			return;
		}

		if (RelativeOrgMode) {
			Param[1] += CursorTop;
			if (Param[1] > CursorBottom) {
				Param[1] = CursorBottom + 1;
			}
			Param[3] += CursorTop;
			if (Param[3] > CursorBottom) {
				Param[3] = CursorBottom + 1;
			}
			Param[6] += CursorTop;
			if (Param[6] > CursorBottom) {
				Param[6] = CursorBottom + 1;
			}
			if (Param[6] + Param[3] - Param[1] > CursorBottom) {
				Param[3] = Param[1] + CursorBottom - Param[6] + 1;
			}

			// TODO: 左右マージンのチェックを行う。
		}

		// TODO: 1 origin になっている。0 origin に直す。
		BuffCopyBox(Param[2], Param[1], Param[4], Param[3], Param[5], Param[7], Param[6], Param[8]);
		break;

	  case 'x': // DECFRA
		RequiredParams(5);
		if (Param[1] < 32 || (Param[1] > 127 && Param[1] < 160) || Param[1] > 255) {
			return;
		}
		CheckParamVal(Param[2], NumOfLines-StatusLine);
		CheckParamVal(Param[3], NumOfColumns);
		CheckParamValMax(Param[4], NumOfLines-StatusLine);
		CheckParamValMax(Param[5], NumOfColumns);

		if (Param[2] > Param[4] || Param[3] > Param[5]) {
			return;
		}

		if (RelativeOrgMode) {
			Param[2] += CursorTop;
			if (Param[2] > CursorBottom) {
				Param[2] = CursorBottom + 1;
			}
			Param[4] += CursorTop;
			if (Param[4] > CursorBottom) {
				Param[4] = CursorBottom + 1;
			}

			// TODO: 左右マージンのチェックを行う。
		}

		BuffFillBox(Param[1], Param[3]-1, Param[2]-1, Param[5]-1, Param[4]-1);
		break;

	  case 'z': // DECERA
	  case '{': // DECSERA
		RequiredParams(4);
		CheckParamVal(Param[1], NumOfLines-StatusLine);
		CheckParamVal(Param[2], NumOfColumns);
		CheckParamValMax(Param[3], NumOfLines-StatusLine);
		CheckParamValMax(Param[4], NumOfColumns);

		if (Param[1] > Param[3] || Param[2] > Param[4]) {
			return;
		}

		if (RelativeOrgMode) {
			Param[1] += CursorTop;
			if (Param[1] > CursorBottom) {
				Param[1] = CursorBottom + 1;
			}
			Param[3] += CursorTop;
			if (Param[3] > CursorBottom) {
				Param[3] = CursorBottom + 1;
			}

			// TODO: 左右マージンのチェックを行う。
		}

		if (b == 'z') {
			BuffEraseBox(Param[2]-1, Param[1]-1, Param[4]-1, Param[3]-1);
		}
		else {
			BuffSelectiveEraseBox(Param[2]-1, Param[1]-1, Param[4]-1, Param[3]-1);
		}
		break;

	  case '}': // DECSASD
		if ((ts.TermFlag & TF_ENABLESLINE) == 0 || !StatusLine) {
			return;
		}

		switch (Param[1]) {
		  case 0:
			if (isCursorOnStatusLine) {
				MoveToMainScreen();
			}
			break;

		  case 1:
			if (!isCursorOnStatusLine) {
				MoveToStatusLine();
			}
			break;
		}
		break;

	  case '~': // DECSSDT
		if ((ts.TermFlag & TF_ENABLESLINE)==0) {
			return;
		}

		switch (Param[1]) {
		  case 0:
		  case 1:
			HideStatusLine();
			break;
		  case 2:
			if (!StatusLine) {
				ShowStatusLine(1); // show
			}
			break;
		}
		break;
	}
}

void CSQDol(BYTE b)
{
	switch (b) {
	  case 'p':
		CSDolRequestMode();
		break;
	}
}

void CSQuote(BYTE b)
{
	int i, x, y;
	switch (b) {
	  case 'w': // Enable Filter Rectangle (DECEFR)
		if (MouseReportMode == IdMouseTrackDECELR) {
			RequiredParams(4);
			if (DecLocatorFlag & DecLocatorPixel) {
				x = LastX + 1;
				y = LastY + 1;
			}
			else {
				DispConvWinToScreen(LastX, LastY, &x, &y, NULL);
				x++;
				y++;
			}
			FilterTop    = (Param[1]==0)? y : Param[1];
			FilterLeft   = (Param[2]==0)? x : Param[2];
			FilterBottom = (Param[3]==0)? y : Param[3];
			FilterRight  = (Param[4]==0)? x : Param[4];
			if (FilterTop > FilterBottom) {
				i = FilterTop; FilterTop = FilterBottom; FilterBottom = i;
			}
			if (FilterLeft > FilterRight) {
				i = FilterLeft; FilterLeft = FilterRight; FilterRight = i;
			}
			DecLocatorFlag |= DecLocatorFiltered;
			DecLocatorReport(IdMouseEventMove, 0);
		}
		break;

	  case 'z': // Enable DEC Locator reporting (DECELR)
		switch (Param[1]) {
		  case 0:
			if (MouseReportMode == IdMouseTrackDECELR) {
				MouseReportMode = IdMouseTrackNone;
			}
			break;
		  case 1:
			if (ts.MouseEventTracking) {
				MouseReportMode = IdMouseTrackDECELR;
				DecLocatorFlag &= ~DecLocatorOneShot;
			}
			break;
		  case 2:
			if (ts.MouseEventTracking) {
				MouseReportMode = IdMouseTrackDECELR;
				DecLocatorFlag |= DecLocatorOneShot;
			}
			break;
		}
		if (NParam > 1 && Param[2] == 1) {
			DecLocatorFlag |= DecLocatorPixel;
		}
		else {
			DecLocatorFlag &= ~DecLocatorPixel;
		}
		break;

	  case '{': // Select Locator Events (DECSLE)
		for (i=1; i<=NParam; i++) {
			switch (Param[i]) {
			  case 0:
				DecLocatorFlag &= ~(DecLocatorButtonUp | DecLocatorButtonDown | DecLocatorFiltered);
				break;
			  case 1:
				DecLocatorFlag |= DecLocatorButtonDown;
				break;
			  case 2:
				DecLocatorFlag &= ~DecLocatorButtonDown;
				break;
			  case 3:
				DecLocatorFlag |= DecLocatorButtonUp;
				break;
			  case 4:
				DecLocatorFlag &= ~DecLocatorButtonUp;
				break;
			}
		}
		break;

	  case '|': // Request Locator Position (DECRQLP)
		DecLocatorReport(IdMouseEventCurStat, 0);
		break;
	}
}

void CSSpace(BYTE b) {
	switch (b) {
	  case 'q': // DECSCUSR
		if (ts.WindowFlag & WF_CURSORCHANGE) {
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

void CSAster(BYTE b)
{
	switch (b) {
	  case 'x': // DECSACE
		switch (Param[1]) {
		  case 0:
		  case 1:
			RectangleMode = FALSE;
			break;
		  case 2:
			RectangleMode = TRUE;
			break;
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
				if (Param[1]==4) {
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
	  case 0: /* no intermediate char */
		switch (Prv) {
		  case 0: /* no private parameter */
			switch (b) {
			// ISO/IEC 6429 / ECMA-48 Sequence
			  case '@': CSInsertCharacter(); break;   // ICH
			  case 'A': CSCursorUp(TRUE); break;      // CUU
			  case 'B': CSCursorDown(TRUE); break;    // CUD
			  case 'C': CSCursorRight(TRUE); break;   // CUF
			  case 'D': CSCursorLeft(TRUE); break;    // CUB
			  case 'E': CSCursorDown1(); break;       // CNL
			  case 'F': CSCursorUp1(); break;         // CPL
			  case 'G': CSMoveToColumnN(); break;     // CHA
			  case 'H': CSMoveToXY(); break;          // CUP
			  case 'I': CSForwardTab(); break;        // CHT
			  case 'J': CSScreenErase(); break;       // ED
			  case 'K': CSLineErase(); break;         // EL
			  case 'L': CSInsertLine(); break;        // IL
			  case 'M': CSDeleteNLines(); break;      // DL
//			  case 'N': break;                        // EF   -- Not support
//			  case 'O': break;                        // EA   -- Not support
			  case 'P': CSDeleteCharacter(); break;   // DCH
//			  case 'Q': break;                        // SEE  -- Not support
//			  case 'R': break;                        // CPR  -- Report only, ignore.
			  case 'S': CSScrollUp(); break;          // SU
			  case 'T': CSScrollDown(); break;        // SD
//			  case 'U': break;                        // NP   -- Not support
//			  case 'V': break;                        // PP   -- Not support
//			  case 'W': break;                        // CTC  -- Not support
			  case 'X': CSEraseCharacter(); break;    // ECH
//			  case 'Y': break;                        // CVT  -- Not support
			  case 'Z': CSBackwardTab(); break;       // CBT
//			  case '[': break;                        // SRS  -- Not support
//			  case '\\': break;                       // PTX  -- Not support
//			  case ']': break;                        // SDS  -- Not support
//			  case '^': break;                        // SIMD -- Not support
			  case '`': CSMoveToColumnN(); break;     // HPA
			  case 'a': CSCursorRight(FALSE); break;  // HPR
//			  case 'b': break;                        // REP  -- Not support
			  case 'c': AnswerTerminalType(); break;  // DA
			  case 'd': CSMoveToLineN(); break;       // VPA
			  case 'e': CSCursorDown(FALSE); break;   // VPR
			  case 'f': CSMoveToXY(); break;          // HVP
			  case 'g': CSDeleteTabStop(); break;     // TBC
			  case 'h': CS_h_Mode(); break;           // SM
			  case 'i': CS_i_Mode(); break;           // MC
			  case 'j': CSCursorLeft(FALSE); break;   // HPB
			  case 'k': CSCursorUp(FALSE); break;     // VPB
			  case 'l': CS_l_Mode(); break;           // RM
			  case 'm': CSSetAttr(); break;           // SGR
			  case 'n': CS_n_Mode(); break;           // DSR
//			  case 'o': break;                        // DAQ  -- Not support

			// Private Sequence
			  case 'r': CSSetScrollRegion(); break;   // DECSTBM
			  case 's':
				if (LRMarginMode)
					CSSetLRScrollRegion();    // DECSLRM
				else
					SaveCursor();             // SCP (Save cursor (ANSI.SYS/SCO?))
				break;
			  case 't': CSSunSequence(); break;       // DECSLPP / Window manipulation(dtterm?)
			  case 'u': RestoreCursor(); break;       // RCP (Restore cursor (ANSI.SYS/SCO))
			}
			break; /* end of case Prv=0 */
		  case '<': CSLT(b); break;    /* private parameter = '<' */
		  case '=': CSEQ(b); break;    /* private parameter = '=' */
		  case '>': CSGT(b); break;    /* private parameter = '>' */
		  case '?': CSQuest(b); break; /* private parameter = '?' */
		} /* end of switch (Prv) */
		break; /* end of no intermediate char */
	  case 1: /* one intermediate char */
		switch (Prv) {
		  case 0:
			switch (IntChar[1]) {
			  case ' ': CSSpace(b); break;  /* intermediate char = ' ' */
			  case '!': CSExc(b); break;    /* intermediate char = '!' */
			  case '"': CSDouble(b); break; /* intermediate char = '"' */
			  case '$': CSDol(b); break;    /* intermediate char = '$' */
			  case '*': CSAster(b); break;  /* intermediate char = '*' */
			  case '\'': CSQuote(b); break; /* intermediate char = '\'' */
			}
			break; /* end of case Prv=0 */
		  case '?':
			switch (IntChar[1]) {
			  case '$': CSQDol(b); break;    /* intermediate char = '$' */
			}
			break; /* end of case Prv=0 */
		} /* end of switch (Prv) */
		break; /* end of one intermediate char */
	} /* end of switch (Icount) */

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

		if ((b>=0x20) && (b<=0x2F)) { /* intermediate char */
			if (ICount<IntCharMax) ICount++;
			IntChar[ICount] = b;
		}
		else if ((b>=0x30) && (b<=0x39)) { /* parameter value */
#define ParamIncr(p, b) \
	do { \
		unsigned int ptmp; \
		if ((p) != (int)UINT_MAX) { \
			ptmp = (unsigned int)(p); \
			if (ptmp > UINT_MAX / 10 || ptmp * 10 > UINT_MAX - (b - 0x30)) { \
				(p) = (int)UINT_MAX; \
			} \
			else { \
				(p) = (int)(ptmp * 10 + b - 0x30); \
			} \
		} \
	} while (0);
			if (NSParam[NParam] > 0) {
				ParamIncr(SubParam[NParam][NSParam[NParam]], b);
			}
			else {
				ParamIncr(Param[NParam], b);
			}
		}
		else if (b==0x3A) { /* ':' Subparameter delimiter */
			if (NSParam[NParam] < NSParamMax) {
				NSParam[NParam]++;
				SubParam[NParam][NSParam[NParam]] = 0;
			}
		}
		else if (b==0x3B) { /* ';' Parameter delimiter */
			if (NParam < NParamMax) {
				NParam++;
				Param[NParam] = 0;
				NSParam[NParam] = 0;
			}
		}
		else if ((b>=0x3C) && (b<=0x3F)) { /* private char */
			if (FirstPrm) Prv = b;
		}
		else if (b>0xA0) {
			ParseMode=ModeFirst;
			ParseFirst(b);
		}
	}
	FirstPrm = FALSE;
}

int CheckUTF8Seq(BYTE b, int utf8_stat)
{
	if (ts.Language == IdUtf8 || (ts.Language==IdJapanese && (ts.KanjiCode==IdUTF8 || ts.KanjiCode==IdUTF8m))) {
		if (utf8_stat > 0) {
			if (b >= 0x80 && b < 0xc0) {
				utf8_stat -= 1;
			}
			else { // Invalid UTF-8 sequence
				utf8_stat = 0;
			}
		}
		else if (b < 0xc0) {
			; // nothing to do
		}
		else if (b < 0xe0) { // 2byte sequence
			utf8_stat = 1;
		}
		else if (b < 0xf0) { // 3byte sequence
			utf8_stat = 2;
		}
		else if (b < 0xf8) { // 4byte sequence
			utf8_stat = 3;
		}
	}
	return utf8_stat;
}

void IgnoreString(BYTE b)
{
	static int utf8_stat = 0;

	if ((ESCFlag && (b=='\\')) ||
	    (b<=US && b!=ESC && b!=HT) ||
	    (b==ST && ts.KanjiCode!=IdSJIS && utf8_stat == 0)) {
		ParseMode = ModeFirst;
	}

	if (b==ESC) {
		ESCFlag = TRUE;
	}
	else {
		ESCFlag = FALSE;
	}

	utf8_stat = CheckUTF8Seq(b, utf8_stat);
}

void RequestStatusString(unsigned char *StrBuff, int StrLen)	// DECRQSS
{
	unsigned char RepStr[256];
	int len = 0;
	int tmp = 0;

	switch (StrBuff[0]) {
	  case ' ':
		switch (StrBuff[1]) {
		  case 'q': // DECSCUSR
			switch (ts.CursorShape) {
			  case IdBlkCur:
				tmp = 1;
				break;
			  case IdHCur:
				tmp = 3;
				break;
			  case IdVCur:
				tmp = 5;
				break;
			  default:
				tmp = 1;
			}
			if (ts.NonblinkingCursor) {
				tmp++;
			}
			len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r%d q", CLocale, tmp);
		}
		break;
	  case '"':
		switch (StrBuff[1]) {
		  case 'p': // DECSCL
			if (VTlevel > 1 && Send8BitMode) {
				len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r6%d;0\"p", CLocale, VTlevel);
			}
			else {
				len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r6%d;1\"p", CLocale, VTlevel);
			}
			break;

		  case 'q': // DECSCA
			if (CharAttr.Attr2 & Attr2Protect) {
				len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r1\"q", CLocale);
			}
			else {
				len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r0\"q", CLocale);
			}
			break;
		}
		break;
	  case '*':
		switch (StrBuff[1]) {
		  case 'x': // DECSACE
			len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r%d*x", CLocale, RectangleMode?2:0);
			break;
		}
		break;
	  case 'm':	// SGR
		if (StrBuff[1] == 0) {
			len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r0", CLocale);
			if (CharAttr.Attr & AttrBold) {
				len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";1", CLocale);
			}
			if (CharAttr.Attr & AttrUnder) {
				len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";4", CLocale);
			}
			if (CharAttr.Attr & AttrBlink) {
				len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";5", CLocale);
			}
			if (CharAttr.Attr & AttrReverse) {
				len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";7", CLocale);
			}
			if (CharAttr.Attr2 & Attr2Fore && ts.ColorFlag & CF_ANSICOLOR) {
				int color = CharAttr.Fore;
				if (color <= 7 && (CharAttr.Attr & AttrBold) && (ts.ColorFlag & CF_PCBOLD16)) {
					color += 8;
				}

				if (color <= 7) {
					len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";3%d", CLocale, color);
				}
				else if (color <= 15) {
					if (ts.ColorFlag & CF_AIXTERM16) {
						len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";9%d", CLocale, color-8);
					}
					else if (ts.ColorFlag & CF_XTERM256) {
						len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";38;5;%d", CLocale, color);
					}
					else if (ts.ColorFlag & CF_PCBOLD16) {
						len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";3%d", CLocale, color-8);
					}
				}
				else if (ts.ColorFlag & CF_XTERM256) {
					len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";38;5;%d", CLocale, color);
				}
			}
			if (CharAttr.Attr2 & Attr2Back && ts.ColorFlag & CF_ANSICOLOR) {
				int color = CharAttr.Back;
				if (color <= 7 && (CharAttr.Attr & AttrBlink) && (ts.ColorFlag & CF_PCBOLD16)) {
					color += 8;
				}
				if (color <= 7) {
					len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";4%d", CLocale, color);
				}
				else if (color <= 15) {
					if (ts.ColorFlag & CF_AIXTERM16) {
						len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";10%d", CLocale, color-8);
					}
					else if (ts.ColorFlag & CF_XTERM256) {
						len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";48;5;%d", CLocale, color);
					}
					else if (ts.ColorFlag & CF_PCBOLD16) {
						len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";4%d", CLocale, color-8);
					}
				}
				else if (ts.ColorFlag & CF_XTERM256) {
					len += _snprintf_s_l(&RepStr[len], sizeof(RepStr) - len, _TRUNCATE, ";48;5;%d", CLocale, color);
				}
			}
			RepStr[len++] = 'm';
			RepStr[len] = 0;
		}
		break;
	  case 'r':	// DECSTBM
		if (StrBuff[1] == 0) {
			len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r%d;%dr", CLocale, CursorTop+1, CursorBottom+1);
		}
		break;
	  case 's':	// DECSLRM
		if (StrBuff[1] == 0) {
			len = _snprintf_s_l(RepStr, sizeof(RepStr), _TRUNCATE, "1$r%d;%ds", CLocale, CursorLeftM+1, CursorRightM+1);
		}
		break;
	}
	if (len == 0) {
		if (strncpy_s(RepStr, sizeof(RepStr), "0$r", _TRUNCATE)) {
			return;
		}
		len = 3;
	}
	if (ts.TermFlag & TF_INVALIDDECRPSS) {
		if (RepStr[0] == '0') {
			RepStr[0] = '1';
		}
		else {
			RepStr[0] = '0';
		}
	}
	SendDCSstr(RepStr, len);
}

int toHexStr(unsigned char *buff, int buffsize, unsigned char *str)
{
	int len, i, copylen = 0;
	unsigned char c;

	len = strlen(str);

	if (buffsize < len*2) {
		return -1;
	}

	for (i=0; i<len; i++) {
		c = str[i] >> 4;
		if (c <= 9) {
			c += '0';
		}
		else {
			c += 'a' - 10;
		}
		buff[copylen++] = c;

		c = str[i] & 0xf;
		if (c <= 9) {
			c += '0';
		}
		else {
			c += 'a' - 10;
		}
		buff[copylen++] = c;
	}

	return copylen;
}

int TermcapString(unsigned char *buff, int buffsize, unsigned char *capname)
{
	int len = 0, l;
	unsigned char *capval = NULL;

	if (strcmp(capname, "Co") == 0 || strcmp(capname, "colors") == 0) {
		if ((ts.ColorFlag & CF_ANSICOLOR) == 0) {
			return 0;
		}

		if (ts.ColorFlag & CF_XTERM256) {
			capval = "256";
		}
		else if (ts.ColorFlag & CF_FULLCOLOR) {
			capval = "16";
		}
		else {
			capval = "8";
		}
	}

	if (capval) {
		if ((len = toHexStr(buff, buffsize, capname)) < 0) {
			return 0;
		}

		if (buffsize <= len) {
			return 0;
		}
		buff[len++] = '=';

		if ((l = toHexStr(&buff[len], buffsize-len, capval)) < 0) {
			return 0;
		}
		len += l;
	}

	return len;
}

void RequestTermcapString(unsigned char *StrBuff, int StrLen)	// xterm experimental
{
	unsigned char RepStr[256];
	unsigned char CapName[16];
	int i, len, replen, caplen = 0;

	RepStr[0] = '1';
	RepStr[1] = '+';
	RepStr[2] = 'r';
	replen = 3;

	for (i=0; i<StrLen; i++) {
		if (StrBuff[i] == ';') {
			if (replen >= sizeof(RepStr)) {
				caplen = 0;
				break;
			}
			if (replen > 3) {
				RepStr[replen++] = ';';
			}
			if (caplen > 0 && caplen < sizeof(CapName)) {
				CapName[caplen] = 0;
				len = TermcapString(&RepStr[replen], sizeof(RepStr)-replen, CapName);
				replen += len;
				caplen = 0;
				if (len == 0) {
					break;
				}
			}
			else {
				caplen = 0;
				break;
			}
		}
		else if (i+1 < StrLen && isxdigit(StrBuff[i]) && isxdigit(StrBuff[i+1])
		  && caplen < sizeof(CapName)-1) {
			if (isdigit(StrBuff[i])) {
				CapName[caplen] = (StrBuff[i] - '0') * 16;
			}
			else {
				CapName[caplen] = ((StrBuff[i] | 0x20) - 'a' + 10) * 16;
			}
			i++;
			if (isdigit(StrBuff[i])) {
				CapName[caplen] += (StrBuff[i] - '0');
			}
			else {
				CapName[caplen] += ((StrBuff[i] | 0x20) - 'a' + 10);
			}
			caplen++;
		}
		else {
			caplen = 0;
			break;
		}
	}

	if (caplen && caplen < sizeof(CapName) && replen < sizeof(RepStr)) {
		if (replen > 3) {
			RepStr[replen++] = ';';
		}
		CapName[caplen] = 0;
		len = TermcapString(&RepStr[replen], sizeof(RepStr)-replen, CapName);
		replen += len;
	}

	if (replen == 3) {
		RepStr[0] = '0';
	}
	SendDCSstr(RepStr, replen);
}

void ParseDCS(BYTE Cmd, unsigned char *StrBuff, int len) {
	switch (ICount) {
	  case 0:
		break;
	  case 1:
		switch (IntChar[1]) {
		  case '!':
			if (Cmd == '{') { // DECSTUI
				if (! (ts.TermFlag & TF_LOCKTUID)) {
					int i;
					for (i=0; i<8 && isxdigit(StrBuff[i]); i++) {
						if (islower(StrBuff[i])) {
							StrBuff[i] = toupper(StrBuff[i]);
						}
					}
					if (len == 8 && i == 8) {
						strncpy_s(ts.TerminalUID, sizeof(ts.TerminalUID), StrBuff, _TRUNCATE);
					}
				}
			}
			break;
		  case '$':
			if (Cmd == 'q')  { // DECRQSS
				RequestStatusString(StrBuff, len);
			}
			break;
		  case '+':
			if (Cmd == 'q') { // Request termcap/terminfo string (xterm)
				RequestTermcapString(StrBuff, len);
			}
			break;
		  default:
			break;
		}
		break;
	  default:
		break;
	}
}

#define ModeDcsFirst     1
#define ModeDcsString    2
void DeviceControl(BYTE b)
{
	static unsigned char StrBuff[256];
	static int DcsParseMode = ModeDcsFirst;
	static int StrLen;
	static int utf8_stat = 0;
	static BYTE Cmd;

	if ((ESCFlag && (b=='\\')) || (b==ST && ts.KanjiCode!=IdSJIS && utf8_stat == 0)) {
		if (DcsParseMode == ModeDcsString) {
			StrBuff[StrLen] = 0;
			ParseDCS(Cmd, StrBuff, StrLen);
		}
		ESCFlag = FALSE;
		ParseMode = ModeFirst;
		DcsParseMode = ModeDcsFirst;
		StrLen = 0;
		utf8_stat = 0;
		return;
	}

	if (b==ESC) {
		ESCFlag = TRUE;
		utf8_stat = 0;
		return;
	}
	else {
		ESCFlag = FALSE;
	}

	utf8_stat = CheckUTF8Seq(b, utf8_stat);

	switch (DcsParseMode) {
	  case ModeDcsFirst:
		if (b<=US) {
			ParseControl(b);
		}
		else if ((b>=0x20) && (b<=0x2F)) {
			if (ICount<IntCharMax) ICount++;
			IntChar[ICount] = b;
		}
		else if ((b>=0x30) && (b<=0x39)) {
			Param[NParam] = Param[NParam]*10 + b - 0x30;
		}
		else if (b==0x3B) {
			if (NParam < NParamMax) {
				NParam++;
				Param[NParam] = 0;
			}
		}
		else if ((b>=0x40) && (b<=0x7E)) {
			if (ICount == 0 && b=='|') {
				ParseMode = ModeDCUserKey;
				if (Param[1] < 1) ClearUserKey();
				WaitKeyId = TRUE;
				NewKeyId = 0;
			}
			else {
				Cmd = b;
				DcsParseMode = ModeDcsString;
			}
		}
		else {
			ParseMode = ModeIgnore;
			utf8_stat = 0;
			IgnoreString(b);
		}
		break;

	  case ModeDcsString:
		if (b <= US && b != HT && b != CR) {
			ESCFlag = FALSE;
			ParseMode = ModeFirst;
			DcsParseMode = ModeDcsFirst;
			StrLen = 0;
		}
		else if (StrLen < sizeof(StrBuff)-1) {
			StrBuff[StrLen++] = b;
		}
		break;
	}
}

void DCUserKey(BYTE b)
{
	static int utf8_stat = 0;

	if (ESCFlag && (b=='\\') || (b==ST && ts.KanjiCode!=IdSJIS && utf8_stat == 0)) {
		if (! WaitKeyId) DefineUserKey(NewKeyId,NewKeyStr,NewKeyLen);
		ESCFlag = FALSE;
		ParseMode = ModeFirst;
		return;
	}

	if (b==ESC) {
		ESCFlag = TRUE;
		return;
	}
	else ESCFlag = FALSE;

	utf8_stat = CheckUTF8Seq(b, utf8_stat);

	if (WaitKeyId) {
		if ((b>=0x30) && (b<=0x39)) {
			if (NewKeyId<1000)
				NewKeyId = NewKeyId*10 + b - 0x30;
		}
		else if (b==0x2F) {
			WaitKeyId = FALSE;
			WaitHi = TRUE;
			NewKeyLen = 0;
		}
	}
	else {
		if (b==0x3B) {
			DefineUserKey(NewKeyId,NewKeyStr,NewKeyLen);
			WaitKeyId = TRUE;
			NewKeyId = 0;
		}
		else {
			if (NewKeyLen < FuncKeyStrMax) {
				if (WaitHi) {
					NewKeyStr[NewKeyLen] = ConvHexChar(b) << 4;
					WaitHi = FALSE;
				}
				else {
					NewKeyStr[NewKeyLen] = NewKeyStr[NewKeyLen] + ConvHexChar(b);
					WaitHi = TRUE;
					NewKeyLen++;
				}
			}
		}
	}
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

unsigned int XtColor2TTColor(int mode, unsigned int xt_color) {
	unsigned int colornum = CS_UNSPEC;

	switch ((mode>=100) ? mode-100 : mode) {
	  case 4:
		switch (xt_color) {
		  case 256:
			colornum = CS_VT_BOLDFG;
			break;
		  case 257:
			// Underline -- not supported.
			// colornum = CS_VT_UNDERFG;
			break;
		  case 258:
			colornum = CS_VT_BLINKFG;
			break;
		  case 259:
			colornum = CS_VT_REVERSEBG;
			break;
		  case CS_UNSPEC:
			if (mode == 104) {
				colornum = CS_ANSICOLOR_ALL;
			}
			break;
		  default:
			if (xt_color <= 255) {
				colornum = xt_color;
			}
		}
		break;
	  case 5:
		switch (xt_color) {
		  case 0:
			colornum = CS_VT_BOLDFG;
			break;
		  case 1:
			// Underline -- not supported.
			// colornum = CS_VT_UNDERFG;
			break;
		  case 2:
			colornum = CS_VT_BLINKFG;
			break;
		  case 3:
			colornum = CS_VT_REVERSEBG;
			break;
		  case CS_UNSPEC:
			if (mode == 105) {
				colornum = CS_SP_ALL;
			}
			break;
		}
		break;
	  case 10:
		colornum = CS_VT_NORMALFG;
		break;
	  case 11:
		colornum = CS_VT_NORMALBG;
		break;
	  case 15:
		colornum = CS_TEK_FG;
		break;
	  case 16:
		colornum = CS_TEK_BG;
		break;
	}
	return colornum;
}

void XsProcColor(int mode, unsigned int ColorNumber, char *ColorSpec, BYTE TermChar) {
	COLORREF color;
	char StrBuff[256];
	unsigned int colornum;
	int len;

	colornum = XtColor2TTColor(mode, ColorNumber);

	if (colornum != CS_UNSPEC) {
		if (strcmp(ColorSpec, "?") == 0) {
			color = DispGetColor(colornum);
			if (mode == 4 || mode == 5) {
				len =_snprintf_s_l(StrBuff, sizeof(StrBuff), _TRUNCATE,
					"%d;%d;rgb:%04x/%04x/%04x", CLocale, mode, ColorNumber,
					GetRValue(color)*257, GetGValue(color)*257, GetBValue(color)*257);
			}
			else {
				len =_snprintf_s_l(StrBuff, sizeof(StrBuff), _TRUNCATE,
					"%d;rgb:%04x/%04x/%04x", CLocale, mode,
					GetRValue(color)*257, GetGValue(color)*257, GetBValue(color)*257);
			}
			SendOSCstr(StrBuff, len, TermChar);
		}
		else if (XsParseColor(ColorSpec, &color)) {
			DispSetColor(colornum, color);
		}
	}
}

void XsProcClipboard(PCHAR buff)
{
	int len, blen;
	char *p, *cbbuff, hdr[20], notify_buff[256], notify_title[MAX_UIMSG];
	HGLOBAL cbmem;
	int wide_len;
	HGLOBAL wide_cbmem;
	LPWSTR wide_buf;

	p = buff;
	while (strchr("cps01234567", *p)) {
		p++;
	}

	if (*p++ == ';') {
		if (*p == '?' && *(p+1) == 0) { // Read access
			if (ts.CtrlFlag & CSF_CBREAD) {
				if (ts.NotifyClipboardAccess) {
					get_lang_msg("MSG_CBACCESS_TITLE", notify_title, sizeof(notify_title),
					             "Clipboard Access", ts.UILanguageFile);
					get_lang_msg("MSG_CBACCESS_READ", notify_buff, sizeof(notify_buff),
					             "Remote host reads clipboard contents.", ts.UILanguageFile);
					NotifyInfoMessage(&cv, notify_buff, notify_title);
				}
				strncpy_s(hdr, sizeof(hdr), "\033]52;", _TRUNCATE);
				if (strncat_s(hdr, sizeof(hdr), buff, p - buff) == 0) {
					CBStartPasteB64(HVTWin, hdr, "\033\\");
				}
			}
			else if (ts.NotifyClipboardAccess) {
				get_lang_msg("MSG_CBACCESS_REJECT_TITLE", notify_title, sizeof(notify_title),
				             "Rejected Clipboard Access", ts.UILanguageFile);
				get_lang_msg("MSG_CBACCESS_READ_REJECT", notify_buff, sizeof(notify_buff),
				             "Reject clipboard read access from remote.", ts.UILanguageFile);
				NotifyWarnMessage(&cv, notify_buff, notify_title);
			}
		}
		else if (ts.CtrlFlag & CSF_CBWRITE) { // Write access
			len = strlen(buff);
			blen = len * 3 / 4 + 1;

			if ((cbmem = GlobalAlloc(GMEM_MOVEABLE, blen)) == NULL) {
				return;
			};
			if ((cbbuff = GlobalLock(cbmem)) == NULL) {
				GlobalFree(cbmem);
				return;
			}

			len = b64decode(cbbuff, blen, p);

			if (len < 0 || len >= blen) {
				GlobalUnlock(cbmem);
				GlobalFree(cbmem);
				return;
			}

			cbbuff[len] = 0;
			GlobalUnlock(cbmem);

			if (ts.NotifyClipboardAccess) {
				get_lang_msg("MSG_CBACCESS_TITLE", notify_title, sizeof(notify_title),
				             "Clipboard Access", ts.UILanguageFile);
				get_lang_msg("MSG_CBACCESS_WRITE", ts.UIMsg, sizeof(ts.UIMsg),
				             "Remote host wirtes clipboard.", ts.UILanguageFile);
				_snprintf_s(notify_buff, sizeof(notify_buff), _TRUNCATE, "%s\n--\n%s", ts.UIMsg, cbbuff);
				NotifyInfoMessage(&cv, notify_buff, notify_title);
			}

			wide_len = MultiByteToWideChar(CP_ACP, 0, cbbuff, -1, NULL, 0);
			wide_cbmem = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * wide_len);
			if (wide_cbmem) {
				wide_buf = (LPWSTR)GlobalLock(wide_cbmem);
				MultiByteToWideChar(CP_ACP, 0, cbbuff, -1, wide_buf, wide_len);
				GlobalUnlock(wide_cbmem);
			}

			if (OpenClipboard(NULL)) {
				EmptyClipboard();
				SetClipboardData(CF_TEXT, cbmem);
				if (wide_buf) {
					SetClipboardData(CF_UNICODETEXT, wide_cbmem);
				}
				CloseClipboard();
			}
		}
		else if (ts.NotifyClipboardAccess) {
			get_lang_msg("MSG_CBACCESS_REJECT_TITLE", notify_title, sizeof(notify_title),
			             "Rejected Clipboard Access", ts.UILanguageFile);
			get_lang_msg("MSG_CBACCESS_WRITE_REJECT", notify_buff, sizeof(notify_buff),
			             "Reject clipboard write access from remote.", ts.UILanguageFile);
			NotifyWarnMessage(&cv, notify_buff, notify_title);
		}
	}
}

void XSequence(BYTE b)
{
	static char *StrBuff = NULL;
	static unsigned int StrLen = 0, StrBuffSize = 0;
	static int utf8_stat = 0;
	static char realloc_failed = FALSE;
	static BOOL ESCflag = FALSE, HasParamStr = FALSE;
	char *p, *color_spec;
	unsigned int new_size;
	int color_num;
	BYTE TermChar;

	TermChar = 0;

	if (ESCflag) {
		ESCflag = FALSE;
		if (b == '\\') {
			TermChar = ST;
		}
		else {	// Invalid Sequence
			ParseMode = ModeIgnore;
			HasParamStr = FALSE;
			IgnoreString(b);
			return;
		}
	}
	else if (b == BEL) {
		TermChar = BEL;
	}
	else if (b==ST && Accept8BitCtrl && !(ts.Language==IdJapanese && ts.KanjiCode==IdSJIS) && utf8_stat==0) {
		TermChar = ST;
	}

	if (TermChar) {
		if (StrBuff) {
			if (StrLen < StrBuffSize) {
				StrBuff[StrLen] = '\0';
			}
			else {
				StrBuff[StrBuffSize-1] = '\0';
			}
		}
		switch (Param[1]) {
		  case 0: /* Change window title and icon name */
		  case 1: /* Change icon name */
		  case 2: /* Change window title */
			if (StrBuff && ts.AcceptTitleChangeRequest) {
				strncpy_s(cv.TitleRemote, sizeof(cv.TitleRemote), StrBuff, _TRUNCATE);
				// (2006.6.15 maya) タイトルに渡す文字列をSJISに変換
				ConvertToCP932(cv.TitleRemote, sizeof(cv.TitleRemote));
				ChangeTitle();
			}
			break;
		  case 4: /* Change/Query color palette */
		  case 5: /* Change/Query special color */
			if (StrBuff) {
				color_num = 0;
				color_spec = NULL;
				for (p = StrBuff; *p; p++) {
					if (color_spec == NULL) {
						if (isdigit(*p)) {
							color_num = color_num * 10 + *p - '0';
						}
						else if (*p == ';') {
							color_spec = p+1;
						}
						else {
							break;
						}
					}
					else {
						if (*p == ';') {
							*p = '\0';
							XsProcColor(Param[1], color_num, color_spec, TermChar);
							color_num = 0;
							color_spec = NULL;
						}
					}
				}
				if (color_spec) {
					XsProcColor(Param[1], color_num, color_spec, TermChar);
				}
			}
			break;
		  case 10: /* Change/Query VT-Window foreground color */
		  case 11: /* Change/Query VT-Window background color */
		  case 12: /* Change/Query VT-Window cursor color */
		  case 13: /* Change/Query mouse cursor foreground color */
		  case 14: /* Change/Query mouse cursor background color */
		  case 15: /* Change/Query Tek-Window foreground color */
		  case 16: /* Change/Query Tek-Window foreground color */
		  case 17: /* Change/Query highlight background color */
		  case 18: /* Change/Query Tek-Window cursor color */
		  case 19: /* Change/Query highlight foreground color */
			if (StrBuff) {
				color_num = Param[1];
				color_spec = StrBuff;
				for (p = StrBuff; *p; p++) {
					if (*p == ';') {
						*p = '\0';
						XsProcColor(color_num, 0, color_spec, TermChar);
						color_num++;
						color_spec = p+1;
					}
				}
				XsProcColor(color_num, 0, color_spec, TermChar);
			}
			break;
		  case 52: /* Manipulate Clipboard data */
			if (StrBuff) {
				XsProcClipboard(StrBuff);
			}
			break;
		  case 104: /* Reset color palette */
		  case 105: /* Reset special color */
			if (HasParamStr) {
				if (StrBuff) {
					color_num = 0;
					for (p = StrBuff; *p; p++) {
						if (isdigit(*p)) {
							color_num = color_num * 10 + *p - '0';
						}
						else if (*p == ';') {
							DispResetColor(XtColor2TTColor(Param[1], color_num));
							color_num = 0;
						}
						else {
							color_num = CS_UNSPEC;
						}
					}
					if (color_num != CS_UNSPEC) {
						DispResetColor(XtColor2TTColor(Param[1], color_num));
					}
				}
			}
			else {
				DispResetColor(XtColor2TTColor(Param[1], CS_UNSPEC));
			}
			break;
		  case 110: /* Reset VT-Window foreground color */
		  case 111: /* Reset VT-Window background color */
		  case 112: /* Reset VT-Window cursor color */
		  case 113: /* Reset mouse cursor foreground color */
		  case 114: /* Reset mouse cursor background color */
		  case 115: /* Reset Tek-Window foreground color */
		  case 116: /* Reset Tek-Window foreground color */
		  case 117: /* Reset highlight background color */
		  case 118: /* Reset Tek-Window cursor color */
		  case 119: /* Reset highlight foreground color */
			DispResetColor(XtColor2TTColor(Param[1], CS_UNSPEC));
			if (HasParamStr && StrBuff) {
				color_num = 0;
				for (p = StrBuff; *p; p++) {
					if (isdigit(*p)) {
						color_num = color_num * 10 + *p - '0';
					}
					else if (*p == ';') {
						DispResetColor(XtColor2TTColor(color_num, CS_UNSPEC));
						color_num = 0;
					}
					else {
						color_num = CS_UNSPEC;
						break;
					}
				}
				if (color_num != CS_UNSPEC) {
					DispResetColor(XtColor2TTColor(color_num, CS_UNSPEC));
				}
			}
			break;
		}
		if (StrBuff) {
			StrBuff[0] = '\0';
			StrLen = 0;
		}
		ParseMode = ModeFirst;
		HasParamStr = FALSE;
		utf8_stat = 0;
	}
	else if (b == ESC) {
		ESCflag = TRUE;
		utf8_stat = 0;
	}
	else if (b <= US) { // Invalid Character
		ParseMode = ModeFirst;
		HasParamStr = FALSE;
		utf8_stat = 0;
	}
	else if (HasParamStr) {
		utf8_stat = CheckUTF8Seq(b, utf8_stat);
		if (StrLen + 1 < StrBuffSize) {
			StrBuff[StrLen++] = b;
		}
		else if (!realloc_failed && StrBuffSize < ts.MaxOSCBufferSize) {
			if (StrBuff == NULL || StrBuffSize == 0) {
				new_size = sizeof(ts.Title);
			}
			else {
				new_size = StrBuffSize * 2;
			}
			if (new_size > ts.MaxOSCBufferSize) {
				new_size = ts.MaxOSCBufferSize;
			}

			p = realloc(StrBuff, new_size);
			if (p == NULL) {
				if (StrBuff==NULL) {
					StrBuffSize = 0;
					ParseMode = ModeIgnore;
					HasParamStr = FALSE;
					IgnoreString(b);
					return;
				}
				realloc_failed = TRUE;
			}
			else {
				StrBuff = p;
				StrBuffSize = new_size;
				if (StrLen + 1 < StrBuffSize) {
					StrBuff[StrLen++] = b;
				}
			}
		}
	}
	else if (isdigit(b)) {
		Param[1] = Param[1] * 10 + b - '0';
	}
	else if (b == ';') {
		HasParamStr = TRUE;
	}
	else {
		ParseMode = ModeIgnore;
		HasParamStr = FALSE;
		IgnoreString(b);
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
	static int state = 0;

	if (ts.FTFlag & FT_ZAUTO) {
		if (state == 0 && b == 'B') {
			state = 1;
		}
		else if (state == 1 && b == '0') {
			state = 2;
		}
		else {
			if (state == 2) {
				if (b =='0') { // ZRQINIT
					/* Auto ZMODEM activation (Receive) */
					ZMODEMStart(IdZAutoR);
				}
				else if (b == '1') { // ZRINIT
					/* Auto ZMODEM activation (Send) */
					ZMODEMStart(IdZAutoS);
				}
			}
			ParseMode = ModeFirst;
			ChangeEmu = -1;
			state = 0;
		}
	}
	else {
		ParseMode = ModeFirst;
		ChangeEmu = -1;
	}
}

BOOL CheckKanji(BYTE b)
{
	BOOL Check;

	if (ts.Language!=IdJapanese)
		return FALSE;

	ConvJIS = FALSE;

	if (ts.KanjiCode==IdSJIS ||
	   (ts.FallbackToCP932 && (ts.KanjiCode==IdUTF8 || ts.KanjiCode==IdUTF8m))) {
		if ((0x80<b) && (b<0xa0) || (0xdf<b) && (b<0xfd)) {
			Fallbacked = TRUE;
			return TRUE; // SJIS kanji
		}
		if ((0xa1<=b) && (b<=0xdf)) {
			return FALSE; // SJIS katakana
		}
	}

	if ((b>=0x21) && (b<=0x7e)) {
		Check = (Gn[Glr[0]]==IdKanji);
		ConvJIS = Check;
	}
	else if ((b>=0xA1) && (b<=0xFE)) {
		Check = (Gn[Glr[1]]==IdKanji);
		if (ts.KanjiCode==IdEUC) {
			Check = TRUE;
		}
		else if (ts.KanjiCode==IdJIS && ((ts.TermFlag & TF_FIXEDJIS)!=0) && (ts.JIS7Katakana==0)) {
			Check = FALSE; // 8-bit katakana
		}
		ConvJIS = Check;
	}
	else {
		Check = FALSE;
	}

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
		switch (ts.KanjiCode) {
		case IdEUC:
			if (ts.ISO2022Flag & ISO2022_SS2) {
				EUCkanaIn = TRUE;
			}
			break;
		case IdUTF8:
		case IdUTF8m:
			PutChar('?');
			break;
		default:
			ParseControl(b);
		}
	}
	else if (b==0x8F) { // SS3
		switch (ts.KanjiCode) {
		case IdEUC:
			if (ts.ISO2022Flag & ISO2022_SS3) {
				EUCcount = 2;
				EUCsupIn = TRUE;
			}
			break;
		case IdUTF8:
		case IdUTF8m:
			PutChar('?');
			break;
		default:
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
	if (ts.Language == IdJapanese) {
		ParseFirstJP(b);
		return;
	}

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
	} else if ((b==0x8E) || (b==0x8F)) {
		PutChar('?');
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
// Unicode Combining Character Support
//
#include "uni_combining.map"

unsigned short GetPrecomposedChar(int start_index, unsigned short first_code, unsigned short code,
								 combining_map_t *table, int tmax)
{
	unsigned short result = 0;
	int i;

	for (i = start_index ; i < tmax ; i++) {
		if (table[i].first_code != first_code) { // 1文字目が異なるなら、以降はもう調べなくてよい。
			break;
		}

		if (table[i].second_code == code) {
			result = table[i].precomposed;
			break;
		}
	}

	return (result);
}

int GetIndexOfCombiningFirstCode(unsigned short code, combining_map_t *table, int tmax)
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
}

// UTF-8で受信データを処理する
BOOL ParseFirstUTF8(BYTE b, int proc_combining)
// returns TRUE if b is processed
//  (actually allways returns TRUE)
{
	static BYTE buf[3];
	static int count = 0;
	static int can_combining = 0;
	static unsigned int first_code;
	static int first_code_index;

	unsigned int code;
	unsigned short cset;
	char *locptr;

	locptr = setlocale(LC_ALL, ts.Locale);

	if (ts.FallbackToCP932 && Fallbacked) {
		return ParseFirstJP(b);
	}

	if ((b & 0x80) != 0x80 || ((b & 0xe0) == 0x80 && count == 0)) {
		// 1バイト目および2バイト目がASCIIの場合は、すべてASCII出力とする。
		// 1バイト目がC1制御文字(0x80-0x9f)の場合も同様。
		if (count == 0 || count == 1) {
			if (proc_combining == 1 && can_combining == 1) {
				UnicodeToCP932(first_code);
				can_combining = 0;
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

	// 2バイトコードの場合
	if ((buf[0] & 0xe0) == 0xc0) {
		if ((buf[1] & 0xc0) == 0x80) {

			if (proc_combining == 1 && can_combining == 1) {
				UnicodeToCP932(first_code);
				can_combining = 0;
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

		if (proc_combining == 1) {
			if (can_combining == 0) {
				if ((first_code_index = GetIndexOfCombiningFirstCode(
						code, mapCombiningToPrecomposed, MAPSIZE(mapCombiningToPrecomposed)
						)) != -1) {
					can_combining = 1;
					first_code = code;
					count = 0;
					return (TRUE);
				}
			} else {
				can_combining = 0;
				cset = GetPrecomposedChar(first_code_index, first_code, code, mapCombiningToPrecomposed, MAPSIZE(mapCombiningToPrecomposed));
				if (cset != 0) { // success
					code = cset;

				} else { // error
					// 2つめの文字が半濁点の1文字目に相当する場合は、再度検索を続ける。(2005.10.15 yutaka)
					if ((first_code_index = GetIndexOfCombiningFirstCode(
							code, mapCombiningToPrecomposed, MAPSIZE(mapCombiningToPrecomposed)
							)) != -1) {

						// 1つめの文字はそのまま出力する
						UnicodeToCP932(first_code);

						can_combining = 1;
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
	if (b>=128) {
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
	UpdateCaretPosition(FALSE);	// 非アクティブの場合のみ再描画する

	ChangeEmu = 0;

	/* Get Device Context */
	DispInitDC();

	LockBuffer();

	while ((c>0) && (ChangeEmu==0)) {
		if (DebugFlag!=DEBUG_FLAG_NONE)
			PutDebugChar(b);
		else {
			switch (ParseMode) {
			case ModeFirst:
				ParseFirst(b);
				break;
			case ModeESC:
				EscapeSequence(b);
				break;
			case ModeDCS:
				DeviceControl(b);
				break;
			case ModeDCUserKey:
				DCUserKey(b);
				break;
			case ModeSOS:
				IgnoreString(b);
				break;
			case ModeCSI:
				ControlSequence(b);
				break;
			case ModeXS:
				XSequence(b);
				break;
			case ModeDLE:
				DLESeen(b);
				break;
			case ModeCAN:
				CANSeen(b);
				break;
			case ModeIgnore:
				IgnoreString(b);
				break;
			default:
				ParseMode = ModeFirst;
				ParseFirst(b);
			}
		}

		PrevCharacter = b;		// memorize previous character for AUTO CR/LF-receive mode

		if (ChangeEmu==0)
			c = CommRead1Byte(&cv,&b);
	}

	BuffUpdateScroll();

	BuffSetCaretWidth();
	UnlockBuffer();

	/* release device context */
	DispReleaseDC();

	CaretOn();

	if (ChangeEmu > 0)
		ParseMode = ModeFirst;

	return ChangeEmu;
}

int MakeLocatorReportStr(char *buff, size_t buffsize, int event, int x, int y) {
	if (x < 0) {
		return _snprintf_s_l(buff, buffsize, _TRUNCATE, "%d;%d&w", CLocale, event, ButtonStat);
	}
	else {
		return _snprintf_s_l(buff, buffsize, _TRUNCATE, "%d;%d;%d;%d;0&w", CLocale, event, ButtonStat, y, x);
	}
}

BOOL DecLocatorReport(int Event, int Button) {
	int x, y, MaxX, MaxY, len = 0;
	char buff[24];

	if (DecLocatorFlag & DecLocatorPixel) {
		x = LastX + 1;
		y = LastY + 1;
		DispConvScreenToWin(NumOfColumns+1, NumOfLines+1, &MaxX, &MaxY);
		if (x < 1 || x > MaxX || y < 1 || y > MaxY) {
			x = -1;
		}
	}
	else {
		DispConvWinToScreen(LastX, LastY, &x, &y, NULL);
		x++; y++;
		if (x < 1 || x > NumOfColumns || y < 1 || y > NumOfLines) {
			x = -1;
		}
	}

	switch (Event) {
	case IdMouseEventCurStat:
		if (MouseReportMode == IdMouseTrackDECELR) {
			len = MakeLocatorReportStr(buff, sizeof(buff), 1, x, y);
		}
		else {
			len = _snprintf_s_l(buff, sizeof(buff), _TRUNCATE, "0&w", CLocale);
		}
		break;

	case IdMouseEventBtnDown:
		if (DecLocatorFlag & DecLocatorButtonDown) {
			len = MakeLocatorReportStr(buff, sizeof(buff), Button*2+2, x, y);
		}
		break;

	case IdMouseEventBtnUp:
		if (DecLocatorFlag & DecLocatorButtonUp) {
			len = MakeLocatorReportStr(buff, sizeof(buff), Button*2+3, x, y);
		}
		break;

	case IdMouseEventMove:
		if (DecLocatorFlag & DecLocatorFiltered) {
			if (y < FilterTop || y > FilterBottom || x < FilterLeft || x > FilterRight) {
				len = MakeLocatorReportStr(buff, sizeof(buff), 10, x, y);
				DecLocatorFlag &= ~DecLocatorFiltered;
			}
		}
		break;
	}

	if (len == 0) {
		return FALSE;
	}

	SendCSIstr(buff, len);

	if (DecLocatorFlag & DecLocatorOneShot) {
		MouseReportMode = IdMouseTrackNone;
	}
	return TRUE;
}

#define MOUSE_POS_LIMIT (255 - 32)
#define MOUSE_POS_EXT_LIMIT (2047 - 32)

int MakeMouseReportStr(char *buff, size_t buffsize, int mb, int x, int y) {
	char tmpx[3], tmpy[3];

	switch (MouseReportExtMode) {
	case IdMouseTrackExtNone:
		if (x >= MOUSE_POS_LIMIT) x = MOUSE_POS_LIMIT;
		if (y >= MOUSE_POS_LIMIT) y = MOUSE_POS_LIMIT;
		return _snprintf_s_l(buff, buffsize, _TRUNCATE, "M%c%c%c", CLocale, mb+32, x+32, y+32);
		break;

	case IdMouseTrackExtUTF8:
		if (x >= MOUSE_POS_EXT_LIMIT) x = MOUSE_POS_EXT_LIMIT;
		if (y >= MOUSE_POS_EXT_LIMIT) y = MOUSE_POS_EXT_LIMIT;
		x += 32;
		y += 32;
		if (x < 128) {
			tmpx[0] = x;
			tmpx[1] = 0;
		}
		else {
			tmpx[0] = (x >> 6) & 0x1f | 0xc0;
			tmpx[1] = x & 0x3f | 0x80;
			tmpx[2] = 0;
		}
		if (y < 128) {
			tmpy[0] = y;
			tmpy[1] = 0;
		}
		else {
			tmpy[0] = (x >> 6) & 0x1f | 0xc0;
			tmpy[1] = y & 0x3f | 0x80;
			tmpy[2] = 0;
		}
		return _snprintf_s_l(buff, buffsize, _TRUNCATE, "M%c%s%s", CLocale, mb+32, tmpx, tmpy);
		break;

	case IdMouseTrackExtSGR:
		return _snprintf_s_l(buff, buffsize, _TRUNCATE, "<%d;%d;%d%c", CLocale, mb&0x7f, x, y, (mb&0x80)?'m':'M');
		break;

	case IdMouseTrackExtURXVT:
		return _snprintf_s_l(buff, buffsize, _TRUNCATE, "%d;%d;%dM", CLocale, mb+32, x, y);
		break;
	}
	buff[0] = 0;
	return 0;
}

BOOL MouseReport(int Event, int Button, int Xpos, int Ypos) {
	char Report[32];
	int x, y, len, modifier;
	static int LastSendX = -1, LastSendY = -1, LastButton = IdButtonRelease;

	len = 0;

	switch (Event) {
	case IdMouseEventBtnDown:
		ButtonStat |= (8>>(Button+1));
		break;
	case IdMouseEventBtnUp:
		ButtonStat &= ~(8>>(Button+1));
		break;
	}
	LastX = Xpos;
	LastY = Ypos;

	if (MouseReportMode == IdMouseTrackNone)
		return FALSE;

	if (ts.DisableMouseTrackingByCtrl && ControlKey())
		return FALSE;

	if (MouseReportMode == IdMouseTrackDECELR)
		return DecLocatorReport(Event, Button);

	DispConvWinToScreen(Xpos, Ypos, &x, &y, NULL);
	x++; y++;

	if (x < 1)
		x = 1;
	else if (x > NumOfColumns)
		x = NumOfColumns;
	if (y < 1)
		y = 1;
	else if (y > NumOfLines)
		y = NumOfLines;

	if (ShiftKey())
		modifier = 4;
	else
		modifier = 0;

	if (ControlKey())
		modifier |= 8;

	if (AltKey())
		modifier |= 16;

	modifier = (ShiftKey()?4:0) | (AltKey()?8:0) | (ControlKey()?16:0);

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
			LastSendX = x;
			LastSendY = y;
			LastButton = Button;
			break;

		case IdMouseTrackNetTerm:
			len = _snprintf_s_l(Report, sizeof Report, _TRUNCATE, "\033}%d,%d\r", CLocale, y, x);
			CommBinaryOut(&cv, Report, len);
			return TRUE;

		case IdMouseTrackVT200Hl: /* not supported yet */
		default:
			return FALSE;
		}
		break;

	case IdMouseEventBtnUp:
		switch (MouseReportMode) {
		case IdMouseTrackVT200:
		case IdMouseTrackBtnEvent:
		case IdMouseTrackAllEvent:
			if (MouseReportExtMode == IdMouseTrackExtSGR) {
				modifier |= 128;
			}
			else {
				Button = IdButtonRelease;
			}
			len = MakeMouseReportStr(Report, sizeof Report, Button | modifier, x, y);
			LastSendX = x;
			LastSendY = y;
			LastButton = IdButtonRelease;
			break;

		case IdMouseTrackX10:     /* nothing to do */
		case IdMouseTrackNetTerm: /* nothing to do */
			return TRUE;

		case IdMouseTrackVT200Hl: /* not supported yet */
		default:
			return FALSE;
		}
		break;

	case IdMouseEventMove:
		switch (MouseReportMode) {
		case IdMouseTrackBtnEvent:
			if (LastButton == 3) {
				return FALSE;
			}
			/* FALLTHROUGH */
		case IdMouseTrackAllEvent:
			if (x == LastSendX && y == LastSendY) {
				return FALSE;
			}
			len = MakeMouseReportStr(Report, sizeof Report, LastButton | modifier | 32, x, y);
			LastSendX = x;
			LastSendY = y;
		break;

		case IdMouseTrackVT200Hl: /* not supported yet */
		case IdMouseTrackX10:     /* nothing to do */
		case IdMouseTrackVT200:   /* nothing to do */
		case IdMouseTrackNetTerm: /* nothing to do */
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

		case IdMouseTrackX10:     /* nothing to do */
		case IdMouseTrackVT200Hl: /* not supported yet */
		case IdMouseTrackNetTerm: /* nothing to do */
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
		SendCSIstr("I", 0);
	} else {
		// Focus Out
		SendCSIstr("O", 0);
	}
}

void VisualBell() {
	CSQExchangeColor();
	Sleep(10);
	CSQExchangeColor();
}

void RingBell(int type) {
	DWORD now;

	now = GetTickCount();
	if (now - BeepSuppressTime < ts.BeepSuppressTime * 1000) {
		BeepSuppressTime = now;
	}
	else {
		if (now - BeepStartTime < ts.BeepOverUsedTime * 1000) {
			if (BeepOverUsedCount <= 1) {
				BeepSuppressTime = now;
			}
			else {
				BeepOverUsedCount--;
			}
		}
		else {
			BeepStartTime = now;
			BeepOverUsedCount = ts.BeepOverUsedCount;
		}

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
	}
}

void EndTerm() {
	if (CLocale) {
		_free_locale(CLocale);
	}
	CLocale = NULL;
}

BOOL BracketedPasteMode() {
	return BracketedPaste;
}

BOOL WheelToCursorMode() {
	return AcceptWheelToCursor && AppliCursorMode && !ts.DisableAppCursor && !(ControlKey() && ts.DisableWheelToCursorByCtrl);
}

void ChangeTerminalID() {
	switch (ts.TerminalID) {
	case IdVT220J:
	case IdVT282:
		VTlevel = 2;
		break;
	case IdVT320:
	case IdVT382:
		VTlevel = 3;
		break;
	case IdVT420:
		VTlevel = 4;
		break;
	case IdVT520:
	case IdVT525:
		VTlevel = 5;
		break;
	default:
		VTlevel = 1;
	}

	if (VTlevel == 1) {
		Send8BitMode = FALSE;
	}
	else {
		Send8BitMode = ts.Send8BitCtrl;
	}
}

void TermPasteString(char *str, int len)
{
	TermSendStartBracket();
	CommTextOut(&cv, str, len);
	if (ts.LocalEcho) {
		CommTextEcho(&cv, str, len);
	}
	TermSendEndBracket();

	return;
}

void TermSendStartBracket()
{
	if (BracketedPaste) {
		CommBinaryOut(&cv, BracketStart, BracketStartLen);
	}

	return;
}

void TermSendEndBracket()
{
	if (BracketedPaste) {
		CommBinaryOut(&cv, BracketEnd, BracketEndLen);
	}

	return;
}
