/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTCMN.DLL, main */
#include "teraterm.h"
#include "tttypes.h"
#include <direct.h>
#include <string.h>
#include "ttftypes.h"
#include "ttlib.h"
#include "language.h"
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <locale.h>

#include "compat_w95.h"

/* first instance flag */
static BOOL FirstInstance = TRUE;

static HINSTANCE hInst;

static PMap pm;

static HANDLE HMap = NULL;
#define VTCLASSNAME "VTWin32"
#define TEKCLASSNAME "TEKWin32"


void PASCAL CopyShmemToTTSet(PTTSet ts)
{
	// 現在の設定を共有メモリからコピーする
	memcpy(ts, &pm->ts, sizeof(TTTSet));
}

void PASCAL CopyTTSetToShmem(PTTSet ts)
{
	// 現在の設定を共有メモリへコピーする
	memcpy(&pm->ts, ts, sizeof(TTTSet));
}


BOOL PASCAL FAR StartTeraTerm(PTTSet ts)
{
	char Temp[MAXPATHLEN];

	if (FirstInstance) {
		// init window list
		pm->NWin = 0;
	}
	else {
		/* only the first instance uses saved position */
		pm->ts.VTPos.x = CW_USEDEFAULT;
		pm->ts.VTPos.y = CW_USEDEFAULT;
		pm->ts.TEKPos.x = CW_USEDEFAULT;
		pm->ts.TEKPos.y = CW_USEDEFAULT;
	}

	memcpy(ts,&(pm->ts),sizeof(TTTSet));

	// if (FirstInstance) { の部分から移動 (2008.3.13 maya)
	// 起動時には、共有メモリの HomeDir と SetupFName は空になる
	/* Get home directory */
	GetModuleFileName(hInst,Temp,sizeof(Temp));
	ExtractDirName(Temp, ts->HomeDir);
	_chdir(ts->HomeDir);
	GetDefaultSetupFName(ts->HomeDir, ts->SetupFName, sizeof(ts->SetupFName));

	if (FirstInstance) {
		FirstInstance = FALSE;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void PASCAL FAR ChangeDefaultSet(PTTSet ts, PKeyMap km)
{
	if ((ts!=NULL) &&
		(_stricmp(ts->SetupFName, pm->ts.SetupFName) == 0)) {
		memcpy(&(pm->ts),ts,sizeof(TTTSet));
	}
	if (km!=NULL) {
		memcpy(&(pm->km),km,sizeof(TKeyMap));
	}
}

void PASCAL FAR GetDefaultSet(PTTSet ts)
{
	memcpy(ts,&(pm->ts),sizeof(TTTSet));
}


/* Key scan code -> Tera Term key code */
WORD PASCAL FAR GetKeyCode(PKeyMap KeyMap, WORD Scan)
{
	WORD Key;

	if (KeyMap==NULL) {
		KeyMap = &(pm->km);
	}
	Key = IdKeyMax;
	while ((Key>0) && (KeyMap->Map[Key-1] != Scan)) {
		Key--;
	}
	return Key;
}

void PASCAL FAR GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
                          BOOL AppliKeyMode, BOOL AppliCursorMode,
                          BOOL Send8BitMode, PCHAR KeyStr, int destlen,
                          LPINT Len, LPWORD Type)
{
	MSG Msg;
	char Temp[201];

	if (KeyMap==NULL) {
		KeyMap = &(pm->km);
	}

	*Type = IdBinary;  // key type
	*Len = 0;
	switch (KeyCode) {
		case IdUp:
			if (Send8BitMode) {
				*Len = 2;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\217A",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\233A",_TRUNCATE);
			} else {
				*Len = 3;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\033OA",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\033[A",_TRUNCATE);
			}
			break;
		case IdDown:
			if (Send8BitMode) {
				*Len = 2;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\217B",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\233B",_TRUNCATE);
			} else {
				*Len = 3;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\033OB",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\033[B",_TRUNCATE);
			}
			break;
		case IdRight:
			if (Send8BitMode) {
				*Len = 2;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\217C",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\233C",_TRUNCATE);
			} else {
				*Len = 3;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\033OC",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\033[C",_TRUNCATE);
			}
			break;
		case IdLeft:
			if (Send8BitMode) {
				*Len = 2;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\217D",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\233D",_TRUNCATE);
			} else {
				*Len = 3;
				if (AppliCursorMode)
					strncpy_s(KeyStr,destlen,"\033OD",_TRUNCATE);
				else
					strncpy_s(KeyStr,destlen,"\033[D",_TRUNCATE);
			}
			break;
		case Id0:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217p",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Op",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '0';
			}
			break;
		case Id1:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217q",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Oq",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '1';
			}
			break;
		case Id2:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217r",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Or",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '2';
			}
			break;
		case Id3:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217s",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Os",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '3';
			}
			break;
		case Id4:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217t",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ot",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '4';
			}
			break;
		case Id5:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217u",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ou",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '5';
			}
			break;
		case Id6:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217v",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ov",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '6';
			}
			break;
		case Id7:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217w",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ow",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '7';
			}
			break;
		case Id8:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217x",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ox",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '8';
			}
			break;
		case Id9:
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217y",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Oy",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '9';
			}
			break;
		case IdMinus: /* numeric pad - key (DEC) */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217m",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Om",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '-';
			}
			break;
		case IdComma: /* numeric pad , key (DEC) */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217l",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ol",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = ',';
			}
			break;
		case IdPeriod: /* numeric pad . key */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217n",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033On",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '.';
			}
			break;
		case IdEnter: /* numeric pad enter key */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217M",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033OM",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = 0x0D;
			}
			break;
		case IdSlash: /* numeric pad slash key */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217o",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Oo",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '/';
			}
			break;
		case IdAsterisk: /* numeric pad asterisk key */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217j",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Oj",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '*';
			}
			break;
		case IdPlus: /* numeric pad plus key */
			if (AppliKeyMode) {
				if (Send8BitMode) {
					*Len = 2;
					strncpy_s(KeyStr,destlen,"\217k",_TRUNCATE);
				} else {
					*Len = 3;
					strncpy_s(KeyStr,destlen,"\033Ok",_TRUNCATE);
				}
			}
			else {
				*Len = 1;
				KeyStr[0] = '+';
			}
			break;
		case IdPF1: /* DEC Key: PF1 */
			if (Send8BitMode) {
				*Len = 2;
				strncpy_s(KeyStr,destlen,"\217P",_TRUNCATE);
			} else {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\033OP",_TRUNCATE);
			}
			break;
		case IdPF2: /* DEC Key: PF2 */
			if (Send8BitMode) {
				*Len = 2;
				strncpy_s(KeyStr,destlen,"\217Q",_TRUNCATE);
			} else {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\033OQ",_TRUNCATE);
			}
			break;
		case IdPF3: /* DEC Key: PF3 */
			if (Send8BitMode) {
				*Len = 2;
				strncpy_s(KeyStr,destlen,"\217R",_TRUNCATE);
			} else {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\033OR",_TRUNCATE);
			}
			break;
		case IdPF4: /* DEC Key: PF4 */
			if (Send8BitMode) {
				*Len = 2;
				strncpy_s(KeyStr,destlen,"\217S",_TRUNCATE);
			} else {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\033OS",_TRUNCATE);
			}
			break;
		case IdFind: /* DEC Key: Find */
			if (Send8BitMode) {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\2331~",_TRUNCATE);
			} else {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\033[1~",_TRUNCATE);
			}
			break;
		case IdInsert: /* DEC Key: Insert Here */
			if (Send8BitMode) {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\2332~",_TRUNCATE);
			} else {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\033[2~",_TRUNCATE);
			}
			break;
		case IdRemove: /* DEC Key: Remove */
			if (Send8BitMode) {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\2333~",_TRUNCATE);
			} else {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\033[3~",_TRUNCATE);
			}
			break;
		case IdSelect: /* DEC Key: Select */
			if (Send8BitMode) {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\2334~",_TRUNCATE);
			} else {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\033[4~",_TRUNCATE);
			}
			break;
		case IdPrev: /* DEC Key: Prev */
			if (Send8BitMode) {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\2335~",_TRUNCATE);
			} else {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\033[5~",_TRUNCATE);
			}
			break;
		case IdNext: /* DEC Key: Next */
			if (Send8BitMode) {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\2336~",_TRUNCATE);
			} else {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\033[6~",_TRUNCATE);
			}
			break;
		case IdF6: /* DEC Key: F6 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23317~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[17~",_TRUNCATE);
			}
			break;
		case IdF7: /* DEC Key: F7 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23318~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[18~",_TRUNCATE);
			}
			break;
		case IdF8: /* DEC Key: F8 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23319~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[19~",_TRUNCATE);
			}
			break;
		case IdF9: /* DEC Key: F9 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23320~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[20~",_TRUNCATE);
			}
			break;
		case IdF10: /* DEC Key: F10 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23321~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[21~",_TRUNCATE);
			}
			break;
		case IdF11: /* DEC Key: F11 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23323~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[23~",_TRUNCATE);
			}
			break;
		case IdF12: /* DEC Key: F12 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23324~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[24~",_TRUNCATE);
			}
			break;
		case IdF13: /* DEC Key: F13 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23325~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[25~",_TRUNCATE);
			}
			break;
		case IdF14: /* DEC Key: F14 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23326~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[26~",_TRUNCATE);
			}
			break;
		case IdHelp: /* DEC Key: Help */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23328~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[28~",_TRUNCATE);
			}
			break;
		case IdDo: /* DEC Key: Do */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23329~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[29~",_TRUNCATE);
			}
			break;
		case IdF17: /* DEC Key: F17 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23331~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[31~",_TRUNCATE);
			}
			break;
		case IdF18: /* DEC Key: F18 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23332~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[32~",_TRUNCATE);
			}
			break;
		case IdF19: /* DEC Key: F19 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23333~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[33~",_TRUNCATE);
			}
			break;
		case IdF20: /* DEC Key: F20 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23334~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[34~",_TRUNCATE);
			}
			break;
		case IdXF1: /* XTERM F1 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23311~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[11~",_TRUNCATE);
			}
			break;
		case IdXF2: /* XTERM F2 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23312~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[12~",_TRUNCATE);
			}
			break;
		case IdXF3: /* XTERM F3 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23313~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[13~",_TRUNCATE);
			}
			break;
		case IdXF4: /* XTERM F4 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23314~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[14~",_TRUNCATE);
			}
			break;
		case IdXF5: /* XTERM F5 */
			if (Send8BitMode) {
				*Len = 4;
				strncpy_s(KeyStr,destlen,"\23315~",_TRUNCATE);
			} else {
				*Len = 5;
				strncpy_s(KeyStr,destlen,"\033[15~",_TRUNCATE);
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
		case IdCmdLocalEcho:
		case IdScrollLock:
			PostMessage(HWin,WM_USER_ACCELCOMMAND,KeyCode,0);
			break;
		default:
			if ((KeyCode >= IdUser1) && (KeyCode <= IdKeyMax)) {
				*Type = (WORD)(*KeyMap).UserKeyType[KeyCode-IdUser1]; // key type
				*Len = KeyMap->UserKeyLen[KeyCode-IdUser1];
				memcpy(Temp,
					   &KeyMap->UserKeyStr[KeyMap->UserKeyPtr[KeyCode-IdUser1]],
					   *Len);
				Temp[*Len] = 0;
				if ((*Type==IdBinary) || (*Type==IdText))
					*Len = Hex2Str(Temp,KeyStr,destlen);
				else
					strncpy_s(KeyStr,destlen,Temp,_TRUNCATE);
			}
			else
				return;
	}
	/* remove WM_CHAR message for used keycode */
	PeekMessage(&Msg,HWin, WM_CHAR, WM_CHAR,PM_REMOVE);
}

void FAR PASCAL SetCOMFlag(int Com)
{
	pm->ComFlag[(Com-1)/CHAR_BIT] |= 1 << ((Com-1)%CHAR_BIT);
}

void FAR PASCAL ClearCOMFlag(int Com)
{
	pm->ComFlag[(Com-1)/CHAR_BIT] &= ~(1 << ((Com-1)%CHAR_BIT));
}

int FAR PASCAL CheckCOMFlag(int Com)
{
	return ((pm->ComFlag[(Com-1)/CHAR_BIT] & 1 << (Com-1)%CHAR_BIT) > 0);
}

int FAR PASCAL RegWin(HWND HWinVT, HWND HWinTEK)
{
	int i, j;

	if (pm->NWin>=MAXNWIN)
		return 0;
	if (HWinVT==NULL)
		return 0;
	if (HWinTEK!=NULL) {
		i = 0;
		while ((i<pm->NWin) && (pm->WinList[i]!=HWinVT))
			i++;
		if (i>=pm->NWin)
			return 0;
		for (j=pm->NWin-1 ; j>i ; j--)
			pm->WinList[j+1] = pm->WinList[j];
		pm->WinList[i+1] = HWinTEK;
		pm->NWin++;
		return 0;
	}
	pm->WinList[pm->NWin++] = HWinVT;
	if (pm->NWin==1) {
		return 1;
	}
	else {
		return (int)(SendMessage(pm->WinList[pm->NWin-2],
		                         WM_USER_GETSERIALNO,0,0)+1);
	}
}

void FAR PASCAL UnregWin(HWND HWin)
{
	int i, j;

	i = 0;
	while ((i<pm->NWin) && (pm->WinList[i]!=HWin)) {
		i++;
	}
	if (pm->WinList[i]!=HWin) {
		return;
	}
	for (j=i ; j<pm->NWin-1 ; j++) {
		pm->WinList[j] = pm->WinList[j+1];
	}
	if (pm->NWin>0) {
		pm->NWin--;
	}
}

void FAR PASCAL SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag)
{
	int i;
	char Temp[MAXPATHLEN];
	HWND Hw;

	// delete all items in Window menu
	i = GetMenuItemCount(menu);
	if (i>0)
		do {
			i--;
			RemoveMenu(menu,i,MF_BYPOSITION);
		} while (i>0);

	i = 0;
	while (i<pm->NWin) {
		Hw = pm->WinList[i]; // get window handle
		if ((GetClassName(Hw,Temp,sizeof(Temp))>0) &&
		    ((strcmp(Temp,VTCLASSNAME)==0) ||
		     (strcmp(Temp,TEKCLASSNAME)==0))) {
			Temp[0] = '&';
			Temp[1] = (char)(0x31 + i);
			Temp[2] = ' ';
			GetWindowText(Hw,&Temp[3],sizeof(Temp)-4);
			AppendMenu(menu,MF_ENABLED | MF_STRING,ID_WINDOW_1+i,Temp);
			i++;
			if (i>8) {
				i = pm->NWin;
			}
		}
		else {
			UnregWin(Hw);
		}
	}
	get_lang_msg("MENU_WINDOW_WINDOW", buf, buflen, "&Window", langFile);
	if (VTFlag == 1) {
		AppendMenu(menu,MF_ENABLED | MF_STRING,ID_WINDOW_WINDOW, buf);
	}
	else {
		AppendMenu(menu,MF_ENABLED | MF_STRING,ID_TEKWINDOW_WINDOW, buf);
	}
}

void FAR PASCAL SetWinList(HWND HWin, HWND HDlg, int IList)
{
	int i;
	char Temp[MAXPATHLEN];
	HWND Hw;

	for (i=0; i<pm->NWin; i++) {
		Hw = pm->WinList[i]; // get window handle
		if ((GetClassName(Hw,Temp,sizeof(Temp))>0) &&
		    ((strcmp(Temp,VTCLASSNAME)==0) ||
		     (strcmp(Temp,TEKCLASSNAME)==0))) {
			GetWindowText(Hw,Temp,sizeof(Temp)-1);
			SendDlgItemMessage(HDlg, IList, LB_ADDSTRING,
			                   0, (LONG)Temp);
			if (Hw==HWin) {
				SendDlgItemMessage(HDlg, IList, LB_SETCURSEL, i,0);
			}
		}
		else {
			UnregWin(Hw);
		}
	}
}

void FAR PASCAL SelectWin(int WinId)
{
	if ((WinId>=0) && (WinId<pm->NWin)) {
		ShowWindow(pm->WinList[WinId],SW_SHOWNORMAL);
		SetForegroundWindow(pm->WinList[WinId]);
	}
}

void FAR PASCAL SelectNextWin(HWND HWin, int Next)
{
	int i;

	i = 0;
	while ((i<pm->NWin) && (pm->WinList[i]!=HWin)) {
		i++;
	}
	if (pm->WinList[i]!=HWin) {
		return;
	}
	i = i + Next;
	if (i >= pm->NWin) {
		i = 0;
	}
	else if (i<0) {
		i = pm->NWin-1;
	}
	SelectWin(i);
}

HWND FAR PASCAL GetNthWin(int n)
{
	if (n<pm->NWin) {
		return pm->WinList[n];
	}
	else {
		return NULL;
	}
}

int FAR PASCAL CommReadRawByte(PComVar cv, LPBYTE b)
{
	if ( ! cv->Ready ) {
		return 0;
	}

	if ( cv->InBuffCount>0 ) {
		*b = cv->InBuff[cv->InPtr];
		cv->InPtr++;
		cv->InBuffCount--;
		if ( cv->InBuffCount==0 ) {
			cv->InPtr = 0;
		}
		return 1;
	}
	else {
		cv->InPtr = 0;
		return 0;
	}
}

void PASCAL FAR CommInsert1Byte(PComVar cv, BYTE b)
{
	if ( ! cv->Ready ) {
		return;
	}

	if (cv->InPtr == 0) {
		memmove(&(cv->InBuff[1]),&(cv->InBuff[0]),cv->InBuffCount);
	}
	else {
		cv->InPtr--;
	}
	cv->InBuff[cv->InPtr] = b;
	cv->InBuffCount++;

	if (cv->HBinBuf!=0 ) {
		cv->BinSkip++;
	}
}

void Log1Bin(PComVar cv, BYTE b)
{
	if (((cv->FilePause & OpLog)!=0) || cv->ProtoFlag) {
		return;
	}
	if (cv->BinSkip > 0) {
		cv->BinSkip--;
		return;
	}
	cv->BinBuf[cv->BinPtr] = b;
	cv->BinPtr++;
	if (cv->BinPtr>=InBuffSize) {
		cv->BinPtr = cv->BinPtr-InBuffSize;
	}
	if (cv->BCount>=InBuffSize) {
		cv->BCount = InBuffSize;
		cv->BStart = cv->BinPtr;
	}
	else {
		cv->BCount++;
	}
}

int FAR PASCAL CommRead1Byte(PComVar cv, LPBYTE b)
{
	int c;

	if ( ! cv->Ready ) {
		return 0;
	}

	if ((cv->HLogBuf!=NULL) &&
	    ((cv->LCount>=InBuffSize-10) ||
	     (cv->DCount>=InBuffSize-10))) {
		// 自分のバッファに余裕がない場合は、CPUスケジューリングを他に回し、
		// CPUがストールするの防ぐ。
		// (2006.10.13 yutaka)
		Sleep(1);
		return 0;
	}

	if ((cv->HBinBuf!=NULL) &&
	    (cv->BCount>=InBuffSize-10)) {
		return 0;
	}

	if ( cv->TelMode ) {
		c = 0;
	}
	else {
		c = CommReadRawByte(cv,b);
	}

	if ((c==1) && cv->TelCRFlag) {
		cv->TelCRFlag = FALSE;
		if (*b==0) {
			c = 0;
		}
	}

	if ( c==1 ) {
		if ( cv->IACFlag ) {
			cv->IACFlag = FALSE;
			if ( *b != 0xFF ) {
				cv->TelMode = TRUE;
				CommInsert1Byte(cv,*b);
				if ( cv->HBinBuf!=0 ) {
					cv->BinSkip--;
				}
				c = 0;
			}
		}
		else if ((cv->PortType==IdTCPIP) && (*b==0xFF)) {
			if (!cv->TelFlag && cv->TelAutoDetect) { /* TTPLUG */
				cv->TelFlag = TRUE;
			}
			if (cv->TelFlag) {
				cv->IACFlag = TRUE;
				c = 0;
			}
		}
		else if (cv->TelFlag && ! cv->TelBinRecv && (*b==0x0D)) {
			cv->TelCRFlag = TRUE;
		}
	}

	if ( (c==1) && (cv->HBinBuf!=0) ) {
		Log1Bin(cv, *b);
	}

	return c;
}

int FAR PASCAL CommRawOut(PComVar cv, PCHAR B, int C)
{
	int a;

	if ( ! cv->Ready ) {
		return C;
	}

	if (C > OutBuffSize - cv->OutBuffCount) {
		a = OutBuffSize - cv->OutBuffCount;
	}
	else {
		a = C;
	}
	if ( cv->OutPtr > 0 ) {
		memmove(&(cv->OutBuff[0]),&(cv->OutBuff[cv->OutPtr]),cv->OutBuffCount);
		cv->OutPtr = 0;
	}
	memcpy(&(cv->OutBuff[cv->OutBuffCount]),B,a);
	cv->OutBuffCount = cv->OutBuffCount + a;
	return a;
}

int FAR PASCAL CommBinaryOut(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready ) {
		return C;
	}

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if ( cv->TelFlag && (B[i]=='\x0d') &&
		     ! cv->TelBinSend ) {
			d[Len] = '\x00';
			Len++;
		};

		if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len] = '\xff';
			Len++;
		}

		if ( OutBuffSize-cv->OutBuffCount-Len >=0 ) {
			CommRawOut(cv,d,Len);
			a = 1;
		}
		else {
			a = 0;
		}

		i = i + a;
	}
	return i;
}

static int OutputTextUTF8(WORD K, char *TempStr, PComVar cv)
{
	unsigned int code;
	int outlen;
	int TempLen = 0;

	code = SJIS2UTF8(K, &outlen, cv->Locale);
	switch (outlen) {
	  case 4:
		TempStr[TempLen++] = (code >> 24) & 0xff;
	  case 3:
		TempStr[TempLen++] = (code >> 16) & 0xff;
	  case 2:
		TempStr[TempLen++] = (code >> 8) & 0xff;
	  case 1:
		TempStr[TempLen++] = code & 0xff;
	}

	return TempLen;
}

// 
// MBCSから各種漢字コードへ変換して出力する。
//
int TextOutMBCS(PComVar cv, PCHAR B, int C)
{
	int i, TempLen, OutLen;
	WORD K;
	char TempStr[12];
	int SendCodeNew;
	BYTE d;
	BOOL Full, KanjiFlagNew;
	_locale_t locale;

	locale = _create_locale(LC_ALL, cv->Locale);

	Full = FALSE;
	i = 0;
	while (! Full && (i < C)) {
		TempLen = 0;
		d = (BYTE)B[i];
		SendCodeNew = cv->SendCode;
		KanjiFlagNew = FALSE;

		if (cv->SendKanjiFlag) {
			SendCodeNew = IdKanji;

			K = (cv->SendKanjiFirst << 8) + d;

			// UTF-8への変換を行う。1～3バイトまでの対応なので注意。
			if (cv->KanjiCodeSend == IdUTF8 || cv->Language == IdUtf8) {
				TempLen += OutputTextUTF8(K, TempStr, cv);
			}
			else {
				switch (cv->Language) {
				  case IdJapanese:
				  	switch (cv->KanjiCodeSend) {
					  case IdEUC:
						K = SJIS2EUC(K);
						break;
					  case IdJIS:
						K = SJIS2JIS(K);
						if ((cv->SendCode==IdKatakana) &&
						    (cv->JIS7KatakanaSend==1)) {
							TempStr[TempLen++] = SI;
						}
						break;
					  case IdSJIS:
						/* nothing to do */
						break;
					}
					break;
				  case IdKorean:
				  	break;
				}
				TempStr[TempLen++] = HIBYTE(K);
				TempStr[TempLen++] = LOBYTE(K);
			}
		}
		else if (_isleadbyte_l(d, locale)) {
			KanjiFlagNew = TRUE;
			cv->SendKanjiFirst = d;
			SendCodeNew = IdKanji;

			if (cv->Language == IdJapanese) {
				if ((cv->SendCode!=IdKanji) && (cv->KanjiCodeSend==IdJIS)) {
					TempStr[0] = 0x1B;
					TempStr[1] = '$';
					if (cv->KanjiIn == IdKanjiInB) {
						TempStr[2] = 'B';
					}
					else {
						TempStr[2] = '@';
					}
					TempLen = 3;
				}
			}
		}
		else {
			if (cv->Language == IdJapanese) {
				if ((cv->SendCode==IdKanji) && (cv->KanjiCodeSend==IdJIS)) {
					TempStr[0] = 0x1B;
					TempStr[1] = '(';
					switch (cv->KanjiOut) {
					  case IdKanjiOutJ:
						TempStr[2] = 'J';
						break;
					  case IdKanjiOutH:
						TempStr[2] = 'H';
						break;
					  default:
						TempStr[2] = 'B';
					}
					TempLen = 3;
				}

				if ((0xa0<d) && (d<0xe0)) {
					SendCodeNew = IdKatakana;
					if ((cv->SendCode!=IdKatakana) &&
					    (cv->KanjiCodeSend==IdJIS) &&
					    (cv->JIS7KatakanaSend==1)) {
						TempStr[TempLen++] = SO;
					}
				}
				else {
					SendCodeNew = IdASCII;
					if ((cv->SendCode==IdKatakana) &&
					    (cv->KanjiCodeSend==IdJIS) &&
					    (cv->JIS7KatakanaSend==1)) {
						TempStr[TempLen++] = SI;
					}
				}
			}

			if (d==CR) {
				TempStr[TempLen++] = 0x0d;
				if (cv->CRSend==IdCRLF) {
					TempStr[TempLen++] = 0x0a;
				}
				else if ((cv->CRSend==IdCR) &&
				          cv->TelFlag && ! cv->TelBinSend) {
					TempStr[TempLen++] = 0;
				}
				if (cv->TelLineMode) {
					cv->Flush = TRUE;
				}
			}
			else if (d==BS) {
				if (cv->TelLineMode) {
					if (cv->FlushLen < cv->LineModeBuffCount) {
						cv->LineModeBuffCount--;
					}
				}
		  		else {
					TempStr[TempLen++] = d;
				}
			}
			else if (d==0x15) { // Ctrl-U
				if (cv->TelLineMode) {
					cv->LineModeBuffCount = cv->FlushLen;
				}
		  		else {
					TempStr[TempLen++] = d;
				}
			}
			else if ((d>=0x80) && (cv->KanjiCodeSend==IdUTF8 || cv->Language==IdUtf8)) {
				TempLen += OutputTextUTF8((WORD)d, TempStr, cv);
			}
			else if ((d>=0xa1) && (d<=0xe0) && (cv->Language == IdJapanese)) {
				/* Katakana */
				if (cv->KanjiCodeSend==IdEUC) {
					TempStr[TempLen++] = (char)SS2;
				}
				if ((cv->KanjiCodeSend==IdJIS) &&
					(cv->JIS7KatakanaSend==1)) {
					TempStr[TempLen++] = d & 0x7f;
				}
				else {
					TempStr[TempLen++] = d;
				}
			}
			else {
				TempStr[TempLen++] = d;
				if (cv->TelFlag && (d==0xff)) {
					TempStr[TempLen++] = (char)0xff;
				}
			}
		} // if (cv->SendKanjiFlag) else if ... else ... end

		if (cv->TelLineMode) {
			if (TempLen == 0) {
				i++;
				cv->SendCode = SendCodeNew;
				cv->SendKanjiFlag = KanjiFlagNew;
			}
			else {
				Full = OutBuffSize - cv->LineModeBuffCount - TempLen < 0;
				if (!Full) {
					i++;
					cv->SendCode = SendCodeNew;
					cv->SendKanjiFlag = KanjiFlagNew;
					memcpy(&(cv->LineModeBuff[cv->LineModeBuffCount]), TempStr, TempLen);
					cv->LineModeBuffCount += TempLen;
					if (cv->Flush) {
						cv->FlushLen = cv->LineModeBuffCount;
					}
				}
			}
			if (cv->FlushLen > 0) {
				OutLen = CommRawOut(cv, cv->LineModeBuff, cv->FlushLen);
				cv->FlushLen -= OutLen;
				cv->LineModeBuffCount -= OutLen;
				memmove(cv->LineModeBuff, &(cv->LineModeBuff[OutLen]), cv->LineModeBuffCount);
			}
			cv->Flush = FALSE;
		}
		else {
			if (TempLen == 0) {
				i++;
				cv->SendCode = SendCodeNew;
				cv->SendKanjiFlag = KanjiFlagNew;
			}
			else {
				Full = OutBuffSize-cv->OutBuffCount-TempLen < 0;
				if (! Full) {
					i++;
					cv->SendCode = SendCodeNew;
					cv->SendKanjiFlag = KanjiFlagNew;
					CommRawOut(cv,TempStr,TempLen);
				}
			}
		}

	} // end of "while {}"

	_free_locale(locale);

	return i;
}

int FAR PASCAL CommTextOut(PComVar cv, PCHAR B, int C)
{
	int i, TempLen, OutLen;
	char TempStr[12];
	BYTE d;
	BOOL Full;

	if (! cv->Ready ) {
		return C;
	}

	switch (cv->Language) {
	  case IdUtf8:
	  case IdJapanese:
	  case IdKorean:
		return TextOutMBCS(cv, B, C);
		break;
	}

	Full = FALSE;
	i = 0;
	while (! Full && (i < C)) {
		TempLen = 0;
		d = (BYTE)B[i];
	
		switch (d) {
		  case CR:
			TempStr[TempLen] = 0x0d;
			TempLen++;
			if (cv->CRSend==IdCRLF) {
				TempStr[TempLen++] = 0x0a;
			}
			else if (cv->CRSend==IdCR && cv->TelFlag && ! cv->TelBinSend) {
				TempStr[TempLen++] = 0;
			}
			if (cv->TelLineMode) {
				cv->Flush = TRUE;
			}
			break;

		  case BS:
			if (cv->TelLineMode) {
				if (cv->FlushLen < cv->LineModeBuffCount) {
					cv->LineModeBuffCount--;
				}
			}
		  	else {
				TempStr[TempLen++] = d;
			}
			break;

		  case 0x15: // Ctrl-U
			if (cv->TelLineMode) {
				cv->LineModeBuffCount = cv->FlushLen;
			}
			else {
				TempStr[TempLen++] = d;
			}

		  default:
			if ((cv->Language==IdRussian) && (d>=128)) {
				d = RussConv(cv->RussClient, cv->RussHost, d);
			}
			TempStr[TempLen++] = d;
			if (cv->TelFlag && (d==0xff)) {
				TempStr[TempLen++] = (char)0xff;
			}
		}

		if (cv->TelLineMode) {
			Full = OutBuffSize - cv->LineModeBuffCount - TempLen < 0;
			if (!Full) {
				memcpy(&(cv->LineModeBuff[cv->LineModeBuffCount]), TempStr, TempLen);
				cv->LineModeBuffCount += TempLen;
				if (cv->Flush) {
					cv->FlushLen = cv->LineModeBuffCount;
				}
			}
			if (cv->FlushLen > 0) {
				OutLen = CommRawOut(cv, cv->LineModeBuff, cv->FlushLen);
				cv->FlushLen -= OutLen;
				cv->LineModeBuffCount -= OutLen;
				memmove(cv->LineModeBuff, &(cv->LineModeBuff[OutLen]), cv->LineModeBuffCount);
			}
			cv->Flush = FALSE;
		}
		else {
			Full = OutBuffSize - cv->OutBuffCount - TempLen < 0;
			if (! Full) {
				i++;
				CommRawOut(cv,TempStr,TempLen);
			}
		}
	} // end of while {}

	return i;
}

int FAR PASCAL CommBinaryEcho(PComVar cv, PCHAR B, int C)
{
	int a, i, Len;
	char d[3];

	if ( ! cv->Ready )
		return C;

	if ( (cv->InPtr>0) && (cv->InBuffCount>0) ) {
		memmove(cv->InBuff,&(cv->InBuff[cv->InPtr]),cv->InBuffCount);
		cv->InPtr = 0;
	}

	i = 0;
	a = 1;
	while ((a>0) && (i<C)) {
		Len = 0;

		d[Len] = B[i];
		Len++;

		if ( cv->TelFlag && (B[i]=='\x0d') &&
		     ! cv->TelBinSend ) {
			d[Len] = 0x00;
			Len++;
		}

		if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len] = '\xff';
			Len++;
		}

		if ( InBuffSize-cv->InBuffCount-Len >=0 ) {
			memcpy(&(cv->InBuff[cv->InBuffCount]),d,Len);
			cv->InBuffCount = cv->InBuffCount + Len;
			a = 1;
		}
		else
			a = 0;
		i = i + a;
	}
	return i;
}

int FAR PASCAL TextEchoMBCS(PComVar cv, PCHAR B, int C)
{
	int i, TempLen;
	WORD K;
	char TempStr[12];
	int EchoCodeNew;
	BYTE d;
	BOOL Full, KanjiFlagNew;
	_locale_t locale;

	locale = _create_locale(LC_ALL, cv->Locale);

	Full = FALSE;
	i = 0;
	while (! Full && (i < C)) {
		TempLen = 0;
		d = (BYTE)B[i];
		EchoCodeNew = cv->EchoCode;
		KanjiFlagNew = FALSE;

		if (cv->EchoKanjiFlag) {
			EchoCodeNew = IdKanji;

			K = (cv->EchoKanjiFirst << 8) + d;

			// UTF-8への変換を行う。1～3バイトまでの対応なので注意。
			if (cv->KanjiCodeEcho == IdUTF8 || cv->Language==IdUtf8) {
				TempLen += OutputTextUTF8(K, TempStr, cv);
			}
			else {
				switch (cv->Language) {
				  case IdJapanese:
					switch (cv->KanjiCodeEcho) {
					  case IdEUC:
						K = SJIS2EUC(K);
						break;
					  case IdJIS:
						K = SJIS2JIS(K);
						if ((cv->EchoCode==IdKatakana) &&
						    (cv->JIS7KatakanaEcho==1)) {
							TempStr[TempLen++] = SI;
						}
						break;
					  case IdSJIS:
						/* nothing to do */
						break;
					}
					break;
				  case IdKorean:
					break;
				}
				TempStr[TempLen++] = HIBYTE(K);
				TempStr[TempLen++] = LOBYTE(K);
			}
		}
		else if (_isleadbyte_l(d, locale)) {
			KanjiFlagNew = TRUE;
			cv->EchoKanjiFirst = d;
			EchoCodeNew = IdKanji;

			if (cv->Language == IdJapanese) {
				if ((cv->EchoCode!=IdKanji) && (cv->KanjiCodeEcho==IdJIS)) {
					TempStr[0] = 0x1B;
					TempStr[1] = '$';
					if (cv->KanjiIn == IdKanjiInB) {
						TempStr[2] = 'B';
					}
					else {
						TempStr[2] = '@';
					}
					TempLen = 3;
				}
			}
		}
		else {
			if (cv->Language == IdJapanese) {
				if ((cv->EchoCode==IdKanji) && (cv->KanjiCodeEcho==IdJIS)) {
					TempStr[0] = 0x1B;
					TempStr[1] = '(';
					switch (cv->KanjiOut) {
					  case IdKanjiOutJ:
						TempStr[2] = 'J';
						break;
					  case IdKanjiOutH:
						TempStr[2] = 'H';
						break;
					  default:
						TempStr[2] = 'B';
					}
					TempLen = 3;
				}

				if ((0xa0<d) && (d<0xe0)) {
					EchoCodeNew = IdKatakana;
					if ((cv->EchoCode!=IdKatakana) &&
					    (cv->KanjiCodeEcho==IdJIS) &&
					    (cv->JIS7KatakanaEcho==1)) {
						TempStr[TempLen++] = SO;
					}
				}
				else {
					EchoCodeNew = IdASCII;
					if ((cv->EchoCode==IdKatakana) &&
					    (cv->KanjiCodeEcho==IdJIS) &&
					    (cv->JIS7KatakanaEcho==1)) {
						TempStr[TempLen++] = SI;
					}
				}
			}

			if (d==CR) {
				TempStr[TempLen++] = 0x0d;
				if (cv->CRSend==IdCRLF) {
					TempStr[TempLen++] = 0x0a;
				}
				else if ((cv->CRSend==IdCR) &&
				          cv->TelFlag && ! cv->TelBinSend) {
					TempStr[TempLen++] = 0;
				}
			}
			else if (d==0x15) { // Ctrl-U
				if (cv->TelLineMode) {
					// Move to top of line (CHA "\033[G") and erase line (EL "\033[K")
					strncpy_s(TempStr, sizeof(TempStr), "\033[G\033[K", _TRUNCATE);
					TempLen += 6;
				}
				else {
					TempStr[TempLen++] = d;
				}
			}
			else if ((d>=0x80) && (cv->KanjiCodeEcho==IdUTF8 || cv->Language==IdUtf8)) {
				TempLen += OutputTextUTF8((WORD)d, TempStr, cv);
			}
			else if ((d>=0xa1) && (d<=0xe0) && (cv->Language == IdJapanese)) {
				/* Katakana */
				if (cv->KanjiCodeEcho==IdEUC) {
					TempStr[TempLen++] = (char)SS2;
				}
				if ((cv->KanjiCodeEcho==IdJIS) &&
					(cv->JIS7KatakanaEcho==1)) {
					TempStr[TempLen++] = d & 0x7f;
				}
				else {
					TempStr[TempLen++] = d;
				}
			}
			else {
				TempStr[TempLen++] = d;
				if (cv->TelFlag && (d==0xff)) {
					TempStr[TempLen++] = (char)0xff;
				}
			}
		} // if (cv->EchoKanjiFlag) else if ... else ... end

		if (TempLen == 0) {
			i++;
			cv->EchoCode = EchoCodeNew;
			cv->EchoKanjiFlag = KanjiFlagNew;
		}
		else {
			Full = InBuffSize-cv->InBuffCount-TempLen < 0;
			if (! Full) {
				i++;
				cv->EchoCode = EchoCodeNew;
				cv->EchoKanjiFlag = KanjiFlagNew;
				memcpy(&(cv->InBuff[cv->InBuffCount]),TempStr,TempLen);
				cv->InBuffCount = cv->InBuffCount + TempLen;
			}
		}

	} // end of "while {}"

	_free_locale(locale);

	return i;
}

int FAR PASCAL CommTextEcho(PComVar cv, PCHAR B, int C)
{
	int i, TempLen;
	char TempStr[11];
	BYTE d;
	BOOL Full;

	if ( ! cv->Ready ) {
		return C;
	}

	if ( (cv->InPtr>0) && (cv->InBuffCount>0) ) {
		memmove(cv->InBuff,&(cv->InBuff[cv->InPtr]),cv->InBuffCount);
		cv->InPtr = 0;
	}

	switch (cv->Language) {
	  case IdUtf8:
	  case IdJapanese:
	  case IdKorean:
		return TextEchoMBCS(cv,B,C);
		break;
	}

	Full = FALSE;
	i = 0;
	while (! Full && (i < C)) {
		TempLen = 0;
		d = (BYTE)B[i];

		switch (d) {
		  case CR:
			TempStr[TempLen] = 0x0d;
			TempLen++;
			if (cv->CRSend==IdCRLF) {
				TempStr[TempLen++] = 0x0a;
			}
			else if (cv->CRSend==IdCR && cv->TelFlag && ! cv->TelBinSend) {
				TempStr[TempLen++] = 0;
			}
			break;
		  
		  case 0x15: // Ctrl-U
			if (cv->TelLineMode) {
				// Move to top of line (CHA "\033[G") and erase line (EL "\033[K")
				strncpy_s(TempStr, sizeof(TempStr), "\033[G\033[K", _TRUNCATE);
				TempLen += 6;
			}
			else {
				TempStr[TempLen++] = d;
			}
			break;

		  default:
			if ((cv->Language==IdRussian) && (d>=128)) {
				d = RussConv(cv->RussClient,cv->RussHost,d);
			}
			TempStr[TempLen++] = d;
			if (cv->TelFlag && (d==0xff)) {
				TempStr[TempLen++] = (char)0xff;
			}
		}

		Full = InBuffSize-cv->InBuffCount-TempLen < 0;
		if (! Full) {
			i++;
			memcpy(&(cv->InBuff[cv->InBuffCount]),TempStr,TempLen);
			cv->InBuffCount = cv->InBuffCount + TempLen;
		}
	} // end of while {}

	return i;
}

// listup serial port driver 
// cf. http://www.codeproject.com/system/setupdi.asp?df=100&forumid=4368&exp=0&select=479661
// (2007.8.17 yutaka)
static void ListupSerialPort(LPWORD ComPortTable, int comports, char **ComPortDesc, int ComPortMax)
{
	GUID ClassGuid[1];
	DWORD dwRequiredSize;
	BOOL bRet;
	HDEVINFO DeviceInfoSet = NULL;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD dwMemberIndex = 0;
	int i;

	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	// 以前のメモリをフリーしておく
	for (i = 0 ; i < ComPortMax ; i++) {
		free(ComPortDesc[i]);
		ComPortDesc[i] = NULL;
	}

// Get ClassGuid from ClassName for PORTS class
	bRet =
		SetupDiClassGuidsFromName(_T("PORTS"), (LPGUID) & ClassGuid, 1,
		                          &dwRequiredSize);
	if (!bRet) {
		goto cleanup;
	}

// Get class devices
	// COMポート番号を強制付け替えした場合に、現在のものではなく、レジストリに残っている
	// 古いFriendlyNameが表示されてしまう問題への対処。(2007.11.8 yutaka)
	DeviceInfoSet =
		SetupDiGetClassDevs(&ClassGuid[0], NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);

	if (DeviceInfoSet) {
// Enumerate devices
		dwMemberIndex = 0;
		while (SetupDiEnumDeviceInfo
		       (DeviceInfoSet, dwMemberIndex++, &DeviceInfoData)) {
			TCHAR szFriendlyName[MAX_PATH];
			TCHAR szPortName[MAX_PATH];
			//TCHAR szMessage[MAX_PATH];
			DWORD dwReqSize = 0;
			DWORD dwPropType;
			DWORD dwType = REG_SZ;
			HKEY hKey = NULL;

// Get friendlyname
			bRet = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
			                                        &DeviceInfoData,
			                                        SPDRP_FRIENDLYNAME,
			                                        &dwPropType,
			                                        (LPBYTE)
			                                        szFriendlyName,
			                                        sizeof(szFriendlyName),
			                                        &dwReqSize);

// Open device parameters reg key
			hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
			                            &DeviceInfoData,
			                            DICS_FLAG_GLOBAL,
			                            0, DIREG_DEV, KEY_READ);
			if (hKey) {
// Qurey for portname
				long lRet;
				dwReqSize = sizeof(szPortName);
				lRet = RegQueryValueEx(hKey,
				                       _T("PortName"),
				                       0,
				                       &dwType,
				                       (LPBYTE) & szPortName,
				                       &dwReqSize);

// Close reg key
				RegCloseKey(hKey);
			}

#if 0
			sprintf(szMessage, _T("Name: %s\nPort: %s\n"), szFriendlyName,
			        szPortName);
			printf("%s\n", szMessage);
#endif

			if (_strnicmp(szPortName, "COM", 3) == 0) {  // COMポートドライバを発見
				int port = atoi(&szPortName[3]);
				int i;

				for (i = 0 ; i < comports ; i++) {
					if (ComPortTable[i] == port) {  // 接続を確認
						ComPortDesc[i] = _strdup(szFriendlyName);
						break;
					}
				}
			}

		}
	}

cleanup:
// Destroy device info list
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
}


int PASCAL DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc)
{
	HMODULE h;
	TCHAR   devicesBuff[65535];
	TCHAR   *p;
	int     comports = 0;
	int     i, j, min;
	WORD    s;

	if (((h = GetModuleHandle("kernel32.dll")) != NULL) &&
	    (GetProcAddress(h, "QueryDosDeviceA") != NULL) &&
	    (QueryDosDevice(NULL, devicesBuff, 65535) != 0)) {
		p = devicesBuff;
		while (*p != '\0') {
			if (strncmp(p, "COM", 3) == 0 && p[3] != '\0') {
				ComPortTable[comports++] = atoi(p+3);
				if (comports >= ComPortMax)
					break;
			}
			p += (strlen(p)+1);
		}

		for (i=0; i<comports-1; i++) {
			min = i;
			for (j=i+1; j<comports; j++) {
				if (ComPortTable[min] > ComPortTable[j]) {
					min = j;
				}
			}
			if (min != i) {
				s = ComPortTable[i];
				ComPortTable[i] = ComPortTable[min];
				ComPortTable[min] = s;
			}
		}
	}
	else {
#if 1
		for (i=1; i<=ComPortMax; i++) {
			FILE *fp;
			char buf[11]; // \\.\COMxxx + NULL
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, "\\\\.\\COM%d", i);
			if ((fp = fopen(buf, "r")) != NULL) {
				fclose(fp);
				ComPortTable[comports++] = i;
			}
		}
#else
		comports = -1;
#endif
	}

	ListupSerialPort(ComPortTable, comports, ComPortDesc, ComPortMax);

	return comports;
}

BOOL WINAPI DllMain(HANDLE hInstance,
                    ULONG ul_reason_for_call,
                    LPVOID lpReserved)
{
	switch( ul_reason_for_call ) {
		case DLL_THREAD_ATTACH:
			/* do thread initialization */
			break;
		case DLL_THREAD_DETACH:
			/* do thread cleanup */
			break;
		case DLL_PROCESS_ATTACH:
			/* do process initialization */
			DoCover_IsDebuggerPresent();
			hInst = hInstance;
			HMap = CreateFileMapping((HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE,
			                         0, sizeof(TMap), TT_FILEMAPNAME);
			if (HMap == NULL) {
				return FALSE;
			}
			FirstInstance = (GetLastError() != ERROR_ALREADY_EXISTS);

			pm = (PMap)MapViewOfFile(HMap,FILE_MAP_WRITE,0,0,0);
			if (pm == NULL) {
				return FALSE;
			}
			break;
		case DLL_PROCESS_DETACH:
			/* do process cleanup */
			UnmapViewOfFile(pm);
			CloseHandle(HMap);
			break;
	}
	return TRUE;
}
