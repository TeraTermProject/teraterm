/*
 * Copyright (C) 2013-2017 TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
//
// ListDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "teraterm.h"
#include "ttlib.h"
#include "ttm_res.h"
#include "ttmlib.h"
#include "tttypes.h"

#include "stdafx.h"
#include "ListDlg.h"


// CListDlg ダイアログ

IMPLEMENT_DYNAMIC(CListDlg, CDialog)

CListDlg::CListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists, int Selected, int x, int y)
	: CDialog(CListDlg::IDD)
{
	m_Text = Text;
	m_Caption = Caption;
	m_Lists = Lists;
	m_Selected = Selected;
	PosX = x;
	PosY = y;
	DlgFont = NULL;
}

CListDlg::~CListDlg()
{
}

void CListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTBOX, m_xcList);
}


BEGIN_MESSAGE_MAP(CListDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CListDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CListDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CListDlg メッセージ ハンドラ

void CListDlg::OnBnClickedOk()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	m_SelectItem = m_xcList.GetCurSel();
	OnOK();
}

BOOL CListDlg::OnInitDialog()
{
	char **p;
	char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG];
	LOGFONT logfont;
	HFONT font, tmpfont;
	int ListMaxWidth = 0;
	int ListWidth;
	int ListCount = 0;
	CDC *pDC;
	CFont *pOldFont;
	RECT R;
	HDC TmpDC;
	HWND HList, HOk;

	CDialog::OnInitDialog();

	// TODO:  ここに初期化を追加してください
	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", m_hWnd, &logfont, &DlgFont, UILanguageFile)) {
		SendDlgItemMessage(IDC_LISTBOX, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_STATIC, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDOK, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDCANCEL, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	GetDlgItemText(IDOK, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_OK", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDOK, uimsg);
	GetDlgItemText(IDCANCEL, uimsg2, sizeof(uimsg2));
	get_lang_msg("BTN_CANCEL", uimsg, sizeof(uimsg), uimsg2, UILanguageFile);
	SetDlgItemText(IDCANCEL, uimsg);

	pDC = m_xcList.GetDC(); // リストボックスを横スクロールできるように最大幅を取得
	pOldFont = pDC->SelectObject(m_xcList.GetFont());

	p = m_Lists;
	while (*p) {
		m_xcList.InsertString(-1, _T(*p));
		ListWidth = pDC->GetTextExtent(*p).cx;
		if (ListWidth > ListMaxWidth) {
			ListMaxWidth = ListWidth;
		}
		ListCount++;
		p++;
	}
	UpdateData(FALSE);

	m_xcList.SetHorizontalExtent(ListMaxWidth + 5);
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	if (m_Selected < 0 || m_Selected >= ListCount) {
		m_Selected = 0;
	}
	m_xcList.SetCurSel(m_Selected);

	// 本文とタイトル
	SetDlgItemText(IDC_STATIC, m_Text);
	SetWindowText(m_Caption);


	TmpDC = ::GetDC(GetDlgItem(IDC_STATIC)->GetSafeHwnd());
	if (DlgFont) {
		tmpfont = (HFONT)SelectObject(TmpDC, DlgFont);
	}
	CalcTextExtent(TmpDC,m_Text,&s);
	if (DlgFont && tmpfont != NULL) {
		SelectObject(TmpDC, tmpfont);
	}
	::ReleaseDC(GetDlgItem(IDC_STATIC)->GetSafeHwnd(),TmpDC);
	TW = s.cx + s.cx/10;
	TH = s.cy;

	HList = ::GetDlgItem(GetSafeHwnd(), IDC_LISTBOX);
	::GetWindowRect(HList,&R);
	LW = R.right-R.left;
	LH = R.bottom-R.top;

	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	::GetWindowRect(HOk,&R);
	BW = R.right-R.left;
	BH = R.bottom-R.top;

	GetWindowRect(&R);
	WW = R.right-R.left;
	WH = R.bottom-R.top;

	Relocation(TRUE, WW);

	BringupWindow(this->m_hWnd);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CListDlg::OnBnClickedCancel()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	OnCancel();
}

void CListDlg::Relocation(BOOL is_init, int new_WW)
{
	RECT R;
	HDC TmpDC;
	HWND HText, HOk, HCancel, HList;
	int CW, CH;

	GetClientRect(&R);
	CW = R.right-R.left;
	CH = R.bottom-R.top;

	// 初回のみ
	if (is_init) {
		// テキストコントロールサイズを補正
		if (TW < CW) {
			TW = CW;
		}
		// ウインドウサイズの計算
		WW = TW + (WW - CW);
		WH = TH + LH + (int)(BH*1.5) + (WH - CH);
		init_WW = WW;
		// リストボックスサイズの計算
		if (LW < WW - BW - 14*3) {
			LW = WW - BW - 14*3;
		}
	}
	else {
		TW = CW;
		WW = new_WW;
	}

	HText = ::GetDlgItem(GetSafeHwnd(), IDC_STATIC);
	HOk = ::GetDlgItem(GetSafeHwnd(), IDOK);
	HCancel = ::GetDlgItem(GetSafeHwnd(), IDCANCEL);
	HList = ::GetDlgItem(GetSafeHwnd(), IDC_LISTBOX);

	::MoveWindow(HText,(TW-s.cx)/2,LH+BH,TW,TH,TRUE);
	::MoveWindow(HList,14,BH/2,LW,LH,TRUE);
	::MoveWindow(HOk,14+14+LW,BH/2,BW,BH,TRUE);
	::MoveWindow(HCancel,14+14+LW,BH*2,BW,BH,TRUE);

	if (PosX<=GetMonitorLeftmost(PosX, PosY)-100) {
		GetWindowRect(&R);
		TmpDC = ::GetDC(GetSafeHwnd());
		PosX = (GetDeviceCaps(TmpDC,HORZRES)-R.right+R.left) / 2;
		PosY = (GetDeviceCaps(TmpDC,VERTRES)-R.bottom+R.top) / 2;
		::ReleaseDC(GetSafeHwnd(),TmpDC);
	}
	SetWindowPos(&wndTop,PosX,PosY,WW,WH,0);

	InvalidateRect(NULL);
}
