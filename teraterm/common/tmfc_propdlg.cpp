/*
 * (C) 2022- TeraTerm Project
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

#include "ttlib.h"
#include "dlglib.h"

#include "tmfc_propdlg.h"

#define REWRITE_TEMPLATE	1
// quick hack :-(
HINSTANCE TTCPropSheetDlg::ghInstance;
class TTCPropSheetDlg* TTCPropSheetDlg::gTTCPS;

LRESULT CALLBACK TTCPropSheetDlg::WndProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg){
	case WM_INITDIALOG:
	case WM_SHOWWINDOW: {
		CenterWindow(dlg, m_hParentWnd);
		break;
		}
	}
	SetWindowLongPtrW(dlg, GWLP_WNDPROC, m_OrgProc);
	SetWindowLongPtrW(dlg, GWLP_USERDATA, m_OrgUserData);
	LRESULT result = CallWindowProcW((WNDPROC)m_OrgProc, dlg, msg, wParam, lParam);
	m_OrgProc = SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)WndProcStatic);
	m_OrgUserData = SetWindowLongPtrW(dlg, GWLP_USERDATA, (LONG_PTR)this);

	return result;
}

LRESULT CALLBACK TTCPropSheetDlg::WndProcStatic(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TTCPropSheetDlg*self = (TTCPropSheetDlg*)GetWindowLongPtr(dlg, GWLP_USERDATA);
	return self->WndProc(dlg, msg, wParam, lParam);
}

int CALLBACK TTCPropSheetDlg::PropSheetProc(HWND hWnd, UINT msg, LPARAM lp)
{
	switch (msg) {
	case PSCB_PRECREATE:
	{
#if defined(REWRITE_TEMPLATE)
		// テンプレートの内容を書き換える
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
		static const DlgTextInfo TextInfos[] = {
			{ IDOK, "BTN_OK" },
			{ IDCANCEL, "BTN_CANCEL" },
			{ IDHELP, "BTN_HELP" },
		};
		TTCPropSheetDlg*self = gTTCPS;
		self->m_hWnd = hWnd;
		SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), self->m_UiLanguageFile);
		self->m_OrgProc = SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcStatic);
		self->m_OrgUserData = SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)self);
		break;
	}
	}
	return 0;
}

void TTCPropSheetDlg::AddPage(HPROPSHEETPAGE page)
{
	hPsp[m_PageCount] = page;
	m_PageCount++;
}

TTCPropSheetDlg::TTCPropSheetDlg(HINSTANCE hInstance, HWND hParentWnd, const wchar_t *uilangfile)
{
	m_hInst = hInstance;
	m_hWnd = 0;
	m_hParentWnd = hParentWnd;
	m_UiLanguageFile = _wcsdup(uilangfile);
	memset(&m_psh, 0, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW | PSH_USECALLBACK;
	//m_psh.dwFlags |= PSH_PROPTITLE;		// 「のプロパティー」が追加される?
	m_psh.hwndParent = hParentWnd;
	m_psh.hInstance = hInstance;
	m_psh.pfnCallback = PropSheetProc;
	m_PageCount = 0;
}
TTCPropSheetDlg::~TTCPropSheetDlg()
{
	free((void*)m_psh.pszCaption);
	free(m_UiLanguageFile);
}

void TTCPropSheetDlg::SetCaption(const wchar_t* caption)
{
	free((void*)m_psh.pszCaption);
	m_psh.pszCaption = _wcsdup(caption);
}

INT_PTR TTCPropSheetDlg::DoModal()
{
	m_psh.nPages = m_PageCount;
	m_psh.phpage = hPsp;
	ghInstance = m_hInst;
	gTTCPS = this;
	return PropertySheetW(&m_psh);
}
