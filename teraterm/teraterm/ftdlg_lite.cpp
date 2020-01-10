/*
 * (C) 2019-2020 TeraTerm Project
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

/* TERATERM.EXE, file transfer dialog box lite */
#include "teraterm_conf.h"

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttlib.h"
#include "dlglib.h"
#include "tt_res.h"
#include "teraterml.h"

#include "ftdlg_lite.h"

//	時間		表示内容
//	0-2sec		砂時計カーソル
//	2sec		進捗50%以下ならダイアログを出す/以上だったら砂時計のまま

#include "tmfc.h"

class PrivateData : public TTCDialog
{
public:
	PrivateData() {
		SmallIcon = NULL;
		BigIcon = NULL;
		check_2sec = NULL;
		show = FALSE;
		UILanguageFile_ = NULL;
		Pause = FALSE;
		observer_ = NULL;
	}

	BOOL Create(HINSTANCE hInstance, HWND hParent) {
		return TTCDialog::Create(hInstance, hParent, IDD_FILETRANSDLG);
	}
	void SetUILanguageFile(const char *UILanguageFile) {
		static const DlgTextInfo TextInfos[] = {
			{ IDC_TRANS_FILENAME, "DLG_FILETRANS_FILENAME" },
			{ IDC_FULLPATH_LABEL, "DLG_FILETRANS_FULLPATH" },
			{ IDC_TRANS_TRANS, "DLG_FILETRANS_TRNAS" },
			{ IDC_TRANS_ELAPSED, "DLG_FILETRANS_ELAPSED" },
			{ IDCANCEL, "DLG_FILETRANS_CLOSE" },
			{ IDC_TRANSPAUSESTART, "DLG_FILETRANS_PAUSE" },
			{ IDC_TRANSHELP, "BTN_HELP" },
		};
		UILanguageFile_ = UILanguageFile;
		SetDlgTexts(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile_);
	}

	void ChangeButton(BOOL PauseFlag)
	{
		wchar_t UIMsg[MAX_UIMSG];
		Pause = PauseFlag;
		if (Pause) {
			get_lang_msgW("DLG_FILETRANS_START", UIMsg, _countof(UIMsg), L"&Start", UILanguageFile_);
		}
		else {
			get_lang_msgW("DLG_FILETRANS_PAUSE", UIMsg, _countof(UIMsg), L"Pau&se", UILanguageFile_);

		}
		SetDlgItemTextW(IDC_TRANSPAUSESTART, UIMsg);
		if (observer_ != NULL) {
			observer_->OnPause(PauseFlag);
		}
	}

private:
	virtual BOOL OnInitDialog() {
		int fuLoad = LR_DEFAULTCOLOR;

		if (HideDialog) {
			// Visible = False でもフォアグラウンドに来てしまうので、そうならない
			// ように拡張スタイル WS_EX_NOACTIVATE を指定する。
			// (Windows 2000 以上で有効)
			// WS_EX_NOACTIVATE を指定すると表示されている時もタスクバーに現れない
			// ので WS_EX_APPWINDOW も指定する。
			ModifyStyleEx(0, WS_EX_NOACTIVATE | WS_EX_APPWINDOW);
		}

		if (IsWindowsNT4()) {
			fuLoad = LR_VGACOLOR;
		}
		SmallIcon = LoadImage(m_hInst,
							  MAKEINTRESOURCE(IDI_TTERM),
							  IMAGE_ICON, 16, 16, fuLoad);
		::PostMessage(m_hWnd, WM_SETICON, ICON_SMALL,
					  (LPARAM)SmallIcon);

		BigIcon = LoadImage(m_hInst,
							MAKEINTRESOURCE(IDI_TTERM),
							IMAGE_ICON, 0, 0, fuLoad);
		::PostMessage(m_hWnd, WM_SETICON, ICON_BIG,
					  (LPARAM)BigIcon);

		AddModelessHandle(m_hWnd);

		if (observer_ == NULL){
			EnableDlgItem(IDCANCEL, FALSE);
			EnableDlgItem(IDC_TRANSPAUSESTART, FALSE);
			EnableDlgItem(IDC_TRANSHELP, FALSE);
		}
		return TRUE;
	}

	virtual BOOL OnClose()
	{
		if (observer_ != NULL) {
			observer_->OnClose();
		}
		return TRUE;
	}

	virtual BOOL OnCancel()
	{
		return OnClose();
	}

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam)) {
			case IDC_TRANSPAUSESTART:
				ChangeButton(!Pause);
				return TRUE;
			case IDC_TRANSHELP:
				observer_->OnHelp();
				return TRUE;
			default:
				return (TTCDialog::OnCommand(wParam, lParam));
		}
	}

	virtual BOOL PostNcDestroy() {
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

private:
	HANDLE SmallIcon;
	HANDLE BigIcon;
	const char *UILanguageFile_;

public:
	BOOL Pause;
	BOOL check_2sec;
	BOOL show;
	DWORD prev_elapsed;
	DWORD StartTime;
	BOOL HideDialog;

	CFileTransLiteDlg::Observer *observer_;
};

CFileTransLiteDlg::CFileTransLiteDlg()
{
	pData = NULL;
}

CFileTransLiteDlg::~CFileTransLiteDlg()
{
	if (pData == NULL) {
		return;
	}

	Destroy();
}

BOOL CFileTransLiteDlg::Create(HINSTANCE hInstance, HWND hParent, const char *UILanguageFile)
{
	pData = new PrivateData();
	pData->check_2sec = FALSE;
	pData->show = FALSE;
	pData->Pause = FALSE;
	pData->HideDialog = FALSE;

	BOOL Ok = pData->Create(hInstance, hParent);
	pData->SetUILanguageFile(UILanguageFile);

	HWND hWnd = ::GetDlgItem(pData->m_hWnd, IDC_TRANSPROGRESS);
	::SendMessage(hWnd, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, 100));
	::SendMessage(hWnd, PBM_SETSTEP, (WPARAM)1, 0);
	::SendMessage(hWnd, PBM_SETPOS, (WPARAM)0, 0);
	::ShowWindow(hWnd, SW_SHOW);

	pData->SetDlgItemTextA(IDC_TRANS_ETIME, "0:00");
	pData->StartTime = GetTickCount();
	pData->prev_elapsed = 0;

	return Ok;
}

void CFileTransLiteDlg::ChangeButton(BOOL PauseFlag)
{
	pData->ChangeButton(PauseFlag);
}

void CFileTransLiteDlg::RefreshNum(size_t ByteCount, size_t FileSize)
{
	const DWORD now = GetTickCount();

	if (!pData->check_2sec) {
		DWORD elapsed_ms = now - pData->StartTime;
		if (elapsed_ms > 2 * 1000) {
			// 2sec経過
			pData->check_2sec = TRUE;
			if ((100.0 * (double)ByteCount / (double)FileSize) < 50) {
				// 50%に満たない
				pData->ShowWindow(SW_SHOWNORMAL);
			}
		}
	}

	char NumStr[24];
	DWORD elapsed = (now - pData->StartTime) / 1000;
	if (elapsed != pData->prev_elapsed && elapsed != 0) {
		char elapsed_str[24];
		_snprintf_s(elapsed_str, sizeof(elapsed_str), _TRUNCATE, "%ld:%02ld",
					elapsed / 60, elapsed % 60);

		char speed_str[24];
		size_t rate2 = ByteCount / elapsed;
		if (rate2 < 1200) {
			_snprintf_s(speed_str, sizeof(speed_str), _TRUNCATE, "%lldBytes/s", (unsigned long long)rate2);
		}
		else if (rate2 < 1200000) {
			_snprintf_s(speed_str, sizeof(speed_str), _TRUNCATE, "%lld.%02lldKB/s",
						(unsigned long long)(rate2 / 1000), (unsigned long long)(rate2 / 10 % 100));
		}
		else {
			_snprintf_s(speed_str, sizeof(speed_str), _TRUNCATE, "%lld.%02lldMB/s",
						(unsigned long long)(rate2 / (1000*1000)), (unsigned long long)(rate2 / 10000 % 100));
		}
		_snprintf_s(NumStr, sizeof(NumStr), _TRUNCATE, "%s (%s)", elapsed_str, speed_str);
		pData->SetDlgItemTextA(IDC_TRANS_ETIME, NumStr);
		pData->prev_elapsed = elapsed;
	}

	if (FileSize > 0) {
		double rate = 100.0 * (double)ByteCount / (double)FileSize;
		pData->SendDlgItemMessage(IDC_TRANSPROGRESS, PBM_SETPOS, (WPARAM)rate, 0);
		_snprintf_s(NumStr,sizeof(NumStr),_TRUNCATE,"%u (%3.1f%%)", (int)ByteCount, rate);
	}
	else {
		_snprintf_s(NumStr,sizeof(NumStr),_TRUNCATE,"%u", (int)ByteCount);
	}
	pData->SetDlgItemTextA(IDC_TRANSBYTES, NumStr);
}

void CFileTransLiteDlg::SetCaption(const wchar_t *caption)
{
	pData->SetWindowTextW(caption);
}

void CFileTransLiteDlg::SetFilename(const wchar_t *filename)
{
	pData->SetDlgItemTextW(IDC_TRANSFNAME, filename);
	pData->SetDlgItemTextW(IDC_EDIT_FULLPATH, filename);
}

void CFileTransLiteDlg::SetObserver(CFileTransLiteDlg::Observer *observer)
{
	pData->observer_ = observer;
	BOOL enable = observer != NULL;
	pData->EnableDlgItem(IDCANCEL, enable);
	pData->EnableDlgItem(IDC_TRANSPAUSESTART, enable);
	pData->EnableDlgItem(IDC_TRANSHELP, enable);
}

void CFileTransLiteDlg::Destroy()
{
	pData->EndDialog(IDOK);
	pData = NULL;
}
