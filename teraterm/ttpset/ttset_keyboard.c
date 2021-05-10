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

#include "teraterm.h"
#include "tttypes_key.h"
#include <stdio.h>
#include <string.h>
#include "ttlib.h"
#include "codeconv.h"
#include "tttypes_key.h"

#define INI_FILE_IS_UNICODE 1

#define VTEditor "VT editor keypad"
#define VTNumeric "VT numeric keypad"
#define VTFunction "VT function keys"
#define XFunction "X function keys"
#define ShortCut "Shortcut keys"

extern DWORD GetPrivateProfileStringAFileW(const char *appA, const char *keyA, const char* defA, char *strA, DWORD size, const wchar_t *filenameW);
#undef GetPrivateProfileString
#define GetPrivateProfileString(p1, p2, p3, p4, p5, p6) GetPrivateProfileStringAFileW(p1, p2, p3, p4, p5, p6)
#define GetPrivateProfileStringA(p1, p2, p3, p4, p5, p6) GetPrivateProfileStringAFileW(p1, p2, p3, p4, p5, p6)

#if	INI_FILE_IS_UNICODE
static void GetInt(PKeyMap KeyMap, int KeyId, const char *Sect, const char *Key, const wchar_t *FName)
#else
static void GetInt(PKeyMap KeyMap, int KeyId, const char *Sect, const char *Key, const char *FName)
#endif
{
	char Temp[11];
	WORD Num;

	GetPrivateProfileString(Sect, Key, "", Temp, sizeof(Temp), FName);
	if (Temp[0] == 0)
		Num = 0xFFFF;
	else if (_stricmp(Temp, "off") == 0)
		Num = 0xFFFF;
	else if (sscanf(Temp, "%hd", &Num) != 1)
		Num = 0xFFFF;

	KeyMap->Map[KeyId - 1] = Num;
}

void PASCAL ReadKeyboardCnf(PCHAR FNameA, PKeyMap KeyMap, BOOL ShowWarning)
{
	int i, j, Ptr;
	char EntName[7];
	char TempStr[221];
	char KStr[201];
#if	INI_FILE_IS_UNICODE
	const wchar_t *FName = ToWcharA(FNameA);
#else
	const char *FName = FNameA;
#endif

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

	// XTERM function / extended keys
	GetInt(KeyMap, IdXF1, XFunction, "XF1", FName);

	GetInt(KeyMap, IdXF2, XFunction, "XF2", FName);

	GetInt(KeyMap, IdXF3, XFunction, "XF3", FName);

	GetInt(KeyMap, IdXF4, XFunction, "XF4", FName);

	GetInt(KeyMap, IdXF5, XFunction, "XF5", FName);

	GetInt(KeyMap, IdXBackTab, XFunction, "XBackTab", FName);

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

	GetInt(KeyMap, IdCmdNextSWin, ShortCut, "NextShownWin", FName);

	GetInt(KeyMap, IdCmdPrevSWin, ShortCut, "PrevShownWin", FName);

	GetInt(KeyMap, IdCmdLocalEcho, ShortCut, "LocalEcho", FName);

	GetInt(KeyMap, IdCmdScrollLock, ShortCut, "ScrollLock", FName);

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

#if	INI_FILE_IS_UNICODE
	free((void *)FName);
#endif
}
