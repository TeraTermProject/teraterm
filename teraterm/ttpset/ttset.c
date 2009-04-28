/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */
/* IPv6 modification is Copyright(C) 2000 Jun-ya kato <kato@win6.jp> */

/* TTSET.DLL, setup file routines*/
#ifndef NO_INET6
#include <winsock2.h>
#include <ws2tcpip.h>
#endif							/* NO_INET6 */
#include "teraterm.h"
#include "tttypes.h"
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include "ttlib.h"
#include "tt_res.h"

#include "compat_w95.h"

#define Section "Tera Term"

static PCHAR far TermList[] =
	{ "VT100", "VT100J", "VT101", "VT102", "VT102J", "VT220J", "VT282",
	"VT320", "VT382", NULL };

static PCHAR far RussList[] =
	{ "Windows", "KOI8-R", "CP-866", "ISO-8859-5", NULL };
static PCHAR far RussList2[] = { "Windows", "KOI8-R", NULL };

WORD str2id(PCHAR far * List, PCHAR str, WORD DefId)
{
	WORD i;
	i = 0;
	while ((List[i] != NULL) && (_stricmp(List[i], str) != 0))
		i++;
	if (List[i] == NULL)
		i = DefId;
	else
		i++;

	return i;
}

void id2str(PCHAR far * List, WORD Id, WORD DefId, PCHAR str, int destlen)
{
	int i;

	if (Id == 0)
		i = DefId - 1;
	else {
		i = 0;
		while ((List[i] != NULL) && (i < Id - 1))
			i++;
		if (List[i] == NULL)
			i = DefId - 1;
	}
	strncpy_s(str, destlen, List[i], _TRUNCATE);
}

int IconName2IconId(const char *name) {
	int id;

	if (_stricmp(name, "tterm") == 0) {
		id = IDI_TTERM;
	}
	else if (_stricmp(name, "vt") == 0) {
		id = IDI_VT;
	}
	else if (_stricmp(name, "tek") == 0) {
		id = IDI_TEK;
	}
	else if (_stricmp(name, "tterm_classic") == 0) {
		id = IDI_TTERM_CLASSIC;
	}
	else if (_stricmp(name, "vt_classic") == 0) {
		id = IDI_VT_CLASSIC;
	}
	else if (_stricmp(name, "cygterm") == 0) {
		id = IDI_CYGTERM;
	}
	else {
		id = IdIconDefault;
	}
	return id;
}

void IconId2IconName(char *name, int len, int id) {
	char *icon;
	switch (id) {
		case IDI_TTERM:
			icon = "tterm";
			break;
		case IDI_VT:
			icon = "vt";
			break;
		case IDI_TEK:
			icon = "tek";
			break;
		case IDI_TTERM_CLASSIC:
			icon = "tterm_classic";
			break;
		case IDI_VT_CLASSIC:
			icon = "vt_classic";
			break;
		case IDI_CYGTERM:
			icon = "cygterm";
			break;
		default:
			icon = "Default";
	}
	strncpy_s(name, len, icon, _TRUNCATE);
}

WORD GetOnOff(PCHAR Sect, PCHAR Key, PCHAR FName, BOOL Default)
{
	char Temp[4];
	GetPrivateProfileString(Sect, Key, "", Temp, sizeof(Temp), FName);
	if (Default) {
		if (_stricmp(Temp, "off") == 0)
			return 0;
		else
			return 1;
	}
	else {
		if (_stricmp(Temp, "on") == 0)
			return 1;
		else
			return 0;
	}
}

void WriteOnOff(PCHAR Sect, PCHAR Key, PCHAR FName, WORD Flag)
{
	char Temp[4];

	if (Flag != 0)
		strncpy_s(Temp, sizeof(Temp), "on", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "off", _TRUNCATE);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void WriteInt(PCHAR Sect, PCHAR Key, PCHAR FName, int i)
{
	char Temp[15];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", i);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void WriteUint(PCHAR Sect, PCHAR Key, PCHAR FName, UINT i)
{
	char Temp[15];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%u", i);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void WriteInt2(PCHAR Sect, PCHAR Key, PCHAR FName, int i1, int i2)
{
	char Temp[32];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d", i1, i2);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void WriteInt4(PCHAR Sect, PCHAR Key, PCHAR FName,
			   int i1, int i2, int i3, int i4)
{
	char Temp[64];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d",
	            i1, i2, i3, i4);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void WriteInt6(PCHAR Sect, PCHAR Key, PCHAR FName,
			   int i1, int i2, int i3, int i4, int i5, int i6)
{
	char Temp[96];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d,%d,%d",
	            i1, i2,i3, i4, i5, i6);
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void WriteFont(PCHAR Sect, PCHAR Key, PCHAR FName,
			   PCHAR Name, int x, int y, int charset)
{
	char Temp[80];
	if (Name[0] != 0)
		_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s,%d,%d,%d",
		            Name, x, y, charset);
	else
		Temp[0] = 0;
	WritePrivateProfileString(Sect, Key, Temp, FName);
}

void FAR PASCAL ReadIniFile(PCHAR FName, PTTSet ts)
{
	int i;
	HDC TmpDC;
	char Temp[MAXPATHLEN];

	ts->Minimize = 0;
	ts->HideWindow = 0;
	ts->LogFlag = 0;			// Log flags
	ts->FTFlag = 0;				// File transfer flags
	ts->MenuFlag = 0;			// Menu flags
	ts->TermFlag = 0;			// Terminal flag
	ts->ColorFlag = 0;			// ANSI/Attribute color flags
	ts->PortFlag = 0;			// Port flags
	ts->TelPort = 23;

	/* Version number */
/*  GetPrivateProfileString(Section,"Version","",
			  Temp,sizeof(Temp),FName); */

	/* Language */
	GetPrivateProfileString(Section, "Language", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "Japanese") == 0)
		ts->Language = IdJapanese;
	else if (_stricmp(Temp, "Russian") == 0)
		ts->Language = IdRussian;
	else if (_stricmp(Temp, "English") == 0)
		ts->Language = IdEnglish;
	else if (_stricmp(Temp,"Korean") == 0) // HKS
		ts->Language = IdKorean;
	else {
		switch (PRIMARYLANGID(GetSystemDefaultLangID())) {
		case LANG_JAPANESE:
			ts->Language = IdJapanese;
			break;
		case LANG_RUSSIAN:
			ts->Language = IdRussian;
			break;
		case LANG_KOREAN:	// HKS
			ts->Language = IdKorean; 
			break; 
		default:
			ts->Language = IdEnglish;
		}
	}

	/* Port type */
	GetPrivateProfileString(Section, "Port", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "tcpip") == 0)
		ts->PortType = IdTCPIP;
	else if (_stricmp(Temp, "serial") == 0)
		ts->PortType = IdSerial;
	else {
		ts->PortType = IdTCPIP;
	}

	/* VT win position */
	GetPrivateProfileString(Section, "VTPos", "-2147483648,-2147483648", Temp, sizeof(Temp), FName);	/* default: random position */
	GetNthNum(Temp, 1, (int far *) (&ts->VTPos.x));
	GetNthNum(Temp, 2, (int far *) (&ts->VTPos.y));

	/* TEK win position */
	GetPrivateProfileString(Section, "TEKPos", "-2147483648,-2147483648", Temp, sizeof(Temp), FName);	/* default: random position */
	GetNthNum(Temp, 1, (int far *) &(ts->TEKPos.x));
	GetNthNum(Temp, 2, (int far *) &(ts->TEKPos.y));

	/* Save VT Window position */
	ts->SaveVTWinPos = GetOnOff(Section, "SaveVTWinPos", FName, FALSE);

	/* VT terminal size  */
	GetPrivateProfileString(Section, "TerminalSize", "80,24",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, &ts->TerminalWidth);
	GetNthNum(Temp, 2, &ts->TerminalHeight);
	if (ts->TerminalWidth < 0)
		ts->TerminalWidth = 1;
	if (ts->TerminalHeight < 0)
		ts->TerminalHeight = 1;

	/* Terminal size = Window size */
	ts->TermIsWin = GetOnOff(Section, "TermIsWin", FName, FALSE);

	/* Auto window resize flag */
	ts->AutoWinResize = GetOnOff(Section, "AutoWinResize", FName, FALSE);

	/* CR Receive */
	GetPrivateProfileString(Section, "CRReceive", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "CRLF") == 0) {
		ts->CRReceive = IdCRLF;
	}
	else if (_stricmp(Temp, "LF") == 0) {
		ts->CRReceive = IdLF;
	}
	else {
		ts->CRReceive = IdCR;
	}
	/* CR Send */
	GetPrivateProfileString(Section, "CRSend", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "CRLF") == 0)
		ts->CRSend = IdCRLF;
	else
		ts->CRSend = IdCR;
	ts->CRSend_ini = ts->CRSend;

	/* Local echo */
	ts->LocalEcho = GetOnOff(Section, "LocalEcho", FName, FALSE);
	ts->LocalEcho_ini = ts->LocalEcho;

	/* Answerback */
	GetPrivateProfileString(Section, "Answerback", "", Temp,
	                        sizeof(Temp), FName);
	ts->AnswerbackLen =
		Hex2Str(Temp, ts->Answerback, sizeof(ts->Answerback));

	/* Kanji Code (receive) */
	GetPrivateProfileString(Section, "KanjiReceive", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "EUC") == 0)
		ts->KanjiCode = IdEUC;
	else if (_stricmp(Temp, "JIS") == 0)
		ts->KanjiCode = IdJIS;
	else if (_stricmp(Temp, "UTF-8") == 0)
		ts->KanjiCode = IdUTF8;
	else if (_stricmp(Temp, "UTF-8m") == 0)
		ts->KanjiCode = IdUTF8m;
	else
		ts->KanjiCode = IdSJIS;

	/* Katakana (receive) */
	GetPrivateProfileString(Section, "KatakanaReceive", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "7") == 0)
		ts->JIS7Katakana = 1;
	else
		ts->JIS7Katakana = 0;

	/* Kanji Code (transmit) */
	GetPrivateProfileString(Section, "KanjiSend", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "EUC") == 0)
		ts->KanjiCodeSend = IdEUC;
	else if (_stricmp(Temp, "JIS") == 0)
		ts->KanjiCodeSend = IdJIS;
	else if (_stricmp(Temp, "UTF-8") == 0)
		ts->KanjiCodeSend = IdUTF8;
	else
		ts->KanjiCodeSend = IdSJIS;

	/* Katakana (receive) */
	GetPrivateProfileString(Section, "KatakanaSend", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "7") == 0)
		ts->JIS7KatakanaSend = 1;
	else
		ts->JIS7KatakanaSend = 0;

	/* KanjiIn */
	GetPrivateProfileString(Section, "KanjiIn", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "@") == 0)
		ts->KanjiIn = IdKanjiInA;
	else
		ts->KanjiIn = IdKanjiInB;

	/* KanjiOut */
	GetPrivateProfileString(Section, "KanjiOut", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "B") == 0)
		ts->KanjiOut = IdKanjiOutB;
	else if (_stricmp(Temp, "H") == 0)
		ts->KanjiOut = IdKanjiOutH;
	else
		ts->KanjiOut = IdKanjiOutJ;

	/* Auto Win Switch VT<->TEK */
	ts->AutoWinSwitch = GetOnOff(Section, "AutoWinSwitch", FName, FALSE);

	/* Terminal ID */
	GetPrivateProfileString(Section, "TerminalID", "",
	                        Temp, sizeof(Temp), FName);
	ts->TerminalID = str2id(TermList, Temp, IdVT100);

	/* Russian character set (host) */
	GetPrivateProfileString(Section, "RussHost", "",
	                        Temp, sizeof(Temp), FName);
	ts->RussHost = str2id(RussList, Temp, IdKOI8);

	/* Russian character set (client) */
	GetPrivateProfileString(Section, "RussClient", "",
	                        Temp, sizeof(Temp), FName);
	ts->RussClient = str2id(RussList, Temp, IdWindows);

	/* Title String */
	GetPrivateProfileString(Section, "Title", "Tera Term",
	                        ts->Title, sizeof(ts->Title), FName);

	/* Cursor shape */
	GetPrivateProfileString(Section, "CursorShape", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "vertical") == 0)
		ts->CursorShape = IdVCur;
	else if (_stricmp(Temp, "horizontal") == 0)
		ts->CursorShape = IdHCur;
	else
		ts->CursorShape = IdBlkCur;

	/* Hide title */
	ts->HideTitle = GetOnOff(Section, "HideTitle", FName, FALSE);

	/* Popup menu */
	ts->PopupMenu = GetOnOff(Section, "PopupMenu", FName, FALSE);

	/* PC-Style bold color mapping */
	if (GetOnOff(Section, "PcBoldColor", FName, FALSE))
		ts->ColorFlag |= CF_PCBOLD16;

	/* aixterm style 16 colors mode */
	if (GetOnOff(Section, "Aixterm16Color", FName, FALSE))
		ts->ColorFlag |= CF_AIXTERM16;

	/* xterm style 256 colors mode */
	if (GetOnOff(Section, "Xterm256Color", FName, TRUE))
		ts->ColorFlag |= CF_XTERM256;

	/* Enable scroll buffer */
	ts->EnableScrollBuff =
		GetOnOff(Section, "EnableScrollBuff", FName, TRUE);

	/* Scroll buffer size */
	ts->ScrollBuffSize =
		GetPrivateProfileInt(Section, "ScrollBuffSize", 100, FName);

	/* VT Color */
	GetPrivateProfileString(Section, "VTColor", "0,0,0,255,255,255",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 5; i++)
		GetNthNum(Temp, i + 1, (int far *) &(ts->TmpColor[0][i]));
	for (i = 0; i <= 1; i++)
		ts->VTColor[i] = RGB((BYTE) ts->TmpColor[0][i * 3],
		                     (BYTE) ts->TmpColor[0][i * 3 + 1],
		                     (BYTE) ts->TmpColor[0][i * 3 + 2]);

	/* VT Bold Color */
	GetPrivateProfileString(Section, "VTBoldColor", "0,0,255,255,255,255",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 5; i++)
		GetNthNum(Temp, i + 1, (int far *) &(ts->TmpColor[0][i]));
	for (i = 0; i <= 1; i++)
		ts->VTBoldColor[i] = RGB((BYTE) ts->TmpColor[0][i * 3],
		                         (BYTE) ts->TmpColor[0][i * 3 + 1],
		                         (BYTE) ts->TmpColor[0][i * 3 + 2]);
	if (GetOnOff(Section, "EnableBoldAttrColor", FName, TRUE))
		ts->ColorFlag |= CF_BOLDCOLOR;

	/* VT Blink Color */
	GetPrivateProfileString(Section, "VTBlinkColor", "255,0,0,255,255,255",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 5; i++)
		GetNthNum(Temp, i + 1, (int far *) &(ts->TmpColor[0][i]));
	for (i = 0; i <= 1; i++)
		ts->VTBlinkColor[i] = RGB((BYTE) ts->TmpColor[0][i * 3],
		                          (BYTE) ts->TmpColor[0][i * 3 + 1],
		                          (BYTE) ts->TmpColor[0][i * 3 + 2]);
	if (GetOnOff(Section, "EnableBlinkAttrColor", FName, TRUE))
		ts->ColorFlag |= CF_BLINKCOLOR;

	/* VT Reverse Color */
	GetPrivateProfileString(Section, "VTReverseColor", "255,255,255,0,0,0",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 5; i++)
		GetNthNum(Temp, i + 1, (int far *) &(ts->TmpColor[0][i]));
	for (i = 0; i <= 1; i++)
		ts->VTReverseColor[i] = RGB((BYTE) ts->TmpColor[0][i * 3],
		                          (BYTE) ts->TmpColor[0][i * 3 + 1],
		                          (BYTE) ts->TmpColor[0][i * 3 + 2]);
	if (GetOnOff(Section, "EnableReverseAttrColor", FName, FALSE))
		ts->ColorFlag |= CF_REVERSECOLOR;

	ts->EnableClickableUrl =
		GetOnOff(Section, "EnableClickableUrl", FName, FALSE);

	/* URL Color */
	GetPrivateProfileString(Section, "URLColor", "0,255,0,255,255,255",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 5; i++)
		GetNthNum(Temp, i + 1, (int far *) &(ts->TmpColor[0][i]));
	for (i = 0; i <= 1; i++)
		ts->URLColor[i] = RGB((BYTE) ts->TmpColor[0][i * 3],
		                      (BYTE) ts->TmpColor[0][i * 3 + 1],
		                      (BYTE) ts->TmpColor[0][i * 3 + 2]);
	if (GetOnOff(Section, "EnableURLColor", FName, TRUE))
		ts->ColorFlag |= CF_URLCOLOR;

	/* TEK Color */
	GetPrivateProfileString(Section, "TEKColor", "0,0,0,255,255,255",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 5; i++)
		GetNthNum(Temp, i + 1, (int far *) &(ts->TmpColor[0][i]));
	for (i = 0; i <= 1; i++)
		ts->TEKColor[i] = RGB((BYTE) ts->TmpColor[0][i * 3],
		                      (BYTE) ts->TmpColor[0][i * 3 + 1],
		                      (BYTE) ts->TmpColor[0][i * 3 + 2]);

#ifndef NO_ANSI_COLOR_EXTENSION
	/* ANSI color definition (in the case FullColor=on)  -- special option
	   o UseTextColor should be off, or the background and foreground color of
	   VTColor are assigned to color-number 0 and 7 respectively, even if
	   they are specified in ANSIColor.
	   o ANSIColor is a set of 4 values that are color-number(0--15),
	   red-value(0--255), green-value(0--255) and blue-value(0--255). */
	GetPrivateProfileString(Section, "ANSIColor",
	                        " 0,  0,  0,  0,"
	                        " 1,255,  0,  0,"
	                        " 2,  0,255,  0,"
	                        " 3,255,255,  0,"
	                        " 4,  0,  0,255,"
	                        " 5,255,  0,255,"
	                        " 6,  0,255,255,"
	                        " 7,255,255,255,"
	                        " 8,128,128,128,"
	                        " 9,128,  0,  0,"
	                        "10,  0,128,  0,"
	                        "11,128,128,  0,"
	                        "12,  0,  0,128,"
	                        "13,128,  0,128,"
	                        "14,  0,128,128,"
	                        "15,192,192,192", Temp, sizeof(Temp), FName);
	{
		char *t;
		int n = 1;
		for (t = Temp; *t; t++)
			if (*t == ',')
				n++;
		n /= 4;
		for (i = 0; i <= n; i++) {
			int colorid, r, g, b;
			GetNthNum(Temp, i * 4 + 1, (int far *) &colorid);
			GetNthNum(Temp, i * 4 + 2, (int far *) &r);
			GetNthNum(Temp, i * 4 + 3, (int far *) &g);
			GetNthNum(Temp, i * 4 + 4, (int far *) &b);
			ts->ANSIColor[colorid & 15] =
				RGB((BYTE) r, (BYTE) g, (BYTE) b);
		}
	}
#endif						/* NO_ANSI_COLOR_EXTENSION */

	TmpDC = GetDC(0);			/* Get screen device context */
	for (i = 0; i <= 1; i++)
		ts->VTColor[i] = GetNearestColor(TmpDC, ts->VTColor[i]);
	for (i = 0; i <= 1; i++)
		ts->VTBoldColor[i] = GetNearestColor(TmpDC, ts->VTBoldColor[i]);
	for (i = 0; i <= 1; i++)
		ts->VTBlinkColor[i] = GetNearestColor(TmpDC, ts->VTBlinkColor[i]);
	for (i = 0; i <= 1; i++)
		ts->TEKColor[i] = GetNearestColor(TmpDC, ts->TEKColor[i]);
	/* begin - ishizaki */
	for (i = 0; i <= 1; i++)
		ts->URLColor[i] = GetNearestColor(TmpDC, ts->URLColor[i]);
	/* end - ishizaki */
#ifndef NO_ANSI_COLOR_EXTENSION
	for (i = 0; i < 16; i++)
		ts->ANSIColor[i] = GetNearestColor(TmpDC, ts->ANSIColor[i]);
#endif						/* NO_ANSI_COLOR_EXTENSION */
	ReleaseDC(0, TmpDC);
	if (GetOnOff(Section, "EnableANSIColor", FName, TRUE))
		ts->ColorFlag |= CF_ANSICOLOR;

	/* TEK color emulation */
	ts->TEKColorEmu = GetOnOff(Section, "TEKColorEmulation", FName, FALSE);

	/* VT Font */
	GetPrivateProfileString(Section, "VTFont", "Terminal,0,-13,1",
	                        Temp, sizeof(Temp), FName);
	GetNthString(Temp, 1, sizeof(ts->VTFont), ts->VTFont);
	GetNthNum(Temp, 2, (int far *) &(ts->VTFontSize.x));
	GetNthNum(Temp, 3, (int far *) &(ts->VTFontSize.y));
	GetNthNum(Temp, 4, &(ts->VTFontCharSet));

	/* Bold font flag */
	ts->EnableBold = GetOnOff(Section, "EnableBold", FName, TRUE);

	/* Russian character set (font) */
	GetPrivateProfileString(Section, "RussFont", "",
	                        Temp, sizeof(Temp), FName);
	ts->RussFont = str2id(RussList, Temp, IdWindows);

	/* TEK Font */
	GetPrivateProfileString(Section, "TEKFont", "Courier,0,-13,0",
	                        Temp, sizeof(Temp), FName);
	GetNthString(Temp, 1, sizeof(ts->TEKFont), ts->TEKFont);
	GetNthNum(Temp, 2, (int far *) &(ts->TEKFontSize.x));
	GetNthNum(Temp, 3, (int far *) &(ts->TEKFontSize.y));
	GetNthNum(Temp, 4, &(ts->TEKFontCharSet));

	/* BS key */
	GetPrivateProfileString(Section, "BSKey", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "DEL") == 0)
		ts->BSKey = IdDEL;
	else
		ts->BSKey = IdBS;
	/* Delete key */
	ts->DelKey = GetOnOff(Section, "DeleteKey", FName, FALSE);

	/* Meta Key */
	ts->MetaKey = GetOnOff(Section, "MetaKey", FName, FALSE);

	/* Application Keypad */
	ts->DisableAppKeypad =
		GetOnOff(Section, "DisableAppKeypad", FName, FALSE);

	/* Application Cursor */
	ts->DisableAppCursor =
		GetOnOff(Section, "DisableAppCursor", FName, FALSE);

	/* Russian keyboard type */
	GetPrivateProfileString(Section, "RussKeyb", "",
	                        Temp, sizeof(Temp), FName);
	ts->RussKeyb = str2id(RussList2, Temp, IdWindows);

	/* Serial port ID */
	ts->ComPort = GetPrivateProfileInt(Section, "ComPort", 1, FName);

	/* Baud rate */
	GetPrivateProfileString(Section, "BaudRate", "9600",
	                        Temp, sizeof(Temp), FName);
	ts->Baud = str2id(BaudList, Temp, IdBaud9600);

	/* Parity */
	GetPrivateProfileString(Section, "Parity", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "even") == 0)
		ts->Parity = IdParityEven;
	else if (_stricmp(Temp, "odd") == 0)
		ts->Parity = IdParityOdd;
	else
		ts->Parity = IdParityNone;

	/* Data bit */
	GetPrivateProfileString(Section, "DataBit", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "7") == 0)
		ts->DataBit = IdDataBit7;
	else
		ts->DataBit = IdDataBit8;

	/* Stop bit */
	GetPrivateProfileString(Section, "StopBit", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "2") == 0)
		ts->StopBit = IdStopBit2;
	else
		ts->StopBit = IdStopBit1;

	/* Flow control */
	GetPrivateProfileString(Section, "FlowCtrl", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "x") == 0)
		ts->Flow = IdFlowX;
	else if (_stricmp(Temp, "hard") == 0)
		ts->Flow = IdFlowHard;
	else
		ts->Flow = IdFlowNone;

	/* Delay per character */
	ts->DelayPerChar =
		GetPrivateProfileInt(Section, "DelayPerChar", 0, FName);

	/* Delay per line */
	ts->DelayPerLine =
		GetPrivateProfileInt(Section, "DelayPerLine", 0, FName);

	/* Telnet flag */
	ts->Telnet = GetOnOff(Section, "Telnet", FName, TRUE);

	/* Telnet terminal type */
	GetPrivateProfileString(Section, "TermType", "xterm", ts->TermType,
	                        sizeof(ts->TermType), FName);

	/* TCP port num */
	ts->TCPPort =
		GetPrivateProfileInt(Section, "TCPPort", ts->TelPort, FName);

	/* Auto window close flag */
	ts->AutoWinClose = GetOnOff(Section, "AutoWinClose", FName, TRUE);

	/* History list */
	ts->HistoryList = GetOnOff(Section, "HistoryList", FName, FALSE);

	/* File transfer binary flag */
	ts->TransBin = GetOnOff(Section, "TransBin", FName, FALSE);

	/* Log append */
	ts->Append = GetOnOff(Section, "LogAppend", FName, FALSE);

	/* Log plain text (2005.5.7 yutaka) */
	ts->LogTypePlainText =
		GetOnOff(Section, "LogTypePlainText", FName, FALSE);

	/* Log with timestamp (2006.7.23 maya) */
	ts->LogTimestamp = GetOnOff(Section, "LogTimestamp", FName, FALSE);

	/* Log without transfer dialog */
	ts->LogHideDialog = GetOnOff(Section, "LogHideDialog", FName, FALSE);

	/* Default Log file name (2006.8.28 maya) */
	GetPrivateProfileString(Section, "LogDefaultName", "teraterm.log",
	                        ts->LogDefaultName, sizeof(ts->LogDefaultName),
	                        FName);

	/* Default Log file path (2007.5.30 maya) */
	GetPrivateProfileString(Section, "LogDefaultPath", "",
	                        ts->LogDefaultPath, sizeof(ts->LogDefaultPath),
	                        FName);

	/* Auto start logging (2007.5.31 maya) */
	ts->LogAutoStart = GetOnOff(Section, "LogAutoStart", FName, FALSE);

	/* XMODEM option */
	GetPrivateProfileString(Section, "XmodemOpt", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "crc") == 0)
		ts->XmodemOpt = XoptCRC;
	else if (_stricmp(Temp, "1k") == 0)
		ts->XmodemOpt = Xopt1K;
	else
		ts->XmodemOpt = XoptCheck;

	/* XMODEM binary file */
	ts->XmodemBin = GetOnOff(Section, "XmodemBin", FName, TRUE);

	/* XMODEM 受信コマンド (2007.12.21 yutaka) */
	GetPrivateProfileString(Section, "XModemRcvCommand", "",
	                        ts->XModemRcvCommand,
	                        sizeof(ts->XModemRcvCommand), FName);

	/* Default directory for file transfer */
	GetPrivateProfileString(Section, "FileDir", "",
	                        ts->FileDir, sizeof(ts->FileDir), FName);
	if (strlen(ts->FileDir) == 0)
		strncpy_s(ts->FileDir, sizeof(ts->FileDir), ts->HomeDir, _TRUNCATE);
	else {
		_getcwd(Temp, sizeof(Temp));
		if (_chdir(ts->FileDir) != 0)
			strncpy_s(ts->FileDir, sizeof(ts->FileDir), ts->HomeDir, _TRUNCATE);
		_chdir(Temp);
	}

	/* filter on file send (2007.6.5 maya) */
	GetPrivateProfileString(Section, "FileSendFilter", "",
	                        ts->FileSendFilter, sizeof(ts->FileSendFilter),
	                        FName);

/*--------------------------------------------------*/
	/* 8 bit control code flag  -- special option */
	if (GetOnOff(Section, "Accept8BitCtrl", FName, TRUE))
		ts->TermFlag |= TF_ACCEPT8BITCTRL;

	/* Wrong sequence flag  -- special option */
	if (GetOnOff(Section, "AllowWrongSequence", FName, FALSE))
		ts->TermFlag |= TF_ALLOWWRONGSEQUENCE;

	if (((ts->TermFlag & TF_ALLOWWRONGSEQUENCE) == 0) &&
	    (ts->KanjiOut == IdKanjiOutH))
		ts->KanjiOut = IdKanjiOutJ;

	// Auto file renaming --- special option
	if (GetOnOff(Section, "AutoFileRename", FName, FALSE))
		ts->FTFlag |= FT_RENAME;

	// Auto invoking (character set->G0->GL) --- special option
	if (GetOnOff(Section, "AutoInvoke", FName, FALSE))
		ts->TermFlag |= TF_AUTOINVOKE;

	// Auto text copy --- special option
	ts->AutoTextCopy = GetOnOff(Section, "AutoTextCopy", FName, TRUE);

	/* Back wrap -- special option */
	if (GetOnOff(Section, "BackWrap", FName, FALSE))
		ts->TermFlag |= TF_BACKWRAP;

	/* Beep type -- special option */
	GetPrivateProfileString(Section, "Beep", "", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "off") == 0)
		ts->Beep = IdBeepOff;
	else if (_stricmp(Temp, "visual") == 0)
		ts->Beep = IdBeepVisual;
	else
		ts->Beep = IdBeepOn;

	/* Beep on connection & disconnection -- special option */
	if (GetOnOff(Section, "BeepOnConnect", FName, FALSE))
		ts->PortFlag |= PF_BEEPONCONNECT;

	/* Auto B-Plus activation -- special option */
	if (GetOnOff(Section, "BPAuto", FName, FALSE))
		ts->FTFlag |= FT_BPAUTO;
	if ((ts->FTFlag & FT_BPAUTO) != 0) {	/* Answerback */
		strncpy_s(ts->Answerback, sizeof(ts->Answerback), "\020++\0200",
		          _TRUNCATE);
		ts->AnswerbackLen = 5;
	}

	/* B-Plus ESCCTL flag  -- special option */
	if (GetOnOff(Section, "BPEscCtl", FName, FALSE))
		ts->FTFlag |= FT_BPESCCTL;

	/* B-Plus log  -- special option */
	if (GetOnOff(Section, "BPLog", FName, FALSE))
		ts->LogFlag |= LOG_BP;

	/* Clear serial port buffer when port opening -- special option */
	ts->ClearComBuffOnOpen =
		GetOnOff(Section, "ClearComBuffOnOpen", FName, TRUE);

	/* Confirm disconnection -- special option */
	if (GetOnOff(Section, "ConfirmDisconnect", FName, TRUE))
		ts->PortFlag |= PF_CONFIRMDISCONN;

	/* Ctrl code in Kanji -- special option */
	if (GetOnOff(Section, "CtrlInKanji", FName, TRUE))
		ts->TermFlag |= TF_CTRLINKANJI;

	/* Debug flag  -- special option */
	ts->Debug = GetOnOff(Section, "Debug", FName, FALSE);

	/* Delimiter list -- special option */
	GetPrivateProfileString(Section, "DelimList",
	                        "$20!\"#$24%&\'()*+,-./:;<=>?@[\\]^`{|}~",
	                        Temp, sizeof(Temp), FName);
	Hex2Str(Temp, ts->DelimList, sizeof(ts->DelimList));

	/* regard DBCS characters as delimiters -- special option */
	ts->DelimDBCS = GetOnOff(Section, "DelimDBCS", FName, TRUE);

	// Enable popup menu -- special option
	if (!GetOnOff(Section, "EnablePopupMenu", FName, TRUE))
		ts->MenuFlag |= MF_NOPOPUP;

	// Enable "Show menu" -- special option
	if (!GetOnOff(Section, "EnableShowMenu", FName, TRUE))
		ts->MenuFlag |= MF_NOSHOWMENU;

	// Enable the status line -- special option
	if (GetOnOff(Section, "EnableStatusLine", FName, TRUE))
		ts->TermFlag |= TF_ENABLESLINE;

	// fixed JIS --- special
	if (GetOnOff(Section, "FixedJIS", FName, FALSE))
		ts->TermFlag |= TF_FIXEDJIS;

	/* IME Flag  -- special option */
	ts->UseIME = GetOnOff(Section, "IME", FName, TRUE);

	/* IME-inline Flag  -- special option */
	ts->IMEInline = GetOnOff(Section, "IMEInline", FName, TRUE);

	/* Kermit log  -- special option */
	if (GetOnOff(Section, "KmtLog", FName, FALSE))
		ts->LogFlag |= LOG_KMT;

	// Enable language selection -- special option
	if (!GetOnOff(Section, "LanguageSelection", FName, TRUE))
		ts->MenuFlag |= MF_NOLANGUAGE;

	/* Maximum scroll buffer size  -- special option */
	ts->ScrollBuffMax =
		GetPrivateProfileInt(Section, "MaxBuffSize", 10000, FName);
	if (ts->ScrollBuffMax < 24)
		ts->ScrollBuffMax = 10000;

	/* Max com port number -- special option */
	ts->MaxComPort = GetPrivateProfileInt(Section, "MaxComPort", 4, FName);
	if (ts->MaxComPort < 4)
		ts->MaxComPort = 4;
	// COM16から99へ拡張 (2005.11.30 yutaka)
	// 99から200へ拡張 (2007.7.23 maya)
	if (ts->MaxComPort > MAXCOMPORT)
		ts->MaxComPort = MAXCOMPORT;
	if ((ts->ComPort < 1) || (ts->ComPort > ts->MaxComPort))
		ts->ComPort = 1;

	/* Non-blinking cursor -- special option */
	ts->NonblinkingCursor =
		GetOnOff(Section, "NonblinkingCursor", FName, FALSE);

	// フォーカス無効時のポリゴンカーソル (2008.1.24 yutaka)
	ts->KillFocusCursor =
		GetOnOff(Section, "KillFocusCursor", FName, TRUE);

	/* Delay for pass-thru printing activation */
	/*   -- special option */
	ts->PassThruDelay =
		GetPrivateProfileInt(Section, "PassThruDelay", 3, FName);

	/* Printer port for pass-thru printing */
	/*   -- special option */
	GetPrivateProfileString(Section, "PassThruPort", "",
	                        ts->PrnDev, sizeof(ts->PrnDev), FName);

	/* Printer Font --- special option */
	GetPrivateProfileString(Section, "PrnFont", "",
	                        Temp, sizeof(Temp), FName);
	if (strlen(Temp) == 0) {
		ts->PrnFont[0] = 0;
		ts->PrnFontSize.x = 0;
		ts->PrnFontSize.y = 0;
		ts->PrnFontCharSet = 0;
	}
	else {
		GetNthString(Temp, 1, sizeof(ts->PrnFont), ts->PrnFont);
		GetNthNum(Temp, 2, (int far *) &(ts->PrnFontSize.x));
		GetNthNum(Temp, 3, (int far *) &(ts->PrnFontSize.y));
		GetNthNum(Temp, 4, &(ts->PrnFontCharSet));
	}

	// Page margins (left, right, top, bottom) for printing
	//    -- special option
	GetPrivateProfileString(Section, "PrnMargin", "50,50,50,50",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 3; i++)
		GetNthNum(Temp, 1 + i, &ts->PrnMargin[i]);

	/* Quick-VAN log  -- special option */
	if (GetOnOff(Section, "QVLog", FName, FALSE))
		ts->LogFlag |= LOG_QV;

	/* Quick-VAN window size -- special */
	ts->QVWinSize = GetPrivateProfileInt(Section, "QVWinSize", 8, FName);

	/* Russian character set (print) -- special option */
	GetPrivateProfileString(Section, "RussPrint", "",
	                        Temp, sizeof(Temp), FName);
	ts->RussPrint = str2id(RussList, Temp, IdWindows);

	/* Scroll threshold -- special option */
	ts->ScrollThreshold =
		GetPrivateProfileInt(Section, "ScrollThreshold", 12, FName);

	ts->MouseWheelScrollLine =
		GetPrivateProfileInt(Section, "MouseWheelScrollLine", 3, FName);

	// Select on activate -- special option
	ts->SelOnActive = GetOnOff(Section, "SelectOnActivate", FName, TRUE);

	/* Send 8bit control sequence -- special option */
	ts->Send8BitCtrl = GetOnOff(Section, "Send8BitCtrl", FName, FALSE);

	/* Startup macro -- special option */
	GetPrivateProfileString(Section, "StartupMacro", "",
	                        ts->MacroFN, sizeof(ts->MacroFN), FName);

	/* TEK GIN Mouse keycode -- special option */
	ts->GINMouseCode =
		GetPrivateProfileInt(Section, "TEKGINMouseCode", 32, FName);

	/* Telnet Auto Detect -- special option */
	ts->TelAutoDetect = GetOnOff(Section, "TelAutoDetect", FName, TRUE);

	/* Telnet binary flag -- special option */
	ts->TelBin = GetOnOff(Section, "TelBin", FName, FALSE);

	/* Telnet Echo flag -- special option */
	ts->TelEcho = GetOnOff(Section, "TelEcho", FName, FALSE);

	/* Telnet log  -- special option */
	if (GetOnOff(Section, "TelLog", FName, FALSE))
		ts->LogFlag |= LOG_TEL;

	/* TCP port num for telnet -- special option */
	ts->TelPort = GetPrivateProfileInt(Section, "TelPort", 23, FName);

	/* Telnet keep-alive packet(NOP command) interval -- special option */
	ts->TelKeepAliveInterval =
		GetPrivateProfileInt(Section, "TelKeepAliveInterval", 300, FName);

	/* Max number of broadcast commad history */
	ts->MaxBroadcatHistory =
		GetPrivateProfileInt(Section, "MaxBroadcatHistory", 99, FName);

	/* Local echo for non-telnet */
	ts->TCPLocalEcho = GetOnOff(Section, "TCPLocalEcho", FName, FALSE);

	/* "new-line (transmit)" option for non-telnet -- special option */
	GetPrivateProfileString(Section, "TCPCRSend", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "CR") == 0)
		ts->TCPCRSend = IdCR;
	else if (_stricmp(Temp, "CRLF") == 0)
		ts->TCPCRSend = IdCRLF;
	else
		ts->TCPCRSend = 0;		// disabled

	/* Use text (background) color for "white (black)" --- special option */
	if (GetOnOff(Section, "UseTextColor", FName, FALSE))
		ts->ColorFlag |= CF_USETEXTCOLOR;

	/* Title format -- special option */
	ts->TitleFormat =
		GetPrivateProfileInt(Section, "TitleFormat", 5, FName);

	/* VT Compatible Tab -- special option */
	ts->VTCompatTab = GetOnOff(Section, "VTCompatTab", FName, FALSE);

	/* VT Font space --- special option */
	GetPrivateProfileString(Section, "VTFontSpace", "0,0,0,0",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, &ts->FontDX);
	GetNthNum(Temp, 2, &ts->FontDW);
	GetNthNum(Temp, 3, &ts->FontDY);
	GetNthNum(Temp, 4, &ts->FontDH);
	if (ts->FontDX < 0)
		ts->FontDX = 0;
	if (ts->FontDW < 0)
		ts->FontDW = 0;
	ts->FontDW = ts->FontDW + ts->FontDX;
	if (ts->FontDY < 0)
		ts->FontDY = 0;
	if (ts->FontDH < 0)
		ts->FontDH = 0;
	ts->FontDH = ts->FontDH + ts->FontDY;

	// VT-print scaling factors (pixels per inch) --- special option
	GetPrivateProfileString(Section, "VTPPI", "0,0",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, (int far *) &ts->VTPPI.x);
	GetNthNum(Temp, 2, (int far *) &ts->VTPPI.y);

	// TEK-print scaling factors (pixels per inch) --- special option
	GetPrivateProfileString(Section, "TEKPPI", "0,0",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, (int far *) &ts->TEKPPI.x);
	GetNthNum(Temp, 2, (int far *) &ts->TEKPPI.y);

	// Show "Window" menu -- special option
	if (GetOnOff(Section, "WindowMenu", FName, TRUE))
		ts->MenuFlag |= MF_SHOWWINMENU;

	/* XMODEM log  -- special option */
	if (GetOnOff(Section, "XmodemLog", FName, FALSE))
		ts->LogFlag |= LOG_X;

	/* YMODEM log  -- special option */
	if (GetOnOff(Section, "YmodemLog", FName, FALSE))
		ts->LogFlag |= LOG_Y;

	/* Auto ZMODEM activation -- special option */
	if (GetOnOff(Section, "ZmodemAuto", FName, FALSE))
		ts->FTFlag |= FT_ZAUTO;

	/* ZMODEM data subpacket length for sending -- special */
	ts->ZmodemDataLen =
		GetPrivateProfileInt(Section, "ZmodemDataLen", 1024, FName);
	/* ZMODEM window size for sending -- special */
	ts->ZmodemWinSize =
		GetPrivateProfileInt(Section, "ZmodemWinSize", 32767, FName);

	/* ZMODEM ESCCTL flag  -- special option */
	if (GetOnOff(Section, "ZmodemEscCtl", FName, FALSE))
		ts->FTFlag |= FT_ZESCCTL;

	/* ZMODEM log  -- special option */
	if (GetOnOff(Section, "ZmodemLog", FName, FALSE))
		ts->LogFlag |= LOG_Z;

	/* ZMODEM 受信コマンド (2007.12.21 yutaka) */
	GetPrivateProfileString(Section, "ZModemRcvCommand", "rz",
	                        ts->ZModemRcvCommand, sizeof(ts->ZModemRcvCommand), FName);

#ifndef NO_COPYLINE_FIX

	/* Enable continued-line copy  -- special option */
	ts->EnableContinuedLineCopy =
		GetOnOff(Section, "EnableContinuedLineCopy", FName, FALSE);
#endif							/* NO_COPYLINE_FIX */

	ts->DisablePasteMouseRButton =
		GetOnOff(Section, "DisablePasteMouseRButton", FName, FALSE);

	// added DisablePasteMouseMButton (2008.3.2 maya)
	ts->DisablePasteMouseMButton =
		GetOnOff(Section, "DisablePasteMouseMButton", FName, TRUE);

	// added ConfirmPasteMouseRButton (2007.3.17 maya)
	ts->ConfirmPasteMouseRButton =
		GetOnOff(Section, "ConfirmPasteMouseRButton", FName, FALSE);

	// added ConfirmChangePaste (2008.2.3 yutaka)
	ts->ConfirmChangePaste =
		GetOnOff(Section, "ConfirmChangePaste", FName, TRUE);

	// added ScrollWindowClearScreen (2008.5.3 yutaka)
	ts->ScrollWindowClearScreen =
		GetOnOff(Section, "ScrollWindowClearScreen", FName, TRUE);

	// added SelectOnlyByLButton (2007.11.20 maya)
	ts->SelectOnlyByLButton =
		GetOnOff(Section, "SelectOnlyByLButton", FName, TRUE);

	// added DisableAcceleratorSendBreak (2007.3.17 maya)
	ts->DisableAcceleratorSendBreak =
		GetOnOff(Section, "DisableAcceleratorSendBreak", FName, FALSE);

	// WinSock connecting timeout value (2007.1.11 yutaka)
	ts->ConnectingTimeout =
		GetPrivateProfileInt(Section, "ConnectingTimeout", 0, FName);

	// mouse cursor 
	GetPrivateProfileString(Section, "MouseCursor", "IBEAM",
	                        Temp, sizeof(Temp), FName);
	strncpy_s(ts->MouseCursorName, sizeof(ts->MouseCursorName), Temp,
	          _TRUNCATE);

	// Translucent window
	ts->AlphaBlend =
		GetPrivateProfileInt(Section, "AlphaBlend ", 255, FName);
	ts->AlphaBlend = max(0, ts->AlphaBlend);
	ts->AlphaBlend = min(255, ts->AlphaBlend);

	// Cygwin install path
	GetPrivateProfileString(Section, "CygwinDirectory ", "c:\\cygwin",
	                        Temp, sizeof(Temp), FName);
	strncpy_s(ts->CygwinDirectory, sizeof(ts->CygwinDirectory), Temp,
	          _TRUNCATE);

	// Viewlog Editor path
	GetPrivateProfileString(Section, "ViewlogEditor ", "notepad.exe",
	                        Temp, sizeof(Temp), FName);
	strncpy_s(ts->ViewlogEditor, sizeof(ts->ViewlogEditor), Temp,
	          _TRUNCATE);

	// Locale for UTF-8
	GetPrivateProfileString(Section, "Locale ", DEFAULT_LOCALE,
	                        Temp, sizeof(Temp), FName);
	strncpy_s(ts->Locale, sizeof(ts->Locale), Temp, _TRUNCATE);

	// CodePage
	ts->CodePage =
		GetPrivateProfileInt(Section, "CodePage ", DEFAULT_CODEPAGE,
		                     FName);

	// UI language message file
	GetPrivateProfileString(Section, "UILanguageFile", "lang\\Default.lng",
	                        Temp, sizeof(Temp), FName);
	{
		char CurDir[MAX_PATH];

		// フルパス化する前に読み込み時の設定を取っておく
		strncpy_s(ts->UILanguageFile_ini, sizeof(ts->UILanguageFile_ini), Temp, _TRUNCATE);

		GetCurrentDirectory(sizeof(CurDir), CurDir);
		SetCurrentDirectory(ts->HomeDir);
		_fullpath(ts->UILanguageFile, Temp, sizeof(ts->UILanguageFile));
		SetCurrentDirectory(CurDir);
	}

	// Broadcast Command History (2007.3.3 maya)
	ts->BroadcastCommandHistory =
		GetOnOff(Section, "BroadcastCommandHistory", FName, FALSE);

	// 337: 2007/03/20 Accept Broadcast
	ts->AcceptBroadcast =
		GetOnOff(Section, "AcceptBroadcast", FName, TRUE);

	// Confirm send a file when drag and drop (2007.12.28 maya)
	ts->ConfirmFileDragAndDrop =
		GetOnOff(Section, "ConfirmFileDragAndDrop", FName, TRUE);

	// Translate mouse wheel to cursor key when application cursor mode
	ts->TranslateWheelToCursor =
		GetOnOff(Section, "TranslateWheelToCursor", FName, TRUE);

	// Display "New Connection" dialog on startup (2008.1.18 maya)
	ts->HostDialogOnStartup =
		GetOnOff(Section, "HostDialogOnStartup", FName, TRUE);

	// Mouse event tracking
	ts->MouseEventTracking =
		GetOnOff(Section, "MouseEventTracking", FName, TRUE);

	// Maximized bug tweak
	ts->MaximizedBugTweak =
		GetOnOff(Section, "MaximizedBugTweak", FName, TRUE);

	// Convert Unicode symbol characters to DEC Special characters
	ts->UnicodeDecSpMapping =
		GetPrivateProfileInt(Section, "UnicodeToDecSpMapping", 3, FName);

	// VT Window Icon
	GetPrivateProfileString(Section, "VTIcon", "Default",
	                        Temp, sizeof(Temp), FName);
	ts->VTIcon = IconName2IconId(Temp);

	// Tek Window Icon
	GetPrivateProfileString(Section, "TEKIcon", "Default",
	                        Temp, sizeof(Temp), FName);
	ts->TEKIcon = IconName2IconId(Temp);

	// Unknown Unicode Character
	ts->UnknownUnicodeCharaAsWide =
		GetOnOff(Section, "UnknownUnicodeCharacterAsWide", FName, FALSE);

#ifdef USE_NORMAL_BGCOLOR
	// UseNormalBGColor
	ts->UseNormalBGColor =
		GetOnOff(Section, "UseNormalBGColor", FName, FALSE);
	// 2006/03/11 by 337
	if (ts->UseNormalBGColor) {
		ts->VTBoldColor[1] =
			ts->VTBlinkColor[1] = ts->URLColor[1] = ts->VTColor[1];
	}
#endif

	// AutoScrollOnlyInBottomLine
	ts->AutoScrollOnlyInBottomLine =
		GetOnOff(Section, "AutoScrollOnlyInBottomLine", FName, FALSE);

	// Accept remote-controlled window title changing
	GetPrivateProfileString(Section, "AcceptTitleChangeRequest", "overwrite",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "overwrite") == 0 || _stricmp(Temp, "on") == 0)
		ts->AcceptTitleChangeRequest = IdTitleChangeRequestOverwrite;
	else if (_stricmp(Temp, "ahead") == 0)
		ts->AcceptTitleChangeRequest = IdTitleChangeRequestAhead;
	else if (_stricmp(Temp, "last") == 0)
		ts->AcceptTitleChangeRequest = IdTitleChangeRequestLast;
	else
		ts->AcceptTitleChangeRequest = IdTitleChangeRequestOff;

	// Size of paste confirm dialog
	GetPrivateProfileString(Section, "PasteDialogSize", "330,220",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, &ts->PasteDialogSize.cx);
	GetNthNum(Temp, 2, &ts->PasteDialogSize.cy);
	if (ts->PasteDialogSize.cx < 0)
		ts->PasteDialogSize.cx = 330;
	if (ts->PasteDialogSize.cy < 0)
		ts->PasteDialogSize.cy = 220;

	// Disable mouse event tracking when Control-Key is pressed.
	ts->DisableMouseTrackingByCtrl =
		GetOnOff(Section, "DisableMouseTrackingByCtrl", FName, TRUE);

	// Disable TranslateWheelToCursor setting when Control-Key is pressed.
	ts->DisableWheelToCursorByCtrl =
		GetOnOff(Section, "DisableWheelToCursorByCtrl", FName, TRUE);

	// Strict Key Mapping.
	ts->StrictKeyMapping =
		GetOnOff(Section, "StrictKeyMapping", FName, FALSE);

	// added Wait4allMacroCommand (2009.3.23 yutaka)
	ts->Wait4allMacroCommand =
		GetOnOff(Section, "Wait4allMacroCommand", FName, FALSE);

	// added DisableMenuSendBreak (2009.4.6 maya)
	ts->DisableMenuSendBreak =
		GetOnOff(Section, "DisableMenuSendBreak", FName, FALSE);

	// added ClearScreenOnCloseConnection (2009.4.6 maya)
	ts->ClearScreenOnCloseConnection =
		GetOnOff(Section, "ClearScreenOnCloseConnection", FName, FALSE);

	// added DisableAcceleratorDuplicateSession (2009.4.6 maya)
	ts->DisableAcceleratorDuplicateSession =
		GetOnOff(Section, "DisableAcceleratorDuplicateSession", FName, FALSE);

	// added PasteDelayPerLine (2009.4.12 maya)
	ts->PasteDelayPerLine =
		GetPrivateProfileInt(Section, "PasteDelayPerLine", 10, FName);
	{
		int tmp = min(max(0, ts->PasteDelayPerLine), 5000);
		ts->PasteDelayPerLine = tmp;
	}
}

void FAR PASCAL WriteIniFile(PCHAR FName, PTTSet ts)
{
	int i;
	char Temp[MAXPATHLEN];
	char buf[20];

	/* version */
	WritePrivateProfileString(Section, "Version", "2.3", FName);

	/* Language */
	switch (ts->Language) {
	case IdJapanese:
		strncpy_s(Temp, sizeof(Temp), "Japanese", _TRUNCATE);
		break;
	case IdKorean:
	  	strncpy_s(Temp, sizeof(Temp), "Korean",   _TRUNCATE);
		break;
	case IdRussian:
	  	strncpy_s(Temp, sizeof(Temp), "Russian",  _TRUNCATE);
		break;
	default:
	  	strncpy_s(Temp, sizeof(Temp), "English",  _TRUNCATE);
	}

	WritePrivateProfileString(Section, "Language", Temp, FName);

	/* Port type */
	if (ts->PortType == IdSerial)
		strncpy_s(Temp, sizeof(Temp), "serial", _TRUNCATE);
	else						/* IdFile -> IdTCPIP */
		strncpy_s(Temp, sizeof(Temp), "tcpip", _TRUNCATE);

	WritePrivateProfileString(Section, "Port", Temp, FName);

	/* Save win position */
	if (ts->SaveVTWinPos) {
		/* VT win position */
		WriteInt2(Section, "VTPos", FName, ts->VTPos.x, ts->VTPos.y);
	}

	/* VT terminal size  */
	WriteInt2(Section, "TerminalSize", FName,
			  ts->TerminalWidth, ts->TerminalHeight);

	/* Terminal size = Window size */
	WriteOnOff(Section, "TermIsWin", FName, ts->TermIsWin);

	/* Auto window resize flag */
	WriteOnOff(Section, "AutoWinResize", FName, ts->AutoWinResize);

	/* CR Receive */
	if (ts->CRReceive == IdCRLF) {
		strncpy_s(Temp, sizeof(Temp), "CRLF", _TRUNCATE);
	}
	else if (ts->CRReceive == IdLF) {
		strncpy_s(Temp, sizeof(Temp), "LF", _TRUNCATE);
	}
	else {
		strncpy_s(Temp, sizeof(Temp), "CR", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "CRReceive", Temp, FName);

	/* CR Send */
	if (ts->CRSend == IdCRLF)
		strncpy_s(Temp, sizeof(Temp), "CRLF", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "CR", _TRUNCATE);
	WritePrivateProfileString(Section, "CRSend", Temp, FName);

	/* Local echo */
	WriteOnOff(Section, "LocalEcho", FName, ts->LocalEcho);

	/* Answerback */
	if ((ts->FTFlag & FT_BPAUTO) == 0) {
		Str2Hex(ts->Answerback, Temp, ts->AnswerbackLen,
		        sizeof(Temp) - 1, TRUE);
		WritePrivateProfileString(Section, "Answerback", Temp, FName);
	}

	/* Kanji Code (receive)  */
	switch (ts->KanjiCode) {
	case IdEUC:
		strncpy_s(Temp, sizeof(Temp), "EUC", _TRUNCATE);
		break;
	case IdJIS:
		strncpy_s(Temp, sizeof(Temp), "JIS", _TRUNCATE);
		break;
	case IdUTF8:
		strncpy_s(Temp, sizeof(Temp), "UTF-8", _TRUNCATE);
		break;
	case IdUTF8m:
		strncpy_s(Temp, sizeof(Temp), "UTF-8m", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "SJIS", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "KanjiReceive", Temp, FName);

	/* Katakana (receive)  */
	if (ts->JIS7Katakana == 1)
		strncpy_s(Temp, sizeof(Temp), "7", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "8", _TRUNCATE);

	WritePrivateProfileString(Section, "KatakanaReceive", Temp, FName);

	/* Kanji Code (transmit)  */
	switch (ts->KanjiCodeSend) {
	case IdEUC:
		strncpy_s(Temp, sizeof(Temp), "EUC", _TRUNCATE);
		break;
	case IdJIS:
		strncpy_s(Temp, sizeof(Temp), "JIS", _TRUNCATE);
		break;
	case IdUTF8:
		strncpy_s(Temp, sizeof(Temp), "UTF-8", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "SJIS", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "KanjiSend", Temp, FName);

	/* Katakana (transmit)  */
	if (ts->JIS7KatakanaSend == 1)
		strncpy_s(Temp, sizeof(Temp), "7", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "8", _TRUNCATE);

	WritePrivateProfileString(Section, "KatakanaSend", Temp, FName);

	/* KanjiIn */
	if (ts->KanjiIn == IdKanjiInA)
		strncpy_s(Temp, sizeof(Temp), "@", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "B", _TRUNCATE);

	WritePrivateProfileString(Section, "KanjiIn", Temp, FName);

	/* KanjiOut */
	switch (ts->KanjiOut) {
	case IdKanjiOutB:
		strncpy_s(Temp, sizeof(Temp), "B", _TRUNCATE);
		break;
	case IdKanjiOutH:
		strncpy_s(Temp, sizeof(Temp), "H", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "J", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "KanjiOut", Temp, FName);

	// new configuration
	WriteInt(Section, "ConnectingTimeout", FName, ts->ConnectingTimeout);

	WriteOnOff(Section, "DisablePasteMouseRButton", FName,
	           ts->DisablePasteMouseRButton);

	// added DisablePasteMouseMButton (2008.3.2 maya)
	WriteOnOff(Section, "DisablePasteMouseMButton", FName,
	           ts->DisablePasteMouseMButton);

	// added ConfirmPasteMouseRButton (2007.3.17 maya)
	WriteOnOff(Section, "ConfirmPasteMouseRButton", FName,
	           ts->ConfirmPasteMouseRButton);

	// added ConfirmChangePaste
	WriteOnOff(Section, "ConfirmChangePaste", FName,
	           ts->ConfirmChangePaste);

	// added ScrollWindowClearScreen
	WriteOnOff(Section, "ScrollWindowClearScreen", FName,
		ts->ScrollWindowClearScreen);

	// added SelectOnlyByLButton (2007.11.20 maya)
	WriteOnOff(Section, "SelectOnlyByLButton", FName,
	           ts->SelectOnlyByLButton);
	// added DisableAcceleratorSendBreak (2007.3.17 maya)
	WriteOnOff(Section, "DisableAcceleratorSendBreak", FName,
	           ts->DisableAcceleratorSendBreak);
	WriteOnOff(Section, "EnableContinuedLineCopy", FName,
	           ts->EnableContinuedLineCopy);
	WritePrivateProfileString(Section, "MouseCursor", ts->MouseCursorName,
	                          FName);
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->AlphaBlend);
	WritePrivateProfileString(Section, "AlphaBlend", Temp, FName);
	WritePrivateProfileString(Section, "CygwinDirectory",
	                          ts->CygwinDirectory, FName);
	WritePrivateProfileString(Section, "ViewlogEditor", ts->ViewlogEditor,
	                          FName);
	WritePrivateProfileString(Section, "Locale", ts->Locale, FName);
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->CodePage);
	WritePrivateProfileString(Section, "CodePage", Temp, FName);

	// ANSI color(2004.9.5 yutaka)
	Temp[0] = '\0';
	for (i = 0; i < 15; i++) {
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d,%d,%d,%d, ",
		            i,
		            GetRValue(ts->ANSIColor[i]),
		            GetGValue(ts->ANSIColor[i]),
		            GetBValue(ts->ANSIColor[i])
		);
		strncat_s(Temp, sizeof(Temp), buf, _TRUNCATE);
	}
	i = 15;
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d,%d,%d,%d",
	            i,
	            GetRValue(ts->ANSIColor[i]),
	            GetGValue(ts->ANSIColor[i]),
	            GetBValue(ts->ANSIColor[i])
	);
	strncat_s(Temp, sizeof(Temp), buf, _TRUNCATE);
	WritePrivateProfileString(Section, "ANSIColor", Temp, FName);

	/* AutoWinChange VT<->TEK */
	WriteOnOff(Section, "AutoWinSwitch", FName, ts->AutoWinSwitch);

	/* Terminal ID */
	id2str(TermList, ts->TerminalID, IdVT100, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "TerminalID", Temp, FName);

	/* Russian character set (host)  */
	id2str(RussList, ts->RussHost, IdKOI8, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "RussHost", Temp, FName);

	/* Russian character set (client)  */
	id2str(RussList, ts->RussClient, IdWindows, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "RussClient", Temp, FName);

	/* Title text */
	WritePrivateProfileString(Section, "Title", ts->Title, FName);

	/* Cursor shape */
	switch (ts->CursorShape) {
	case IdVCur:
		strncpy_s(Temp, sizeof(Temp), "vertical", _TRUNCATE);
		break;
	case IdHCur:
		strncpy_s(Temp, sizeof(Temp), "horizontal", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "block", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "CursorShape", Temp, FName);

	/* Hide title */
	WriteOnOff(Section, "HideTitle", FName, ts->HideTitle);

	/* Popup menu */
	WriteOnOff(Section, "PopupMenu", FName, ts->PopupMenu);

	/* PC-Style bold color mapping */
	WriteOnOff(Section, "PcBoldColor", FName,
	           (WORD) (ts->ColorFlag & CF_PCBOLD16));

	/* aixterm 16 colors mode */
	WriteOnOff(Section, "Aixterm16Color", FName,
	           (WORD) (ts->ColorFlag & CF_AIXTERM16));

	/* xterm 256 colors mode */
	WriteOnOff(Section, "Xterm256Color", FName,
	           (WORD) (ts->ColorFlag & CF_XTERM256));

	/* Enable scroll buffer */
	WriteOnOff(Section, "EnableScrollBuff", FName, ts->EnableScrollBuff);

	/* Scroll buffer size */
	WriteInt(Section, "ScrollBuffSize", FName, ts->ScrollBuffSize);

	/* VT Color */
	for (i = 0; i <= 1; i++) {
		if (ts->ColorFlag & CF_REVERSEVIDEO) {
			if (ts->ColorFlag & CF_REVERSECOLOR) {
				ts->TmpColor[0][i * 3] = GetRValue(ts->VTReverseColor[i]);
				ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTReverseColor[i]);
				ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTReverseColor[i]);
			}
			else {
				ts->TmpColor[0][i * 3] = GetRValue(ts->VTColor[!i]);
				ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTColor[!i]);
				ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTColor[!i]);
			}
		}
		else {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTColor[i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTColor[i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTColor[i]);
		}
	}
	WriteInt6(Section, "VTColor", FName,
	          ts->TmpColor[0][0], ts->TmpColor[0][1], ts->TmpColor[0][2],
	          ts->TmpColor[0][3], ts->TmpColor[0][4], ts->TmpColor[0][5]);

	/* VT Bold Color */
	for (i = 0; i <= 1; i++) {
		if (ts->ColorFlag & CF_REVERSEVIDEO) {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTBoldColor[!i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTBoldColor[!i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTBoldColor[!i]);
		}
		else {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTBoldColor[i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTBoldColor[i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTBoldColor[i]);
		}
	}
	WriteInt6(Section, "VTBoldColor", FName,
	          ts->TmpColor[0][0], ts->TmpColor[0][1], ts->TmpColor[0][2],
	          ts->TmpColor[0][3], ts->TmpColor[0][4], ts->TmpColor[0][5]);

	/* VT Blink Color */
	for (i = 0; i <= 1; i++) {
		if (ts->ColorFlag & CF_REVERSEVIDEO) {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTBlinkColor[!i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTBlinkColor[!i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTBlinkColor[!i]);
		}
		else {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTBlinkColor[i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTBlinkColor[i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTBlinkColor[i]);
		}
	}
	WriteInt6(Section, "VTBlinkColor", FName,
	          ts->TmpColor[0][0], ts->TmpColor[0][1], ts->TmpColor[0][2],
	          ts->TmpColor[0][3], ts->TmpColor[0][4], ts->TmpColor[0][5]);

	/* VT Reverse Color */
	for (i = 0; i <= 1; i++) {
		if (ts->ColorFlag & CF_REVERSEVIDEO && ts->ColorFlag & CF_REVERSECOLOR) {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTColor[i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTColor[i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTColor[i]);
		}
		else {
			ts->TmpColor[0][i * 3] = GetRValue(ts->VTReverseColor[i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->VTReverseColor[i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->VTReverseColor[i]);
		}
	}
	WriteInt6(Section, "VTReverseColor", FName,
	          ts->TmpColor[0][0], ts->TmpColor[0][1], ts->TmpColor[0][2],
	          ts->TmpColor[0][3], ts->TmpColor[0][4], ts->TmpColor[0][5]);

	WriteOnOff(Section, "EnableClickableUrl", FName,
	           ts->EnableClickableUrl);

	/* URL color */
	for (i = 0; i <= 1; i++) {
		if (ts->ColorFlag & CF_REVERSEVIDEO) {
			ts->TmpColor[0][i * 3] = GetRValue(ts->URLColor[!i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->URLColor[!i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->URLColor[!i]);
		}
		else {
			ts->TmpColor[0][i * 3] = GetRValue(ts->URLColor[i]);
			ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->URLColor[i]);
			ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->URLColor[i]);
		}
	}
	WriteInt6(Section, "URLColor", FName,
	          ts->TmpColor[0][0], ts->TmpColor[0][1], ts->TmpColor[0][2],
	          ts->TmpColor[0][3], ts->TmpColor[0][4], ts->TmpColor[0][5]);

	WriteOnOff(Section, "EnableBoldAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_BOLDCOLOR));

	WriteOnOff(Section, "EnableBlinkAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_BLINKCOLOR));

	WriteOnOff(Section, "EnableReverseAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_REVERSECOLOR));

	WriteOnOff(Section, "EnableURLColor", FName,
	           (WORD) (ts->ColorFlag & CF_URLCOLOR));

	WriteOnOff(Section, "EnableANSIColor", FName,
	           (WORD) (ts->ColorFlag & CF_ANSICOLOR));

	/* TEK Color */
	for (i = 0; i <= 1; i++) {
		ts->TmpColor[0][i * 3] = GetRValue(ts->TEKColor[i]);
		ts->TmpColor[0][i * 3 + 1] = GetGValue(ts->TEKColor[i]);
		ts->TmpColor[0][i * 3 + 2] = GetBValue(ts->TEKColor[i]);
	}
	WriteInt6(Section, "TEKColor", FName,
	          ts->TmpColor[0][0], ts->TmpColor[0][1], ts->TmpColor[0][2],
	          ts->TmpColor[0][3], ts->TmpColor[0][4], ts->TmpColor[0][5]);

	/* TEK color emulation */
	WriteOnOff(Section, "TEKColorEmulation", FName, ts->TEKColorEmu);

	/* VT Font */
	WriteFont(Section, "VTFont", FName,
	          ts->VTFont, ts->VTFontSize.x, ts->VTFontSize.y,
	          ts->VTFontCharSet);

	/* Enable bold font flag */
	WriteOnOff(Section, "EnableBold", FName, ts->EnableBold);

	/* Russian character set (font) */
	id2str(RussList, ts->RussFont, IdWindows, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "RussFont", Temp, FName);

	/* TEK Font */
	WriteFont(Section, "TEKFont", FName,
	          ts->TEKFont, ts->TEKFontSize.x, ts->TEKFontSize.y,
	          ts->TEKFontCharSet);

	/* BS key */
	if (ts->BSKey == IdDEL)
		strncpy_s(Temp, sizeof(Temp), "DEL", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "BS", _TRUNCATE);
	WritePrivateProfileString(Section, "BSKey", Temp, FName);

	/* Delete key */
	WriteOnOff(Section, "DeleteKey", FName, ts->DelKey);

	/* Meta key */
	WriteOnOff(Section, "MetaKey", FName, ts->MetaKey);

	/* Application Keypad */
	WriteOnOff(Section, "DisableAppKeypad", FName, ts->DisableAppKeypad);

	/* Application Cursor */
	WriteOnOff(Section, "DisableAppCursor", FName, ts->DisableAppCursor);

	/* Russian keyboard type */
	id2str(RussList2, ts->RussKeyb, IdWindows, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "RussKeyb", Temp, FName);

	/* Serial port ID */
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->ComPort);
	WritePrivateProfileString(Section, "ComPort", Temp, FName);

	/* Baud rate */
	id2str(BaudList, ts->Baud, IdBaud9600, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "BaudRate", Temp, FName);

	/* Parity */
	switch (ts->Parity) {
	case IdParityEven:
		strncpy_s(Temp, sizeof(Temp), "even", _TRUNCATE);
		break;
	case IdParityOdd:
		strncpy_s(Temp, sizeof(Temp), "odd", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "none", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "Parity", Temp, FName);

	/* Data bit */
	if (ts->DataBit == IdDataBit7)
		strncpy_s(Temp, sizeof(Temp), "7", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "8", _TRUNCATE);

	WritePrivateProfileString(Section, "DataBit", Temp, FName);

	/* Stop bit */
	if (ts->StopBit == IdStopBit2)
		strncpy_s(Temp, sizeof(Temp), "2", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "1", _TRUNCATE);

	WritePrivateProfileString(Section, "StopBit", Temp, FName);

	/* Flow control */
	switch (ts->Flow) {
	case IdFlowX:
		strncpy_s(Temp, sizeof(Temp), "x", _TRUNCATE);
		break;
	case IdFlowHard:
		strncpy_s(Temp, sizeof(Temp), "hard", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "none", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "FlowCtrl", Temp, FName);

	/* Delay per character */
	WriteInt(Section, "DelayPerChar", FName, ts->DelayPerChar);

	/* Delay per line */
	WriteInt(Section, "DelayPerLine", FName, ts->DelayPerLine);

	/* Telnet flag */
	WriteOnOff(Section, "Telnet", FName, ts->Telnet);

	/* Telnet terminal type */
	WritePrivateProfileString(Section, "TermType", ts->TermType, FName);

	/* TCP port num for non-telnet */
	WriteUint(Section, "TCPPort", FName, ts->TCPPort);

	/* Auto close flag */
	WriteOnOff(Section, "AutoWinClose", FName, ts->AutoWinClose);

	/* History list */
	WriteOnOff(Section, "Historylist", FName, ts->HistoryList);

	/* File transfer binary flag */
	WriteOnOff(Section, "TransBin", FName, ts->TransBin);

	/* Log append */
	WriteOnOff(Section, "LogAppend", FName, ts->Append);

	/* File transfer binary flag */
	WriteOnOff(Section, "LogTypePlainText", FName, ts->LogTypePlainText);

	/* Log with timestamp (2006.7.23 maya) */
	WriteOnOff(Section, "LogTimestamp", FName, ts->LogTimestamp);

	/* Log without transfer dialog */
	WriteOnOff(Section, "LogHideDialog", FName, ts->LogHideDialog);

	/* Default Log file name (2006.8.28 maya) */
	WritePrivateProfileString(Section, "LogDefaultName",
	                          ts->LogDefaultName, FName);

	/* Default Log file path (2007.5.30 maya) */
	WritePrivateProfileString(Section, "LogDefaultPath",
	                          ts->LogDefaultPath, FName);

	/* Auto start logging (2007.5.31 maya) */
	WriteOnOff(Section, "LogAutoStart", FName, ts->LogAutoStart);

	/* XMODEM option */
	switch (ts->XmodemOpt) {
	case XoptCRC:
		strncpy_s(Temp, sizeof(Temp), "crc", _TRUNCATE);
		break;
	case Xopt1K:
		strncpy_s(Temp, sizeof(Temp), "1k", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "checksum", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "XmodemOpt", Temp, FName);

	/* XMODEM binary flag */
	WriteOnOff(Section, "XmodemBin", FName, ts->XmodemBin);

	/* XMODEM 受信コマンド (2007.12.21 yutaka) */
	WritePrivateProfileString(Section, "XmodemRcvCommand",
	                          ts->XModemRcvCommand, FName);

	/* Default directory for file transfer */
	WritePrivateProfileString(Section, "FileDir", ts->FileDir, FName);

	/* filter on file send (2007.6.5 maya) */
	WritePrivateProfileString(Section, "FileSendFilter",
	                          ts->FileSendFilter, FName);

/*------------------------------------------------------------------*/
	/* 8 bit control code flag  -- special option */
	WriteOnOff(Section, "Accept8BitCtrl", FName,
	           (WORD) (ts->TermFlag & TF_ACCEPT8BITCTRL));

	/* Wrong sequence flag  -- special option */
	WriteOnOff(Section, "AllowWrongSequence", FName,
	           (WORD) (ts->TermFlag & TF_ALLOWWRONGSEQUENCE));

	/* Auto file renaming --- special option */
	WriteOnOff(Section, "AutoFileRename", FName,
	           (WORD) (ts->FTFlag & FT_RENAME));

	/* Auto text copy --- special option */
	WriteOnOff(Section, "AutoTextCopy", FName, ts->AutoTextCopy);

	/* Back wrap -- special option */
	WriteOnOff(Section, "BackWrap", FName,
	           (WORD) (ts->TermFlag & TF_BACKWRAP));

	/* Beep type -- special option */
	WriteOnOff(Section, "Beep", FName, ts->Beep);
	switch (ts->Beep) {
	case IdBeepOff:
		WritePrivateProfileString(Section, "Beep", "off", FName);
		break;
	case IdBeepOn:
		WritePrivateProfileString(Section, "Beep", "on", FName);
		break;
	case IdBeepVisual:
		WritePrivateProfileString(Section, "Beep", "visual", FName);
		break;
	}

	/* Beep on connection & disconnection -- special option */
	WriteOnOff(Section, "BeepOnConnect", FName,
	           (WORD) (ts->PortFlag & PF_BEEPONCONNECT));

	/* Auto B-Plus activation -- special option */
	WriteOnOff(Section, "BPAuto", FName, (WORD) (ts->FTFlag & FT_BPAUTO));

	/* B-Plus ESCCTL flag  -- special option */
	WriteOnOff(Section, "BPEscCtl", FName,
	           (WORD) (ts->FTFlag & FT_BPESCCTL));

	/* B-Plus log  -- special option */
	WriteOnOff(Section, "BPLog", FName, (WORD) (ts->LogFlag & LOG_BP));

	/* Clear serial port buffer when port opening -- special option */
	WriteOnOff(Section, "ClearComBuffOnOpen", FName, ts->ClearComBuffOnOpen);

	/* Confirm disconnection -- special option */
	WriteOnOff(Section, "ConfirmDisconnect", FName,
	           (WORD) (ts->PortFlag & PF_CONFIRMDISCONN));

	/* Ctrl code in Kanji -- special option */
	WriteOnOff(Section, "CtrlInKanji", FName,
	           (WORD) (ts->TermFlag & TF_CTRLINKANJI));

	/* Debug flag  -- special option */
	WriteOnOff(Section, "Debug", FName, ts->Debug);

	/* Delimiter list -- special option */
	Str2Hex(ts->DelimList, Temp, strlen(ts->DelimList),
	        sizeof(Temp) - 1, TRUE);
	WritePrivateProfileString(Section, "DelimList", Temp, FName);

	/* regard DBCS characters as delimiters -- special option */
	WriteOnOff(Section, "DelimDBCS", FName, ts->DelimDBCS);

	// Enable popup menu -- special option
	if ((ts->MenuFlag & MF_NOPOPUP) == 0)
		WriteOnOff(Section, "EnablePopupMenu", FName, 1);
	else
		WriteOnOff(Section, "EnablePopupMenu", FName, 0);

	// Enable "Show menu" -- special option
	if ((ts->MenuFlag & MF_NOSHOWMENU) == 0)
		WriteOnOff(Section, "EnableShowMenu", FName, 1);
	else
		WriteOnOff(Section, "EnableShowMenu", FName, 0);

	/* Enable the status line -- special option */
	WriteOnOff(Section, "EnableStatusLine", FName,
	           (WORD) (ts->TermFlag & TF_ENABLESLINE));

	/* IME Flag  -- special option */
	WriteOnOff(Section, "IME", FName, ts->UseIME);

	/* IME-inline Flag  -- special option */
	WriteOnOff(Section, "IMEInline", FName, ts->IMEInline);

	/* Kermit log  -- special option */
	WriteOnOff(Section, "KmtLog", FName, (WORD) (ts->LogFlag & LOG_KMT));

	// Enable language selection -- special option
	if ((ts->MenuFlag & MF_NOLANGUAGE) == 0)
		WriteOnOff(Section, "LanguageSelection", FName, 1);
	else
		WriteOnOff(Section, "LanguageSelection", FName, 0);

	/* Maximum scroll buffer size  -- special option */
	WriteInt(Section, "MaxBuffSize", FName, ts->ScrollBuffMax);

	/* Max com port number -- special option */
	WriteInt(Section, "MaxComPort", FName, ts->MaxComPort);

	/* Non-blinking cursor -- special option */
	WriteOnOff(Section, "NonblinkingCursor", FName, ts->NonblinkingCursor);

	WriteOnOff(Section, "KillFocusCursor", FName, ts->KillFocusCursor);

	/* Delay for pass-thru printing activation */
	/*   -- special option */
	WriteUint(Section, "PassThruDelay", FName, ts->PassThruDelay);

	/* Printer port for pass-thru printing */
	/*   -- special option */
	WritePrivateProfileString(Section, "PassThruPort", ts->PrnDev, FName);

	/* Printer Font --- special option */
	WriteFont(Section, "PrnFont", FName,
	          ts->PrnFont, ts->PrnFontSize.x, ts->PrnFontSize.y,
	          ts->PrnFontCharSet);

	// Page margins (left, right, top, bottom) for printing
	//    -- special option
	WriteInt4(Section, "PrnMargin", FName,
	          ts->PrnMargin[0], ts->PrnMargin[1],
	          ts->PrnMargin[2], ts->PrnMargin[3]);

	/* Quick-VAN log  -- special option */
	WriteOnOff(Section, "QVLog", FName, (WORD) (ts->LogFlag & LOG_QV));

	/* Quick-VAN window size -- special */
	WriteInt(Section, "QVWinSize", FName, ts->QVWinSize);

	/* Russian character set (print) -- special option */
	id2str(RussList, ts->RussPrint, IdWindows, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "RussPrint", Temp, FName);

	/* Scroll threshold -- special option */
	WriteInt(Section, "ScrollThreshold", FName, ts->ScrollThreshold);

	WriteInt(Section, "MouseWheelScrollLine", FName, ts->MouseWheelScrollLine);

	// Select on activate -- special option
	WriteOnOff(Section, "SelectOnActivate", FName, ts->SelOnActive);

	/* Send 8bit control sequence -- special option */
	WriteOnOff(Section, "Send8BitCtrl", FName, ts->Send8BitCtrl);

	/* Startup macro -- special option */
	WritePrivateProfileString(Section, "StartupMacro", ts->MacroFN, FName);

	/* TEK GIN Mouse keycode -- special option */
	WriteInt(Section, "TEKGINMouseCode", FName, ts->GINMouseCode);

	/* Telnet Auto Detect -- special option */
	WriteOnOff(Section, "TelAutoDetect", FName, ts->TelAutoDetect);

	/* Telnet binary flag -- special option */
	WriteOnOff(Section, "TelBin", FName, ts->TelBin);

	/* Telnet Echo flag -- special option */
	WriteOnOff(Section, "TelEcho", FName, ts->TelEcho);

	/* Telnet log  -- special option */
	WriteOnOff(Section, "TelLog", FName, (WORD) (ts->LogFlag & LOG_TEL));

	/* TCP port num for telnet -- special option */
	WriteUint(Section, "TelPort", FName, ts->TelPort);

	/* Telnet keep-alive packet(NOP command) interval -- special option */
	WriteUint(Section, "TelKeepAliveInterval", FName,
	          ts->TelKeepAliveInterval);

	/* Max number of broadcast commad history */
	WriteUint(Section, "MaxBroadcatHistory", FName,
	          ts->MaxBroadcatHistory);

	/* Local echo for non-telnet */
	WriteOnOff(Section, "TCPLocalEcho", FName, ts->TCPLocalEcho);

	/* "new-line (transmit)" option for non-telnet -- special option */
	if (ts->TCPCRSend == IdCRLF)
		strncpy_s(Temp, sizeof(Temp), "CRLF", _TRUNCATE);
	else if (ts->TCPCRSend == IdCR)
		strncpy_s(Temp, sizeof(Temp), "CR", _TRUNCATE);
	else
		Temp[0] = 0;
	WritePrivateProfileString(Section, "TCPCRSend", Temp, FName);

	/* Use text (background) color for "white (black)"
	   --- special option */
	WriteOnOff(Section, "UseTextColor", FName,
	           (WORD) (ts->ColorFlag & CF_USETEXTCOLOR));

	/* Title format -- special option */
	WriteUint(Section, "TitleFormat", FName, ts->TitleFormat);

	/* VT Compatible Tab -- special option */
	WriteOnOff(Section, "VTCompatTab", FName, ts->VTCompatTab);

	/* VT Font space --- special option */
	WriteInt4(Section, "VTFontSpace", FName,
	          ts->FontDX, ts->FontDW - ts->FontDX,
	          ts->FontDY, ts->FontDH - ts->FontDY);

	// VT-print scaling factors (pixels per inch) --- special option
	WriteInt2(Section, "VTPPI", FName, ts->VTPPI.x, ts->VTPPI.y);

	// TEK-print scaling factors (pixels per inch) --- special option
	WriteInt2(Section, "TEKPPI", FName, ts->TEKPPI.x, ts->TEKPPI.y);

	// Show "Window" menu -- special option
	WriteOnOff(Section, "WindowMenu", FName,
	           (WORD) (ts->MenuFlag & MF_SHOWWINMENU));

	/* XMODEM log  -- special option */
	WriteOnOff(Section, "XmodemLog", FName, (WORD) (ts->LogFlag & LOG_X));

	/* YMODEM log  -- special option */
	WriteOnOff(Section, "YmodemLog", FName, (WORD) (ts->LogFlag & LOG_Y));

	/* Auto ZMODEM activation -- special option */
	WriteOnOff(Section, "ZmodemAuto", FName,
	           (WORD) (ts->FTFlag & FT_ZAUTO));

	/* ZMODEM data subpacket length for sending -- special */
	WriteInt(Section, "ZmodemDataLen", FName, ts->ZmodemDataLen);
	/* ZMODEM window size for sending -- special */
	WriteInt(Section, "ZmodemWinSize", FName, ts->ZmodemWinSize);

	/* ZMODEM ESCCTL flag  -- special option */
	WriteOnOff(Section, "ZmodemEscCtl", FName,
	           (WORD) (ts->FTFlag & FT_ZESCCTL));

	/* ZMODEM log  -- special option */
	WriteOnOff(Section, "ZmodemLog", FName, (WORD) (ts->LogFlag & LOG_Z));

	/* ZMODEM 受信コマンド (2007.12.21 yutaka) */
	WritePrivateProfileString(Section, "ZmodemRcvCommand", ts->ZModemRcvCommand, FName);

	/* update file */
	WritePrivateProfileString(NULL, NULL, NULL, FName);

	// Eterm lookfeel alphablend (2005.4.24 yutaka)
#define ETERM_SECTION "BG"
	WriteOnOff(ETERM_SECTION, "BGEnable", FName,
	           ts->EtermLookfeel.BGEnable);
	WriteOnOff(ETERM_SECTION, "BGUseAlphaBlendAPI", FName,
	           ts->EtermLookfeel.BGUseAlphaBlendAPI);
	WritePrivateProfileString(ETERM_SECTION, "BGSPIPath",
	                          ts->EtermLookfeel.BGSPIPath, FName);
	WriteOnOff(ETERM_SECTION, "BGFastSizeMove", FName,
	           ts->EtermLookfeel.BGFastSizeMove);
	WriteOnOff(ETERM_SECTION, "BGFlickerlessMove", FName,
	           ts->EtermLookfeel.BGNoCopyBits);
	WriteOnOff(ETERM_SECTION, "BGNoFrame", FName,
	           ts->EtermLookfeel.BGNoFrame);
	WritePrivateProfileString(ETERM_SECTION, "BGThemeFile",
	                          ts->EtermLookfeel.BGThemeFile, FName);

#ifdef USE_NORMAL_BGCOLOR
	// UseNormalBGColor
	WriteOnOff(Section, "UseNormalBGColor", FName, ts->UseNormalBGColor);
#endif

	// UI language message file
	WritePrivateProfileString(Section, "UILanguageFile",
	                          ts->UILanguageFile_ini, FName);

	// Broadcast Command History (2007.3.3 maya)
	WriteOnOff(Section, "BroadcastCommandHistory", FName,
	           ts->BroadcastCommandHistory);

	// 337: 2007/03/20 Accept Broadcast
	WriteOnOff(Section, "AcceptBroadcast", FName, ts->AcceptBroadcast);

	// Confirm send a file when drag and drop (2007.12.28 maya)
	WriteOnOff(Section, "ConfirmFileDragAndDrop", FName,
	           ts->ConfirmFileDragAndDrop);

	// Translate mouse wheel to cursor key when application cursor mode
	WriteOnOff(Section, "TranslateWheelToCursor", FName,
	           ts->TranslateWheelToCursor);

	// Display "New Connection" dialog on startup (2008.1.18 maya)
	WriteOnOff(Section, "HostDialogOnStartup", FName,
	           ts->HostDialogOnStartup);

	// Mouse event tracking
	WriteOnOff(Section, "MouseEventTracking", FName,
	           ts->MouseEventTracking);

	// Maximized bug tweak
	WriteOnOff(Section, "MaximizedBugTweak", FName, ts->MaximizedBugTweak);

	// Convert Unicode symbol characters to DEC Special characters
	WriteUint(Section, "UnicodeToDecSpMapping", FName, ts->UnicodeDecSpMapping);

	// VT Window Icon
	IconId2IconName(Temp, sizeof(Temp), ts->VTIcon);
	WritePrivateProfileString(Section, "VTIcon", Temp, FName);

	// Tek Window Icon
	IconId2IconName(Temp, sizeof(Temp), ts->TEKIcon);
	WritePrivateProfileString(Section, "TEKIcon", Temp, FName);

	// AutoScrollOnlyInBottomLine
	WriteOnOff(Section, "AutoScrollOnlyInBottomLine", FName,
		ts->AutoScrollOnlyInBottomLine);

	// Unknown Unicode Character
	WriteOnOff(Section, "UnknownUnicodeCharacterAsWide", FName,
	           ts->UnknownUnicodeCharaAsWide);

	// Accept remote-controlled window title changing
	if (ts->AcceptTitleChangeRequest == IdTitleChangeRequestOff)
		strncpy_s(Temp, sizeof(Temp), "off", _TRUNCATE);
	else if (ts->AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite)
		strncpy_s(Temp, sizeof(Temp), "overwrite", _TRUNCATE);
	else if (ts->AcceptTitleChangeRequest == IdTitleChangeRequestAhead)
		strncpy_s(Temp, sizeof(Temp), "ahead", _TRUNCATE);
	else if (ts->AcceptTitleChangeRequest == IdTitleChangeRequestLast)
		strncpy_s(Temp, sizeof(Temp), "last", _TRUNCATE);
	else
		Temp[0] = 0;
	WritePrivateProfileString(Section, "AcceptTitleChangeRequest", Temp, FName);

	// Size of paste confirm dialog
	WriteInt2(Section, "PasteDialogSize", FName,
	          ts->PasteDialogSize.cx, ts->PasteDialogSize.cy);

	// Disable mouse event tracking when Control-Key is pressed.
	WriteOnOff(Section, "DisableMouseTrackingByCtrl", FName,
	           ts->DisableMouseTrackingByCtrl);

	// Disable TranslateWHeelToCursor when Control-Key is pressed.
	WriteOnOff(Section, "DisableWheelToCursorByCtrl", FName,
	           ts->DisableWheelToCursorByCtrl);

	// Strict Key Mapping.
	WriteOnOff(Section, "StrictKeyMapping", FName,
	           ts->StrictKeyMapping);

	// Wait4allMacroCommand
	WriteOnOff(Section, "Wait4allMacroCommand", FName,
	           ts->Wait4allMacroCommand);

	// DisableMenuSendBreak
	WriteOnOff(Section, "DisableMenuSendBreak", FName,
	           ts->DisableMenuSendBreak);

	// ClearScreenOnCloseConnection
	WriteOnOff(Section, "ClearScreenOnCloseConnection", FName,
	           ts->ClearScreenOnCloseConnection);

	// DisableAcceleratorDuplicateSession
	WriteOnOff(Section, "DisableAcceleratorDuplicateSession", FName,
	           ts->DisableAcceleratorDuplicateSession);

	// added PasteDelayPerLine (2009.4.12 maya)
	WriteInt(Section, "PasteDelayPerLine", FName,
	         ts->PasteDelayPerLine);
}

#define VTEditor "VT editor keypad"
#define VTNumeric "VT numeric keypad"
#define VTFunction "VT function keys"
#define XFunction "X function keys"
#define ShortCut "Shortcut keys"

void GetInt(PKeyMap KeyMap, int KeyId, PCHAR Sect, PCHAR Key, PCHAR FName)
{
	char Temp[11];
	WORD Num;

	GetPrivateProfileString(Sect, Key, "", Temp, sizeof(Temp), FName);
	if (Temp[0] == 0)
		Num = 0xFFFF;
	else if (_stricmp(Temp, "off") == 0)
		Num = 0xFFFF;
	else if (sscanf(Temp, "%d", &Num) != 1)
		Num = 0xFFFF;

	KeyMap->Map[KeyId - 1] = Num;
}

void FAR PASCAL ReadKeyboardCnf
	(PCHAR FName, PKeyMap KeyMap, BOOL ShowWarning) {
	int i, j, Ptr;
	char EntName[7];
	char TempStr[221];
	char KStr[201];

	// clear key map
	for (i = 0; i <= IdKeyMax - 1; i++)
		KeyMap->Map[i] = 0xFFFF;
	for (i = 0; i <= NumOfUserKey - 1; i++) {
		KeyMap->UserKeyPtr[i] = 0;
		KeyMap->UserKeyLen[i] = 0;
	}

	// VT editor keypad
	GetInt(KeyMap, IdUp, VTEditor, "Up", FName);

	GetInt(KeyMap, IdDown, VTEditor, "Down", FName);

	GetInt(KeyMap, IdRight, VTEditor, "Right", FName);

	GetInt(KeyMap, IdLeft, VTEditor, "Left", FName);

	GetInt(KeyMap, IdFind, VTEditor, "Find", FName);

	GetInt(KeyMap, IdInsert, VTEditor, "Insert", FName);

	GetInt(KeyMap, IdRemove, VTEditor, "Remove", FName);

	GetInt(KeyMap, IdSelect, VTEditor, "Select", FName);

	GetInt(KeyMap, IdPrev, VTEditor, "Prev", FName);

	GetInt(KeyMap, IdNext, VTEditor, "Next", FName);

	// VT numeric keypad
	GetInt(KeyMap, Id0, VTNumeric, "Num0", FName);

	GetInt(KeyMap, Id1, VTNumeric, "Num1", FName);

	GetInt(KeyMap, Id2, VTNumeric, "Num2", FName);

	GetInt(KeyMap, Id3, VTNumeric, "Num3", FName);

	GetInt(KeyMap, Id4, VTNumeric, "Num4", FName);

	GetInt(KeyMap, Id5, VTNumeric, "Num5", FName);

	GetInt(KeyMap, Id6, VTNumeric, "Num6", FName);

	GetInt(KeyMap, Id7, VTNumeric, "Num7", FName);

	GetInt(KeyMap, Id8, VTNumeric, "Num8", FName);

	GetInt(KeyMap, Id9, VTNumeric, "Num9", FName);

	GetInt(KeyMap, IdMinus, VTNumeric, "NumMinus", FName);

	GetInt(KeyMap, IdComma, VTNumeric, "NumComma", FName);

	GetInt(KeyMap, IdPeriod, VTNumeric, "NumPeriod", FName);

	GetInt(KeyMap, IdEnter, VTNumeric, "NumEnter", FName);

	GetInt(KeyMap, IdSlash, VTNumeric, "NumSlash", FName);

	GetInt(KeyMap, IdAsterisk, VTNumeric, "NumAsterisk", FName);

	GetInt(KeyMap, IdPlus, VTNumeric, "NumPlus", FName);

	GetInt(KeyMap, IdPF1, VTNumeric, "PF1", FName);

	GetInt(KeyMap, IdPF2, VTNumeric, "PF2", FName);

	GetInt(KeyMap, IdPF3, VTNumeric, "PF3", FName);

	GetInt(KeyMap, IdPF4, VTNumeric, "PF4", FName);

	// VT function keys
	GetInt(KeyMap, IdHold, VTFunction, "Hold", FName);

	GetInt(KeyMap, IdPrint, VTFunction, "Print", FName);

	GetInt(KeyMap, IdBreak, VTFunction, "Break", FName);

	GetInt(KeyMap, IdF6, VTFunction, "F6", FName);

	GetInt(KeyMap, IdF7, VTFunction, "F7", FName);

	GetInt(KeyMap, IdF8, VTFunction, "F8", FName);

	GetInt(KeyMap, IdF9, VTFunction, "F9", FName);

	GetInt(KeyMap, IdF10, VTFunction, "F10", FName);

	GetInt(KeyMap, IdF11, VTFunction, "F11", FName);

	GetInt(KeyMap, IdF12, VTFunction, "F12", FName);

	GetInt(KeyMap, IdF13, VTFunction, "F13", FName);

	GetInt(KeyMap, IdF14, VTFunction, "F14", FName);

	GetInt(KeyMap, IdHelp, VTFunction, "Help", FName);

	GetInt(KeyMap, IdDo, VTFunction, "Do", FName);

	GetInt(KeyMap, IdF17, VTFunction, "F17", FName);

	GetInt(KeyMap, IdF18, VTFunction, "F18", FName);

	GetInt(KeyMap, IdF19, VTFunction, "F19", FName);

	GetInt(KeyMap, IdF20, VTFunction, "F20", FName);

	// UDK
	GetInt(KeyMap, IdUDK6, VTFunction, "UDK6", FName);

	GetInt(KeyMap, IdUDK7, VTFunction, "UDK7", FName);

	GetInt(KeyMap, IdUDK8, VTFunction, "UDK8", FName);

	GetInt(KeyMap, IdUDK9, VTFunction, "UDK9", FName);

	GetInt(KeyMap, IdUDK10, VTFunction, "UDK10", FName);

	GetInt(KeyMap, IdUDK11, VTFunction, "UDK11", FName);

	GetInt(KeyMap, IdUDK12, VTFunction, "UDK12", FName);

	GetInt(KeyMap, IdUDK13, VTFunction, "UDK13", FName);

	GetInt(KeyMap, IdUDK14, VTFunction, "UDK14", FName);

	GetInt(KeyMap, IdUDK15, VTFunction, "UDK15", FName);

	GetInt(KeyMap, IdUDK16, VTFunction, "UDK16", FName);

	GetInt(KeyMap, IdUDK17, VTFunction, "UDK17", FName);

	GetInt(KeyMap, IdUDK18, VTFunction, "UDK18", FName);

	GetInt(KeyMap, IdUDK19, VTFunction, "UDK19", FName);

	GetInt(KeyMap, IdUDK20, VTFunction, "UDK20", FName);

	// XTERM function keys
	GetInt(KeyMap, IdXF1, XFunction, "XF1", FName);

	GetInt(KeyMap, IdXF2, XFunction, "XF2", FName);

	GetInt(KeyMap, IdXF3, XFunction, "XF3", FName);

	GetInt(KeyMap, IdXF4, XFunction, "XF4", FName);

	GetInt(KeyMap, IdXF5, XFunction, "XF5", FName);

	// accelerator keys
	GetInt(KeyMap, IdCmdEditCopy, ShortCut, "EditCopy", FName);

	GetInt(KeyMap, IdCmdEditPaste, ShortCut, "EditPaste", FName);

	GetInt(KeyMap, IdCmdEditPasteCR, ShortCut, "EditPasteCR", FName);

	GetInt(KeyMap, IdCmdEditCLS, ShortCut, "EditCLS", FName);

	GetInt(KeyMap, IdCmdEditCLB, ShortCut, "EditCLB", FName);

	GetInt(KeyMap, IdCmdCtrlOpenTEK, ShortCut, "ControlOpenTEK", FName);

	GetInt(KeyMap, IdCmdCtrlCloseTEK, ShortCut, "ControlCloseTEK", FName);

	GetInt(KeyMap, IdCmdLineUp, ShortCut, "LineUp", FName);

	GetInt(KeyMap, IdCmdLineDown, ShortCut, "LineDown", FName);

	GetInt(KeyMap, IdCmdPageUp, ShortCut, "PageUp", FName);

	GetInt(KeyMap, IdCmdPageDown, ShortCut, "PageDown", FName);

	GetInt(KeyMap, IdCmdBuffTop, ShortCut, "BuffTop", FName);

	GetInt(KeyMap, IdCmdBuffBottom, ShortCut, "BuffBottom", FName);

	GetInt(KeyMap, IdCmdNextWin, ShortCut, "NextWin", FName);

	GetInt(KeyMap, IdCmdPrevWin, ShortCut, "PrevWin", FName);

	GetInt(KeyMap, IdCmdLocalEcho, ShortCut, "LocalEcho", FName);

	GetInt(KeyMap, IdScrollLock, ShortCut, "ScrollLock", FName);

	/* user keys */

	Ptr = 0;

	i = IdUser1;
	do {
		_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "User%d", i - IdUser1 + 1);
		GetPrivateProfileString("User keys", EntName, "",
								TempStr, sizeof(TempStr), FName);
		if (strlen(TempStr) > 0) {
			/* scan code */
			GetNthString(TempStr, 1, sizeof(KStr), KStr);
			if (_stricmp(KStr, "off") == 0)
				KeyMap->Map[i - 1] = 0xFFFF;
			else {
				GetNthNum(TempStr, 1, &j);
				KeyMap->Map[i - 1] = (WORD) j;
			}
			/* conversion flag */
			GetNthNum(TempStr, 2, &j);
			KeyMap->UserKeyType[i - IdUser1] = (BYTE) j;
			/* key string */
/*	GetNthString(TempStr,3,sizeof(KStr),KStr); */
			KeyMap->UserKeyPtr[i - IdUser1] = Ptr;
/*	KeyMap->UserKeyLen[i-IdUser1] =
	Hex2Str(KStr,&(KeyMap->UserKeyStr[Ptr]),KeyStrMax-Ptr+1);
*/
			GetNthString(TempStr, 3, KeyStrMax - Ptr + 1,
			             &(KeyMap->UserKeyStr[Ptr]));
			KeyMap->UserKeyLen[i - IdUser1] =
				strlen(&(KeyMap->UserKeyStr[Ptr]));
			Ptr = Ptr + KeyMap->UserKeyLen[i - IdUser1];
		}

		i++;
	}
	while ((i <= IdKeyMax) && (strlen(TempStr) > 0) && (Ptr <= KeyStrMax));

	for (j = 1; j <= IdKeyMax - 1; j++)
		if (KeyMap->Map[j] != 0xFFFF)
			for (i = 0; i <= j - 1; i++)
				if (KeyMap->Map[i] == KeyMap->Map[j]) {
					if (ShowWarning) {
						_snprintf_s(TempStr, sizeof(TempStr), _TRUNCATE,
						            "Keycode %d is used more than once",
						            KeyMap->Map[j]);
						MessageBox(0, TempStr,
						           "Tera Term: Error in keyboard setup file",
						           MB_ICONEXCLAMATION);
					}
					KeyMap->Map[i] = 0xFFFF;
				}
}

void FAR PASCAL CopySerialList(PCHAR IniSrc, PCHAR IniDest, PCHAR section,
                               PCHAR key, int MaxList)
{
	int i;
	char EntName[10];
	char TempHost[HostNameMaxLength + 1];

	if (_stricmp(IniSrc, IniDest) == 0)
		return;

	WritePrivateProfileString(section, NULL, NULL, IniDest);

	i = 1;
	do {
		_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "%s%i", key, i);

		/* Get one hostname from file IniSrc */
		GetPrivateProfileString(section, EntName, "",
		                        TempHost, sizeof(TempHost), IniSrc);
		/* Copy it to the file IniDest */
		if (strlen(TempHost) > 0)
			WritePrivateProfileString(section, EntName, TempHost, IniDest);
		i++;
	}
	while ((i <= MaxList) && (strlen(TempHost) > 0));

	/* update file */
	WritePrivateProfileString(NULL, NULL, NULL, IniDest);
}

void FAR PASCAL AddValueToList(PCHAR FName, PCHAR Host, PCHAR section,
                               PCHAR key, int MaxList)
{
	HANDLE MemH;
	PCHAR MemP;
	char EntName[13];
	int i, j, Len;
	BOOL Update;

	if ((FName[0] == 0) || (Host[0] == 0))
		return;
	MemH = GlobalAlloc(GHND, (HostNameMaxLength + 1) * MaxList);
	if (MemH == NULL)
		return;
	MemP = GlobalLock(MemH);
	if (MemP != NULL) {
		strncpy_s(MemP, (HostNameMaxLength + 1) * MaxList, Host, _TRUNCATE);
		j = strlen(Host) + 1;
		i = 1;
		Update = TRUE;
		do {
			_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "%s%i", key, i);

			/* Get a hostname */
			GetPrivateProfileString(section, EntName, "",
			                        &MemP[j], HostNameMaxLength + 1,
			                        FName);
			Len = strlen(&MemP[j]);
			if (_stricmp(&MemP[j], Host) == 0) {
				if (i == 1)
					Update = FALSE;
			}
			else
				j = j + Len + 1;
			i++;
		} while ((i <= MaxList) && (Len != 0) && Update);

		if (Update) {
			WritePrivateProfileString(section, NULL, NULL, FName);

			j = 0;
			i = 1;
			do {
				_snprintf_s(EntName, sizeof(EntName), _TRUNCATE, "%s%i", key, i);

				if (MemP[j] != 0)
					WritePrivateProfileString(section, EntName, &MemP[j],
					                          FName);
				j = j + strlen(&MemP[j]) + 1;
				i++;
			} while ((i <= MaxList) && (MemP[j] != 0));
			/* update file */
			WritePrivateProfileString(NULL, NULL, NULL, FName);
		}
		GlobalUnlock(MemH);
	}
	GlobalFree(MemH);
}

 /* copy hostlist from source IniFile to dest IniFile */
void FAR PASCAL CopyHostList(PCHAR IniSrc, PCHAR IniDest)
{
	CopySerialList(IniSrc, IniDest, "Hosts", "Host", MAXHOSTLIST);
}

void FAR PASCAL AddHostToList(PCHAR FName, PCHAR Host)
{
	AddValueToList(FName, Host, "Hosts", "Host", MAXHOSTLIST);
}

BOOL NextParam(PCHAR Param, int *i, PCHAR Temp, int Size)
{
	int j;
	char c;
	BOOL Quoted;

	if ((unsigned int) (*i) >= strlen(Param))
		return FALSE;
	j = 0;

	while (Param[*i] == ' ')
		(*i)++;

	Quoted = FALSE;
	c = Param[*i];
	(*i)++;
	while ((c != 0) && (Quoted || (c != ' ')) &&
	       (Quoted || (c != ';')) && (j < Size - 1)) {
		if (c == '"')
			Quoted = !Quoted;
		Temp[j] = c;
		j++;
		c = Param[*i];
		(*i)++;
	}
	if (!Quoted && (c == ';'))
		(*i)--;

	Temp[j] = 0;
	return (strlen(Temp) > 0);
}

void Dequote(PCHAR Source, PCHAR Dest)
{
	int i, j;
	char q, c;

	Dest[0] = 0;
	if (Source[0] == 0)
		return;
	i = 0;
	/* quoting char */
	q = Source[i];
	/* only '"' is used as quoting char */
	if (q != '"')
		q = 0;
	else
		i++;

	c = Source[i];
	i++;
	j = 0;
	while ((c != 0) && (c != q)) {
		Dest[j] = c;
		j++;
		c = Source[i];
		i++;
	}

	Dest[j] = 0;
}


#ifndef NO_INET6
static void ParseHostName(char *HostStr, WORD * port)
{
	/*
	 * hostname.domain.com
	 * hostname.domain.com:23
	 * [3ffe:1234:1234::1]     IPv6 raw address
	 * [3ffe:1234:1234::1]:23  IPv6 raw address and port#
	 * telnet://hostname.domain.com/
	 * telnet://hostname.domain.com:23/
	 * telnet://[3ffe:1234:1234::1]/
	 * telnet://[3ffe:1234:1234::1]:23/
	 * tn3270:// .... /
	 */

	int i, is_telnet_handler = 0, is_port = 0;
	char *s;
	char b;

	/* strlen("telnet://") == 9 */
	if ((_strnicmp(HostStr, "telnet://", 9) == 0) ||
		(_strnicmp(HostStr, "tn3270://", 9) == 0)) {
		/* trim "telnet://" and tail "/" */
		memmove(HostStr, &(HostStr[9]), strlen(HostStr) - 8);
		i = strlen(HostStr);
		if (i > 0 && (HostStr[i - 1] == '/'))
			HostStr[i - 1] = '\0';
		is_telnet_handler = 1;
	}

	/* parsing string enclosed by [ ] */
	s = HostStr;
	if (*s == '[') {
		BOOL inet6found = FALSE;
		s++;
		while (*s) {
			if (*s == ']') {
				/* found IPv6 raw address */
				/* triming [ ] */
				int len = strlen(HostStr);
				char *lastptr = &HostStr[len - 1];
				memmove(HostStr, HostStr + 1, len - 1);
				s = s - 1;
				lastptr = lastptr - 1;
				memmove(s, s + 1, lastptr - s);
				/* because of triming 2 characters */
				HostStr[len - 2] = '\0';


				inet6found = TRUE;
				break;
			}
			s++;
		}
		if (inet6found == FALSE)
			s = HostStr;
	}

	/* parsing port# */
	/*
	 * s points:
	 *   [3ffe:1234:1234::1]:XXX....
	 *   3ffe:1234:1234::1:XXX....
	 *                    |
	 *                    s
	 *
	 *   hostname.domain.com
	 *   |
	 *   s
	 */
	i = 0;
	do {
		b = s[i];
		i++;
	} while (b != '\0' && b != ':');
	if (b == ':') {
		s[i - 1] = '\0';
		if (sscanf(&(s[i]), "%d", port) != 1)
			*port = 65535;
		is_port = 1;
	}
	if (is_telnet_handler == 1 && is_port == 0) {
		*port = 23;
	}
}
#endif							/* NO_INET6 */


void FAR PASCAL ParseParam(PCHAR Param, PTTSet ts, PCHAR DDETopic)
{
	int i, pos, c;
#ifdef NO_INET6
	BYTE b;
#endif							/* NO_INET6 */
	char Temp[MAXPATHLEN + 3];
	char Temp2[MAXPATHLEN];
	char TempDir[MAXPATHLEN];
	WORD ParamPort = 0;
	WORD ParamCom = 0;
	WORD ParamTCP = 65535;
	WORD ParamTel = 2;
	WORD ParamBin = 2;
	WORD ParamBaud = IdBaudNone;
	BOOL HostNameFlag = FALSE;
	BOOL JustAfterHost = FALSE;
	WORD DisableTCPEchoCR = FALSE;

	ts->HostName[0] = 0;
	ts->KeyCnfFN[0] = 0;
	/* Set AutoConnect true as default (2008.2.16 by steven)*/
	ts->ComAutoConnect = TRUE;

#ifndef NO_INET6
	/* user specifies the protocol connecting to the host */
	/* ts->ProtocolFamily = AF_UNSPEC; */
#endif							/* NO_INET6 */

	/* Get command line parameters */
	if (DDETopic != NULL)
		DDETopic[0] = 0;
	i = 0;
	/* the first term shuld be executable filename of Tera Term */
	NextParam(Param, &i, Temp, sizeof(Temp));
	while (NextParam(Param, &i, Temp, sizeof(Temp))) {
		if (HostNameFlag) {
			JustAfterHost = TRUE;
			HostNameFlag = FALSE;
		}

		if (_strnicmp(Temp, "/BAUD=", 6) == 0) {	/* Serial port baud rate */
			ParamPort = IdSerial;
			ParamBaud = str2id(BaudList, &Temp[6], IdBaudNone);
		}
		else if (_strnicmp(Temp, "/B", 2) == 0) {	/* telnet binary */
			ParamPort = IdTCPIP;
			ParamBin = 1;
		}
		else if (_strnicmp(Temp, "/C=", 3) == 0) {	/* COM port num */
			ParamPort = IdSerial;
			ParamCom = atoi(&Temp[3]);
			if ((ParamCom < 1) || (ParamCom > ts->MaxComPort))
				ParamCom = 0;
		}
		else if (_strnicmp(Temp, "/D=", 3) == 0) {
			if (DDETopic != NULL)
				strncpy_s(DDETopic, 21, &Temp[3], _TRUNCATE);	// 21 = sizeof(TopicName)
		}
		// "New connection" ダイアログを表示しない (2008.11.14 maya)
		else if (_strnicmp(Temp, "/DS", 3) == 0) {
			ts->HostDialogOnStartup = FALSE;
		}
		// TCPLocalEcho/TCPCRSend を無効にする (maya 2007.4.25)
		else if (_strnicmp(Temp, "/E", 2) == 0) {
			DisableTCPEchoCR = TRUE;
		}
		else if (_strnicmp(Temp, "/F=", 3) == 0) {	/* setup filename */
			Dequote(&Temp[3], Temp2);
			if (strlen(Temp2) > 0) {
				ConvFName(ts->HomeDir, Temp2, sizeof(Temp2), ".INI", Temp,
				          sizeof(Temp));
				if (_stricmp(ts->SetupFName, Temp) != 0) {
					strncpy_s(ts->SetupFName, sizeof(ts->SetupFName), Temp,
					          _TRUNCATE);
					ReadIniFile(ts->SetupFName, ts);
				}
			}
		}
		else if (_strnicmp(Temp, "/FD=", 4) == 0) {	/* file transfer directory */
			Dequote(&Temp[4], Temp2);
			if (strlen(Temp2) > 0) {
				_getcwd(TempDir, sizeof(TempDir));
				if (_chdir(Temp2) == 0)
					strncpy_s(ts->FileDir, sizeof(ts->FileDir), Temp2,
					          _TRUNCATE);
				_chdir(TempDir);
			}
		}
		else if (_strnicmp(Temp, "/H", 2) == 0)	/* hide title bar */
			ts->HideTitle = 1;
		else if (_strnicmp(Temp, "/I", 2) == 0)	/* iconize */
			ts->Minimize = 1;
		else if (_strnicmp(Temp, "/K=", 3) == 0) {	/* Keyboard setup file */
			Dequote(&Temp[3], Temp2);
			ConvFName(ts->HomeDir, Temp2, sizeof(Temp2), ".CNF",
			          ts->KeyCnfFN, sizeof(ts->KeyCnfFN));
		}
		else if ((_strnicmp(Temp, "/KR=", 4) == 0) ||
		         (_strnicmp(Temp, "/KT=", 4) == 0)) {	/* kanji code */
			if (_strnicmp(&Temp[4], "UTF8m",  5) == 0 ||
			    _strnicmp(&Temp[4], "UTF-8m", 6) == 0)
				c = IdUTF8m;
			else if (_strnicmp(&Temp[4], "UTF8",  4) == 0 ||
			         _strnicmp(&Temp[4], "UTF-8", 5) == 0)
				c = IdUTF8;
			else if (_strnicmp(&Temp[4], "SJIS", 4) == 0)
				c = IdSJIS;
			else if (_strnicmp(&Temp[4], "EUC", 3) == 0)
				c = IdEUC;
			else if (_strnicmp(&Temp[4], "JIS", 3) == 0)
				c = IdJIS;
			else
				c = -1;
			if (c != -1) {
				if (_strnicmp(Temp, "/KR=", 4) == 0)
					ts->KanjiCode = c;
				else
					ts->KanjiCodeSend = c;
			}
		}
		else if (_strnicmp(Temp, "/L=", 3) == 0) {	/* log file */
			Dequote(&Temp[3], Temp2);
			strncpy_s(ts->LogFN, sizeof(ts->LogFN), Temp2, _TRUNCATE);
		}
		else if (_strnicmp(Temp, "/LA=", 4) == 0) {	/* language */
			switch (Temp[4]) {
			  case 'E':
			  case 'e':
				ts->Language = IdEnglish; break;
			  case 'J':
			  case 'j':
				ts->Language = IdJapanese; break;
			  case 'K':
			  case 'k':
				ts->Language = IdKorean; break;
			  case 'R':
			  case 'r':
				ts->Language = IdRussian; break;
			}
		}
		else if (_strnicmp(Temp, "/M=", 3) == 0) {	/* macro filename */
			if ((Temp[3] == 0) || (Temp[3] == '*'))
				strncpy_s(ts->MacroFN, sizeof(ts->MacroFN), "*",
				          _TRUNCATE);
			else {
				Dequote(&Temp[3], Temp2);
				ConvFName(ts->HomeDir, Temp2, sizeof(Temp2), ".TTL",
				          ts->MacroFN, sizeof(ts->MacroFN));
			}
			/* Disable auto connect to serial when macro mode (2006.9.15 maya) */
			ts->ComAutoConnect = FALSE;
		}
		else if (_strnicmp(Temp, "/M", 2) == 0) {	/* macro option without file name */
			strncpy_s(ts->MacroFN, sizeof(ts->MacroFN), "*", _TRUNCATE);
			/* Disable auto connect to serial when macro mode (2006.9.15 maya) */
			ts->ComAutoConnect = FALSE;
		}
		else if (_strnicmp(Temp, "/P=", 3) == 0) {	/* TCP port num */
			ParamPort = IdTCPIP;
			if (sscanf(&Temp[3], "%d", &ParamTCP) != 1)
				ParamTCP = 65535;
		}
		else if (_strnicmp(Temp, "/R=", 3) == 0) {	/* Replay filename */
			Dequote(&Temp[3], Temp2);
			ConvFName(ts->HomeDir, Temp2, sizeof(Temp2), "", ts->HostName,
			          sizeof(ts->HostName));
			if (strlen(ts->HostName) > 0)
				ParamPort = IdFile;
		}
		else if (_strnicmp(Temp, "/T=0", 4) == 0) {	/* telnet disable */
			ParamPort = IdTCPIP;
			ParamTel = 0;
		}
		else if (_strnicmp(Temp, "/T=1", 4) == 0) {	/* telnet enable */
			ParamPort = IdTCPIP;
			ParamTel = 1;
		}
		else if (_strnicmp(Temp, "/TEKICON=", 9) == 0) { /* Tek window icon */
			ts->TEKIcon = IconName2IconId(&Temp[9]);
		}
		else if (_strnicmp(Temp, "/VTICON=", 8) == 0) {	/* VT window icon */
			ts->VTIcon = IconName2IconId(&Temp[8]);
		}
		else if (_strnicmp(Temp, "/V", 2) == 0) {	/* invisible */
			ts->HideWindow = 1;
		}
		else if (_strnicmp(Temp, "/W=", 3) == 0) {	/* Window title */
			Dequote(&Temp[3], ts->Title);
		}
		else if (_strnicmp(Temp, "/X=", 3) == 0) {	/* Window pos (X) */
			if (sscanf(&Temp[3], "%d", &pos) == 1) {
				ts->VTPos.x = pos;
				if (ts->VTPos.y == CW_USEDEFAULT)
					ts->VTPos.y = 0;
			}
		}
		else if (_strnicmp(Temp, "/Y=", 3) == 0) {	/* Window pos (Y) */
			if (sscanf(&Temp[3], "%d", &pos) == 1) {
				ts->VTPos.y = pos;
				if (ts->VTPos.x == CW_USEDEFAULT)
					ts->VTPos.x = 0;
			}
		}
#ifndef NO_INET6
		else if (_strnicmp(Temp, "/4", 2) == 0)	/* Protocol Tera Term speaking */
			ts->ProtocolFamily = AF_INET;
		else if (_strnicmp(Temp, "/6", 2) == 0)
			ts->ProtocolFamily = AF_INET6;
#endif
		else if (_strnicmp(Temp, "/DUPLICATE", 9) == 0) {	// duplicate session (2004.12.7. yutaka)
			ts->DuplicateSession = 1;

		}
		else if (_strnicmp(Temp, "/TIMEOUT=", 9) == 0) {	// Connecting Timeout value (2007.1.11. yutaka)
			if (sscanf(&Temp[9], "%d", &pos) == 1) {
				if (pos >= 0)
					ts->ConnectingTimeout = pos;
			}

		}
		else if ((Temp[0] != '/') && (strlen(Temp) > 0)) {
			if (JustAfterHost && (sscanf(Temp, "%d", &c) == 1))
				ParamTCP = c;
			else {
				ParamPort = IdTCPIP;
				strncpy_s(ts->HostName, sizeof(ts->HostName), Temp, _TRUNCATE);	/* host name */
				HostNameFlag = TRUE;
			}
		}
		JustAfterHost = FALSE;
	}

	if ((DDETopic != NULL) && (DDETopic[0] != 0))
		ts->MacroFN[0] = 0;

	if ((ts->HostName[0] != 0) && (ParamPort == IdTCPIP)) {
#ifndef NO_INET6
		ParseHostName(ts->HostName, &ParamTCP);
#else							/* NO_INET6 */
		if ((_strnicmp(ts->HostName, "telnet://", 9) == 0) ||
		    (_strnicmp(ts->HostName, "tn3270://", 9) == 0)) {
			memmove(ts->HostName, &(ts->HostName[9]),
			        strlen(ts->HostName) - 8);
			i = strlen(ts->HostName);
			if ((i > 0) && (ts->HostName[i - 1] == '/'))
				ts->HostName[i - 1] = 0;
		}
		i = 0;
		do {
			b = ts->HostName[i];
			i++;
		} while ((b != 0) && (b != ':'));
		if (b == ':') {
			ts->HostName[i - 1] = 0;
			if (sscanf(&(ts->HostName[i]), "%d", &ParamTCP) != 1)
				ParamTCP = 65535;
		}
#endif							/* NO_INET6 */
	}

	switch (ParamPort) {
	case IdTCPIP:
		ts->PortType = IdTCPIP;
		if (ParamTCP < 65535)
			ts->TCPPort = ParamTCP;
		if (ParamTel < 2)
			ts->Telnet = ParamTel;
		if (ParamBin < 2)
			ts->TelBin = ParamBin;
		break;
	case IdSerial:
		ts->PortType = IdSerial;
		if (ParamCom > 0) {
			ts->ComPort = ParamCom;
 		    /* Don't display new connection dialog if COM port is specified explicitly (2006.9.15 maya) */ 
			ts->ComAutoConnect = TRUE; 
		}
		if (ParamBaud != IdBaudNone)
			ts->Baud = ParamBaud;
		break;
	case IdFile:
		ts->PortType = IdFile;
	}

	// TCPLocalEcho/TCPCRSend を無効にする (maya 2007.4.25)
	if (DisableTCPEchoCR == TRUE) {
		ts->DisableTCPEchoCR = TRUE;
	}
}

BOOL WINAPI DllMain(HANDLE hInst,
                    ULONG ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_THREAD_ATTACH:
		/* do thread initialization */
		break;
	case DLL_THREAD_DETACH:
		/* do thread cleanup */
		break;
	case DLL_PROCESS_ATTACH:
		/* do process initialization */
		DoCover_IsDebuggerPresent();
		break;
	case DLL_PROCESS_DETACH:
		/* do process cleanup */
		break;
	}
	return TRUE;
}
