#pragma once

#include "tt_res.h"


typedef struct {
	char *name;
	LPCTSTR id;
} mouse_cursor_t;

typedef struct cygterm {
	char term[128];
	char term_type[80];
	char port_start[80];
	char port_range[80];
	char shell[80];
	char env1[128];
	char env2[128];
	BOOL login_shell;
	BOOL home_chdir;
	BOOL agent_proxy;
} cygterm_t;



// General Page
class CGeneralPropPageDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CGeneralPropPageDlg)

public:
	CGeneralPropPageDlg();
	virtual ~CGeneralPropPageDlg();
	BOOL OnInitDialog();
	void OnOK();

	enum { IDD = IDD_TABSHEET_GENERAL };

private:
	HFONT DlgGeneralFont;
	LOGFONT logfont;
	HFONT font;

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};



// Copypaste Page
class CCopypastePropPageDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CCopypastePropPageDlg)

public:
	CCopypastePropPageDlg();
	virtual ~CCopypastePropPageDlg();
	BOOL OnInitDialog();
	void OnOK();

	enum { IDD = IDD_TABSHEET_COPYPASTE };

private:
	HFONT DlgCopypasteFont;
	LOGFONT logfont;
	HFONT font;

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};



// Visual Page
class CVisualPropPageDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CVisualPropPageDlg)

public:
	CVisualPropPageDlg();
	virtual ~CVisualPropPageDlg();
	BOOL OnInitDialog();
	void OnOK();

	enum { IDD = IDD_TABSHEET_VISUAL };

private:
	HFONT DlgVisualFont;
	LOGFONT logfont;
	HFONT font;

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};



// Log Page
class CLogPropPageDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CLogPropPageDlg)

public:
	CLogPropPageDlg();
	virtual ~CLogPropPageDlg();
	BOOL OnInitDialog();
	void OnOK();

	enum { IDD = IDD_TABSHEET_LOG };

private:
	HFONT DlgLogFont;
	LOGFONT logfont;
	HFONT font;

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};



// Cygwin Page
class CCygwinPropPageDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CCygwinPropPageDlg)

public:
	CCygwinPropPageDlg();
	virtual ~CCygwinPropPageDlg();
	BOOL OnInitDialog();
	void OnOK();

	enum { IDD = IDD_TABSHEET_CYGWIN };

private:
	HFONT DlgCygwinFont;
	LOGFONT logfont;
	HFONT font;
	cygterm_t settings;

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};



// Property Sheet
class CAddSettingPropSheetDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CAddSettingPropSheetDlg)

public:
	CAddSettingPropSheetDlg(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CAddSettingPropSheetDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CAddSettingPropSheetDlg();
	BOOL OnInitDialog();

private:
	HFONT DlgAdditionalFont;
	LOGFONT logfont;
	HFONT font;

protected:
	DECLARE_MESSAGE_MAP()

public:
	CGeneralPropPageDlg   m_GeneralPage;
	CCopypastePropPageDlg m_CopypastePage;
	CVisualPropPageDlg    m_VisualPage;
	CLogPropPageDlg       m_LogPage;
	CCygwinPropPageDlg    m_CygwinPage;
};
