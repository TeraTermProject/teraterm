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
 *	�O�����A�㏈���ɂ���
 *		�]���̓_�C�A���O���ɁA�O�����A�㏈����������Ă���
 *		���݂̓^�u������S�Ă̐ݒ肪�s����̂ŁA�S�Ă̑O�����A�㏈�����s����
 */

/**
 *	�ݒ�_�C�A���O���o���O�̏���
 *
 *	@param	page	��������^�u
 *					�폜�\��
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
 *	�ݒ�_�C�A���O�������̏���
 *		ok = TRUE �̎���
 *			�ݒ�(ts�Ȃ�)�̒l�𔽉f����
 *		ok = FALSE �̎���
 *			�K�v�ł���Ό㏈�����s��
 *
 *	@param	page	��������^�u
 *					�폜�\��
 *	@param	ok		TRUE/FALSE = OK�������ꂽ/������Ȃ�����
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
		// Font�^�u
		if (ExternalSetupData.old_use_unicode_api != UnicodeDebugParam.UseUnicodeApi) {
			BuffSetDispAPI(UnicodeDebugParam.UseUnicodeApi);
		}
		// ANSI�\���p�̃R�[�h�y�[�W��ݒ肷��
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

			// �^�C�g�����ύX����Ă�����A�����[�g�^�C�g�����N���A����
			if ((ts.AcceptTitleChangeRequest == IdTitleChangeRequestOverwrite) &&
				(strcmp(ExternalSetupData.orgTitle, ts.Title) != 0)) {
				free(cv.TitleRemoteW);
				cv.TitleRemoteW = NULL;
			}
			ChangeWin();
			ChangeFont(0);

		}
		free(ExternalSetupData.orgTitle);
		ExternalSetupData.orgTitle = NULL;
	}
	if (all || page == CAddSettingPropSheetDlgPage::SerialPortPage) {
		if (ok) {
			if (ts.ComPort > 0) {

				if (cv.Ready && (cv.PortType != IdSerial)) {
					// �V���A���ȊO�ɐڑ����̏ꍇ
					//  TODO cv.Ready �� cv.Open�̍���?
#if 0
					OpenNewComport(&ts);
					return;
#endif
				}
				else if (!cv.Open) {
					// ���ڑ��̏ꍇ
#if 0
					CommOpen(m_hWnd,&ts,&cv);
#endif
				}
				else {
					// �V���A���ɐڑ����̏ꍇ
#if 0
					if (ts.ComPort != cv.ComPort) {
						// �|�[�g��ύX����
						CommClose(&cv);
						CommOpen(HVTWin,&ts,&cv);
					}
					else
#endif
					{
						// �ʐM�p�����[�^��ύX����
						CommResetSerial(&ts, &cv, ts.ClearComBuffOnOpen);
					}
				}
			}
		}
	}
}

/**
 *	Additional Setting ��\������
 *
 *	@param	page	DefaultPage		�S�Ẵ^�u��\�����ĕ\������
 *					���̑�			����̃^�u��\������
 *	@retval	TRUE	"OK"�������ꂽ
 *	@retval	FALSE	"Cancel"�������ꂽ
 *
 *	�֐����R�[�����鏇(VTWin����̏ꍇ)
 *	- ExternalSetupPreProcess()
 *	- OpenExternalSetupTab()
 *	- ExternalSetupPostProcess()
 */
BOOL OpenExternalSetupTab(HWND hWndParent, CAddSettingPropSheetDlgPage page)
{
	SetDialogFont(ts.DialogFontNameW, ts.DialogFontPoint, ts.DialogFontCharSet,
				  ts.UILanguageFileW, "Tera Term", "DLG_TAHOMA_FONT");

	// TEKWin���ʏ���
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

	// PreProces���Ă΂�Ă��邩�`�F�b�N
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
 *	�����ȍ~�� vtwin.cpp ���� UI����/�v���O�C������R�[�������
 *		OpenExternalSetup() �ȊO�̓t�b�N����Ă��ă_�C�A���O���J���Ȃ��ꍇ������
 *		�_�C�A���O���J���ꍇ�� OpenExternalSetupTab() ���R�[�������
 */
void OpenExternalSetup(HWND hWndParent)
{
	ExternalSetupPreProcess(CAddSettingPropSheetDlgPage::DefaultPage);
	BOOL r = OpenExternalSetupTab(hWndParent, CAddSettingPropSheetDlgPage::DefaultPage);
	ExternalSetupPostProcess(CAddSettingPropSheetDlgPage::DefaultPage, r);
}

/**
 *
 *	�v���O�C������̌Ăяo��
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
 *	�v���O�C������̌Ăяo��
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

