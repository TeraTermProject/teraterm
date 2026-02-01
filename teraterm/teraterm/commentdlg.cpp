/*
 * (C) 2025- TeraTerm Project
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

#include <windows.h>
#include <assert.h>

#include "i18n.h"
#include "ttlib.h"
#include "dlglib.h"
#include "filesys_log_res.h"
#include "tttypes.h"
#include "win32helper.h"

#include "commentdlg.h"

typedef struct {
	wchar_t *comment;
	const TTTSet *pts;
} DlgData;

static INT_PTR CALLBACK OnCommentDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_COMMENT_TITLE" },
		{ IDOK, "BTN_OK" }
	};

	switch (msg) {
		case WM_INITDIALOG: {
			DlgData *data = (DlgData *)lp;
			SetWindowLongPtrW(hDlgWnd, DWLP_USER, (LONG_PTR)data);

			const wchar_t *UILanguageFileW = data->pts->UILanguageFileW;
			SetDlgTextsW(hDlgWnd, TextInfos, _countof(TextInfos), UILanguageFileW);

			// エディットコントロールにフォーカスをあてる
			SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_COMMENT));

			CenterWindow(hDlgWnd, GetParent(hDlgWnd));
			return FALSE;
		}

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK: {
					DlgData *data = (DlgData *)GetWindowLongPtrW(hDlgWnd, DWLP_USER);
					hGetDlgItemTextW(hDlgWnd, IDC_EDIT_COMMENT, &data->comment);
					TTEndDialog(hDlgWnd, IDOK);
					break;
				}
				default:
					return FALSE;
			}
			break;

		case WM_CLOSE:
			TTEndDialog(hDlgWnd, 0);
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

/**
 *	コメント用1行取得
 *
 *	@param[in]		hInst
 *	@param[in]		hWnd
 *	@param[in]		pts			TTTSet
 *	@param[out]		comment		コメント(不要になったらfree()すること)
 *	@retval			0			エラー
 *	@retval			IDOK		ok
 */
INT_PTR CommentDlg(HINSTANCE hInst, HWND hWnd, const TTTSet *pts,
						  wchar_t **comment)
{
	DlgData *data = (DlgData *)calloc(1, sizeof(*data));
	assert(data != NULL);
	if (data == NULL) {
		*comment = NULL;
		return 0;
	}
	data->pts = pts;
	INT_PTR ret = TTDialogBoxParam(
		hInst, MAKEINTRESOURCEW(IDD_COMMENT_DIALOG),
		hWnd, OnCommentDlgProc, (LPARAM)data);
	*comment = data->comment;
	free(data);
	return ret;
}
