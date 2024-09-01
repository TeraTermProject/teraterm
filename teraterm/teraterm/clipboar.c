/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* TERATERM.EXE, Clipboard routines */
#include "teraterm.h"
#include "tttypes.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <wchar.h>

#include "ttwinman.h"
#include "ttlib.h"
#include "codeconv.h"
#include "fileread.h"
#include "sendmem.h"
#include "clipboarddlg.h"
#include "tttypes_charset.h"

#include "clipboar.h"


static const wchar_t BracketStartW[] = L"\033[200~";
static const wchar_t BracketEndW[] = L"\033[201~";

static void TrimTrailingNLW(wchar_t *src)
{
	wchar_t *tail = src + wcslen(src) - 1;
	while(tail >= src) {
		if (*tail != L'\r' && *tail != L'\n') {
			break;
		}
		*tail = L'\0';
		tail--;
	}
}

/**
 * �t�@�C���ɒ�`���ꂽ�����񂪁Atext�Ɋ܂܂�邩�𒲂ׂ�B
 * ������� TRUE��Ԃ�
 */
static BOOL search_dictW(const wchar_t *filename, const wchar_t *text)
{
	BOOL result = FALSE;
	const wchar_t *buf_top = LoadFileWW(filename, NULL);
	const wchar_t *buf = buf_top;
	if (buf == NULL) {
		return FALSE;
	}

	for(;;) {
		const wchar_t *line_end;
		size_t len;
		wchar_t *search_str;
		if (*buf == 0) {
			break;
		}
		if (*buf == '\r' || *buf == '\n') {
			buf++;
			continue;
		}
		line_end = wcspbrk(buf, L"\r\n");
		if (line_end == NULL) {
			// ���s���Ȃ�
			len = wcslen(buf);
			if (len == 0) {
				// �I��
				break;
			}
		} else {
			len = line_end - buf;
		}
		search_str = (wchar_t *)malloc(sizeof(wchar_t) * (len+1));
		if (search_str == NULL)
			continue;
		memcpy(search_str, buf, sizeof(wchar_t) * len);
		search_str[len] = 0;
		buf += len;
		result = (wcsstr(text, search_str) != NULL);
		free(search_str);
		if (result) {
			result = TRUE;
			break;
		}
	}
	free((void *)buf_top);
	return result;
}

/*
 * �N���b�v�{�[�h�̓��e���m�F���A�\��t�����s�����m�F�_�C�A���O���o���B
 *
 * �Ԃ�l:
 *   TRUE  -> ���Ȃ��A�\��t�������{
 *   FALSE -> �\��t�����~
 */
static BOOL CheckClipboardContentW(HWND HWin, const wchar_t *str_w, BOOL AddCR, wchar_t **out_str_w)
{
	INT_PTR ret;
	BOOL confirm = FALSE;

	*out_str_w = NULL;
	if ((ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE) == 0) {
		return TRUE;
	}

/*
 * ConfirmChangePasteCR �̋������
 * �ȉ��̓���Ŗ��Ȃ����B
 *
 *		ChangePasteCR	!ChangePasteCR
 *		AddCR	!AddCR	AddCR	!AddCR
 * ���s����	o	o	x(2)	o
 * ���s����	o(1)	x	x	x
 *
 * ChangePasteCR �� AddCR �̎��Ɋm�F���s����(1�Ŋm�F����)�Ƃ����ݒ肾���A
 * !ChangePasteCR �̎����l����ƁAAddCR �̎��͏�� CR �������Ă���̂�
 * �m�F���s��Ȃ������� 2 �̏ꍇ�ł��m�F�͕s�p�Ƃ����ӎv�\���Ƃ��Ƃ��B
 * 2 �̓���͂ǂ��炪������?
 */
	if (AddCR) {
		if (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE_CR) {
			confirm = TRUE;
		}
	}
	else {
		size_t pos = wcscspn(str_w, L"\r\n");  // ���s���܂܂�Ă�����
		if (str_w[pos] != '\0') {
			confirm = TRUE;
		}
	}

	// �������T�[�`����
	if (ts.ConfirmChangePasteStringFile[0] != '\0') {
		wchar_t *fname_rel =ToWcharA(ts.ConfirmChangePasteStringFile);
		wchar_t *fnameW = GetFullPathW(ts.HomeDirW, fname_rel);
		if (!confirm && search_dictW(fnameW, str_w)) {
			confirm = TRUE;
		}
		free(fnameW);
		free(fname_rel);
	}

	ret = IDOK;
	if (confirm) {
		clipboarddlgdata dlg_data;
		dlg_data.strW_ptr = str_w;
		dlg_data.UILanguageFileW = ts.UILanguageFileW;
		dlg_data.PasteDialogSize = ts.PasteDialogSize;
		ret = clipboarddlg(hInst, HWin, &dlg_data);
		ts.PasteDialogSize = dlg_data.PasteDialogSize;
		*out_str_w = dlg_data.strW_edited_ptr;
	}

	if (ret == IDOK) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**
 *	�N���b�v�{�[�h�p�e�L�X�g���M����
 *
 *	@param	str_w	������ւ̃|�C���^
 *					malloc()���ꂽ�o�b�t�@�A���M�������Ɏ�����free()�����
 */
static void CBSendStart(wchar_t *str_w)
{
	SendMem *sm = SendMemTextW(str_w, 0);
	if (sm == NULL) {
		free(str_w);
		return;
	}
	if (ts.PasteDelayPerLine == 0) {
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0);
	}
	else {
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_PER_LINE, ts.PasteDelayPerLine, 0);
	}
	if (ts.LocalEcho > 0) {
		SendMemInitEcho(sm, TRUE);
	}

#if 0
	SendMemInitDialog(sm, hInst, HVTWin, ts.UILanguageFile);
	SendMemInitDialogCaption(sm, L"from clipboard");
	SendMemInitDialogFilename(sm, L"Clipboard");
#endif
	SendMemStart(sm);
}

void CBPreparePaste(HWND HWin, BOOL shouldBeReady, BOOL AddCR, BOOL Bracketed, wchar_t **text)
{
	wchar_t *str_w;
	wchar_t *str_w_edited;

	if (shouldBeReady && ! cv.Ready) {
		return;
	}
	if (TalkStatus!=IdTalkKeyb) {
		return;
	}

	str_w = GetClipboardTextW(HWin, FALSE);
	if (str_w == NULL || !IsTextW(str_w, 0)) {
		// �N���b�v�{�[�h���當������擾�ł��Ȃ�����
		return;
	}

	if (ts.PasteFlag & CPF_TRIM_TRAILING_NL) {
		// �o�b�t�@�Ō�̉��s���폜
		TrimTrailingNLW(str_w);
	}

	{
		// ���s�� CR+LF �ɐ��K���A�_�C�A���O�ŉ��s�𐳂����\�����邽��
		wchar_t *dest = NormalizeLineBreakCRLF(str_w);
		free(str_w);
		str_w = dest;
	}

	if (!CheckClipboardContentW(HWin, str_w, AddCR, &str_w_edited)) {
		free(str_w);
		return;
	}
	if (str_w_edited != NULL) {
		// �_�C�A���O�ŕҏW���ꂽ
		free(str_w);
		str_w = str_w_edited;
	}

	// �u���P�b�g���邩�ǂ���
	BOOL AddBracket = FALSE;
	if (ts.BracketedSupport) {
		if (!ts.BracketedControlOnly) {
			AddBracket = TRUE;
		}
		else {
			wchar_t *c = str_w;
			while (*c) {
				if (iswcntrl(*c)) {
					AddBracket = TRUE;
					break;
				}
				c++;
			}
		}
	}

	if (AddCR) {
		size_t str_len = wcslen(str_w) + 2;
		str_w = realloc(str_w, sizeof(wchar_t) * str_len);
		str_w[str_len-2] = L'\r';
		str_w[str_len-1] = 0;
	}

	{
		// ���s�� CR �݂̂ɐ��K��
		wchar_t *dest = NormalizeLineBreakCR(str_w, 0);
		free(str_w);
		str_w = dest;
	}

	if (Bracketed && AddBracket) {
		const size_t BracketStartLenW = _countof(BracketStartW) - 1;
		const size_t BracketEndLenW = _countof(BracketEndW) - 1;
		size_t str_len = wcslen(str_w);
		size_t dest_len = str_len + BracketStartLenW + BracketEndLenW;
		wchar_t *dest = malloc(sizeof(wchar_t) * (dest_len+1));
		size_t i = 0;
		wmemcpy(&dest[i], BracketStartW, BracketStartLenW);
		i += BracketStartLenW;
		wmemcpy(&dest[i], str_w, str_len);
		i += str_len;
		wmemcpy(&dest[i], BracketEndW, BracketEndLenW);
		i += BracketEndLenW;
		dest[i] = 0;
		free(str_w);
		str_w = dest;
	}

	*text = str_w;
}

void CBStartPaste(HWND HWin, BOOL AddCR, BOOL Bracketed)
{
	wchar_t *text = NULL;
	CBPreparePaste(HWin, TRUE, AddCR, Bracketed, &text);
	if (text != NULL) {
		CBSendStart(text);
	}
}

void CBStartPasteB64(HWND HWin, PCHAR header, PCHAR footer)
{
	size_t mb_len, b64_len, header_len = 0, footer_len = 0;
	wchar_t *str_w = NULL;
	char *str_mb = NULL;
	char *str_b64 = NULL;

	if (! cv.Ready) {
		return;
	}
	if (TalkStatus!=IdTalkKeyb) {
		return;
	}

	str_w = GetClipboardTextW(HWin, FALSE);
	if (str_w == NULL || !IsTextW(str_w, 0)) {
		// �N���b�v�{�[�h���當������擾�ł��Ȃ�����
		goto error;
	}

	if (ts.Language == IdUtf8 || ts.KanjiCodeSend == IdUTF8) {
		str_mb = ToU8W(str_w);
	}
	else {
		str_mb = ToCharW(str_w);
	}

	if (str_mb == NULL) {
		goto error;
	}

	if (header != NULL) {
		header_len = strlen(header);
	}
	if (footer != NULL) {
		footer_len = strlen(footer);
	}

	mb_len = strlen(str_mb);
	b64_len = (mb_len + 2) / 3 * 4 + header_len + footer_len + 1;

	if ((str_b64 = malloc(b64_len)) == NULL) {;
		goto error;
	}

	if (header_len > 0) {
		strncpy_s(str_b64, b64_len, header, _TRUNCATE);
	}

	b64encode(str_b64 + header_len, b64_len - header_len, str_mb, mb_len);

	if (footer_len > 0) {
		strncat_s(str_b64, b64_len, footer, _TRUNCATE);
	}

	free(str_w);
	if ((str_w = ToWcharA(str_b64)) == NULL) {
		goto error;
	}

	free(str_mb);
	free(str_b64);

	// �\��t���̏���������ɏo����
	CBSendStart(str_w);

	return;

error:
	free(str_w);
	free(str_mb);
	free(str_b64);
	return;
}
