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
 */
#include "tmfc.h"

#include <windowsx.h>
#include <assert.h>
#include "dlglib.h"
#include "ttlib.h"
#include "compat_win.h"
#include "layer_for_unicode.h"

// テンプレートの書き換えを行う
#define REWRITE_TEMPLATE

#if (defined(_MSC_VER) && (_MSC_VER <= 1500)) || \
	(__cplusplus <= 199711L)
#define nullptr NULL	// C++11,nullptr / > VS2010
#endif

TTCWnd::TTCWnd()
{
	m_hWnd = nullptr;
	m_hInst = nullptr;
	m_hAccel = nullptr;
	m_hParentWnd = nullptr;
	m_WindowUnicode = FALSE;
}

LRESULT TTCWnd::SendMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	return ::SendMessage(m_hWnd, msg, wp, lp);
}

HWND TTCWnd::GetDlgItem(int id)
{
	return ::GetDlgItem(m_hWnd, id);
}

LRESULT TTCWnd::SendDlgItemMessageW(int id, UINT msg, WPARAM wp, LPARAM lp)
{
	return ::_SendDlgItemMessageW(m_hWnd, id, msg, wp, lp);
}

LRESULT TTCWnd::SendDlgItemMessageA(int id, UINT msg, WPARAM wp, LPARAM lp)
{
	return ::SendDlgItemMessageA(m_hWnd, id, msg, wp, lp);
}

void TTCWnd::GetDlgItemTextW(int id, wchar_t *buf, size_t size)
{
	_GetDlgItemTextW(m_hWnd, id, buf, (int)size);
}

void TTCWnd::GetDlgItemTextA(int id, char *buf, size_t size)
{
	::GetDlgItemTextA(m_hWnd, id, buf, (int)size);
}

void TTCWnd::SetDlgItemTextW(int id, const wchar_t *str)
{
	_SetDlgItemTextW(m_hWnd, id, str);
}

void TTCWnd::SetDlgItemTextA(int id, const char *str)
{
	::SetDlgItemTextA(m_hWnd, id, str);
}

void TTCWnd::SetDlgItemNum(int id, LONG Num)
{
	SetDlgNum(m_hWnd, id, Num);
}

// nCheck	BST_UNCHECKED / BST_CHECKED / BST_INDETERMINATE
void TTCWnd::SetCheck(int id, int nCheck)
{
	::SendMessage(GetDlgItem(id), BM_SETCHECK, nCheck, 0);
}

UINT TTCWnd::GetCheck(int id)
{
	return ::IsDlgButtonChecked(m_hWnd, id);
}

void TTCWnd::SetCurSel(int id, int no)
{
	HWND hWnd = GetDlgItem(id);
	assert(hWnd != 0);
	char ClassName[32];
	int r = GetClassNameA(hWnd, ClassName, _countof(ClassName));
	assert(r != 0); (void)r;
	UINT msg =
		(strcmp(ClassName, "ListBox") == 0) ? LB_SETCURSEL :
		(strcmp(ClassName, "ComboBox") == 0) ? CB_SETCURSEL : 0;
	assert(msg != 0);
	::SendMessage(hWnd, msg, no, 0);
}

int TTCWnd::GetCurSel(int id)
{
	HWND hWnd = GetDlgItem(id);
	assert(hWnd != 0);
	char ClassName[32];
	int r = GetClassNameA(hWnd, ClassName, _countof(ClassName));
	assert(r != 0); (void)r;
	UINT msg =
		(strcmp(ClassName, "ListBox") == 0) ? LB_GETCURSEL :
		(strcmp(ClassName, "ComboBox") == 0) ? CB_GETCURSEL : 0;
	assert(msg != 0);
	LRESULT lResult = ::SendMessage(hWnd, msg, 0, 0);
	return (int)lResult;
}

void TTCWnd::EnableDlgItem(int id, BOOL enable)
{
	::EnableWindow(GetDlgItem(id), enable);
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

void TTCWnd::SetWindowTextW(const wchar_t *str)
{
	_SetWindowTextW(m_hWnd, str);
}

void TTCWnd::SetWindowTextA(const char *str)
{
	::SetWindowTextA(m_hWnd, str);
}

LONG_PTR TTCWnd::SetWindowLongPtr(int nIndex, LONG_PTR dwNewLong)
{
	return ::SetWindowLongPtr(m_hWnd, nIndex, dwNewLong);
}

LONG_PTR TTCWnd::GetWindowLongPtr(int nIndex)
{
	return ::GetWindowLongPtr(m_hWnd, nIndex);
}

void TTCWnd::ModifyStyleCom(int nStyleOffset,
							DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	const LONG_PTR dwStyle = GetWindowLongPtr(nStyleOffset);
	const LONG_PTR add = dwAdd;
	const LONG_PTR remove = dwRemove;
	const LONG_PTR dwNewStyle = (dwStyle & ~remove) | add;
	if (dwStyle != dwNewStyle) {
		SetWindowLongPtr(nStyleOffset, dwNewStyle);
	}
	if (nFlags != 0)
	{
		SetWindowPos(nullptr, 0, 0, 0, 0,
					 SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}
}

void TTCWnd::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	ModifyStyleCom(GWL_STYLE, dwRemove, dwAdd, nFlags);
}

void TTCWnd::ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	ModifyStyleCom(GWL_EXSTYLE, dwRemove, dwAdd, nFlags);
}

int TTCWnd::MessageBoxA(const char *lpText, const char *lpCaption, UINT uType)
{
	return ::MessageBoxA(m_hWnd, lpText, lpCaption, uType);
}

#if defined(UNICODE)
int TTCWnd::MessageBoxW(const wchar_t *lpText, const wchar_t *lpCaption, UINT uType)
{
	return ::MessageBoxW(m_hWnd, lpText, lpCaption, uType);
}
#endif

BOOL TTCWnd::GetWindowRect(RECT *R)
{
	return ::GetWindowRect(m_hWnd, R);
}

BOOL TTCWnd::SetWindowPos(HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	return ::SetWindowPos(m_hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

BOOL TTCWnd::GetClientRect(RECT *R)
{
	return ::GetClientRect(m_hWnd, R);
}

BOOL TTCWnd::InvalidateRect(RECT *R, BOOL bErase)
{
	return ::InvalidateRect(m_hWnd, R, bErase);
}

BOOL TTCWnd::EndDialog(int nResult)
{
	return ::EndDialog(m_hWnd, nResult);
}

void TTCWnd::DestroyWindow()
{
	::DestroyWindow(m_hWnd);
}

HDC TTCWnd::BeginPaint(LPPAINTSTRUCT lpPaint)
{
	return ::BeginPaint(m_hWnd, lpPaint);
}

BOOL TTCWnd::EndPaint(LPPAINTSTRUCT lpPaint)
{
	return ::EndPaint(m_hWnd, lpPaint);
}

LRESULT TTCWnd::DefWindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_WindowUnicode && pDefWindowProcW != NULL) {
		// Unicode API あり && Unicode Window
		return pDefWindowProcW(m_hWnd, msg, wParam, lParam);
	}
	else {
		return ::DefWindowProcA(m_hWnd, msg, wParam, lParam);
	}
}

////////////////////////////////////////

TTCDialog *TTCDialog::pseudoPtr;

TTCDialog::TTCDialog()
{
}

TTCDialog::~TTCDialog()
{
}

/**
 * ダイアログ初期化
 * @retval	TRUE	処理が行われた(次のメッセージ処理は呼び出されない)
 * @retval	FALSE	次のメッセージ処理は呼び出される
 *
 * 次のメッセージ処理
 *		TTCDialog::DlgProc(msg=WM_INITDIALOG)
 */
BOOL TTCDialog::OnInitDialog()
{
	return FALSE;
}

/**
 * OKボタン
 * @retval	TRUE	処理が行われた(次のメッセージ処理は呼び出されない)
 * @retval	FALSE	次のメッセージ処理は呼び出される
 *
 * 次のメッセージ処理
 *		TTCDialog::OnCommand()
 */
BOOL TTCDialog::OnOK()
{
	EndDialog(IDOK);
	return TRUE;
}

/**
 * CANCELボタン
 * @retval	TRUE	処理が行われた(次のメッセージ処理は呼び出されない)
 * @retval	FALSE	次のメッセージ処理は呼び出される
 *
 * 次のメッセージ処理
 *		TTCDialog::OnCommand()
 */
BOOL TTCDialog::OnCancel()
{
	EndDialog(IDCANCEL);
	return TRUE;
}

BOOL TTCDialog::OnCommand(WPARAM wp, LPARAM lp)
{
	return FALSE;
}

/**
 * WM_CLOSEメッセージ処理
 * @retval	TRUE	処理が行われた(次のメッセージ処理は呼び出されない)
 * @retval	FALSE	次のメッセージ処理は呼び出される
 *
 * 次のメッセージ処理
 *		TTCDialog::OnCancel()
 */
BOOL TTCDialog::OnClose()
{
	return FALSE;
}

/**
 * WM_NCDESTROYメッセージ処理
 * @retval	TRUE	処理が行われた(次のメッセージ処理は呼び出されない)
 * @retval	FALSE	次のメッセージ処理は呼び出される
 *
 * 次のメッセージ処理
 *		TTCDialog::DlgProc()
 */
BOOL TTCDialog::PostNcDestroy()
{
	return FALSE;
}

/*
 * @retval	TRUE	メッセージを処理した時
 * @retval	FALSE	メッセージを処理しなかった時
 * @retval	その他	メッセージによって異なることがある
 */
LRESULT TTCDialog::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	(void)msg;
	(void)wp;
	(void)lp;
	return (LRESULT)FALSE;
}

/*
 * @retval	TRUE	メッセージを処理した時
 * @retval	FALSE	メッセージを処理しなかった時
 */
LRESULT TTCDialog::DlgProcBase(UINT msg, WPARAM wp, LPARAM lp)
{
	BOOL Processed = FALSE;
	switch (msg) {
	case WM_INITDIALOG:
		Processed = OnInitDialog();
		break;
	case WM_COMMAND:
	{
		const WORD wID = GET_WM_COMMAND_ID(wp, lp);
		switch (wID) {
		case IDOK:
			Processed = OnOK();
			//self->DestroyWindow();
			//SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, lResult)
			// return TRUE
			break;
		case IDCANCEL:
			Processed = OnCancel();
			break;
		}

		if (Processed == FALSE) {
			Processed = OnCommand(wp, lp);
		}
		break;
	}
	case WM_NCDESTROY:
		Processed = PostNcDestroy();
		break;
	case WM_CLOSE:
		// CLOSEボタンを押した時、
		Processed = OnClose();
		if (Processed == FALSE) {
			//	オーバーライドされていなければ
			//	dialogならOnCancel()が発生する
			Processed = OnCancel();
		}
		break;
	default:
		Processed = FALSE;
		break;
	}

	if (Processed == TRUE) {
		// 処理した
		return TRUE;
	}

	// 最後のメッセージ処理
	return DlgProc(msg, wp, lp);
}

LRESULT TTCDialog::WndProcBase(UINT msg, WPARAM wp, LPARAM lp)
{
	BOOL Processed = FALSE;
	switch (msg) {
	case WM_INITDIALOG:
		Processed = OnInitDialog();
		break;
	case WM_COMMAND:
	{
		const WORD wID = GET_WM_COMMAND_ID(wp, lp);
		switch (wID) {
		case IDOK:
			Processed = OnOK();
			//self->DestroyWindow();
			//SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, lResult)
			// return TRUE
			break;
		case IDCANCEL:
			Processed = OnCancel();
			break;
		}

		if (Processed == FALSE) {
			Processed = OnCommand(wp, lp);
		}
		break;
	}
	case WM_NCDESTROY:
		Processed = PostNcDestroy();
		break;
	case WM_CLOSE:
		// CLOSEボタンを押した時、
		Processed = OnClose();
		if (Processed == FALSE) {
			//	オーバーライドされていなければ
			//	dialogならOnCancel()が発生する
			Processed = OnCancel();
		}
		break;
	default:
		Processed = FALSE;
		break;
	}

	if (Processed == TRUE) {
		// 処理した
		return TRUE;
	}

	// DlgProcをオーバライドするのではなく、
	// DefWindowProcをオーバーライドすること
	return DefWindowProc(msg, wp, lp);
}

/**
 * for modal dialog
 */
INT_PTR TTCDialog::DoModal(HINSTANCE hInstance, HWND hParent, int idd)
{
	m_hInst = hInstance;
	m_hParentWnd = hParent;
	pseudoPtr = this;
#if defined(REWRITE_TEMPLATE)
	INT_PTR result =
		TTDialogBoxParam(hInstance,
						 MAKEINTRESOURCE(idd),
						 hParent,
						 &DlgProcStub, (LPARAM)this);
#else
	INT_PTR result =
		DialogBoxParam(hInstance,
					   MAKEINTRESOURCE(idd),
					   hParent,
					   &DlgProcStub, (LPARAM)this);
#endif
	pseudoPtr = nullptr;
	return result;
}

/**
 * for modeless dialog
 */
BOOL TTCDialog::Create(HINSTANCE hInstance, HWND hParent, int idd)
{
	m_hInst = hInstance;
	m_hParentWnd = hParent;
#if defined(REWRITE_TEMPLATE)
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, MAKEINTRESOURCE(idd));
#else
	HRSRC hResource = ::FindResource(hInstance, MAKEINTRESOURCE(idd), RT_DIALOG);
	HANDLE hDlgTemplate = ::LoadResource(hInstance, hResource);
	DLGTEMPLATE *lpTemplate = (DLGTEMPLATE *)::LockResource(hDlgTemplate);
#endif
	DLGPROC dlgproc = DlgProcStub;
	const wchar_t *dialog_class = TTGetClassName(lpTemplate);
	if (dialog_class != nullptr) {
		// Modaless Dialog & Dialog application
		//  WNDCLASS.lpfnWndProc = TTCDialog::WndProcBase
		dlgproc = nullptr;
	}
	pseudoPtr = this;
	HWND hWnd = _CreateDialogIndirectParamW(
		hInstance, lpTemplate, hParent,
		dlgproc, (LPARAM)this);
	pseudoPtr = nullptr;
#if defined(REWRITE_TEMPLATE)
	free(lpTemplate);
#endif
	if (hWnd == nullptr)
	{
		assert(false);
		return FALSE;
	}
	m_hWnd = hWnd;

	return TRUE;
}

/*
 * @retval	TRUE	メッセージを処理した時
 * @retval	FALSE	メッセージを処理しなかった時
 */
INT_PTR CALLBACK TTCDialog::DlgProcStub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TTCDialog *self = (TTCDialog *)::GetWindowLongPtr(hWnd, DWLP_USER);
	if (self == nullptr) {
		self = pseudoPtr;
		self->m_hWnd = hWnd;
		if (msg == WM_INITDIALOG) {
			::SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)self);
			pseudoPtr = nullptr;
		}
	}
	assert(self != nullptr);

	LRESULT result = self->DlgProcBase(msg, wp, lp);
	return result;
}

/*
 * @retval	TRUE	メッセージを処理した時
 * @retval	FALSE	メッセージを処理しなかった時
 */
LRESULT CALLBACK TTCDialog::WndProcStub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TTCDialog *self = (TTCDialog *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (self == nullptr) {
		assert(pseudoPtr != nullptr);
		self = pseudoPtr;
		self->m_hWnd = hWnd;
		if (msg == WM_CREATE) {
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
			pseudoPtr = nullptr;
		}
	}
	assert(self != nullptr);

	return self->WndProcBase(msg, wp, lp);
}

void TTCDialog::DestroyWindow()
{
	if (m_hWnd != nullptr) {
		HWND hWnd;
		if (m_hParentWnd != nullptr) {
			::EnableWindow(m_hParentWnd,TRUE);
			::SetFocus(m_hParentWnd);
		}
		hWnd = m_hWnd;
		m_hWnd = nullptr;
		::DestroyWindow(hWnd);
	}
}

#if 0
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
#endif
