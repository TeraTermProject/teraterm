/*
 * Copyright (C) 2018 TeraTerm Project
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

/*
 * Tera term Micro Framework class
 */
#include "tmfc.h"

#include <windowsx.h>
#include "dlglib.h"

LRESULT TTCWnd::SendMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	return ::SendMessage(m_hWnd, msg, wp, lp);
}

LRESULT TTCWnd::SendDlgItemMessage(int id, UINT msg, WPARAM wp, LPARAM lp)
{
	return ::SendDlgItemMessage(m_hWnd, id, msg, wp, lp);
}

void TTCWnd::GetDlgItemText(int id, TCHAR *buf, size_t size)
{
	::GetDlgItemText(m_hWnd, id, buf, size);
}

void TTCWnd::SetDlgItemText(int id, const TCHAR *str)
{
	::SetDlgItemText(m_hWnd, id, str);
}

// nCheck	BST_UNCHECKED / BST_CHECKED / BST_INDETERMINATE 
void TTCWnd::SetCheck(int id, int nCheck)
{
	::SendMessage(::GetDlgItem(m_hWnd, id), BM_SETCHECK, nCheck, 0);
}

UINT TTCWnd::GetCheck(int id)
{
	return ::IsDlgButtonChecked(m_hWnd, id);
}

void TTCWnd::SetCurSel(int id, int no)
{
	::SendMessage(::GetDlgItem(m_hWnd, id), CB_SETCURSEL, no, 0);
}

int TTCWnd::GetCurSel(int id)
{
	LRESULT lResult = ::SendMessage(::GetDlgItem(m_hWnd, id), CB_GETCURSEL, 0, 0);
	return (int)lResult;
}

void TTCWnd::EnableDlgItem(int id, BOOL enable)
{
	::EnableWindow(::GetDlgItem(m_hWnd, id), enable);
}

void TTCWnd::SetDlgItemInt(int id, UINT val, BOOL bSigned)
{
	::SetDlgItemInt(m_hWnd, id, val, bSigned);
}

UINT TTCWnd::GetDlgItemInt(int id, BOOL* lpTrans, BOOL bSigned) const
{
	return ::GetDlgItemInt(m_hWnd, id, lpTrans, bSigned);
}

void TTCWnd::ShowWindow(int nCmdShow)
{
	::ShowWindow(m_hWnd, nCmdShow);
}

void TTCWnd::SetWindowText(TCHAR *str)
{
	::SetWindowText(m_hWnd, str);
}

static void ModifyStyleCom(
	HWND hWnd, int nStyleOffset,
	DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0)
{
	const DWORD dwStyle = ::GetWindowLong(hWnd, nStyleOffset);
	const DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if (dwStyle != dwNewStyle) {
		::SetWindowLong(hWnd, nStyleOffset, dwNewStyle);
	}
	if (nFlags != 0)
	{
		::SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
					   SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}
}

void TTCWnd::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	ModifyStyleCom(m_hWnd, GWL_STYLE, dwRemove, dwAdd, nFlags);
}

void TTCWnd::ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	ModifyStyleCom(m_hWnd, GWL_EXSTYLE, dwRemove, dwAdd, nFlags);
}

int TTCWnd::MessageBox(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	return ::MessageBox(m_hWnd, lpText, lpCaption, uType);
}

////////////////////////////////////////

TTCDialog::TTCDialog()
{
}

TTCDialog::~TTCDialog()
{
}

void TTCDialog::OnInitDialog()
{
}

BOOL TTCDialog::Create(HINSTANCE hInstance, HWND hParent, int idd)
{
#if 0
	HRSRC hResource = ::FindResource(hInstance, MAKEINTRESOURCE(idd), RT_DIALOG);
	HANDLE hDlgTemplate = ::LoadResource(hInstance, hResource);
	DLGTEMPLATE *lpTemplate = (DLGTEMPLATE *)::LockResource(hDlgTemplate);
#else
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, MAKEINTRESOURCE(idd));
#endif
	HWND hWnd = ::CreateDialogIndirectParam(	
		hInstance, lpTemplate, hParent,
		(DLGPROC)OnDlgProc, (LPARAM)this);
	if (hWnd == NULL)
	{
		return FALSE;
	}

	m_hWnd = hWnd;
//	::EnableWindow(hParent,FALSE);
	::ShowWindow(hWnd, SW_SHOW);
//	::EnableWindow(m_hWnd,TRUE);

	return TRUE;
}

BOOL TTCDialog::OnCommand(WPARAM wp, LPARAM lp)
{
	return TRUE;
}

void TTCDialog::OnOK()
{
	DestroyWindow();
}

void TTCDialog::OnCancel()
{
	DestroyWindow();
}

void TTCDialog::PostNcDestroy()
{
	delete this;
}

void TTCDialog::DestroyWindow()
{
	::EnableWindow(m_hParentWnd,TRUE);
	::SetFocus(m_hParentWnd);
	::DestroyWindow(m_hWnd);
}

LRESULT CALLBACK TTCDialog::OnDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TTCDialog *self = (TTCDialog *)GetWindowLongPtr(hDlgWnd, DWLP_USER);

	switch (msg) {
	case WM_INITDIALOG:
	{
		self = (TTCDialog *)lp;
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)self);
		self->OnInitDialog();
		return TRUE;
	}

	case WM_COMMAND:
	{
		WORD wID = GET_WM_COMMAND_ID(wp, lp);
		const WORD wCMD = GET_WM_COMMAND_CMD(wp, lp);
		if (wID == IDOK) {
			self->OnOK();
			self->DestroyWindow();
		}
		if (wID == IDCANCEL) {
			self->OnCancel();
		}
		return FALSE;
	}
	case WM_NCDESTROY:
		self->PostNcDestroy();
		return TRUE;

	default:
		return FALSE;
	}
	return TRUE;
}

////////////////////////////////////////

// quick hack :-(
static HINSTANCE ghInstance;
static class TTCPropertySheet *gTTCPS;

TTCPropertyPage::TTCPropertyPage(HINSTANCE inst, int id, TTCPropertySheet *sheet)
{
	memset(&m_psp, 0, sizeof(m_psp));
	m_psp.dwSize = sizeof(m_psp);
	m_psp.dwFlags = PSP_DEFAULT;
	m_psp.hInstance = inst;
	m_psp.pszTemplate = MAKEINTRESOURCE(id);
#if 1
	m_psp.dwFlags |= PSP_DLGINDIRECT;
	m_psp.pResource = TTGetDlgTemplate(inst, m_psp.pszTemplate);
#endif
	m_psp.pfnDlgProc = (DLGPROC)Proc;
	m_psp.lParam = (LPARAM)this;

	m_pSheet = sheet;
}

TTCPropertyPage::~TTCPropertyPage()
{
	free((void *)m_psp.pResource);
}

void TTCPropertyPage::OnInitDialog()
{
}

void TTCPropertyPage::OnOK()
{
}

BOOL TTCPropertyPage::OnCommand(WPARAM wp, LPARAM lp)
{
	return TRUE;
}

HBRUSH TTCPropertyPage::OnCtlColor(HDC hDC, HWND hWnd)
{
	return (HBRUSH)::DefWindowProc(m_hWnd, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)hWnd);
}

UINT CALLBACK TTCPropertyPage::PropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	return 0;
}

BOOL CALLBACK TTCPropertyPage::Proc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TTCPropertyPage *self = (TTCPropertyPage *)GetWindowLongPtr(hDlgWnd, DWLP_USER);
	switch (msg)
	{
	case WM_INITDIALOG:
		self = (TTCPropertyPage *)(((PROPSHEETPAGE *)lp)->lParam);
		SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)self);
		self->m_hWnd = hDlgWnd;
		self->OnInitDialog();
		break;
	case WM_NOTIFY:
	{
		NMHDR * nmhdr = (NMHDR *)lp;
		switch (nmhdr->code)
		{
		case PSN_APPLY:
			self->OnOK();
			break;
		default:
			break;
		}
		break;
	}
	case WM_COMMAND:
		self->OnCommand(wp, lp);
		break;
	case WM_CTLCOLORSTATIC:
		return (BOOL)self->OnCtlColor((HDC)wp, (HWND)lp);
	}
	return FALSE;
}

////////////////////////////////////////

TTCPropertySheet::TTCPropertySheet(HINSTANCE hInstance, LPCTSTR pszCaption, HWND hParentWnd)
{
	m_hInst = hInstance;
	memset(&m_psh, 0, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW | PSH_USECALLBACK;	// | PSH_MODELESS
	if (pszCaption != NULL) {
		m_psh.pszCaption = pszCaption;
		//m_psh.dwFlags |= PSH_PROPTITLE;		// 「のプロパティー」が追加される?
	}
	m_psh.hwndParent = hParentWnd;
	m_psh.pfnCallback = PropSheetProc;
}
	
TTCPropertySheet::~TTCPropertySheet()
{
}

int TTCPropertySheet::DoModal()
{
	ghInstance = m_hInst;
	gTTCPS = this;
	return PropertySheet(&m_psh);

	// モーダレスにするとタブの動きがおかしい
#if 0
	// モードレスダイアログボックスの場合はウィンドウのハンドル
	m_hWnd = (HWND)::PropertySheet(&m_psh);
//	ShowWindow(m_hWnd, SW_SHOW);
	
//	::ModifyStyle(m_hWnd, TCS_MULTILINE, TCS_SINGLELINE, 0);

	ModalResult = 0;
	HWND hDlgWnd = m_hWnd;
	for(;;) {
		if (ModalResult != 0) {
			break;
		}
		MSG Msg;
		BOOL quit = !::GetMessage(&Msg, NULL, NULL, NULL);
		if (quit) {
			// QM_QUIT
			PostQuitMessage(0);
			return IDCANCEL;
		}
		if ((hDlgWnd == Msg.hwnd) ||
			::SendMessage(hDlgWnd, PSM_ISDIALOGMESSAGE, NULL, (LPARAM)&Msg))
		{
			// ダイアログ以外の処理
			::TranslateMessage(&Msg);
			::DispatchMessage(&Msg);
		}
		if (!SendMessage(hDlgWnd, PSM_GETCURRENTPAGEHWND, 0, 0)) {
			// プロパティーシート終了
			break;
		}
	}
	return ModalResult;
#endif
}

int CALLBACK TTCPropertySheet::PropSheetProc(HWND hWnd, UINT msg, LPARAM lp)
{
	switch (msg) {
	case PSCB_PRECREATE:
	{
		// テンプレートの内容を書き換える 危険
		// http://home.att.ne.jp/banana/akatsuki/doc/atlwtl/atlwtl15-01/index.html
		size_t PrevTemplSize;
		size_t NewTemplSize;
		DLGTEMPLATE *NewTempl =
			TTGetNewDlgTemplate(ghInstance, (DLGTEMPLATE *)lp,
								&PrevTemplSize, &NewTemplSize);
		NewTempl->style &= ~DS_CONTEXTHELP;		// check DLGTEMPLATEEX
		memcpy((void *)lp, NewTempl, NewTemplSize);
		free(NewTempl);
		break;
	}
	case PSCB_INITIALIZED:
	{
		//TTCPropertySheet *self = (TTCPropertySheet *)lp;
		TTCPropertySheet *self = gTTCPS;
		self->m_hWnd = hWnd;
		self->OnInitDialog();
		break;
	}
	}
	return 0;
}

void TTCPropertySheet::OnInitDialog()
{
}
