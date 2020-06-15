/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2020 TeraTerm Project
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
#include "vtdisp.h"
#include "vtterm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <wchar.h>

#include "ttwinman.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"
#include "codeconv.h"

#include "clipboar.h"
#include "tt_res.h"
#include "fileread.h"
#include "sendmem.h"
#include "clipboarddlg.h"

// for clipboard paste
static HGLOBAL CBMemHandle = NULL;
static PCHAR CBMemPtr = NULL;
static LONG CBMemPtr2 = 0;
static BYTE CBByte;
static BOOL CBRetrySend;
static BOOL CBRetryEcho;
static BOOL CBSendCR;
static BOOL CBEchoOnly;
static BOOL CBInsertDelay = FALSE;

static const wchar_t BracketStartW[] = L"\033[200~";
static const wchar_t BracketEndW[] = L"\033[201~";

static void CBEcho(void);

/**
 * 文字列送信
 * 次のところから使用されている
 *  - DDE(TTL)
 *	- ブロードキャスト
 */
void CBStartSend(PCHAR DataPtr, int DataSize, BOOL EchoOnly)
{
	if (! cv.Ready) {
		return;
	}
	if (TalkStatus!=IdTalkKeyb) {
		return;
	}

	CBEchoOnly = EchoOnly;

	if (CBMemHandle) {
		GlobalFree(CBMemHandle);
	}
	CBMemHandle = NULL;
	CBMemPtr = NULL;
	CBMemPtr2 = 0;

	CBInsertDelay = FALSE;

	CBRetrySend = FALSE;
	CBRetryEcho = FALSE;
	CBSendCR = FALSE;

	if ((CBMemHandle = GlobalAlloc(GHND, DataSize+1)) != NULL) {
		if ((CBMemPtr = GlobalLock(CBMemHandle)) != NULL) {
			memcpy(CBMemPtr, DataPtr, DataSize);
			// WM_COPYDATA で送られて来たデータは NUL Terminate されていないので NUL を付加する
			CBMemPtr[DataSize] = 0;
			GlobalUnlock(CBMemHandle);
			CBMemPtr=NULL;
			TalkStatus=IdTalkCB;
		}
	}
	if (TalkStatus != IdTalkCB) {
		CBEndPaste();
	}
}

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
 *	改行コードを CR+LF に変換する
 *	@return 変換された文字列
 */
static wchar_t *NormalizeLineBreakW(const wchar_t *src_)
{
	const wchar_t *src = src_;
	wchar_t *dest_top;
	wchar_t *dest;
	size_t len, need_len, alloc_len;

	// 貼り付けデータの長さ(len)、および正規化後のデータの長さ(need_len)のカウント
	for (len=0, need_len=0, src=src_; *src != '\0'; src++, len++, need_len++) {
		if (*src == CR) {
			need_len++;
			if (*(src+1) == LF) {
				len++;
				src++;
			}
		}
		else if (*src == LF) {
			need_len++;
		}
	}

	// 正規化後もデータ長が変わらない => 正規化は必要なし
	if (need_len == len) {
		dest = _wcsdup(src_);
		return dest;
	}
	alloc_len = need_len + 1;

	dest_top = (wchar_t *)malloc(sizeof(wchar_t) * alloc_len);

	src = src_ + len - 1;
	dest = dest_top + need_len;
	*dest-- = '\0';

	while (len > 0 && dest_top <= dest) {
		if (*src == LF) {
			*dest-- = *src--;
			if (--len == 0) {
				*dest = CR;
				break;
			}
			if (*src != CR) {
				*dest-- = CR;
				continue;
			}
		}
		else if (*src == CR) {
			*dest-- = LF;
			if (src == dest)
				break;
		}
		*dest-- = *src--;
		len--;
	}

	return dest_top;
}

/**
 * ファイルに定義された文字列が、textに含まれるかを調べる。
 * 見つかれば TRUEを返す
 */
static BOOL search_dictW(char *filename, const wchar_t *text)
{
	BOOL result = FALSE;
	const wchar_t *buf_top = LoadFileWA(filename, NULL);
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
			// 改行がない
			len = wcslen(buf);
			if (len == 0) {
				// 終了
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
 * クリップボードの内容を確認し、貼り付けを行うか確認ダイアログを出す。
 *
 * 返り値:
 *   TRUE  -> 問題なし、貼り付けを実施
 *   FALSE -> 貼り付け中止
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
 * ConfirmChangePasteCR の挙動問題
 * 以下の動作で問題ないか。
 *
 *		ChangePasteCR	!ChangePasteCR
 *		AddCR	!AddCR	AddCR	!AddCR
 * 改行あり	o	o	x(2)	o
 * 改行無し	o(1)	x	x	x
 *
 * ChangePasteCR は AddCR の時に確認を行うか(1で確認する)という設定だが、
 * !ChangePasteCR の時を考えると、AddCR の時は常に CR が入っているのに
 * 確認を行わない事から 2 の場合でも確認は不用という意思表示ともとれる。
 * 2 の動作はどちらがいいか?
 */
	if (AddCR) {
		if (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE_CR) {
			confirm = TRUE;
		}
	}
	else {
		size_t pos = wcscspn(str_w, L"\r\n");  // 改行が含まれていたら
		if (str_w[pos] != '\0') {
			confirm = TRUE;
		}
	}

	// 辞書をサーチする
	if (!confirm && search_dictW(ts.ConfirmChangePasteStringFile, str_w)) {
		confirm = TRUE;
	}

	ret = IDOK;
	if (confirm) {
		clipboarddlgdata dlg_data;
		dlg_data.strW_ptr = str_w;
		dlg_data.UILanguageFile = ts.UILanguageFile;
		ret = clipboarddlg(hInst, HWin, &dlg_data);
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
 *	クリップボード用テキスト送信する
 *
 *	@param	str_w	文字列へのポインタ
 *					malloc()されたバッファ、送信完了時に自動でfree()される
 */
static void CBSendStart(wchar_t *str_w)
{
	SendMem *sm = SendMemTextW(str_w, 0);
	if (sm == NULL)
		return;
	if (ts.PasteDelayPerLine == 0) {
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_NO_DELAY, 0, 0);
	}
	else {
		SendMemInitDelay(sm, SENDMEM_DELAYTYPE_PER_LINE, ts.PasteDelayPerLine, 0);
	}
#if 0
	SendMemInitDialog(sm, hInst, HVTWin, ts.UILanguageFile);
	SendMemInitDialogCaption(sm, L"from clipboard");
	SendMemInitDialogFilename(sm, L"Clipboard");
#endif
	SendMemStart(sm);
}

void CBStartPaste(HWND HWin, BOOL AddCR, BOOL Bracketed)
{
//	unsigned int StrLen = 0;
	wchar_t *str_w;
	wchar_t *str_w_edited;

	if (! cv.Ready) {
		return;
	}
	if (TalkStatus!=IdTalkKeyb) {
		return;
	}

	CBEchoOnly = FALSE;

	str_w = GetClipboardTextW(HWin, FALSE);
	if (str_w == NULL || !IsTextW(str_w, 0)) {
		// クリップボードから文字列を取得できなかった
		CBEndPaste();
		return;
	}

	if (ts.PasteFlag & CPF_TRIM_TRAILING_NL) {
		// バッファ最後の改行を削除
		TrimTrailingNLW(str_w);
	}

	{
		// 改行を正規化
		wchar_t *dest = NormalizeLineBreakW(str_w);
		free(str_w);
		str_w = dest;
	}

	if (!CheckClipboardContentW(HWin, str_w, AddCR, &str_w_edited)) {
		free(str_w);
		CBEndPaste();
		return;
	}
	if (str_w_edited != NULL) {
		// ダイアログで編集された
		free(str_w);
		str_w = str_w_edited;
	}

	if (AddCR) {
		size_t str_len = wcslen(str_w) + 2;
		str_w = realloc(str_w, sizeof(wchar_t) * str_len);
		str_w[str_len-2] = L'\r';
		str_w[str_len-1] = 0;
	}

	if (Bracketed) {
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

	CBSendStart(str_w);
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

	CBEchoOnly = FALSE;

	str_w = GetClipboardTextW(HWin, FALSE);
	if (str_w == NULL || !IsTextW(str_w, 0)) {
		// クリップボードから文字列を取得できなかった
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

	// 貼り付けの準備が正常に出来た
	CBSendStart(str_w);

	return;

error:
	free(str_w);
	free(str_mb);
	free(str_b64);
	CBEndPaste();
	return;
}

// この関数はクリップボードおよびDDEデータを端末へ送り込む。
//
// CBMemHandleハンドルはグローバル変数なので、この関数が終了するまでは、
// 次のクリップボードおよびDDEデータを処理することはできない（破棄される可能性あり）。
// また、データ列で null-terminate されていることを前提としているため、後続のデータ列は
// 無視される。
// (2006.11.6 yutaka)
void CBSend()
{
	int c;
	BOOL EndFlag;
	static DWORD lastcr;
	DWORD now;

	if (CBMemHandle==NULL) {
		return;
	}

	if (CBEchoOnly) {
		CBEcho();
		return;
	}

	if (CBInsertDelay) {
		now = GetTickCount();
		if (now - lastcr < (DWORD)ts.PasteDelayPerLine) {
			return;
		}
	}

	if (CBRetrySend) {
		CBRetryEcho = (ts.LocalEcho>0);
		c = CommTextOut(&cv,(PCHAR)&CBByte,1);
		CBRetrySend = (c==0);
		if (CBRetrySend) {
			return;
		}
	}

	if (CBRetryEcho) {
		c = CommTextEcho(&cv,(PCHAR)&CBByte,1);
		CBRetryEcho = (c==0);
		if (CBRetryEcho) {
			return;
		}
	}

	CBMemPtr = GlobalLock(CBMemHandle);
	if (CBMemPtr==NULL) {
		return;
	}

	do {
		if (CBSendCR && (CBMemPtr[CBMemPtr2]==0x0a)) {
			CBMemPtr2++;
			// added PasteDelayPerLine (2009.4.12 maya)
			if (CBInsertDelay) {
				lastcr = now;
				CBSendCR = FALSE;
				SetTimer(HVTWin, IdPasteDelayTimer, ts.PasteDelayPerLine, NULL);
				break;
			}
		}

		EndFlag = (CBMemPtr[CBMemPtr2]==0);
		if (! EndFlag) {
			CBByte = CBMemPtr[CBMemPtr2];
			CBMemPtr2++;
// Decoding characters which are encoded by MACRO
//   to support NUL character sending
//
//  [encoded character] --> [decoded character]
//         01 01        -->     00
//         01 02        -->     01
			if (CBByte==0x01) { /* 0x01 from MACRO */
				CBByte = CBMemPtr[CBMemPtr2];
				CBMemPtr2++;
				CBByte = CBByte - 1; // character just after 0x01
			}
		}
		else {
			CBEndPaste();
			return;
		}

		if (! EndFlag) {
			c = CommTextOut(&cv,(PCHAR)&CBByte,1);
			CBSendCR = (CBByte==0x0D);
			CBRetrySend = (c==0);
			if ((! CBRetrySend) &&
			    (ts.LocalEcho>0)) {
				c = CommTextEcho(&cv,(PCHAR)&CBByte,1);
				CBRetryEcho = (c==0);
			}
		}
		else {
			c=0;
		}
	}
	while (c>0);

	if (CBMemPtr!=NULL) {
		GlobalUnlock(CBMemHandle);
		CBMemPtr=NULL;
	}
}

static void CBEcho()
{
	if (CBMemHandle==NULL) {
		return;
	}

	if (CBRetryEcho && CommTextEcho(&cv,(PCHAR)&CBByte,1) == 0) {
		return;
	}

	if ((CBMemPtr = GlobalLock(CBMemHandle)) == NULL) {
		return;
	}

	do {
		if (CBSendCR && (CBMemPtr[CBMemPtr2]==0x0a)) {
			CBMemPtr2++;
		}

		if (CBMemPtr[CBMemPtr2] == 0) {
			CBRetryEcho = FALSE;
			CBEndPaste();
			return;
		}

		CBByte = CBMemPtr[CBMemPtr2];
		CBMemPtr2++;

		// Decoding characters which are encoded by MACRO
		//   to support NUL character sending
		//
		//  [encoded character] --> [decoded character]
		//         01 01        -->     00
		//         01 02        -->     01
		if (CBByte==0x01) { /* 0x01 from MACRO */
			CBByte = CBMemPtr[CBMemPtr2];
			CBMemPtr2++;
			CBByte = CBByte - 1; // character just after 0x01
		}

		CBSendCR = (CBByte==0x0D);

	} while (CommTextEcho(&cv,(PCHAR)&CBByte,1) > 0);

	CBRetryEcho = TRUE;

	if (CBMemHandle != NULL) {
		GlobalUnlock(CBMemHandle);
		CBMemPtr=NULL;
	}
}

void CBEndPaste()
{
	TalkStatus = IdTalkKeyb;

	if (CBMemHandle!=NULL) {
		if (CBMemPtr!=NULL) {
			GlobalUnlock(CBMemHandle);
		}
		GlobalFree(CBMemHandle);
	}

	CBMemHandle = NULL;
	CBMemPtr = NULL;
	CBMemPtr2 = 0;
	CBEchoOnly = FALSE;
	CBInsertDelay = FALSE;
	_CrtCheckMemory();
}

