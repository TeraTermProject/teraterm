/*
 * (C) 2008-2017 TeraTerm Project
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

#pragma once

#include "tt_res.h"
#include "teraterm.h"


typedef struct {
	char *name;
	LPCTSTR id;
} mouse_cursor_t;



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
};



// Control Sequence Page
class CSequencePropPageDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CSequencePropPageDlg)

public:
	CSequencePropPageDlg();
	virtual ~CSequencePropPageDlg();
	BOOL OnInitDialog();
	void OnOK();

	enum { IDD = IDD_TABSHEET_SEQUENCE };

private:
	HFONT DlgSequenceFont;
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
	CSequencePropPageDlg  m_SequencePage;
	CCopypastePropPageDlg m_CopypastePage;
	CVisualPropPageDlg    m_VisualPage;
	CLogPropPageDlg       m_LogPage;
	CCygwinPropPageDlg    m_CygwinPage;
};
