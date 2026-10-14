/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

/* TERATERM.EXE, variables, flags related to VT win and TEK win */

#include "teraterm.h"
#include "tttypes.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "ttlib.h"
#include "helpid.h"
#include "i18n.h"
#include "commlib.h"
#include "codeconv.h"
#include "ttwinman.h"
#include "asprintf.h"
#include "session.h"

HWND HVTWin = NULL;
HWND HTEKWin = NULL;

int ActiveWin = IdVT; /* IdVT, IdTEK */
IdTalk TalkStatus = IdTalkKeyb; /* IdTalkKeyb, IdTalkCB, IdTalkTextFile */
BOOL KeybEnabled = TRUE; /* keyboard switch */
BOOL Connecting = FALSE;

/* 'help' button on dialog box */
WORD MsgDlgHelp;

/* settings and connection of the active session (see session.c) */
static TTTSet first_ts;
static TComVar first_cv;
TTTSet *g_ts = &first_ts;
TComVar *g_cv = &first_cv;

/* pointers to window objects */
void* pTEKWin = NULL;
/* instance handle */
HINSTANCE hInst;

int SerialNo;

void VTActivate()
{
  ActiveWin = IdVT;
  ShowWindow(HVTWin, SW_SHOWNORMAL);
  SetFocus(HVTWin);
}

// キャプションの変更
//
// (2005.2.19 yutaka) format ID=13の新規追加、COM5以上の表示に対応
// (2005.3.13 yutaka) タイトルのSJISへの変換（日本語）を追加
// (2006.6.15 maya)   g_ts->KanjiCodeがEUCだと、SJISでもEUCとして
//                    変換してしまうので、ここでは変換しない
// (2007.7.19 maya)   TCP ポート番号 と シリアルポートのボーレートの表示に対応
/*
 *  TitleFormat
 *    0 0 0 0 0 0 (2)
 *    | | | | | +----- displays TCP host/serial port
 *    | | | | +------- displays session no
 *    | | | +--------- displays VT/TEK
 *    | | +----------- displays TCP host/serial port first
 *    | +------------- displays TCP port number
 *    +--------------- displays speed of serial port
 */
void ChangeTitle(void)
{
#define HostNameMaxLength 1024	// ホスト名最大長  TODO 別の最大長基準 or 最大長制限をなくす
	wchar_t TempTitle[HostNameMaxLength + TitleBuffSize * 2 + 1]; // バッファ拡張
	wchar_t TempTitleWithRemote[TitleBuffSize * 2];

	{
		const wchar_t *title_remote;
		const wchar_t *title = (g_ts->TitleW != NULL) ? g_ts->TitleW : L"";
		title_remote = g_cv->TitleRemoteW;
		if (Connecting || !g_cv->Ready || title_remote == NULL || title_remote[0] == 0) {
			// リモートタイトルを使用しない or 設定されていない
			wcsncpy_s(TempTitleWithRemote, _countof(TempTitleWithRemote), title, _TRUNCATE);
			wcsncpy_s(TempTitle, _countof(TempTitle), title, _TRUNCATE);
		}
		else {
			// リモートからのタイトルを別に扱う (2008.11.1 maya)
			if (g_ts->AcceptTitleChangeRequest == IdTitleChangeRequestOff) {
				wcsncpy_s(TempTitleWithRemote, _countof(TempTitleWithRemote), title, _TRUNCATE);
			}
			else if (g_ts->AcceptTitleChangeRequest == IdTitleChangeRequestAhead) {
				_snwprintf_s(TempTitleWithRemote, _countof(TempTitleWithRemote), _TRUNCATE,
							 L"%s %s", title_remote, title);
			}
			else if (g_ts->AcceptTitleChangeRequest == IdTitleChangeRequestLast) {
				_snwprintf_s(TempTitleWithRemote, _countof(TempTitleWithRemote), _TRUNCATE,
							 L"%s %s", title, title_remote);
			}
			else {
				wcsncpy_s(TempTitleWithRemote, _countof(TempTitleWithRemote), title_remote, _TRUNCATE);
			}
			wcsncpy_s(TempTitle, _countof(TempTitle), TempTitleWithRemote, _TRUNCATE);
		}
	}

	if ((g_ts->TitleFormat & 1)!=0)
	{ // host name
		wchar_t *UIMsg;
		wcsncat_s(TempTitle,_countof(TempTitle), L" - ",_TRUNCATE);
		if (Connecting) {
			GetI18nStrWW("Tera Term", "DLG_MAIN_TITLE_CONNECTING", L"[connecting...]", g_ts->UILanguageFileW, &UIMsg);
			wcsncat_s(TempTitle,_countof(TempTitle), UIMsg,_TRUNCATE);
			free(UIMsg);
		}
		else if (! g_cv->Ready) {
			GetI18nStrWW("Tera Term", "DLG_MAIN_TITLE_DISCONNECTED", L"[disconnected]", g_ts->UILanguageFileW, &UIMsg);
			wcsncat_s(TempTitle,_countof(TempTitle), UIMsg,_TRUNCATE);
			free(UIMsg);
		}
		else if (g_cv->PortType==IdSerial)
		{
			// COM5 overに対応
			wchar_t str[24]; // COMxxxx:xxxxxxxxxxbps
			if (g_ts->TitleFormat & 32) {
				_snwprintf_s(str, _countof(str), _TRUNCATE, L"COM%d:%ubps", g_ts->ComPort, g_ts->Baud);
			}
			else {
				_snwprintf_s(str, _countof(str), _TRUNCATE, L"COM%d", g_ts->ComPort);
			}

			if (g_ts->TitleFormat & 8) {
				_snwprintf_s(TempTitle, _countof(TempTitle), _TRUNCATE, L"%s - %s", str, TempTitleWithRemote);
			} else {
				wcsncat_s(TempTitle, _countof(TempTitle), str, _TRUNCATE);
			}
		}
		else if (g_cv->PortType == IdNamedPipe)
		{
			const wchar_t *str = GetConnectHostName();

			if (g_ts->TitleFormat & 8) {
				// format ID = 13(8 + 5): <hots/port> - <title>
				_snwprintf_s(TempTitle, _countof(TempTitle), _TRUNCATE, L"%s - %s", str, TempTitleWithRemote);
			}
			else {
				wcsncat_s(TempTitle, _countof(TempTitle), str, _TRUNCATE);
			}
		}
		else {
			wchar_t host_port[_countof(TempTitle)];
			const wchar_t *str = GetConnectHostName();
			if (g_ts->TitleFormat & 16) {
				_snwprintf_s(host_port, _countof(host_port), _TRUNCATE, L"%s:%d", str, g_ts->TCPPort);
				str = host_port;
			}

			if (g_ts->TitleFormat & 8) {
				// format ID = 13(8 + 5): <hots/port> - <title>
				_snwprintf_s(TempTitle, _countof(TempTitle), _TRUNCATE, L"%s - %s", str, TempTitleWithRemote);
			}
			else {
				wcsncat_s(TempTitle, _countof(TempTitle), str, _TRUNCATE);
			}
		}
	}

	if ((g_ts->TitleFormat & 2)!=0)
	{ // serial no.
		wchar_t Num[11];
		wcsncat_s(TempTitle,_countof(TempTitle),L" (",_TRUNCATE);
		_snwprintf_s(Num,_countof(Num),_TRUNCATE,L"%u",SerialNo);
		wcsncat_s(TempTitle,_countof(TempTitle),Num,_TRUNCATE);
		wcsncat_s(TempTitle,_countof(TempTitle),L")",_TRUNCATE);
	}

	if ((g_ts->TitleFormat & 4)!=0) // VT
		wcsncat_s(TempTitle,_countof(TempTitle),L" VT",_TRUNCATE);

	SetWindowTextW(HVTWin,TempTitle);

	if (HTEKWin!=0)
	{
		if ((g_ts->TitleFormat & 4)!=0) // TEK
		{
			wcsncat_s(TempTitle,_countof(TempTitle),L" TEK",_TRUNCATE);
		}
		SetWindowTextW(HTEKWin,TempTitle);
	}
}

void SwitchMenu()
{
  HWND H1, H2;

  if (ActiveWin==IdVT)
  {
    H1 = HTEKWin;
    H2 = HVTWin;
  }
  else {
    H1 = HVTWin;
    H2 = HTEKWin;
  }

  if (H1!=0)
    PostMessage(H1,WM_USER_CHANGEMENU,0,0);
  if (H2!=0)
    PostMessage(H2,WM_USER_CHANGEMENU,0,0);
}

void SwitchTitleBar()
{
  HWND H1, H2;

  if (ActiveWin==IdVT)
  {
    H1 = HTEKWin;
    H2 = HVTWin;
  }
  else {
    H1 = HVTWin;
    H2 = HTEKWin;
  }

  if (H1!=0)
    PostMessage(H1,WM_USER_CHANGETBAR,0,0);
  if (H2!=0)
    PostMessage(H2,WM_USER_CHANGETBAR,0,0);
}

HMODULE LoadHomeDLL(const wchar_t *DLLname)
{
	HMODULE handle;
	wchar_t *DLLpath;
	aswprintf(&DLLpath, L"%s\\%s", g_ts->ExeDirW, DLLname);
	handle = LoadLibraryW(DLLpath);
	free(DLLpath);
	return handle;
}

/**
 *	ローカルタイトルを設定する
 *
 *	@param	title	タイトル文字列
 *					g_ts->TitleW を指定しない
 */
void SetLocalTitle(const wchar_t *title)
{
	if (title == g_ts->TitleW) {
		return;
	}
	free(g_ts->TitleW);
	g_ts->TitleW = _wcsdup(title != NULL ? title : L"");
	ChangeTitle();
}

/**
 *	ローカルタイトルを取得する
 *
 *	@return	タイトル文字列(NULLを返さない)
 *			次に SetLocalTitle() が呼ばれると無効になる
 */
const wchar_t *GetLocalTitle(void)
{
	return (g_ts->TitleW != NULL) ? g_ts->TitleW : L"";
}

/**
 *	接続先ホスト名を設定する
 *
 *	g_ts->HostNameW と g_ts->HostName(ANSI) の両方を設定する
 *
 *	@param	hostname	ホスト名
 *						NULL のときは空にする
 *						g_ts->HostNameW を指定しない
 */
void SetConnectHostName(const wchar_t *hostname)
{
	if (hostname == g_ts->HostNameW) {
		return;
	}
	free(g_ts->HostNameW);
	g_ts->HostNameW = _wcsdup(hostname != NULL ? hostname : L"");
	WideCharToACP_t(g_ts->HostNameW, g_ts->HostName, _countof(g_ts->HostName));
}

/**
 *	接続先ホスト名を取得する
 *
 *	g_ts->HostName(ANSI) だけを書き換えるプラグイン(TTProxy 1.0.0.27 以前など)が
 *	あるため、g_ts->HostName が g_ts->HostNameW と食い違っているときは
 *	g_ts->HostName から g_ts->HostNameW を作り直して返す
 *
 *	@return	ホスト名(NULLを返さない)
 *			次に SetConnectHostName() または GetConnectHostName() が
 *			呼ばれると無効になる
 */
const wchar_t *GetConnectHostName(void)
{
	if (g_ts->HostNameW != NULL) {
		char hostnameA[_countof(g_ts->HostName)];
		WideCharToACP_t(g_ts->HostNameW, hostnameA, _countof(hostnameA));
		if (strcmp(hostnameA, g_ts->HostName) == 0) {
			return g_ts->HostNameW;
		}
	}
	// g_ts->HostNameW が未設定、または g_ts->HostName だけが書き換えられている
	free(g_ts->HostNameW);
	g_ts->HostNameW = ToWcharA(g_ts->HostName);
	return (g_ts->HostNameW != NULL) ? g_ts->HostNameW : L"";
}

/**
 *	Variables kept per session (see session.h)
 */
void WinManRegisterSessionVars(void)
{
	static const SessionVar v[] = {
		SESSION_VAR(ActiveWin),
		SESSION_VAR(Connecting),
		SESSION_VAR(HTEKWin),
		SESSION_VAR(HVTWin),
		SESSION_VAR(KeybEnabled),
		SESSION_VAR(SerialNo),
		SESSION_VAR(TalkStatus),
		SESSION_VAR(pTEKWin),
	};
	SessionRegisterVars(v, _countof(v));
}
