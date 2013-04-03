#pragma once
#include "afxwin.h"


// CListDlg ダイアログ

class CListDlg : public CDialog
{
	DECLARE_DYNAMIC(CListDlg)

public:
	CListDlg(PCHAR Text, PCHAR Caption, CHAR **Lists, int x, int y);
	virtual ~CListDlg();

// ダイアログ データ
	enum { IDD = IDD_LISTDLG };

protected:
	PCHAR m_Text;
	PCHAR m_Caption;
	CHAR **m_Lists;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート
	int PosX, PosY, init_WW, WW, WH, TW, TH, BH, BW, LW, LH;
	SIZE s;
	HFONT DlgFont;

	void Relocation(BOOL is_init, int WW);

	DECLARE_MESSAGE_MAP()
public:
	CListBox m_xcList;
	int m_SelectItem;
public:
	afx_msg void OnBnClickedOk();
public:
	virtual BOOL OnInitDialog();
public:
	afx_msg void OnBnClickedCancel();
};
