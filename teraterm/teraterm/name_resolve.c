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

/* NameResolve: 非同期名前解決 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "tt_res.h"		// ID_FILE_EXIT
#include "ttwsk.h"
#include "asprintf.h"
#include "codeconv.h"
#include "WSAAsyncGetAddrInfo.h"
#include "name_resolve.h"

struct name_resolve_st {
	HWND notify_wnd;     // 完了通知先ウィンドウ
	UINT notify_msg;     // 完了通知メッセージ
	HANDLE task;         // PWSAAsyncGetAddrInfo() のタスクハンドル
};

DWORD NameResolveStart(name_resolve_t **nr, HWND notify_wnd, UINT notify_msg,
                       const wchar_t *host, int port,
                       int protocol_family, int socktype,
                       struct addrinfo **res)
{
	name_resolve_t *r;
	struct addrinfo hints;

	if (nr == NULL || host == NULL || res == NULL) {
		return ERROR_INVALID_PARAMETER;
	}
	*nr = NULL;

	r = (name_resolve_t *)calloc(1, sizeof(*r));
	if (r == NULL) {
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	r->notify_wnd = notify_wnd;
	r->notify_msg = notify_msg;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = protocol_family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = (socktype == SOCK_DGRAM) ? IPPROTO_UDP : IPPROTO_TCP;
#if 0
	wchar_t *pname;
	aswprintf(&pname, L"%d", port);
	r->task = WSAAsyncGetAddrInfoW(notify_wnd, notify_msg, host, pname, &hints, res);
#else
	char *pname;
	asprintf(&pname, "%d", port);
	char *hostA = ToCharW(host);
	r->task = PWSAAsyncGetAddrInfo(notify_wnd, notify_msg, hostA, pname, &hints, res);
	free(hostA);
#endif
	free(pname);
	if (r->task == 0) {
		free(r);
		return ERROR_GEN_FAILURE;	// 解決を開始できなかった
	}
	*nr = r;
	return ERROR_SUCCESS;
}

void NameResolveCancel(name_resolve_t **nr)
{
	name_resolve_t *r;

	if (nr == NULL || *nr == NULL) {
		return;
	}
	r = *nr;
	if (r->task != 0) {
		PWSACancelAsyncRequest(r->task);
		CloseHandle(r->task);
		r->task = 0;
	}
	free(r);
	*nr = NULL;
}

name_resolve_result_t NameResolveWaitPump(name_resolve_t **nr)
{
	name_resolve_t *r = *nr;
	MSG Msg;

	do {
		if (GetMessage(&Msg, 0, 0, 0)) {
			if ((Msg.hwnd == r->notify_wnd) &&
			    (((Msg.message == WM_SYSCOMMAND) && ((Msg.wParam & 0xfff0) == SC_CLOSE)) ||
			     ((Msg.message == WM_COMMAND) && (LOWORD(Msg.wParam) == ID_FILE_EXIT)) ||
			     (Msg.message == WM_CLOSE))) { /* Exit when the user closes Tera Term */
				HWND wnd = r->notify_wnd;
				NameResolveCancel(nr);
				PostMessage(wnd, Msg.message, Msg.wParam, Msg.lParam);
				return NAME_RESOLVE_ABORT;
			}
			if (Msg.message != r->notify_msg) { /* Prosess messages */
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
		else {
			NameResolveCancel(nr);
			return NAME_RESOLVE_QUIT;
		}
	} while (Msg.message != r->notify_msg);

	CloseHandle(r->task);
	r->task = 0;
	free(r);
	*nr = NULL;
	return (WSAGETASYNCERROR(Msg.lParam) == 0) ? NAME_RESOLVE_OK : NAME_RESOLVE_ERROR;
}
