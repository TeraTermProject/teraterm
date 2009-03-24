/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, keyboard routines */

#include "teraterm.h"
#include "tttypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ttlib.h"
#include "ttsetup.h"
#include "ttcommon.h"
#include "ttwinman.h"
#include "ttdde.h"

#include "keyboard.h"

BOOL AutoRepeatMode;
BOOL AppliKeyMode, AppliCursorMode;
BOOL Send8BitMode;
BOOL DebugFlag = FALSE;

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

void SetKeyMap()
{
  char TempDir[MAXPATHLEN];
  char TempName[MAXPATHLEN];

  if ( strlen(ts.KeyCnfFN)==0 ) return;
  ExtractFileName(ts.KeyCnfFN,TempName,sizeof(TempName));
  ExtractDirName(ts.KeyCnfFN,TempDir);
  if (TempDir[0]==0)
    strncpy_s(TempDir, sizeof(TempDir),ts.HomeDir, _TRUNCATE);
  FitFileName(TempName,sizeof(TempName),".CNF");

  strncpy_s(ts.KeyCnfFN, sizeof(ts.KeyCnfFN),TempDir, _TRUNCATE);
  AppendSlash(ts.KeyCnfFN,sizeof(ts.KeyCnfFN));
  strncat_s(ts.KeyCnfFN,sizeof(ts.KeyCnfFN),TempName,_TRUNCATE);

  if ( KeyMap==NULL )
    KeyMap = (PKeyMap)malloc(sizeof(TKeyMap));
  if ( KeyMap!=NULL )
  {
    if ( LoadTTSET() )
      (*ReadKeyboardCnf)(ts.KeyCnfFN, KeyMap, TRUE);
    FreeTTSET();
  }
  if ((_stricmp(TempDir,ts.HomeDir)==0) &&
      (_stricmp(TempName,"KEYBOARD.CNF")==0))
  {
     ChangeDefaultSet(NULL,KeyMap);
     free(KeyMap);
     KeyMap = NULL;
  }
}

void ClearUserKey()
{
  int i;

  i = 0;
  while (i < NumOfUDK)
    FuncKeyLen[i++] = 0;
}

void DefineUserKey(int NewKeyId, PCHAR NewKeyStr, int NewKeyLen)
{
  if ((NewKeyLen==0) || (NewKeyLen>FuncKeyStrMax)) return;

  if ((NewKeyId>=17) && (NewKeyId<=21))
    NewKeyId = NewKeyId-17;
  else if ((NewKeyId>=23) && (NewKeyId<=26))
    NewKeyId = NewKeyId-18;
  else if ((NewKeyId>=28) && (NewKeyId<=29))
    NewKeyId = NewKeyId-19;
  else if ((NewKeyId>=31) && (NewKeyId<=34))
    NewKeyId = NewKeyId-20;
  else
    return;

  memcpy(&FuncKeyStr[NewKeyId][0], NewKeyStr, NewKeyLen);
  FuncKeyLen[NewKeyId] = NewKeyLen;
}

int KeyDown(HWND HWin, WORD VKey, WORD Count, WORD Scan)
{
  WORD Key;
  MSG M;
  BYTE KeyState[256];
  BOOL Single, Control;
  int i;
  int CodeCount;
  int CodeLength;
  char Code[MAXPATHLEN];
  WORD CodeType;
  WORD wId;

  if (VKey==VK_PROCESSKEY) return KEYDOWN_CONTROL;

  if ((VKey==VK_SHIFT) ||
      (VKey==VK_CONTROL) ||
      (VKey==VK_MENU)) return KEYDOWN_CONTROL;

  /* debug mode */
  if ((ts.Debug>0) && (VKey == VK_ESCAPE) &&
      ShiftKey())
  {
    MessageBeep(0);
    DebugFlag = ! DebugFlag;
    CodeCount = 0;
    PeekMessage((LPMSG)&M,HWin,WM_CHAR,WM_CHAR,PM_REMOVE);
    return KEYDOWN_CONTROL;
  }

  if (! AutoRepeatMode && (PreviousKey==VKey))
  {
    PeekMessage((LPMSG)&M,HWin,WM_CHAR,WM_CHAR,PM_REMOVE);
    return KEYDOWN_CONTROL;
  }

  PreviousKey = VKey;

  if (Scan==0)
    Scan = MapVirtualKey(VKey,0);

  Single = TRUE;
  Control = TRUE;
  if (ShiftKey())
  {
    Scan = Scan | 0x200;
    Single = FALSE;
    Control = FALSE;
  }

  if (ControlKey())
  {
    Scan = Scan | 0x400;
    Single = FALSE;
  }
  else
    Control = FALSE;

  if (AltKey())
  {
    Scan = Scan | 0x800;
    if (ts.MetaKey==0)
    {
      Single = FALSE;
      Control = FALSE;
    }
  }

  CodeCount = Count;
  CodeLength = 0;
  CodeType = IdBinary;

  /* exclude numeric keypad "." (scan code:83) */
  if ((VKey!=VK_DELETE) || (ts.DelKey==0) || (Scan==83))
    /* Windows keycode -> Tera Term keycode */
    Key = GetKeyCode(KeyMap,Scan);
  else
    Key = 0;

  if (Key==0)
  {
    switch (VKey) {
      case VK_BACK:
	if (Control)
	{
	  CodeLength = 1;
	  if (ts.BSKey==IdDEL)
	    Code[0] = 0x08;
	  else
	    Code[0] = 0x7F;
	}
        else if (Single)
	{
	  CodeLength = 1;
	  if (ts.BSKey==IdDEL)
	    Code[0] = 0x7F;
	  else
	    Code[0] = 0x08;
	}
	break;
      case VK_RETURN: /* CR Key */
	if (Single)
	{
	  CodeType = IdText; // do new-line conversion
	  CodeLength = 1;
	  Code[0] = 0x0D;
	}
	break;
      case VK_SPACE:
	if (Control)
	{ // Ctrl-Space -> NUL
	  CodeLength = 1;
	  Code[0] = 0;
	}
	break;
      case VK_DELETE:
	if (Single) {
	  if (ts.DelKey > 0) { // DEL character
	    CodeLength = 1;
	    Code[0] = 0x7f;
	  }
	  else if (!ts.StrictKeyMapping) {
	    GetKeyStr(HWin, KeyMap, IdRemove,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_UP:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdUp,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_DOWN:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdDown,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_RIGHT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdRight,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_LEFT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdLeft,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_INSERT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdInsert,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_HOME:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdFind,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_END:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdSelect,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_PRIOR:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdPrev,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_NEXT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdNext,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F1:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdXF1,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F2:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdXF2,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F3:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdXF3,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF13,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F4:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdXF4,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF14,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F5:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdXF5,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdHelp,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F6:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF6,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdDo,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F7:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF7,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF17,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F8:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF8,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF18,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F9:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF9,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF19,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F10:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF10,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF20,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F11:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF11,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F12:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF12,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case '2':
      case '@':
	if (Control && !ts.StrictKeyMapping) {
	  // Ctrl-2 -> NUL
	  CodeLength = 1;
	  Code[0] = 0;
	}
	break;
      case '3':
	if (Control && !ts.StrictKeyMapping) {
	  // Ctrl-3 -> ESC
	  CodeLength = 1;
	  Code[0] = 0x1b;
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
      case '^':
	if (Control && !ts.StrictKeyMapping) {
	  // Ctrl-6 -> RS
	  CodeLength = 1;
	  Code[0] = 0x1e;
	}
	break;
      case '7':
      case '/':
      case '?':
      case '_':
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
      default:
	if ((VKey==VKBackslash) && Control)
	{ // Ctrl-\ support for NEC-PC98
	  CodeLength = 1;
	  Code[0] = 0x1c;
	}
    }
    if ((ts.MetaKey>0) && (CodeLength==1) &&
	AltKey())
    {
      Code[1] = Code[0];
      Code[0] = 0x1b;
      CodeLength = 2;
      PeekMessage((LPMSG)&M,HWin,WM_SYSCHAR,WM_SYSCHAR,PM_REMOVE);
    }
  }
  else if ((IdUDK6<=Key) && (Key<=IdUDK20) &&
	   (FuncKeyLen[Key-IdUDK6]>0))
  {
    memcpy(Code,&FuncKeyStr[Key-IdUDK6][0],FuncKeyLen[Key-IdUDK6]);
    CodeLength = FuncKeyLen[Key-IdUDK6];
    CodeType = IdBinary;
  }
  else
    GetKeyStr(HWin,KeyMap,Key,
              AppliKeyMode && ! ts.DisableAppKeypad,
              AppliCursorMode && ! ts.DisableAppCursor,
              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);

  if (CodeLength==0) return KEYDOWN_OTHER;

  if (VKey==VK_NUMLOCK)
  {
    /* keep NumLock LED status */
    GetKeyboardState((PBYTE)KeyState);
    KeyState[VK_NUMLOCK] = KeyState[VK_NUMLOCK] ^ 1;
    SetKeyboardState((PBYTE)KeyState);
  }

  PeekMessage((LPMSG)&M,HWin,WM_CHAR,WM_CHAR,PM_REMOVE);

  if (KeybEnabled)
  {
    switch (CodeType) {
      case IdBinary:
	if (TalkStatus==IdTalkKeyb)
	{
	  for (i = 1 ; i <= CodeCount ; i++)
	  {
	    CommBinaryOut(&cv,Code,CodeLength);
	    if (ts.LocalEcho>0)
	      CommBinaryEcho(&cv,Code,CodeLength);
	  }
	}
	break;
      case IdText:
	if (TalkStatus==IdTalkKeyb)
	{
	  for (i = 1 ; i <= CodeCount ; i++)
	  {
	    if (ts.LocalEcho>0)
	      CommTextEcho(&cv,Code,CodeLength);
	    CommTextOut(&cv,Code,CodeLength);
	  }
	}
	break;
      case IdMacro:
	Code[CodeLength] = 0;
	RunMacro(Code,FALSE);
	break;
      case IdCommand:
	Code[CodeLength] = 0;
	if (sscanf(Code, "%d", &wId) == 1)
	  PostMessage(HWin,WM_COMMAND,MAKELONG(wId,0),0);
	break;
    }
  }
  return (CodeType == IdBinary || CodeType == IdText)? KEYDOWN_COMMOUT: KEYDOWN_CONTROL;
}

void KeyCodeSend(WORD KCode, WORD Count)
{
  WORD Key;
  int i, CodeLength;
  char Code[MAXPATHLEN];
  WORD CodeType;
  WORD Scan, VKey, State;
  BOOL Single, Control;
  DWORD dw;
  BOOL Ok;
  HWND HWin;

  if (ActiveWin==IdTEK)
    HWin = HTEKWin;
  else
    HWin = HVTWin;

  CodeLength = 0;
  CodeType = IdBinary;
  Key = GetKeyCode(KeyMap,KCode);
  if (Key==0)
  {
    Scan = KCode & 0x1FF;
    VKey = MapVirtualKey(Scan,1);
    State = 0;
    Single = TRUE;
    Control = TRUE;
    if ((KCode & 512) != 0)
    { /* shift */
      State = State | 2; /* bit 1 */
      Single = FALSE;
      Control = FALSE;
    }
    
    if ((KCode & 1024) != 0)
    { /* control */
      State = State | 4; /* bit 2 */
      Single = FALSE;
    }
    else
      Control = FALSE;

    if ((KCode & 2048) != 0)
    { /* alt */
      State = State | 16; /* bit 4 */
      Single = FALSE;
      Control = FALSE;
    }

    switch (VKey) {
      case VK_BACK:
	if (Control)
	{
	  CodeLength = 1;
	  if (ts.BSKey==IdDEL)
	    Code[0] = 0x08;
	  else
	    Code[0] = 0x7F;
	}
        else if (Single)
	{
	  CodeLength = 1;
	  if (ts.BSKey==IdDEL)
	    Code[0] = 0x7F;
	  else
	    Code[0] = 0x08;
	}
	break;
      case VK_RETURN: /* CR Key */
	if (Single)
	{
	  CodeType = IdText; // do new-line conversion
	  CodeLength = 1;
	  Code[0] = 0x0D;
	}
	break;
      case VK_SPACE:
	if (Control)
	{ // Ctrl-Space -> NUL
	  CodeLength = 1;
	  Code[0] = 0;
	}
	break;
      case VK_DELETE:
	if (Single) {
	  if (ts.DelKey > 0) { // DEL character
	    CodeLength = 1;
	    Code[0] = 0x7f;
	  }
	  else if (!ts.StrictKeyMapping) {
	    GetKeyStr(HWin, KeyMap, IdRemove,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_UP:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdUp,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_DOWN:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdDown,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_RIGHT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdRight,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_LEFT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdLeft,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_INSERT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdInsert,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_HOME:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdFind,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_END:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdSelect,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_PRIOR:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdPrev,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_NEXT:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdNext,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F1:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdXF1,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F2:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdXF2,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F3:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdXF3,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF13,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F4:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdXF4,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF14,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F5:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdXF5,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdHelp,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F6:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF6,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdDo,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F7:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF7,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF17,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F8:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF8,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF18,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F9:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF9,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF19,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F10:
        if (!ts.StrictKeyMapping) {
	  if (Single) {
	    GetKeyStr(HWin, KeyMap, IdF10,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	  else if (ShiftKey()) {
	    GetKeyStr(HWin, KeyMap, IdF20,
	              AppliKeyMode && ! ts.DisableAppKeypad,
	              AppliCursorMode && ! ts.DisableAppCursor,
	              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	  }
	}
	break;
      case VK_F11:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF11,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F12:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF12,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F13:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF13,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F14:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF14,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F15:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdHelp,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F16:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdDo,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F17:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF17,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F18:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF18,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F19:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF19,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case VK_F20:
	if (Single && !ts.StrictKeyMapping) {
	  GetKeyStr(HWin, KeyMap, IdF20,
	            AppliKeyMode && ! ts.DisableAppKeypad,
	            AppliCursorMode && ! ts.DisableAppCursor,
	            Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);
	}
	break;
      case '2':
      case '@':
	if (Control && !ts.StrictKeyMapping) {
	  // Ctrl-2 -> NUL
	  CodeLength = 1;
	  Code[0] = 0;
	}
	break;
      case '3':
	if (Control && !ts.StrictKeyMapping) {
	  // Ctrl-3 -> ESC
	  CodeLength = 1;
	  Code[0] = 0x1b;
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
      case '^':
	if (Control && !ts.StrictKeyMapping) {
	  // Ctrl-6 -> RS
	  CodeLength = 1;
	  Code[0] = 0x1e;
	}
	break;
      case '7':
      case '/':
      case '?':
      case '_':
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
      default:
	if ((VKey==VKBackslash) && Control)
	{ // Ctrl-\ support for NEC-PC98
	  CodeLength = 1;
	  Code[0] = 0x1c;
	}
    }

    if (CodeLength==0)
    {
      i = -1;
      do {
	i++;
	dw = OemKeyScan((WORD)i);
	Ok = (LOWORD(dw)==Scan) &&
	     (HIWORD(dw)==State);
      } while ((i<255) && ! Ok);
      if (Ok)
      {
	CodeType = IdText;
	CodeLength = 1;
	Code[0] = (char)i;
      }
    }
  }
  else if ((IdUDK6<=Key) && (Key<=IdUDK20) &&
	   (FuncKeyLen[Key-IdUDK6]>0))
  {
    memcpy(Code,&FuncKeyStr[Key-IdUDK6][0],FuncKeyLen[Key-IdUDK6]);
    CodeLength = FuncKeyLen[Key-IdUDK6];
    CodeType = IdBinary;
  }
  else
    GetKeyStr(HWin,KeyMap,Key,
              AppliKeyMode && ! ts.DisableAppKeypad,
              AppliCursorMode && ! ts.DisableAppCursor,
              Send8BitMode, Code, sizeof(Code), &CodeLength, &CodeType);

  if (CodeLength==0) return;
  if (TalkStatus==IdTalkKeyb)
  {
    switch (CodeType) {
      case IdBinary:
	for (i = 1 ; i <= Count ; i++)
	{
	  CommBinaryOut(&cv,Code,CodeLength);
	  if (ts.LocalEcho>0)
	    CommBinaryEcho(&cv,Code,CodeLength);
	}
	break;
      case IdText:
	for (i = 1 ; i <= Count ; i++)
	{
	  if (ts.LocalEcho>0)
	    CommTextEcho(&cv,Code,CodeLength);
	  CommTextOut(&cv,Code,CodeLength);
	}
	break;
      case IdMacro:
	Code[CodeLength] = 0;
	RunMacro(Code,FALSE);
	break;
    }
  }
}

void KeyUp(WORD VKey)
{
  if (PreviousKey == VKey) PreviousKey = 0;
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

void InitKeyboard()
{
  KeyMap = NULL;
  ClearUserKey();
  PreviousKey = 0;
  VKBackslash = LOBYTE(VkKeyScan('\\'));
}

void EndKeyboard()
{
  if (KeyMap != NULL)
    free(KeyMap);
}
