/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2004-2019 TeraTerm Project
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

#ifndef _WIN32_IE
#define _WIN32_IE 0x501
#endif

/* TTCMN.DLL, main */
#include <direct.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <locale.h>
#include <htmlhelp.h>

#define DllExport __declspec(dllexport)
#include "language.h"
#undef DllExport

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttlib.h"
#include "compat_w95.h"
#include "tt_res.h"
#include "codeconv.h"

#define DllExport __declspec(dllexport)
#include "ttcommon.h"


// TMap を格納するファイルマッピングオブジェクト(共有メモリ)の名前
// TMap(とそのメンバ)の更新時は旧バージョンとの同時起動の為に変える必要があるが
// 連番からバージョン番号を使うように変更した為、通常は手動で変更する必要は無い
#define TT_FILEMAPNAME "ttset_memfilemap_" TT_VERSION_STR("_")

/* first instance flag */
static BOOL FirstInstance = TRUE;

static HINSTANCE hInst;

static PMap pm;

static HANDLE HMap = NULL;
#define VTCLASSNAME _T("VTWin32")
#define TEKCLASSNAME _T("TEKWin32")

#ifdef UNICODE
static HWND(WINAPI *pHtmlHelp)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
#define HTMLHELP_API_NAME	"HtmlHelpW"
#else
static HWND(WINAPI *pHtmlHelp)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);
#define HTMLHELP_API_NAME	"HtmlHelpA"
#endif

enum window_style {
	WIN_CASCADE,
	WIN_STACKED,
	WIN_SIDEBYSIDE,
};


void WINAPI CopyShmemToTTSet(PTTSet ts)
{
	// 現在の設定を共有メモリからコピーする
	memcpy(ts, &pm->ts, sizeof(TTTSet));
}

void WINAPI CopyTTSetToShmem(PTTSet ts)
{
	// 現在の設定を共有メモリへコピーする
	memcpy(&pm->ts, ts, sizeof(TTTSet));
}


BOOL WINAPI StartTeraTerm(PTTSet ts)
{
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
	GetHomeDir(hInst, ts->HomeDir, sizeof(ts->HomeDir));
	_chdir(ts->HomeDir);
	GetDefaultSetupFName(ts->HomeDir, ts->SetupFName, sizeof(ts->SetupFName));

	strncpy_s(ts->KeyCnfFN, sizeof(ts->KeyCnfFN), ts->HomeDir, _TRUNCATE);
	AppendSlash(ts->KeyCnfFN, sizeof(ts->KeyCnfFN));
	strncat_s(ts->KeyCnfFN, sizeof(ts->KeyCnfFN), "KEYBOARD.CNF", _TRUNCATE);

	if (FirstInstance) {
		FirstInstance = FALSE;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// 設定ファイルをディスクに保存し、Tera Term本体を再起動する。
// (2012.4.30 yutaka)
void WINAPI RestartTeraTerm(HWND hwnd, PTTSet ts)
{
	char path[1024];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char uimsg[MAX_UIMSG];
	int ret;

	get_lang_msg("MSG_TT_TAKE_EFFECT", uimsg, sizeof(uimsg),
		"This option takes effect the next time a session is started.\n"
		"Are you sure that you want to relaunch Tera Term?"
		, ts->UILanguageFile);
	ret = MessageBox(hwnd, uimsg, "Tera Term: Configuration Warning", MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
	if (ret != IDYES)
		return;

	SendMessage(hwnd, WM_COMMAND, ID_SETUP_SAVE, 0);
	// ID_FILE_EXIT メッセージではアプリが落ちることがあるため、WM_QUIT をポストする。
	//PostMessage(hwnd, WM_COMMAND, ID_FILE_EXIT, 0);
	PostQuitMessage(0);

	// 自プロセスの再起動。
	if (GetModuleFileName(NULL, path, sizeof(path)) == 0) {
		return;
	}
	memset(&si, 0, sizeof(si));
	GetStartupInfo(&si);
	memset(&pi, 0, sizeof(pi));
	if (CreateProcess(NULL, path, NULL, NULL, FALSE, 0,
	                  NULL, NULL, &si, &pi) == 0) {
	}
}

void WINAPI ChangeDefaultSet(PTTSet ts, PKeyMap km)
{
	if ((ts!=NULL) &&
		(_stricmp(ts->SetupFName, pm->ts.SetupFName) == 0)) {
		memcpy(&(pm->ts),ts,sizeof(TTTSet));
	}
	if (km!=NULL) {
		memcpy(&(pm->km),km,sizeof(TKeyMap));
	}
}

void WINAPI GetDefaultSet(PTTSet ts)
{
	memcpy(ts,&(pm->ts),sizeof(TTTSet));
}


/* Key scan code -> Tera Term key code */
WORD WINAPI GetKeyCode(PKeyMap KeyMap, WORD Scan)
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

void WINAPI GetKeyStr(HWND HWin, PKeyMap KeyMap, WORD KeyCode,
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
				*Type = IdText; // do new-line conversion
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
		case IdXBackTab: /* XTERM Back Tab */
			if (Send8BitMode) {
				*Len = 2;
				strncpy_s(KeyStr,destlen,"\233Z",_TRUNCATE);
			} else {
				*Len = 3;
				strncpy_s(KeyStr,destlen,"\033[Z",_TRUNCATE);
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

void WINAPI SetCOMFlag(int Com)
{
	pm->ComFlag[(Com-1)/CHAR_BIT] |= 1 << ((Com-1)%CHAR_BIT);
}

void WINAPI ClearCOMFlag(int Com)
{
	pm->ComFlag[(Com-1)/CHAR_BIT] &= ~(1 << ((Com-1)%CHAR_BIT));
}

int WINAPI CheckCOMFlag(int Com)
{
	return ((pm->ComFlag[(Com-1)/CHAR_BIT] & 1 << (Com-1)%CHAR_BIT) > 0);
}

int WINAPI RegWin(HWND HWinVT, HWND HWinTEK)
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
	memset(&pm->WinPrevRect[pm->NWin - 1], 0, sizeof(pm->WinPrevRect[pm->NWin - 1])); // RECT clear
	if (pm->NWin==1) {
		return 1;
	}
	else {
		return (int)(SendMessage(pm->WinList[pm->NWin-2],
		                         WM_USER_GETSERIALNO,0,0)+1);
	}
}

void WINAPI UnregWin(HWND HWin)
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
		pm->WinPrevRect[j] = pm->WinPrevRect[j+1];  // RECT shift
	}
	if (pm->NWin>0) {
		pm->NWin--;
	}
}

char GetWindowTypeChar(HWND Hw, HWND HWin)
{
#if 0
	if (HWin == Hw)
		return '*';
	else if (!IsWindowVisible(Hw))
#else
	if (!IsWindowVisible(Hw))
#endif
		return '#';
	else if (IsIconic(Hw))
		return '-';
	else if (IsZoomed(Hw))
		return '@';
	else
		return '+';
}

void WINAPI SetWinMenu(HMENU menu, PCHAR buf, int buflen, PCHAR langFile, int VTFlag)
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
			Temp[3] = GetWindowTypeChar(Hw, NULL);
			Temp[4] = ' ';
			GetWindowText(Hw,&Temp[5],sizeof(Temp)-6);
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
		AppendMenu(menu, MF_SEPARATOR, 0, NULL);
		AppendMenu(menu,MF_ENABLED | MF_STRING,ID_WINDOW_WINDOW, buf);

		get_lang_msg("MENU_WINDOW_MINIMIZEALL", buf, buflen, "&Minimize All", langFile);
		AppendMenu(menu, MF_ENABLED | MF_STRING, ID_WINDOW_MINIMIZEALL, buf);

		get_lang_msg("MENU_WINDOW_RESTOREALL", buf, buflen, "&Restore All", langFile);
		AppendMenu(menu, MF_ENABLED | MF_STRING, ID_WINDOW_RESTOREALL, buf);

		get_lang_msg("MENU_WINDOW_CASCADE", buf, buflen, "&Cascade", langFile);
		AppendMenu(menu, MF_ENABLED | MF_STRING, ID_WINDOW_CASCADEALL, buf);

		get_lang_msg("MENU_WINDOW_STACKED", buf, buflen, "&Stacked", langFile);
		AppendMenu(menu, MF_ENABLED | MF_STRING, ID_WINDOW_STACKED, buf);

		get_lang_msg("MENU_WINDOW_SIDEBYSIDE", buf, buflen, "Side &by Side", langFile);
		AppendMenu(menu, MF_ENABLED | MF_STRING, ID_WINDOW_SIDEBYSIDE, buf);

		if (pm->WinUndoFlag) {
			if (pm->WinUndoStyle == WIN_CASCADE)
				get_lang_msg("MENU_WINDOW_CASCADE_UNDO", buf, buflen, "&Undo - Cascade", langFile);
			else if (pm->WinUndoStyle == WIN_STACKED)
				get_lang_msg("MENU_WINDOW_STACKED_UNDO", buf, buflen, "&Undo - Stacked", langFile);
			else
				get_lang_msg("MENU_WINDOW_SIDEBYSIDE_UNDO", buf, buflen, "&Undo - Side by Side", langFile);
			AppendMenu(menu, MF_ENABLED | MF_STRING, ID_WINDOW_UNDO, buf);
		}

	}
	else {
		AppendMenu(menu,MF_ENABLED | MF_STRING,ID_TEKWINDOW_WINDOW, buf);
	}
}

void WINAPI SetWinList(HWND HWin, HWND HDlg, int IList)
{
	int i;
	char Temp[MAXPATHLEN];
	HWND Hw;

	for (i=0; i<pm->NWin; i++) {
		Hw = pm->WinList[i]; // get window handle
		if ((GetClassName(Hw,Temp,sizeof(Temp))>0) &&
		    ((strcmp(Temp,VTCLASSNAME)==0) ||
		     (strcmp(Temp,TEKCLASSNAME)==0))) {
			Temp[0] = GetWindowTypeChar(Hw, HWin);
			Temp[1] = ' ';
			GetWindowText(Hw,&Temp[2],sizeof(Temp)-3);
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

void WINAPI SelectWin(int WinId)
{
	if ((WinId>=0) && (WinId<pm->NWin)) {
		/* ウィンドウが最大化および最小化されていた場合、その状態を維持できるように、
		 * SW_SHOWNORMAL から SW_SHOW へ変更した。
		 * (2009.11.8 yutaka)
		 * ウィンドウが最小化されているときは元のサイズに戻す(SW_RESTORE)ようにした。
		 * (2009.11.9 maya)
		 */
		if (IsIconic(pm->WinList[WinId])) {
			ShowWindow(pm->WinList[WinId],SW_RESTORE);
		}
		else {
			ShowWindow(pm->WinList[WinId],SW_SHOW);
		}
		SetForegroundWindow(pm->WinList[WinId]);
	}
}

void WINAPI SelectNextWin(HWND HWin, int Next, BOOL SkipIconic)
{
	int i;

	i = 0;
	while ((i < pm->NWin) && (pm->WinList[i]!=HWin)) {
		i++;
	}
	if (pm->WinList[i]!=HWin) {
		return;
	}

	do {
		i += Next;
		if (i >= pm->NWin) {
			i = 0;
		}
		else if (i < 0) {
			i = pm->NWin-1;
		}

		if (pm->WinList[i] == HWin) {
			break;
		}
	} while ((SkipIconic && IsIconic(pm->WinList[i])) || !IsWindowVisible(pm->WinList[i]));

	SelectWin(i);
}

void WINAPI ShowAllWin(int stat) {
	int i;

	for (i=0; i < pm->NWin; i++) {
		ShowWindow(pm->WinList[i], stat);
	}
}

void WINAPI UndoAllWin(void) {
	int i;
	WINDOWPLACEMENT rc0;
	RECT rc;
	HMONITOR hMonitor;
	MONITORINFO mi;
	int stat = SW_RESTORE;
	int multi_mon = 0;

	if (HasMultiMonitorSupport()) {
		multi_mon = 1;
	}

	// 一度、復元したらフラグは落とす。
	pm->WinUndoFlag = FALSE;

	memset(&rc0, 0, sizeof(rc0));

	for (i=0; i < pm->NWin; i++) {
		// 復元指定で、前回の状態が残っている場合は、ウィンドウの状態を元に戻す。
		if (stat == SW_RESTORE && memcmp(&pm->WinPrevRect[i], &rc0, sizeof(rc0)) != 0) {
			rc = pm->WinPrevRect[i].rcNormalPosition;

			// NT4.0, 95 はマルチモニタAPIに非対応
			if (multi_mon) {
				// 対象モニタの情報を取得
				hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
				mi.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hMonitor, &mi);

				// 位置補正（復元前後で解像度が変わっている場合への対策）
				if (rc.right > mi.rcMonitor.right) {
					rc.left -= rc.right - mi.rcMonitor.right;
					rc.right = mi.rcMonitor.right;
				}
				if (rc.left < mi.rcMonitor.left) {
					rc.right += mi.rcMonitor.left - rc.left;
					rc.left = mi.rcMonitor.left;
				}
				if (rc.bottom > mi.rcMonitor.bottom) {
					rc.top -= rc.bottom - mi.rcMonitor.bottom;
					rc.bottom = mi.rcMonitor.bottom;
				}
				if (rc.top < mi.rcMonitor.top) {
					rc.bottom += mi.rcMonitor.top - rc.top;
					rc.top = mi.rcMonitor.top;
				}
			}

			// ウィンドウ位置復元
			SetWindowPos(
				pm->WinList[i], NULL,
				rc.left,
				rc.top,
				rc.right - rc.left,
				rc.bottom - rc.top,
				SWP_NOZORDER);

			// ウィンドウの状態復元
			ShowWindow(pm->WinList[i], pm->WinPrevRect[i].showCmd);

		} else {
			ShowWindow(pm->WinList[i], stat);
		}
	}
}

void WINAPI OpenHelp(UINT Command, DWORD Data, char *UILanguageFile)
{
	char HomeDir[MAX_PATH];
	char Temp[MAX_PATH];
	HWND HWin;
	TCHAR HelpFN[MAX_PATH];
	TCHAR uimsg[MAX_UIMSG];
	TCHAR dllName[MAX_PATH];
	const TCHAR *HomeDirT;
	const TCHAR *errorFile;
	TCHAR buf[MAX_PATH];

	/* Get home directory */
	if (GetModuleFileNameA(NULL,Temp,_countof(Temp)) == 0) {
		return;
	}
	ExtractDirName(Temp, HomeDir);
	HomeDirT = ToTcharA(HomeDir);

	get_lang_msgT("HELPFILE", uimsg, _countof(uimsg),
				  _T("teraterm.chm"), UILanguageFile);

	if (pHtmlHelp == NULL) {
		HINSTANCE hDll;
		GetSystemDirectory(dllName, _countof(dllName));
		_tcscat_s(dllName, _countof(dllName), _T("\\hhctrl.ocx"));
		hDll = LoadLibrary(dllName);
		if (hDll == NULL) {
			errorFile = dllName;
			goto error;
		}
		pHtmlHelp = (void *)GetProcAddress(hDll, HTMLHELP_API_NAME);
		if (pHtmlHelp == NULL) {
			errorFile = dllName;
			goto error;
		}
	}
	// ヘルプのオーナーは常にデスクトップになる (2007.5.12 maya)
	HWin = GetDesktopWindow();
	_sntprintf_s(HelpFN, _countof(HelpFN), _TRUNCATE, _T("%s\\%s"), (TCHAR *)HomeDirT, uimsg);
	if (pHtmlHelp != NULL && pHtmlHelp(HWin, HelpFN, Command, Data) == NULL && Command != HH_CLOSE_ALL) {
		errorFile = HelpFN;
		goto error;
	}
	goto finish;

error:
	get_lang_msgT("MSG_OPENHELP_ERROR", uimsg, _countof(uimsg),
				  _T("Can't open HTML help file(%s)."), UILanguageFile);
	_sntprintf_s(buf, _countof(buf), _TRUNCATE, uimsg, HelpFN);
	MessageBox(HWin, buf, _T("Tera Term: HTML help"), MB_OK | MB_ICONERROR);

finish:
	free((void *)HomeDirT);
}

HWND WINAPI GetNthWin(int n)
{
	if (n<pm->NWin) {
		return pm->WinList[n];
	}
	else {
		return NULL;
	}
}

int WINAPI GetRegisteredWindowCount()
{
	return (pm->NWin);
}

// 有効なウィンドウを探し、現在位置を記憶させておく。
static void get_valid_window_and_memorize_rect(HWND myhwnd, HWND hwnd[], int *num, int style)
{
	int i, n;
	WINDOWPLACEMENT wndPlace;

	// 元に戻す(Undo)メニューは一度だけ表示させる。
	if (pm->WinUndoFlag == FALSE) {
		pm->WinUndoFlag = TRUE;
	} else {
		// すでにメニューが表示されていて、かつ以前と同じスタイルが選択されたら、
		// メニューを消す。
		// Windows8では、さらに連続して同じスタイルを選択してもメニューが消えたままだが、
		// Tera Termではメニューが表示されるため、動作が異なる。
		if (pm->WinUndoStyle == style)
			pm->WinUndoFlag = FALSE;
	}
	pm->WinUndoStyle = style;

	n = 0;
	for (i = 0 ; i < pm->NWin ; i++) {
		// 現在位置を覚えておく。
		wndPlace.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(pm->WinList[i], &wndPlace);
		pm->WinPrevRect[i] = wndPlace;

		// 自分自身は先頭にする。
		if (pm->WinList[i] == myhwnd) {
			hwnd[n] = hwnd[0];
			hwnd[0] = myhwnd;
		} else {
			hwnd[n] = pm->WinList[i];
		}
		n++;
	}
	*num = n;
}

// ウィンドウを左右に並べて表示する(Show Windows Side by Side)
void WINAPI ShowAllWinSidebySide(HWND myhwnd)
{
	int n;
	HWND hwnd[MAXNWIN];

	get_valid_window_and_memorize_rect(myhwnd, hwnd, &n, WIN_SIDEBYSIDE);
	TileWindows(NULL, MDITILE_VERTICAL, NULL, n, hwnd);
}

// ウィンドウを上下に並べて表示する(Show Windows Stacked)
void WINAPI ShowAllWinStacked(HWND myhwnd)
{
	int n;
	HWND hwnd[MAXNWIN];

	get_valid_window_and_memorize_rect(myhwnd, hwnd, &n, WIN_STACKED);
	TileWindows(NULL, MDITILE_HORIZONTAL, NULL, n, hwnd);
}

// ウィンドウを重ねて表示する(Cascade)
void WINAPI ShowAllWinCascade(HWND myhwnd)
{
	int n;
	HWND hwnd[MAXNWIN];

	get_valid_window_and_memorize_rect(myhwnd, hwnd, &n, WIN_CASCADE);
	CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, n, hwnd);
}

// 全Tera Termに終了指示を出す。
void WINAPI BroadcastClosingMessage(HWND myhwnd)
{
	int i, max;
	HWND hwnd[MAXNWIN];

	// Tera Termを終了させると、共有メモリが変化するため、
	// いったんバッファにコピーしておく。
	max = pm->NWin;
	for (i = 0 ; i < pm->NWin ; i++) {
		hwnd[i] = pm->WinList[i];
	}

	for (i = 0 ; i < max ; i++) {
		// 自分自身は最後にする。
		if (hwnd[i] == myhwnd)
			continue;

		PostMessage(hwnd[i], WM_USER_NONCONFIRM_CLOSE, 0, 0);
	}
	PostMessage(myhwnd, WM_USER_NONCONFIRM_CLOSE, 0, 0);
}


int WINAPI CommReadRawByte(PComVar cv, LPBYTE b)
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

void WINAPI CommInsert1Byte(PComVar cv, BYTE b)
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

int WINAPI CommRead1Byte(PComVar cv, LPBYTE b)
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

int WINAPI CommRawOut(PComVar cv, PCHAR B, int C)
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

int WINAPI CommBinaryOut(PComVar cv, PCHAR B, int C)
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

		if ( cv->TelFlag && (B[i]=='\x0d') && ! cv->TelBinSend ) {
			d[Len++] = '\x00';
		}
		else if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len++] = '\xff';
		}

		if ( OutBuffSize - cv->OutBuffCount - Len >= 0 ) {
			CommRawOut(cv, d, Len);
			a = 1;
		}
		else {
			a = 0;
		}

		i += a;
	}
	return i;
}

int WINAPI CommBinaryBuffOut(PComVar cv, PCHAR B, int C)
{
	int a, i, Len, OutLen;
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

		if (B[i] == CR) {
			if ( cv->TelFlag && ! cv->TelBinSend ) {
				d[Len++] = '\x00';
			}
			if (cv->TelLineMode) {
				cv->Flush = TRUE;
			}
		}
		else if ( cv->TelFlag && (B[i]=='\xff') ) {
			d[Len++] = '\xff';
		}

		if (cv->TelLineMode) {
			if (OutBuffSize - cv->LineModeBuffCount - Len >= 0) {
				memcpy(&(cv->LineModeBuff[cv->LineModeBuffCount]), d, Len);
				cv->LineModeBuffCount += Len;
				if (cv->Flush) {
					cv->FlushLen = cv->LineModeBuffCount;
				}
				a = 1;
			}
			else {
				a = 0;
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
			if ( OutBuffSize - cv->OutBuffCount - Len >= 0 ) {
				CommRawOut(cv, d, Len);
				a = 1;
			}
			else {
				a = 0;
			}
		}

		i += a;
	}
	return i;
}

// 内部コード(CodePage)をUTF-8へ出力する
static int OutputTextUTF8(WORD K, char *TempStr, PComVar cv)
{
	int CodePage = *cv->CodePage;
	unsigned int code;
	int outlen;

	code = MBCP_UTF32(K, CodePage);
	if (code == 0) {
		// 変換失敗
		code = 0xfffd; // U+FFFD: Replacement Character
	}
	outlen = UTF32ToUTF8(code, TempStr, 4);
	return outlen;
}

//
// MBCSから各種漢字コードへ変換して出力する。
//
static int TextOutMBCS(PComVar cv, PCHAR B, int C)
{
	int i, TempLen, OutLen;
	WORD K;
	char TempStr[12];
	int SendCodeNew;
	BYTE d;
	BOOL Full, KanjiFlagNew;

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
		else if (_isleadbyte_l(d, cv->locale)) {
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
				else if (cv->CRSend == IdLF) {
					TempStr[TempLen-1] = 0x0a;
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

	return i;
}

int WINAPI CommTextOut(PComVar cv, PCHAR B, int C)
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
			else if (cv->CRSend == IdLF) {
				TempStr[TempLen-1] = 0x0a;
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
			break;

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
				i++;
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

// TODO: UTF-16から直接変換して出力する
int WINAPI CommTextOutW(PComVar cv, const wchar_t *B, int C)
{
	int CodePage = *cv->CodePage;
	size_t mb_len;
	int r;
	char *mb_str = _WideCharToMultiByte(B, C, CodePage, &mb_len);
	if (mb_str == NULL) {
		r = 0;
	} else {
		r = CommTextOut(cv, mb_str, mb_len);
		free(mb_str);
	}
	return r;
}

// TODO: UTF-16から直接変換して出力する
int WINAPI CommTextEchoW(PComVar cv, const wchar_t *B, int C)
{
	int CodePage = *cv->CodePage;
	size_t mb_len;
	int r;
	char *mb_str = _WideCharToMultiByte(B, C, CodePage, &mb_len);
	if (mb_str == NULL) {
		r = 0;
	} else {
		r = CommTextEcho(cv, mb_str, mb_len);
		free(mb_str);
	}
	return r;
}

int WINAPI CommBinaryEcho(PComVar cv, PCHAR B, int C)
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

static int WINAPI TextEchoMBCS(PComVar cv, PCHAR B, int C)
{
	int i, TempLen;
	WORD K;
	char TempStr[12];
	int EchoCodeNew;
	BYTE d;
	BOOL Full, KanjiFlagNew;

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
		else if (_isleadbyte_l(d, cv->locale)) {
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
				else if (cv->CRSend == IdLF) {
					TempStr[TempLen-1] = 0x0a;
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

	return i;
}

int WINAPI CommTextEcho(PComVar cv, PCHAR B, int C)
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
			else if (cv->CRSend == IdLF) {
				TempStr[TempLen-1] = 0x0a;
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


int WINAPI DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc)
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
			char buf[12]; // \\.\COMxxxx + NULL
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

int WINAPI CheckComPort(WORD ComPort)
{
	HMODULE h;
	TCHAR   devicesBuff[65535];
	char    com_str[64];
	BOOL bRet;
	GUID ClassGuid[1];
	DWORD dwRequiredSize;
	HDEVINFO DeviceInfoSet = NULL;
	SP_DEVINFO_DATA DeviceInfoData;
	int found = 0;

	_snprintf_s(com_str, sizeof(com_str), _TRUNCATE, "COM%d", ComPort);

	if (((h = GetModuleHandle("kernel32.dll")) == NULL) | (GetProcAddress(h, "QueryDosDeviceA") == NULL) ) {
		/* ERROR */
		return -1;
	}

	if (QueryDosDevice(com_str, devicesBuff, 65535) == 0) {
		DWORD err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND) {
			/* NOT FOUND */
			return 0;
		}
		/* ERROR */
		return -1;
	}

	/* QueryDosDeviceで切断を検知できない環境があるでさらにチェック */
	bRet = SetupDiClassGuidsFromName(_T("PORTS"), (LPGUID) & ClassGuid, 1, &dwRequiredSize);
	if (bRet == FALSE) {
		return -1;
	}

	DeviceInfoSet = SetupDiGetClassDevs(&ClassGuid[0], NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
	if (DeviceInfoSet == NULL) {
		return -1;
	}

	if (DeviceInfoSet) {
		DWORD dwMemberIndex = 0;
		HKEY hKey = NULL;
		TCHAR szPortName[MAX_PATH];
		DWORD dwReqSize;
		DWORD dwType;

		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		while (SetupDiEnumDeviceInfo(DeviceInfoSet, dwMemberIndex, &DeviceInfoData)) {
			hKey = SetupDiOpenDevRegKey(DeviceInfoSet, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (hKey) {
				long lRet;
				dwReqSize = sizeof(szPortName);
				lRet = RegQueryValueEx(hKey, _T("PortName"), 0, &dwType, (LPBYTE)& szPortName, &dwReqSize);
				RegCloseKey(hKey);
				if (_stricmp(szPortName, com_str) == 0) {
					found = TRUE;
					break;
				}
			}
			dwMemberIndex++;
		}
	}

	SetupDiDestroyDeviceInfoList(DeviceInfoSet);

	return found;
}

// Notify Icon 関連
static NOTIFYICONDATA notify_icon = {0};
static int NotifyIconShowCount = 0;
static HICON CustomIcon = NULL;

void WINAPI SetCustomNotifyIcon(HICON icon)
{
	CustomIcon = icon;
}

HICON WINAPI GetCustomNotifyIcon()
{
	return CustomIcon;
}

void WINAPI CreateNotifyIcon(PComVar cv)
{
	if (cv->NotifyIcon == NULL) {
		notify_icon.cbSize = sizeof(notify_icon);
		notify_icon.hWnd = cv->HWin;
		notify_icon.uID = 1;
		notify_icon.uFlags = NIF_ICON | NIF_MESSAGE;
		notify_icon.uCallbackMessage = WM_USER_NOTIFYICON;
		if (CustomIcon) {
			notify_icon.hIcon = CustomIcon;
		}
		else {
			notify_icon.hIcon = (HICON)SendMessage(cv->HWin, WM_GETICON, ICON_SMALL, 0);
		}
		notify_icon.szTip[0] = '\0';
		notify_icon.dwState = 0;
		notify_icon.dwStateMask = 0;
		notify_icon.szInfo[0] = '\0';
		notify_icon.uTimeout = 0;
		notify_icon.szInfoTitle[0] = '\0';
		notify_icon.dwInfoFlags = 0;

		cv->NotifyIcon = &notify_icon;

		Shell_NotifyIcon(NIM_ADD, cv->NotifyIcon);

		NotifyIconShowCount = 0;
	}

	return;
}

void WINAPI DeleteNotifyIcon(PComVar cv)
{
	if (cv->NotifyIcon) {
		Shell_NotifyIcon(NIM_DELETE, cv->NotifyIcon);
		cv->NotifyIcon = NULL;
		NotifyIconShowCount = 0;
	}

	return;
}

void WINAPI ShowNotifyIcon(PComVar cv)
{
	if (cv->NotifyIcon == NULL) {
		CreateNotifyIcon(cv);
	}

	cv->NotifyIcon->uFlags = NIF_STATE;
	cv->NotifyIcon->dwState = 0;
	cv->NotifyIcon->dwStateMask = NIS_HIDDEN;
	Shell_NotifyIcon(NIM_MODIFY, cv->NotifyIcon);
	NotifyIconShowCount += 1;

	return;
}

void WINAPI HideNotifyIcon(PComVar cv)
{
	if (NotifyIconShowCount > 1) {
		NotifyIconShowCount -= 1;
	}
	else {
		if (cv->NotifyIcon) {
			cv->NotifyIcon->uFlags = NIF_STATE;
			cv->NotifyIcon->dwState = NIS_HIDDEN;
			cv->NotifyIcon->dwStateMask = NIS_HIDDEN;
			Shell_NotifyIcon(NIM_MODIFY, cv->NotifyIcon);
		}
		NotifyIconShowCount = 0;
	}

	return;
}

void WINAPI SetVerNotifyIcon(PComVar cv, unsigned int ver)
{
	if (cv->NotifyIcon) {
		cv->NotifyIcon->uVersion = ver;
		Shell_NotifyIcon(NIM_SETVERSION, cv->NotifyIcon);
	}
	return;
}

void WINAPI NotifyMessage(PComVar cv, char *msg, char *title, DWORD flag)
{
	if (msg == NULL) {
		return;
	}

	if (! HasBalloonTipSupport()) {
		return;
	}

	if (cv->NotifyIcon == NULL) {
		CreateNotifyIcon(cv);
	}

	cv->NotifyIcon->uFlags = NIF_INFO | NIF_STATE;
	cv->NotifyIcon->dwState = 0;
	cv->NotifyIcon->dwStateMask = NIS_HIDDEN;

	if (title) {
		cv->NotifyIcon->dwInfoFlags = flag;
		strncpy_s(cv->NotifyIcon->szInfoTitle, sizeof(cv->NotifyIcon->szInfoTitle), title, _TRUNCATE);
	}
	else {
		cv->NotifyIcon->dwInfoFlags = NIIF_NONE;
		cv->NotifyIcon->szInfoTitle[0] = 0;
	}

	strncpy_s(cv->NotifyIcon->szInfo, sizeof(cv->NotifyIcon->szInfo), msg, _TRUNCATE);

	Shell_NotifyIcon(NIM_MODIFY, cv->NotifyIcon);

	NotifyIconShowCount += 1;

	return;
}

/*
 *	@return		エラーが有る場合 FALSE
 *	@param[in]	BOOL first_instance
 */
static BOOL OpenSharedMemory(BOOL *first_instance_)
{
	int i;
	HMap = NULL;
	pm = NULL;
	for (i = 0; i < 100; i++) {
		char tmp[32];
		HANDLE hMap;
		BOOL first_instance;
		TMap *map;
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, i == 0 ? "%s" : "%s_%d", TT_FILEMAPNAME, i);
		hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
								 0, sizeof(TMap), tmp);
		if (hMap == NULL) {
			return FALSE;
		}

		first_instance = (GetLastError() != ERROR_ALREADY_EXISTS);

		map = (TMap *)MapViewOfFile(hMap,FILE_MAP_WRITE,0,0,0);
		if (map == NULL) {
			return FALSE;
		}

		if (first_instance ||
			(map->size_tmap == sizeof(TMap) &&
			 map->size_tttset == sizeof(TTTSet)))
		{
			map->size_tmap = sizeof(TMap);
			map->size_tttset = sizeof(TTTSet);
			HMap = hMap;
			pm = map;
			*first_instance_ = first_instance;
			return TRUE;
		}

		// next try
		UnmapViewOfFile(map);
		CloseHandle(hMap);
	}
	return FALSE;
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
			if (OpenSharedMemory(&FirstInstance) == FALSE) {
				// dllロード失敗、teratermが起動しない
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
