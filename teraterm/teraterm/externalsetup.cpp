/*
 * Copyright (C) 2024- TeraTerm Project
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

#include <assert.h>

#include "tttypes.h"
#include "ttwinman.h"
#include "buffer.h"
#include "vtdisp.h"
#include "vtwin.h"
#include "vtterm.h"
#include "commlib.h"
#include "dlglib.h"
#include "telnet.h"
#include "setting.h"
#include "ttdialog.h"

#include "externalsetup.h"

static struct {
	BOOL PerProcessCalled;
	BOOL old_use_unicode_api;
	char *orgTitle;
} ExternalSetupData;

/*
 *	前処理、後処理について
 *		従来はダイアログ毎に、前処理、後処理が分かれていた
 *		現在はタブ化され全ての設定が行えるので、全ての前処理、後処理が行われる
 */

/**
 *	設定ダイアログを出す前の処理
 *
 *	@param	page	処理するタブ
 *					削除予定
 */
static void ExternalSetupPreProcess(CAddSettingPropSheetDlgPage page)
{
	ExternalSetupData.PerProcessCalled = TRUE;
	BOOL all = TRUE;
	//BOOL all = FALSE;
	if (page == CAddSettingPropSheetDlgPage::DefaultPage) {
		all = TRUE;
	}
	if (all || page == CAddSettingPropSheetDlgPage::CodingPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::FontPage) {
		ts.SampleFont = VTFont[0];
		ExternalSetupData.old_use_unicode_api = UnicodeDebugParam.UseUnicodeApi;
	}
	if (all || page == CAddSettingPropSheetDlgPage::KeyboardPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::TcpIpPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::GeneralPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::TermPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::WinPage) {
		ExternalSetupData.orgTitle = _strdup(ts.Title);
	}
	if (all || page == CAddSettingPropSheetDlgPage::SerialPortPage) {
		;
	}
}

/**
 *	設定ダイアログを閉じた後の処理
 *		ok = TRUE の時は
 *			設定(tsなど)の値を反映する
 *		ok = FALSE の時は
 *			必要であれば後処理を行う
 *
 *	@param	page	処理するタブ
 *					削除予定
 *	@param	ok		TRUE/FALSE = OKが押された/押されなかった
 */
static void ExternalSetupPostProcess(CAddSettingPropSheetDlgPage page, BOOL ok)
{
	ExternalSetupData.PerProcessCalled = FALSE;

	//BOOL all = FALSE;
	BOOL all = TRUE;
	if (page == CAddSettingPropSheetDlgPage::DefaultPage) {
		all = TRUE;
	}
	if (all || page == CAddSettingPropSheetDlgPage::CodingPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::FontPage) {
		// Fontタブ
		if (ExternalSetupData.old_use_unicode_api != UnicodeDebugParam.UseUnicodeApi) {
			BuffSetDispAPI(UnicodeDebugParam.UseUnicodeApi);
		}
		// ANSI表示用のコードページを設定する
		BuffSetDispCodePage(UnicodeDebugParam.CodePageForANSIDraw);
	}
	if (all || page == CAddSettingPropSheetDlgPage::KeyboardPage) {
		//ResetKeypadMode(TRUE);
		//ResetIME();
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::TcpIpPage) {
		;
	}
	if (all || page == CAddSettingPropSheetDlgPage::GeneralPage) {
		if (ok) {
			ResetCharSet();
			ResetIME();
		}
	}
	if (all || page == CAddSettingPropSheetDlgPage::TermPage) {
		if (ok) {
			pVTWin->SetupTerm();
		}
	}
	if (all || page == CAddSettingPropSheetDlgPage::WinPage) {
		if (ok) {
			pVTWin->SetColor();

			// タイトルが変更されていたら、リモートタイトルをクリアする
			if ((ts.AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite) &&
				(strcmp(ExternalSetupData.orgTitle, ts.Title) != 0)) {
				free(cv.TitleRemoteW);
				cv.TitleRemoteW = NULL;
			}
			ChangeWin();
			ChangeFont();

		}
		free(ExternalSetupData.orgTitle);
		ExternalSetupData.orgTitle = NULL;
	}
	if (all || page == CAddSettingPropSheetDlgPage::SerialPortPage) {
		if (ok) {
			if (ts.ComPort > 0) {

				if (cv.Ready && (cv.PortType != IdSerial)) {
					// シリアル以外に接続中の場合
					//  TODO cv.Ready と cv.Openの差は?
#if 0
					OpenNewComport(&ts);
					return;
#endif
				}
				else if (!cv.Open) {
					// 未接続の場合
#if 0
					CommOpen(m_hWnd,&ts,&cv);
#endif
				}
				else {
					// シリアルに接続中の場合
#if 0
					if (ts.ComPort != cv.ComPort) {
						// ポートを変更する
						CommClose(&cv);
						CommOpen(HVTWin,&ts,&cv);
					}
					else
#endif
					{
						// 通信パラメータを変更する
						CommResetSerial(&ts, &cv, ts.ClearComBuffOnOpen);
					}
				}
			}
		}
	}
}

/**
 *	Additional Setting を表示する
 *
 *	@param	page	DefaultPage		全てのタブを表示して表示する
 *					その他			特定のタブを表示する
 *	@retval	TRUE	"OK"が押された
 *	@retval	FALSE	"Cancel"が押された
 *
 *	関数をコールする順(VTWinからの場合)
 *	- ExternalSetupPreProcess()
 *	- OpenExternalSetupTab()
 *	- ExternalSetupPostProcess()
 */
BOOL OpenExternalSetupTab(HWND hWndParent, CAddSettingPropSheetDlgPage page)
{
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");

	// TEKWin特別処理
	if (AddsettingCheckWin(hWndParent) == ADDSETTING_WIN_TEK) {
		if (page == CAddSettingPropSheetDlgPage::WinPage) {
			// Window Setup
			CAddSettingPropSheetDlg CAddSetting(hInst, hWndParent);
			INT_PTR ret = CAddSetting.DoModal();
			return (ret == IDOK) ? TRUE : FALSE;
		}
		assert(FALSE);
	}

	// VTWin

	// PreProcesが呼ばれているかチェック
	assert(ExternalSetupData.PerProcessCalled == TRUE);

	int one_page = DefaultPage;
	CAddSettingPropSheetDlg CAddSetting(hInst, hWndParent);
	if (one_page == CAddSettingPropSheetDlgPage::DefaultPage) {
		CAddSetting.SetTreeViewMode(ts.ExperimentalTreePropertySheetEnable);
	}
	CAddSetting.SetStartPage((CAddSettingPropSheetDlg::Page)page);
	INT_PTR ret = CAddSetting.DoModal();
	return (ret == IDOK) ? TRUE : FALSE;
}

/*
 *	ここ以降は vtwin.cpp から UI操作/プラグインからコールされる
 *		OpenExternalSetup() 以外はフックされていてダイアログが開かない場合がある
 *		ダイアログが開く場合は OpenExternalSetupTab() がコールされる
 */
void OpenExternalSetup(HWND hWndParent)
{
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::DefaultPage);
	BOOL r = OpenExternalSetupTab(hWndParent, CAddSettingPropSheetDlgPage::DefaultPage);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::DefaultPage, r);
}

/**
 *
 *	プラグインからの呼び出し
 *		SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);
 */
void OpenSetupTerminal()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::TermPage);
	BOOL r = (*SetupTerminal)(HVTWin, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::TermPage, r);
}

/**
 *
 *	プラグインからの呼び出し
 *		SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
 */
void OpenSetupWin()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::WinPage);
	BOOL r = (*SetupWin)(HVTWin, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::WinPage, r);
}

void OpenSetupFont()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::FontPage);
	BOOL r = (*ChooseFontDlg)(HVTWin, NULL, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::FontPage, r);
}

void OpenSetupKeyboard()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::KeyboardPage);
	BOOL r = (*SetupKeyboard)(HVTWin, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::KeyboardPage, r);
}

void OpenSetupTCPIP()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::TcpIpPage);
	BOOL r = (*SetupTCPIP)(HVTWin, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::TcpIpPage, r);
}

void OpenSetupGeneral()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::GeneralPage);
	BOOL r = (*SetupGeneral)(HVTWin,&ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::GeneralPage, r);
}

void OpenSetupSerialPort()
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::SerialPortPage);
	BOOL r = (*SetupSerialPort)(HVTWin, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::SerialPortPage, r);
}

