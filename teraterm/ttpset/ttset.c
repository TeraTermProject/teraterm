/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004- TeraTerm Project
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
/* IPv6 modification is Copyright(C) 2000 Jun-ya kato <kato@win6.jp> */

/* TTSET.DLL, setup file routines*/
#include <winsock2.h>
#include <ws2tcpip.h>
#include "teraterm.h"
#include "tttypes.h"
#include "tttypes_charset.h"
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <errno.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "tt-version.h"
#include "ttlib.h"
#include "ttlib_types.h"
#include "tt_res.h"
#include "servicenames.h"
#include "codeconv.h"
#include "win32helper.h"
#include "inifile_com.h"
#include "ttlib_charset.h"
#include "asprintf.h"
#include "compat_win.h"
#include "vtdisp.h"
#include "makeoutputstring.h"
#include "history_store.h"
#include "xmodem.h"
#include "ymodem.h"
#include "kermit.h"

#define DllExport __declspec(dllexport)
#include "ttset.h"

#define Section "Tera Term"
#define SectionW L"Tera Term"
#define BG_SECTION "BG"
#define BG_SECTIONW L"BG"

#define MaxStrLen (LONG)512

static const PCHAR TermList[] =
	{ "VT100", "VT100J", "VT101", "VT102", "VT102J", "VT220J", "VT282",
	"VT320", "VT382", "VT420", "VT520", "VT525", NULL };

static const PCHAR RussList2[] = { "Windows", "KOI8-R", NULL };


/*
 * �V���A���|�[�g�֘A�̐ݒ��`
 */
#define IDENDMARK 0xFFFF

typedef struct id_str_pair {
	WORD id;
	const char *str;
} id_str_pair_t;

static const id_str_pair_t serial_conf_databit[] = {
	{IdDataBit7, "7"},
	{IdDataBit8, "8"},
	{IDENDMARK, NULL},
};

static const id_str_pair_t serial_conf_parity[] = {
	{IdParityNone, "none"},
	{IdParityOdd, "odd"},
	{IdParityEven, "even"},
	{IdParityMark, "mark"},
	{IdParitySpace, "space"},
	{IDENDMARK, NULL},
};

static const id_str_pair_t serial_conf_stopbit[] = {
	{IdStopBit1, "1"},
	{IdStopBit2, "2"},
	{IDENDMARK, NULL},
};

static id_str_pair_t serial_conf_flowctrl[] = {
	{IdFlowX, "x"},
	{IdFlowHard, "hard"},
	{IdFlowHard, "rtscts"},
	{IdFlowNone, "none"},
	{IdFlowHardDsrDtr, "dsrdtr"},
	{IDENDMARK, NULL},
};


/*
 * �V���A���|�[�g�֘A�̐ݒ�
 * Id���當����ɕϊ�����B
 *
 * return
 *    TRUE: �ϊ�����
 *    FALSE: �ϊ����s
 */
int SerialPortConfconvertId2Str(enum serial_port_conf type, WORD id, PCHAR str, int strlen)
{
	const id_str_pair_t *conf;
	int ret = FALSE;
	int i;

	switch (type) {
		case COM_DATABIT:
			conf = serial_conf_databit;
			break;
		case COM_PARITY:
			conf = serial_conf_parity;
			break;
		case COM_STOPBIT:
			conf = serial_conf_stopbit;
			break;
		case COM_FLOWCTRL:
			conf = serial_conf_flowctrl;
			break;
		default:
			conf = NULL;
			break;
	}
	if (conf == NULL)
		goto error;

	for (i = 0 ;  ; i++) {
		if (conf[i].id == IDENDMARK)
			goto error;
		if (conf[i].id == id) {
			strncpy_s(str, strlen, conf[i].str, _TRUNCATE);
			break;
		}
	}

	ret = TRUE;

error:
	return (ret);
}

#undef GetPrivateProfileInt
#undef GetPrivateProfileString
#define GetPrivateProfileInt(p1, p2, p3, p4) GetPrivateProfileIntAFileW(p1, p2, p3, p4)
#define GetPrivateProfileString(p1, p2, p3, p4, p5, p6) GetPrivateProfileStringAFileW(p1, p2, p3, p4, p5, p6)
#define GetPrivateProfileStringA(p1, p2, p3, p4, p5, p6) GetPrivateProfileStringAFileW(p1, p2, p3, p4, p5, p6)
#define WritePrivateProfileStringA(p1, p2, p3, p4) WritePrivateProfileStringAFileW(p1, p2, p3, p4)

/*
 * �V���A���|�[�g�֘A�̐ݒ�
 * �����񂩂�Id�ɕϊ�����B
 *
 * return
 *    TRUE: �ϊ�����
 *    FALSE: �ϊ����s
 */
static int SerialPortConfconvertStr2Id(enum serial_port_conf type, const wchar_t *str, WORD *id)
{
	const id_str_pair_t *conf;
	int ret = FALSE;
	int i;
	char *strA;

	switch (type) {
		case COM_DATABIT:
			conf = serial_conf_databit;
			break;
		case COM_PARITY:
			conf = serial_conf_parity;
			break;
		case COM_STOPBIT:
			conf = serial_conf_stopbit;
			break;
		case COM_FLOWCTRL:
			conf = serial_conf_flowctrl;
			break;
		default:
			conf = NULL;
			break;
	}
	if (conf == NULL) {
		return FALSE;
	}

	strA = ToCharW(str);
	for (i = 0 ;  ; i++) {
		if (conf[i].id == IDENDMARK) {
			ret = FALSE;
			break;
		}
		if (_stricmp(conf[i].str, strA) == 0) {
			*id = conf[i].id;
			ret = TRUE;
			break;
		}
	}
	free(strA);
	return ret;
}

static WORD str2id(const PCHAR *List, PCHAR str, WORD DefId)
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

static void id2str(const PCHAR *List, WORD Id, WORD DefId, PCHAR str, int destlen)
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

static int IconName2IconId(const wchar_t *name)
{
	int id;

	if (_wcsicmp(name, L"tterm") == 0) {
		id = IDI_TTERM;
	}
	else if (_wcsicmp(name, L"vt") == 0) {
		id = IDI_VT;
	}
	else if (_wcsicmp(name, L"tek") == 0) {
		id = IDI_TEK;
	}
	else if (_wcsicmp(name, L"tterm_classic") == 0) {
		id = IDI_TTERM_CLASSIC;
	}
	else if (_wcsicmp(name, L"vt_classic") == 0) {
		id = IDI_VT_CLASSIC;
	}
	else if (_wcsicmp(name, L"tterm_3d") == 0) {
		id = IDI_TTERM_3D;
	}
	else if (_wcsicmp(name, L"vt_3d") == 0) {
		id = IDI_VT_3D;
	}
	else if (_wcsicmp(name, L"tterm_flat") == 0) {
		id = IDI_TTERM_FLAT;
	}
	else if (_wcsicmp(name, L"vt_flat") == 0) {
		id = IDI_VT_FLAT;
	}
	else if (_wcsicmp(name, L"cygterm") == 0) {
		id = IDI_CYGTERM;
	}
	else {
		id = IdIconDefault;
	}
	return id;
}

static int IconName2IconIdA(const char *name)
{
	wchar_t *nameW = ToWcharA(name);
	int id = IconName2IconId(nameW);
	free(nameW);
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
		case IDI_TTERM_3D:
			icon = "tterm_3d";
			break;
		case IDI_VT_3D:
			icon = "vt_3d";
			break;
		case IDI_TTERM_FLAT:
			icon = "tterm_flat";
			break;
		case IDI_VT_FLAT:
			icon = "vt_flat";
			break;
		case IDI_CYGTERM:
			icon = "cygterm";
			break;
		default:
			icon = "Default";
	}
	strncpy_s(name, len, icon, _TRUNCATE);
}

static WORD GetOnOff(PCHAR Sect, PCHAR Key, const wchar_t *FName, BOOL Default)
{
	char Temp[4];
	GetPrivateProfileStringAFileW(Sect, Key, "", Temp, sizeof(Temp), FName);
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

void WriteOnOff(PCHAR Sect, PCHAR Key, const wchar_t *FName, WORD Flag)
{
	const char *on_off = (Flag != 0) ? "on" : "off";
	WritePrivateProfileStringA(Sect, Key, on_off, FName);
}

void WriteInt(PCHAR Sect, PCHAR Key, const wchar_t *FName, int i)
{
	char Temp[15];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", i);
	WritePrivateProfileStringA(Sect, Key, Temp, FName);
}

void WriteUint(PCHAR Sect, PCHAR Key, const wchar_t *FName, UINT i)
{
	char Temp[15];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%u", i);
	WritePrivateProfileStringA(Sect, Key, Temp, FName);
}

void WriteInt2(PCHAR Sect, PCHAR Key, const wchar_t *FName, int i1, int i2)
{
	char Temp[32];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d", i1, i2);
	WritePrivateProfileStringA(Sect, Key, Temp, FName);
}

void WriteInt4(PCHAR Sect, PCHAR Key, const wchar_t *FName,
			   int i1, int i2, int i3, int i4)
{
	char Temp[64];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d",
	            i1, i2, i3, i4);
	WritePrivateProfileStringA(Sect, Key, Temp, FName);
}

void WriteInt6(PCHAR Sect, PCHAR Key, const wchar_t *FName,
			   int i1, int i2, int i3, int i4, int i5, int i6)
{
	char Temp[96];
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d,%d,%d",
	            i1, i2,i3, i4, i5, i6);
	WritePrivateProfileStringA(Sect, Key, Temp, FName);
}

// �t�H���g��񏑂����݁A4�p�����[�^��
static void WriteFont(PCHAR Sect, PCHAR Key, const wchar_t *FName,
					  PCHAR Name, int x, int y, int charset)
{
	char Temp[80];
	if (Name[0] != 0)
		_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%s,%d,%d,%d",
		            Name, x, y, charset);
	else
		Temp[0] = 0;
	WritePrivateProfileStringA(Sect, Key, Temp, FName);
}

static int GetNthNumA(/*const*/ char *str, int Nth)
{
	int i;
	GetNthNum(str, Nth, &i);
	return i;
}

// �t�H���g���ǂݍ��݁A4�p�����[�^��
static void ReadFont(
	const char *Sect, const char *Key, const char *Default, const wchar_t *FName,
	char *FontName, size_t FontNameLen, POINT *FontSize, int *FontCharSet)
{
	char Temp[MAX_PATH];
	GetPrivateProfileString(Sect, Key, Default,
	                        Temp, _countof(Temp), FName);
	if (Temp[0] == 0) {
		// �f�t�H���g���Z�b�g����Ă��Ȃ� & ini�ɃG���g���[���Ȃ��ꍇ
		FontName[0] = 0;
		FontSize->x = 0;
		FontSize->y = 0;
		*FontCharSet = 0;
	} else {
		GetNthString(Temp, 1, FontNameLen, FontName);
		FontSize->x = GetNthNumA(Temp, 2);
		FontSize->y = GetNthNumA(Temp, 3);
		*FontCharSet = GetNthNumA(Temp, 4);
		// TODO �����ƃp�[�X����
	}
}

// �t�H���g���ǂݍ��݁A3�p�����[�^��
static void ReadFont3(
	const wchar_t *Sect, const wchar_t *Key, const wchar_t *Default, const wchar_t *FName,
	wchar_t *FontName, size_t FontNameLen, int *FontPoint, int *FontCharSet)
{
	wchar_t *Temp;
	hGetPrivateProfileStringW(Sect, Key, Default, FName, & Temp);
	if (Temp[0] == 0) {
		// �f�t�H���g���Z�b�g����Ă��Ȃ� & ini�ɃG���g���[���Ȃ��ꍇ
		FontName[0] = 0;
		*FontPoint = 0;
		*FontCharSet = 0;
	} else {
		GetNthStringW(Temp, 1, FontNameLen, FontName);
		GetNthNumW(Temp, 2, FontPoint);
		GetNthNumW(Temp, 3, FontCharSet);
		// TODO �����ƃp�[�X����
	}
	free(Temp);
}

/**
 *	BG�Z�N�V�����̓ǂݍ���
 *		�e�[�}�ȊO�̃A�C�e��
 */
static void DispReadIni(const wchar_t *FName, PTTSet ts)
{
	wchar_t *base;
	ts->EtermLookfeel.BGEnable = GetPrivateProfileInt(BG_SECTION, "BGEnable", 0, FName);
	ts->EtermLookfeel.BGUseAlphaBlendAPI = GetOnOff(BG_SECTION, "BGUseAlphaBlendAPI", FName, TRUE);
	ts->EtermLookfeel.BGNoFrame = GetOnOff(BG_SECTION, "BGNoFrame", FName, FALSE);
	ts->EtermLookfeel.BGFastSizeMove = GetOnOff(BG_SECTION, "BGFastSizeMove", FName, TRUE);
	ts->EtermLookfeel.BGNoCopyBits = GetOnOff(BG_SECTION, "BGFlickerlessMove", FName, TRUE);
	if (ts->EtermLookfeel.BGSPIPathW != NULL) {
		free(ts->EtermLookfeel.BGSPIPathW);
	}
	hGetPrivateProfileStringW(BG_SECTIONW, L"BGSPIPath", L"plugin", FName, &base);
	if (base[0] == 0) {
		free(base);
		ts->EtermLookfeel.BGSPIPathW = NULL;
	}
	else {
		wchar_t *full;
		if (IsRelativePathW(base)) {
			aswprintf(&full, L"%s\\%s", ts->HomeDirW, base);
			ts->EtermLookfeel.BGSPIPathW = full;
			free(base);
		}
		else {
			ts->EtermLookfeel.BGSPIPathW = base;
		}
	}
	hGetPrivateProfileStringW(BG_SECTIONW, L"BGThemeFile", L"", FName, &base);
	if (base[0] == 0) {
		free(base);
		ts->EtermLookfeel.BGThemeFileW = NULL;
	}
	else {
		if (IsRelativePathW(base)) {
			wchar_t *full;
			aswprintf(&full, L"%s\\%s", ts->HomeDirW, base);
			ts->EtermLookfeel.BGThemeFileW = full;
			free(base);
		}
		else {
			ts->EtermLookfeel.BGThemeFileW = base;
		}
	}
}

/**
 *	BG�Z�N�V�����̏�������
 *		�e�[�}�ȊO�̃A�C�e��
 */
static void DispWriteIni(const wchar_t *FName, PTTSet ts)
{
	// Eterm lookfeel alphablend (theme file)
	WriteInt(BG_SECTION, "BGEnable", FName, ts->EtermLookfeel.BGEnable);
	WritePrivateProfileStringW(BG_SECTIONW, L"BGThemeFile",
	                          ts->EtermLookfeel.BGThemeFileW, FName);
	WriteOnOff(BG_SECTION, "BGUseAlphaBlendAPI", FName,
	           ts->EtermLookfeel.BGUseAlphaBlendAPI);
	WritePrivateProfileStringW(BG_SECTIONW, L"BGSPIPath",
	                          ts->EtermLookfeel.BGSPIPathW, FName);
	WriteOnOff(BG_SECTION, "BGFastSizeMove", FName,
	           ts->EtermLookfeel.BGFastSizeMove);
	WriteOnOff(BG_SECTION, "BGFlickerlessMove", FName,
	           ts->EtermLookfeel.BGNoCopyBits);
	WriteOnOff(BG_SECTION, "BGNoFrame", FName,
	           ts->EtermLookfeel.BGNoFrame);
}

/**
 *	Unicode Ambiguous,Emoji �̃f�t�H���g��
 */
static int GetDefaultUnicodeWidth(void)
{
	return 2;
}

static void GetPrivateProfileColor2(const char *appA, const char *keyA, const char *defA, const wchar_t *fname,
									COLORREF *color)
{
	int i;
	int j;

	wchar_t *str;
	wchar_t *appW = ToWcharA(appA);
	wchar_t *keyW = ToWcharA(keyA);
	wchar_t *defW = ToWcharA(defA);
	hGetPrivateProfileStringW(appW, keyW, defW, fname, &str);
	free(appW);
	free(keyW);
	free(defW);

	for (i = 0; i < 2; i++) {
		BYTE rgb[3];
		for (j = 0; j < 3; j++) {
			int t;
			GetNthNumW(str, 1 + (i * 3) + j, &t);
			rgb[j] = (BYTE)t;
		}
		color[i] = RGB(rgb[0], rgb[1], rgb[2]);
	}

	free(str);
}

void PASCAL _ReadIniFile(const wchar_t *FName, PTTSet ts)
{
	int i;
	char Temp[MAX_PATH], Temp2[MAX_PATH], *p;
	wchar_t TempW[MAX_PATH];

	ts->Minimize = 0;
	ts->HideWindow = 0;
	ts->LogFlag = 0;			// Log flags
	ts->FTFlag = 0;				// File transfer flags
	ts->MenuFlag = 0;			// Menu flags
	ts->TermFlag = 0;			// Terminal flag
	ts->ColorFlag = 0;			// ANSI/Attribute color flags
	ts->FontFlag = 0;			// Font flag
	ts->PortFlag = 0;			// Port flags
	ts->WindowFlag = 0;			// Window flags
	ts->CtrlFlag = 0;			// Control sequence flags
	ts->PasteFlag = 0;			// Clipboard Paste flags
	ts->TelPort = 23;

	ts->DisableTCPEchoCR = FALSE;

	/*
	 * Version number
	 * �ݒ�t�@�C�����ǂ̃o�[�W������ Tera Term �ŕۑ����ꂽ����\��
	 * �ݒ�t�@�C���̕ۑ����͂��̒l�ł͂Ȃ��A���݂� Tera Term �̃o�[�W�������g����
	 */
	GetPrivateProfileString(Section, "Version", TT_VERSION_STR("."), Temp, sizeof(Temp), FName);
	p = strchr(Temp, '.');
	if (p) {
		*p++ = 0;
		ts->ConfigVersion = atoi(Temp) * 10000 + atoi(p);
	}
	else {
		ts->ConfigVersion = 0;
	}

	// TTX �� �m�F�ł���悤�ATera Term �̃o�[�W�������i�[���Ă���
	ts->RunningVersion = TT_VERSION_MAJOR * 10000 + TT_VERSION_MINOR;

	/* Port type */
	GetPrivateProfileString(Section, "Port", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "serial") == 0)
		ts->PortType = IdSerial;
	else {
		ts->PortType = IdTCPIP;
	}

	/* VT win position */
	GetPrivateProfileString(Section, "VTPos", "-2147483648,-2147483648", Temp, sizeof(Temp), FName);	/* default: random position */
	GetNthNum(Temp, 1, (int *) (&ts->VTPos.x));
	GetNthNum(Temp, 2, (int *) (&ts->VTPos.y));

	/* TEK win position */
	GetPrivateProfileString(Section, "TEKPos", "-2147483648,-2147483648", Temp, sizeof(Temp), FName);	/* default: random position */
	GetNthNum(Temp, 1, (int *) &(ts->TEKPos.x));
	GetNthNum(Temp, 2, (int *) &(ts->TEKPos.y));

	/* Save VT Window position */
	ts->SaveVTWinPos = GetOnOff(Section, "SaveVTWinPos", FName, FALSE);

	/* VT terminal size  */
	GetPrivateProfileString(Section, "TerminalSize", "80,24",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, &ts->TerminalWidth);
	GetNthNum(Temp, 2, &ts->TerminalHeight);
	if (ts->TerminalWidth <= 0)
		ts->TerminalWidth = 80;
	else if (ts->TerminalWidth > TermWidthMax)
		ts->TerminalWidth = TermWidthMax;
	if (ts->TerminalHeight <= 0)
		ts->TerminalHeight = 24;
	else if (ts->TerminalHeight > TermHeightMax)
		ts->TerminalHeight = TermHeightMax;

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
	else if (_stricmp(Temp, "AUTO") == 0) {
		ts->CRReceive = IdAUTO;
	}
	else {
		ts->CRReceive = IdCR;
	}
	/* CR Send */
	GetPrivateProfileString(Section, "CRSend", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "CRLF") == 0) {
		ts->CRSend = IdCRLF;
	}
	else if (_stricmp(Temp, "LF") == 0) {
		ts->CRSend = IdLF;
	}
	else {
		ts->CRSend = IdCR;
	}
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
	ts->KanjiCode = GetKanjiCodeFromStr(Temp);

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
	ts->KanjiCodeSend = GetKanjiCodeFromStr(Temp);

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
	ts->KanjiIn = GetKanjiInCodeFromIni(Temp);

	/* KanjiOut */
	GetPrivateProfileString(Section, "KanjiOut", "",
	                        Temp, sizeof(Temp), FName);
	ts->KanjiOut = GetKanjiOutCodeFromIni(Temp);

	/* Auto Win Switch VT<->TEK */
	ts->AutoWinSwitch = GetOnOff(Section, "AutoWinSwitch", FName, FALSE);

	/* Terminal ID */
	GetPrivateProfileString(Section, "TerminalID", "",
	                        Temp, sizeof(Temp), FName);
	ts->TerminalID = str2id(TermList, Temp, IdVT100);

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
	GetPrivateProfileColor2(Section, "VTColor", "0,0,0,255,255,255", FName, ts->VTColor);

	/* VT Bold Color */
	GetPrivateProfileColor2(Section, "VTBoldColor", "0,0,255,255,255,255", FName, ts->VTBoldColor);
	if (GetOnOff(Section, "EnableBoldAttrColor", FName, TRUE))
		ts->ColorFlag |= CF_BOLDCOLOR;

	/* VT Blink Color */
	GetPrivateProfileColor2(Section, "VTBlinkColor", "255,0,0,255,255,255", FName, ts->VTBlinkColor);
	if (GetOnOff(Section, "EnableBlinkAttrColor", FName, TRUE))
		ts->ColorFlag |= CF_BLINKCOLOR;

	/* VT Reverse Color */
	GetPrivateProfileColor2(Section, "VTReverseColor", "255,255,255,0,0,0", FName, ts->VTReverseColor);
	if (GetOnOff(Section, "EnableReverseAttrColor", FName, FALSE))
		ts->ColorFlag |= CF_REVERSECOLOR;

	ts->EnableClickableUrl =
		GetOnOff(Section, "EnableClickableUrl", FName, FALSE);

	/* URL Color */
	GetPrivateProfileColor2(Section, "URLColor", "0,255,0,255,255,255", FName, ts->URLColor);
	if (GetOnOff(Section, "EnableURLColor", FName, TRUE))
		ts->ColorFlag |= CF_URLCOLOR;

	/* Underline */
	if (GetOnOff(Section, "URLUnderline", FName, TRUE))
		ts->FontFlag |= FF_URLUNDERLINE;
	if (GetOnOff(Section, "UnderlineAttrFont", FName, TRUE))
		ts->FontFlag |= FF_UNDERLINE;
	if (GetOnOff(Section, "UnderlineAttrColor", FName, TRUE))
		ts->ColorFlag |= CF_UNDERLINE;
	GetPrivateProfileColor2(Section, "VTUnderlineColor", "255,0,255,255,255,255", FName, ts->VTUnderlineColor);

	/* TEK Color */
	GetPrivateProfileColor2(Section, "TEKColor", "0,0,0,255,255,255", FName, ts->TEKColor);

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
		for (i = 0; i < n; i++) {
			int colorid, r, g, b;
			GetNthNum(Temp, i * 4 + 1, (int *) &colorid);
			GetNthNum(Temp, i * 4 + 2, (int *) &r);
			GetNthNum(Temp, i * 4 + 3, (int *) &g);
			GetNthNum(Temp, i * 4 + 4, (int *) &b);
			ts->ANSIColor[colorid & 15] =
				RGB((BYTE) r, (BYTE) g, (BYTE) b);
		}
	}

	// �`�悷��f�o�C�X(�f�B�X�v���C/�v�����^)�ɍ��킹��
	// GetNearestColor()����ق����ǂ��̂ł͂Ȃ����낤��
	// �ŋ߂̃f�B�X�v���C�A�_�v�^�Ȃ�24bit color���Ǝv����̂�
	// ���̃u���b�N�͎�����e���Ȃ��̂�������Ȃ�
#if 1
	{
		HDC TmpDC = GetDC(0);			/* Get screen device context */
		for (i = 0; i <= 1; i++)
			ts->VTColor[i] = GetNearestColor(TmpDC, ts->VTColor[i]);
		for (i = 0; i <= 1; i++)
			ts->VTBoldColor[i] = GetNearestColor(TmpDC, ts->VTBoldColor[i]);
		for (i = 0; i <= 1; i++)
			ts->VTBlinkColor[i] = GetNearestColor(TmpDC, ts->VTBlinkColor[i]);
		for (i = 0; i <= 1; i++)
			ts->TEKColor[i] = GetNearestColor(TmpDC, ts->TEKColor[i]);
		for (i = 0; i <= 1; i++)
			ts->URLColor[i] = GetNearestColor(TmpDC, ts->URLColor[i]);
		for (i = 0; i <= 1; i++)
			ts->VTUnderlineColor[i] = GetNearestColor(TmpDC, ts->VTUnderlineColor[i]);
		for (i = 0; i < 16; i++)
			ts->ANSIColor[i] = GetNearestColor(TmpDC, ts->ANSIColor[i]);
		ReleaseDC(0, TmpDC);
	}
#endif
	if (GetOnOff(Section, "EnableANSIColor", FName, TRUE))
		ts->ColorFlag |= CF_ANSICOLOR;

	/* TEK color emulation */
	ts->TEKColorEmu = GetOnOff(Section, "TEKColorEmulation", FName, FALSE);

	/* VT Font */
	ReadFont(Section, "VTFont", "Terminal,0,-13,1", FName,
			 ts->VTFont, _countof(ts->VTFont),
			 &ts->VTFontSize, &(ts->VTFontCharSet));

	/* Bold font flag */
	if (GetOnOff(Section, "EnableBold", FName, TRUE))
		ts->FontFlag |= FF_BOLD;

	/* TEK Font */
	ReadFont(Section, "TEKFont", "Courier,0,-13,0", FName,
			 ts->TEKFont, _countof(ts->TEKFont),
			 &ts->TEKFontSize, &(ts->TEKFontCharSet));

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
	GetPrivateProfileString(Section, "MetaKey", "off", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "on") == 0)
	  ts->MetaKey = IdMetaOn;
	else if (_stricmp(Temp, "left") == 0)
	  ts->MetaKey = IdMetaLeft;
	else if (_stricmp(Temp, "right") == 0)
	  ts->MetaKey = IdMetaRight;
	else
	  ts->MetaKey = IdMetaOff;

	// Windows95 �n�͍��E�� Alt �̔��ʂɔ�Ή�
	if (!IsWindowsNTKernel() && ts->MetaKey != IdMetaOff) {
	  ts->MetaKey = IdMetaOn;
	}

	/* Application Keypad */
	ts->DisableAppKeypad =
		GetOnOff(Section, "DisableAppKeypad", FName, FALSE);

	/* Application Cursor */
	ts->DisableAppCursor =
		GetOnOff(Section, "DisableAppCursor", FName, FALSE);

	/* Russian keyboard type */
	GetPrivateProfileString(Section, "RussKeyb", "",
	                        Temp, sizeof(Temp), FName);
	ts->RussKeyb = str2id(RussList2, Temp, /*IdWindows*/0);

	/* Serial port ID */
	ts->ComPort = GetPrivateProfileInt(Section, "ComPort", 1, FName);

	/* Baud rate */
	ts->Baud = GetPrivateProfileInt(Section, "BaudRate", 9600, FName);

	/* Parity */
	GetPrivateProfileStringW(SectionW, L"Parity", L"",
							 TempW, _countof(TempW), FName);
	if (!SerialPortConfconvertStr2Id(COM_PARITY, TempW, &ts->Parity)) {
		ts->Parity = IdParityNone;
	}

	/* Data bit */
	GetPrivateProfileStringW(SectionW, L"DataBit", L"",
							 TempW, _countof(TempW), FName);
	if (!SerialPortConfconvertStr2Id(COM_DATABIT, TempW, &ts->DataBit)) {
		ts->DataBit = IdDataBit8;
	}

	/* Stop bit */
	GetPrivateProfileStringW(SectionW, L"StopBit", L"",
							 TempW, _countof(TempW), FName);
	if (!SerialPortConfconvertStr2Id(COM_STOPBIT, TempW, &ts->StopBit)) {
		ts->StopBit = IdStopBit1;
	}

	/* Flow control */
	GetPrivateProfileStringW(SectionW, L"FlowCtrl", L"",
							 TempW, _countof(TempW), FName);
	if (!SerialPortConfconvertStr2Id(COM_FLOWCTRL, TempW, &ts->Flow)) {
		ts->Flow = IdFlowNone;
	}

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

	/* Log binary flag */
	ts->LogBinary = GetOnOff(Section, "LogBinary", FName, FALSE);

	/* Log append */
	ts->Append = GetOnOff(Section, "LogAppend", FName, FALSE);

	/* Log plain text (2005.5.7 yutaka) */
	ts->LogTypePlainText =
		GetOnOff(Section, "LogTypePlainText", FName, FALSE);

	/* Log with timestamp (2006.7.23 maya) */
	ts->LogTimestamp = GetOnOff(Section, "LogTimestamp", FName, FALSE);

	/* Log without transfer dialog */
	ts->LogHideDialog = GetOnOff(Section, "LogHideDialog", FName, FALSE);

	ts->LogAllBuffIncludedInFirst = GetOnOff(Section, "LogIncludeScreenBuffer", FName, FALSE);

	/* Timestamp format of Log each line */
	GetPrivateProfileString(Section, "LogTimestampFormat", "%Y-%m-%d %H:%M:%S.%N",
	                        ts->LogTimestampFormat, sizeof(ts->LogTimestampFormat),
	                        FName);

	/* Timestamp type */
	GetPrivateProfileString(Section, "LogTimestampType", "", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "UTC") == 0)
		ts->LogTimestampType = TIMESTAMP_UTC;
	else if (_stricmp(Temp, "LoggingElapsed") == 0)
		ts->LogTimestampType = TIMESTAMP_ELAPSED_LOGSTART;
	else if (_stricmp(Temp, "ConnectionElapsed") == 0)
		ts->LogTimestampType = TIMESTAMP_ELAPSED_CONNECTED;
	else if (_stricmp(Temp, "") == 0 && GetOnOff(Section, "LogTimestampUTC", FName, FALSE))
		// LogTimestampType �����ݒ�̏ꍇ�� LogTimestampUTC �̒l���Q�Ƃ���
		ts->LogTimestampType = TIMESTAMP_UTC;
	else
		ts->LogTimestampType = TIMESTAMP_LOCAL;

	/* File Transfer dialog visibility */
	ts->FTHideDialog = GetOnOff(Section, "FTHideDialog", FName, FALSE);

	/* Default Log file name */
	hGetPrivateProfileStringW(SectionW, L"LogDefaultName", L"teraterm.log", FName,
							  &ts->LogDefaultNameW);

	/* Default Log file path */
	hGetPrivateProfileStringW(SectionW, L"LogDefaultPath", ts->LogDirW, FName, &ts->LogDefaultPathW);
	if (ts->LogDefaultPathW[0] == 0) {
		// ���w��("LogDefaultPath=")�������ANULL������
		free(ts->LogDefaultPathW);
		ts->LogDefaultPathW = NULL;
	}

	/* Auto start logging (2007.5.31 maya) */
	ts->LogAutoStart = GetOnOff(Section, "LogAutoStart", FName, FALSE);

	/* Log Rotate (2013.3.24 yutaka) */
	ts->LogRotate = GetPrivateProfileInt(Section, "LogRotate", 0, FName);
	ts->LogRotateSize = GetPrivateProfileInt(Section, "LogRotateSize", 0, FName);
	ts->LogRotateSizeType = GetPrivateProfileInt(Section, "LogRotateSizeType", 0, FName);
	ts->LogRotateStep = GetPrivateProfileInt(Section, "LogRotateStep", 0, FName);

	/* Deferred Log Write Mode (2013.4.20 yutaka) */
	ts->DeferredLogWriteMode = GetOnOff(Section, "DeferredLogWriteMode", FName, TRUE);


	/* XMODEM option */
	GetPrivateProfileString(Section, "XmodemOpt", "",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "crc") == 0)
		ts->XmodemOpt = XoptCRC;
	else if (_stricmp(Temp, "1k") == 0)
		ts->XmodemOpt = Xopt1kCRC;
	else if (_stricmp(Temp, "1ksum") == 0)
		ts->XmodemOpt = Xopt1kCksum;
	else
		ts->XmodemOpt = XoptCheck;

	/* XMODEM binary file */
	ts->XmodemBin = GetOnOff(Section, "XmodemBin", FName, TRUE);

	/* XMODEM ��M�R�}���h (2007.12.21 yutaka) */
	GetPrivateProfileString(Section, "XModemRcvCommand", "",
	                        ts->XModemRcvCommand,
	                        sizeof(ts->XModemRcvCommand), FName);

	/* Default directory for file transfer */
	hGetPrivateProfileStringW(SectionW, L"FileDir", L"", FName, &ts->FileDirW);
	if (ts->FileDirW != NULL && ts->FileDirW[0] == 0) {
		free(ts->FileDirW);
		ts->FileDirW = NULL;
	}

	/* filter on file send (2007.6.5 maya) */
	GetPrivateProfileString(Section, "FileSendFilter", "",
	                        ts->FileSendFilter, sizeof(ts->FileSendFilter),
	                        FName);

	/* SCP���M��p�X (2012.4.6 yutaka) */
	GetPrivateProfileString(Section, "ScpSendDir", "",
	                        ts->ScpSendDir, sizeof(ts->ScpSendDir), FName);


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

	// Detect disconnect/reconnect of serial port --- special option
	ts->AutoComPortReconnect = GetOnOff(Section, "AutoComPortReconnect", FName, TRUE);
	ts->AutoComPortReconnectDelayNormal =
		GetPrivateProfileInt(Section, "AutoComPortReconnectDelayNormal", 500, FName);
	ts->AutoComPortReconnectDelayIllegal =
		GetPrivateProfileInt(Section, "AutoComPortReconnectDelayIllegal", 2000, FName);
	ts->AutoComPortReconnectRetryInterval =
		GetPrivateProfileInt(Section, "AutoComPortReconnectRetryInterval", 1000, FName);
	ts->AutoComPortReconnectRetryCount =
		GetPrivateProfileInt(Section, "AutoComPortReconnectRetryCount", 3, FName);

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

	/* Wait time (ms) when Beep is Visual Bell -- special option */
	ts->BeepVBellWait = GetPrivateProfileInt(Section, "BeepVBellWait", 10, FName);
	if (ts->BeepVBellWait < 1)
		ts->BeepVBellWait = 1;

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

	/* When serial port is specified with with /C= option and the port does not exist, Tera Term will wait for port connection.  */
	ts->WaitCom = GetOnOff(Section, "WaitCom", FName, FALSE);

	/* Confirm disconnection -- special option */
	if (GetOnOff(Section, "ConfirmDisconnect", FName, TRUE))
		ts->PortFlag |= PF_CONFIRMDISCONN;

	/* Ctrl code in Kanji -- special option */
	if (GetOnOff(Section, "CtrlInKanji", FName, TRUE))
		ts->TermFlag |= TF_CTRLINKANJI;

	/* Debug flag  -- special option */
	ts->Debug = GetOnOff(Section, "Debug", FName, FALSE);

	/* Delimiter list -- special option */
	{
		wchar_t *s;
		hGetPrivateProfileStringW(SectionW, L"DelimList",
								  L"$20!\"#$24%&\'()*+,-./:;<=>?@[\\]^`{|}~",
								  FName, &s);
		free(ts->DelimListW);
		ts->DelimListW = Hex2StrW(s, 0);
		free(s);
	}

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

	// Enable multiple bytes send -- special option
	ts->FileSendHighSpeedMode = GetOnOff(Section, "FileSendHighSpeedMode", FName, TRUE);

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
	if (GetOnOff(Section, "KmtLongPacket", FName, FALSE))
		ts->KermitOpt |= KmtOptLongPacket;
	if (GetOnOff(Section, "KmtFileAttr", FName, FALSE))
		ts->KermitOpt |= KmtOptFileAttr;

	/* Maximum scroll buffer size  -- special option */
	ts->ScrollBuffMax =
		GetPrivateProfileInt(Section, "MaxBuffSize", 10000, FName);
	if (ts->ScrollBuffMax < 24)
		ts->ScrollBuffMax = 10000;

	/* Max com port number -- special option */
	ts->MaxComPort = GetPrivateProfileInt(Section, "MaxComPort", 256, FName);
	if (ts->MaxComPort < 4)
		ts->MaxComPort = 4;
	if (ts->MaxComPort > MAXCOMPORT)
		ts->MaxComPort = MAXCOMPORT;
	if ((ts->ComPort < 1) || (ts->ComPort > ts->MaxComPort))
		ts->ComPort = 1;

	/* Non-blinking cursor -- special option */
	ts->NonblinkingCursor =
		GetOnOff(Section, "NonblinkingCursor", FName, FALSE);

	// �t�H�[�J�X�������̃|���S���J�[�\�� (2008.1.24 yutaka)
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

	/* �v�����^�p����R�[�h���󂯕t���邩 */
	if (GetOnOff(Section, "PrinterCtrlSequence", FName, FALSE))
		ts->TermFlag |= TF_PRINTERCTRL;

	/* Printer Font --- special option */
	ReadFont(Section, "PrnFont", NULL, FName,
			 ts->PrnFont, _countof(ts->PrnFont),
			 &ts->PrnFontSize, &(ts->PrnFontCharSet));

	// Page margins (left, right, top, bottom) for printing
	//    -- special option
	GetPrivateProfileString(Section, "PrnMargin", "50,50,50,50",
	                        Temp, sizeof(Temp), FName);
	for (i = 0; i <= 3; i++)
		GetNthNum(Temp, 1 + i, &ts->PrnMargin[i]);

	/* Disable (convert to NL) Form Feed when printing */
	/*   --- special option */
	ts->PrnConvFF =
		GetOnOff(Section, "PrnConvFF", FName, FALSE);

	/* Quick-VAN log  -- special option */
	if (GetOnOff(Section, "QVLog", FName, FALSE))
		ts->LogFlag |= LOG_QV;

	/* Quick-VAN window size -- special */
	ts->QVWinSize = GetPrivateProfileInt(Section, "QVWinSize", 8, FName);

	/* Scroll threshold -- special option */
	ts->ScrollThreshold =
		GetPrivateProfileInt(Section, "ScrollThreshold", 12, FName);

	ts->MouseWheelScrollLine =
		GetPrivateProfileInt(Section, "MouseWheelScrollLine", 3, FName);

	// Select on activate -- special option
	ts->SelOnActive = GetOnOff(Section, "SelectOnActivate", FName, TRUE);

	/* Send 8bit control sequence -- special option */
	ts->Send8BitCtrl = GetOnOff(Section, "Send8BitCtrl", FName, FALSE);

	/* SendBreak time (in msec) -- special option */
	ts->SendBreakTime =
		GetPrivateProfileInt(Section, "SendBreakTime", 1000, FName);

	/* Startup macro -- special option */
	hGetPrivateProfileStringW(SectionW, L"StartupMacro", L"", FName, &ts->MacroFNW);
	if (ts->MacroFNW != NULL && ts->MacroFNW[0] == L'\0') {
		// �w��Ȃ�
		free(ts->MacroFNW);
		ts->MacroFNW = NULL;
	}

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
		GetPrivateProfileInt(Section, "TitleFormat", 13, FName);

	/* VT Compatible Tab -- special option */
	ts->VTCompatTab = GetOnOff(Section, "VTCompatTab", FName, FALSE);

	/* VT Font space --- special option */
	GetPrivateProfileString(Section, "VTFontSpace", "0,0,0,0",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, &ts->FontDX);
	GetNthNum(Temp, 2, &ts->FontDW);
	GetNthNum(Temp, 3, &ts->FontDY);
	GetNthNum(Temp, 4, &ts->FontDH);
	/*
	if (ts->FontDX < 0)
		ts->FontDX = 0;
	if (ts->FontDW < 0)
		ts->FontDW = 0;
	*/
	ts->FontDW = ts->FontDW + ts->FontDX;
	/*
	if (ts->FontDY < 0)
		ts->FontDY = 0;
	if (ts->FontDH < 0)
		ts->FontDH = 0;
	*/
	ts->FontDH = ts->FontDH + ts->FontDY;

	// VT-print scaling factors (pixels per inch) --- special option
	GetPrivateProfileString(Section, "VTPPI", "0,0",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, (int *) &ts->VTPPI.x);
	GetNthNum(Temp, 2, (int *) &ts->VTPPI.y);

	// TEK-print scaling factors (pixels per inch) --- special option
	GetPrivateProfileString(Section, "TEKPPI", "0,0",
	                        Temp, sizeof(Temp), FName);
	GetNthNum(Temp, 1, (int *) &ts->TEKPPI.x);
	GetNthNum(Temp, 2, (int *) &ts->TEKPPI.y);

	// Show "Window" menu -- special option
	if (GetOnOff(Section, "WindowMenu", FName, TRUE))
		ts->MenuFlag |= MF_SHOWWINMENU;

	/* XMODEM log  -- special option */
	if (GetOnOff(Section, "XmodemLog", FName, FALSE))
		ts->LogFlag |= LOG_X;

	/* YMODEM log  -- special option */
	if (GetOnOff(Section, "YmodemLog", FName, FALSE))
		ts->LogFlag |= LOG_Y;

	/* YMODEM ��M�R�}���h (2010.3.23 yutaka) */
	GetPrivateProfileString(Section, "YModemRcvCommand", "rb",
	                        ts->YModemRcvCommand, sizeof(ts->YModemRcvCommand), FName);

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

	/* ZMODEM ��M�R�}���h (2007.12.21 yutaka) */
	GetPrivateProfileString(Section, "ZModemRcvCommand", "rz",
	                        ts->ZModemRcvCommand, sizeof(ts->ZModemRcvCommand), FName);

	/* Enable continued-line copy  -- special option */
	ts->EnableContinuedLineCopy =
		GetOnOff(Section, "EnableContinuedLineCopy", FName, FALSE);

	if (GetOnOff(Section, "DisablePasteMouseRButton", FName, FALSE))
		ts->PasteFlag |= CPF_DISABLE_RBUTTON;

	if (GetOnOff(Section, "DisablePasteMouseMButton", FName, TRUE))
		ts->PasteFlag |= CPF_DISABLE_MBUTTON;

	if (GetOnOff(Section, "ConfirmPasteMouseRButton", FName, FALSE))
		ts->PasteFlag |= CPF_CONFIRM_RBUTTON;

	if (GetOnOff(Section, "ConfirmChangePaste", FName, TRUE))
		ts->PasteFlag |= CPF_CONFIRM_CHANGEPASTE;

	if (GetOnOff(Section, "ConfirmChangePasteCR", FName, TRUE))
		ts->PasteFlag |= CPF_CONFIRM_CHANGEPASTE_CR;

	GetPrivateProfileString(Section, "ConfirmChangePasteStringFile", "",
	                        Temp, sizeof(Temp), FName);

	strncpy_s(ts->ConfirmChangePasteStringFile, sizeof(ts->ConfirmChangePasteStringFile), Temp,
	          _TRUNCATE);

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
	ts->AlphaBlendInactive =
		GetPrivateProfileInt(Section, "AlphaBlend", 255, FName);
	ts->AlphaBlendInactive = max(0, ts->AlphaBlendInactive);
	ts->AlphaBlendInactive = min(255, ts->AlphaBlendInactive);
	ts->AlphaBlendActive =
		GetPrivateProfileInt(Section, "AlphaBlendActive", ts->AlphaBlendInactive, FName);
	ts->AlphaBlendActive = max(0, ts->AlphaBlendActive);
	ts->AlphaBlendActive = min(255, ts->AlphaBlendActive);

	// Cygwin install path
	GetPrivateProfileString(Section, "CygwinDirectory ", "c:\\cygwin",
	                        Temp, sizeof(Temp), FName);
	strncpy_s(ts->CygwinDirectory, sizeof(ts->CygwinDirectory), Temp,
	          _TRUNCATE);

	// Viewlog Editor path
	hGetPrivateProfileStringW(SectionW, L"ViewlogEditor", L"notepad.exe", FName, &ts->ViewlogEditorW);
	hGetPrivateProfileStringW(SectionW, L"ViewlogEditorArg", NULL, FName, &ts->ViewlogEditorArg);

	// UI language message file (full path)
	ts->UILanguageFileW = GetUILanguageFileFullW(FName);


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
	GetPrivateProfileString(Section, "MaximizedBugTweak", "2", Temp,
	                        sizeof(Temp), FName);
	if (_stricmp(Temp, "on") == 0) {
		ts->MaximizedBugTweak = 2;
	}
	else {
		ts->MaximizedBugTweak = atoi(Temp);
	}

	{
		const UINT ui = GetPrivateProfileIntW(SectionW, L"DecSpMappingDir",
											  (INT)IdDecSpecialUniToDec, FName);
		ts->Dec2Unicode =
			ui == 0 ? IdDecSpecialUniToDec :
			ui == 1 ? IdDecSpecialDecToUni :
			ui == 2 ? IdDecSpecialDoNot : IdDecSpecialUniToDec;
	}

	// Convert Unicode symbol characters to DEC Special characters
	ts->UnicodeDecSpMapping =
		GetPrivateProfileInt(Section, "UnicodeToDecSpMapping", 3, FName);

	// VT Window Icon
	GetPrivateProfileString(Section, "VTIcon", "Default",
	                        Temp, sizeof(Temp), FName);
	ts->VTIcon = IconName2IconIdA(Temp);

	// Tek Window Icon
	GetPrivateProfileString(Section, "TEKIcon", "Default",
	                        Temp, sizeof(Temp), FName);
	ts->TEKIcon = IconName2IconIdA(Temp);

	// UseNormalBGColor
	ts->UseNormalBGColor =
		GetOnOff(Section, "UseNormalBGColor", FName, FALSE);

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
	ts->PasteDialogSize.cx = GetNthNumA(Temp, 1);
	ts->PasteDialogSize.cy = GetNthNumA(Temp, 2);
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

	ts->AcceleratorNewConnection =
		GetOnOff(Section, "AcceleratorNewConnection", FName, TRUE);

	ts->AcceleratorCygwinConnection =
		GetOnOff(Section, "AcceleratorCygwinConnection", FName, TRUE);

	// added DisableMenuDuplicateSession (2010.8.3 maya)
	ts->DisableMenuDuplicateSession =
		GetOnOff(Section, "DisableMenuDuplicateSession", FName, FALSE);

	// added DisableMenuNewConnection (2010.8.4 maya)
	ts->DisableMenuNewConnection =
		GetOnOff(Section, "DisableMenuNewConnection", FName, FALSE);

	// added PasteDelayPerLine (2009.4.12 maya)
	ts->PasteDelayPerLine =
		GetPrivateProfileInt(Section, "PasteDelayPerLine", 10, FName);
	{
		int tmp = min(max(0, ts->PasteDelayPerLine), 5000);
		ts->PasteDelayPerLine = tmp;
	}

	// Font scaling -- test
	ts->FontScaling = GetOnOff(Section, "FontScaling", FName, FALSE);

	// Meta sets MSB
	GetPrivateProfileString(Section, "Meta8Bit", "off", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "raw") == 0 || _stricmp(Temp, "on") == 0)
		ts->Meta8Bit = IdMeta8BitRaw;
	else if (_stricmp(Temp, "text") == 0)
		ts->Meta8Bit = IdMeta8BitText;
	else
		ts->Meta8Bit = IdMeta8BitOff;

	// Window control sequence
	if (GetOnOff(Section, "WindowCtrlSequence", FName, TRUE))
		ts->WindowFlag |= WF_WINDOWCHANGE;

	// Cursor control sequence
	if (GetOnOff(Section, "CursorCtrlSequence", FName, FALSE))
		ts->WindowFlag |= WF_CURSORCHANGE;

	// Window report sequence
	if (GetOnOff(Section, "WindowReportSequence", FName, TRUE))
		ts->WindowFlag |= WF_WINDOWREPORT;

	// Title report sequence
	GetPrivateProfileString(Section, "TitleReportSequence", "Empty", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "accept") == 0)
		ts->WindowFlag |= IdTitleReportAccept;
	else if (_stricmp(Temp, "ignore") == 0 || _stricmp(Temp, "off") == 0)
		ts->WindowFlag &= ~WF_TITLEREPORT;
	else // empty
		ts->WindowFlag |= IdTitleReportEmpty;

	// Line at a time mode
	ts->EnableLineMode = GetOnOff(Section, "EnableLineMode", FName, TRUE);

	// Clear window on resize
	if (GetOnOff(Section, "ClearOnResize", FName, FALSE))
		ts->TermFlag |= TF_CLEARONRESIZE;

	// Alternate Screen Buffer
	if (GetOnOff(Section, "AlternateScreenBuffer", FName, TRUE))
		ts->TermFlag |= TF_ALTSCR;

	// IME status related cursor style
	if (GetOnOff(Section, "IMERelatedCursor", FName, FALSE))
		ts->WindowFlag |= WF_IMECURSORCHANGE;

	// Terminal Unique ID
	GetPrivateProfileString(Section, "TerminalUID", "FFFFFFFF", Temp, sizeof(Temp), FName);
	if (strlen(Temp) == 8) {
		for (i=0; i<8 && isxdigit((unsigned char)Temp[i]); i++) {
			if (islower(Temp[i])) {
				ts->TerminalUID[i] = toupper(Temp[i]);
			}
			else {
				ts->TerminalUID[i] = Temp[i];
			}
		}
		if (i == 8) {
			ts->TerminalUID[i] = 0;
		}
		else {
			strncpy_s(ts->TerminalUID, sizeof(ts->TerminalUID), "FFFFFFFF", _TRUNCATE);
		}
	}
	else {
		strncpy_s(ts->TerminalUID, sizeof(ts->TerminalUID), "FFFFFFFF", _TRUNCATE);
	}

	// Lock Terminal UID
	if (GetOnOff(Section, "LockTUID", FName, TRUE))
		ts->TermFlag |= TF_LOCKTUID;

	// Jump List
	ts->JumpList = GetOnOff(Section, "JumpList", FName, TRUE);

	// TabStopModifySequence
	GetPrivateProfileString(Section, "TabStopModifySequence", "on", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "on") == 0 || _stricmp(Temp, "all") == 0)
		ts->TabStopFlag = TABF_ALL;
	else if (_stricmp(Temp, "off") == 0 || _stricmp(Temp, "none") == 0)
		ts->TabStopFlag = TABF_NONE;
	else {
		ts->TabStopFlag = TABF_NONE;
		for (i=1; GetNthString(Temp, i, sizeof(Temp2), Temp2); i++) {
			if (_stricmp(Temp2, "HTS") == 0)
				ts->TabStopFlag |= TABF_HTS;
			else if (_stricmp(Temp2, "HTS7") == 0)
				ts->TabStopFlag |= TABF_HTS7;
			else if (_stricmp(Temp2, "HTS8") == 0)
				ts->TabStopFlag |= TABF_HTS8;
			else if (_stricmp(Temp2, "TBC") == 0)
				ts->TabStopFlag |= TABF_TBC;
			else if (_stricmp(Temp2, "TBC0") == 0)
				ts->TabStopFlag |= TABF_TBC0;
			else if (_stricmp(Temp2, "TBC3") == 0)
				ts->TabStopFlag |= TABF_TBC3;
		}
	}

	// Clipboard Access from Remote
	ts->CtrlFlag &= ~CSF_CBMASK;
	GetPrivateProfileString(Section, "ClipboardAccessFromRemote", "off", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "on") == 0 || _stricmp(Temp, "readwrite") == 0)
		ts->CtrlFlag |= CSF_CBRW;
	else if (_stricmp(Temp, "read") == 0)
		ts->CtrlFlag |= CSF_CBREAD;
	else if (_stricmp(Temp, "write") == 0)
		ts->CtrlFlag |= CSF_CBWRITE;
	else
		ts->CtrlFlag |= CSF_CBNONE; // �����������Ȃ�

	// Notify Clipboard Access from Remote
	ts->NotifyClipboardAccess = GetOnOff(Section, "NotifyClipboardAccess", FName, TRUE);

	// Use invalid DECRPSS (for testing)
	if (GetOnOff(Section, "UseInvalidDECRQSSResponse", FName, FALSE))
		ts->TermFlag |= TF_INVALIDDECRPSS;

	// ClickableUrlBrowser
	GetPrivateProfileString(Section, "ClickableUrlBrowser", "",
	                        ts->ClickableUrlBrowser, sizeof(ts->ClickableUrlBrowser), FName);
	GetPrivateProfileString(Section, "ClickableUrlBrowserArg", "",
	                        ts->ClickableUrlBrowserArg, sizeof(ts->ClickableUrlBrowserArg), FName);

	// Exclusive Lock when open the log file
	ts->LogLockExclusive = GetOnOff(Section, "LogLockExclusive", FName, TRUE);

	// Font quality
	GetPrivateProfileString(Section, "FontQuality", "default",
	                        Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "nonantialiased") == 0)
		ts->FontQuality = NONANTIALIASED_QUALITY;
	else if (_stricmp(Temp, "antialiased") == 0)
		ts->FontQuality = ANTIALIASED_QUALITY;
	else if (_stricmp(Temp, "cleartype") == 0)
		ts->FontQuality = CLEARTYPE_QUALITY;
	else
		ts->FontQuality = DEFAULT_QUALITY;

	// Beep Over Used
	ts->BeepOverUsedCount =
		GetPrivateProfileInt(Section, "BeepOverUsedCount", 5, FName);
	ts->BeepOverUsedTime =
		GetPrivateProfileInt(Section, "BeepOverUsedTime", 2, FName);
	ts->BeepSuppressTime =
		GetPrivateProfileInt(Section, "BeepSuppressTime", 5, FName);

	// Max OSC string buffer size
	ts->MaxOSCBufferSize =
		GetPrivateProfileInt(Section, "MaxOSCBufferSize", 4096, FName);

	ts->JoinSplitURL = GetOnOff(Section, "JoinSplitURL", FName, FALSE);

	GetPrivateProfileString(Section, "JoinSplitURLIgnoreEOLChar", "\\", Temp, sizeof(Temp), FName);
	ts->JoinSplitURLIgnoreEOLChar = Temp[0];

	// Debug modes.
	GetPrivateProfileString(Section, "DebugModes", "all", Temp, sizeof(Temp), FName);
	if (_stricmp(Temp, "on") == 0 || _stricmp(Temp, "all") == 0)
		ts->DebugModes = DBGF_ALL;
	else if (_stricmp(Temp, "off") == 0 || _stricmp(Temp, "none") == 0) {
		ts->DebugModes = DBGF_NONE;
		ts->Debug = FALSE;
	}
	else {
		ts->DebugModes = DBGF_NONE;
		for (i=1; GetNthString(Temp, i, sizeof(Temp2), Temp2); i++) {
			if (_stricmp(Temp2, "normal") == 0)
				ts->DebugModes |= DBGF_NORM;
			else if (_stricmp(Temp2, "hex") == 0)
				ts->DebugModes |= DBGF_HEXD;
			else if (_stricmp(Temp2, "noout") == 0)
				ts->DebugModes |= DBGF_NOUT;
		}
		if (ts->DebugModes == DBGF_NONE)
			ts->Debug = FALSE;
	}

	// Xmodem Timeout
	GetPrivateProfileString(Section, "XmodemTimeouts", "10,3,10,20,60", Temp, sizeof(Temp), FName);
	ts->XmodemTimeOutInit    = GetNthNum2(Temp, 1, 10);
	if (ts->XmodemTimeOutInit < 1)
		ts->XmodemTimeOutInit = 1;
	ts->XmodemTimeOutInitCRC = GetNthNum2(Temp, 2, 3);
	if (ts->XmodemTimeOutInitCRC < 1)
		ts->XmodemTimeOutInitCRC = 1;
	ts->XmodemTimeOutShort   = GetNthNum2(Temp, 3, 10);
	if (ts->XmodemTimeOutShort < 1)
		ts->XmodemTimeOutShort = 1;
	ts->XmodemTimeOutLong    = GetNthNum2(Temp, 4, 20);
	if (ts->XmodemTimeOutLong < 1)
		ts->XmodemTimeOutLong = 1;
	ts->XmodemTimeOutVLong   = GetNthNum2(Temp, 5, 60);
	if (ts->XmodemTimeOutVLong < 1)
		ts->XmodemTimeOutVLong = 1;

	// Ymodem Timeout
	GetPrivateProfileString(Section, "YmodemTimeouts", "10,3,10,20,60", Temp, sizeof(Temp), FName);
	ts->YmodemTimeOutInit = GetNthNum2(Temp, 1, 10);
	if (ts->YmodemTimeOutInit < 1)
		ts->YmodemTimeOutInit = 1;
	ts->YmodemTimeOutInitCRC = GetNthNum2(Temp, 2, 3);
	if (ts->YmodemTimeOutInitCRC < 1)
		ts->YmodemTimeOutInitCRC = 1;
	ts->YmodemTimeOutShort = GetNthNum2(Temp, 3, 10);
	if (ts->YmodemTimeOutShort < 1)
		ts->YmodemTimeOutShort = 1;
	ts->YmodemTimeOutLong = GetNthNum2(Temp, 4, 20);
	if (ts->YmodemTimeOutLong < 1)
		ts->YmodemTimeOutLong = 1;
	ts->YmodemTimeOutVLong = GetNthNum2(Temp, 5, 60);
	if (ts->YmodemTimeOutVLong < 1)
		ts->YmodemTimeOutVLong = 1;

	// Zmodem Timeout
	GetPrivateProfileString(Section, "ZmodemTimeouts", "10,0,10,3", Temp, sizeof(Temp), FName);
	ts->ZmodemTimeOutNormal = GetNthNum2(Temp, 1, 10);
	if (ts->ZmodemTimeOutNormal < 1)
		ts->ZmodemTimeOutNormal = 1;
	ts->ZmodemTimeOutTCPIP = GetNthNum2(Temp, 2, 0);
	if (ts->ZmodemTimeOutTCPIP < 0)
		ts->ZmodemTimeOutTCPIP = 0;
	ts->ZmodemTimeOutInit = GetNthNum2(Temp, 3, 10);
	if (ts->ZmodemTimeOutInit < 1)
		ts->ZmodemTimeOutInit = 1;
	ts->ZmodemTimeOutFin = GetNthNum2(Temp, 4, 3);
	if (ts->ZmodemTimeOutFin < 1)
		ts->ZmodemTimeOutFin = 1;

	// Trim trailing new line character when pasting
	if (GetOnOff(Section, "TrimTrailingNLonPaste", FName, FALSE))
		ts->PasteFlag |= CPF_TRIM_TRAILING_NL;

	// List Inactive Font
	ts->ListHiddenFonts = GetOnOff(Section, "ListHiddenFonts", FName, FALSE);

	// ISO2022ShiftFunction
	GetPrivateProfileString(Section, "ISO2022ShiftFunction", "on", Temp, sizeof(Temp), FName);
	ts->ISO2022Flag = ISO2022_SHIFT_NONE;
	for (i=1; GetNthString(Temp, i, sizeof(Temp2), Temp2); i++) {
		BOOL add=TRUE;
		char *p = Temp2, *p2;
		int mask = 0;

		while (*p == ' ' || *p == '\t') {
			p++;
		}
		p2 = p + strlen(p);
		while (p2 > p) {
			p2--;
			if (*p2 != ' ' && *p2 != '\t') {
				break;
			}
		}
		*++p2 = 0;

		if (*p == '-') {
			p++;
			add=FALSE;
		}
		else if (*p == '+') {
			p++;
		}

		if (_stricmp(p, "on") == 0 || _stricmp(p, "all") == 0)
			ts->ISO2022Flag = ISO2022_SHIFT_ALL;
		else if (_stricmp(p, "off") == 0 || _stricmp(p, "none") == 0)
			ts->ISO2022Flag = ISO2022_SHIFT_NONE;
		else if (_stricmp(p, "SI") == 0 || _stricmp(p, "LS0") == 0)
			mask = ISO2022_SI;
		else if (_stricmp(p, "SO") == 0 || _stricmp(p, "LS1") == 0)
			mask = ISO2022_SO;
		else if (_stricmp(p, "LS2") == 0)
			mask = ISO2022_LS2;
		else if (_stricmp(p, "LS3") == 0)
			mask = ISO2022_LS3;
		else if (_stricmp(p, "LS1R") == 0)
			mask = ISO2022_LS1R;
		else if (_stricmp(p, "LS2R") == 0)
			mask = ISO2022_LS2R;
		else if (_stricmp(p, "LS3R") == 0)
			mask = ISO2022_LS3R;
		else if (_stricmp(p, "SS2") == 0)
			mask = ISO2022_SS2;
		else if (_stricmp(p, "SS3") == 0)
			mask = ISO2022_SS3;

		if (mask) {
			if (add) {
				ts->ISO2022Flag |= mask;
			}
			else {
				ts->ISO2022Flag &= ~mask;
			}
		}
	}

	// Terminal Speed (Used by telnet and ssh)
	GetPrivateProfileString(Section, "TerminalSpeed", "38400", Temp, sizeof(Temp), FName);
	GetNthNum(Temp,  1, &i);
	if (i > 0)
		ts->TerminalInputSpeed = i;
	else
		ts->TerminalInputSpeed = 38400;
	GetNthNum(Temp,  2, &i);
	if (i > 0)
		ts->TerminalOutputSpeed = i;
	else
		ts->TerminalOutputSpeed = ts->TerminalInputSpeed;

	// Clear scroll buffer from remote -- special option
	if (GetOnOff(Section, "ClearScrollBufferFromRemote", FName, TRUE))
		ts->TermFlag |= TF_REMOTECLEARSBUFF;

	// Delay for start of mouse selection
	ts->SelectStartDelay =
		GetPrivateProfileInt(Section, "MouseSelectStartDelay", 0, FName);

	// Fallback CP932 (Experimental)
	ts->FallbackToCP932 = GetOnOff(Section, "FallbackToCP932", FName, FALSE);

	// dialog font
	ReadFont3(L"Tera Term", L"DlgFont", NULL, FName,
			  ts->DialogFontNameW, _countof(ts->DialogFontNameW),
			  &ts->DialogFontPoint, &ts->DialogFontCharSet);

	// Unicode�ݒ�
	ts->UnicodeAmbiguousWidth = GetPrivateProfileInt(Section, "UnicodeAmbiguousWidth", 0, FName);
	if (ts->UnicodeAmbiguousWidth < 1 || 2 < ts->UnicodeAmbiguousWidth) {
		ts->UnicodeAmbiguousWidth = GetDefaultUnicodeWidth();
	}
	ts->UnicodeEmojiOverride = (BYTE)GetOnOff(Section, "UnicodeEmojiOverride", FName, FALSE);
	ts->UnicodeEmojiWidth = GetPrivateProfileInt(Section, "UnicodeEmojiWidth", 0, FName);
	if (ts->UnicodeEmojiWidth < 1 || 2 < ts->UnicodeEmojiWidth) {
		ts->UnicodeEmojiWidth = GetDefaultUnicodeWidth();
	}
	DispEnableResizedFont(GetOnOff(Section, "DrawingResizedFont", FName, TRUE));

	DispReadIni(FName, ts);

	// rounded corner preference for VT/TEK window
	ts->WindowCornerDontround = GetOnOff(Section, "WindowCornerDontround", FName, FALSE);

	// �ʒm��
	ts->NotifySound = GetOnOff(Section, "NotifySound", FName, TRUE);

	// �����o�b�N�A�b�v
	ts->IniAutoBackup = GetOnOff(Section, "IniAutoBackup", FName, TRUE);

	// Bracketed paste mode
	ts->BracketedSupport = GetOnOff(Section, "BracketedSupport", FName, TRUE);
	ts->BracketedControlOnly = GetOnOff(Section, "BracketedControlOnly", FName, FALSE);

	// Sendfile
	GetPrivateProfileString(Section, "SendfileDelayType", "NoDelay", Temp, sizeof(Temp), FName);
	ts->SendfileDelayType =
		(_stricmp(Temp, "PerChar") == 0) ? 1 :
		(_stricmp(Temp, "PerLine") == 0) ? 2 :
		(_stricmp(Temp, "PerSendSize") == 0) ? 3 : /*NoDelay*/ 0;
	ts->SendfileDelayTick = GetPrivateProfileInt(Section, "SendfileDelayTick", 0, FName);
	ts->SendfileSize = GetPrivateProfileInt(Section, "SendfileSize", 4096, FName);
	ts->SendfileSequential = GetOnOff(Section, "SendfileSequential", FName, FALSE);
	ts->SendfileSkipOptionDialog = GetOnOff(Section, "SendfileSkipOptionDialog", FName, FALSE);

	// Experimental
	ts->ExperimentalTreePropertySheetEnable = GetOnOff("Experimental", "TreeProprtySheet", FName, FALSE);
	ts->ExperimentalTreePropertySheetEnable = GetOnOff("Experimental", "TreePropertySheet", FName, ts->ExperimentalTreePropertySheetEnable);
}

/**
 *	UILanguage File �� ExeDir���΃p�X�ɕϊ�
 *
 *	@return ����UILanguageFile�p�X�A�s�v�ɂȂ�����free()���邱��
 */
static wchar_t *GetUILanguageFileRelPath(const wchar_t *UILanguageFile)
{
	wchar_t *ExeDirW = GetExeDirW(NULL);
	size_t ExeDirLen = wcslen(ExeDirW);
	int r = wcsncmp(ExeDirW, UILanguageFile, ExeDirLen);
	free(ExeDirW);
	if (r != 0) {
		// ExeDir �t�H���_�ȉ��� lng �t�@�C���ł͂Ȃ��̂ł��̂܂ܕԂ�
		return _wcsdup(UILanguageFile);
	}

	//   ExeDir���΂ɕϊ�����
	wchar_t *UILanguageFileRel = _wcsdup(UILanguageFile + ExeDirLen + 1);
	return UILanguageFileRel;
}

void PASCAL _WriteIniFile(const wchar_t *FName, PTTSet ts)
{
	int i;
	char Temp[MAX_PATH];
	char buf[20];
	int ret;
	WORD TmpColor[12][6];
	wchar_t *TempW;

	WriteIniBom(FName, FALSE);

	/* version */
	ret = WritePrivateProfileString(Section, "Version", TT_VERSION_STR("."), FName);
	if (ret == 0) {
		// ini�t�@�C���̏������ݎ��s
		wchar_t *msg, *msg_err, *title;
		ret = GetLastError();
		GetI18nStrWW("Tera Term", "MSG_INI_WRITE_ERROR", L"Cannot write ini file", ts->UILanguageFileW, &msg);
		aswprintf(&msg_err, L"%s (%d)", msg, ret);
		free(msg);

		GetI18nStrWW("Tera Term", "MSG_INI_ERROR", L"Tera Term: Error", ts->UILanguageFileW, &title);

		MessageBoxW(NULL, msg_err, title, MB_ICONEXCLAMATION);
		free(msg_err);
		free(title);
	}

	/* Port type */
	WritePrivateProfileString(Section, "Port", (ts->PortType==IdSerial)?"serial":"tcpip", FName);

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
	else if (ts->CRReceive == IdAUTO) {
		strncpy_s(Temp, sizeof(Temp), "AUTO", _TRUNCATE);
	}
	else {
		strncpy_s(Temp, sizeof(Temp), "CR", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "CRReceive", Temp, FName);

	/* CR Send */
	if (ts->CRSend == IdCRLF) {
		strncpy_s(Temp, sizeof(Temp), "CRLF", _TRUNCATE);
	}
	else if (ts->CRSend == IdLF) {
		strncpy_s(Temp, sizeof(Temp), "LF", _TRUNCATE);
	}
	else {
		strncpy_s(Temp, sizeof(Temp), "CR", _TRUNCATE);
	}
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
	{
		const char *code_str = GetKanjiCodeStr(ts->KanjiCode);
		WritePrivateProfileString(Section, "KanjiReceive", code_str, FName);
	}

	/* Katakana (receive)  */
	if (ts->JIS7Katakana == 1)
		strncpy_s(Temp, sizeof(Temp), "7", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "8", _TRUNCATE);

	WritePrivateProfileString(Section, "KatakanaReceive", Temp, FName);

	/* Kanji Code (transmit)  */
	{
		const char *code_str = GetKanjiCodeStr(ts->KanjiCodeSend);
		WritePrivateProfileString(Section, "KanjiSend", code_str, FName);
	}

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
	           (WORD) (ts->PasteFlag & CPF_DISABLE_RBUTTON));

	WriteOnOff(Section, "DisablePasteMouseMButton", FName,
	           (WORD) (ts->PasteFlag & CPF_DISABLE_MBUTTON));

	WriteOnOff(Section, "ConfirmPasteMouseRButton", FName,
	           (WORD) (ts->PasteFlag & CPF_CONFIRM_RBUTTON));

	// added ConfirmChangePaste
	WriteOnOff(Section, "ConfirmChangePaste", FName,
	           (WORD) (ts->PasteFlag & CPF_CONFIRM_CHANGEPASTE));

	WriteOnOff(Section, "ConfirmChangePasteCR", FName,
	           (WORD) (ts->PasteFlag & CPF_CONFIRM_CHANGEPASTE_CR));

	WritePrivateProfileString(Section, "ConfirmChangePasteStringFile",
	                          ts->ConfirmChangePasteStringFile, FName);

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
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->AlphaBlendInactive);
	WritePrivateProfileString(Section, "AlphaBlend", Temp, FName);
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->AlphaBlendActive);
	WritePrivateProfileString(Section, "AlphaBlendActive", Temp, FName);
	WritePrivateProfileString(Section, "CygwinDirectory",
	                          ts->CygwinDirectory, FName);
	WritePrivateProfileStringW(SectionW, L"ViewlogEditor", ts->ViewlogEditorW, FName);
	WritePrivateProfileStringW(SectionW, L"ViewlogEditorArg", ts->ViewlogEditorArg, FName);

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
		TmpColor[0][i * 3] = GetRValue(ts->VTColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->VTColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->VTColor[i]);
	}
	WriteInt6(Section, "VTColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	/* VT Bold Color */
	for (i = 0; i <= 1; i++) {
		TmpColor[0][i * 3] = GetRValue(ts->VTBoldColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->VTBoldColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->VTBoldColor[i]);
	}
	WriteInt6(Section, "VTBoldColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	/* VT Blink Color */
	for (i = 0; i <= 1; i++) {
		TmpColor[0][i * 3] = GetRValue(ts->VTBlinkColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->VTBlinkColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->VTBlinkColor[i]);
	}
	WriteInt6(Section, "VTBlinkColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	/* VT Reverse Color */
	for (i = 0; i <= 1; i++) {
		TmpColor[0][i * 3] = GetRValue(ts->VTReverseColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->VTReverseColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->VTReverseColor[i]);
	}
	WriteInt6(Section, "VTReverseColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	WriteOnOff(Section, "EnableClickableUrl", FName,
	           ts->EnableClickableUrl);

	/* Underline */
	WriteOnOff(Section, "UnderlineAttrFont", FName,
	           (WORD) (ts->FontFlag & FF_UNDERLINE));
	WriteOnOff(Section, "UnderlineAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_UNDERLINE));
	for (i = 0; i <= 1; i++) {
		TmpColor[0][i * 3] = GetRValue(ts->VTUnderlineColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->VTUnderlineColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->VTUnderlineColor[i]);
	}
	WriteInt6(Section, "VTUnderlineColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	/* URL color */
	for (i = 0; i <= 1; i++) {
		TmpColor[0][i * 3] = GetRValue(ts->URLColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->URLColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->URLColor[i]);
	}
	WriteInt6(Section, "URLColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	WriteOnOff(Section, "EnableBoldAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_BOLDCOLOR));

	WriteOnOff(Section, "EnableBlinkAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_BLINKCOLOR));

	WriteOnOff(Section, "EnableReverseAttrColor", FName,
	           (WORD) (ts->ColorFlag & CF_REVERSECOLOR));

	WriteOnOff(Section, "EnableURLColor", FName,
	           (WORD) (ts->ColorFlag & CF_URLCOLOR));

	WriteOnOff(Section, "URLUnderline", FName,
	           (WORD) (ts->FontFlag & FF_URLUNDERLINE));

	WriteOnOff(Section, "EnableANSIColor", FName,
	           (WORD) (ts->ColorFlag & CF_ANSICOLOR));

	/* TEK Color */
	for (i = 0; i <= 1; i++) {
		TmpColor[0][i * 3] = GetRValue(ts->TEKColor[i]);
		TmpColor[0][i * 3 + 1] = GetGValue(ts->TEKColor[i]);
		TmpColor[0][i * 3 + 2] = GetBValue(ts->TEKColor[i]);
	}
	WriteInt6(Section, "TEKColor", FName,
	          TmpColor[0][0], TmpColor[0][1], TmpColor[0][2],
	          TmpColor[0][3], TmpColor[0][4], TmpColor[0][5]);

	/* TEK color emulation */
	WriteOnOff(Section, "TEKColorEmulation", FName, ts->TEKColorEmu);

	/* VT Font */
	WriteFont(Section, "VTFont", FName,
	          ts->VTFont, ts->VTFontSize.x, ts->VTFontSize.y,
	          ts->VTFontCharSet);

	/* Enable bold font flag */
	WriteOnOff(Section, "EnableBold", FName,
		(WORD) (ts->FontFlag & FF_BOLD));

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
	switch (ts->MetaKey) {
	case 1:
		strncpy_s(Temp, sizeof(Temp), "on", _TRUNCATE);
		break;
	case 2:
		strncpy_s(Temp, sizeof(Temp), "left", _TRUNCATE);
		break;
	case 3:
		strncpy_s(Temp, sizeof(Temp), "right", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "off", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "Metakey", Temp, FName);

	/* Application Keypad */
	WriteOnOff(Section, "DisableAppKeypad", FName, ts->DisableAppKeypad);

	/* Application Cursor */
	WriteOnOff(Section, "DisableAppCursor", FName, ts->DisableAppCursor);

	/* Russian keyboard type */
	id2str(RussList2, ts->RussKeyb, /*IdWindows*/0, Temp, sizeof(Temp));
	WritePrivateProfileString(Section, "RussKeyb", Temp, FName);

	/* Serial port ID */
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->ComPort);
	WritePrivateProfileString(Section, "ComPort", Temp, FName);

	/* Baud rate */
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d", ts->Baud);
	WritePrivateProfileString(Section, "BaudRate", Temp, FName);

	/* Parity */
	if (!SerialPortConfconvertId2Str(COM_PARITY, ts->Parity, Temp, sizeof(Temp))) {
		strncpy_s(Temp, sizeof(Temp), "none", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "Parity", Temp, FName);

	/* Data bit */
	if (!SerialPortConfconvertId2Str(COM_DATABIT, ts->DataBit, Temp, sizeof(Temp))) {
		strncpy_s(Temp, sizeof(Temp), "8", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "DataBit", Temp, FName);

	/* Stop bit */
	if (!SerialPortConfconvertId2Str(COM_STOPBIT, ts->StopBit, Temp, sizeof(Temp))) {
		strncpy_s(Temp, sizeof(Temp), "1", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "StopBit", Temp, FName);

	/* Flow control */
	if (!SerialPortConfconvertId2Str(COM_FLOWCTRL, ts->Flow, Temp, sizeof(Temp))) {
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

	/* Log binary flag */
	WriteOnOff(Section, "LogBinary", FName, ts->LogBinary);

	/* Log append */
	WriteOnOff(Section, "LogAppend", FName, ts->Append);

	/* Log plain text flag */
	WriteOnOff(Section, "LogTypePlainText", FName, ts->LogTypePlainText);

	/* Log with timestamp (2006.7.23 maya) */
	WriteOnOff(Section, "LogTimestamp", FName, ts->LogTimestamp);

	/* Log without transfer dialog */
	WriteOnOff(Section, "LogHideDialog", FName, ts->LogHideDialog);

	WriteOnOff(Section, "LogIncludeScreenBuffer", FName, ts->LogAllBuffIncludedInFirst);

	/* Timestamp format of Log each line */
	WritePrivateProfileString(Section, "LogTimestampFormat",
	                          ts->LogTimestampFormat, FName);

	/* Timestamp type */
	switch (ts->LogTimestampType) {
	case TIMESTAMP_LOCAL:
		WritePrivateProfileString(Section, "LogTimestampType", "Local", FName);
		break;
	case TIMESTAMP_UTC:
		WritePrivateProfileString(Section, "LogTimestampType", "UTC", FName);
		break;
	case TIMESTAMP_ELAPSED_LOGSTART:
		WritePrivateProfileString(Section, "LogTimestampType", "LoggingElapsed", FName);
		break;
	case TIMESTAMP_ELAPSED_CONNECTED:
		WritePrivateProfileString(Section, "LogTimestampType", "ConnectionElapsed", FName);
		break;
	}

	/* Default Log file name */
	WritePrivateProfileStringW(SectionW, L"LogDefaultName",
							   ts->LogDefaultNameW, FName);

	/* Default Log file path */
	TempW = NULL;
	if (ts->LogDefaultPathW != NULL &&
		wcscmp(ts->LogDefaultPathW, ts->LogDirW) != 0) {
		// �ݒ肳��Ă��� && �قȂ��Ă���Ƃ��A�t�H���_���w�肵�Ă�
		TempW = ts->LogDefaultPathW;
	}
	else {
		// �ݒ肳��Ă��Ȃ�, �폜
		TempW = NULL;
	}
	WritePrivateProfileStringW(SectionW, L"LogDefaultPath", TempW, FName);

	/* Auto start logging (2007.5.31 maya) */
	WriteOnOff(Section, "LogAutoStart", FName, ts->LogAutoStart);

	/* Log Rotate (2013.3.24 yutaka) */
	WriteInt(Section, "LogRotate", FName, ts->LogRotate);
	WriteInt(Section, "LogRotateSize", FName, ts->LogRotateSize);
	WriteInt(Section, "LogRotateSizeType", FName, ts->LogRotateSizeType);
	WriteInt(Section, "LogRotateStep", FName, ts->LogRotateStep);

	/* Deferred Log Write Mode (2013.4.20 yutaka) */
	WriteOnOff(Section, "DeferredLogWriteMode", FName, ts->DeferredLogWriteMode);

	/* XMODEM option */
	switch (ts->XmodemOpt) {
	case XoptCRC:
		strncpy_s(Temp, sizeof(Temp), "crc", _TRUNCATE);
		break;
	case Xopt1kCRC:
		strncpy_s(Temp, sizeof(Temp), "1k", _TRUNCATE);
		break;
	case Xopt1kCksum:
		strncpy_s(Temp, sizeof(Temp), "1ksum", _TRUNCATE);
		break;
	default:
		strncpy_s(Temp, sizeof(Temp), "checksum", _TRUNCATE);
	}
	WritePrivateProfileString(Section, "XmodemOpt", Temp, FName);

	/* XMODEM binary flag */
	WriteOnOff(Section, "XmodemBin", FName, ts->XmodemBin);

	/* XMODEM ��M�R�}���h (2007.12.21 yutaka) */
	WritePrivateProfileString(Section, "XmodemRcvCommand",
	                          ts->XModemRcvCommand, FName);

	/* Default directory for file transfer */
	WritePrivateProfileStringW(SectionW, L"FileDir", ts->FileDirW, FName);

	/* filter on file send (2007.6.5 maya) */
	WritePrivateProfileString(Section, "FileSendFilter",
	                          ts->FileSendFilter, FName);

	WritePrivateProfileString(Section, "ScpSendDir", ts->ScpSendDir, FName);

/*------------------------------------------------------------------*/
	/* 8 bit control code flag  -- special option */
	WriteOnOff(Section, "Accept8BitCtrl", FName,
	           (WORD) (ts->TermFlag & TF_ACCEPT8BITCTRL));

	/* Wrong sequence flag  -- special option */
	WriteOnOff(Section, "AllowWrongSequence", FName,
	           (WORD) (ts->TermFlag & TF_ALLOWWRONGSEQUENCE));

	/* Detect disconnect/reconnect of serial port --- special option */
	WriteOnOff(Section, "AutoComPortReconnect", FName, ts->AutoComPortReconnect);

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

	/* Wait time (ms) when Beep is Visual Bell -- special option */
	WriteInt(Section, "BeepVBellWait", FName, ts->BeepVBellWait);

	/* Auto B-Plus activation -- special option */
	WriteOnOff(Section, "BPAuto", FName, (WORD) (ts->FTFlag & FT_BPAUTO));

	/* B-Plus ESCCTL flag  -- special option */
	WriteOnOff(Section, "BPEscCtl", FName,
	           (WORD) (ts->FTFlag & FT_BPESCCTL));

	/* B-Plus log  -- special option */
	WriteOnOff(Section, "BPLog", FName, (WORD) (ts->LogFlag & LOG_BP));

	/* Clear serial port buffer when port opening -- special option */
	WriteOnOff(Section, "ClearComBuffOnOpen", FName, ts->ClearComBuffOnOpen);

	/* When serial port is specified with with /C= option and the port does not exist, Tera Term will wait for port connection.  */
	WriteOnOff(Section, "WaitCom", FName, ts->WaitCom);

	/* Confirm disconnection -- special option */
	WriteOnOff(Section, "ConfirmDisconnect", FName,
	           (WORD) (ts->PortFlag & PF_CONFIRMDISCONN));

	/* Ctrl code in Kanji -- special option */
	WriteOnOff(Section, "CtrlInKanji", FName,
	           (WORD) (ts->TermFlag & TF_CTRLINKANJI));

	/* Debug flag  -- special option */
	WriteOnOff(Section, "Debug", FName, ts->Debug);

	/* Delimiter list -- special option */
	{
		wchar_t *s = Str2HexW(ts->DelimListW, 0, TRUE);
		WritePrivateProfileStringW(SectionW, L"DelimList", s, FName);
		free(s);
	}

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

	/* Enable multiple bytes send -- special option */
	WriteOnOff(Section, "FileSendHighSpeedMode", FName, ts->FileSendHighSpeedMode);

	/* IME Flag  -- special option */
	WriteOnOff(Section, "IME", FName, ts->UseIME);

	/* IME-inline Flag  -- special option */
	WriteOnOff(Section, "IMEInline", FName, ts->IMEInline);

	/* Kermit log  -- special option */
	WriteOnOff(Section, "KmtLog", FName, (WORD) (ts->LogFlag & LOG_KMT));
	WriteOnOff(Section, "KmtLongPacket", FName, (WORD) (ts->KermitOpt & KmtOptLongPacket));
	WriteOnOff(Section, "KmtFileAttr", FName, (WORD) (ts->KermitOpt & KmtOptFileAttr));

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

	/* �v�����^�p����R�[�h���󂯕t���邩 */
	WriteOnOff(Section, "PrinterCtrlSequence", FName,
		ts->TermFlag & TF_PRINTERCTRL);

	/* Printer Font --- special option */
	WriteFont(Section, "PrnFont", FName,
	          ts->PrnFont, ts->PrnFontSize.x, ts->PrnFontSize.y,
	          ts->PrnFontCharSet);

	// Page margins (left, right, top, bottom) for printing
	//    -- special option
	WriteInt4(Section, "PrnMargin", FName,
	          ts->PrnMargin[0], ts->PrnMargin[1],
	          ts->PrnMargin[2], ts->PrnMargin[3]);

	/* Disable (convert to NL) Form Feed when printing */
	/*   --- special option */
	WriteOnOff(Section, "PrnConvFF", FName, ts->PrnConvFF);

	/* Quick-VAN log  -- special option */
	WriteOnOff(Section, "QVLog", FName, (WORD) (ts->LogFlag & LOG_QV));

	/* Quick-VAN window size -- special */
	WriteInt(Section, "QVWinSize", FName, ts->QVWinSize);

	/* Scroll threshold -- special option */
	WriteInt(Section, "ScrollThreshold", FName, ts->ScrollThreshold);

	WriteInt(Section, "MouseWheelScrollLine", FName, ts->MouseWheelScrollLine);

	// Select on activate -- special option
	WriteOnOff(Section, "SelectOnActivate", FName, ts->SelOnActive);

	/* Send 8bit control sequence -- special option */
	WriteOnOff(Section, "Send8BitCtrl", FName, ts->Send8BitCtrl);

	/* SendBreak time (in msec) -- special option */
	WriteInt(Section, "SendBreakTime", FName, ts->SendBreakTime);

	/* Startup macro -- special option */
	WritePrivateProfileStringW(SectionW, L"StartupMacro", ts->MacroFNW, FName);

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

	/* YMODEM ��M�R�}���h (2010.3.23 yutaka) */
	WritePrivateProfileString(Section, "YmodemRcvCommand", ts->YModemRcvCommand, FName);

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

	/* ZMODEM ��M�R�}���h (2007.12.21 yutaka) */
	WritePrivateProfileString(Section, "ZmodemRcvCommand", ts->ZModemRcvCommand, FName);

	DispWriteIni(FName, ts);

	// theme�t�H���_�����
#if 0	// Tera Term ���t�@�C���ۑ����ɍ��?
	{
#define BG_THEME_DIR L"theme"
		wchar_t *theme_folder = NULL;
		awcscats(&theme_folder, ts->HomeDirW, L"\\", BG_THEME_DIR, NULL);
		CreateDirectoryW(theme_folder, NULL);
		free(theme_folder);
	}
#endif

	// UseNormalBGColor
	WriteOnOff(Section, "UseNormalBGColor", FName, ts->UseNormalBGColor);

	// UI language message file
	TempW = GetUILanguageFileRelPath(ts->UILanguageFileW);
	WritePrivateProfileStringW(SectionW, L"UILanguageFile", TempW, FName);
	free(TempW);

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
	WriteInt(Section, "MaximizedBugTweak", FName, ts->MaximizedBugTweak);

	WritePrivateProfileIntW(
		SectionW, L"DecSpMappingDir",
		(INT)(ts->Dec2Unicode == 0 ? IdDecSpecialUniToDec :
			  ts->Dec2Unicode == 1 ? IdDecSpecialDecToUni : IdDecSpecialDoNot),
		FName);

	// Convert Unicode symbol characters to DEC Special characters
	WriteUint(Section, "UnicodeToDecSpMapping", FName, ts->UnicodeDecSpMapping);

	// AutoScrollOnlyInBottomLine
	WriteOnOff(Section, "AutoScrollOnlyInBottomLine", FName,
		ts->AutoScrollOnlyInBottomLine);

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

	WriteOnOff(Section, "AcceleratorNewConnection", FName,
	           ts->AcceleratorNewConnection);

	WriteOnOff(Section, "AcceleratorCygwinConnection", FName,
	           ts->AcceleratorCygwinConnection);

	// DisableMenuDuplicateSession
	WriteOnOff(Section, "DisableMenuDuplicateSession", FName,
	           ts->DisableMenuDuplicateSession);

	// DisableMenuNewConnection
	WriteOnOff(Section, "DisableMenuNewConnection", FName,
	           ts->DisableMenuNewConnection);

	// added PasteDelayPerLine (2009.4.12 maya)
	WriteInt(Section, "PasteDelayPerLine", FName,
	         ts->PasteDelayPerLine);

	// Meta sets MSB
	switch (ts->Meta8Bit) {
	  case IdMeta8BitRaw:
		WritePrivateProfileString(Section, "Meta8Bit", "raw", FName);
		break;
	  case IdMeta8BitText:
		WritePrivateProfileString(Section, "Meta8Bit", "text", FName);
		break;
	  default:
		WritePrivateProfileString(Section, "Meta8Bit", "off", FName);
	}

	// Window control sequence
	WriteOnOff(Section, "WindowCtrlSequence", FName,
		ts->WindowFlag & WF_WINDOWCHANGE);

	// Cursor control sequence
	WriteOnOff(Section, "CursorCtrlSequence", FName,
		ts->WindowFlag & WF_CURSORCHANGE);

	// Window report sequence
	WriteOnOff(Section, "WindowReportSequence", FName,
		ts->WindowFlag & WF_WINDOWREPORT);

	// Title report sequence
	switch (ts->WindowFlag & WF_TITLEREPORT) {
	case IdTitleReportIgnore:
		WritePrivateProfileString(Section, "TitleReportSequence", "ignore", FName);
		break;
	case IdTitleReportAccept:
		WritePrivateProfileString(Section, "TitleReportSequence", "accept", FName);
		break;
	default: // IdTitleReportEmpty
		WritePrivateProfileString(Section, "TitleReportSequence", "empty", FName);
		break;
	}

	// Line at a time mode
	WriteOnOff(Section, "EnableLineMode", FName, ts->EnableLineMode);

	// Clear window on resize
	WriteOnOff(Section, "ClearOnResize", FName,
		ts->TermFlag & TF_CLEARONRESIZE);

	// Alternate Screen Buffer
	WriteOnOff(Section, "AlternateScreenBuffer", FName,
		ts->TermFlag & TF_ALTSCR);

	// IME status related cursor style
	WriteOnOff(Section, "IMERelatedCursor", FName,
		ts->WindowFlag & WF_IMECURSORCHANGE);

	// Terminal Unique ID
	WritePrivateProfileString(Section, "TerminalUID", ts->TerminalUID, FName);

	// Lock Terminal UID
	WriteOnOff(Section, "LockTUID", FName, ts->TermFlag & TF_LOCKTUID);

	// Jump List
	WriteOnOff(Section, "JumpList", FName, ts->JumpList);

	// TabStopModifySequence
	switch (ts->TabStopFlag) {
	case TABF_ALL:
		strncpy_s(Temp, sizeof(Temp), "on", _TRUNCATE);
		break;
	case TABF_NONE:
		strncpy_s(Temp, sizeof(Temp), "off", _TRUNCATE);
		break;
	default:
		switch (ts->TabStopFlag & TABF_HTS) {
			case TABF_HTS7: strncpy_s(Temp, sizeof(Temp), "HTS7", _TRUNCATE); break;
			case TABF_HTS8: strncpy_s(Temp, sizeof(Temp), "HTS8", _TRUNCATE); break;
			case TABF_HTS:  strncpy_s(Temp, sizeof(Temp), "HTS",  _TRUNCATE); break;
			default: Temp[0] = 0; break;
		}

		if (ts->TabStopFlag & TABF_TBC) {
			if (Temp[0] != 0) {
				strncat_s(Temp, sizeof(Temp), ",", _TRUNCATE);
			}
			switch (ts->TabStopFlag & TABF_TBC) {
				case TABF_TBC0: strncat_s(Temp, sizeof(Temp), "TBC0", _TRUNCATE); break;
				case TABF_TBC3: strncat_s(Temp, sizeof(Temp), "TBC3", _TRUNCATE); break;
				case TABF_TBC:  strncat_s(Temp, sizeof(Temp), "TBC",  _TRUNCATE); break;
			}
		}

		if (Temp[0] == 0) { // �����͂�������ǔO�̂���
			strncpy_s(Temp, sizeof(Temp), "off", _TRUNCATE);
		}
		break;
	}
	WritePrivateProfileString(Section, "TabStopModifySequence", Temp, FName);

	// Clipboard Access from Remote
	switch (ts->CtrlFlag & CSF_CBMASK) {
	case CSF_CBREAD:
		WritePrivateProfileString(Section, "ClipboardAccessFromRemote", "read", FName);
		break;
	case CSF_CBWRITE:
		WritePrivateProfileString(Section, "ClipboardAccessFromRemote", "write", FName);
		break;
	case CSF_CBRW:
		WritePrivateProfileString(Section, "ClipboardAccessFromRemote", "on", FName);
		break;
	default:
		WritePrivateProfileString(Section, "ClipboardAccessFromRemote", "off", FName);
		break;
	}

	// Notify Clipboard Access from Remote
	WriteOnOff(Section, "NotifyClipboardAccess", FName, ts->NotifyClipboardAccess);

	// ClickableUrlBrowser
	WritePrivateProfileString(Section, "ClickableUrlBrowser", ts->ClickableUrlBrowser, FName);
	WritePrivateProfileString(Section, "ClickableUrlBrowserArg", ts->ClickableUrlBrowserArg, FName);

	// Exclusive Lock when open the log file
	WriteOnOff(Section, "LogLockExclusive", FName, ts->LogLockExclusive);

	// Font quality
	if (ts->FontQuality == NONANTIALIASED_QUALITY)
		strncpy_s(Temp, sizeof(Temp), "nonantialiased", _TRUNCATE);
	else if (ts->FontQuality == ANTIALIASED_QUALITY)
		strncpy_s(Temp, sizeof(Temp), "antialiased", _TRUNCATE);
	else if (ts->FontQuality == CLEARTYPE_QUALITY)
		strncpy_s(Temp, sizeof(Temp), "cleartype", _TRUNCATE);
	else
		strncpy_s(Temp, sizeof(Temp), "default", _TRUNCATE);
	WritePrivateProfileString(Section, "FontQuality", Temp, FName);

	// Beep Over Used
	WriteInt(Section, "BeepOverUsedCount", FName, ts->BeepOverUsedCount);
	WriteInt(Section, "BeepOverUsedTime", FName, ts->BeepOverUsedTime);
	WriteInt(Section, "BeepSuppressTime", FName, ts->BeepSuppressTime);

	// Max OSC string buffer size
	WriteInt(Section, "MaxOSCBufferSize", FName, ts->MaxOSCBufferSize);

	WriteOnOff(Section, "JoinSplitURL", FName, ts->JoinSplitURL);

	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%c", ts->JoinSplitURLIgnoreEOLChar);
	WritePrivateProfileString(Section, "JoinSplitURLIgnoreEOLChar", Temp, FName);

	// Debug modes.
	if (ts->DebugModes == DBGF_ALL) {
		strncpy_s(Temp, sizeof(Temp), "all", _TRUNCATE);
	}
	else {
		if (ts->DebugModes & DBGF_NORM) {
			strncpy_s(Temp, sizeof(Temp), "normal", _TRUNCATE);
		}
		else {
			Temp[0] = 0;
		}

		if (ts->DebugModes & DBGF_HEXD) {
			if (Temp[0] != 0) {
				strncat_s(Temp, sizeof(Temp), ",", _TRUNCATE);
			}
			strncat_s(Temp, sizeof(Temp), "hex", _TRUNCATE);
		}

		if (ts->DebugModes & DBGF_NOUT) {
			if (Temp[0] != 0) {
				strncat_s(Temp, sizeof(Temp), ",", _TRUNCATE);
			}
			strncat_s(Temp, sizeof(Temp), "noout", _TRUNCATE);
		}

		if (Temp[0] == 0) {
			strncpy_s(Temp, sizeof(Temp), "none", _TRUNCATE);
		}
	}
	WritePrivateProfileString(Section, "DebugModes", Temp, FName);

	// Xmodem Timeout
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d,%d",
		ts->XmodemTimeOutInit,
		ts->XmodemTimeOutInitCRC,
		ts->XmodemTimeOutShort,
		ts->XmodemTimeOutLong,
		ts->XmodemTimeOutVLong
	);
	WritePrivateProfileString(Section, "XmodemTimeouts", Temp, FName);

	// Ymodem Timeout
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d,%d",
		ts->YmodemTimeOutInit,
		ts->YmodemTimeOutInitCRC,
		ts->YmodemTimeOutShort,
		ts->YmodemTimeOutLong,
		ts->YmodemTimeOutVLong
		);
	WritePrivateProfileString(Section, "YmodemTimeouts", Temp, FName);

	// Zmodem Timeout
	_snprintf_s(Temp, sizeof(Temp), _TRUNCATE, "%d,%d,%d,%d",
		ts->ZmodemTimeOutNormal,
		ts->ZmodemTimeOutTCPIP,
		ts->ZmodemTimeOutInit,
		ts->ZmodemTimeOutFin
		);
	WritePrivateProfileString(Section, "ZmodemTimeouts", Temp, FName);

	// Trim trailing new line character when pasting
	WriteOnOff(Section, "TrimTrailingNLonPaste", FName,
		(WORD) (ts->PasteFlag & CPF_TRIM_TRAILING_NL));

	// List Inactive Font
	WriteOnOff(Section, "ListHiddenFonts", FName, ts->ListHiddenFonts);

	// ISO2022ShiftFunction
	if (ts->ISO2022Flag == ISO2022_SHIFT_ALL) {
		strncpy_s(Temp, sizeof(Temp), "on", _TRUNCATE);
	}
	else {
		Temp[0]=0;
		if (ts->ISO2022Flag & ISO2022_SI) {
			strncat_s(Temp, sizeof(Temp), "SI,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_SO) {
			strncat_s(Temp, sizeof(Temp), "SO,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_LS2) {
			strncat_s(Temp, sizeof(Temp), "LS2,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_LS3) {
			strncat_s(Temp, sizeof(Temp), "LS3,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_LS1R) {
			strncat_s(Temp, sizeof(Temp), "LS1R,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_LS2R) {
			strncat_s(Temp, sizeof(Temp), "LS2R,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_LS3R) {
			strncat_s(Temp, sizeof(Temp), "LS3R,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_SS2) {
			strncat_s(Temp, sizeof(Temp), "SS2,", _TRUNCATE);
		}
		if (ts->ISO2022Flag & ISO2022_SS3) {
			strncat_s(Temp, sizeof(Temp), "SS3,", _TRUNCATE);
		}

		i = strlen(Temp);
		if (i == 0) {
			strncpy_s(Temp, sizeof(Temp), "off", _TRUNCATE);
		}
		else if (Temp[i-1] == ',') {
			Temp[i-1] = 0;
		}
	}
	WritePrivateProfileString(Section, "ISO2022ShiftFunction", Temp, FName);

	// Terminal Speed
	if (ts->TerminalInputSpeed == ts->TerminalOutputSpeed) {
		WriteInt(Section, "TerminalSpeed", FName, ts->TerminalInputSpeed);
	}
	else {
		WriteInt2(Section, "TerminalSpeed", FName,
			ts->TerminalInputSpeed, ts->TerminalOutputSpeed);
	}

	// Clear scroll buffer from remote -- special option
	WriteOnOff(Section, "ClearScrollBufferFromRemote", FName,
		(WORD) (ts->PasteFlag & TF_REMOTECLEARSBUFF));

	// Delay for start of mouse selection
	WriteInt(Section, "MouseSelectStartDelay", FName, ts->SelectStartDelay);

	// Fallback CP932 (Experimental)
	WriteOnOff(Section, "FallbackToCP932", FName, ts->FallbackToCP932);

	// dialog font
	aswprintf(&TempW, L"%s,%d,%d", ts->DialogFontNameW, ts->DialogFontPoint, ts->DialogFontCharSet);
	WritePrivateProfileStringW(L"Tera Term", L"DlgFont", TempW, FName);
	free(TempW);

	// Unicode�ݒ�
	WriteInt(Section, "UnicodeAmbiguousWidth", FName, ts->UnicodeAmbiguousWidth);
	WriteOnOff(Section, "UnicodeEmojiOverride", FName, ts->UnicodeEmojiOverride);
	WriteInt(Section, "UnicodeEmojiWidth", FName, ts->UnicodeEmojiWidth);

	WriteOnOff(Section, "DrawingResizedFont", FName, DispIsResizedFont());

	// rounded corner preference for VT/TEK window
	WriteOnOff(Section, "WindowCornerDontround", FName, ts->WindowCornerDontround);

	// �ʒm��
	WriteOnOff(Section, "NotifySound", FName, ts->NotifySound);

	// �����o�b�N�A�b�v
	WriteOnOff(Section, "IniAutoBackup", FName, ts->IniAutoBackup);

	// Bracketed paste mode
	WriteOnOff(Section, "BracketedSupport", FName, ts->BracketedSupport);
	WriteOnOff(Section, "BracketedControlOnly", FName, ts->BracketedControlOnly);

	// Sendfile
	WritePrivateProfileString(Section, "SendfileDelayType",
							  ts->SendfileDelayType == 1 ? "PerChar" :
							  ts->SendfileDelayType == 2 ? "PerLine" :
							  ts->SendfileDelayType == 3 ? "PerSendSize" :
							  "NoDelay",
							  FName);
	WriteInt(Section, "SendfileDelayTick", FName, ts->SendfileDelayTick);
	WriteInt(Section, "SendfileSize", FName, ts->SendfileSize);
	WriteOnOff(Section, "SendfileSequential", FName, ts->SendfileSequential);
	WriteOnOff(Section, "SendfileSkipOptionDialog", FName, ts->SendfileSkipOptionDialog);
}

void PASCAL _CopySerialList(const wchar_t *IniSrc, const wchar_t *IniDest, const wchar_t *section,
							const wchar_t *key, int MaxList)
{
	int i, j;

	if (_wcsicmp(IniSrc, IniDest) == 0)
		return;

	WritePrivateProfileStringW(section, NULL, NULL, IniDest);

	i = j = 1;
	do {
		wchar_t *TempHost;
		wchar_t *EntName;
		wchar_t *EntName2;

		aswprintf(&EntName, L"%s%i", key, i);
		aswprintf(&EntName2, L"%s%i", key, j);

		/* Get one hostname from file IniSrc */
		hGetPrivateProfileStringW(section, EntName, L"", IniSrc, &TempHost);

		/* Copy it to the file IniDest */
		if (TempHost != NULL && TempHost[0] != 0) {
			WritePrivateProfileStringW(section, EntName2, TempHost, IniDest);
			j++;
		}

		free(EntName);
		free(EntName2);
		free(TempHost);

		i++;
	}
	while (i <= MaxList);

	/* update file */
	WritePrivateProfileStringW(NULL, NULL, NULL, IniDest);
}

void PASCAL _AddValueToList(const wchar_t *FName, const wchar_t *Host, const wchar_t *section,
							const wchar_t *key, int MaxList)
{
	if ((FName[0] == 0) || (Host[0] == 0))
		return;

	if (MaxList <= 0) {
		return;
	}

	HistoryStore *hs = HistoryStoreCreate(MaxList);
	HistoryStoreReadIni(hs, FName, section, key);
	if (HistoryStoreAddTop(hs, Host, FALSE)) {
		HistoryStoreSaveIni(hs, FName, section, key);
	}
	HistoryStoreDestroy(hs);
}

 /* copy hostlist from source IniFile to dest IniFile */
void PASCAL _CopyHostList(const wchar_t *IniSrc, const wchar_t *IniDest)
{
	_CopySerialList(IniSrc, IniDest, L"Hosts", L"Host", MAXHOSTLIST);
}

void PASCAL _AddHostToList(const wchar_t *FName, const wchar_t *Host)
{
	_AddValueToList(FName, Host, L"Hosts", L"Host", MAXHOSTLIST);
}
#if 0
BOOL NextParam(PCHAR Param, int *i, PCHAR Temp, int Size)
{
	int j;
	char c;
	BOOL Quoted;

	if ((unsigned int) (*i) >= strlen(Param)) {
		return FALSE;
	}
	j = 0;

	while (Param[*i] == ' ' || Param[*i] == '\t') {
		(*i)++;
	}

	Quoted = FALSE;
	c = Param[*i];
	(*i)++;
	while ((c != 0) && (j < Size - 1) &&
	       (Quoted || ((c != ' ') && (c != ';') && (c != '\t')))) {
		if (c == '"')
			Quoted = !Quoted;
		Temp[j] = c;
		j++;
		c = Param[*i];
		(*i)++;
	}
	if (!Quoted && (c == ';')) {
		(*i)--;
	}

	Temp[j] = 0;
	return (strlen(Temp) > 0);
}
#endif

static int ParsePortName(const char *buff)
{
	int port = parse_port_from_buf(buff);

	if (port > 0 || sscanf(buff, "%d", &port) == 1)
		return port;
	else
		return 0;
}

static int ParsePortNameW(const wchar_t *buff)
{
	char *buffA = ToCharW(buff);
	int port = ParsePortName(buffA);
	free(buffA);
	return port;
}

static void ParseHostName(char *HostStr, WORD * port)
{
	/*
	 * hostname.example.jp
	 * hostname.example.jp:23
	 * hostname.example.jp:telnet
	 * [3ffe:1234:1234::1]         IPv6 raw address
	 * [3ffe:1234:1234::1]:23      IPv6 raw address and port#
	 * [3ffe:1234:1234::1]:telnet  IPv6 raw address and service name
	 * telnet://hostname.example.jp/
	 * telnet://hostname.example.jp:23/
	 * telnet://hostname.example.jp:telnet/
	 * telnet://[3ffe:1234:1234::1]/
	 * telnet://[3ffe:1234:1234::1]:23/
	 * telnet://[3ffe:1234:1234::1]:telnet/
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
				size_t len = strlen(HostStr);
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
	 *   hostname.example.jp
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
		*port = ParsePortName(&(s[i]));
		is_port = 1;
	}
	if (is_telnet_handler == 1 && is_port == 0) {
		*port = 23;
	}
}

/**
 *	�R�}���h���C���̃t�@�C��������t���p�X���쐬����
 *
 *	@param[in]	command_line	�R�}���h���C���̕�����(�t�@�C����)
 *	@param[in]	default_path	�t�@�C���̑��݂���p�X(�f�t�H���g�p�X)
 *								�t�@�C�������΃p�X�̎��A�t�@�C�����̑O�ɒǉ������
 *								NULL�̂Ƃ��A�J�����g�f�B���N�g�����ǉ������
 *	@param[in]	default_ini		�t�@�C���Ɋg���q�����݂��Ȃ��ꍇ�ǉ������
 *								L".ini"��
 *								NULL�̂Ƃ��ǉ����Ȃ�
 *	@return		�t���p�X�t�@�C����
 */
static wchar_t *GetFilePath(const wchar_t *command_line, const wchar_t *default_path, const wchar_t *default_ini)
{
	wchar_t *full_path;
	wchar_t *filepart;
	wchar_t *tmp;
	if (command_line == NULL || *command_line == 0) {
		// ���͂���������
		return NULL;
	}
	if (IsRelativePathW(command_line) && default_path != NULL) {
		full_path = NULL;
		awcscats(&full_path, default_path, L"\\", command_line, NULL);
	}
	else {
		full_path = _wcsdup(command_line);
	}

	// �t�@�C�����̃t���p�X��(���K��)
	hGetFullPathNameW(full_path, &tmp, &filepart);
	free(full_path);
	full_path = tmp;
	if (filepart == NULL) {
		// �t�@�C���������Ȃ�?
		assert(FALSE);
		free(full_path);
		return _wcsdup(command_line);
	}

	// �g���q�̒ǉ�
	if (default_ini != NULL) {
		if (wcsrchr(filepart, L'.') == NULL) {
			awcscat(&full_path, default_ini);
		}
	}

	return full_path;
}

void PASCAL _ParseParam(wchar_t *Param, PTTSet ts, PCHAR DDETopic)
{
	int pos, c;
	wchar_t Temp[MaxStrLen]; // ttpmacro����Ă΂�邱�Ƃ�z�肵MaxStrLen�T�C�Y�Ƃ���
	WORD ParamPort = 0;
	WORD ParamCom = 0;
	WORD ParamTCP = 0;
	WORD ParamTel = 2;
	WORD ParamBin = 2;
	DWORD ParamBaud = BaudNone;
	BOOL HostNameFlag = FALSE;
	BOOL JustAfterHost = FALSE;
	wchar_t *start, *cur, *next;

	ts->HostName[0] = 0;
	//ts->KeyCnfFN[0] = 0;

	/* Set AutoConnect true as default (2008.2.16 by steven)*/
	ts->ComAutoConnect = TRUE;

	/* user specifies the protocol connecting to the host */
	/* ts->ProtocolFamily = AF_UNSPEC; */

	/* Get command line parameters */
	if (DDETopic != NULL)
		DDETopic[0] = 0;

	/* the first term shuld be executable filename of Tera Term */
	start = GetParam(Temp, _countof(Temp), Param);

	cur = start;
	while ((next = GetParam(Temp, _countof(Temp), cur))) {
		DequoteParam(Temp, _countof(Temp), Temp);
		if (_wcsnicmp(Temp, L"/F=", 3) == 0) {	/* setup filename */
			wchar_t *f = GetFilePath(&Temp[3], ts->HomeDirW, L".INI");
			if (f != NULL && _wcsicmp(ts->SetupFNameW, f) != 0) {
				free(ts->SetupFNameW);
				ts->SetupFNameW = f;
				WideCharToACP_t(ts->SetupFNameW, ts->SetupFName, _countof(ts->SetupFName));
				_ReadIniFile(ts->SetupFNameW, ts);
			}
		}
		cur = next;
	}

	cur = start;
	while ((next = GetParam(Temp, _countof(Temp), cur))) {
		DequoteParam(Temp, _countof(Temp), Temp);

		if (HostNameFlag) {
			JustAfterHost = TRUE;
			HostNameFlag = FALSE;
		}

		if (_wcsnicmp(Temp, L"/AUTOWINCLOSE=", 14) == 0) {	/* AutoWinClose=on|off */
			wchar_t *s = &Temp[14];
			if (_wcsicmp(s, L"on") == 0)
				ts->AutoWinClose = 1;
			else
				ts->AutoWinClose = 0;
		}
		else if (_wcsnicmp(Temp, L"/SPEED=", 7) == 0) {	/* Serial port speed */
			ParamPort = IdSerial;
			ParamBaud = _wtoi(&Temp[7]);
		}
		else if (_wcsnicmp(Temp, L"/BAUD=", 6) == 0) {	/* for backward compatibility */
			ParamPort = IdSerial;
			ParamBaud = _wtoi(&Temp[6]);
		}
		else if (_wcsicmp(Temp, L"/B") == 0) {	/* telnet binary */
			ParamPort = IdTCPIP;
			ParamBin = 1;
		}
		else if (_wcsnicmp(Temp, L"/C=", 3) == 0) {	/* COM port num */
			ParamPort = IdSerial;
			ParamCom = _wtoi(&Temp[3]);
			if ((ParamCom < 1) || (ParamCom > ts->MaxComPort))
				ParamCom = 0;
		}
		else if (_wcsnicmp(Temp, L"/CDATABIT=", 10) == 0) {	/* COM data bit */
			ParamPort = IdSerial;
			SerialPortConfconvertStr2Id(COM_DATABIT, &Temp[10], &ts->DataBit);
		}
		else if (_wcsnicmp(Temp, L"/CPARITY=", 9) == 0) {	/* COM Parity */
			ParamPort = IdSerial;
			SerialPortConfconvertStr2Id(COM_PARITY, &Temp[9], &ts->Parity);
		}
		else if (_wcsnicmp(Temp, L"/CSTOPBIT=", 10) == 0) {	/* COM Stop bit */
			ParamPort = IdSerial;
			SerialPortConfconvertStr2Id(COM_STOPBIT, &Temp[10], &ts->StopBit);
		}
		else if (_wcsnicmp(Temp, L"/CFLOWCTRL=", 11) == 0) {	/* COM Flow control */
			ParamPort = IdSerial;
			SerialPortConfconvertStr2Id(COM_FLOWCTRL, &Temp[11], &ts->Flow);
		}
		else if (_wcsnicmp(Temp, L"/CDELAYPERCHAR=", 15) == 0) {	/* COM Transmit delay per character (in msec) */
			WORD val = 0;

			ParamPort = IdSerial;
			val = _wtoi(&Temp[15]);
			ts->DelayPerChar = val;
		}
		else if (_wcsnicmp(Temp, L"/CDELAYPERLINE=", 15) == 0) {	/* COM Transmit delay per line (in msec) */
			WORD val = 0;

			ParamPort = IdSerial;
			val = _wtoi(&Temp[15]);
			ts->DelayPerLine = val;
		}
		else if (_wcsicmp(Temp, L"/WAITCOM") == 0) {	/* wait COM arrival */
			ts->WaitCom = 1;
		}
		else if (_wcsnicmp(Temp, L"/D=", 3) == 0) {
			if (DDETopic != NULL) {
				char *DDETopicA = ToCharW(&Temp[3]);
				strncpy_s(DDETopic, 21, DDETopicA, _TRUNCATE);	// 21 = sizeof(TopicName)
				free(DDETopicA);
			}
		}
		// "New connection" �_�C�A���O��\�����Ȃ� (2008.11.14 maya)
		else if (_wcsicmp(Temp, L"/DS") == 0) {
			ts->HostDialogOnStartup = FALSE;
		}
		// TCPLocalEcho/TCPCRSend �𖳌��ɂ��� (maya 2007.4.25)
		else if (_wcsicmp(Temp, L"/E") == 0) {
			ts->DisableTCPEchoCR = TRUE;
		}
		// "New connection" �_�C�A���O��\������ (2013.10.08 maya)
		else if (_wcsicmp(Temp, L"/ES") == 0) {
			ts->HostDialogOnStartup = TRUE;
		}
		else if (_wcsnicmp(Temp, L"/FD=", 4) == 0) {	/* file transfer directory */
			wchar_t *dir;
			hGetFullPathNameW(&Temp[4], &dir, NULL);
			if (dir != NULL && wcslen(dir) > 0 && DoesFolderExistW(dir)) {
				free(ts->FileDirW);
				ts->FileDirW = _wcsdup(dir);
#if 0
				WideCharToACP_t(ts->FileDirW, ts->FileDir, sizeof(ts->FileDir));
#endif
			}
			free(dir);
		}
		else if (_wcsicmp(Temp, L"/H") == 0)	/* hide title bar */
			ts->HideTitle = 1;
		else if (_wcsicmp(Temp, L"/I") == 0)	/* iconize */
			ts->Minimize = 1;
		else if (_wcsnicmp(Temp, L"/K=", 3) == 0) {	/* Keyboard setup file */
			wchar_t *f = GetFilePath(&Temp[3], ts->HomeDirW, L".CNF");
			if (f != NULL) {
				free(ts->KeyCnfFNW);
				ts->KeyCnfFNW = f;
			}
		}
		else if (_wcsnicmp(Temp, L"/KR=", 4) == 0) {
			ts->KanjiCode = GetKanjiCodeFromStrW(&Temp[4]);
		}
		else if (_wcsnicmp(Temp, L"/KT=", 4) == 0) {
			ts->KanjiCodeSend = GetKanjiCodeFromStrW(&Temp[4]);
		}
		else if (_wcsnicmp(Temp, L"/L=", 3) == 0) {	/* log file */
			wchar_t *log_dir = GetTermLogDir(ts);
			wchar_t *f = GetFilePath(&Temp[3], log_dir, NULL);
			free(log_dir);
			if (f != NULL) {
				ts->LogFNW = f;
				WideCharToACP_t(ts->LogFNW, ts->LogFN, _countof(ts->LogFN));
			}
		}
		else if (_wcsnicmp(Temp, L"/MN=", 4) == 0) {	/* multicastname */
			WideCharToACP_t(&Temp[4], ts->MulticastName, _countof(ts->MulticastName));
		}
		else if (_wcsnicmp(Temp, L"/M=", 3) == 0) {	/* macro filename */
			if ((Temp[3] == 0) || (Temp[3] == '*')) {
				ts->MacroFNW = _wcsdup(L"*");
			} else {
				ts->MacroFNW = GetFilePath(&Temp[3], ts->HomeDirW, L".TTL");
			}
			/* Disable auto connect to serial when macro mode (2006.9.15 maya) */
			ts->ComAutoConnect = FALSE;
		}
		else if (_wcsicmp(Temp, L"/M") == 0) {	/* macro option without file name */
			ts->MacroFNW = _wcsdup(L"*");
			/* Disable auto connect to serial when macro mode (2006.9.15 maya) */
			ts->ComAutoConnect = FALSE;
		}
		else if (_wcsicmp(Temp, L"/NOLOG") == 0) {	/* disable auto logging */
			ts->LogFN[0] = '\0';
			ts->LogAutoStart = 0;
		}
		else if (_wcsnicmp(Temp, L"/OSC52=", 7) == 0) {	/* Clipboard access */
			ts->CtrlFlag &= ~CSF_CBMASK;
			if (_wcsicmp(&Temp[7], L"on") == 0 || _wcsicmp(&Temp[7], L"readwrite") == 0)
				ts->CtrlFlag |= CSF_CBRW;
			else if (_wcsicmp(&Temp[7], L"read") == 0)
				ts->CtrlFlag |= CSF_CBREAD;
			else if (_wcsicmp(&Temp[7], L"write") == 0)
				ts->CtrlFlag |= CSF_CBWRITE;
			else if (_wcsicmp(&Temp[7], L"off") == 0)
				ts->CtrlFlag |= CSF_CBNONE;
		}
		else if (_wcsnicmp(Temp, L"/P=", 3) == 0) {	/* TCP port num */
			ParamPort = IdTCPIP;
			ParamTCP = ParsePortNameW(&Temp[3]);
		}
		else if (_wcsicmp(Temp, L"/PIPE") == 0 ||
		         _wcsicmp(Temp, L"/NAMEDPIPE") == 0) {	/* ���O�t���p�C�v */
			ParamPort = IdNamedPipe;
		}
		else if (_wcsnicmp(Temp, L"/R=", 3) == 0) {	/* Replay filename */
			wchar_t *f = GetFilePath(&Temp[3], ts->HomeDirW, NULL);
			if (f != NULL) {
				WideCharToACP_t(f, ts->HostName, _countof(ts->HostName));
				if (strlen(ts->HostName) > 0) {
					ParamPort = IdFile;
				}
				free(f);
			}
		}
		else if (_wcsicmp(Temp, L"/T=0") == 0) {	/* telnet disable */
			ParamPort = IdTCPIP;
			ParamTel = 0;
		}
		else if (_wcsicmp(Temp, L"/T=1") == 0) {	/* telnet enable */
			ParamPort = IdTCPIP;
			ParamTel = 1;
		}
		else if (_wcsnicmp(Temp, L"/TEKICON=", 9) == 0) { /* Tek window icon */
			ts->TEKIcon = IconName2IconId(&Temp[9]);
		}
		else if (_wcsnicmp(Temp, L"/THEME=", 7) == 0) {
			wchar_t *f = GetFilePath(&Temp[7], ts->HomeDirW, NULL);
			if (f != NULL) {
				free(ts->EtermLookfeel.BGThemeFileW);
				ts->EtermLookfeel.BGThemeFileW = f;
				ts->EtermLookfeel.BGEnable = 1;
			}
		}
		else if (_wcsnicmp(Temp, L"/VTICON=", 8) == 0) {	/* VT window icon */
			ts->VTIcon = IconName2IconId(&Temp[8]);
		}
		else if (_wcsicmp(Temp, L"/V") == 0) {	/* invisible */
			ts->HideWindow = 1;
		}
		else if (_wcsnicmp(Temp, L"/W=", 3) == 0) {	/* Window title */
		    char* TitleA = ToCharW(&Temp[3]);
			strncpy_s(ts->Title, sizeof(ts->Title), TitleA, _TRUNCATE);
			free(TitleA);
		}
		else if (_wcsnicmp(Temp, L"/X=", 3) == 0) {	/* Window pos (X) */
			if (swscanf(&Temp[3], L"%d", &pos) == 1) {
				ts->VTPos.x = pos;
				if (ts->VTPos.y == CW_USEDEFAULT)
					ts->VTPos.y = 0;
			}
		}
		else if (_wcsnicmp(Temp, L"/Y=", 3) == 0) {	/* Window pos (Y) */
			if (swscanf(&Temp[3], L"%d", &pos) == 1) {
				ts->VTPos.y = pos;
				if (ts->VTPos.x == CW_USEDEFAULT)
					ts->VTPos.x = 0;
			}
		}
		else if (_wcsicmp(Temp, L"/4") == 0)	/* Protocol Tera Term speaking */
			ts->ProtocolFamily = AF_INET;
		else if (_wcsicmp(Temp, L"/6") == 0)
			ts->ProtocolFamily = AF_INET6;
		else if (_wcsicmp(Temp, L"/DUPLICATE") == 0) {	// duplicate session (2004.12.7. yutaka)
			ts->DuplicateSession = 1;

		}
		else if (_wcsnicmp(Temp, L"/TIMEOUT=", 9) == 0) {	// Connecting Timeout value (2007.1.11. yutaka)
			if (swscanf(&Temp[9], L"%d", &pos) == 1) {
				if (pos >= 0)
					ts->ConnectingTimeout = pos;
			}

		}
		else if ((Temp[0] != '/') && (wcslen(Temp) > 0)) {
			if (JustAfterHost && ((c=ParsePortNameW(Temp)) > 0))
				ParamTCP = c;
			else {
				char *HostNameA = ToCharW(Temp);
				strncpy_s(ts->HostName, sizeof(ts->HostName), HostNameA, _TRUNCATE);	/* host name */
				free(HostNameA);
				if (ParamPort == IdNamedPipe) {
					// �������Ȃ��B

				} else {
					ParamPort = IdTCPIP;
					HostNameFlag = TRUE;
				}
			}
		}
		JustAfterHost = FALSE;
		cur = next;
	}

	if ((DDETopic != NULL) && (DDETopic[0] != 0)) {
		free(ts->MacroFNW);
		ts->MacroFNW = NULL;
	}

	if ((ts->HostName[0] != 0) && (ParamPort == IdTCPIP)) {
		ParseHostName(ts->HostName, &ParamTCP);
	}

	switch (ParamPort) {
	case IdTCPIP:
		ts->PortType = IdTCPIP;
		if (ParamTCP != 0)
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
		if (ParamBaud != BaudNone)
			ts->Baud = ParamBaud;
		break;
	case IdFile:
		ts->PortType = IdFile;
		break;
	case IdNamedPipe:
		if (ts->HostName[0] != 0 && ts->HostName[0] != '\\') {
			char * p = strchr(ts->HostName, '\\');
			if (p == NULL) {
				*p++ = '\0';
				_snwprintf_s(Temp, _countof(Temp), _TRUNCATE, L"\\\\%hs\\pipe\\%hs", ts->HostName, p);
			}
			else {
				_snwprintf_s(Temp, _countof(Temp), _TRUNCATE, L"\\\\.\\pipe\\%hs", ts->HostName);
			}
			WideCharToACP_t(Temp, ts->HostName, _countof(ts->HostName));
		}
		ts->PortType = IdNamedPipe;
		ts->ComPort = 0;
		break;
	}
}

/**
 *	���̃��W���[���̏�����
 *		���݂�邱�ƂȂ�
 *		*��* ���I�ȃ��[�h���s���Ă��Ȃ����߁A��Ƀ��[�h����Ă����ԂƂȂ��Ă���
 */
void TTSetInit(void)
{
}

/**
 *	���̃��W���[���̏I��
 *		�m�ۂ����������̊J��
 */
void TTSetUnInit(TTTSet *ts)
{
	void **ptr_list[] = {
		(void **)&ts->HomeDirW,
		(void **)&ts->SetupFNameW,
		(void **)&ts->KeyCnfFNW,
		(void **)&ts->EtermLookfeel.BGThemeFileW,
		(void **)&ts->EtermLookfeel.BGSPIPathW,
		(void **)&ts->UILanguageFileW,
		(void **)&ts->ExeDirW,
		(void **)&ts->LogDirW,
		(void **)&ts->FileDirW,
		(void **)&ts->LogDefaultPathW,
		(void **)&ts->MacroFNW,
		(void **)&ts->LogFNW,
		(void **)&ts->LogDefaultNameW,
		(void **)&ts->DelimListW,
		(void **)&ts->ViewlogEditorW,
		(void **)&ts->ViewlogEditorArg,
	};
	int i;
	for(i = 0; i < _countof(ptr_list); i++) {
		void **ptr = ptr_list[i];
		_CrtIsValidHeapPointer(*ptr);
		free(*ptr);
		*ptr = NULL;
	}
}

BOOL WINAPI DllMain(HANDLE hInst,
                    ULONG ul_reason_for_call, LPVOID lpReserved)
{
	(void)hInst;
	(void)lpReserved;
	switch (ul_reason_for_call) {
	case DLL_THREAD_ATTACH:
		/* do thread initialization */
		break;
	case DLL_THREAD_DETACH:
		/* do thread cleanup */
		break;
	case DLL_PROCESS_ATTACH:
		/* do process initialization */
#ifdef _DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		break;
	case DLL_PROCESS_DETACH:
		/* do process cleanup */
		break;
	}
	return TRUE;
}
