/* Tera Term
 Copyright(C) 1994-1998 T. Teranishi
 All rights reserved. */

/* TTMACRO.EXE, main */

#include "stdafx.h"
#include "teraterm.h"
#include "ttm_res.h"
#include "ttmmain.h"
#include "ttl.h"

#include "ttmacro.h"
#include "ttmlib.h"
#include "ttlib.h"

#include "compat_w95.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char UILanguageFile[MAX_PATH];

/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CCtrlApp, CWinApp)
	//{{AFX_MSG_MAP(CCtrlApp)
	//}}AFX_MSG
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

CCtrlApp::CCtrlApp()
{
	typedef BOOL (WINAPI *pSetDllDir)(LPCSTR);
	typedef BOOL (WINAPI *pSetDefDllDir)(DWORD);

	HMODULE module;
	pSetDllDir setDllDir;
	pSetDefDllDir setDefDllDir;

	if ((module = GetModuleHandle("kernel32.dll")) != NULL) {
		if ((setDefDllDir = (pSetDefDllDir)GetProcAddress(module, "SetDefaultDllDirectories")) != NULL) {
			// SetDefaultDllDirectories() が使える場合は、検索パスを %WINDOWS%\system32 のみに設定する
			(*setDefDllDir)((DWORD)0x00000800); // LOAD_LIBRARY_SEARCH_SYSTEM32
		}
		else if ((setDllDir = (pSetDllDir)GetProcAddress(module, "SetDllDirectoryA")) != NULL) {
			// SetDefaultDllDirectories() が使えなくても、SetDllDirectory() が使える場合は
			// カレントディレクトリだけでも検索パスからはずしておく。
			(*setDllDir)("");
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

CCtrlApp theApp;

/////////////////////////////////////////////////////////////////////////////



BOOL CCtrlApp::InitInstance()
{
	static HMODULE HTTSET = NULL;

	GetUILanguageFile(UILanguageFile, sizeof(UILanguageFile));

	Busy = TRUE;
	m_pMainWnd = new CCtrlWindow();
	PCtrlWindow(m_pMainWnd)->Create();
	Busy = FALSE;  
	return TRUE;
}

int CCtrlApp::ExitInstance()
{
	//delete m_pMainWnd;
	if (m_pMainWnd) {
		m_pMainWnd->DestroyWindow();
	}
	m_pMainWnd = NULL;
	return ExitCode;
}

// TTMACRO main engine
BOOL CCtrlApp::OnIdle(LONG lCount)
{
	BOOL Continue;

	// Avoid multi entry
	if (Busy) {
		return FALSE;
	}
	Busy = TRUE;
	if (m_pMainWnd != NULL) {
		Continue = PCtrlWindow(m_pMainWnd)->OnIdle();
	}
	else {
		Continue = FALSE;
	}
	Busy = FALSE;
	return Continue;
}
