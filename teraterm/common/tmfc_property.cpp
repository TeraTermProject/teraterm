/*
 * Copyright (C) 2018-2020 TeraTerm Project
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
 *		プロパティダイアログ
 */
#include "tmfc.h"

#include <windowsx.h>
#include <assert.h>
#include "dlglib.h"
#include "ttlib.h"
#include "layer_for_unicode.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

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
	m_psp.pszTemplate = MAKEINTRESOURCEW(id);
#if defined(REWRITE_TEMPLATE)
	m_psp.dwFlags |= PSP_DLGINDIRECT;
	m_psp.pResource = TTGetDlgTemplate(inst, MAKEINTRESOURCEA(id));
#endif
	m_psp.pfnDlgProc = Proc;
	m_psp.lParam = (LPARAM)this;

	m_pSheet = sheet;
}

TTCPropertyPage::~TTCPropertyPage()
{
	free((void *)m_psp.pResource);
}

HPROPSHEETPAGE TTCPropertyPage::CreatePropertySheetPage()
{
	return ::_CreatePropertySheetPageW(&m_psp);
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

void TTCPropertyPage::OnHScroll(UINT nSBCode, UINT nPos, HWND pScrollBar)
{
}

HBRUSH TTCPropertyPage::OnCtlColor(HDC hDC, HWND hWnd)
{
	return (HBRUSH)::DefWindowProc(m_hWnd, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)hWnd);
}

void TTCPropertyPage::OnHelp()
{
}

UINT CALLBACK TTCPropertyPage::PropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	return 0;
}

INT_PTR CALLBACK TTCPropertyPage::Proc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TTCPropertyPage *self = (TTCPropertyPage *)::GetWindowLongPtr(hDlgWnd, DWLP_USER);
	switch (msg)
	{
	case WM_INITDIALOG:
		self = (TTCPropertyPage *)(((PROPSHEETPAGE *)lp)->lParam);
		::SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)self);
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
		case PSN_HELP:
			self->OnHelp();
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
		return (INT_PTR)self->OnCtlColor((HDC)wp, (HWND)lp);
	case WM_HSCROLL:
		self->OnHScroll(LOWORD(wp), HIWORD(wp), (HWND)lp);
		break;
	}
	return FALSE;
}

////////////////////////////////////////

TTCPropertySheet::TTCPropertySheet(HINSTANCE hInstance, HWND hParentWnd)
{
	m_hInst = hInstance;
	m_hWnd = 0;
	m_hParentWnd = hParentWnd;
	memset(&m_psh, 0, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW | PSH_USECALLBACK;	// | PSH_MODELESS
	//m_psh.dwFlags |= PSH_PROPTITLE;		// 「のプロパティー」が追加される?
	m_psh.hwndParent = hParentWnd;
	m_psh.hInstance = hInstance;
	m_psh.pfnCallback = PropSheetProc;
}

TTCPropertySheet::~TTCPropertySheet()
{
}

INT_PTR TTCPropertySheet::DoModal()
{
	ghInstance = m_hInst;
	gTTCPS = this;
	return _PropertySheetW(&m_psh);

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
		BOOL quit = !::GetMessage(&Msg, nullptr, nullptr, nullptr);
		if (quit) {
			// QM_QUIT
			PostQuitMessage(0);
			return IDCANCEL;
		}
		if ((hDlgWnd == Msg.hwnd) ||
			::SendMessage(hDlgWnd, PSM_ISDIALOGMESSAGE, nullptr, (LPARAM)&Msg))
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
#if defined(REWRITE_TEMPLATE)
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
#endif
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
