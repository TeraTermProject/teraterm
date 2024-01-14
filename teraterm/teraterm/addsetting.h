/*
 * (C) 2008- TeraTerm Project
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

#include "tmfc.h"
#include "tmfc_propdlg.h"
#include "tt_res.h"
#include "teraterm.h"
#include "tipwin.h"

typedef struct {
	const char *name;
	LPCTSTR id;
} mouse_cursor_t;

extern const mouse_cursor_t MouseCursor[];

// Control Sequence Page
class CSequencePropPageDlg : public TTCPropertyPage
{
public:
	CSequencePropPageDlg(HINSTANCE inst);
	virtual ~CSequencePropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	enum { IDD = IDD_TABSHEET_SEQUENCE };
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHelp();
};

// Copypaste Page
class CCopypastePropPageDlg : public TTCPropertyPage
{
public:
	CCopypastePropPageDlg(HINSTANCE inst);
	virtual ~CCopypastePropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	enum { IDD = IDD_TABSHEET_COPYPASTE };
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHelp();
};

// Cygwin Page
extern "C" {
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

void ReadCygtermConfFile(const char *homedir, cygterm_t *psettings);
BOOL WriteCygtermConfFile(const char *homedir, cygterm_t *psettings);
BOOL CmpCygtermConfFile(const cygterm_t *a, const cygterm_t *b);
}

class CCygwinPropPageDlg : public TTCPropertyPage
{
public:
	CCygwinPropPageDlg(HINSTANCE inst);
	virtual ~CCygwinPropPageDlg();
private:
	void OnInitDialog();
	void OnOK();
	enum { IDD = IDD_TABSHEET_CYGWIN };
	cygterm_t settings;
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnHelp();
};

// AddSetting Property Sheet
class CAddSettingPropSheetDlg: public TTCPropSheetDlg
{
public:
	CAddSettingPropSheetDlg(HINSTANCE hInstance, HWND hParentWnd);
	~CAddSettingPropSheetDlg();
	enum Page {
		DefaultPage,
		FontPage,
		TermPage,
		CodingPage,
	};
	void SetStartPage(Page page);

private:
	int m_PageCountCPP;
	TTCPropertyPage *m_Page[7];
};
