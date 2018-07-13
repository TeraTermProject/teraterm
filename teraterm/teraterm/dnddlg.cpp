/*
 * (C) 2005-2018 TeraTerm Project
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
 
#include "dnddlg.h"

#include <windowsx.h>
#include <stdio.h>		// for _snprintf_s

#include "tt_res.h"

#include "i18n.h"
#include "ttlib.h"
#include "dlglib.h"
#include "tttypes.h"	// for ttwinman.h
#include "ttwinman.h"	// for ts

struct DlgTextInfo {
	int nIDDlgItem;
	char *key;
};

struct DrapDropDlgParam {
	const char *TargetFilename;
	enum drop_type DropType;
	unsigned char DropTypePaste;
	bool ScpEnable;
	char *ScpSendDirPtr;
	int ScpSendDirSize;
	bool SendfileEnable;
	bool PasteNewlineEnable;
	int RemaingFileCount;
	bool DoSameProcess;
	bool DoSameProcessNextDrop;
	bool DoNotShowDialogEnable;
	bool DoNotShowDialog;
	const char *UILanguageFile;
};

struct DrapDropDlgData {
	HFONT hPrevFont;
	DrapDropDlgParam *Param;
};

static HFONT SetDlgFonts(HWND hDlg, const int nIDDlgItems[], int nIDDlgItemCount, const char *UILanguageFile, PCHAR key)
{
	if (key == NULL) key = "DLG_TAHOMA_FONT";
	HFONT hPrevFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0);
	LOGFONT logfont;
	GetObject(hPrevFont, sizeof(LOGFONT), &logfont);
	HFONT hNewFont;
	if (get_lang_font("DLG_TAHOMA_FONT", hDlg, &logfont, &hNewFont, UILanguageFile)) {
		for (int i = 0 ; i < nIDDlgItemCount ; i++) {
			const int nIDDlgItem = nIDDlgItems[i];
			SendDlgItemMessage(hDlg, nIDDlgItem, WM_SETFONT, (WPARAM)hNewFont, MAKELPARAM(TRUE,0));
		}
	}
	return hNewFont;
}

static void SetDlgTexts(HWND hDlgWnd, const DlgTextInfo *infos, int infoCount, const char *UILanguageFile)
{
	for (int i = 0 ; i < infoCount; i++) {
		char *key = infos[i].key;
		char uimsg[MAX_UIMSG];
		get_lang_msg(key, uimsg, sizeof(uimsg), "", UILanguageFile);
		if (uimsg[0] != '\0') {
			const int nIDDlgItem = infos[i].nIDDlgItem;
			if (nIDDlgItem == 0) {
				SetWindowText(hDlgWnd, uimsg);
			} else {
				SetDlgItemText(hDlgWnd, nIDDlgItem, uimsg);
			}
		}
	}
}

static LRESULT CALLBACK OnDragDropDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const int FontIDs[] = {
		IDC_FILENAME_EDIT,
		IDC_DAD_STATIC,
		IDC_SCP_RADIO, IDC_SENDFILE_RADIO, IDC_PASTE_RADIO,
		IDC_SCP_PATH_LABEL, IDC_SCP_PATH, IDC_SCP_PATH_NOTE,
		IDC_BINARY_CHECK,
		IDC_ESCAPE_CHECK, IDC_NEWLINE_RADIO, IDC_SPACE_RADIO,
		IDC_SAME_PROCESS_CHECK, IDC_SAME_PROCESS_NEXTDROP_CHECK, IDC_DONTSHOW_CHECK,
		IDC_DAD_NOTE,
		IDOK, IDCANCEL,
	};
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_DANDD_TITLE" },
		{ IDC_DAD_STATIC, "DLG_DANDD_TEXT" },
		{ IDC_SCP_PATH_NOTE, "DLG_DANDD_DEST_NOTE" },
		{ IDC_SAME_PROCESS_CHECK, "DLG_DANDD_CONFLICTS" },
		{ IDC_SAME_PROCESS_NEXTDROP_CHECK, "DLG_DANDD_SAME_NEEXTDROP" },
		{ IDC_DONTSHOW_CHECK, "DLG_DANDD_DONTSHOW_NEEXTDROP" },
		{ IDC_DAD_NOTE, "DLG_DANDD_NOTE" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
	};
	DrapDropDlgData *DlgData = (DrapDropDlgData *)GetWindowLongPtr(hDlgWnd, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
	{
		DlgData = (DrapDropDlgData *)malloc(sizeof(DrapDropDlgData));
		SetWindowLongPtr(hDlgWnd, GWLP_USERDATA, (LONG_PTR)DlgData);
		DrapDropDlgParam *Param = (DrapDropDlgParam *)lp;
		DlgData->Param = Param;
		DlgData->hPrevFont = SetDlgFonts(hDlgWnd, FontIDs, _countof(FontIDs), Param->UILanguageFile, NULL);
		SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), Param->UILanguageFile);

		// target file
		SetDlgItemText(hDlgWnd, IDC_FILENAME_EDIT, Param->TargetFilename);
			
		// checkbox
		CheckRadioButton(hDlgWnd, IDC_SCP_RADIO, IDC_PASTE_RADIO,
						 (Param->DropType == DROP_TYPE_SEND_FILE ||
						  Param->DropType == DROP_TYPE_SEND_FILE_BINARY) ? IDC_SENDFILE_RADIO :
						 Param->DropType == DROP_TYPE_PASTE_FILENAME  ? IDC_PASTE_RADIO :
						 IDC_SCP_RADIO);

		// SCP
		SendMessage(GetDlgItem(hDlgWnd, IDC_SCP_PATH), WM_SETTEXT, 0, (LPARAM)Param->ScpSendDirPtr);
		if (!Param->ScpEnable) {
			// 無効化
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SCP_RADIO), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SCP_PATH_LABEL), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SCP_PATH), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SCP_PATH_NOTE), FALSE);
		}

		// Send File
		if (Param->DropType == DROP_TYPE_SEND_FILE_BINARY) {
			SendMessage(GetDlgItem(hDlgWnd, IDC_BINARY_CHECK), BM_SETCHECK, BST_CHECKED, 0);
		}
		if (!Param->SendfileEnable) {
			// 無効化
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SENDFILE_RADIO), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_BINARY_CHECK), FALSE);
		}

		// Paste Filename
		if (Param->DropTypePaste & DROP_TYPE_PASTE_ESCAPE) {
			SendMessage(GetDlgItem(hDlgWnd, IDC_ESCAPE_CHECK), BM_SETCHECK, BST_CHECKED, 0);
		}
		CheckRadioButton(hDlgWnd, IDC_SPACE_RADIO, IDC_NEWLINE_RADIO, 
						 Param->DropTypePaste & DROP_TYPE_PASTE_NEWLINE?
						 IDC_NEWLINE_RADIO : IDC_SPACE_RADIO);
		if (Param->RemaingFileCount < 2) {
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SPACE_RADIO), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_NEWLINE_RADIO), FALSE);
		}

		// Do this for the next %d conflicts
		char orgmsg[MAX_UIMSG];
		GetDlgItemText(hDlgWnd, IDC_SAME_PROCESS_CHECK, orgmsg, sizeof(orgmsg));
		char uimsg[MAX_UIMSG];
		_snprintf_s(uimsg, sizeof(uimsg), _TRUNCATE, orgmsg, Param->RemaingFileCount - 1);
		SetDlgItemText(hDlgWnd, IDC_SAME_PROCESS_CHECK, uimsg);
		if (Param->RemaingFileCount < 2) {
			EnableWindow(GetDlgItem(hDlgWnd, IDC_SAME_PROCESS_CHECK), FALSE);
		}

		// Dont Show Dialog
		if (Param->DoNotShowDialog) {
			SendMessage(GetDlgItem(hDlgWnd, IDC_DONTSHOW_CHECK), BM_SETCHECK, BST_CHECKED, 0);
		}
		if (!Param->DoNotShowDialogEnable) {
			EnableWindow(GetDlgItem(hDlgWnd, IDC_DONTSHOW_CHECK), FALSE);
			EnableWindow(GetDlgItem(hDlgWnd, IDC_DAD_NOTE), FALSE);
		}

		// focus to "SCP dest textbox" or "Cancel"
		if (Param->ScpEnable) {
			// "SCP" 有効時は Cancel にフォーカスを当て、最終的に SCP PATH にフォーカスが
			// 当たるようにする。
			SetFocus(GetDlgItem(hDlgWnd, IDC_SCP_RADIO));
		} else {
			// フォーカスの初期状態を Cancel にする為、この時点では IDOK に
			// フォーカスを当てる。後で WM_NEXTDLGCTL でフォーカスが次のボタンになる。
			SetFocus(GetDlgItem(hDlgWnd, IDOK));
		}
		// フォーカスを次のボタンに移す
		// SetFocus() で直接フォーカスを当てるとタブキーの動作等に問題が出るため、
		// このメッセージを併用する
		PostMessage(hDlgWnd, WM_NEXTDLGCTL, 0, 0L);

		// TRUEにするとボタンにフォーカスが当たらない。
		return FALSE;
	}

	case WM_COMMAND:
	{
		WORD wID = GET_WM_COMMAND_ID(wp, lp);
		const WORD wCMD = GET_WM_COMMAND_CMD(wp, lp);
		if (wCMD == BN_DBLCLK &&
			(wID == IDC_SCP_RADIO || wID == IDC_SENDFILE_RADIO || wID == IDC_PASTE_RADIO))
		{	// radio buttons double click
			wID = IDOK;
		}
		if (wCMD == EN_SETFOCUS && wID == IDC_SCP_PATH) {
			CheckRadioButton(hDlgWnd, IDC_SCP_RADIO, IDC_PASTE_RADIO, IDC_SCP_RADIO);
		}
		if (wID == IDC_BINARY_CHECK) {
			CheckRadioButton(hDlgWnd, IDC_SCP_RADIO, IDC_PASTE_RADIO, IDC_SENDFILE_RADIO);
		}
		if (wID == IDC_ESCAPE_CHECK ||
			wID == IDC_SPACE_RADIO || wID == IDC_NEWLINE_RADIO)
		{
			CheckRadioButton(hDlgWnd, IDC_SCP_RADIO, IDC_PASTE_RADIO, IDC_PASTE_RADIO);
		}
		if (wID == IDOK) {
			if (IsDlgButtonChecked(hDlgWnd, IDC_SCP_RADIO) == BST_CHECKED) {
				// SCP
				DlgData->Param->DropType = DROP_TYPE_SCP;
				SendMessage(GetDlgItem(hDlgWnd, IDC_SCP_PATH), WM_GETTEXT,
							(WPARAM)DlgData->Param->ScpSendDirSize,
							(LPARAM)DlgData->Param->ScpSendDirPtr);
			} else if (IsDlgButtonChecked(hDlgWnd, IDC_SENDFILE_RADIO) == BST_CHECKED) {
				// Send File
				DlgData->Param->DropType =
					(IsDlgButtonChecked(hDlgWnd, IDC_BINARY_CHECK) == BST_CHECKED) ?
					DROP_TYPE_SEND_FILE_BINARY : DROP_TYPE_SEND_FILE;
			} else /* if (IsDlgButtonChecked(hDlgWnd, IDC_PASTE_RADIO) == BST_CHECKED) */ {
				// Paste Filename
				DlgData->Param->DropType = DROP_TYPE_PASTE_FILENAME;
				DlgData->Param->DropTypePaste = 0;
				DlgData->Param->DropTypePaste |=
					(IsDlgButtonChecked(hDlgWnd, IDC_ESCAPE_CHECK) == BST_CHECKED) ?
					DROP_TYPE_PASTE_ESCAPE : 0;
				DlgData->Param->DropTypePaste |=
					(IsDlgButtonChecked(hDlgWnd, IDC_NEWLINE_RADIO) == BST_CHECKED) ?
					DROP_TYPE_PASTE_NEWLINE : 0;
			}
			DlgData->Param->DoSameProcess = 
				(IsDlgButtonChecked(hDlgWnd, IDC_SAME_PROCESS_CHECK) == BST_CHECKED) ?
				true : false;
			DlgData->Param->DoSameProcessNextDrop =
				(IsDlgButtonChecked(hDlgWnd, IDC_SAME_PROCESS_NEXTDROP_CHECK) == BST_CHECKED) ?
				true : false;
			DlgData->Param->DoNotShowDialog = 
				(IsDlgButtonChecked(hDlgWnd, IDC_DONTSHOW_CHECK) == BST_CHECKED) ?
				true : false;
		}
		if (wID == IDCANCEL) {
			DlgData->Param->DropType = DROP_TYPE_CANCEL;
		}
		if (wID == IDOK || wID == IDCANCEL) {
			if (DlgData->hPrevFont != NULL) {
				DeleteObject(DlgData->hPrevFont);
			}
			EndDialog(hDlgWnd, wID);
			free(DlgData);
			break;
		}
		return FALSE;
	}

	default:
		return FALSE;
	}
	return TRUE;
}

enum drop_type ShowDropDialogBox(
	HINSTANCE hInstance, HWND hWndParent,
	const char *TargetFilename,
	enum drop_type DefaultDropType,
	int RemaingFileCount,
	bool EnableSCP,
	bool EnableSendFile,
	bool EnableDoNotShowDialog,
	unsigned char *DropTypePaste,
	bool *DoSameProcess,
	bool *DoSameProcessNextDrop,
	bool *DoNotShowDialog)
{
	DrapDropDlgParam Param;
	Param.TargetFilename = TargetFilename;
	Param.DropType = DefaultDropType;
	Param.DropTypePaste = *DropTypePaste;
	Param.ScpEnable = EnableSCP;
	Param.ScpSendDirPtr = ts.ScpSendDir;
	Param.ScpSendDirSize = sizeof(ts.ScpSendDir);
	Param.SendfileEnable = EnableSendFile;
	Param.PasteNewlineEnable = true;
	Param.RemaingFileCount = RemaingFileCount;
	Param.DoNotShowDialog = *DoNotShowDialog;
	Param.DoNotShowDialogEnable = EnableDoNotShowDialog;
	Param.UILanguageFile = ts.UILanguageFile;
	int ret = DialogBoxParam(
		hInstance, MAKEINTRESOURCE(IDD_DAD_DIALOG),
		hWndParent, (DLGPROC)OnDragDropDlgProc,
		(LPARAM)&Param);
	if (ret != IDOK) {
		return DROP_TYPE_CANCEL;
	}
	*DropTypePaste = Param.DropTypePaste;
	*DoSameProcess = Param.DoSameProcess;
	*DoSameProcessNextDrop = Param.DoSameProcessNextDrop;
	*DoNotShowDialog = Param.DoNotShowDialog;
	return Param.DropType;
}
