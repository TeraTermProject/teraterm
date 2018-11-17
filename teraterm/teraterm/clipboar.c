/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006-2017 TeraTerm Project
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

#include "ttwinman.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"

#include "clipboar.h"
#include "tt_res.h"

// for clipboard copy
static HGLOBAL CBCopyHandle = NULL;
static PCHAR CBCopyPtr = NULL;
static HGLOBAL CBCopyWideHandle = NULL;
static LPWSTR CBCopyWidePtr = NULL;

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

static LRESULT CALLBACK OnClipboardDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);

PCHAR CBOpen(LONG MemSize)
{
	if (MemSize==0) {
		return (NULL);
	}
	if (CBCopyHandle!=NULL) {
		return (NULL);
	}
	CBCopyPtr = NULL;
	CBCopyHandle = GlobalAlloc(GMEM_MOVEABLE, MemSize);
	if (CBCopyHandle == NULL) {
		MessageBeep(0);
	}
	else {
		CBCopyPtr = GlobalLock(CBCopyHandle);
		if (CBCopyPtr == NULL) {
			GlobalFree(CBCopyHandle);
			CBCopyHandle = NULL;
			MessageBeep(0);
		}
	}
	return (CBCopyPtr);
}

void CBClose()
{
	BOOL Empty;
	int WideCharLength;

	if (CBCopyHandle==NULL) {
		return;
	}

	WideCharLength = MultiByteToWideChar(CP_ACP, 0, CBCopyPtr, -1, NULL, 0);
	CBCopyWideHandle = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * WideCharLength);
	if (CBCopyWideHandle) {
		CBCopyWidePtr = (LPWSTR)GlobalLock(CBCopyWideHandle);
		MultiByteToWideChar(CP_ACP, 0, CBCopyPtr, -1, CBCopyWidePtr, WideCharLength);
		GlobalUnlock(CBCopyWideHandle);
	}

	Empty = FALSE;
	if (CBCopyPtr!=NULL) {
		Empty = (CBCopyPtr[0]==0);
	}

	GlobalUnlock(CBCopyHandle);
	CBCopyPtr = NULL;

	if (OpenClipboard(HVTWin)) {
		EmptyClipboard();
		if (! Empty) {
			SetClipboardData(CF_TEXT, CBCopyHandle);
			if (CBCopyWidePtr) {
				SetClipboardData(CF_UNICODETEXT, CBCopyWideHandle);
				CBCopyWidePtr = NULL;
			}
		}
		CloseClipboard();
	}
	CBCopyHandle = NULL;
	CBCopyWideHandle = NULL;
}

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

// クリップボードバッファの末尾にある CR / LF をすべて削除する
BOOL TrimTrailingNL(BOOL AddCR, BOOL Bracketed) {
	PCHAR tail;
	if (ts.PasteFlag & CPF_TRIM_TRAILING_NL) {
		for (tail = CBMemPtr+strlen(CBMemPtr)-1; tail >= CBMemPtr; tail--) {
			if (*tail != '\r' && *tail != '\n') {
				break;
			}
			*tail = '\0';
		}
	}

	return TRUE;
}

// 改行を CR+LF に正規化する
BOOL NormalizeLineBreak(BOOL AddCR, BOOL Bracketed) {
	char *p, *p2;
	unsigned int len, need_len, alloc_len;
	HGLOBAL TmpHandle;

	if (!(ts.PasteFlag & CPF_NORMALIZE_LINEBREAK)) {
		return TRUE;
	}

	p = CBMemPtr;

	// 貼り付けデータの長さ(len)、および正規化後のデータの長さ(need_len)のカウント
	for (len=0, need_len=0, p=CBMemPtr; *p != '\0'; p++, len++, need_len++) {
		if (*p == CR) {
			need_len++;
			if (*(p+1) == LF) {
				len++;
				p++;
			}
		}
		else if (*p == LF) {
			need_len++;
		}
	}

	// 正規化後もデータ長が変わらない => 正規化は必要なし
	if (need_len == len) {
		return TRUE;
	}

	// AddCR / Bracketed の時はその分のバッファも計算に入れる
	// あまりここではやりたくないんだけれど
	alloc_len = need_len + 1;
	if (AddCR) {
		alloc_len++;
	}
	if (Bracketed) {
		// 手抜き
		alloc_len += 12;
	}

	// バッファサイズが正規化後に必要となる値より小さい場合はバッファを確保し直す
	if (GlobalSize(CBMemHandle) < alloc_len) {
		GlobalUnlock(CBMemHandle);
		CBMemPtr = NULL;
		if ((TmpHandle = GlobalReAlloc(CBMemHandle, alloc_len, 0)) == NULL) {
			// メモリ再割り当て失敗
			CBMemPtr = GlobalLock(CBMemHandle);

			// とりあえず正規化なしで貼り付ける事にする
			return TRUE;
		}
		CBMemHandle = TmpHandle;
		CBMemPtr = GlobalLock(CBMemHandle);
	}

	p = CBMemPtr + len - 1;
	p2 = CBMemPtr + need_len;
	*p2-- = '\0';

	while (len > 0 && p < p2) {
		if (*p == LF) {
			*p2-- = *p--;
			if (--len == 0) {
				*p2 = CR;
				break;
			}
			if (*p != CR) {
				*p2-- = CR;
				if (p2 <= p) {
					break;
				}
				else {
					continue;
				}
			}
		}
		else if (*p == CR) {
			*p2-- = LF;
			if (p == p2)
				break;
		}
		*p2-- = *p--;
		len--;
	}

	return TRUE;
}

// ファイルに定義された文字列が、textに含まれるかを調べる。
BOOL search_dict(char *filename, char *text)
{
	BOOL ret = FALSE;
	FILE *fp = NULL;
	char buf[256];
	int len;

	if (filename == NULL || filename[0] == '\0')
		return FALSE;

	if ((fp = fopen(filename, "r")) == NULL)
		return FALSE;

	// TODO: 一行が256byteを超えている場合の考慮
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		len = strlen(buf);
		if (len <= 1) {
			continue;
		}
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		}
		if (strstr(text, buf)) { // hit
			ret = 1;
			break;
		}
	}

	fclose(fp);

	return (ret);
}

/*
 * クリップボードの内容を確認し、貼り付けを行うか確認ダイアログを出す。
 *
 * 返り値:
 *   TRUE  -> 問題なし、貼り付けを実施
 *   FALSE -> 貼り付け中止
 */
BOOL CheckClipboardContent(HWND HWin, BOOL AddCR, BOOL Bracketed)
{
	int pos;
	int ret = IDOK;
	BOOL confirm = FALSE;

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
		pos = strcspn(CBMemPtr, "\r\n");  // 改行が含まれていたら
		if (CBMemPtr[pos] != '\0') {
			confirm = TRUE;
		}
	}

	// 辞書をサーチする
	if (!confirm && search_dict(ts.ConfirmChangePasteStringFile, CBMemPtr)) {
		confirm = TRUE;
	}

	if (confirm) {
		ret = TTDialogBox(hInst, MAKEINTRESOURCE(IDD_CLIPBOARD_DIALOG),
						  HWin, (DLGPROC)OnClipboardDlgProc);
		/*
		 * 以前はダイアログの内容をクリップボードに書き戻していたけれど、必要?
		 */
	}

	if (ret == IDOK) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void CBStartPaste(HWND HWin, BOOL AddCR, BOOL Bracketed)
{
	UINT Cf;
	PCHAR TmpPtr;
	LPWSTR TmpPtrW;
	HGLOBAL TmpHandle;
	unsigned int StrLen = 0, BuffLen = 0;

	if (! cv.Ready) {
		return;
	}
	if (TalkStatus!=IdTalkKeyb) {
		return;
	}

	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		Cf = CF_UNICODETEXT;
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		Cf = CF_TEXT;
	}
	else if (IsClipboardFormatAvailable(CF_OEMTEXT)) {
		Cf = CF_OEMTEXT;
	}
	else {
		return;
	}

	CBEchoOnly = FALSE;

	if (CBMemHandle) {
		GlobalFree(CBMemHandle);
	}
	CBMemHandle = NULL;
	CBMemPtr = NULL;
	CBMemPtr2 = 0;

	if (ts.PasteDelayPerLine > 0) {
		CBInsertDelay = TRUE;
	}
	else {
		CBInsertDelay = FALSE;
	}

	CBRetrySend = FALSE;
	CBRetryEcho = FALSE;
	CBSendCR = FALSE;

	if (OpenClipboard(HWin)) {
		if ((TmpHandle = GetClipboardData(Cf)) != NULL) {
			if (Cf == CF_UNICODETEXT) {
				TmpPtrW = (LPWSTR)GlobalLock(TmpHandle);
				BuffLen = WideCharToMultiByte(CP_ACP, 0, TmpPtrW, -1, 0, 0, NULL, NULL);
			}
			else {
				TmpPtr = (PCHAR)GlobalLock(TmpHandle);
				BuffLen = strlen(TmpPtr) + 1;
			}

			if (Bracketed) {
				BuffLen += BracketStartLen + BracketEndLen;
			}

			if (AddCR) {
				BuffLen++;
			}

			if ((CBMemHandle = GlobalAlloc(GHND, BuffLen)) != NULL) {
				if ((CBMemPtr = GlobalLock(CBMemHandle)) != NULL) {
					if (Cf == CF_UNICODETEXT) {
						WideCharToMultiByte(CP_ACP, 0, TmpPtrW, -1, CBMemPtr, BuffLen, NULL, NULL);
					}
					else {
						strncpy_s(CBMemPtr, BuffLen, TmpPtr, _TRUNCATE);
					}

					TalkStatus = IdTalkCB;
				}
			}
			GlobalUnlock(TmpHandle);
		}
		CloseClipboard();
	}

	// 貼り付けの準備が正常に出来た場合は IdTalkCB となる

	if (TalkStatus != IdTalkCB) {
		// 準備が行えなかった場合は貼り付けを中断する
		CBEndPaste();
		return;
	}

	// 貼り付け前にクリップボードの内容を確認/加工等する場合はここで行う

	if (!TrimTrailingNL(AddCR, Bracketed)) {
		CBEndPaste();
		return;
	}

	if (!NormalizeLineBreak(AddCR, Bracketed)) {
		CBEndPaste();
		return;
	}

	if (!CheckClipboardContent(HWin, AddCR, Bracketed)) {
		CBEndPaste();
		return;
	}

	// AddCR / Bracket 用の領域があるかの確認、無ければ追加確保
	StrLen = strlen(CBMemPtr);
	BuffLen = StrLen + 1; // strlen + NUL
	if (AddCR) {
		BuffLen++;
	}
	if (Bracketed) {
		BuffLen += BracketStartLen + BracketEndLen;
	}

	if (GlobalSize(CBMemHandle) < BuffLen) {
		GlobalUnlock(CBMemHandle);
		CBMemPtr = NULL;
		if ((TmpHandle = GlobalReAlloc(CBMemHandle, BuffLen, 0)) == NULL) {
			/*
			 * 不足分の確保失敗した時は CR/Bracket 無しで貼り付けを行うべきか、
			 * それとも貼り付け自体を中止する(CBEndPaste()を呼ぶ)べきか。
			 */
			// CBEndPaste();
			return;
		}
		CBMemHandle = TmpHandle;
		CBMemPtr = GlobalLock(CBMemHandle);
	}

	if (AddCR) {
		CBMemPtr[StrLen++] = '\r';
		CBMemPtr[StrLen++] = 0;
	}

	if (Bracketed) {
		BuffLen = GlobalSize(CBMemHandle);
		memmove_s(CBMemPtr+BracketStartLen, BuffLen-BracketStartLen, CBMemPtr, StrLen);
		memcpy_s(CBMemPtr, BuffLen, BracketStart, BracketStartLen);
		strncat_s(CBMemPtr, BuffLen, BracketEnd, _TRUNCATE);
	}

	GlobalUnlock(CBMemHandle);
	CBMemPtr = NULL;
}

void CBStartPasteB64(HWND HWin, PCHAR header, PCHAR footer)
{
	HANDLE tmpHandle = NULL;
	char *tmpPtr = NULL;
	int len, header_len, footer_len, b64_len;
	UINT Cf;
	LPWSTR tmpPtrWide = NULL;

	if (! cv.Ready) {
		return;
	}
	if (TalkStatus!=IdTalkKeyb) {
		return;
	}

	CBEchoOnly = FALSE;

	if (CBMemHandle) {
		GlobalFree(CBMemHandle);
	}
	CBMemHandle = NULL;
	CBMemPtr = NULL;
	CBMemPtr2 = 0;

	if (ts.PasteDelayPerLine > 0) {
		CBInsertDelay = TRUE;
	}
	else {
		CBInsertDelay = FALSE;
	}

	CBRetrySend = FALSE;
	CBRetryEcho = FALSE;
	CBSendCR = FALSE;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(HWin)) {
		Cf = CF_UNICODETEXT;
		if ((tmpHandle = GetClipboardData(CF_UNICODETEXT)) == NULL) {
			CloseClipboard();
		}
	}
	else if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(HWin)) {
		Cf = CF_TEXT;
		if ((tmpHandle = GetClipboardData(CF_TEXT)) == NULL) {
			CloseClipboard();
		}
	}
	else if (IsClipboardFormatAvailable(CF_OEMTEXT) && OpenClipboard(HWin)) {
		Cf = CF_OEMTEXT;
		if ((tmpHandle = GetClipboardData(CF_OEMTEXT)) == NULL) {
			CloseClipboard();
		}
	}

	if (tmpHandle) {
		if (Cf == CF_UNICODETEXT) {
			if ((tmpPtrWide = GlobalLock(tmpHandle)) != NULL) {
				len = WideCharToMultiByte(CP_ACP, 0, tmpPtrWide, -1, 0, 0, NULL, NULL);
				if ((tmpPtr = (char *)calloc(sizeof(char), len)) != NULL) {
					WideCharToMultiByte(CP_ACP, 0, tmpPtrWide, -1, tmpPtr, len, NULL, NULL);
				}
				// WideCharToMultiByte で得られるのは末尾の \0 込みの長さ
				// \0 をエンコード対象に含めない為に 1 減らす
				len--;
				GlobalUnlock(tmpHandle);
			}
		}
		else {
			if ((tmpPtr = GlobalLock(tmpHandle)) != NULL) {
				len = strlen(tmpPtr);
			}
		}

		if (tmpPtr) {
			header_len = strlen(header);
			footer_len = strlen(footer);

			b64_len = (len + 2) / 3 * 4 + header_len + footer_len + 1;

			if ((CBMemHandle = GlobalAlloc(GHND, b64_len)) != NULL) {
				if ((CBMemPtr = GlobalLock(CBMemHandle)) != NULL) {
					if (header_len > 0) {
						strncpy_s(CBMemPtr, b64_len, header, _TRUNCATE);
					}
					b64encode(CBMemPtr + header_len, b64_len - header_len, tmpPtr, len);
					if (footer_len > 0) {
						strncat_s(CBMemPtr, b64_len, footer, _TRUNCATE);
					}
					TalkStatus=IdTalkCB;
					GlobalUnlock(CBMemPtr);
					CBMemPtr = NULL;
				}
			}

			if (Cf == CF_UNICODETEXT) {
				free(tmpPtr);
			}
			else {
				GlobalUnlock(tmpHandle);
			}
		}
		CloseClipboard();
	}

	if (TalkStatus != IdTalkCB) {
		CBEndPaste();
	}
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

void CBEcho()
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
}

BOOL CBSetClipboard(HWND owner, HGLOBAL hMem)
{
	char *buf;
	int wide_len;
	HGLOBAL wide_hMem;
	LPWSTR wide_buf = 0;

	if (OpenClipboard(owner) == 0)
		return FALSE;

	buf = GlobalLock(hMem);

	wide_len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
	wide_hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * wide_len);
	if (wide_hMem) {
		wide_buf = (LPWSTR)GlobalLock(wide_hMem);
		MultiByteToWideChar(CP_ACP, 0, buf, -1, wide_buf, wide_len);
		GlobalUnlock(wide_hMem);
	}

	GlobalUnlock(hMem);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	if (wide_buf) {
		SetClipboardData(CF_UNICODETEXT, wide_hMem);
	}
	CloseClipboard();

	return TRUE;
}

HGLOBAL CBAllocClipboardMem(char *text)
{
	HGLOBAL hMem;
	char *buf;
	int len;

	len = strlen(text);

	hMem = GlobalAlloc(GMEM_MOVEABLE, len+1);
	if (hMem) {
		buf = GlobalLock(hMem);
		strncpy_s(buf, len+1, text, _TRUNCATE);
		GlobalUnlock(hMem);
	}

	return hMem;
}

static LRESULT CALLBACK OnClipboardDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_CLIPBOARD_TITLE" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDOK, "BTN_OK" },
	};
	POINT p;
	RECT rc_dsk, rc_dlg;
	int dlg_height, dlg_width;
	static int ok2right, edit2ok, edit2bottom;
	RECT rc_edit, rc_ok, rc_cancel;
	// for status bar
	static HWND hStatus = NULL;
	static int init_width, init_height;

	switch (msg) {
		case WM_INITDIALOG:
			SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);

			SetDlgItemTextA(hDlgWnd, IDC_EDIT, CBMemPtr);

			if (ActiveWin == IdVT) { // VT Window
				/*
				 * Caret off 時に GetCaretPos() で正確な場所が取れないので、
				 * vtdisp.c 内部で管理している値から計算する
				 */
				DispConvScreenToWin(CursorX, CursorY, &p.x, &p.y);
			}
			else if (!GetCaretPos(&p)) { // Tek Window
				/*
				 * Tek Window は内部管理の値を取るのが面倒なので GetCaretPos() を使う
				 * GetCaretPos() がエラーになった場合は念のため 0, 0 を入れておく
				 */
				p.x = 0;
				p.y = 0;
			}

			// x, y の両方が 0 の時は親ウィンドウの中央に移動させられるので、
			// それを防ぐ為に x を 1 にする
			if (p.x == 0 && p.y == 0) {
				p.x = 1;
			}

			ClientToScreen(GetParent(hDlgWnd), &p);

			// キャレットが画面からはみ出しているときに貼り付けをすると
			// 確認ウインドウが見えるところに表示されないことがある。
			// ウインドウからはみ出した場合に調節する (2008.4.24 maya)
			if (!HasMultiMonitorSupport()) {
				// NT4.0, 95 はマルチモニタAPIに非対応
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc_dsk, 0);
			}
			else {
				HMONITOR hm;
				POINT pt;
				MONITORINFO mi;

				pt.x = p.x;
				pt.y = p.y;
				hm = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

				mi.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hm, &mi);
				rc_dsk = mi.rcWork;
			}
			GetWindowRect(hDlgWnd, &rc_dlg);
			dlg_height = rc_dlg.bottom-rc_dlg.top;
			dlg_width  = rc_dlg.right-rc_dlg.left;
			if (p.y < rc_dsk.top) {
				p.y = rc_dsk.top;
			}
			else if (p.y + dlg_height > rc_dsk.bottom) {
				p.y = rc_dsk.bottom - dlg_height;
			}
			if (p.x < rc_dsk.left) {
				p.x = rc_dsk.left;
			}
			else if (p.x + dlg_width > rc_dsk.right) {
				p.x = rc_dsk.right - dlg_width;
			}

			SetWindowPos(hDlgWnd, NULL, p.x, p.y,
			             0, 0, SWP_NOSIZE | SWP_NOZORDER);

			// ダイアログの初期サイズを保存
			GetWindowRect(hDlgWnd, &rc_dlg);
			init_width = rc_dlg.right - rc_dlg.left;
			init_height = rc_dlg.bottom - rc_dlg.top;

			// 現在サイズから必要な値を計算
			GetClientRect(hDlgWnd,                                 &rc_dlg);
			GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
			GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);

			p.x = rc_dlg.right;
			p.y = rc_dlg.bottom;
			ClientToScreen(hDlgWnd, &p);
			ok2right = p.x - rc_ok.left;
			edit2bottom = p.y - rc_edit.bottom;
			edit2ok = rc_ok.left - rc_edit.right;

			// サイズを復元
			SetWindowPos(hDlgWnd, NULL, 0, 0,
			             ts.PasteDialogSize.cx, ts.PasteDialogSize.cy,
			             SWP_NOZORDER | SWP_NOMOVE);

			// リサイズアイコンを右下に表示させたいので、ステータスバーを付ける。
			InitCommonControls();
			hStatus = CreateStatusWindow(
				WS_CHILD | WS_VISIBLE |
				CCS_BOTTOM | SBARS_SIZEGRIP, NULL, hDlgWnd, 1);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
				{
					unsigned len = SendMessageA(GetDlgItem(hDlgWnd, IDC_EDIT), WM_GETTEXTLENGTH, 0, 0);
					HGLOBAL hMem;
					INT_PTR result = IDCANCEL;

					if (CBMemHandle == NULL) {
						CBMemHandle = GlobalAlloc(GHND, len+1);
					}
					else if (GlobalSize(CBMemHandle) <= len) {
						if (CBMemPtr) {
							GlobalUnlock(CBMemHandle);
							CBMemPtr = NULL;
						}
						hMem = GlobalReAlloc(CBMemHandle, len+1, 0);
						if (hMem) {
							CBMemHandle = hMem;
							CBMemPtr = GlobalLock(CBMemHandle);
						}
						else {
							/*
							 * メモリが確保できなかった場合はどうするべきか。
							 *
							 * ダイアログで書き換えが行われた場合を考えると
							 * キャンセル扱いにする方が無難だが、大抵は書き換えを
							 * 行わないと思われるので、その場合は旧領域の内容を
							 * 貼り付けた方が親切。
							 *
							 * 取りあえずは安全側に倒し、旧領域を開放して貼り付けが
							 * 行われないようにする。
							 */
							GlobalFree(CBMemHandle);
							CBMemHandle = NULL;
						}
					}

					if (CBMemHandle) {
						if (CBMemPtr == NULL) {
							CBMemPtr = GlobalLock(CBMemHandle);
						}
						GetDlgItemTextA(hDlgWnd, IDC_EDIT, CBMemPtr, GlobalSize(CBMemHandle));
						result = IDOK;
					}

					DestroyWindow(hStatus);
					TTEndDialog(hDlgWnd, result);
				}
					break;

				case IDCANCEL:
					DestroyWindow(hStatus);
					TTEndDialog(hDlgWnd, IDCANCEL);
					break;

				default:
					return FALSE;
			}

		case WM_SIZE:
			{
				// 再配置
				POINT p;
				int dlg_w, dlg_h;

				GetClientRect(hDlgWnd,                                 &rc_dlg);
				dlg_w = rc_dlg.right;
				dlg_h = rc_dlg.bottom;

				GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT),           &rc_edit);
				GetWindowRect(GetDlgItem(hDlgWnd, IDOK),               &rc_ok);
				GetWindowRect(GetDlgItem(hDlgWnd, IDCANCEL),           &rc_cancel);

				// OK
				p.x = rc_ok.left;
				p.y = rc_ok.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDOK), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// CANCEL
				p.x = rc_cancel.left;
				p.y = rc_cancel.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDCANCEL), 0,
				             dlg_w - ok2right, p.y, 0, 0,
				             SWP_NOSIZE | SWP_NOZORDER);

				// EDIT
				p.x = rc_edit.left;
				p.y = rc_edit.top;
				ScreenToClient(hDlgWnd, &p);
				SetWindowPos(GetDlgItem(hDlgWnd, IDC_EDIT), 0,
				             0, 0, dlg_w - p.x - edit2ok - ok2right, dlg_h - p.y - edit2bottom,
				             SWP_NOMOVE | SWP_NOZORDER);

				// サイズを保存
				GetWindowRect(hDlgWnd, &rc_dlg);
				ts.PasteDialogSize.cx = rc_dlg.right - rc_dlg.left;
				ts.PasteDialogSize.cy = rc_dlg.bottom - rc_dlg.top;

				// status bar
				SendMessage(hStatus , msg , wp , lp);
			}
			return TRUE;

		case WM_GETMINMAXINFO:
			{
				// ダイアログの初期サイズより小さくできないようにする
				LPMINMAXINFO lpmmi;
				lpmmi = (LPMINMAXINFO)lp;
				lpmmi->ptMinTrackSize.x = init_width;
				lpmmi->ptMinTrackSize.y = init_height;
			}
			return FALSE;

		default:
			return FALSE;
	}
	return TRUE;
}
