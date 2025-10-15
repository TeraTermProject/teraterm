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

/* TERATERM.EXE, print-abort dialog box */
#pragma once

#include "tttypes.h"	// for TTSet

// CPrnAbortDlg dialog
class CPrnAbortDlg
{
public:
	CPrnAbortDlg();
	BOOL Create(HINSTANCE hInstance, HWND hParent, PBOOL AbortFlag, PTTSet pts);
	BOOL DestroyWindow();
	int SetPrintDC(HDC hPrintDC);
	BOOL IsAborted();

private:
	HWND m_hWnd;
	HWND m_hParentWnd;
	BOOL *m_pAbort;
	TTTSet *m_ts;
	BOOL m_PrintAbortFlag;
	static HWND m_hWnd_static;

	BOOL AbortProc(HDC PDC, int Code);
	static INT_PTR CALLBACK OnDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
	static BOOL CALLBACK AbortProcStatic(HDC hDC, int Error);
};
