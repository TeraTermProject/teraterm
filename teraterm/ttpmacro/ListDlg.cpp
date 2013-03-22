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

CListDlg::CListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists)
	: CDialog(CListDlg::IDD)
{
	m_Text = Text;
	m_Caption = Caption;
	m_Lists = Lists;
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
	HFONT font;

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

	p = m_Lists;
	while (*p) {
		m_xcList.InsertString(-1, _T(*p));
		p++;
	}
    UpdateData(FALSE);

	// 1つめを選択状態にする。
	m_xcList.SetCurSel(0);

	// 本文とタイトル
	SetDlgItemText(IDC_STATIC, m_Text);
	SetWindowText(m_Caption);

	SetForegroundWindow();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CListDlg::OnBnClickedCancel()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	OnCancel();
}
