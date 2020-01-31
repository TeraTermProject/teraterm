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
#include "ttlib.h"
#include "compat_win.h"
#include "layer_for_unicode.h"

#if (defined(_MSC_VER) && (_MSC_VER <= 1500)) || \
	(__cplusplus <= 199711L)
#define nullptr NULL	// C++11,nullptr / > VS2010
#endif

const RECT TTCFrameWnd::rectDefault =
{
	CW_USEDEFAULT, CW_USEDEFAULT,
//	2*CW_USEDEFAULT, 2*CW_USEDEFAULTg
	0, 0
};

TTCFrameWnd::TTCFrameWnd()
{
}

TTCFrameWnd::~TTCFrameWnd()
{
}

BOOL TTCFrameWnd::CreateA(
	HINSTANCE hInstance,
	LPCSTR lpszClassName,
	LPCSTR lpszWindowName,
	DWORD dwStyle,
	const RECT& rect,
	HWND hParentWnd,
	LPCTSTR lpszMenuName,
	DWORD dwExStyle)
{
	m_hInst = hInstance;
	m_hParentWnd = hParentWnd;
	pseudoPtr = this;
	HWND hWnd = ::CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		lpszClassName,
		lpszWindowName,
		dwStyle,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		hParentWnd,
		nullptr,
		hInstance,
		nullptr);
	pseudoPtr = nullptr;
	if (hWnd == nullptr) {
		OutputDebugPrintf("CreateWindow %d\n", GetLastError());
		return FALSE;
	} else {
		m_hWnd = hWnd;
		SetWindowLongPtr(GWLP_USERDATA, (LONG_PTR)this);
		return TRUE;
	}
}

BOOL TTCFrameWnd::CreateW(
	HINSTANCE hInstance,
	LPCWSTR lpszClassName,
	LPCWSTR lpszWindowName,
	DWORD dwStyle,
	const RECT& rect,
	HWND hParentWnd,
	LPCTSTR lpszMenuName,
	DWORD dwExStyle)
{
	m_hInst = hInstance;
	m_hParentWnd = hParentWnd;
	pseudoPtr = this;
	HWND hWnd = _CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,
		lpszClassName,
		lpszWindowName,
		dwStyle,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		hParentWnd,
		nullptr,
		hInstance,
		nullptr);
	pseudoPtr = nullptr;
	if (hWnd == nullptr) {
		OutputDebugPrintf("CreateWindow %d\n", GetLastError());
		return FALSE;
	} else {
		m_hWnd = hWnd;
		if (pCreateWindowExW != NULL) {
			// Unicode API‚ª‘¶Ý‚·‚é
			m_WindowUnicode = TRUE;
		}
		SetWindowLongPtr(GWLP_USERDATA, (LONG_PTR)this);
		return TRUE;
	}
}

TTCFrameWnd *TTCFrameWnd::pseudoPtr;

LRESULT TTCFrameWnd::ProcStub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TTCFrameWnd *self;
	if (pseudoPtr != nullptr) {
		self = pseudoPtr;
		self->m_hWnd = hWnd;
	} else {
		self = (TTCFrameWnd *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	}
	return self->Proc(msg, wp, lp);
}

BOOL TTCFrameWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

void TTCFrameWnd::OnKillFocus(HWND hNewWnd)
{}

void TTCFrameWnd::OnSetFocus(HWND hOldWnd)
{}

void TTCFrameWnd::OnSysCommand(WPARAM nID, LPARAM lParam)
{}

void TTCFrameWnd::OnSysKeyDown(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	DefWindowProc(WM_SYSKEYDOWN, nChar, MAKELONG(nRepCnt, nFlags));
}

void TTCFrameWnd::OnSysKeyUp(WPARAM nChar, UINT nRepCnt, UINT nFlags)
{
	DefWindowProc(WM_SYSKEYUP, nChar, MAKELONG(nRepCnt, nFlags));
}

void TTCFrameWnd::OnClose()
{}
