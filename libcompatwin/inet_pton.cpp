/*
 * Copyright (C) 2026- TeraTerm Project
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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>

/**
 * inet_pton
 * af:	 AF_INET (IPv4) または AF_INET6 (IPv6)
 * src:	 IPアドレス文字列
 * dst:	 バイナリ形式のアドレスを格納する構造体 (in_addr または in6_addr)
 */
extern "C" int WSAAPI _inet_pton(int af, const char* src, void* dst)
{
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN + 1];

	// 引数チェック
	if (src == NULL || dst == NULL) return 0;

	// WSAStringToAddress は非 const の文字列を要求する場合があるためコピー
	strncpy(src_copy, src, INET6_ADDRSTRLEN);
	src_copy[INET6_ADDRSTRLEN] = '\0';

	// アドレス情報を格納する構造体を初期化
	memset(&ss, 0, sizeof(ss));
	ss.ss_family = af;

	// 文字列からバイナリ形式のアドレスに変換
	// 成功すれば 0 が返る
	int result = WSAStringToAddressA(
		src_copy,			// アドレス文字列
		af,					// アドレスファミリー
		NULL,				// プロトコル情報 (通常NULL)
		(LPSOCKADDR)&ss,	// 結果を受け取る構造体
		&size				// 構造体のサイズ
		);
	if (result == 0) {
		// エラー
		return 0;
	}

	// 成功時、適切な構造体部分をコピー
	if (af == AF_INET) {
		memcpy(dst, &(((struct sockaddr_in*)&ss)->sin_addr), sizeof(struct in_addr));
	} else if (af == AF_INET6) {
		memcpy(dst, &(((struct sockaddr_in6*)&ss)->sin6_addr), sizeof(struct in6_addr));
	}
	return 1;
}
