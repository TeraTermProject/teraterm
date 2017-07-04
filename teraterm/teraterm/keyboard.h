/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2017 TeraTerm Project
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

/* proto types */
#ifdef __cplusplus
extern "C" {
#endif

/* KeyDown return type */
#define KEYDOWN_COMMOUT	1	/* リモートに送信（BS Enter Spaceなど） */
#define KEYDOWN_CONTROL	2	/* Ctrl,Shiftなど */
#define KEYDOWN_OTHER	0	/* その他 */

#define DEBUG_FLAG_NONE  0
#define DEBUG_FLAG_NORM  1
#define DEBUG_FLAG_HEXD  2
#define DEBUG_FLAG_NOUT  3
#define DEBUG_FLAG_MAXD  4

void SetKeyMap();
void ClearUserKey();
void DefineUserKey(int NewKeyId, PCHAR NewKeyStr, int NewKeyLen);
int KeyDown(HWND HWin, WORD VKey, WORD Count, WORD Scan);
void KeyCodeSend(WORD KCode, WORD Count);
void KeyUp(WORD VKey);
BOOL ShiftKey();
BOOL ControlKey();
BOOL AltKey();
BOOL MetaKey(int mode);
void InitKeyboard();
void EndKeyboard();

#define FuncKeyStrMax 32

extern BOOL AutoRepeatMode;
extern BOOL AppliKeyMode, AppliCursorMode, AppliEscapeMode;
extern BOOL Send8BitMode;
extern BYTE DebugFlag;

#ifdef __cplusplus
}
#endif
