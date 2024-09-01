/*
 * (C) 2019- TeraTerm Project
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
#include "compat_win.h"

#include "ftdlg_lite.h"

//	����		�\�����e
//	0-2sec		�����v�J�[�\��
//	2sec		�i��50%�ȉ��Ȃ�_�C�A���O���o��/�ȏゾ�����獻���v�̂܂�

#include "tmfc.h"

class PrivateData : public TTCDialog
{
public:
	PrivateData() {
		check_2sec = FALSE;
		show = FALSE;
		UILanguageFile_ = NULL;
		Pause = FALSE;
		observer_ = NULL;
	}

	BOOL Create(HINSTANCE hInstance, HWND hParent) {
		return TTCDialog::Create(hInstance, hParent, IDD_FILETRANSDLG);
	}
	void SetUILanguageFile(const wchar_t *UILanguageFile) {
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
		SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), UILanguageFile_);
	}

	void ChangeButton(BOOL PauseFlag)
	{
		wchar_t *UIMsg;
		Pause = PauseFlag;
		if (Pause) {
			GetI18nStrWW("Tera Term", "DLG_FILETRANS_START", L"&Start", UILanguageFile_, &UIMsg);
		}
		else {
			GetI18nStrWW("Tera Term", "DLG_FILETRANS_PAUSE", L"Pau&se", UILanguageFile_, &UIMsg);
		}
		SetDlgItemTextW(IDC_TRANSPAUSESTART, UIMsg);
		free(UIMsg);
		if (observer_ != NULL) {
			observer_->OnPause(PauseFlag);
		}
	}

private:
	virtual BOOL OnInitDialog() {
		if (HideDialog) {
			// Visible = False �ł��t�H�A�O���E���h�ɗ��Ă��܂��̂ŁA�����Ȃ�Ȃ�
			// �悤�Ɋg���X�^�C�� WS_EX_NOACTIVATE ���w�肷��B
			// (Windows 2000 �ȏ�ŗL��)
			// WS_EX_NOACTIVATE ���w�肷��ƕ\������Ă��鎞���^�X�N�o�[�Ɍ���Ȃ�
			// �̂� WS_EX_APPWINDOW ���w�肷��B
			ModifyStyleEx(0, WS_EX_NOACTIVATE | WS_EX_APPWINDOW);
		}

		TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTERM), 0);

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
		TTSetIcon(m_hInst, m_hWnd, NULL, 0);
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
		RemoveModelessHandle(m_hWnd);

		delete this;
		return TRUE;
	}

	virtual LRESULT DlgProc(UINT msg, WPARAM wp, LPARAM) {
		switch (msg) {
		case WM_DPICHANGED: {
			const UINT NewDPI = LOWORD(wp);
			TTSetIcon(m_hInst, m_hWnd, MAKEINTRESOURCEW(IDI_TTERM), NewDPI);
			return (LRESULT)TRUE;
		}
		default:
			return (LRESULT)FALSE;
		}
	}

private:
	const wchar_t *UILanguageFile_;

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

BOOL CFileTransLiteDlg::Create(HINSTANCE hInstance, HWND hParent, const wchar_t *UILanguageFile)
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
			// 2sec�o��
			pData->check_2sec = TRUE;
			if ((100.0 * (double)ByteCount / (double)FileSize) < 50) {
				// 50%�ɖ����Ȃ�
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
