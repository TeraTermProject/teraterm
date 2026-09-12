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

/*
 *	NameResolve: 非同期名前解決
 *
 *	CommOpen() に埋め込まれていた名前解決(非同期 getaddrinfo +
 *	完了待ちのメッセージポンプ + キャンセル)を切り出したもの。
 *	内部でフック済みの PWSAAsyncGetAddrInfo()(TTPLUG フック対応。
 *	TTProxy がプロキシ経由の解決に差し替え得る)を呼ぶため、
 *	完了通知は「HWND + ウィンドウメッセージ」形式を維持している。
 *
 *	socktype を SOCK_DGRAM にすれば UDP(将来の Mosh 等)でも使える。
 */

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct name_resolve_st name_resolve_t;

/**
 *	非同期名前解決を開始する
 *
 *	完了すると notify_wnd へ notify_msg が post される。
 *	結果 *res は呼び出し側所有(Pfreeaddrinfo() で解放すること)。
 *
 *	@param[out]	nr				生成したインスタンス(失敗時は NULL)
 *	@param[in]	notify_wnd		完了通知先ウィンドウ
 *	@param[in]	notify_msg		完了通知メッセージ(WM_USER_GETHOST 等)
 *	@param[in]	host			ホスト名
 *	@param[in]	port			ポート番号
 *	@param[in]	protocol_family	AF_UNSPEC / AF_INET / AF_INET6
 *	@param[in]	socktype		SOCK_STREAM(TCP)/ SOCK_DGRAM(UDP)
 *	@param[out]	res				解決結果(addrinfo リスト)の格納先
 */
DWORD NameResolveStart(name_resolve_t **nr, HWND notify_wnd, UINT notify_msg,
                       const wchar_t *host, int port,
                       int protocol_family, int socktype,
                       struct addrinfo **res);

/* NameResolveWaitPump() の結果 */
typedef enum {
	NAME_RESOLVE_OK,     /* 解決成功(*res 有効) */
	NAME_RESOLVE_ERROR,  /* 解決失敗 */
	NAME_RESOLVE_ABORT,  /* ユーザーの閉じる操作。そのメッセージは再投函済み。
	                        呼び出し側は即 return すること */
	NAME_RESOLVE_QUIT,   /* WM_QUIT を受けた。呼び出し側は即 return すること */
} name_resolve_result_t;

/**
 *	メッセージポンプしながら完了を待つ(GUI スレッド用)
 *
 *	notify_msg 以外のメッセージは通常どおり処理し(ウィンドウが固まらない)、
 *	notify_wnd 宛の閉じる操作(WM_SYSCOMMAND+SC_CLOSE / WM_COMMAND+
 *	ID_FILE_EXIT / WM_CLOSE)を検出したらキャンセルして中断する。
 *	返るとき *nr は解放済み(NULL)。
 */
name_resolve_result_t NameResolveWaitPump(name_resolve_t **nr);

/* キャンセルして破棄する。*nr は NULL になる。*nr == NULL なら何もしない */
void NameResolveCancel(name_resolve_t **nr);

#ifdef __cplusplus
}
#endif
