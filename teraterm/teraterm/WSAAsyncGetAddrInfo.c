/*
 * Copyright (C) 2010-2019 TeraTerm Project
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
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <windows.h>
#include <process.h>
#include "WSAAsyncGetAddrInfo.h"
#include "ttwsk.h"

static unsigned __stdcall getaddrinfo_thread(void * p);

HANDLE PASCAL WSAAsyncGetAddrInfo(HWND hWnd, unsigned int wMsg,
                           const char *hostname,
                           const char *portname,
                           struct addrinfo *hints,
                           struct addrinfo **res)
{
	HANDLE thread;
	unsigned tid;
	struct getaddrinfo_args * ga;

	/*
	* allocate structure to pass args to sub-thread dynamically
	* WSAAsyncGetAddrInfo() is reentrant
	*/
	if ((ga = (struct getaddrinfo_args *)malloc(sizeof(struct getaddrinfo_args))) == NULL)
		return NULL;

	/* packing arguments struct addrinfo_args */
	ga->hWnd = hWnd;
	ga->wMsg = wMsg;
	ga->hostname = _strdup(hostname); // ポインタだけ渡すと、スレッド先で不定となる。(2012.11.7 yutaka)
	ga->portname = _strdup(portname);
	ga->hints = *hints; // ポインタだけ渡すと、スレッド先で不定となる。(2016.3.11 yutaka)
	ga->res = res;

	ga->lpHandle = (HANDLE *)malloc(sizeof(HANDLE));
	if (ga->lpHandle == NULL) {
		free(ga->hostname);
		free(ga->portname);
		return NULL;
	}

	/* create sub-thread running getaddrinfo() */
	thread = (HANDLE)_beginthreadex(NULL, 0, getaddrinfo_thread, ga, CREATE_SUSPENDED, &tid);
	*ga->lpHandle = (HANDLE)thread;
	ResumeThread(thread);

	/* return thread handle */
	if (thread == 0) {
		free(ga->lpHandle);
		free(ga->hostname);
		free(ga->portname);
		free(ga);
		return NULL;
	} else
		return (HANDLE)thread;
}

static unsigned __stdcall getaddrinfo_thread(void * p)
{
	int gai;
	HWND hWnd;
	unsigned int wMsg;
	const char *hostname;
	const char *portname;
	struct addrinfo *hints;
	struct addrinfo **res;
	struct getaddrinfo_args *ga;

	/* unpacking arguments */
	ga = (struct getaddrinfo_args *)p;
	hWnd = ga->hWnd;
	wMsg = ga->wMsg;
	hostname = ga->hostname;
	portname = ga->portname;
	hints = &ga->hints;
	res = ga->res;

	/* call getaddrinfo */
//	gai = Pgetaddrinfo(hostname, portname, hints, res);
	gai = getaddrinfo(hostname, portname, hints, res);

	/* send value of gai as message to window hWnd */
	PostMessage(hWnd, wMsg, (WPARAM)*ga->lpHandle, MAKELPARAM(0, gai));

	free(ga->lpHandle);
	free(ga->hostname);
	free(ga->portname);
	free(p);

	return 0;
}
