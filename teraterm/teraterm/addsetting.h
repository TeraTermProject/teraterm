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

#ifdef __cplusplus
#include "tmfc.h"
#include "tmfc_propdlg.h"
#endif

typedef struct {
	const char *name;
	LPCSTR id;
} mouse_cursor_t;

extern const mouse_cursor_t MouseCursor[];

#ifdef __cplusplus

// AddSetting Property Sheet
class CAddSettingPropSheetDlg: public TTCPropSheetDlg
{
public:
	enum Page {
		DefaultPage,
		CodingPage,
		FontPage,
		KeyboardPage,
		TcpIpPage,
		GeneralPage,
		TermPage,
		WinPage,
		SerialPortPage,
		TekWinPage,
		TekFontPage
	};

	CAddSettingPropSheetDlg(HINSTANCE hInstance, HWND hParentWnd, Page StartPage);
	~CAddSettingPropSheetDlg();

private:
	int m_PageCountCPP;
	TTCPropertyPage *m_Page[7];
};
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DefaultPage,
	CodingPage,
	FontPage,
	KeyboardPage,
	TcpIpPage,
	GeneralPage,
	TermPage,
	WinPage,
	SerialPortPage,
	TekWinPage,
	TekFontPage,
} CAddSettingPropSheetDlgPage;

typedef enum {
	ADDSETTING_WIN_VT,
	ADDSETTING_WIN_TEK,
} AddsettingWin;

AddsettingWin AddsettingCheckWin(HWND hWnd);

#ifdef __cplusplus
}
#endif
