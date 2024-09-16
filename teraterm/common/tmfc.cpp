/*
 * Copyright (C) 2018- TeraTerm Project
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

// �e���v���[�g�̏����������s��
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
	return ::SendDlgItemMessageW(m_hWnd, id, msg, wp, lp);
}

LRESULT TTCWnd::SendDlgItemMessageA(int id, UINT msg, WPARAM wp, LPARAM lp)
{
	return ::SendDlgItemMessageA(m_hWnd, id, msg, wp, lp);
}

void TTCWnd::GetDlgItemTextW(int id, wchar_t *buf, size_t size)
{
	::GetDlgItemTextW(m_hWnd, id, buf, (int)size);
}

void TTCWnd::GetDlgItemTextA(int id, char *buf, size_t size)
{
	::GetDlgItemTextA(m_hWnd, id, buf, (int)size);
}

void TTCWnd::SetDlgItemTextW(int id, const wchar_t *str)
{
	::SetDlgItemTextW(m_hWnd, id, str);
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
	::SetWindowTextW(m_hWnd, str);
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
	return ::DefWindowProcW(m_hWnd, msg, wParam, lParam);
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
 * �_�C�A���O������
 * @retval	TRUE	�������s��ꂽ(���̃��b�Z�[�W�����͌Ăяo����Ȃ�)
 * @retval	FALSE	���̃��b�Z�[�W�����͌Ăяo�����
 *
 * ���̃��b�Z�[�W����
 *		TTCDialog::DlgProc(msg=WM_INITDIALOG)
 */
BOOL TTCDialog::OnInitDialog()
{
	return FALSE;
}

/**
 * OK�{�^��
 * @retval	TRUE	�������s��ꂽ(���̃��b�Z�[�W�����͌Ăяo����Ȃ�)
 * @retval	FALSE	���̃��b�Z�[�W�����͌Ăяo�����
 *
 * ���̃��b�Z�[�W����
 *		TTCDialog::OnCommand()
 */
BOOL TTCDialog::OnOK()
{
	EndDialog(IDOK);
	return TRUE;
}

/**
 * CANCEL�{�^��
 * @retval	TRUE	�������s��ꂽ(���̃��b�Z�[�W�����͌Ăяo����Ȃ�)
 * @retval	FALSE	���̃��b�Z�[�W�����͌Ăяo�����
 *
 * ���̃��b�Z�[�W����
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
 * WM_CLOSE���b�Z�[�W����
 * @retval	TRUE	�������s��ꂽ(���̃��b�Z�[�W�����͌Ăяo����Ȃ�)
 * @retval	FALSE	���̃��b�Z�[�W�����͌Ăяo�����
 *
 * ���̃��b�Z�[�W����
 *		TTCDialog::OnCancel()
 */
BOOL TTCDialog::OnClose()
{
	return FALSE;
}

/**
 * WM_NCDESTROY���b�Z�[�W����
 * @retval	TRUE	�������s��ꂽ(���̃��b�Z�[�W�����͌Ăяo����Ȃ�)
 * @retval	FALSE	���̃��b�Z�[�W�����͌Ăяo�����
 *
 * ���̃��b�Z�[�W����
 *		TTCDialog::DlgProc()
 */
BOOL TTCDialog::PostNcDestroy()
{
	return FALSE;
}

/*
 * @retval	TRUE	���b�Z�[�W������������
 * @retval	FALSE	���b�Z�[�W���������Ȃ�������
 * @retval	���̑�	���b�Z�[�W�ɂ���ĈقȂ邱�Ƃ�����
 */
LRESULT TTCDialog::DlgProc(UINT msg, WPARAM wp, LPARAM lp)
{
	(void)msg;
	(void)wp;
	(void)lp;
	return (LRESULT)FALSE;
}

/*
 * @retval	TRUE	���b�Z�[�W������������
 * @retval	FALSE	���b�Z�[�W���������Ȃ�������
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
		// CLOSE�{�^�������������A
		Processed = OnClose();
		if (Processed == FALSE) {
			//	�I�[�o�[���C�h����Ă��Ȃ����
			//	dialog�Ȃ�OnCancel()����������
			Processed = OnCancel();
		}
		break;
	default:
		Processed = FALSE;
		break;
	}

	if (Processed == TRUE) {
		// ��������
		return TRUE;
	}

	// �Ō�̃��b�Z�[�W����
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
		// CLOSE�{�^�������������A
		Processed = OnClose();
		if (Processed == FALSE) {
			//	�I�[�o�[���C�h����Ă��Ȃ����
			//	dialog�Ȃ�OnCancel()����������
			Processed = OnCancel();
		}
		break;
	default:
		Processed = FALSE;
		break;
	}

	if (Processed == TRUE) {
		// ��������
		return TRUE;
	}

	// DlgProc���I�[�o���C�h����̂ł͂Ȃ��A
	// DefWindowProc���I�[�o�[���C�h���邱��
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
						 MAKEINTRESOURCEW(idd),
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
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, MAKEINTRESOURCEW(idd));
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
	HWND hWnd = CreateDialogIndirectParamW(
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
 * @retval	TRUE	���b�Z�[�W������������
 * @retval	FALSE	���b�Z�[�W���������Ȃ�������
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
 * @retval	TRUE	���b�Z�[�W������������
 * @retval	FALSE	���b�Z�[�W���������Ȃ�������
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
