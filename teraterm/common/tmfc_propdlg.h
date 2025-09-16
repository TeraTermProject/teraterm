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

#pragma once

#include <windows.h>
#include <commctrl.h>	// for HTREEITEM

// Property Sheet
class TTCPropSheetDlg
{
public:
	TTCPropSheetDlg(HINSTANCE hInstance, HWND hParentWnd, const wchar_t *uilangfile);
	virtual ~TTCPropSheetDlg();
	INT_PTR DoModal();
	void AddPage(HPROPSHEETPAGE page, const wchar_t *path = NULL);
	void SetCaption(const wchar_t *caption);
	void SetTreeViewMode(BOOL enable);
	static void SetTreeViewModeInit(BOOL enable);
	void SetStartPage(int start);

private:
	HWND m_hWnd;
	HWND m_hParentWnd;
	HINSTANCE m_hInst;
	wchar_t *m_UiLanguageFile;
	int m_StartPage;

	static int CALLBACK PropSheetProc(HWND hWnd, UINT msg, LPARAM lParam);
	static LRESULT CALLBACK WndProcStatic(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
	static class TTCPropSheetDlg *gTTCPS;
	LRESULT CALLBACK WndProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
	LONG_PTR m_OrgProc;
	LONG_PTR m_OrgUserData;

	PROPSHEETHEADERW m_psh;
	int m_PageCount;
	struct TTPropSheetPage {
		HPROPSHEETPAGE hPsp;
		wchar_t *path;
	};
	TTPropSheetPage *m_Page;

	// tree control
	BOOL m_TreeView;
	HWND m_hWndTV;
	void AddTreeControl();
	void CreateTree(HWND dlg);
	HTREEITEM CreatePath(const wchar_t *path, HTREEITEM hParent, int data);
	HTREEITEM GetTreeItem(int nPage, HTREEITEM hParent);

	// 初期値
	static BOOL m_TreeViewInit;
};
