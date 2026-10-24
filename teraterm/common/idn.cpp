/*
 * (C) 2026- TeraTerm Project
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

/* Internationalized Domain Name (IDN, 国際化ドメイン名) */

#include <windows.h>
#include <stdlib.h>
#include <wchar.h>

#include "win32helper.h"

#include "idn.h"

/**
 *	ホスト名を名前解決やサーバへの送信に使える ASCII に変換する
 *
 *	国際化ドメイン名(IDN)など非 ASCII 文字を含むときは
 *	IdnToAscii() で ACE 形式 (xn--) へ変換する
 *	ASCII のみのとき、変換できないときは元のホスト名をそのまま返す
 *
 *	@param	hostname	ホスト名
 *	@return	ASCII のホスト名、不要になったら free() すること
 *			hostname が NULL のときは NULL
 */
wchar_t *IdnHostNameToAscii(const wchar_t *hostname)
{
	if (hostname == NULL) {
		return NULL;
	}

	BOOL is_non_ascii = FALSE;
	for (const wchar_t *s = hostname; *s != 0; s++) {
		if (*s >= 0x80) {
			is_non_ascii = TRUE;
			break;
		}
	}

	if (is_non_ascii) {
		wchar_t *ace;
		if (hIdnToAscii(hostname, &ace) == NO_ERROR) {
			return ace;
		}
		// 変換できなかったときは変換せずそのまま返す
	}
	return _wcsdup(hostname);
}
