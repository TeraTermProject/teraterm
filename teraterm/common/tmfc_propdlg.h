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

#include <windows.h>

#pragma once

// Property Sheet
class TTCPropSheetDlg
{
public:
	TTCPropSheetDlg(HINSTANCE hInstance, HWND hParentWnd, const wchar_t *uilangfile);
	virtual ~TTCPropSheetDlg();
	INT_PTR DoModal();
	void AddPage(HPROPSHEETPAGE page);
	void SetCaption(const wchar_t *caption);

private:
	static int CALLBACK PropSheetProc(HWND hWnd, UINT msg, LPARAM lParam);
	static LRESULT CALLBACK WndProcStatic(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static HINSTANCE ghInstance;
	static class TTCPropSheetDlg *gTTCPS;
	LRESULT CALLBACK WndProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

	PROPSHEETHEADERW m_psh;
	HWND m_hWnd;
	HWND m_hParentWnd;
	HINSTANCE m_hInst;

	int m_PageCount;
	HPROPSHEETPAGE hPsp[9];

	LONG_PTR m_OrgProc;
	LONG_PTR m_OrgUserData;

	wchar_t *m_UiLanguageFile;
};
