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
#pragma once
#include <windows.h>

class TTCWnd
{
public:
	HWND m_hWnd;
	HINSTANCE m_hInst;
	HACCEL m_hAccel;

	TTCWnd();
	void DestroyWindow();
	HWND GetSafeHwnd() const {return m_hWnd;}
	HWND GetSafeHwnd() { return m_hWnd; }
	HDC BeginPaint(LPPAINTSTRUCT lpPaint);
	BOOL EndPaint(LPPAINTSTRUCT lpPaint);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT SendMessage(UINT msg, WPARAM wp, LPARAM lp);
	LRESULT SendDlgItemMessage(int id, UINT msg, WPARAM wp, LPARAM lp);
	void GetDlgItemText(int id, TCHAR *buf, size_t size);
	void SetDlgItemText(int id, const TCHAR *str);
	void SetCheck(int id, int nCheck);
	UINT GetCheck(int id);
	void SetCurSel(int id, int no);
	int GetCurSel(int id);
	void EnableDlgItem(int id, BOOL enable);
	void SetDlgItemInt(int id, UINT val, BOOL bSigned = TRUE);
	UINT GetDlgItemInt(int id, BOOL* lpTrans = NULL, BOOL bSigned = TRUE) const;
	void ShowWindow(int nCmdShow);
	void SetWindowText(const TCHAR *str);
	void ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
	void ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
	int MessageBox(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
};

class TTCFrameWnd : public TTCWnd
{
public:
	TTCFrameWnd();
	virtual ~TTCFrameWnd();
	static TTCFrameWnd *pseudoPtr;
	static LRESULT CALLBACK ProcStub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	virtual BOOL Create(HINSTANCE hInstance,
						LPCTSTR lpszClassName,
						LPCTSTR lpszWindowName,
						DWORD dwStyle = WS_OVERLAPPEDWINDOW,
						const RECT& rect = rectDefault,
						HWND pParentWnd = NULL,        // != NULL for popups
						LPCTSTR lpszMenuName = NULL,
						DWORD dwExStyle = 0);//,
						//CCreateContext* pContext = NULL);
	virtual LRESULT Proc(UINT msg, WPARAM wp, LPARAM lp);
	static const RECT rectDefault;
	///
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	///
#if 1
	void OnKillFocus(HWND hNewWnd);
	void OnDestroy();
	void OnSetFocus(HWND hOldWnd);
	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnClose();
#endif
	void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
};

class TTCDialog : public TTCWnd
{
public:
	TTCDialog();
	virtual ~TTCDialog();
	static TTCDialog *pseudoPtr;
	static LRESULT CALLBACK ProcStub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	BOOL Create(HINSTANCE hInstance, HWND hParent, int idd);
	void DestroyWindow();
	virtual void OnInitDialog();
	virtual	void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();
	virtual BOOL OnCommand(WPARAM wp, LPARAM lp);
	HWND m_hParentWnd;

private:
	int DoModal();

	static LRESULT CALLBACK OnDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
};

class TTCPropertySheet
{
public:
	TTCPropertySheet(HINSTANCE hInstance, LPCTSTR pszCaption, HWND hParentWnd);
	virtual ~TTCPropertySheet();
	virtual void OnInitDialog();
	int DoModal();
	PROPSHEETHEADER m_psh;
	HWND m_hWnd;
	static int CALLBACK PropSheetProc(HWND hWnd, UINT msg, LPARAM lParam);
	HINSTANCE m_hInst;
};

class TTCPropertyPage : public TTCWnd
{
public:
	TTCPropertyPage(HINSTANCE inst, int id, TTCPropertySheet *sheet);
	virtual ~TTCPropertyPage();
	virtual void OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wp, LPARAM lp);
	virtual HBRUSH OnCtlColor(HDC hDC, HWND hWnd);

	PROPSHEETPAGE m_psp;
	static BOOL CALLBACK Proc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
	static UINT CALLBACK PropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
	TTCPropertySheet *m_pSheet;
};

