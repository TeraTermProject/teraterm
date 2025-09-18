/*
 * Copyright (c) 1998-2001, Robert O'Callahan
 * (C) 2004- TeraTerm Project
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

/*
This code is copyright (C) 1998-1999 Robert O'Callahan.
See LICENSE.TXT for the license.
*/

#include "ttxssh.h"
#include "codeconv.h"

void UTIL_init_sock_write_buf(UTILSockWriteBuf *buf)
{
	buf_create(&buf->bufdata, &buf->buflen);
	buf->datastart = 0;
	buf->datalen = 0;
}

static int send_until_block(PTInstVar pvar, SOCKET s,
                            const char *data, int len)
{
	int total_sent = 0;

	while (len > 0) {
		int sent_amount = (pvar->Psend) (s, data, len, 0);

		if (sent_amount < 0) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				return total_sent;
			} else {
				return sent_amount;
			}
		} else {
			total_sent += sent_amount;
			data += sent_amount;
			len -= sent_amount;
		}
	}

	return total_sent;
}

/* Tera Termが動作しているPC上の X サーバプログラムに対して、データを送る。
 * 一度で送れない場合は、リングバッファに格納し、遅延配送する。 
 *
 * 動作フローは下記の通り。
 * (1) 初回のデータが届く。もしくは、リングバッファが空。
 * (2) non-blockingで送信を試みる。全送信できたらreturn。
 * (3) 送信できなかったデータはリングバッファへ格納し、return。
 * (4) 次のデータが届く。
 * (5) リングバッファへ格納し、return。
 * (6) 次のデータが届く。
 * (7) リングバッファがフルになったら、バッファに残っているデータを blocking で送信を試みる。
 *     送信失敗したら、エラーreturn。
 * (8) ユーザデータ(data/len)の送信を blocking で送信を試みる。
 *     送信失敗したら、エラーreturn。 
 * (9) 送信できなかったユーザデータはリングバッファへ格納し、return。
 * (10) リングバッファに残っているデータを non-blockingで送信を試みる。
 *
 * pvar: 共有リソース
 * buf: リングバッファ
 * blocking_write: 同期型のパケット送信関数
 * socket: 非同期型のソケットハンドル
 * data: データ
 * len: データ長
 *
 * return TRUE: 送信成功
 *        FALSE: 送信エラー
 */
BOOL UTIL_sock_buffered_write(PTInstVar pvar, UTILSockWriteBuf *buf,
                              UTILBlockingWriteCallback blocking_write,
                              SOCKET socket, const char *data,
                              int len)
{
	int curlen;
	int desiredlen;
	int space_required;
	int amount_to_write_from_buffer;
	BOOL did_block = FALSE;
	int first_copy_start;
	int first_copy_amount;

	// 初回呼び出し時は、かならず下記 if 文に入る。
	/* Fast path case: buffer is empty, try nonblocking write */
	if (buf->datalen == 0) {
#if 1
		// まずは non-blocking でパケット送信する。一度でも WSAEWOULDBLOCK エラーになったら、
		// 関数は「送信済みデータ長」を返す。
		// たとえば、X サーバプログラムとして"xterm"を起動していた場合、xtermの端末内に何か
		// 文字を表示し続けている状態で、端末のウィンドウをドラッグすると、関数は 0 を返してくる。
		int sent_amount = send_until_block(pvar, socket, data, len);

		if (sent_amount < 0) {
			return FALSE;
		}
		data += sent_amount;
		len -= sent_amount;
#else
		// ノンブロッキングモードで送れなかった場合、以降の処理へ続くが、バグがあるため、
		// まともに動かない。ゆえに、初回でブロッキングモードを使い、確実に送信してしまう。
		// ポート転送(local-to-remote)において、でかいパケットを受信できない問題への対処。
		// (2007.11.29 yutaka)
		// オリジナルコードにバグがあると思っていたが、SSH遅延送信処理に問題があったため、
		// それが根本原因であり、オリジナルコードには問題がなかったと考える。
		// 本来はノンブロッキングで扱うべきところを、無理矢理ブロッキングにすることにより、
		// Tera Termが「応答なし」、Xmingが「CPUストール」という不可思議な現象が出てしまう
		// ように見える。そのため、本来のコードに戻すことを決断する。
		// (2012.10.14 yutaka)
		if (!blocking_write(pvar, socket, data, len)) {
			return FALSE;
		} else {
			len = 0;
		}
#endif
	}

	// 初回呼び出し時の non-blocking 送信で、すべて送り切れたら、即座に成功で返る。
	if (len == 0) {
		return TRUE;
	}

	// リングバッファ(buf)に残存しているデータと、新規送信データを足して(desiredlen)、
	// 現在のバッファ長(curlen)が足りるかを計算する。
	//
	// (1)データが先頭に格納されているケース
	//
	//                 <----- buflen ------------->
	// buf->bufdata -> +--------------------------+
	//                 |XXXXXXX                   |
	//                 +--------------------------+ 
	//                 <------>
	//                   buf->datalen
	//                 ^
	//                 |
	//                 buf->datastart
	//
	//
	// (2)データが両端に格納されているケース
	//
	//                 <----- buflen ------------->
	// buf->bufdata -> +--------------------------+
	//                 |XXXX                  XXXX|
	//                 +--------------------------+ 
	//                 <--->                  <-->
	//                  (a)                    (b)
	//                      (a)+(b) = buf->datalen
	//                                        ^
	//                                        |
	//                                       buf->datastart
	//
	/* We blocked or the buffer has data in it. We need to put this data
	   into the buffer.
	   First, expand buffer as much as possible and necessary. */
	curlen = buf->buflen;
	desiredlen =
		min(pvar->session_settings.WriteBufferSize, buf->datalen + len);

	if (curlen < desiredlen) {
		int wrap_amount = buf->datastart + buf->datalen - curlen;
		int newlen =
			min(pvar->session_settings.WriteBufferSize, 2 * desiredlen);

		buf->bufdata = realloc(buf->bufdata, newlen);
		buf->buflen = newlen;

		if (wrap_amount > 0) {
			int wrap_to_copy = min(wrap_amount, newlen - curlen);

			memmove(buf->bufdata + curlen, buf->bufdata, wrap_to_copy);
			memmove(buf->bufdata, buf->bufdata + wrap_to_copy,
					wrap_amount - wrap_to_copy);
		}
	}

	/* 1) Write data from buffer
	   2) Write data from user
	   3) Copy remaining user data into buffer
	 */
	// バッファがいっぱいになり、新しいデータ(data)が溢れる場合に space_required が正となる。
	// すなわち、space_requiredは「溢れた分」を表す。
	//
	//                                               space_required
	//                 <----- buflen -------------><-------->
	// buf->bufdata -> +--------------------------+
	//                 |XXXXXXXXXXXXXXXXX         |
	//                 +--------------------------+ 
	//                 <----------------><------------------>
	//                   buf->datalen            len
	//                 ^
	//                 |
	//                 buf->datastart
	space_required = max(0, buf->datalen + len - buf->buflen);
	amount_to_write_from_buffer = min(buf->datalen, space_required);

	if (amount_to_write_from_buffer > 0) {
		int first_part =
			min(amount_to_write_from_buffer, buf->buflen - buf->datastart);

		did_block = TRUE;
		if (!blocking_write
			(pvar, socket, buf->bufdata + buf->datastart, first_part)) {
			return FALSE;
		}
		if (first_part < amount_to_write_from_buffer) {
			if (!blocking_write
				(pvar, socket, buf->bufdata,
				 amount_to_write_from_buffer - first_part)) {
				return FALSE;
			}
		}

		buf->datalen -= amount_to_write_from_buffer;
		if (buf->datalen == 0) {
			buf->datastart = 0;
		} else {
			buf->datastart =
				(buf->datastart +
				 amount_to_write_from_buffer) % buf->buflen;
		}
		space_required -= amount_to_write_from_buffer;
	}

	if (space_required > 0) {
		did_block = TRUE;
		if (!blocking_write(pvar, socket, data, space_required)) {
			return FALSE;
		}
		data += space_required;
		len -= space_required;
	}

	first_copy_start = (buf->datastart + buf->datalen) % buf->buflen;
	first_copy_amount = min(len, buf->buflen - first_copy_start);
	memcpy(buf->bufdata + first_copy_start, data, first_copy_amount);
	if (first_copy_amount < len) {
		memcpy(buf->bufdata, data + first_copy_amount,
			   len - first_copy_amount);
	}
	buf->datalen += len;

	if (did_block) {
		return UTIL_sock_write_more(pvar, buf, socket);
	} else {
		return TRUE;
	}
}

BOOL UTIL_sock_write_more(PTInstVar pvar, UTILSockWriteBuf *buf,
						  SOCKET socket)
{
	int first_amount = min(buf->buflen - buf->datastart, buf->datalen);
	int sent =
		send_until_block(pvar, socket, buf->bufdata + buf->datastart,
						 first_amount);

	if (sent < 0) {
		return FALSE;
	} else {
		if (sent == first_amount && first_amount < buf->datalen) {
			int sentmore =
				send_until_block(pvar, socket, buf->bufdata,
								 buf->datalen - first_amount);

			if (sentmore < 0) {
				return FALSE;
			}
			sent += sentmore;
		}

		buf->datalen -= sent;
		if (buf->datalen == 0) {
			buf->datastart = 0;
		} else {
			buf->datastart = (buf->datastart + sent) % buf->buflen;
		}
	}

	return TRUE;
}

void UTIL_destroy_sock_write_buf(UTILSockWriteBuf *buf)
{
	SecureZeroMemory(buf->bufdata, buf->buflen);
	buf_destroy(&buf->bufdata, &buf->buflen);
}

BOOL UTIL_is_sock_deeply_buffered(UTILSockWriteBuf *buf)
{
	return buf->buflen / 2 < buf->datalen;
}

void UTIL_get_lang_msg(const char *key, PTInstVar pvar, const char *def)
{
	wchar_t *defW = ToWcharA(def);
	wchar_t *strW;
	GetI18nStrWW("TTSSH", key, defW, pvar->ts->UILanguageFileW, &strW);
	WideCharToACP_t(strW, pvar->UIMsg, sizeof(pvar->UIMsg));
	free(strW);
	free(defW);
}

void UTIL_get_lang_msgU8(const char *key, PTInstVar pvar, const char *def)
{
	const wchar_t *UILanguageFileW = pvar->ts->UILanguageFileW;
	char *strU8;
	GetI18nStrU8W("TTSSH", key, def, UILanguageFileW, &strU8);
	strcpy_s(pvar->UIMsg, MAX_UIMSG, strU8);
	free(strU8);
}

void UTIL_get_lang_msgW(const char *key, PTInstVar pvar, const wchar_t *def, wchar_t *UIMsg)
{
	const wchar_t *UILanguageFileW = pvar->ts->UILanguageFileW;
	wchar_t *strW;
	GetI18nStrWW("TTSSH", key, def, UILanguageFileW, &strW);
	wcscpy_s(UIMsg, MAX_UIMSG, strW);
	free(strW);
}

/*
 *	等幅フォントを取得
 *	@retval		フォントハンドル
 *	@retval		NULL(エラー)
 */
HFONT UTIL_get_lang_fixedfont(HWND hWnd, const wchar_t *UILanguageFile)
{
	HFONT hFont;
	LOGFONTW logfont;
	int dpi = GetMonitorDpiFromWindow(hWnd);
	BOOL result = GetI18nLogfontW(L"TTSSH", L"DLG_ABOUT_FONT", &logfont,
								 dpi, UILanguageFile);
	if (result == FALSE) {
		// 読み込めなかった場合は等幅フォントを指定する。
		// エディットコントロールはダイアログと同じフォントを持っており
		// 等幅フォントではないため。

		// ウィンドウ(ダイアログ)のフォントを取得、フォント高を参照する
		HFONT hFontDlg;
		LOGFONTW logfontDlg;
		hFontDlg = (HFONT)SendMessage(hWnd, WM_GETFONT, (WPARAM)0, (LPARAM)0);
		GetObject(hFontDlg, sizeof(logfontDlg), &logfontDlg);

		memset(&logfont, 0, sizeof(logfont));
		wcsncpy_s(logfont.lfFaceName, _countof(logfont.lfFaceName), L"Courier New", _TRUNCATE);
		logfont.lfCharSet = ANSI_CHARSET;	// = 0
		logfont.lfHeight = logfontDlg.lfHeight;
		logfont.lfWidth = 0;
	}
	hFont = CreateFontIndirectW(&logfont);	// エラー時 NULL
#if 1
	if (hFont == NULL) {
		// フォントが生成できなかった場合 stock object を使用する
		// DeleteObject() してもokのはず
		// It is not necessary (but it is not harmful) to
		// delete stock objects by calling DeleteObject.
		hFont = GetStockObject(ANSI_FIXED_FONT);
	}
#endif
	return hFont;
}
