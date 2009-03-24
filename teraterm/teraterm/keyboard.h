/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* proto types */
#ifdef __cplusplus
extern "C" {
#endif

/* KeyDown return type */
#define KEYDOWN_COMMOUT	1	/* リモートに送信（BS Enter Spaceなど） */
#define KEYDOWN_CONTROL	2	/* Ctrl,Shiftなど */
#define KEYDOWN_OTHER	0	/* その他 */

void SetKeyMap();
void ClearUserKey();
void DefineUserKey(int NewKeyId, PCHAR NewKeyStr, int NewKeyLen);
int KeyDown(HWND HWin, WORD VKey, WORD Count, WORD Scan);
void KeyCodeSend(WORD KCode, WORD Count);
void KeyUp(WORD VKey);
BOOL ShiftKey();
BOOL ControlKey();
BOOL AltKey();
void InitKeyboard();
void EndKeyboard();

#define FuncKeyStrMax 32

extern BOOL AutoRepeatMode;
extern BOOL AppliKeyMode, AppliCursorMode;
extern BOOL Send8BitMode;
extern BOOL DebugFlag;

#ifdef __cplusplus
}
#endif
