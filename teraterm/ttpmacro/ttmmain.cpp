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

/* TTMACRO.EXE, main */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <assert.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

#include "compat_win.h"
#include "teraterm.h"
#include "ttm_res.h"
#include "ttmdlg.h"
#include "ttl.h"
#include "ttmparse.h"
#include "ttmdde.h"
#include "ttmmain.h"
#include "ttmbuff.h"
#include "ttmlib.h"
#include "dlglib.h"
#include "ttlib.h"
#include "wait4all.h"
#include "tmfc.h"
#include "ttmacro.h"
#include "codeconv.h"

static void ClientToScreen(HWND hWnd, RECT *rect)
{
	POINT pos;
	pos.x = rect->left;
	pos.y = rect->top;
	::ClientToScreen(hWnd, &pos);
	rect->left = pos.x;
	rect->top = pos.y;

	pos.x = rect->right;
	pos.y = rect->bottom;
	::ClientToScreen(hWnd, &pos);
	rect->right = pos.x;
	rect->bottom = pos.y;
}

static void ScreenToClient(HWND hWnd, RECT *rect)
{
	POINT pos;
	pos.x = rect->left;
	pos.y = rect->top;
	::ScreenToClient(hWnd, &pos);
	rect->left = pos.x;
	rect->top = pos.y;

	pos.x = rect->right;
	pos.y = rect->bottom;
	::ScreenToClient(hWnd, &pos);
	rect->right = pos.x;
	rect->bottom = pos.y;
}

// CCtrlWindow dialog
CCtrlWindow::CCtrlWindow(HINSTANCE hInst)
{
	m_hInst = hInst;
	m_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TTMACRO));
}

BOOL CCtrlWindow::Create()
{
	if (! TTCDialog::Create(m_hInst, NULL, CCtrlWindow::IDD)) {
		PostQuitMessage(0);
		return FALSE;
	}
	return TRUE;
}

// TTMACRO main engine
BOOL CCtrlWindow::OnIdle()
{
	int ResultCode;
	char Temp[2];

	if (TTLStatus==IdTTLEnd) {
		::DestroyWindow(m_hStatus);
		DestroyWindow();
		return FALSE;
	}

	SendSync(); // for sync mode

	if (OutLen>0) {
		DDESend();

		// Windows2000/XP（シングルプロセッサ環境）においてマクロ実行中にCPU使用率が100%になってしまう
		// 現象への暫定処置。ttermpro.exeへのDDE送信後に無条件に100ミリ秒のスリープを追加。
		// (2005.5.25 yutaka)
		// 以下の暫定処置は解除。
		// (2006.10.13 yutaka)
		// 以下の暫定処置は復活。Tera Term本体側のOnIdle()において、XTYP_ADVREQが頻繁に飛ぶことが
		// CPUをストールさせてしまう要因と思われるのだが、根本的原因はいまだ不明。
		// (2006.11.4 yutaka)
		// 以下の暫定処置は DDESend() で行うため、削除。
		// (2006.11.6 yutaka)
#if 0
		Sleep(100);
#endif
		return TRUE;
	}
	else if (! Pause && (TTLStatus==IdTTLRun)) {
		Exec();

		// 更新対象のマクロコマンドの場合のみ、ウィンドウに更新指示を出す。
		// 毎度 WM_PAINT を送っているとマクロの動作が遅くなるため。(2006.2.24 yutaka)
		if (IsUpdateMacroCommand()) {
			::InvalidateRect(m_hWnd, NULL, TRUE);
		}
		return TRUE;
	}
	else if (TTLStatus==IdTTLWait) {
		ResultCode = Wait();
		if (ResultCode>0) {
			::KillTimer(m_hWnd, IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetResult(ResultCode);
			UnlockVar();
			ClearWait();
			return TRUE;
		}
		else if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWaitLn) {
		ResultCode = Wait();
		if (ResultCode>0) {
			LockVar();
			SetResult(ResultCode);
			UnlockVar();
			Temp[0] = 0x0a;
			Temp[1] = 0;
			if (CmpWait(ResultCode,Temp)==0) { // new-line is received
				::KillTimer(m_hWnd, IdTimeOutTimer);
				ClearWait();
				TTLStatus = IdTTLRun;
				LockVar();
				SetInputStr(GetRecvLnBuff());
				UnlockVar();
			}
			else { // wait new-line
				ClearWait();
				SetWait(1,Temp);
				TTLStatus = IdTTLWaitNL;
			}
			return TRUE;
		}
		else if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWaitNL) {
		ResultCode = Wait();
		if (ResultCode>0) {
			::KillTimer(m_hWnd, IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetInputStr(GetRecvLnBuff());
			UnlockVar();
			return TRUE;
		}
		else if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWait2) {
		if (Wait2()) {
			::KillTimer(m_hWnd, IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetInputStr(Wait2Str);
			SetResult(1);
			UnlockVar();
			return TRUE;
		}
		else if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWaitN) {
		if (WaitN()) {
			::KillTimer(m_hWnd, IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetResult(1);
			SetInputStr(GetRecvLnBuff());
			UnlockVar();
			ClearWaitN();
			return TRUE;
		}
		else if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLWait4all) {
		ResultCode = Wait4all();
		if (ResultCode>0) {
			::KillTimer(m_hWnd, IdTimeOutTimer);
			TTLStatus = IdTTLRun;
			LockVar();
			SetResult(ResultCode);
			UnlockVar();
			ClearWait();
			return TRUE;
		}
		else if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}

	return FALSE;
}

static void FitTTLFileName()
{
	size_t dirlen, fnpos;
	if (!GetFileNamePosW(FileName, &dirlen, &fnpos)) {
		FileName[0] = 0;
		ShortName[0] = 0;
		return;
	}

	FitFileNameW(&FileName[fnpos], _countof(FileName) - fnpos, L".TTL");
	wcsncpy_s(ShortName, _countof(ShortName), &FileName[fnpos], _TRUNCATE);
}

// CCtrlWindow message handler

BOOL CCtrlWindow::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_CTRLPAUSESTART, "BTN_PAUSE" },
		{ IDC_CTRLEND, "BTN_END" },
	};
	RECT rc_dlg, rc_filename, rc_lineno;
	LONG dlg_len, len;

	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFileW);

	Pause = FALSE;

	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), 0);

	ParseParam();

	if (FileName[0] == 0 || FileName[0] == '*') {
		// TTLファイル指定がない(or "*")のときダイアログから入力してもらう
		wchar_t *ttl;
		FileName[0] = 0;
		BOOL r = GetFileName(GetSafeHwnd(), &ttl);
		if (r == TRUE) {
			wcsncpy_s(FileName, _countof(FileName), ttl, _TRUNCATE);
			free(ttl);
		}
	}

	FitTTLFileName();
	Params[1] = _wcsdup(ShortName);

	if (FileName[0] == 0) {
		// TTL ファイル指定なし
		PostQuitMessage(0);
		return TRUE;
	}

	if (TopicName[0] != 0) {
		InitDDE(GetSafeHwnd());
	}

	if (! InitTTL(GetSafeHwnd())) {
		EndDDE();
		PostQuitMessage(0);
	}

	// wait4all
	register_macro_window(GetSafeHwnd());

	wchar_t Temp[MAX_PATH + 8]; // MAX_PATH + "MACRO - "(8)
	wcsncpy_s(Temp, _countof(Temp), L"MACRO - ", _TRUNCATE);
	wcsncat_s(Temp, _countof(Temp), ShortName, _TRUNCATE);
	SetWindowTextW(Temp);

	// send the initialization signal to TT
	SendCmnd(CmdInit,0);

	// ダイアログの初期サイズを保存
	GetWindowRect(&rc_dlg);
	m_init_width = rc_dlg.right - rc_dlg.left;
	m_init_height = rc_dlg.bottom - rc_dlg.top;

	/* マクロウィンドウをリサイズ可能にする。
	 * (2015.1.2 yutaka)
	 */
	// 現在サイズから必要な値を計算
	::GetClientRect(m_hWnd, &rc_dlg);
	ClientToScreen(m_hWnd, &rc_dlg);
	dlg_len = rc_dlg.right - rc_dlg.left;

	::GetWindowRect(GetDlgItem(IDC_FILENAME), &rc_filename);
	len = rc_filename.right - rc_filename.left;
	m_filename_ratio = len*100 / dlg_len;

	::GetWindowRect(GetDlgItem(IDC_LINENO), &rc_lineno);
	len = rc_lineno.right - rc_lineno.left;
	m_lineno_ratio = len * 100 / dlg_len;

	// リサイズアイコンを右下に表示させたいので、ステータスバーを付ける。
	m_hStatus = ::CreateStatusWindow(
		WS_CHILD | WS_VISIBLE |
		CCS_BOTTOM | SBARS_SIZEGRIP, NULL, GetSafeHwnd(), 1);

	CenterWindow(m_hWnd, NULL);

	return TRUE;
}

// ダイアログ上にキャンセルボタン IDCANCEL がないので、
// ESCが押されたときだけ呼び出される
BOOL CCtrlWindow::OnCancel()
{
	// 何もせずにTRUEを返す -> ESCキーを無効化
	return TRUE;
}

BOOL CCtrlWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	wchar_t *uimsg;

	switch (LOWORD(wParam)) {
	case IDC_CTRLPAUSESTART:
		if (Pause) {
			GetI18nStrWW("Tera Term", "BTN_PAUSE", L"Pau&se", UILanguageFileW, &uimsg);
			SetDlgItemTextW(IDC_CTRLPAUSESTART, uimsg);
			free(uimsg);
		}
		else {
			GetI18nStrWW("Tera Term", "BTN_START", L"&Start", UILanguageFileW, &uimsg);
			SetDlgItemTextW(IDC_CTRLPAUSESTART, uimsg);
			free(uimsg);
		}
		Pause = ! Pause;
		return TRUE;
	case IDC_CTRLEND:
		TTLStatus = IdTTLEnd;
		PostQuitMessage(0);
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL CCtrlWindow::OnClose()
{
	DestroyWindow();
	return TRUE;
}

void CCtrlWindow::OnDestroy()
{
	unregister_macro_window(GetSafeHwnd());

	EndTTL();
	EndDDE();

	// アプリケーション終了時にアイコンを破棄すると、ウィンドウが消える前に
	// タイトルバーのアイコンが "Windows の実行ファイルのアイコン" に変わる
	// ことがあるので破棄しない
	// TTSetIcon(m_hInst, m_hWnd, NULL, 0);

	::DestroyWindow(m_hStatus);
}

// for icon drawing in Win NT 3.5
void CCtrlWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC dc;
	char buf[128];

	dc = BeginPaint(&ps);

	// line number (2005.7.18 yutaka)
	// added line buffer (2005.7.22 yutaka)
	// added MACRO filename (2013.9.8 yutaka)
	// ファイル名の末尾は省略表示とする。(2014.12.30 yutaka)
	SetDlgItemTextW(IDC_FILENAME, wc::fromUtf8(GetMacroFileName()));
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, ":%d:%s", GetLineNo(), GetLineBuffer());
	SetDlgItemTextA(IDC_LINENO, buf);

	EndPaint(&ps);
}

// マクロウィンドウをリサイズ可能とするために、OnSizeハンドラをoverrideする。
// (2015.1.1 yutaka)
void CCtrlWindow::OnSize(UINT nType, int cx, int cy)
{
	RECT rc_dlg, rc_filename, rc_lineno;
	LONG new_w, new_h, new_x, new_y;
	LONG len;

	::GetClientRect(m_hWnd, &rc_dlg);
	ClientToScreen(m_hWnd, &rc_dlg);
	len = rc_dlg.right - rc_dlg.left;

	// TTLファイル名の再配置
	HWND hWnd = GetDlgItem(IDC_FILENAME);
	::GetWindowRect(hWnd, &rc_filename);
	ScreenToClient(m_hWnd, &rc_filename);
	new_w = (len * m_filename_ratio) / 100;
	new_h = rc_filename.bottom - rc_filename.top;
	::SetWindowPos(hWnd, HWND_BOTTOM,
		0, 0, new_w, new_h,
		SWP_NOMOVE | SWP_NOZORDER
	);
	new_x = rc_filename.left + new_w;

	// 行番号の再配置
	hWnd = GetDlgItem(IDC_LINENO);
	::GetWindowRect(hWnd, &rc_lineno);
	ScreenToClient(m_hWnd, &rc_lineno);
	new_w = (len * m_lineno_ratio) / 100;
	new_h = rc_lineno.bottom - rc_lineno.top;
	new_y = rc_lineno.top;
	::SetWindowPos(hWnd, HWND_BOTTOM,
		new_x, new_y, new_w, new_h,
		SWP_NOZORDER
		);

	// status bar
	::SendMessage(m_hStatus, WM_SIZE, nType, (cy<<16)|cx );
}

// マクロウィンドウをリサイズ可能とするために、OnGetMinMaxInfoハンドラをoverrideする。
// (2015.1.1 yutaka)
void CCtrlWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// 下記の処理があると、Release build版で Tera Term から ttpmacro.exe を呼び出すと、
	// ストールするため、いったん無効化する。
#if 0
	LPMINMAXINFO lpmmi;

	// ダイアログの初期サイズより小さくできないようにする
	lpmmi = (LPMINMAXINFO)lpMMI;
	lpmmi->ptMinTrackSize.x = m_init_width;
	lpmmi->ptMinTrackSize.y = m_init_height;
#endif
}

void CCtrlWindow::OnTimer(UINT_PTR nIDEvent)
{
	BOOL TimeOut;

	::KillTimer(m_hWnd, nIDEvent);
	if (nIDEvent!=IdTimeOutTimer) {
		return;
	}
	if (TTLStatus==IdTTLRun) {
		return;
	}

	TimeOut = CheckTimeout();
	LockVar();

	if ((TTLStatus == IdTTLWait) ||
	    (TTLStatus == IdTTLWaitLn) ||
	    (TTLStatus == IdTTLWaitNL)) {
		if ((! Linked) || (ComReady==0)|| TimeOut) {
			SetResult(0);
			if (TTLStatus==IdTTLWaitNL) {
				SetInputStr(GetRecvLnBuff());
			}
			ClearWait();
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus == IdTTLWait2) {
		if ((! Linked) || (ComReady==0) || TimeOut) {
			if (Wait2Found) {
				SetInputStr(Wait2Str);
				SetResult(-1);
			}
			else {
				SetInputStr("");
				SetResult(0);
			}
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus == IdTTLWaitN) {
		if ((! Linked) || (ComReady==0) || TimeOut) {
			PCHAR p = GetRecvLnBuff();
			SetInputStr(p);
			if (strlen(p) > 0) {
				SetResult(-1);
			}
			else {
				SetResult(0);
			}
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus==IdTTLPause) {
		if (TimeOut) {
			TTLStatus = IdTTLRun;
		}
	}
	else if (TTLStatus==IdTTLSleep) {
		if ((TimeOut) && TestWakeup(IdWakeupTimeout)) {
			SetResult(IdWakeupTimeout);
			TTLStatus = IdTTLRun;
		}
	}
	else {
		TTLStatus = IdTTLRun;
	}

	UnlockVar();

	if (TimeOut || (TTLStatus==IdTTLRun)) {
		return;
	}

	::SetTimer(m_hWnd, IdTimeOutTimer, TIMEOUT_TIMER_MS, NULL);
}

BOOL CCtrlWindow::PostNcDestroy()
{
	PostQuitMessage(0);
	return TRUE;
}

LRESULT CCtrlWindow::OnDdeCmndEnd(WPARAM wParam, LPARAM lParam)
{
	if (TTLStatus == IdTTLWaitCmndResult) {
		LockVar();
		SetResult(wParam);
		UnlockVar();
	}

	if ((TTLStatus == IdTTLWaitCmndEnd) ||
		(TTLStatus == IdTTLWaitCmndResult)) {
		TTLStatus = IdTTLRun;
	}
	return 0;
}

LRESULT CCtrlWindow::OnDdeComReady(WPARAM wParam, LPARAM lParam)
{
	ComReady = wParam;
	if ((TTLStatus == IdTTLWait) ||
	    (TTLStatus == IdTTLWaitLn) ||
	    (TTLStatus == IdTTLWaitNL) ||
	    (TTLStatus == IdTTLWait2) ||
	    (TTLStatus == IdTTLWaitN)) {
		if (ComReady==0) {
			::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
		}
	}
	else if (TTLStatus==IdTTLSleep) {
		LockVar();
		if (TestWakeup(IdWakeupInit)) {
			if (ComReady!=0) {
				SetResult(2);
			}
			else {
				SetResult(1);
			}
			TTLStatus = IdTTLRun;
		}
		else if ((ComReady!=0) && TestWakeup(IdWakeupConnect)) {
			SetResult(IdWakeupConnect);
			TTLStatus = IdTTLRun;
		}
		else if ((ComReady==0) && TestWakeup(IdWakeupDisconn)) {
			SetResult(IdWakeupDisconn);
			TTLStatus = IdTTLRun;
		}
		UnlockVar();
	}
	return 0;
}

LRESULT CCtrlWindow::OnDdeReady(WPARAM wParam, LPARAM lParam)
{
	if (TTLStatus != IdTTLInitDDE) {
		return 0;
	}
	SetWakeup(IdWakeupInit);
	TTLStatus = IdTTLSleep;

	if (! InitDDE(GetSafeHwnd())) {
		LockVar();
		SetResult(0);
		UnlockVar();
		TopicName[0] = 0;
		TTLStatus = IdTTLRun;
	}
	return 0;
}

LRESULT CCtrlWindow::OnDdeEnd(WPARAM wParam, LPARAM lParam)
{
	EndDDE();
	if ((TTLStatus == IdTTLWaitCmndEnd) ||
		(TTLStatus == IdTTLWaitCmndResult)) {
		TTLStatus = IdTTLRun;
	}
	else if ((TTLStatus == IdTTLWait) ||
	         (TTLStatus == IdTTLWaitLn) ||
	         (TTLStatus == IdTTLWaitNL) ||
	         (TTLStatus == IdTTLWait2) ||
	         (TTLStatus == IdTTLWaitN)) {
		::SetTimer(m_hWnd, IdTimeOutTimer,0, NULL);
	}
	else if (TTLStatus==IdTTLSleep) {
		LockVar();
		if (TestWakeup(IdWakeupInit)) {
			SetResult(0);
			TTLStatus = IdTTLRun;
		}
		else if (TestWakeup(IdWakeupUnlink)) {
			SetResult(IdWakeupUnlink);
			TTLStatus = IdTTLRun;
		}
		UnlockVar();
	}
	return 0;
}

LRESULT CCtrlWindow::OnMacroBringup(WPARAM wParam, LPARAM lParam)
{
	DWORD pid;
	DWORD thisThreadId;
	DWORD fgThreadId;

	thisThreadId = GetWindowThreadProcessId(GetSafeHwnd(), &pid);
	fgThreadId = GetWindowThreadProcessId(::GetForegroundWindow(), &pid);

	if (thisThreadId == fgThreadId) {
		::SetForegroundWindow(m_hWnd);
		::BringWindowToTop(m_hWnd);
	} else {
		AttachThreadInput(thisThreadId, fgThreadId, TRUE);
		::SetForegroundWindow(m_hWnd);
		::BringWindowToTop(m_hWnd);
		AttachThreadInput(thisThreadId, fgThreadId, FALSE);
	}
	return 0;
}

void CCtrlWindow::OnDpiChanged(UINT dpi)
{
	TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTMACRO), dpi);
}

LRESULT CCtrlWindow::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case WM_DESTROY:
		OnDestroy();
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		OnPaint();
		break;
	case WM_SIZE:
		OnSize((UINT)wp, LOWORD(lp), HIWORD(lp));
		break;
	case WM_GETMINMAXINFO:
		OnGetMinMaxInfo((MINMAXINFO *)lp);
		break;
	case WM_TIMER:
		OnTimer(wp);
		break;
	case WM_USER_DDECMNDEND:
		OnDdeCmndEnd(wp, lp);
		break;
	case WM_USER_DDECOMREADY:
		OnDdeComReady(wp, lp);
		break;
	case WM_USER_DDEREADY:
		OnDdeReady(wp, lp);
		break;
	case WM_USER_MACROBRINGUP:
		OnMacroBringup(wp, lp);
		break;
	case WM_USER_DDEEND:
		OnDdeEnd(wp, lp);
		break;
	case WM_DPICHANGED:
		OnDpiChanged(LOWORD(wp));
		break;
	}
	return FALSE;
}
