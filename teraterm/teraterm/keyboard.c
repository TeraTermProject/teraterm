/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* TERATERM.EXE, keyboard routines */

#include "teraterm.h"
#include "tttypes.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <crtdbg.h>

#include "tttypes_key.h"
#include "ttlib.h"
#include "ttsetup.h"
#include "ttcommon.h"
#include "ttwinman.h"
#include "ttdde.h"
#include "codeconv.h"

#include "keyboard.h"
#include "keyboard_i.h"

BOOL AutoRepeatMode;
BOOL AppliKeyMode;
BOOL AppliCursorMode;
int AppliEscapeMode;
BOOL Send8BitMode;
BYTE DebugFlag = DEBUG_FLAG_NONE;

static char FuncKeyStr[NumOfUDK][FuncKeyStrMax];
static int FuncKeyLen[NumOfUDK];

/*keyboard status*/
static int PreviousKey;

/*key code map*/
static PKeyMap KeyMap = NULL;

// Ctrl-\ support for NEC-PC98
static short VKBackslash;

#ifndef VK_PROCESSKEY
#define VK_PROCESSKEY 0xE5
#endif

static void FreeUserKey(PKeyMap KeyMap_)
{
	int i;
	UserKey_t *p;

	if (KeyMap_->UserKeyData == NULL) {
		KeyMap_->UserKeyCount = 0;
		return;
	}

	p = KeyMap_->UserKeyData;
	for (i = 0; i < KeyMap_->UserKeyCount; i++) {
		free(p->ptr);
		p++;
	}
	free(KeyMap_->UserKeyData);
	KeyMap_->UserKeyData = NULL;
	KeyMap_->UserKeyCount = 0;
}

/**
 *	必要ならKeyMap を確保、初期化する
 */
static void InitKeyMap()
{
	int i;

	if (KeyMap == NULL) {
		KeyMap = (PKeyMap)calloc(sizeof(TKeyMap), 1);
		if (KeyMap == NULL) {
			return;
		}
	}
	for (i = 0; i <= IdKeyMax - 1; i++)
		KeyMap->Map[i] = 0xFFFF;

	FreeUserKey(KeyMap);
}

void SetKeyMap()
{
	char TempDir[MAXPATHLEN];
	char TempName[MAX_PATH];

	ExtractFileName(ts.KeyCnfFN, TempName, sizeof(TempName));
	ExtractDirName(ts.KeyCnfFN, TempDir);
	if (TempDir[0] == 0)
		strncpy_s(TempDir, sizeof(TempDir), ts.HomeDir, _TRUNCATE);
	FitFileName(TempName, sizeof(TempName), ".CNF");

	strncpy_s(ts.KeyCnfFN, sizeof(ts.KeyCnfFN), TempDir, _TRUNCATE);
	AppendSlash(ts.KeyCnfFN, sizeof(ts.KeyCnfFN));
	strncat_s(ts.KeyCnfFN, sizeof(ts.KeyCnfFN), TempName, _TRUNCATE);

	InitKeyMap();
	if (KeyMap == NULL) {
		return;
	}

	if (LoadTTSET())
		(*ReadKeyboardCnf)(ts.KeyCnfFNW, KeyMap, TRUE);
	FreeTTSET();
}

void ClearUserKey()
{
	int i;

	i = 0;
	while (i < NumOfUDK) FuncKeyLen[i++] = 0;
}

void DefineUserKey(int NewKeyId, PCHAR NewKeyStr, int NewKeyLen)
{
	if ((NewKeyLen == 0) || (NewKeyLen > FuncKeyStrMax))
		return;

	if ((NewKeyId >= 17) && (NewKeyId <= 21))
		NewKeyId = NewKeyId - 17;
	else if ((NewKeyId >= 23) && (NewKeyId <= 26))
		NewKeyId = NewKeyId - 18;
	else if ((NewKeyId >= 28) && (NewKeyId <= 29))
		NewKeyId = NewKeyId - 19;
	else if ((NewKeyId >= 31) && (NewKeyId <= 34))
		NewKeyId = NewKeyId - 20;
	else
		return;

	memcpy(&FuncKeyStr[NewKeyId][0], NewKeyStr, NewKeyLen);
	FuncKeyLen[NewKeyId] = NewKeyLen;
}

static void GetKeyStr(HWND HWin, const PKeyMap KeyMap_, WORD KeyCode, BOOL AppliKeyMode_, BOOL AppliCursorMode_,
					  BOOL Send8BitMode_, wchar_t *KeyStr, size_t destlen, size_t *Len, UserKeyType_t *Type)
{
	MSG Msg;

	*Type = IdBinary;  // key type
	*Len = 0;
	switch (KeyCode) {
		case IdUp:
			if (Send8BitMode_) {
				*Len = 2;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\217A", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\233A", _TRUNCATE);
			}
			else {
				*Len = 3;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\033OA", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\033[A", _TRUNCATE);
			}
			break;
		case IdDown:
			if (Send8BitMode_) {
				*Len = 2;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\217B", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\233B", _TRUNCATE);
			}
			else {
				*Len = 3;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\033OB", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\033[B", _TRUNCATE);
			}
			break;
		case IdRight:
			if (Send8BitMode_) {
				*Len = 2;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\217C", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\233C", _TRUNCATE);
			}
			else {
				*Len = 3;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\033OC", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\033[C", _TRUNCATE);
			}
			break;
		case IdLeft:
			if (Send8BitMode_) {
				*Len = 2;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\217D", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\233D", _TRUNCATE);
			}
			else {
				*Len = 3;
				if (AppliCursorMode_)
					wcsncpy_s(KeyStr, destlen, L"\033OD", _TRUNCATE);
				else
					wcsncpy_s(KeyStr, destlen, L"\033[D", _TRUNCATE);
			}
			break;
		case Id0:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217p", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Op", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '0';
			}
			break;
		case Id1:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217q", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Oq", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '1';
			}
			break;
		case Id2:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217r", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Or", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '2';
			}
			break;
		case Id3:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217s", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Os", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '3';
			}
			break;
		case Id4:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217t", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ot", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '4';
			}
			break;
		case Id5:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217u", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ou", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '5';
			}
			break;
		case Id6:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217v", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ov", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '6';
			}
			break;
		case Id7:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217w", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ow", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '7';
			}
			break;
		case Id8:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217x", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ox", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '8';
			}
			break;
		case Id9:
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217y", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Oy", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '9';
			}
			break;
		case IdMinus: /* numeric pad - key (DEC) */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217m", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Om", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '-';
			}
			break;
		case IdComma: /* numeric pad , key (DEC) */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217l", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ol", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = ',';
			}
			break;
		case IdPeriod: /* numeric pad . key */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217n", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033On", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '.';
			}
			break;
		case IdEnter: /* numeric pad enter key */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217M", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033OM", _TRUNCATE);
				}
			}
			else {
				*Type = IdText;	 // do new-line conversion
				*Len = 1;
				KeyStr[0] = 0x0D;
			}
			break;
		case IdSlash: /* numeric pad slash key */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217o", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Oo", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '/';
			}
			break;
		case IdAsterisk: /* numeric pad asterisk key */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217j", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Oj", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '*';
			}
			break;
		case IdPlus: /* numeric pad plus key */
			if (AppliKeyMode_) {
				if (Send8BitMode_) {
					*Len = 2;
					wcsncpy_s(KeyStr, destlen, L"\217k", _TRUNCATE);
				}
				else {
					*Len = 3;
					wcsncpy_s(KeyStr, destlen, L"\033Ok", _TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '+';
			}
			break;
		case IdPF1: /* DEC Key: PF1 */
			if (Send8BitMode_) {
				*Len = 2;
				wcsncpy_s(KeyStr, destlen, L"\217P", _TRUNCATE);
			}
			else {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\033OP", _TRUNCATE);
			}
			break;
		case IdPF2: /* DEC Key: PF2 */
			if (Send8BitMode_) {
				*Len = 2;
				wcsncpy_s(KeyStr, destlen, L"\217Q", _TRUNCATE);
			}
			else {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\033OQ", _TRUNCATE);
			}
			break;
		case IdPF3: /* DEC Key: PF3 */
			if (Send8BitMode_) {
				*Len = 2;
				wcsncpy_s(KeyStr, destlen, L"\217R", _TRUNCATE);
			}
			else {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\033OR", _TRUNCATE);
			}
			break;
		case IdPF4: /* DEC Key: PF4 */
			if (Send8BitMode_) {
				*Len = 2;
				wcsncpy_s(KeyStr, destlen, L"\217S", _TRUNCATE);
			}
			else {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\033OS", _TRUNCATE);
			}
			break;
		case IdFind: /* DEC Key: Find */
			if (Send8BitMode_) {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\2331~", _TRUNCATE);
			}
			else {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\033[1~", _TRUNCATE);
			}
			break;
		case IdInsert: /* DEC Key: Insert Here */
			if (Send8BitMode_) {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\2332~", _TRUNCATE);
			}
			else {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\033[2~", _TRUNCATE);
			}
			break;
		case IdRemove: /* DEC Key: Remove */
			if (Send8BitMode_) {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\2333~", _TRUNCATE);
			}
			else {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\033[3~", _TRUNCATE);
			}
			break;
		case IdSelect: /* DEC Key: Select */
			if (Send8BitMode_) {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\2334~", _TRUNCATE);
			}
			else {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\033[4~", _TRUNCATE);
			}
			break;
		case IdPrev: /* DEC Key: Prev */
			if (Send8BitMode_) {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\2335~", _TRUNCATE);
			}
			else {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\033[5~", _TRUNCATE);
			}
			break;
		case IdNext: /* DEC Key: Next */
			if (Send8BitMode_) {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\2336~", _TRUNCATE);
			}
			else {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\033[6~", _TRUNCATE);
			}
			break;
		case IdF6: /* DEC Key: F6 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23317~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[17~", _TRUNCATE);
			}
			break;
		case IdF7: /* DEC Key: F7 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23318~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[18~", _TRUNCATE);
			}
			break;
		case IdF8: /* DEC Key: F8 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23319~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[19~", _TRUNCATE);
			}
			break;
		case IdF9: /* DEC Key: F9 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23320~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[20~", _TRUNCATE);
			}
			break;
		case IdF10: /* DEC Key: F10 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23321~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[21~", _TRUNCATE);
			}
			break;
		case IdF11: /* DEC Key: F11 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23323~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[23~", _TRUNCATE);
			}
			break;
		case IdF12: /* DEC Key: F12 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23324~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[24~", _TRUNCATE);
			}
			break;
		case IdF13: /* DEC Key: F13 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23325~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[25~", _TRUNCATE);
			}
			break;
		case IdF14: /* DEC Key: F14 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23326~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[26~", _TRUNCATE);
			}
			break;
		case IdHelp: /* DEC Key: Help */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23328~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[28~", _TRUNCATE);
			}
			break;
		case IdDo: /* DEC Key: Do */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23329~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[29~", _TRUNCATE);
			}
			break;
		case IdF17: /* DEC Key: F17 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23331~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[31~", _TRUNCATE);
			}
			break;
		case IdF18: /* DEC Key: F18 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23332~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[32~", _TRUNCATE);
			}
			break;
		case IdF19: /* DEC Key: F19 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23333~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[33~", _TRUNCATE);
			}
			break;
		case IdF20: /* DEC Key: F20 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23334~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[34~", _TRUNCATE);
			}
			break;
		case IdXF1: /* XTERM F1 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23311~", _TRUNCATE);
			}
			else {
				*Len = 5;

				wcsncpy_s(KeyStr, destlen, L"\033[11~", _TRUNCATE);
			}
			break;
		case IdXF2: /* XTERM F2 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23312~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[12~", _TRUNCATE);
			}
			break;
		case IdXF3: /* XTERM F3 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23313~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[13~", _TRUNCATE);
			}
			break;
		case IdXF4: /* XTERM F4 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23314~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[14~", _TRUNCATE);
			}
			break;
		case IdXF5: /* XTERM F5 */
			if (Send8BitMode_) {
				*Len = 4;
				wcsncpy_s(KeyStr, destlen, L"\23315~", _TRUNCATE);
			}
			else {
				*Len = 5;
				wcsncpy_s(KeyStr, destlen, L"\033[15~", _TRUNCATE);
			}
			break;
		case IdXBackTab: /* XTERM Back Tab */
			if (Send8BitMode_) {
				*Len = 2;
				wcsncpy_s(KeyStr, destlen, L"\233Z", _TRUNCATE);
			}
			else {
				*Len = 3;
				wcsncpy_s(KeyStr, destlen, L"\033[Z", _TRUNCATE);
			}
			break;
		case IdHold:
		case IdPrint:
		case IdBreak:
		case IdCmdEditCopy:
		case IdCmdEditPaste:
		case IdCmdEditPasteCR:
		case IdCmdEditCLS:
		case IdCmdEditCLB:
		case IdCmdCtrlOpenTEK:
		case IdCmdCtrlCloseTEK:
		case IdCmdLineUp:
		case IdCmdLineDown:
		case IdCmdPageUp:
		case IdCmdPageDown:
		case IdCmdBuffTop:
		case IdCmdBuffBottom:
		case IdCmdNextWin:
		case IdCmdPrevWin:
		case IdCmdNextSWin:
		case IdCmdPrevSWin:
		case IdCmdLocalEcho:
		case IdCmdScrollLock:
			PostMessage(HWin, WM_USER_ACCELCOMMAND, KeyCode, 0);
			break;
		default:
			if ((KeyCode >= IdUser1) && (KeyCode <= IdKeyMax)) {
				//
				UserKey_t *p = KeyMap_->UserKeyData;
				int i;
				wchar_t *s;
				for (i = 0; i < KeyMap_->UserKeyCount; i++) {
					if (p->ttkeycode == KeyCode) {
						break;
					}
					p++;
				}
				if (i == KeyMap_->UserKeyCount){
					// ユーザーキーに設定がない
					return;
				}
				*Type = p->type;
				s = p->ptr;
				*Len = p->len;
				if ((*Type == IdBinary) || (*Type == IdText))
					*Len = Hex2StrW(s, KeyStr, destlen);
				else
					wcsncpy_s(KeyStr, destlen, s, _TRUNCATE);
			}
			else
				return;
	}
	/* remove WM_CHAR message for used keycode */
	PeekMessage(&Msg, HWin, WM_CHAR, WM_CHAR, PM_REMOVE);
}

static size_t VKey2KeyStr(WORD VKey, HWND HWin, wchar_t *Code, size_t CodeSize, UserKeyType_t *CodeType, WORD ModStat)
{
	BOOL Single, Control, Shift;
	size_t CodeLength = 0;

	Single = FALSE;
	Shift = FALSE;
	Control = FALSE;
	switch (ModStat) {
		case 0:
			Single = TRUE;
			break;
		case 2:
			Shift = TRUE;
			break;
		case 4:
			Control = TRUE;
			break;
	}

	switch (VKey) {
		case VK_BACK:
			if (Control) {
				CodeLength = 1;
				if (ts.BSKey == IdDEL)
					Code[0] = 0x08;
				else
					Code[0] = 0x7F;
			}
			else if (Single) {
				CodeLength = 1;
				if (ts.BSKey == IdDEL)
					Code[0] = 0x7F;
				else
					Code[0] = 0x08;
			}
			break;
		case VK_RETURN: /* CR Key */
			if (Single) {
				*CodeType = IdText;	 // do new-line conversion
				CodeLength = 1;
				Code[0] = 0x0D;
			}
			break;
		case VK_ESCAPE:	 // Escape Key
			if (Single) {
				switch (AppliEscapeMode) {
					case 1:
						CodeLength = 3;
						Code[0] = 0x1B;
						Code[1] = 'O';
						Code[2] = '[';
						break;
					case 2:
						CodeLength = 2;
						Code[0] = 0x1B;
						Code[1] = 0x1B;
						break;
					case 3:
						CodeLength = 2;
						Code[0] = 0x1B;
						Code[1] = 0x00;
						break;
					case 4:
						CodeLength = 8;
						Code[0] = 0x1B;
						Code[1] = 0x1B;
						Code[2] = '[';
						Code[3] = '=';
						Code[4] = '2';
						Code[5] = '7';
						Code[6] = '%';
						Code[7] = '~';
						break;
				}
			}
			break;
		case VK_SPACE:
			if (Control) {	// Ctrl-Space -> NUL
				CodeLength = 1;
				Code[0] = 0;
			}
			break;
		case VK_DELETE:
			if (Single) {
				if (ts.DelKey > 0) {  // DEL character
					CodeLength = 1;
					Code[0] = 0x7f;
				}
				else if (!ts.StrictKeyMapping) {
					GetKeyStr(HWin, KeyMap, IdRemove, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_UP:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdUp, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_DOWN:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdDown, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_RIGHT:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdRight, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_LEFT:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdLeft, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_INSERT:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdInsert, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_HOME:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdFind, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_END:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdSelect, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_PRIOR:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdPrev, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_NEXT:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdNext, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F1:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdXF1, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F2:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdXF2, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F3:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdXF3, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdF13, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F4:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdXF4, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdF14, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F5:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdXF5, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdHelp, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F6:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdF6, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdDo, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F7:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdF7, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdF17, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F8:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdF8, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdF18, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F9:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdF9, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdF19, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F10:
			if (!ts.StrictKeyMapping) {
				if (Single) {
					GetKeyStr(HWin, KeyMap, IdF10, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
				else if (Shift) {
					GetKeyStr(HWin, KeyMap, IdF20, AppliKeyMode && !ts.DisableAppKeypad,
							  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength,
							  CodeType);
				}
			}
			break;
		case VK_F11:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF11, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F12:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF12, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F13:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF13, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F14:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF14, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F15:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdHelp, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F16:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdDo, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F17:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF17, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F18:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF18, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F19:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF19, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case VK_F20:
			if (Single && !ts.StrictKeyMapping) {
				GetKeyStr(HWin, KeyMap, IdF20, AppliKeyMode && !ts.DisableAppKeypad,
						  AppliCursorMode && !ts.DisableAppCursor, Send8BitMode, Code, CodeSize, &CodeLength, CodeType);
			}
			break;
		case '2':
			//  case VK_OEM_3: /* @ (106-JP Keyboard) */
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-2 -> NUL
				CodeLength = 1;
				Code[0] = 0;
			}
			break;
		case '3':
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-3 -> ESC
				switch (AppliEscapeMode) {
					case 1:
						CodeLength = 3;
						Code[0] = 0x1B;
						Code[1] = 'O';
						Code[2] = '[';
						break;
					case 2:
						CodeLength = 2;
						Code[0] = 0x1B;
						Code[1] = 0x1B;
						break;
					case 3:
						CodeLength = 2;
						Code[0] = 0x1B;
						Code[1] = 0x00;
						break;
					case 4:
						CodeLength = 8;
						Code[0] = 0x1B;
						Code[1] = 0x1B;
						Code[2] = '[';
						Code[3] = '=';
						Code[4] = '2';
						Code[5] = '7';
						Code[6] = '%';
						Code[7] = '~';
						break;
					default:
						CodeLength = 1;
						Code[0] = 0x1b;
						break;
				}
			}
			break;
		case '4':
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-4 -> FS
				CodeLength = 1;
				Code[0] = 0x1c;
			}
			break;
		case '5':
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-5 -> GS
				CodeLength = 1;
				Code[0] = 0x1d;
			}
			break;
		case '6':
			//  case VK_OEM_7: /* ^ (106-JP Keyboard) */
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-6 -> RS
				CodeLength = 1;
				Code[0] = 0x1e;
			}
			break;
		case '7':
		case VK_OEM_2: /* / (101/106-JP Keyboard) */
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-7 -> US
				CodeLength = 1;
				Code[0] = 0x1f;
			}
			break;
		case '8':
			if (Control && !ts.StrictKeyMapping) {
				// Ctrl-8 -> DEL
				CodeLength = 1;
				Code[0] = 0x7f;
			}
			break;
		case VK_OEM_102:
			if (Control && Shift && !ts.StrictKeyMapping) {
				// Shift-Ctrl-_ (102RT/106-JP Keyboard)
				CodeLength = 1;
				Code[0] = 0x7f;
			}
			break;
		default:
			if ((VKey == VKBackslash) && Control) {	 // Ctrl-\ support for NEC-PC98
				CodeLength = 1;
				Code[0] = 0x1c;
			}
	}

	return CodeLength;
}

/* Key scan code -> Tera Term key code */
static WORD GetKeyCode(PKeyMap KeyMap_, WORD Scan)
{
	WORD Key;

	Key = IdKeyMax;
	while ((Key > 0) && (KeyMap_->Map[Key - 1] != Scan)) {
		Key--;
	}
	return Key;
}

static void SendBinary(const wchar_t *strW, size_t len, size_t repeat)
{
	unsigned char *strA = malloc(len + 1);
	unsigned char *p = strA;
	size_t i;
	for (i = 0; i < len + 1; i++) {
		wchar_t w;
		unsigned char c;
		w = strW[i];
		c = w < 256 ? (unsigned char)w : 0xff;
		*p++ = c;
	}

	for (i = 1; i <= repeat; i++) {
		CommBinaryBuffOut(&cv, strA, len);
		if (ts.LocalEcho > 0)
			CommBinaryEcho(&cv, strA, len);
	}

	free(strA);
}

static void RunMacroW(const wchar_t *FNameW, BOOL Startup)
{
	char *fname = ToCharW(FNameW);
	RunMacro(fname, Startup);
	free(fname);
}

/**
 *	@param	VKey	virtual-key code
 *	@param	count	The value is the number of times the keystroke
 *	@param	Scan	The scan code. The value depends on the OEM
 */
KeyDownResult KeyDown(HWND HWin, WORD VKey, WORD Count, WORD Scan)
{
	WORD Key;
	MSG M;
	int i;
	int CodeCount;
	size_t CodeLength;
	wchar_t Code[MAXPATHLEN];
	UserKeyType_t CodeType;
	WORD ModStat;

	if (VKey == VK_PROCESSKEY)
		return KEYDOWN_CONTROL;

	if ((VKey == VK_SHIFT) || (VKey == VK_CONTROL) || (VKey == VK_MENU))
		return KEYDOWN_CONTROL;

	/* debug mode */
	if (ts.Debug && (VKey == VK_ESCAPE) && ShiftKey()) {
		MessageBeep(0);
		do {
			DebugFlag = (DebugFlag + 1) % DEBUG_FLAG_MAXD;
		} while (DebugFlag != DEBUG_FLAG_NONE && !((ts.DebugModes >> (DebugFlag - 1)) & 1));
		CodeCount = 0;
		PeekMessage((LPMSG)&M, HWin, WM_CHAR, WM_CHAR, PM_REMOVE);
		return KEYDOWN_CONTROL;
	}

	if (!AutoRepeatMode && (PreviousKey == VKey)) {
		PeekMessage((LPMSG)&M, HWin, WM_CHAR, WM_CHAR, PM_REMOVE);
		return KEYDOWN_CONTROL;
	}

	PreviousKey = VKey;

	if (Scan == 0)
		Scan = MapVirtualKey(VKey, 0);

	ModStat = 0;
	if (ShiftKey()) {
		Scan |= 0x200;
		ModStat = 2;
	}

	if (ControlKey()) {
		Scan |= 0x400;
		ModStat |= 4;
	}

	if (AltKey()) {
		Scan |= 0x800;
		if (!MetaKey(ts.MetaKey)) {
			ModStat |= 16;
		}
	}

	CodeCount = Count;
	CodeLength = 0;
	if (cv.TelLineMode) {
		CodeType = IdText;
	}
	else {
		CodeType = IdBinary;
	}

	/* exclude numeric keypad "." (scan code:83) */
	if ((VKey != VK_DELETE) || (ts.DelKey == 0) || (Scan == 83))
		/* Windows keycode -> Tera Term keycode */
		Key = GetKeyCode(KeyMap, Scan);
	else
		Key = 0;

	if (Key == 0) {
		CodeLength = VKey2KeyStr(VKey, HWin, Code, _countof(Code), &CodeType, ModStat);

		if (MetaKey(ts.MetaKey) && (CodeLength == 1)) {
			switch (ts.Meta8Bit) {
				case IdMeta8BitRaw:
					Code[0] |= 0x80;
					CodeType = IdBinary;
					break;
				case IdMeta8BitText:
					Code[0] |= 0x80;
					CodeType = IdText;
					break;
				default:
					Code[1] = Code[0];
					Code[0] = 0x1b;
					CodeLength = 2;
			}
			PeekMessage((LPMSG)&M, HWin, WM_SYSCHAR, WM_SYSCHAR, PM_REMOVE);
		}
	}
	else {
		if (MetaKey(ts.MetaKey)) {
			PeekMessage((LPMSG)&M, HWin, WM_SYSCHAR, WM_SYSCHAR, PM_REMOVE);
		}

		if ((IdUDK6 <= Key) && (Key <= IdUDK20) && (FuncKeyLen[Key - IdUDK6] > 0)) {
			memcpy(Code, &FuncKeyStr[Key - IdUDK6][0], FuncKeyLen[Key - IdUDK6]);
			CodeLength = FuncKeyLen[Key - IdUDK6];
			CodeType = IdBinary;
		}
		else
			GetKeyStr(HWin, KeyMap, Key, AppliKeyMode && !ts.DisableAppKeypad, AppliCursorMode && !ts.DisableAppCursor,
					  Send8BitMode, Code, _countof(Code), &CodeLength, &CodeType);
	}

	if (CodeLength == 0)
		return KEYDOWN_OTHER;

	if (VKey == VK_NUMLOCK) {
		/* keep NumLock LED status */
		BYTE KeyState[256];
		GetKeyboardState((PBYTE)KeyState);
		KeyState[VK_NUMLOCK] = KeyState[VK_NUMLOCK] ^ 1;
		SetKeyboardState((PBYTE)KeyState);
	}

	PeekMessage((LPMSG)&M, HWin, WM_CHAR, WM_CHAR, PM_REMOVE);

	if (KeybEnabled) {
		switch (CodeType) {
			case IdBinary:
				if (TalkStatus == IdTalkKeyb) {
					SendBinary(Code, CodeLength, CodeCount);
				}
				break;
			case IdText:
				if (TalkStatus == IdTalkKeyb) {
					for (i = 1; i <= CodeCount; i++) {
						if (ts.LocalEcho > 0)
							CommTextEchoW(&cv, Code, CodeLength);
						CommTextOutW(&cv, Code, CodeLength);
					}
				}
				break;
			case IdMacro:
				Code[CodeLength] = 0;
				RunMacroW(Code, FALSE);
				break;
			case IdCommand: {
				WORD wId;
				Code[CodeLength] = 0;
				if (swscanf(Code, L"%hd", &wId) == 1)
					PostMessage(HWin, WM_COMMAND, MAKELONG(wId, 0), 0);
				break;
			}
		}
	}
	return (CodeType == IdBinary || CodeType == IdText) ? KEYDOWN_COMMOUT : KEYDOWN_CONTROL;
}

void KeyCodeSend(WORD KCode, WORD Count)
{
	WORD Key;
	int i;
	size_t CodeLength;
	wchar_t Code[MAXPATHLEN];
	UserKeyType_t CodeType;
	WORD Scan, VKey, State;
	DWORD dw;
	BOOL Ok;
	HWND HWin;

	if (ActiveWin == IdTEK)
		HWin = HTEKWin;
	else
		HWin = HVTWin;

	CodeLength = 0;
	CodeType = IdBinary;
	Key = GetKeyCode(KeyMap, KCode);
	if (Key == 0) {
		Scan = KCode & 0x1FF;
		VKey = MapVirtualKey(Scan, 1);
		State = 0;
		if ((KCode & 512) != 0) { /* shift */
			State = State | 2;	  /* bit 1 */
		}

		if ((KCode & 1024) != 0) { /* control */
			State = State | 4;	   /* bit 2 */
		}

		if ((KCode & 2048) != 0) { /* alt */
			State = State | 16;	   /* bit 4 */
		}

		CodeLength = VKey2KeyStr(VKey, HWin, Code, _countof(Code), &CodeType, State);

		if (CodeLength == 0) {
			i = -1;
			do {
				i++;
				dw = OemKeyScan((WORD)i);
				Ok = (LOWORD(dw) == Scan) && (HIWORD(dw) == State);
			} while ((i < 255) && !Ok);
			if (Ok) {
				CodeType = IdText;
				CodeLength = 1;
				Code[0] = (char)i;
			}
		}
	}
	else if ((IdUDK6 <= Key) && (Key <= IdUDK20) && (FuncKeyLen[Key - IdUDK6] > 0)) {
		memcpy(Code, &FuncKeyStr[Key - IdUDK6][0], FuncKeyLen[Key - IdUDK6]);
		CodeLength = FuncKeyLen[Key - IdUDK6];
		CodeType = IdBinary;
	}
	else
		GetKeyStr(HWin, KeyMap, Key, AppliKeyMode && !ts.DisableAppKeypad, AppliCursorMode && !ts.DisableAppCursor,
				  Send8BitMode, Code, _countof(Code), &CodeLength, &CodeType);

	if (CodeLength == 0)
		return;
	if (TalkStatus == IdTalkKeyb) {
		switch (CodeType) {
			case IdBinary:
				SendBinary(Code, CodeLength, Count);
				break;
			case IdText:
				for (i = 1; i <= Count; i++) {
					if (ts.LocalEcho > 0)
						CommTextEchoW(&cv, Code, CodeLength);
					CommTextOutW(&cv, Code, CodeLength);
				}
				break;
			case IdMacro:
				Code[CodeLength] = 0;
				RunMacroW(Code, FALSE);
				break;
		}
	}
}

void KeyUp(WORD VKey)
{
	if (PreviousKey == VKey)
		PreviousKey = 0;
}

BOOL ShiftKey()
{
	return ((GetAsyncKeyState(VK_SHIFT) & 0xFFFFFF80) != 0);
}

BOOL ControlKey()
{
	return ((GetAsyncKeyState(VK_CONTROL) & 0xFFFFFF80) != 0);
}

BOOL AltKey()
{
	return ((GetAsyncKeyState(VK_MENU) & 0xFFFFFF80) != 0);
}

BOOL MetaKey(int mode)
{
	switch (mode) {
		case IdMetaOn:
			return ((GetAsyncKeyState(VK_MENU) & 0xFFFFFF80) != 0);
		case IdMetaLeft:
			return ((GetAsyncKeyState(VK_LMENU) & 0xFFFFFF80) != 0);
		case IdMetaRight:
			return ((GetAsyncKeyState(VK_RMENU) & 0xFFFFFF80) != 0);
		default:
			return FALSE;
	}
}

void InitKeyboard()
{
	KeyMap = NULL;
	ClearUserKey();
	PreviousKey = 0;
	VKBackslash = LOBYTE(VkKeyScan('\\'));
}

void EndKeyboard()
{
	if (KeyMap == NULL)
		return;

	FreeUserKey(KeyMap);
	free(KeyMap);
	KeyMap = NULL;
}
