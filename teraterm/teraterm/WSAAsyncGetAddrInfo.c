/*
 * WSAAsyncGetAddrInfo.c -- asynchronous version of getaddrinfo
 * Copyright(C) 2000-2003 Jun-ya Kato <kato@win6.jp>
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include "WSAASyncGetAddrInfo.h"

static unsigned __stdcall getaddrinfo_thread(void FAR * p);

HANDLE WSAAsyncGetAddrInfo(HWND hWnd, unsigned int wMsg,
                           const char FAR * hostname,
                           const char FAR * portname,
                           struct addrinfo FAR * hints,
                           struct addrinfo FAR * FAR * res)
{
	HANDLE thread;
	unsigned tid;
	struct getaddrinfo_args FAR * ga;

	/*
	* allocate structure to pass args to sub-thread dynamically
	* WSAAsyncGetAddrInfo() is reentrant
	*/
	if ((ga = (struct getaddrinfo_args FAR *)malloc(sizeof(struct getaddrinfo_args))) == NULL)
		return NULL;

	/* packing arguments struct addrinfo_args */
	ga->hWnd = hWnd;
	ga->wMsg = wMsg;
	ga->hostname = hostname;
	ga->portname = portname;
	ga->hints = hints;
	ga->res = res;

	ga->lpHandle = (HANDLE FAR *)malloc(sizeof(HANDLE));
	if (ga->lpHandle == NULL)
		return NULL;

	/* create sub-thread running getaddrinfo() */
	thread = (HANDLE)_beginthreadex(NULL, 0, getaddrinfo_thread, ga, CREATE_SUSPENDED, &tid);
	*ga->lpHandle = (HANDLE)thread;
	ResumeThread(thread);

	/* return thread handle */
	if (thread == 0) {
		free(ga->lpHandle);
		free(ga);
		return NULL;
	} else
		return (HANDLE)thread;
}

static unsigned __stdcall getaddrinfo_thread(void FAR * p)
{
	int gai;
	HWND hWnd;
	unsigned int wMsg;
	const char FAR * hostname;
	const char FAR * portname;
	struct addrinfo FAR * hints;
	struct addrinfo FAR * FAR * res;
	struct getaddrinfo_args FAR * ga;

	/* unpacking arguments */
	ga = (struct getaddrinfo_args FAR *)p;
	hWnd = ga->hWnd;
	wMsg = ga->wMsg;
	hostname = ga->hostname;
	portname = ga->portname;
	hints = ga->hints;
	res = ga->res;

	/* call getaddrinfo */
	gai = getaddrinfo(hostname, portname, hints, res);

	/* send value of gai as message to window hWnd */
	PostMessage(hWnd, wMsg, (WPARAM)*ga->lpHandle, MAKELPARAM(0, gai));

	free(ga->lpHandle);
	free(p);

	return 0;
}
