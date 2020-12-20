/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* TERATERM.EXE, file transfer dialog box */
#include "teraterm_conf.h"

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "ftdlg.h"
#include "teraterml.h"
#include "helpid.h"
#include "filesys.h"
#include "codeconv.h"

/////////////////////////////////////////////////////////////////////////////
// CFileTransDlg dialog

CFileTransDlg::CFileTransDlg()
{
	SmallIcon = NULL;
	BigIcon = NULL;
	DlgCaption = NULL;
	FileName = NULL;
	FullName = NULL;
	ProgStat = 0;
}

CFileTransDlg::~CFileTransDlg()
{
	free(DlgCaption);
	free(FileName);
	free(FullName);
}

BOOL CFileTransDlg::Create(HINSTANCE hInstance, CFileTransDlg::Info *info)
{
	BOOL Ok;
	int fuLoad = LR_DEFAULTCOLOR;
	HWND hwnd;

	UILanguageFile = info->UILanguageFile;
	OpId = info->OpId;	// OpLog or OpSendFile のみ
	DlgCaption = _wcsdup(info->DlgCaption);
	FullName = _wcsdup(info->FullName);
	if (info->FileName != NULL) {
		FileName = _wcsdup(info->FileName);
	}
	else {
		const wchar_t *fullname = info->FullName;
		const wchar_t *p = wcsrchr(fullname, L'\\');
		if (p == NULL) {
			p = wcsrchr(fullname, L'/');
		}
		if (p == NULL) {
			FileName = _wcsdup(fullname);
		}
		else {
			FileName = _wcsdup(p + 1);
		}
	}
	HideDialog = info->HideDialog;
	HMainWin = info->HMainWin;

	Pause = FALSE;
	hwnd = GetForegroundWindow();
	if (OpId == OpLog) { // parent window is desktop
		Ok = TTCDialog::Create(hInstance, GetDesktopWindow(), IDD_FILETRANSDLG);
	}
	else { // parent window is VT window
		Ok = TTCDialog::Create(hInstance, NULL, IDD_FILETRANSDLG);
	}
	if (OpId == OpSendFile) {
		HWND HProg = ::GetDlgItem(m_hWnd, IDC_TRANSPROGRESS);

		ProgStat = 0;

		::SendMessage(HProg, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 100));
		::SendMessage(HProg, PBM_SETSTEP, (WPARAM)1, 0);
		::SendMessage(HProg, PBM_SETPOS, (WPARAM)0, 0);

		::ShowWindow(HProg, SW_SHOW);

		::ShowWindow(GetDlgItem(IDC_TRANS_ELAPSED), SW_SHOW);
	}

	if (!HideDialog) {
		// Visible = False のダイアログを表示する
		ShowWindow(SW_SHOWNORMAL);
		if (OpId == OpLog) {
			ShowWindow(SW_MINIMIZE);
		}
	}
	else {
		// 直前にフォアグラウンドだったウィンドウにフォーカスを戻す。
		// メニューからログをスタートした時に VTWin にフォーカスが戻らないので必要っぽい。
		::SetForegroundWindow(hwnd);
	}

	return Ok;
}

/**
 *	テキストの変更のみ
 */
void CFileTransDlg::ChangeButton(BOOL PauseFlag)
{
	Pause = PauseFlag;
	if (Pause) {
		static const DlgTextInfo TextInfos[] = {
			{ IDC_TRANSPAUSESTART, "DLG_FILETRANS_START" },
		};
		SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);
	}
	else {
		static const DlgTextInfo TextInfos[] = {
			{ IDC_TRANSPAUSESTART, "DLG_FILETRANS_PAUSE" },
		};
		SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);
	}
}

void CFileTransDlg::RefreshNum(DWORD StartTime, LONG FileSize, LONG ByteCount)
{
	char NumStr[24];
	double rate;
	int rate2;
	static DWORD prev_elapsed;
	DWORD elapsed;

	if (OpId == OpSendFile) {
		if (StartTime == 0) {
			SetDlgItemText(IDC_TRANS_ETIME, "0:00");
			prev_elapsed = 0;
		}
		else {
			elapsed = (GetTickCount() - StartTime) / 1000;
			if (elapsed != prev_elapsed && elapsed != 0) {
				rate2 = ByteCount / elapsed;
				if (rate2 < 1200) {
					_snprintf_s(NumStr, sizeof(NumStr), _TRUNCATE, "%d:%02d (%dBytes/s)", elapsed / 60, elapsed % 60, rate2);
				}
				else if (rate2 < 1200000) {
					_snprintf_s(NumStr, sizeof(NumStr), _TRUNCATE, "%d:%02d (%d.%02dKB/s)", elapsed / 60, elapsed % 60, rate2 / 1000, rate2 / 10 % 100);
				}
				else {
					_snprintf_s(NumStr, sizeof(NumStr), _TRUNCATE, "%d:%02d (%d.%02dMB/s)", elapsed / 60, elapsed % 60, rate2 / (1000*1000), rate2 / 10000 % 100);
				}
				SetDlgItemText(IDC_TRANS_ETIME, NumStr);
				prev_elapsed = elapsed;
			}
		}

		if (FileSize > 0) {
			rate = 100.0 * (double)ByteCount / (double)FileSize;
			if (ProgStat < (int)rate) {
				ProgStat = (int)rate;
				SendDlgItemMessage(IDC_TRANSPROGRESS, PBM_SETPOS, (WPARAM)ProgStat, 0);
			}
			_snprintf_s(NumStr,sizeof(NumStr),_TRUNCATE,"%u (%3.1f%%)",ByteCount, rate);
		}
		SetDlgItemText(IDC_TRANSBYTES, NumStr);
	}
	else {
		_snprintf_s(NumStr,sizeof(NumStr),_TRUNCATE,"%u",ByteCount);
		SetDlgItemText(IDC_TRANSBYTES, NumStr);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFileTransDlg message handler

BOOL CFileTransDlg::OnInitDialog()
{
	static const DlgTextInfo TextInfos[] = {
		{ IDC_TRANS_FILENAME, "DLG_FILETRANS_FILENAME" },
		{ IDC_FULLPATH_LABEL, "DLG_FILETRANS_FULLPATH" },
		{ IDC_TRANS_TRANS, "DLG_FILETRANS_TRNAS" },
		{ IDC_TRANS_ELAPSED, "DLG_FILETRANS_ELAPSED" },
		{ IDCANCEL, "DLG_FILETRANS_CLOSE" },
		{ IDC_TRANSPAUSESTART, "DLG_FILETRANS_PAUSE" },
		{ IDC_TRANSHELP, "BTN_HELP" },
	};

	int fuLoad = LR_DEFAULTCOLOR;

	if (HideDialog) {
		// Visible = False でもフォアグラウンドに来てしまうので、そうならない
		// ように拡張スタイル WS_EX_NOACTIVATE を指定する。
		// (Windows 2000 以上で有効)
		// WS_EX_NOACTIVATE を指定すると表示されている時もタスクバーに現れない
		// ので WS_EX_APPWINDOW も指定する。
		ModifyStyleEx(0, WS_EX_NOACTIVATE | WS_EX_APPWINDOW);
	}

	SetWindowTextW(DlgCaption);
	SetDlgItemTextW(IDC_TRANSFNAME, FileName);
	SetDlgItemTextW(IDC_EDIT_FULLPATH, FullName);

	SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile);

	if (IsWindowsNT4()) {
		fuLoad = LR_VGACOLOR;
	}
	SmallIcon = LoadImage(m_hInst,
						  MAKEINTRESOURCE(IDI_TTERM),
						  IMAGE_ICON, 16, 16, fuLoad);
	::PostMessage(GetSafeHwnd(), WM_SETICON, ICON_SMALL,
				  (LPARAM)SmallIcon);

	BigIcon = LoadImage(m_hInst,
						MAKEINTRESOURCE(IDI_TTERM),
						IMAGE_ICON, 0, 0, fuLoad);
	::PostMessage(GetSafeHwnd(), WM_SETICON, ICON_BIG,
				  (LPARAM)BigIcon);

	AddModelessHandle(m_hWnd);

	return TRUE;
}

BOOL CFileTransDlg::OnCancel( )
{
	if (OpId == OpLog) {
		FLogClose();
	}
	else {
		FileSendEnd();
	}
	return TRUE;
}

BOOL CFileTransDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
		case IDC_TRANSPAUSESTART:
			ChangeButton(! Pause);
			if (OpId == OpLog) {
				FLogPause(Pause);
			}
			else {
				FileSendPause(Pause);
			}
			return TRUE;
		case IDC_TRANSHELP:
			if (OpId == OpLog) {
				::PostMessage(HMainWin, WM_USER_DLGHELP2, HlpFileLog, 0);
			}
			else {
				::PostMessage(HMainWin, WM_USER_DLGHELP2, HlpFileSend, 0);
			}
			return TRUE;
		default:
			return (TTCDialog::OnCommand(wParam,lParam));
	}
}

BOOL CFileTransDlg::PostNcDestroy()
{
	// logopenとlogcloseを繰り返すと、GDIリソースリークとなる問題を修正した。
	//   - LoadImage()によるアイコンリソースを解放する。
	// (2016.10.5 yutaka)
	if (SmallIcon) {
		DestroyIcon((HICON)SmallIcon);
		SmallIcon = NULL;
	}

	if (BigIcon) {
		DestroyIcon((HICON)BigIcon);
		BigIcon = NULL;
	}

	RemoveModelessHandle(m_hWnd);

	delete this;
	return TRUE;
}
