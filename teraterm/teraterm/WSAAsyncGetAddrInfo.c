/*
 * Copyright (C) 2010- TeraTerm Project
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
 * WSAAsyncGetAddrInfo.c -- asynchronous version of getaddrinfo
 * Copyright(C) 2000-2003 Jun-ya Kato <kato@win6.jp>
 */
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <wspiapi.h>
#include <windows.h>
#include <process.h>
#include "WSAAsyncGetAddrInfo.h"
#include "ttwsk.h"
#include "codeconv.h"

typedef int (WINAPI *TIdnToAscii)(DWORD dwFlags, LPCWSTR lpUnicodeCharStr, int cchUnicodeChar,
                                  LPWSTR lpASCIICharStr, int cchASCIIChar);
static TIdnToAscii pIdnToAscii;
static BOOL Initialized = FALSE;

struct getaddrinfo_args {
	HWND hWnd;
	unsigned int wMsg;
	wchar_t *hostname;
	wchar_t *portname;
	struct addrinfo hints;
	struct addrinfo **res;
	HANDLE handle;
};

static unsigned __stdcall getaddrinfo_thread(void * p)
{
	int gai;
	struct getaddrinfo_args *ga = (struct getaddrinfo_args *)p;

	// ホスト名に非 ASCII 文字が含まれる?
	BOOL is_non_ascii = FALSE;
	for (const wchar_t *s = ga->hostname; *s != 0; s++) {
		if (*s >= 0x80) {
			is_non_ascii = TRUE;
			break;
		}
	}

	// 国際化ドメイン名(IDN)の解決
	// IdnToAscii() で ACE 形式 (xn--) へ変換
	// 変換後は ASCII なので getaddrinfo() で解決できる。
	const wchar_t *hostname = ga->hostname;
	wchar_t ace_host[256];
	if (pIdnToAscii != NULL && is_non_ascii) {
		if (pIdnToAscii(0, ga->hostname, -1, ace_host, _countof(ace_host)) > 0) {
			// 変換できた時は ACE形式から解決
			hostname = ace_host;
		}
	}

	// ACE 形式へ変換済みなら、全文字 ASCII なので変換可能
	char *hostnameA = ToCharW(hostname);
	char *portnameA = ToCharW(ga->portname);
	gai = getaddrinfo(hostnameA, portnameA, &ga->hints, ga->res);
	free(hostnameA);
	free(portnameA);

	/* send value of gai as message to window hWnd */
	PostMessage(ga->hWnd, ga->wMsg, (WPARAM)ga->handle, MAKELPARAM(0, gai));

	free(ga->hostname);
	free(ga->portname);
	free(p);

	return 0;
}

/**
 *	ホスト名解決
 *
 *	Internationalized Domain Name(IDN、国際化ドメイン名)
 *
 *	@param	hWnd		通知するウィンドウ
 *	@param	wMsg		通知するメッセージ
 *	@param	hostname	ホスト名
 *	@param	portname	ポート名
 *	@param	hints
 *	@param	res
 *
 */
HANDLE PASCAL WSAAsyncGetAddrInfoW(
	HWND hWnd, unsigned int wMsg,
	const wchar_t *hostname,
	const wchar_t *portname,
	struct addrinfo *hints,
	struct addrinfo **res)
{
	HANDLE thread;
	unsigned tid;
	struct getaddrinfo_args * ga;

	if (Initialized == FALSE) {
		Initialized = TRUE;
		HMODULE normaliz = LoadLibraryA("Normaliz.dll");
		if (normaliz != NULL) {
			// XP+IE7,Vista以降
			pIdnToAscii = (TIdnToAscii)GetProcAddress(normaliz, "IdnToAscii");
		}
	}

	if (hostname == NULL || portname == NULL) {
		return NULL;
	}

	/*
	* allocate structure to pass args to sub-thread dynamically
	* WSAAsyncGetAddrInfo() is reentrant
	*/
	if ((ga = (struct getaddrinfo_args *)malloc(sizeof(struct getaddrinfo_args))) == NULL) {
		return NULL;
	}

	/* packing arguments struct addrinfo_args */
	ga->hWnd = hWnd;
	ga->wMsg = wMsg;
	ga->hostname = _wcsdup(hostname);
	ga->portname = _wcsdup(portname);
	ga->hints = *hints;
	ga->res = res;

	/* create sub-thread running getaddrinfo() */
	thread = (HANDLE)_beginthreadex(NULL, 0, getaddrinfo_thread, ga, CREATE_SUSPENDED, &tid);
	if (thread == 0) {
		free(ga->hostname);
		free(ga->portname);
		free(ga);
		return NULL;	// return error
	}

	/* return thread handle */
	ga->handle = thread;
	ResumeThread(thread);
	return thread;
}

/*
 *	ホスト名解決 ANSI版
 */
HANDLE PASCAL WSAAsyncGetAddrInfo(
	HWND hWnd, unsigned int wMsg,
	const char *hostname,
	const char *portname,
	struct addrinfo *hints,
	struct addrinfo **res)
{
	wchar_t *hostnameW = ToWcharA(hostname);
	wchar_t *portnameW = ToWcharA(portname);
	HANDLE h = WSAAsyncGetAddrInfoW(hWnd, wMsg, hostnameW, portnameW, hints, res);
	free(hostnameW);
	free(portnameW);
	return h;
}
