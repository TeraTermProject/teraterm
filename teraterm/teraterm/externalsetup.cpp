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
#include "setting.h"
#include "ttdialog.h"
#include "ttwinman.h"
#include "tekwin.h"

#include "externalsetup.h"

static struct {
	BOOL PerProcessCalled;
	BOOL old_VTDrawAPI;
	char *orgTitle;
	HWND hWnd_disable;
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
static void ExternalSetupPreProcess(HWND hWnd, CAddSettingPropSheetDlgPage page)
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
		ExternalSetupData.old_VTDrawAPI = ts.VTDrawAPI;
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

	// TEK Win
	if (pTEKWin != NULL) {
		((CTEKWindow*)pTEKWin)->OnSetupPreProcess();
	}

	// 入力の無効化(アプリケーションモーダル状態にする)
	HWND hWnd_disable = hWnd == HVTWin ? HTEKWin : HVTWin;
	ExternalSetupData.hWnd_disable = hWnd_disable;
	if (hWnd_disable != NULL) {
		EnableWindow(hWnd_disable, FALSE);
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
	TTTSet *pts = &ts;

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
		if (ExternalSetupData.old_VTDrawAPI != pts->VTDrawAPI) {
			BuffSetDispAPI(pts->VTDrawAPI);
		}
		// ANSI表示用のコードページを設定する
		BuffSetDispCodePage(pts->VTDrawAnsiCodePage);
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
			ResetIME(vt_src);
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
			ChangeFont(vt_src, 0);

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

		// ダイアログフォントの変更
		if (ok) {
			SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint,
						  ts.DialogFontCharSet, ts.UILanguageFileW,
						  "Tera Term", "DLG_SYSTEM_FONT");
		}
	}

	// TEK Win
	if (pTEKWin != NULL) {
		((CTEKWindow*)pTEKWin)->OnSetupPostProcess(ok);
	}

	// 通常状態にする
	HWND hWnd_disable = ExternalSetupData.hWnd_disable;
	if (hWnd_disable != NULL) {
		EnableWindow(hWnd_disable, TRUE);
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

	// PreProcesが呼ばれているかチェック
	assert(ExternalSetupData.PerProcessCalled == TRUE);

	// TEKWin特別処理
	if (AddsettingCheckWin(hWndParent) == ADDSETTING_WIN_TEK) {
		if (page == CAddSettingPropSheetDlgPage::WinPage) {
			// Window Setup
			page = TekWinPage;
		}
		else if (page == FontPage) {
			page = TekFontPage;
		}
		else {
			assert(FALSE);
		}
	}

	// VTWin

	int one_page = DefaultPage;
	CAddSettingPropSheetDlg CAddSetting(hInst, hWndParent, (CAddSettingPropSheetDlg::Page)page);
	if (one_page == CAddSettingPropSheetDlgPage::DefaultPage) {
		CAddSetting.SetTreeViewMode(ts.ExperimentalTreePropertySheetEnable);
	}
	INT_PTR ret = CAddSetting.DoModal();
	return (ret == IDOK) ? TRUE : FALSE;
}

/*
 *	ここ以降は vtwin.cpp から UI操作/プラグインからコールされる
 *		OpenExternalSetup() 以外はフックされていてダイアログが開かない場合がある
 *		ダイアログが開く場合は OpenExternalSetupTab() がコールされる
 */
void OpenExternalSetup(HWND hWnd)
{
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::DefaultPage);
	BOOL r = OpenExternalSetupTab(hWnd, CAddSettingPropSheetDlgPage::DefaultPage);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::DefaultPage, r);
}

/**
 *
 *	プラグインからの呼び出し
 *		SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_TERMINAL, 0), 0);
 */
void OpenSetupTerminal(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::TermPage);
	BOOL r = (*SetupTerminal)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::TermPage, r);
}

/**
 *
 *	プラグインからの呼び出し
 *		SendMessage(HWin, WM_COMMAND, MAKELONG(ID_SETUP_WINDOW, 0), 0);
 */
void OpenSetupWin(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::WinPage);
	BOOL r = (*SetupWin)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::WinPage, r);
}

void OpenSetupFont(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	CAddSettingPropSheetDlgPage page;
	if (AddsettingCheckWin(hWnd) == ADDSETTING_WIN_VT) {
		page = FontPage;
		hWnd = HVTWin;
	}
	else {
		page = TekFontPage;
		hWnd = HTEKWin;
	}


	ExternalSetupPreProcess(hWnd, page);
	BOOL r = (*ChooseFontDlg)(hWnd, NULL, &ts);
	ExternalSetupPostProcess(page, r);
}

void OpenSetupKeyboard(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::KeyboardPage);
	BOOL r = (*SetupKeyboard)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::KeyboardPage, r);
}

void OpenSetupTCPIP(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::TcpIpPage);
	BOOL r = (*SetupTCPIP)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::TcpIpPage, r);
}

void OpenSetupGeneral(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::GeneralPage);
	BOOL r = (*SetupGeneral)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::GeneralPage, r);
}

void OpenSetupSerialPort(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::SerialPortPage);
	BOOL r = (*SetupSerialPort)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::SerialPortPage, r);
}

void OpenSetupTekWindow(HWND hWnd)
{
	if (! LoadTTDLG()) {
		return;
	}
	ExternalSetupPreProcess(hWnd, CAddSettingPropSheetDlgPage::TekWinPage);
	BOOL r = (*SetupWin)(hWnd, &ts);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::TekWinPage, r);
}

void OpenSetupTekFont(HWND hWnd)
{
	return OpenSetupFont(hWnd);
}
