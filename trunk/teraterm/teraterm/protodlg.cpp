/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TERATERM.EXE, file-transfer-protocol dialog box */
#include "stdafx.h"
#include "teraterm.h"
#include "tt_res.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ttlib.h"
#include "protodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProtoDlg dialog

BEGIN_MESSAGE_MAP(CProtoDlg, CDialog)
	//{{AFX_MSG_MAP(CProtoDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CProtoDlg::Create(PFileVar pfv, PTTSet pts)
{
	BOOL Ok;
	LOGFONT logfont;
	HFONT font;

	fv = pfv;

	Ok = CDialog::Create(CProtoDlg::IDD, NULL);
	fv->HWin = GetSafeHwnd();

	font = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(font, sizeof(LOGFONT), &logfont);
	if (get_lang_font("DLG_SYSTEM_FONT", GetSafeHwnd(), &logfont, &DlgFont, pts->UILanguageFile)) {
		SendDlgItemMessage(IDC_PROT_FILENAME, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROTOFNAME, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROT_PROT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROTOPROT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROT_PACKET, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROTOPKTNUM, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROT_TRANS, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROTOBYTECOUNT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDC_PROTOPERCENT, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
		SendDlgItemMessage(IDCANCEL, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE,0));
	}

	return Ok;
}

/////////////////////////////////////////////////////////////////////////////
// CProtoDlg message handler

void CProtoDlg::OnCancel( )
{
	::PostMessage(fv->HMainWin,WM_USER_PROTOCANCEL,0,0);
}

BOOL CProtoDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch (LOWORD(wParam)) {
		case IDCANCEL:
			::PostMessage(fv->HMainWin,WM_USER_PROTOCANCEL,0,0);
			return TRUE;
		default:
			return (CDialog::OnCommand(wParam,lParam));
	}
}

void CProtoDlg::PostNcDestroy()
{
	delete this;
}
