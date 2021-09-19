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
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "ttlib.h"
#include "codeconv.h"
#include "../teraterm/keyboard_i.h"

typedef struct {
	int key_id;
	const wchar_t *key_str;
} keymap_list_t;

static void ReadList(PKeyMap KeyMap, const wchar_t *section, const keymap_list_t *list, size_t count, const wchar_t *FName)
{
	size_t i;
	const keymap_list_t *p = list;
	for (i = 0; i < count; i++) {
		wchar_t Temp[11];
		WORD Num;
		GetPrivateProfileStringW(section, p->key_str, L"", Temp, _countof(Temp), FName);
		if (Temp[0] == 0)
			Num = 0xFFFF;
		else if (_wcsicmp(Temp, L"off") == 0)
			Num = 0xFFFF;
		else if (swscanf(Temp, L"%hd", &Num) != 1)
			Num = 0xFFFF;

		KeyMap->Map[p->key_id - 1] = Num;
	}
}

static void ReadKeyboardMap(PKeyMap KeyMap, const wchar_t* FName)
{
	static const keymap_list_t vteditor_list[] = {
		// VT editor keypad
		{ IdUp, L"Up" },
		{ IdDown, L"Down" },
		{ IdRight, L"Right" },
		{ IdLeft, L"Left" },
		{ IdFind, L"Find" },
		{ IdInsert, L"Insert" },
		{ IdRemove, L"Remove" },
		{ IdSelect, L"Select" },
		{ IdPrev, L"Prev" },
		{ IdNext, L"Next" },
	};

	static const keymap_list_t vtnumeric_list[] = {
		// VT numeric keypad
		{ Id0, L"Num0" },
		{ Id1, L"Num1" },
		{ Id2, L"Num2" },
		{ Id3, L"Num3" },
		{ Id4, L"Num4" },
		{ Id5, L"Num5" },
		{ Id6, L"Num6" },
		{ Id7, L"Num7" },
		{ Id8, L"Num8" },
		{ Id9, L"Num9" },
		{ IdMinus, L"NumMinus" },
		{ IdComma, L"NumComma" },
		{ IdPeriod, L"NumPeriod" },
		{ IdEnter, L"NumEnter" },
		{ IdSlash, L"NumSlash" },
		{ IdAsterisk, L"NumAsterisk" },
		{ IdPlus, L"NumPlus" },
		{ IdPF1, L"PF1" },
		{ IdPF2, L"PF2" },
		{ IdPF3, L"PF3" },
		{ IdPF4, L"PF4" },
	};

	static const keymap_list_t vtfunction_list[] = {
		// VT function keys
		{ IdHold, L"Hold" },
		{ IdPrint, L"Print" },
		{ IdBreak, L"Break" },
		{ IdF6, L"F6" },
		{ IdF7, L"F7" },
		{ IdF8, L"F8" },
		{ IdF9, L"F9" },
		{ IdF10, L"F10" },
		{ IdF11, L"F11" },
		{ IdF12, L"F12" },
		{ IdF13, L"F13" },
		{ IdF14, L"F14" },
		{ IdHelp, L"Help" },
		{ IdDo, L"Do" },
		{ IdF17, L"F17" },
		{ IdF18, L"F18" },
		{ IdF19, L"F19" },
		{ IdF20, L"F20" },

		// UDK
		{ IdUDK6, L"UDK6" },
		{ IdUDK7, L"UDK7" },
		{ IdUDK8, L"UDK8" },
		{ IdUDK9, L"UDK9" },
		{ IdUDK10, L"UDK10" },
		{ IdUDK11, L"UDK11" },
		{ IdUDK12, L"UDK12" },
		{ IdUDK13, L"UDK13" },
		{ IdUDK14, L"UDK14" },
		{ IdUDK15, L"UDK15" },
		{ IdUDK16, L"UDK16" },
		{ IdUDK17, L"UDK17" },
		{ IdUDK18, L"UDK18" },
		{ IdUDK19, L"UDK19" },
	};

	static const keymap_list_t xterm_list[] = {
		// XTERM function / extended keys
		{ IdXF1, L"XF1" },
		{ IdXF2, L"XF2" },
		{ IdXF3, L"XF3" },
		{ IdXF4, L"XF4" },
		{ IdXF5, L"XF5" },
		{ IdXBackTab, L"XBackTab" },
	};

	static const keymap_list_t shortcut_list[] = {
		// accelerator keys
		{ IdCmdEditCopy, L"EditCopy" },
		{ IdCmdEditPaste, L"EditPaste" },
		{ IdCmdEditPasteCR, L"EditPasteCR" },
		{ IdCmdEditCLS, L"EditCLS" },
		{ IdCmdEditCLB, L"EditCLB" },
		{ IdCmdCtrlOpenTEK, L"ControlOpenTEK" },
		{ IdCmdCtrlCloseTEK, L"ControlCloseTEK" },
		{ IdCmdLineUp, L"LineUp" },
		{ IdCmdLineDown, L"LineDown" },
		{ IdCmdPageUp, L"PageUp" },
		{ IdCmdPageDown, L"PageDown" },
		{ IdCmdBuffTop, L"BuffTop" },
		{ IdCmdBuffBottom, L"BuffBottom" },
		{ IdCmdNextWin, L"NextWin" },
		{ IdCmdPrevWin, L"PrevWin" },
		{ IdCmdNextSWin, L"NextShownWin" },
		{ IdCmdPrevSWin, L"PrevShownWin" },
		{ IdCmdLocalEcho, L"LocalEcho" },
		{ IdCmdScrollLock, L"ScrollLock" },
	};

	ReadList(KeyMap, L"VT editor keypad", vteditor_list, _countof(vteditor_list), FName);
	ReadList(KeyMap, L"VT numeric keypad", vtnumeric_list, _countof(vtnumeric_list), FName);
	ReadList(KeyMap, L"VT function keys", vtfunction_list, _countof(vtfunction_list), FName);
	ReadList(KeyMap, L"X function keys", xterm_list, _countof(xterm_list), FName);
	ReadList(KeyMap, L"Shortcut keys", shortcut_list, _countof(shortcut_list), FName);
}

static void ReadUserkeysSection(const wchar_t *FName, PKeyMap KeyMap)
{
	int i;

	for (i = 0; i < NumOfUserKey; i++) {
		const int ttkeycode = IdUser1 + i;	// tera term key code
		wchar_t EntName[7];
		wchar_t TempStr[256];
		_snwprintf_s(EntName, _countof(EntName), _TRUNCATE, L"User%d", i + 1);
		GetPrivateProfileStringW(L"User keys", EntName, L"",
								 TempStr, _countof(TempStr), FName);
		if (TempStr[0] == 0) {
			continue;
		}

		if(_wcsnicmp(TempStr, L"off", 3) == 0) {
			KeyMap->Map[ttkeycode - 1] = 0xFFFF;
			continue;
		}

		{
			int keycode;
			int type;
			wchar_t str[256];
			int r;
			r = swscanf_s(TempStr, L"%d,%d,%[^\n]", &keycode, &type, &str, (unsigned)_countof(str));
			if (r != 3) {
				continue;
			}

			{
				UserKey_t *UserKeyData;
				const int UserKeyCount = KeyMap->UserKeyCount;
				UserKey_t *p;

				UserKeyData = (UserKey_t *)realloc(KeyMap->UserKeyData, sizeof(UserKey_t) * (UserKeyCount + 1));
				p = UserKeyData + UserKeyCount;
				p->ptr = _wcsdup(str);
				p->len = wcslen(p->ptr);
				p->type = type;
				p->keycode = keycode;
				p->ttkeycode = ttkeycode;

				KeyMap->UserKeyData = UserKeyData;
				KeyMap->UserKeyCount++;

				KeyMap->Map[ttkeycode - 1] = (WORD)keycode;
			}
		}
	}
}

/**
 *	keyboard.cnf を読み込む
 *		このファイルは ttermpro.exe にリンクされている
 *		ttpset.dll ttste_keyboard_entry.c の ReadKeyboardCnf() からここがコールされる
 *		KeyMap は初期化済み
 */
__declspec(dllexport) void ReadKeyboardCnfExe(const wchar_t *FName, PKeyMap KeyMap, BOOL ShowWarning)
{
	int i, j;

	ReadKeyboardMap(KeyMap, FName);
	ReadUserkeysSection(FName, KeyMap);

	// 重複チェック
	for (j = 1; j <= IdKeyMax - 1; j++)
		if (KeyMap->Map[j] != 0xFFFF)
			for (i = 0; i <= j - 1; i++)
				if (KeyMap->Map[i] == KeyMap->Map[j]) {
					if (ShowWarning) {
						char TempStr[128];
						_snprintf_s(TempStr, sizeof(TempStr), _TRUNCATE,
						            "Keycode %d is used more than once",
						            KeyMap->Map[j]);
						MessageBoxA(NULL, TempStr,
									"Tera Term: Error in keyboard setup file",
									MB_ICONEXCLAMATION);
					}
					KeyMap->Map[i] = 0xFFFF;
				}
}
