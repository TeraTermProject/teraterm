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

// ツリーをマウスでクリックしたとき、ページ側へフォーカスを移すための内部メッセージ
#define WM_TTCPS_SET_PAGE_FOCUS		(WM_APP + 1)

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
	m_ShowTab = FALSE;
	m_hWndTV = NULL;
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

/**
 *	ツリー表示時にタブを表示するか設定する
 *
 *	既定は FALSE(隠す)。ツリーでページを選べるのでタブは無くてよく、
 *	隠した分はページ領域に使う。TRUE にするとツリーとタブが両方出る。
 *	ツリー非表示(SetTreeViewMode(FALSE))のときはタブでしかページを
 *	選べないため、この設定に関わらずタブは表示される。
 *
 *	DoModal() より前に呼ぶこと。
 */
void TTCPropSheetDlg::SetTabMode(BOOL enable)
{
	m_ShowTab = enable;
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
	case WM_TTCPS_SET_PAGE_FOCUS: {
		// ツリーのクリックで切り替えたページへフォーカスを移す。
		// ページはダイアログなので SetFocus(hPage) でページ内のコントロールへ
		// フォーカスが移る。comctl32 はページをアクティブにするたびフォーカスを
		// 先頭の TABSTOP へリセットするため、常にそのページの最初のコントロールに
		// なる(前回操作していたコントロールには戻らない)。
		HWND hPage = PropSheet_GetCurrentPageHwnd(m_hWnd);
		if (hPage != NULL) {
			SetFocus(hPage);
			// フォーカス枠を表示する。
			// マウス操作で始まった場合、Windows はフォーカス枠を抑止する
			// (UISF_HIDEFOCUS)。ここはユーザーがクリックしていないコントロールへ
			// フォーカスを移すので、抑止されたままだと移動先が分からない。
			// エディットはキャレットで分かるが、チェックボックスは枠が出ないと
			// フォーカスの有無が全く見えない(TCP/IP ページなど)。
			// WM_UPDATEUISTATE はページとその子コントロールへ伝播する。
			SendMessageW(hPage, WM_UPDATEUISTATE,
						 MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);
		}
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
			// PSM_SETCURSEL はページをアクティブするときに、
			// そのページ内のコントロールへフォーカスを移す。
			//
			// つまり操作の種類に関わらず、
			// この時点でツリーからフォーカスが外れている。
			// タブのクリックから同期した場合は TVC_UNKNOWN になる。
			if (pnmtv->action == TVC_BYMOUSE) {
				// マウスでクリックしたときは、
				// クリック処理でツリーへフォーカスが移動する。
				// PostMessage でページ内へフォーカスを移動する
				PostMessageW(m_hWnd, WM_TTCPS_SET_PAGE_FOCUS, 0, 0);
			}
			else if (pnmtv->action == TVC_BYKEYBOARD) {
				// キーボードではツリーの操作を続けられるようフォーカスを戻す。
				// マウスのときと違い、この後ツリーへフォーカスが戻る処理は
				// 無いので、遅延させずここで戻す。
				SetFocus(m_hWndTV);
			}
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

/**
 *	タブ行を隠し、空いた分だけページ領域を上へ広げる
 *
 *	ページの位置はプロパティシートマネージャがタブコントロールの表示領域
 *	(TabCtrl_AdjustRect)から決めるので、タブコントロール自体を上へ広げて
 *	表示領域を広げる。タブコントロールは非表示にするので、はみ出したタブ行は
 *	描画されない。
 *
 *	既に生成済みのページはページ切り替えまで再配置されないため、
 *	現在のページだけはここで明示的に移動する。
 *
 *	@param	hTab	タブコントロール(PropSheet_GetTabControl の戻り値)
 */
void TTCPropSheetDlg::HideTab(HWND hTab)
{
	// タブ行 + 枠の高さ(タブコントロールの上端から表示領域の上端まで)
	RECT adjust_rect;
	SetRect(&adjust_rect, 0, 0, 0, 0);
	TabCtrl_AdjustRect(hTab, TRUE, &adjust_rect);	// 表示領域→ウィンドウ矩形
	const int top_margin = -adjust_rect.top;
	if (top_margin <= 0) {
		return;
	}

	// タブを隠す。EnableWindow(FALSE) はしない
	// (プロパティシートマネージャがタブの選択状態を参照するため)
	ShowWindow(hTab, SW_HIDE);

	// タブコントロールを上へ広げる。表示領域の下端は変えない
	RECT tab_rect;
	GetWindowRect(hTab, &tab_rect);
	POINT tab_pt;
	tab_pt.x = tab_rect.left;
	tab_pt.y = tab_rect.top - top_margin;
	ScreenToClient(m_hWnd, &tab_pt);
	SetWindowPos(hTab, NULL,
				 tab_pt.x, tab_pt.y,
				 tab_rect.right - tab_rect.left,
				 tab_rect.bottom - tab_rect.top + top_margin,
				 SWP_NOZORDER);

	// 生成済みのページを新しい表示領域へ移動する
	// (これをしないと、ページを切り替えるまで元の位置に残る)
	HWND hPage = PropSheet_GetCurrentPageHwnd(m_hWnd);
	if (hPage != NULL) {
		RECT page_rect;
		GetWindowRect(hTab, &page_rect);
		TabCtrl_AdjustRect(hTab, FALSE, &page_rect);	// ウィンドウ矩形→表示領域
		POINT page_pt;
		page_pt.x = page_rect.left;
		page_pt.y = page_rect.top;
		ScreenToClient(m_hWnd, &page_pt);
		SetWindowPos(hPage, NULL,
					 page_pt.x, page_pt.y,
					 page_rect.right - page_rect.left,
					 page_rect.bottom - page_rect.top,
					 SWP_NOZORDER);
	}
}

void TTCPropSheetDlg::AddTreeControl()
{
	if (m_hWndTV != NULL) {
		// WM_INITDIALOG と WM_SHOWWINDOW の両方から呼ばれるので、2 回目は何もしない
		// (ツリーが 2 つできる、タブを 2 回隠す、を防ぐ)
		return;
	}

	HWND hTab = PropSheet_GetTabControl(m_hWnd);

	// ツリーで選択できるのでタブは1行設定にする
	SetWindowLongPtr(hTab, GWL_STYLE, GetWindowLongPtr(hTab, GWL_STYLE) & ~TCS_MULTILINE);

	// ツリーコントロールの位置を決める
	// タブを隠す前の矩形を使う(隠すとタブコントロールを上へ広げるため)
	RECT tree_rect;
	GetWindowRect(hTab, &tree_rect);
 	int tree_w = TREE_WIDTH;
	int tree_h = tree_rect.bottom - tree_rect.top;
	POINT pt;
	pt.x = tree_rect.left;
	pt.y = tree_rect.top;
	ScreenToClient(m_hWnd, &pt);

	if (!m_ShowTab) {
		// ツリーでページを選べるのでタブ行は不要。隠して、空いた分だけページを広げる
		HideTab(hTab);
	}

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
