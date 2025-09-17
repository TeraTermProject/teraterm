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
#include <commctrl.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <wchar.h>

#include "ttlib.h"
#include "dlglib.h"

#include "tmfc_propdlg.h"

#define REWRITE_TEMPLATE	1
#define TREE_WIDTH 200
#define CONTROL_SPACE 5

BOOL TTCPropSheetDlg::m_TreeViewInit = FALSE;

// quick hack :-(
class TTCPropSheetDlg* TTCPropSheetDlg::gTTCPS;

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
	m_Page = NULL;
	m_PageCount = 0;
	m_StartPage = 0;
	m_TreeView = m_TreeViewInit;
}

TTCPropSheetDlg::~TTCPropSheetDlg()
{
	free((void*)m_psh.pszCaption);
	free(m_UiLanguageFile);

	for (int i = 0; i < m_PageCount; i++) {
		free(m_Page[i].path);
		m_Page[i].path = NULL;
	}
	free(m_Page);
}

void TTCPropSheetDlg::SetTreeViewModeInit(BOOL enable)
{
	m_TreeViewInit = enable;
}

void TTCPropSheetDlg::SetTreeViewMode(BOOL enable)
{
	m_TreeView = enable;
}

void TTCPropSheetDlg::AddPage(HPROPSHEETPAGE hpage, const wchar_t *path)
{
	TTPropSheetPage *p =
		(TTPropSheetPage *)realloc(m_Page, sizeof(TTPropSheetPage) * (m_PageCount + 1));
	if (p == NULL) {
		return;
	}
	m_Page = p;
	m_Page[m_PageCount].hPsp = hpage;
	m_Page[m_PageCount].path = (path == NULL) ? NULL : _wcsdup(path);
	m_PageCount++;
}

void TTCPropSheetDlg::SetCaption(const wchar_t* caption)
{
	free((void*)m_psh.pszCaption);
	m_psh.pszCaption = _wcsdup(caption);
}

void TTCPropSheetDlg::SetStartPage(int start)
{
	m_StartPage = start;
}

INT_PTR TTCPropSheetDlg::DoModal()
{
	INT_PTR result;
	int i;
	HPROPSHEETPAGE *hPsp = (HPROPSHEETPAGE *)malloc(sizeof(HPROPSHEETPAGE) * m_PageCount);
	for(i = 0; i < m_PageCount; i++) {
		hPsp[i] = m_Page[i].hPsp;
	}
	m_psh.nPages = m_PageCount;
	m_psh.phpage = hPsp;
	m_psh.nStartPage = m_StartPage;
	gTTCPS = this;
	result = PropertySheetW(&m_psh);
	free(hPsp);
	return result;
}

LRESULT CALLBACK TTCPropSheetDlg::WndProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg){
	case WM_INITDIALOG:
	case WM_SHOWWINDOW: {
		if (m_TreeView) {
			AddTreeControl();
		}
		CenterWindow(dlg, m_hParentWnd);
		break;
		}
	case WM_NOTIFY: {
		NMHDR *nmhdr = (NMHDR *)lParam;
		switch(nmhdr->code) {
#if 0
		// TVN_SELCHANGED があれば不要、そのうち消す
		case TVN_SELCHANGINGW:
		case TVN_SELCHANGING: {
			NMTREEVIEWW *pnmtv = (LPNMTREEVIEWW)lParam;
			TVITEMW item;
			item.mask = TVIF_PARAM;
			item.hItem = pnmtv->itemNew.hItem;
			SendMessageW(m_hWndTV, TVM_GETITEMW, 0, (LPARAM)&item);
			WPARAM page_index = item.lParam;
			SendMessageW(m_hWnd, PSM_SETCURSEL, page_index, 0);
			break;
		}
#endif
		case TVN_SELCHANGEDW:
		case TVN_SELCHANGED: {
			// ツリービューが選択された、タブを切り替える
			NMTREEVIEWW *pnmtv = (LPNMTREEVIEWW)lParam;
			TVITEMW item;
			item.mask = TVIF_PARAM;
			item.hItem = pnmtv->itemNew.hItem;
			SendMessageW(m_hWndTV, TVM_GETITEMW, 0, (LPARAM)&item);
			WPARAM page_index = item.lParam;
			SendMessageW(m_hWnd, PSM_SETCURSEL, page_index, 0);
			break;
		}
		case TCN_SELCHANGE: {
			// タブが切り替わるときに発生、ツリービューと同期
			//		タブをマウスでクリック、
			// 		タブにフォーカスがあるときに左右キー
			//		CTRL+PgUp,PgDn や CTRL+TABでは発生しない?? TCN_KEYDOWNで処理が必要?
			HWND hTab = PropSheet_GetTabControl(dlg);
			int cur_sel = TabCtrl_GetCurSel(hTab);
			HTREEITEM item = GetTreeItem(cur_sel, TVI_ROOT);
			if (TreeView_GetSelection(m_hWndTV) != item) {
				TreeView_SelectItem(m_hWndTV, item);
			}
			break;
		}
		}
	}
	}
	SetWindowLongPtrW(dlg, GWLP_WNDPROC, m_OrgProc);
	SetWindowLongPtrW(dlg, GWLP_USERDATA, m_OrgUserData);
	LRESULT result = CallWindowProcW((WNDPROC)m_OrgProc, dlg, msg, wParam, lParam);
	m_OrgProc = SetWindowLongPtrW(dlg, GWLP_WNDPROC, (LONG_PTR)WndProcStatic);
	m_OrgUserData = SetWindowLongPtrW(dlg, GWLP_USERDATA, (LONG_PTR)this);

#if 0
	// 		?? CTRL+PgUp,PgDn ではこのメッセージは発生しない
	// そのうち消す
	switch(msg){
	case PSM_CHANGED:
	case PSM_SETCURSEL:
	case PSM_SETCURSELID: {
		if (m_TreeView) {
			HWND hTab = PropSheet_GetTabControl(dlg);
			int cur_sel = TabCtrl_GetCurSel(hTab);
			int a = 0;
		}

		break;
	}
	}
#endif
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
	case PSCB_PRECREATE: {
#if defined(REWRITE_TEMPLATE)
		// テンプレートの内容を書き換える
		// http://home.att.ne.jp/banana/akatsuki/doc/atlwtl/atlwtl15-01/index.html
		TTCPropSheetDlg*self = gTTCPS;
		HINSTANCE hInst = self->m_hInst;
		size_t PrevTemplSize;
		size_t NewTemplSize;
		DLGTEMPLATE *NewTempl =
			TTGetNewDlgTemplate(hInst, (DLGTEMPLATE *)lp,
								&PrevTemplSize, &NewTemplSize);
		NewTempl->style &= ~DS_CONTEXTHELP;		// check DLGTEMPLATEEX
		memcpy((void *)lp, NewTempl, NewTemplSize);
		free(NewTempl);
#endif
		break;
	}
	case PSCB_INITIALIZED: {
		static const DlgTextInfo TextInfos[] = {
			{ IDOK, "BTN_OK" },
			{ IDCANCEL, "BTN_CANCEL" },
			{ IDHELP, "BTN_HELP" },
		};
		static const int ID_APPLY_NOW = 0x3021;		// afxres.h で定義されている
		TTCPropSheetDlg*self = gTTCPS;
		self->m_hWnd = hWnd;
		SetDlgTextsW(hWnd, TextInfos, _countof(TextInfos), self->m_UiLanguageFile);
		SetDlgItemTextW(hWnd, ID_APPLY_NOW, L"");	// &A が使えるよう "&Apply" を削除する
		self->m_OrgProc = SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcStatic);
		self->m_OrgUserData = SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)self);
		break;
	}
	}
	return 0;
}

static void MoveChildWindows(HWND hWnd, int nDx, int nDy)
{
	HWND hChildWnd = GetWindow(hWnd, GW_CHILD);
	while (hChildWnd != NULL) {
		RECT rect;
		GetWindowRect(hChildWnd, &rect);
		int x = rect.left + nDx;
		int y = rect.top + nDy;
		POINT p;
		p.x = x;
		p.y = y;
		ScreenToClient(hWnd, &p);
		x = p.x;
		y = p.y;
		SetWindowPos(hChildWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

		hChildWnd = GetNextWindow(hChildWnd, GW_HWNDNEXT);
	}
}

HTREEITEM TTCPropSheetDlg::GetTreeItem(int nPage, HTREEITEM hParent)
{
	HTREEITEM hItem = TreeView_GetChild(m_hWndTV, hParent);
	while (hItem) {
		TVITEMW item;
		item.hItem = hItem;
		item.mask = TVIF_PARAM;
		TreeView_GetItem(m_hWndTV, &item);

		if (item.lParam == nPage) {
			// 見つかった
			return hItem;
		}

		// 子
		HTREEITEM hItemFound = GetTreeItem(nPage, hItem);
		if (hItemFound != NULL) {
			// 見つかった
			return hItemFound;
		}

		// 次
		hItem = TreeView_GetNextItem(m_hWndTV, hItem, TVGN_NEXT);
	}

	return NULL;
}

/**
 *	パスを作成して返す
 *		存在していたらそれを返す
 *	@param	path		L"path1/path2/path3"
 *	@param	HTREEITEM	呼び出し元はTVI_ROOT (再帰用)
 *	@param	page_index	作成したパスに設定するpage_index(0...)
 *	@return	HTREEITEM
 */
HTREEITEM TTCPropSheetDlg::CreatePath(const wchar_t *path, HTREEITEM hParent, int page_index)
{
	if (path == NULL || path[0] == 0) {
		return hParent;
	}

	wchar_t *path_cur = _wcsdup(path);
	wchar_t *path_next = NULL;
	wchar_t *p = wcschr(path_cur, L'/');
	if (p != NULL) {
		path_next = p + 1;
		*p = 0;
	}

	// パスが既に存在しているか?
	HTREEITEM hItem = TreeView_GetChild(m_hWndTV, hParent);
	while (hItem) {
		wchar_t text[MAX_PATH];
		TVITEMW item;
		item.hItem = hItem;
		item.mask = TVIF_TEXT;
		item.cchTextMax = _countof(text);
		item.pszText = text;
		// TreeView_GetItem(m_hWndTV, &item);
		SendMessageW(m_hWndTV, TVM_GETITEMW, 0, (LPARAM)&item);

		if (wcscmp(item.pszText, path_cur) ==0) {
			// 存在している
			if (path_next != NULL) {
				// 再帰、次を探す
				hItem = CreatePath(path_next, hItem, page_index);
			}
			free(path_cur);
			return hItem;
		}

		// 次
		hItem = TreeView_GetNextItem(m_hWndTV, hItem, TVGN_NEXT);
	}

	// 見つからない、作る
	TVINSERTSTRUCTW tvi;
	tvi.hParent = hParent;
	tvi.hInsertAfter = TVI_LAST;
	tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvi.item.pszText = (wchar_t *)path_cur;
	tvi.item.lParam = (LPARAM)page_index;
	//TreeView_InsertItem(m_hWndTV, &tvi);
	hItem = (HTREEITEM)SendMessageW(m_hWndTV, TVM_INSERTITEMW, 0, (LPARAM)&tvi);

	if (path_next != NULL) {
		// 再帰で探す(見つからず、作ることになる)
		hItem = CreatePath(path_next, hItem, page_index);
	}

	free(path_cur);
	return hItem;
}

void TTCPropSheetDlg::CreateTree(HWND dlg)
{
	HWND hTab = PropSheet_GetTabControl(dlg);
	const int nPageCount = TabCtrl_GetItemCount(hTab);

	// シート(木の葉)を追加する
	for (int i = 0; i < nPageCount; i++) {
		// ページのタイトル取得
		wchar_t page_title[MAX_PATH];
		TCITEMW	ti;
		ZeroMemory(&ti, sizeof(ti));
		ti.mask = TCIF_TEXT;
		ti.cchTextMax = _countof(page_title);
		ti.pszText = page_title;
		//TabCtrl_GetItem(hTab, i, &ti);
		SendMessageW(hTab, TCM_GETITEMW, (WPARAM)i, (LPARAM)&ti);

		// パス(枝)を作る
		wchar_t *path = m_Page[i].path;
		HTREEITEM hItem = CreatePath(path, TVI_ROOT, i);

		// シートを追加する
		TVINSERTSTRUCTW tvi;
		tvi.hParent = hItem;
		tvi.hInsertAfter = TVI_LAST;
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
		tvi.item.pszText = page_title;
		tvi.item.lParam = (LPARAM)i;
		//hItem = TreeView_InsertItem(m_hWndTV, &tvi);
		hItem = (HTREEITEM)SendMessageW(m_hWndTV, TVM_INSERTITEMW, 0, (LPARAM)&tvi);
	}
}

void TTCPropSheetDlg::AddTreeControl()
{
	HWND hTab = PropSheet_GetTabControl(m_hWnd);

	// ツリーで選択できるのでタブは1行設定にする
	SetWindowLongPtr(hTab, GWL_STYLE, GetWindowLongPtr(hTab, GWL_STYLE) & ~TCS_MULTILINE);

#if 0
	// タブを消して領域を移動しようとしたがうまくいかなかった
	// そのうちこのブロックは消す
	const bool m_HideTab = true;
	if (m_HideTab) {
		// タブ部分を隠す
		ShowWindow(hTab, SW_HIDE);
		EnableWindow(hTab, FALSE);

		// タブ部分のサイズ(高さ)
		RECT item_rect;
		TabCtrl_GetItemRect(hTab, 0, &item_rect);
		const int item_height = item_rect.bottom - item_rect.top;

		// tab controlのタブ分、ダイアログを小さくする
		RECT dlg_rect;
		GetWindowRect(m_hWnd, &dlg_rect);
		int w = dlg_rect.right - dlg_rect.left;
		int h = dlg_rect.bottom - dlg_rect.top - item_height;
		SetWindowPos(m_hWnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);

		// タブコントロールをタブ分上に移動する
		RECT tab_rect;
		GetWindowRect(hTab, &tab_rect);

		POINT start;
		start.x = tab_rect.left;
		start.y = tab_rect.top - item_height;
		ScreenToClient(m_hWnd, &start);
		SetWindowPos(hTab, NULL, start.x, start.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}
#endif

	// ツリーコントロールの位置を決める
	RECT tree_rect;
	GetWindowRect(hTab, &tree_rect);
 	int tree_w = TREE_WIDTH;
	int tree_h = tree_rect.bottom - tree_rect.top;
	POINT pt;
	pt.x = tree_rect.left;
	pt.y = tree_rect.top;
	ScreenToClient(m_hWnd, &pt);

	// ツリーコントロール分ダイアログのサイズを大きくする
	RECT dlg_rect;
	GetWindowRect(m_hWnd, &dlg_rect);
	int w = dlg_rect.right - dlg_rect.left + TREE_WIDTH + CONTROL_SPACE;
	int h = dlg_rect.bottom - dlg_rect.top;
	SetWindowPos(m_hWnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);

	// ダイアログのサイズを大きくした分、コントロールを移動する
	MoveChildWindows(m_hWnd, TREE_WIDTH + CONTROL_SPACE, 0);

	const DWORD	dwTreeStyle =
		TVS_SHOWSELALWAYS|TVS_TRACKSELECT|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS;
	m_hWndTV = CreateWindowExW(0,
							   WC_TREEVIEWW,
							   L"Tree View",
							   WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | dwTreeStyle,
							   pt.x,
							   pt.y,
							   tree_w,
							   tree_h,
							   m_hWnd,
							   NULL,
							   m_hInst,
							   NULL);
	CreateTree(m_hWnd);
}
